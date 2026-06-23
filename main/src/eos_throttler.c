#define _DEFAULT_SOURCE
#include "eos_throttler.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define HOST_REFERENCE_FREQ_MHZ 3000U
#define FPU_PENALTY    1.5f
#define ICACHE_PENALTY 1.15f
#define DCACHE_PENALTY 1.1f

static eos_throttler_config_t g_config;
static eos_throttler_stats_t g_stats;
static bool g_master_enabled = false;

static uint64_t g_pending_flash_read_bytes;
static uint64_t g_pending_flash_write_bytes;
static uint32_t g_total_ram_allocated_kb;
static uint32_t g_peak_ram_kb;
static uint32_t g_ram_alloc_failures;

static void _reset_stats(void)
{
    memset(&g_stats, 0, sizeof(g_stats));
}

static void _apply_preset_m0(void)
{
    g_config.cpu_limit_enabled = true;
    g_config.cpu_freq_mhz = 48;
    g_config.fpu_enabled = false;
    g_config.icache_enabled = false;
    g_config.dcache_enabled = false;
    g_config.cache_line_size = 0;

    g_config.flash_limit_enabled = true;
    g_config.flash_read_speed_kbps = 1024;
    g_config.flash_write_speed_kbps = 256;
    g_config.flash_erase_sector_ms = 100;


    g_config.memory_limit_enabled = true;
    g_config.max_ram_kb = 256;
    g_config.max_stack_kb = 16;

    g_config.system_limit_enabled = true;
    g_config.tick_rate_hz = 100;
}

static void _apply_preset_m3(void)
{
    g_config.cpu_limit_enabled = true;
    g_config.cpu_freq_mhz = 72;
    g_config.fpu_enabled = false;
    g_config.icache_enabled = false;
    g_config.dcache_enabled = false;
    g_config.cache_line_size = 0;

    g_config.flash_limit_enabled = true;
    g_config.flash_read_speed_kbps = 2048;
    g_config.flash_write_speed_kbps = 512;
    g_config.flash_erase_sector_ms = 80;


    g_config.memory_limit_enabled = true;
    g_config.max_ram_kb = 256;
    g_config.max_stack_kb = 8;

    g_config.system_limit_enabled = true;
    g_config.tick_rate_hz = 1000;
}

static void _apply_preset_m4(void)
{
    g_config.cpu_limit_enabled = true;
    g_config.cpu_freq_mhz = 168;
    g_config.fpu_enabled = true;
    g_config.icache_enabled = false;
    g_config.dcache_enabled = false;
    g_config.cache_line_size = 0;

    g_config.flash_limit_enabled = true;
    g_config.flash_read_speed_kbps = 4096;
    g_config.flash_write_speed_kbps = 1024;
    g_config.flash_erase_sector_ms = 60;


    g_config.memory_limit_enabled = true;
    g_config.max_ram_kb = 128;
    g_config.max_stack_kb = 16;

    g_config.system_limit_enabled = true;
    g_config.tick_rate_hz = 1000;
}

static void _apply_preset_m4f(void)
{
    g_config.cpu_limit_enabled = true;
    g_config.cpu_freq_mhz = 180;
    g_config.fpu_enabled = true;
    g_config.icache_enabled = true;
    g_config.dcache_enabled = true;
    g_config.cache_line_size = 32;

    g_config.flash_limit_enabled = true;
    g_config.flash_read_speed_kbps = 5120;
    g_config.flash_write_speed_kbps = 2048;
    g_config.flash_erase_sector_ms = 50;


    g_config.memory_limit_enabled = true;
    g_config.max_ram_kb = 256;
    g_config.max_stack_kb = 32;

    g_config.system_limit_enabled = true;
    g_config.tick_rate_hz = 1000;
}

static void _apply_preset_m7(void)
{
    g_config.cpu_limit_enabled = true;
    g_config.cpu_freq_mhz = 480;
    g_config.fpu_enabled = true;
    g_config.icache_enabled = true;
    g_config.dcache_enabled = true;
    g_config.cache_line_size = 32;

    g_config.flash_limit_enabled = true;
    g_config.flash_read_speed_kbps = 10240;
    g_config.flash_write_speed_kbps = 5120;
    g_config.flash_erase_sector_ms = 30;


    g_config.memory_limit_enabled = true;
    g_config.max_ram_kb = 512;
    g_config.max_stack_kb = 64;

    g_config.system_limit_enabled = true;
    g_config.tick_rate_hz = 1000;
}

static void _apply_preset_m33(void)
{
    g_config.cpu_limit_enabled = true;
    g_config.cpu_freq_mhz = 160;
    g_config.fpu_enabled = true;
    g_config.icache_enabled = true;
    g_config.dcache_enabled = true;
    g_config.cache_line_size = 32;

    g_config.flash_limit_enabled = true;
    g_config.flash_read_speed_kbps = 5120;
    g_config.flash_write_speed_kbps = 2048;
    g_config.flash_erase_sector_ms = 50;


    g_config.memory_limit_enabled = true;
    g_config.max_ram_kb = 256;
    g_config.max_stack_kb = 32;

    g_config.system_limit_enabled = true;
    g_config.tick_rate_hz = 1000;
}

