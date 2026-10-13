/**
 * @file tiny_ttf_profile.c
 * @brief Standalone Simulator TinyTTF resource and cache profile.
 *
 * Build against the Simulator's LVGL static library. This program does not
 * initialize ElenixOS or change the production font system.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __APPLE__
#include <mach/mach.h>
#endif

#include "lvgl.h"

#define FONT_PATH "A:fs/.sys/res/font/font.ttf"
#define WARM_REPEATS 200
#define STRESS_REPEATS 10
#define REDUCED_CACHE_SIZE 64

typedef struct
{
    size_t lv_bytes;
    size_t lv_blocks;
    int open_fds;
    size_t rss_bytes;
} snapshot_t;

static volatile uint64_t glyph_sink;

/* The Simulator's LVGL assert hook expects EOS; keep this harness standalone. */
void eos_dump(const char *reason)
{
    fprintf(stderr, "LVGL assertion: %s\n", reason ? reason : "unknown");
    abort();
}

static const uint32_t chinese[] = {
    0x4e2d, 0x56fd, 0x4f60, 0x597d, 0x4eca, 0x5929, 0x65f6, 0x95f4, 0x6628, 0x6c14, 0x6e29, 0x5ea6,
    0x5065, 0x5eb7, 0x6d3b, 0x52a8, 0x6d88, 0x606f, 0x97f3, 0x4e50, 0x7535, 0x91cf, 0x8bbe, 0x7f6e,
};
static const uint32_t english[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
};
static const uint32_t digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

static double now_us(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return -1.0;
    return (double)ts.tv_sec * 1000000.0 + (double)ts.tv_nsec / 1000.0;
}

static int count_open_fds(void)
{
    int count = 0;
    for (int fd = 0; fd < 1024; ++fd)
    {
        errno = 0;
        if (fcntl(fd, F_GETFD) != -1 || errno != EBADF)
            ++count;
    }
    return count;
}

static size_t current_rss(void)
{
#ifdef __APPLE__
    mach_task_basic_info_data_t info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS)
        return (size_t)info.resident_size;
#endif
    return 0;
}

static snapshot_t snapshot(void)
{
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    return (snapshot_t){mon.total_size - mon.free_size, mon.used_cnt, count_open_fds(), current_rss()};
}

static void print_snapshot(const char *phase,
                           int px,
                           const char *category,
                           snapshot_t current,
                           snapshot_t baseline,
                           double elapsed_us,
                           size_t glyphs)
{
    printf("sample,%s,%d,%s,%zu,%zu,%d,%zu,%lld,%lld,%d,%lld,%.3f,%zu\n",
           phase,
           px,
           category,
           current.lv_bytes,
           current.lv_blocks,
           current.open_fds,
           current.rss_bytes,
           (long long)current.lv_bytes - (long long)baseline.lv_bytes,
           (long long)current.lv_blocks - (long long)baseline.lv_blocks,
           current.open_fds - baseline.open_fds,
           (long long)current.rss_bytes - (long long)baseline.rss_bytes,
           elapsed_us,
           glyphs);
    fflush(stdout);
}

static size_t use_glyphs(lv_font_t *font, const uint32_t *points, size_t length)
{
    size_t found = 0;
    for (size_t i = 0; i < length; ++i)
    {
        lv_font_glyph_dsc_t dsc;
        uint32_t next = i + 1 < length ? points[i + 1] : 0;
        if (!lv_font_get_glyph_dsc(font, &dsc, points[i], next))
            continue;
        const void *bitmap = lv_font_get_glyph_bitmap(&dsc, NULL);
        if (bitmap != NULL)
        {
            glyph_sink += dsc.gid.index;
            ++found;
        }
        lv_font_glyph_release_draw_data(&dsc);
    }
    return found;
}

static size_t collect_supported_cjk(uint32_t *points, size_t capacity)
{
    lv_font_t *probe = lv_tiny_ttf_create_file(FONT_PATH, 26);
    if (probe == NULL)
        return 0;
    size_t found = 0;
    for (uint32_t cp = 0x4e00; cp <= 0x9fff && found < capacity; ++cp)
    {
        lv_font_glyph_dsc_t dsc;
        if (lv_font_get_glyph_dsc(probe, &dsc, cp, 0))
            points[found++] = cp;
    }
    lv_tiny_ttf_destroy(probe);
    return found;
}

static lv_font_t *create_profile_font(int px, size_t cache_size)
{
    if (cache_size == LV_TINY_TTF_CACHE_GLYPH_CNT)
        return lv_tiny_ttf_create_file(FONT_PATH, px);
    return lv_tiny_ttf_create_file_ex(FONT_PATH, px, LV_FONT_KERNING_NORMAL, cache_size);
}

static int profile_category(lv_font_t *font,
                            int px,
                            const char *name,
                            const uint32_t *points,
                            size_t length,
                            snapshot_t baseline)
{
    double start = now_us();
    size_t found = use_glyphs(font, points, length);
    double cold_us = now_us() - start;
    if (found != length)
    {
        fprintf(stderr, "missing glyphs: px=%d category=%s found=%zu expected=%zu\n", px, name, found, length);
        return 1;
    }
    print_snapshot("cold", px, name, snapshot(), baseline, cold_us, found);

    start = now_us();
    for (int i = 0; i < WARM_REPEATS; ++i)
        found = use_glyphs(font, points, length);
    double warm_us = now_us() - start;
    print_snapshot("warm", px, name, snapshot(), baseline, warm_us, length * WARM_REPEATS);
    return found == length ? 0 : 1;
}

