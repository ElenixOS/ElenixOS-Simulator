#ifndef EOS_THROTTLER_H
#define EOS_THROTTLER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EOS_THROTTLE_PRESET_NONE = 0,
    EOS_THROTTLE_PRESET_M0,       /* Cortex-M0 ~48MHz, no cache, no FPU */
    EOS_THROTTLE_PRESET_M3,       /* Cortex-M3 ~72MHz, no cache, no FPU */
    EOS_THROTTLE_PRESET_M4,       /* Cortex-M4 ~168MHz, no cache, FPU */
    EOS_THROTTLE_PRESET_M4F,      /* Cortex-M4F ~180MHz, cache, FPU */
    EOS_THROTTLE_PRESET_M7,       /* Cortex-M7 ~480MHz, cache, FPU */
    EOS_THROTTLE_PRESET_M33,      /* Cortex-M33 ~160MHz, cache, FPU */
    EOS_THROTTLE_PRESET_RISCV,    /* RISC-V ~144MHz, no cache, no FPU */
    EOS_THROTTLE_PRESET_ESP32,    /* ESP32 ~240MHz, no flash cache, FPU */
    EOS_THROTTLE_PRESET_CUSTOM,
    EOS_THROTTLE_PRESET_COUNT
} eos_throttle_preset_t;

typedef struct {
    /* CPU */
    bool cpu_limit_enabled;
    uint32_t cpu_freq_mhz;        /* Target MCU frequency in MHz */
    bool fpu_enabled;             /* Hardware FPU present */
    bool icache_enabled;          /* Instruction cache */
    bool dcache_enabled;          /* Data cache */
    uint32_t cache_line_size;     /* Cache line size in bytes (0 = no cache) */

    /* Flash storage */
    bool flash_limit_enabled;
    uint32_t flash_read_speed_kbps;    /* KB/s read */
    uint32_t flash_write_speed_kbps;   /* KB/s write */
    uint32_t flash_erase_sector_ms;    /* sector erase time in ms */

    /* Memory */
    bool memory_limit_enabled;
    uint32_t max_ram_kb;               /* Total RAM limit */
    uint32_t max_stack_kb;             /* Stack limit (reserved from total) */

    /* System */
    bool system_limit_enabled;
    uint32_t tick_rate_hz;             /* OS tick rate */
} eos_throttler_config_t;

typedef struct {
    float effective_slowdown;
    float actual_tick_rate_hz;
    float actual_fps;
    uint64_t total_ticks;
    uint64_t cpu_delay_us;             /* accumulated CPU throttle delay */
    uint64_t flash_read_bytes;
    uint64_t flash_write_bytes;
    uint64_t flash_read_delay_us;      /* accumulated flash read delay */
    uint64_t flash_write_delay_us;     /* accumulated flash write delay */
    uint32_t current_ram_usage_kb;
    uint32_t peak_ram_usage_kb;
    uint32_t ram_alloc_failures;
} eos_throttler_stats_t;

void eos_throttler_init(void);
void eos_throttler_deinit(void);

void eos_throttler_set_config(const eos_throttler_config_t *cfg);
void eos_throttler_get_config(eos_throttler_config_t *cfg);

void eos_throttler_apply_preset(eos_throttle_preset_t preset);

/* Called from main loop before sleep; returns adjusted delay in ms */
uint32_t eos_throttler_adjust_tick_delay(uint32_t orig_delay_ms);

/* Adjust an arbitrary delay (e.g. eos_delay) by CPU slowdown factor only */
uint32_t eos_throttler_adjust_delay(uint32_t orig_ms);

/* Called before/after flash read/write to account for transfer time */
void eos_throttler_flash_read(uint32_t byte_count);
void eos_throttler_flash_write(uint32_t byte_count);

/* Called to check memory allocation limit */
bool eos_throttler_mem_try_alloc(size_t size);
void eos_throttler_mem_free(size_t size);

/* Master enable/disable */
void eos_throttler_set_master_enable(bool enabled);
bool eos_throttler_get_master_enable(void);

/* Get real-time stats */
void eos_throttler_get_stats(eos_throttler_stats_t *stats);

/* Get preset name string */
const char *eos_throttler_preset_name(eos_throttle_preset_t preset);

#ifdef __cplusplus
}
#endif

#endif