static void _apply_preset_riscv(void)
{
    g_config.cpu_limit_enabled = true;
    g_config.cpu_freq_mhz = 144;
    g_config.fpu_enabled = false;
    g_config.icache_enabled = false;
    g_config.dcache_enabled = false;
    g_config.cache_line_size = 0;

    g_config.flash_limit_enabled = true;
    g_config.flash_read_speed_kbps = 3072;
    g_config.flash_write_speed_kbps = 768;
    g_config.flash_erase_sector_ms = 70;


    g_config.memory_limit_enabled = true;
    g_config.max_ram_kb = 256;
    g_config.max_stack_kb = 8;

    g_config.system_limit_enabled = true;
    g_config.tick_rate_hz = 1000;
}

static void _apply_preset_esp32(void)
{
    g_config.cpu_limit_enabled = true;
    g_config.cpu_freq_mhz = 240;
    g_config.fpu_enabled = true;
    g_config.icache_enabled = true;
    g_config.dcache_enabled = true;
    g_config.cache_line_size = 32;

    g_config.flash_limit_enabled = true;
    g_config.flash_read_speed_kbps = 5120;
    g_config.flash_write_speed_kbps = 1536;
    g_config.flash_erase_sector_ms = 100;


    g_config.memory_limit_enabled = true;
    g_config.max_ram_kb = 520;
    g_config.max_stack_kb = 32;

    g_config.system_limit_enabled = true;
    g_config.tick_rate_hz = 1000;
}

static void _apply_preset_none(void)
{
    memset(&g_config, 0, sizeof(g_config));
    g_config.cpu_freq_mhz = HOST_REFERENCE_FREQ_MHZ;
    g_config.fpu_enabled = true;
    g_config.icache_enabled = true;
    g_config.dcache_enabled = true;
    g_config.cache_line_size = 64;
    g_config.flash_read_speed_kbps = UINT32_MAX / 1024;
    g_config.flash_write_speed_kbps = UINT32_MAX / 1024;
    g_config.max_ram_kb = UINT32_MAX / 1024;
    g_config.tick_rate_hz = 1000;
}

static float _calc_cpu_slowdown(void)
{
    if (!g_config.cpu_limit_enabled) return 1.0f;

    float slowdown = (float)HOST_REFERENCE_FREQ_MHZ / (float)g_config.cpu_freq_mhz;
    if (!g_config.fpu_enabled) slowdown *= FPU_PENALTY;
    if (!g_config.icache_enabled) slowdown *= ICACHE_PENALTY;
    if (!g_config.dcache_enabled) slowdown *= DCACHE_PENALTY;
    return slowdown;
}

static uint32_t _calc_flash_delay_ms(void)
{
    if (!g_config.flash_limit_enabled) return 0;

    float delay = 0.0f;
    if (g_config.flash_read_speed_kbps > 0) {
        delay += (float)g_pending_flash_read_bytes / (float)(g_config.flash_read_speed_kbps * 1024) * 1000.0f;
    }
    if (g_config.flash_write_speed_kbps > 0) {
        delay += (float)g_pending_flash_write_bytes / (float)(g_config.flash_write_speed_kbps * 1024) * 1000.0f;
    }
    return (uint32_t)delay;
}

void eos_throttler_init(void)
{
    _apply_preset_none();
    _reset_stats();
    g_master_enabled = false;
    g_pending_flash_read_bytes = 0;
    g_pending_flash_write_bytes = 0;
    g_total_ram_allocated_kb = 0;
    g_peak_ram_kb = 0;
    g_ram_alloc_failures = 0;
}

void eos_throttler_deinit(void)
{
}

void eos_throttler_set_config(const eos_throttler_config_t *cfg)
{
    if (cfg) {
        g_config = *cfg;
    }
}

void eos_throttler_get_config(eos_throttler_config_t *cfg)
{
    if (cfg) {
        *cfg = g_config;
    }
}

void eos_throttler_apply_preset(eos_throttle_preset_t preset)
{
    switch (preset) {
    case EOS_THROTTLE_PRESET_NONE: _apply_preset_none(); break;
    case EOS_THROTTLE_PRESET_M0:   _apply_preset_m0(); break;
    case EOS_THROTTLE_PRESET_M3:   _apply_preset_m3(); break;
    case EOS_THROTTLE_PRESET_M4:   _apply_preset_m4(); break;
    case EOS_THROTTLE_PRESET_M4F:  _apply_preset_m4f(); break;
    case EOS_THROTTLE_PRESET_M7:   _apply_preset_m7(); break;
    case EOS_THROTTLE_PRESET_M33:  _apply_preset_m33(); break;
    case EOS_THROTTLE_PRESET_RISCV:_apply_preset_riscv(); break;
    case EOS_THROTTLE_PRESET_ESP32:_apply_preset_esp32(); break;
    case EOS_THROTTLE_PRESET_CUSTOM:
    default: break;
    }
    _reset_stats();
}

