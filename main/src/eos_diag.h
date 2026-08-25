/**
 * @file eos_diag.h
 * @brief Simulator-side diagnostic module for performance monitoring.
 *
 * Provides periodic LVGL state snapshots (memory, objects, timers, anims)
 * and a simple FPS counter.  All calls are no-ops in RELEASE builds.
 */

#ifndef EOS_DIAG_H
#define EOS_DIAG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Forward-declare LVGL types to avoid pulling in lvgl.h from this header */
typedef struct _lv_obj_t lv_obj_t;

#ifdef __cplusplus
extern "C"
{
#endif

/* -------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------- */

/**
 * @brief Periodic state snapshot — call once per main-loop iteration.
 *        Emits one log line every 5 seconds with LVGL heap, object,
 *        timer, animation, and recents counts.
 */
void eos_diag_periodic_sample(void);

/**
 * @brief Simple FPS counter driven from the main loop.
 *        Returns the current FPS value (0 until first second elapses).
 */
uint32_t eos_diag_get_fps(void);

#ifdef __cplusplus
}
#endif

#endif /* EOS_DIAG_H */
