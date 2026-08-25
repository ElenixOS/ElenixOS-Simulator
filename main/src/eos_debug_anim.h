/**
 * @file eos_debug_anim.h
 * @brief Animation debug tuner — simulator-side only.
 *
 * Hooks into eos_anim's intercept callback to modify animation parameters
 * at runtime without recompilation.  Provides a curve name↔pointer
 * mapping and state management for the visual tuner panel.
 *
 * NEVER include this from ElenixOS kernel code — it lives in main/src/
 * and links into the simulator binary only.
 */

#ifndef EOS_DEBUG_ANIM_H
#define EOS_DEBUG_ANIM_H

#include <stdbool.h>
#include <stdint.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* -------------------------------------------------------------------
     * Curve entry — one row in the built-in curve table.
     * ------------------------------------------------------------------- */

    typedef struct
    {
        const char        *name;        /* human-readable, e.g. "ease_out"     */
        lv_anim_path_cb_t  cb;          /* LVGL path function pointer          */
        bool               is_bezier3;  /* needs lv_anim_set_bezier3_param()?  */
        int16_t            bx1, by1;    /* bezier3 control point 1 (0 if N/A)  */
        int16_t            bx2, by2;    /* bezier3 control point 2              */
    } eos_debug_anim_curve_entry_t;

    /* -------------------------------------------------------------------
     * Custom bezier control-point values (fixed-point, LV_BEZIER_VAL_MAX=1024).
     * ------------------------------------------------------------------- */

    typedef struct
    {
        int16_t  bx1, by1;
        int16_t  bx2, by2;
    } eos_debug_anim_custom_bezier_t;

    /* -------------------------------------------------------------------
     * Tuning state (all in-memory, never persisted).
     * ------------------------------------------------------------------- */

    typedef struct
    {
        bool     enabled;                /* master on/off switch                */
        float    speed_multiplier;       /* 0.1x – 5.0x,  1.0 = no change      */
        int32_t  duration_override_ms;   /* -1 = auto,  >0 = force this ms      */
        int      curve_index;           /* index into curve table, -1 = none   */
        int      total_intercepted;      /* running counter                     */

        /* Custom bezier curve control */
        bool                          use_custom_bezier;  /* true → ignore curve_index, use custom_bezier params */
        eos_debug_anim_custom_bezier_t custom_bezier;
    } eos_debug_anim_state_t;

    /* -------------------------------------------------------------------
     * Public API
     * ------------------------------------------------------------------- */

    /** Register the intercept callback with eos_anim.  Call once at boot. */
    void eos_debug_anim_init(void);

    /** Return a pointer to the live tuning state. */
    eos_debug_anim_state_t *eos_debug_anim_get_state(void);

    /** Convenience setters (also update sub-animation durations if active). */
    void eos_debug_anim_set_enabled(bool en);
    void eos_debug_anim_set_speed(float multiplier);
    void eos_debug_anim_set_duration_override(int32_t ms);
    void eos_debug_anim_set_curve_index(int idx);

    /** Enable/disable custom bezier mode and set its control-point values. */
    void eos_debug_anim_set_use_custom_bezier(bool use);
    void eos_debug_anim_set_custom_bezier3(int16_t bx1, int16_t by1,
                                           int16_t bx2, int16_t by2);

    /** Reset every tunable back to pass-through defaults. */
    void eos_debug_anim_reset(void);

    /* ----- Curve table access (for the UI panel) ----- */

    /** Return the static curve table and write its length to *out_count. */
    const eos_debug_anim_curve_entry_t *eos_debug_anim_curve_table(int *out_count);
    int  eos_debug_anim_curve_count(void);
    const char *eos_debug_anim_curve_name_by_index(int idx);

#ifdef __cplusplus
}
#endif

#endif /* EOS_DEBUG_ANIM_H */