uint32_t eos_throttler_adjust_tick_delay(uint32_t orig_delay_ms)
{
    if (!g_master_enabled) {
        g_pending_flash_read_bytes = 0;
        g_pending_flash_write_bytes = 0;
        return orig_delay_ms;
    }

    float cpu_slowdown = _calc_cpu_slowdown();
    float cpu_adjusted = (float)orig_delay_ms * cpu_slowdown;

    uint32_t flash_delay = _calc_flash_delay_ms();

    uint32_t adjusted = (uint32_t)cpu_adjusted + flash_delay;

    g_stats.effective_slowdown = cpu_slowdown;
    if (adjusted > 0) {
        g_stats.actual_tick_rate_hz = 1000.0f / (float)adjusted;
        g_stats.actual_fps = g_stats.actual_tick_rate_hz;
    }
    g_stats.total_ticks++;
    g_stats.cpu_delay_us += (uint64_t)(adjusted > orig_delay_ms ? (adjusted - orig_delay_ms) * 1000 : 0);

    g_pending_flash_read_bytes = 0;
    g_pending_flash_write_bytes = 0;

    if (g_config.system_limit_enabled && g_config.tick_rate_hz > 0) {
        uint32_t min_delay = 1000 / g_config.tick_rate_hz;
        if (adjusted < min_delay) adjusted = min_delay;
    }

    return adjusted;
}

uint32_t eos_throttler_adjust_delay(uint32_t orig_ms)
{
    if (!g_master_enabled || !g_config.cpu_limit_enabled) return orig_ms;
    float s = _calc_cpu_slowdown();
    return (uint32_t)((float)orig_ms * s);
}

void eos_throttler_flash_read(uint32_t byte_count)
{
    if (!g_master_enabled || !g_config.flash_limit_enabled) return;
    g_pending_flash_read_bytes += byte_count;
    g_stats.flash_read_bytes += byte_count;
}

void eos_throttler_flash_write(uint32_t byte_count)
{
    if (!g_master_enabled || !g_config.flash_limit_enabled) return;
    g_pending_flash_write_bytes += byte_count;
    g_stats.flash_write_bytes += byte_count;
}

bool eos_throttler_mem_try_alloc(size_t size)
{
    if (!g_master_enabled || !g_config.memory_limit_enabled) return true;

    uint32_t size_kb = (uint32_t)((size + 1023) / 1024);
    uint32_t limit_kb = g_config.max_ram_kb;

    if (limit_kb == 0) return true;

    g_total_ram_allocated_kb += size_kb;
    if (g_total_ram_allocated_kb > limit_kb) {
        g_ram_alloc_failures++;
        g_stats.ram_alloc_failures = g_ram_alloc_failures;
    }
    if (g_total_ram_allocated_kb > g_peak_ram_kb) {
        g_peak_ram_kb = g_total_ram_allocated_kb;
    }
    g_stats.current_ram_usage_kb = g_total_ram_allocated_kb;
    g_stats.peak_ram_usage_kb = g_peak_ram_kb;
    return true;
}

void eos_throttler_mem_free(size_t size)
{
    if (!g_master_enabled || !g_config.memory_limit_enabled) return;

    uint32_t size_kb = (uint32_t)((size + 1023) / 1024);
    if (size_kb > g_total_ram_allocated_kb) {
        g_total_ram_allocated_kb = 0;
    } else {
        g_total_ram_allocated_kb -= size_kb;
    }
}

void eos_throttler_set_master_enable(bool enabled)
{
    g_master_enabled = enabled;
    if (enabled) {
        _reset_stats();
        g_pending_flash_read_bytes = 0;
        g_pending_flash_write_bytes = 0;
    }
}

bool eos_throttler_get_master_enable(void)
{
    return g_master_enabled;
}

void eos_throttler_get_stats(eos_throttler_stats_t *stats)
{
    if (stats) {
        *stats = g_stats;
        stats->current_ram_usage_kb = g_total_ram_allocated_kb;
        stats->peak_ram_usage_kb = g_peak_ram_kb;
        stats->ram_alloc_failures = g_ram_alloc_failures;
    }
}

const char *eos_throttler_preset_name(eos_throttle_preset_t preset)
{
    switch (preset) {
    case EOS_THROTTLE_PRESET_NONE:  return "None (Full Speed)";
    case EOS_THROTTLE_PRESET_M0:    return "Cortex-M0 48MHz";
    case EOS_THROTTLE_PRESET_M3:    return "Cortex-M3 72MHz";
    case EOS_THROTTLE_PRESET_M4:    return "Cortex-M4 168MHz";
    case EOS_THROTTLE_PRESET_M4F:   return "Cortex-M4F 180MHz";
    case EOS_THROTTLE_PRESET_M7:    return "Cortex-M7 480MHz";
    case EOS_THROTTLE_PRESET_M33:   return "Cortex-M33 160MHz";
    case EOS_THROTTLE_PRESET_RISCV: return "RISC-V 144MHz";
    case EOS_THROTTLE_PRESET_ESP32: return "ESP32 240MHz";
    case EOS_THROTTLE_PRESET_CUSTOM: return "Custom";
    default: return "Unknown";
    }
}
