/**
 * @file font_size_v07_smoke.c
 * @brief Standalone LVGL and ElenixOS dual-TTF font-size lifecycle smoke test.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "lvgl.h"
#include "eos_font.h"
#include "eos_image.h"
#include "eos_icon.h"
#include "eos_mem.h"

#define CHECK(condition)                                                            \
    do                                                                              \
    {                                                                               \
        if (!(condition))                                                           \
        {                                                                           \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
            return 1;                                                               \
        }                                                                           \
    } while (0)

void eos_dump(const char *reason)
{
    fprintf(stderr, "LVGL assertion: %s\n", reason ? reason : "unknown");
    abort();
}

void *eos_malloc(size_t size)
{
    return lv_malloc(size);
}

void eos_free(void *ptr)
{
    lv_free(ptr);
}

static void *(*stdio_open_cb)(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode);

static void *open_simulator_fs(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
    char real_path[512];
    if (snprintf(real_path, sizeof(real_path), "fs%s", path) >= (int)sizeof(real_path))
        return NULL;
    return stdio_open_cb(drv, real_path, mode);
}

static int count_open_fds(void)
{
    int count = 0;
    for (int fd = 0; fd < 1024; fd++)
    {
        errno = 0;
        if (fcntl(fd, F_GETFD) != -1 || errno != EBADF)
            count++;
    }
    return count;
}

static size_t lv_used_bytes(void)
{
    lv_mem_monitor_t monitor;
    lv_mem_monitor(&monitor);
    return monitor.total_size - monitor.free_size;
}

static bool _get_glyph(const lv_font_t *font, uint32_t codepoint, lv_font_glyph_dsc_t *glyph)
{
    *glyph = (lv_font_glyph_dsc_t){0};
    return lv_font_get_glyph_dsc(font, glyph, codepoint, 0);
}

static const lv_image_dsc_t *_get_image_dsc(lv_obj_t *image)
{
    return (const lv_image_dsc_t *)lv_image_get_src(image);
}

static uint32_t _first_codepoint(const char *text)
{
    const unsigned char *bytes = (const unsigned char *)text;
    if (bytes[0] < 0x80)
        return bytes[0];
    if ((bytes[0] & 0xE0) == 0xC0)
        return ((uint32_t)(bytes[0] & 0x1F) << 6) | (bytes[1] & 0x3F);
    if ((bytes[0] & 0xF0) == 0xE0)
        return ((uint32_t)(bytes[0] & 0x0F) << 12) | ((uint32_t)(bytes[1] & 0x3F) << 6) | (bytes[2] & 0x3F);
    return ((uint32_t)(bytes[0] & 0x07) << 18) | ((uint32_t)(bytes[1] & 0x3F) << 12)
           | ((uint32_t)(bytes[2] & 0x3F) << 6) | (bytes[3] & 0x3F);
}

int main(void)
{
    static lv_fs_drv_t eos_test_fs;
    const int fd_before = count_open_fds();

    lv_init();
    lv_fs_drv_t *stdio_fs = lv_fs_get_drv('A');
    CHECK(stdio_fs != NULL);
    stdio_open_cb = stdio_fs->open_cb;
    eos_test_fs = *stdio_fs;
    eos_test_fs.letter = 'Z';
    eos_test_fs.open_cb = open_simulator_fs;
    lv_fs_drv_register(&eos_test_fs);

    lv_display_t *display = lv_display_create(390, 450);
    CHECK(display != NULL);
    size_t memory_before_default_fonts = lv_used_bytes();
    lv_font_t *default_font = eos_font_init();
    CHECK(default_font != NULL);
    CHECK(count_open_fds() == fd_before + 2);
    size_t default_pair_bytes = lv_used_bytes() - memory_before_default_fonts;

    lv_obj_t *screen = lv_screen_active();
    lv_obj_t *parent = lv_obj_create(screen);
    lv_obj_t *label_a = lv_label_create(parent);
    lv_obj_t *label_b = lv_label_create(parent);
    lv_obj_t *label_c = lv_label_create(parent);
    CHECK(parent != NULL && label_a != NULL && label_b != NULL && label_c != NULL);
    size_t memory_before_size_requests = lv_used_bytes();
    int fds_before_size_requests = count_open_fds();

    const int32_t test_sizes[] = {14, 18, 24, 32, 36, 48};
    int32_t previous_text_height = 0;
    int32_t previous_icon_height = 0;
    uint32_t home_codepoint = _first_codepoint(RI_HOME_FILL);
    for (size_t i = 0; i < sizeof(test_sizes) / sizeof(test_sizes[0]); i++)
    {
        const int32_t size = test_sizes[i];
        lv_obj_t *label = (i == 0) ? label_a : (i == 1) ? label_b : (i == 2) ? label_c : lv_label_create(parent);
        CHECK(label != NULL);
        lv_label_set_text(label, RI_HOME_FILL " Home 中文");
        CHECK(eos_font_set_size(label, size) == EOS_OK);

        const lv_font_t *text_font = lv_obj_get_style_text_font(label, LV_PART_MAIN);
        CHECK(text_font != NULL);
        if (previous_text_height)
            CHECK(text_font->line_height > previous_text_height);
        previous_text_height = text_font->line_height;

        lv_font_glyph_dsc_t text_glyph;
        CHECK(_get_glyph(text_font, 'A', &text_glyph));
        CHECK(text_glyph.resolved_font == text_font && text_glyph.box_w > 0 && text_glyph.box_h > 0);
        lv_font_glyph_release_draw_data(&text_glyph);

        lv_font_glyph_dsc_t chinese_glyph;
        CHECK(_get_glyph(text_font, 0x4E2D, &chinese_glyph));
        CHECK(chinese_glyph.resolved_font == text_font && chinese_glyph.box_w > 0 && chinese_glyph.box_h > 0);
        lv_font_glyph_release_draw_data(&chinese_glyph);

        lv_font_glyph_dsc_t icon_glyph;
        CHECK(_get_glyph(text_font, home_codepoint, &icon_glyph));
        CHECK(icon_glyph.resolved_font != text_font && icon_glyph.box_w > 0 && icon_glyph.box_h > 0);
        CHECK(icon_glyph.resolved_font->line_height > previous_icon_height);
        previous_icon_height = icon_glyph.resolved_font->line_height;

        lv_font_t *icon_font = NULL;
        CHECK(eos_font_get_icon_font(size, &icon_font) == EOS_OK);
        CHECK(icon_glyph.resolved_font == icon_font);
        lv_font_glyph_release_draw_data(&icon_glyph);

        lv_obj_t *same_size_label = lv_label_create(parent);
        CHECK(same_size_label != NULL);
        CHECK(eos_font_set_size(same_size_label, size) == EOS_OK);
        CHECK(lv_obj_get_style_text_font(same_size_label, LV_PART_MAIN) == text_font);
        lv_obj_delete(same_size_label);

        lv_obj_t *icon_image = eos_icon_glyph_image_create(parent, RI_HOME_FILL, size);
        CHECK(icon_image != NULL);
        const lv_image_dsc_t *image_dsc = _get_image_dsc(icon_image);
        CHECK(image_dsc != NULL && image_dsc->data != NULL && image_dsc->data_size > 0);
        CHECK(image_dsc->header.cf == LV_COLOR_FORMAT_A8);
        CHECK(image_dsc->header.w == icon_glyph.box_w && image_dsc->header.h == icon_glyph.box_h);
        bool has_ink = false;
        for (uint32_t pixel = 0; pixel < image_dsc->data_size; pixel++)
        {
            if (image_dsc->data[pixel] != 0)
            {
                has_ink = true;
                break;
            }
        }
        CHECK(has_ink);
        lv_obj_update_layout(icon_image);
        CHECK(lv_obj_get_width(icon_image) == size && lv_obj_get_height(icon_image) == size);
        lv_obj_delete(icon_image);

        if (i >= 3)
            lv_obj_delete(label);
    }

    CHECK(eos_font_set_size(parent, 24) == EOS_OK);
    const lv_font_t *font_24 = lv_obj_get_style_text_font(parent, LV_PART_MAIN);
    CHECK(eos_font_set_size(label_a, 18) == EOS_OK);
    const lv_font_t *font_18 = lv_obj_get_style_text_font(label_a, LV_PART_MAIN);
    CHECK(eos_font_set_size(label_b, 18) == EOS_OK);
    CHECK(lv_obj_get_style_text_font(label_b, LV_PART_MAIN) == font_18);
    CHECK(font_18 != font_24);

    CHECK(eos_font_clear_size(label_a) == EOS_OK);
    CHECK(lv_obj_get_style_text_font(label_a, LV_PART_MAIN) == font_24);
    CHECK(eos_font_clear_size(label_a) == EOS_OK);
    CHECK(eos_font_set_size(label_a, 18) == EOS_OK);
    CHECK(lv_obj_get_style_text_font(label_a, LV_PART_MAIN) == font_18);

    eos_label_set_font_size(label_c, EOS_FONT_SIZE_LARGE);
    const lv_font_t *font_30 = lv_obj_get_style_text_font(label_c, LV_PART_MAIN);
    CHECK(font_30 != NULL && font_30->line_height > font_24->line_height);
    CHECK(eos_font_set_size(label_a, 65) == EOS_ERR_INVALID_ARG);
    CHECK(lv_obj_get_style_text_font(label_a, LV_PART_MAIN) == font_18);

    /* Existing LVGL FontAwesome symbols continue through the terminal rescue font. */
    lv_font_glyph_dsc_t lv_symbol;
    CHECK(_get_glyph(font_18, 0xF001, &lv_symbol));
    CHECK(lv_symbol.resolved_font != font_18 && lv_symbol.box_w > 0);
    lv_font_glyph_release_draw_data(&lv_symbol);

    size_t memory_after_image_warmup = lv_used_bytes();
    int fds_after_image_warmup = count_open_fds();
    for (int i = 0; i < 100; i++)
    {
        lv_obj_t *icon_image = eos_icon_glyph_image_create(parent, RI_HOME_FILL, 48);
        CHECK(icon_image != NULL);
        lv_obj_delete(icon_image);
    }
    CHECK(lv_used_bytes() == memory_after_image_warmup);
    CHECK(count_open_fds() == fds_after_image_warmup);

    for (int32_t size = EOS_FONT_MIN_SIZE_PX; size <= EOS_FONT_MAX_SIZE_PX; size++)
    {
        CHECK(eos_font_set_size(label_c, size) == EOS_OK);
        const lv_font_t *current_font = lv_obj_get_style_text_font(label_c, LV_PART_MAIN);
        CHECK(current_font != NULL && current_font->line_height > 0);
    }
    const int32_t supported_size_count = EOS_FONT_MAX_SIZE_PX - EOS_FONT_MIN_SIZE_PX + 1;
    CHECK(count_open_fds() == fd_before + 2 * supported_size_count);
    size_t size_request_bytes = lv_used_bytes() - memory_before_size_requests;
    int size_request_fds = count_open_fds() - fds_before_size_requests;
    CHECK(size_request_fds == 2 * (supported_size_count - 1));

    size_t memory_before_repeat = lv_used_bytes();
    int fds_before_repeat = count_open_fds();
    for (int i = 0; i < 1000; i++)
        CHECK(eos_font_set_size(label_b, 18) == EOS_OK);
    CHECK(lv_used_bytes() == memory_before_repeat);
    CHECK(count_open_fds() == fds_before_repeat);

    CHECK(eos_font_reload("ignored.ttf") == NULL);
    CHECK(lv_obj_get_style_text_font(label_b, LV_PART_MAIN) == font_18);
    eos_font_deinit();
    CHECK(count_open_fds() == fds_before_repeat);

    lv_obj_delete(parent);
    CHECK(count_open_fds() == fds_before_repeat);
    lv_obj_t *new_label = lv_label_create(screen);
    CHECK(new_label != NULL);
    CHECK(eos_font_set_size(new_label, 18) == EOS_OK);
    CHECK(lv_obj_get_style_text_font(new_label, LV_PART_MAIN) == font_18);
    lv_display_delete(display);

    eos_font_deinit();
    CHECK(count_open_fds() == fds_before_repeat);
    lv_theme_default_deinit();
    eos_font_deinit();
    CHECK(count_open_fds() == fd_before);
    printf("PASS text+icon=14,18,24,32,48 fallback=matching-size images=dynamic "
           "size_range=%d-%d repeat=1000 default_pair_bytes=%zu additional_instance_bytes=%zu font_fds=%d "
           "lv_bytes=%zu\n",
           EOS_FONT_MIN_SIZE_PX,
           EOS_FONT_MAX_SIZE_PX,
           default_pair_bytes,
           size_request_bytes,
           fds_before_repeat - fd_before,
           memory_before_repeat);
    lv_deinit();
    return 0;
}