int main(int argc, char **argv)
{
    static const int sizes[] = {12, 16, 18, 24, 26, 32, 48, 64};
    lv_font_t *fonts[sizeof(sizes) / sizeof(sizes[0])] = {0};
    uint32_t saturation[256];
    int saturate = argc == 2 && (strcmp(argv[1], "saturate") == 0 || strcmp(argv[1], "saturate64") == 0);
    int reduced = argc == 2 && (strcmp(argv[1], "common64") == 0 || strcmp(argv[1], "saturate64") == 0);
    size_t cache_size = reduced ? REDUCED_CACHE_SIZE : LV_TINY_TTF_CACHE_GLYPH_CNT;
    int result = 0;

    if (argc > 2 || (argc == 2 && !saturate && !reduced))
    {
        fprintf(stderr, "usage: %s [common64|saturate|saturate64]\n", argv[0]);
        return 2;
    }
    lv_init();
    if (saturate
        && collect_supported_cjk(saturation, sizeof(saturation) / sizeof(saturation[0]))
               != sizeof(saturation) / sizeof(saturation[0]))
    {
        fprintf(stderr, "fewer than 256 supported CJK glyphs found\n");
        lv_deinit();
        return 1;
    }
    printf("metadata,font_path,%s\n", FONT_PATH);
    printf("metadata,lv_mem_size,%d\n", LV_MEM_SIZE);
    printf("metadata,glyph_cache_count,%d\n", LV_TINY_TTF_CACHE_GLYPH_CNT);
    printf("metadata,instance_cache_count,%zu\n", cache_size);
    printf("metadata,kerning_cache_count,%d\n", LV_TINY_TTF_CACHE_KERNING_CNT);
    printf("metadata,warm_repeats,%d\n", WARM_REPEATS);
    printf("metadata,mode,%s\n", saturate ? "saturate" : "common");
    printf("header,phase,px,category,lv_bytes,lv_blocks,open_fds,rss_bytes,delta_lv_bytes,delta_lv_blocks,delta_fds,"
           "delta_rss_bytes,elapsed_us,glyphs\n");
    snapshot_t baseline = snapshot();
    print_snapshot("baseline", 0, "none", baseline, baseline, 0, 0);

    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i)
    {
        double start = now_us();
        fonts[i] = create_profile_font(sizes[i], cache_size);
        double create_us = now_us() - start;
        if (fonts[i] == NULL)
        {
            fprintf(stderr, "font creation failed: px=%d\n", sizes[i]);
            result = 1;
            goto cleanup;
        }
        print_snapshot("create", sizes[i], "none", snapshot(), baseline, create_us, 0);
    }

    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i)
    {
        if (saturate)
        {
            double start = now_us();
            size_t found = use_glyphs(fonts[i], saturation, sizeof(saturation) / sizeof(saturation[0]));
            double elapsed_us = now_us() - start;
            print_snapshot("saturate", sizes[i], "cjk256", snapshot(), baseline, elapsed_us, found);
            if (found != sizeof(saturation) / sizeof(saturation[0]))
                result = 1;
            start = now_us();
            for (int repeat = 0; repeat < STRESS_REPEATS; ++repeat)
                found = use_glyphs(fonts[i], saturation, sizeof(saturation) / sizeof(saturation[0]));
            elapsed_us = now_us() - start;
            print_snapshot("stress_warm", sizes[i], "cjk256", snapshot(), baseline, elapsed_us, found * STRESS_REPEATS);
            if (found != sizeof(saturation) / sizeof(saturation[0]))
                result = 1;
            continue;
        }
        if (profile_category(fonts[i], sizes[i], "chinese", chinese, sizeof(chinese) / sizeof(chinese[0]), baseline)
            || profile_category(fonts[i], sizes[i], "english", english, sizeof(english) / sizeof(english[0]), baseline)
            || profile_category(fonts[i], sizes[i], "digits", digits, sizeof(digits) / sizeof(digits[0]), baseline))
        {
            result = 1;
            goto cleanup;
        }
    }

cleanup:
    for (size_t i = sizeof(sizes) / sizeof(sizes[0]); i > 0; --i)
    {
        if (fonts[i - 1] != NULL)
            lv_tiny_ttf_destroy(fonts[i - 1]);
        print_snapshot("destroy", sizes[i - 1], "none", snapshot(), baseline, 0, 0);
    }
    if (!saturate && result == 0)
    {
        for (int cycle = 0; cycle < 20; ++cycle)
        {
            double start = now_us();
            lv_font_t *font = lv_tiny_ttf_create_file(FONT_PATH, 26);
            if (font == NULL)
            {
                result = 1;
                break;
            }
            size_t found = use_glyphs(font, chinese, sizeof(chinese) / sizeof(chinese[0]));
            found += use_glyphs(font, english, sizeof(english) / sizeof(english[0]));
            found += use_glyphs(font, digits, sizeof(digits) / sizeof(digits[0]));
            lv_tiny_ttf_destroy(font);
            double elapsed_us = now_us() - start;
            snapshot_t current = snapshot();
            print_snapshot("cycle", cycle + 1, "26px", current, baseline, elapsed_us, found);
            if (found
                    != sizeof(chinese) / sizeof(chinese[0]) + sizeof(english) / sizeof(english[0])
                           + sizeof(digits) / sizeof(digits[0])
                || current.lv_bytes != baseline.lv_bytes || current.lv_blocks != baseline.lv_blocks
                || current.open_fds != baseline.open_fds)
                result = 1;
        }
    }
    if (lv_mem_test() != LV_RESULT_OK)
        result = 1;
    snapshot_t after = snapshot();
    if (after.lv_bytes != baseline.lv_bytes || after.lv_blocks != baseline.lv_blocks
        || after.open_fds != baseline.open_fds)
        result = 1;
    printf("result,%s,glyph_sink,%llu\n", result == 0 ? "pass" : "fail", (unsigned long long)glyph_sink);
    lv_deinit();
    return result;
}
