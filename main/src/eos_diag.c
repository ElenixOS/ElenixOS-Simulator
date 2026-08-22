/**
 * @file eos_diag.c
 * @brief Simulator-side diagnostic module for performance monitoring.
 */

#include "eos_diag.h"

/* Includes ---------------------------------------------------*/
#include "lvgl.h"
#define EOS_LOG_TAG "Diag"
#include "eos_log.h"
#include "eos_activity.h"
#include "eos_recent_apps.h"
#include "eos_overlay_layer.h"
#include "eos_config.h"

/* Macros and Definitions -------------------------------------*/

/** Seconds between periodic state-snapshot log lines. */
#define _DIAG_PERIOD_SEC 5

/* Variables --------------------------------------------------*/

static uint32_t _last_sample_tick = 0;
static uint32_t _frame_count = 0;
static uint32_t _last_fps_tick = 0;
static uint32_t _cached_fps = 0;

/* Function Implementations -----------------------------------*/

/* -------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------- */

/**
 * @brief Recursively count children.
 */
static uint32_t _count_children(lv_obj_t *parent)
{
    if (!parent || !lv_obj_is_valid(parent))
        return 0;

    uint32_t cnt = lv_obj_get_child_cnt(parent);
    uint32_t total = cnt;
    for (uint32_t i = 0; i < cnt; i++)
    {
        lv_obj_t *child = lv_obj_get_child(parent, i);
        total += _count_children(child);
    }
    return total;
}

/**
 * @brief Count objects in a specific layer (NULL-safe).
 */
static uint32_t _count_layer(lv_obj_t *layer)
{
    if (!layer || !lv_obj_is_valid(layer))
        return 0;
    return _count_children(layer);
}

/**
 * @brief Count running LVGL timers.
 */
static uint32_t _count_timers(void)
{
    uint32_t count = 0;
    lv_timer_t *t = lv_timer_get_next(NULL);
    while (t)
    {
        count++;
        t = lv_timer_get_next(t);
    }
    return count;
}

/* -------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------- */

void eos_diag_periodic_sample(void)
{
#if EOS_ENABLE_DIAG
    /* FPS tracking */
    _frame_count++;
    uint32_t now = eos_tick_get();
    if (now - _last_fps_tick >= 1000)
    {
        _cached_fps = _frame_count;
        _frame_count = 0;
        _last_fps_tick = now;
    }

    /* Full state snapshot every _DIAG_PERIOD_SEC seconds */
    if (now - _last_sample_tick >= _DIAG_PERIOD_SEC * 1000)
    {
        _last_sample_tick = now;

        lv_mem_monitor_t mon;
        lv_memzero(&mon, sizeof(mon));
        lv_mem_monitor(&mon);

        lv_obj_t *screen = lv_screen_active();
        uint32_t objs_active = screen ? _count_children(screen) : 0;
        uint32_t objs_snap = _count_layer(eos_overlay_get_snapshot_layer());
        uint32_t objs_sys = _count_layer(lv_layer_sys());
        uint32_t timers = _count_timers();
        uint32_t anims = (uint32_t)lv_anim_count_running();
        uint32_t recents = eos_recent_apps_count();

        EOS_LOG_I("[DIAG t=%" PRIu32 "] fps=%" PRIu32
                  " lvgl_used=%" PRIu8 "%% frag=%" PRIu8 "%%"
                  " free_big=%zu"
                  " objs=%" PRIu32
                  " snap=%" PRIu32
                  " sys=%" PRIu32
                  " timers=%" PRIu32
                  " anims=%" PRIu32
                  " recents=%" PRIu32,
                  now,
                  _cached_fps,
                  mon.used_pct,
                  mon.frag_pct,
                  mon.free_biggest_size,
                  objs_active,
                  objs_snap,
                  objs_sys,
                  timers,
                  anims,
                  recents);
    }
#else
    (void)0;
#endif
}

uint32_t eos_diag_get_fps(void)
{
#if EOS_ENABLE_DIAG
    return _cached_fps;
#else
    return 0;
#endif
}
