/**
 * @file eos_debug_anim.c
 * @brief Animation debug tuner — intercept callback and curve table.
 */

#include "eos_debug_anim.h"
#include "eos_anim.h"

#include <string.h>
#include <stdio.h>

/* ===================================================================
 * Curve table
 * =================================================================== */

/* Fixed-point scale used by LVGL bezier3 params (×1024). */
#define _FX(v) ((int16_t)((v) * 1024.0f))

static const eos_debug_anim_curve_entry_t _curve_table[] = {
    /* ---- LVGL built-in paths (no bezier3 params) ---- */
    { "linear",           lv_anim_path_linear,       false, 0, 0, 0, 0 },
    { "ease_in",          lv_anim_path_ease_in,      false, 0, 0, 0, 0 },
    { "ease_out",         lv_anim_path_ease_out,     false, 0, 0, 0, 0 },
    { "ease_in_out",      lv_anim_path_ease_in_out,  false, 0, 0, 0, 0 },
    { "overshoot",        lv_anim_path_overshoot,    false, 0, 0, 0, 0 },
    { "bounce",           lv_anim_path_bounce,       false, 0, 0, 0, 0 },
    { "step",             lv_anim_path_step,         false, 0, 0, 0, 0 },

    /* ---- Cubic-bezier variants (lv_anim_path_custom_bezier3) ---- */
    { "ease_in_sine",     lv_anim_path_custom_bezier3, true, _FX(0.12), _FX(0),    _FX(0.39), _FX(0)    },
    { "ease_out_sine",    lv_anim_path_custom_bezier3, true, _FX(0.61), _FX(1),    _FX(0.88), _FX(1)    },
    { "ease_in_out_sine", lv_anim_path_custom_bezier3, true, _FX(0.37), _FX(0),    _FX(0.63), _FX(1)    },
    { "ease_in_quad",     lv_anim_path_custom_bezier3, true, _FX(0.11), _FX(0),    _FX(0.5),  _FX(0)    },
    { "ease_out_quad",    lv_anim_path_custom_bezier3, true, _FX(0.5),  _FX(1),    _FX(0.89), _FX(1)    },
    { "ease_in_out_quad", lv_anim_path_custom_bezier3, true, _FX(0.45), _FX(0),    _FX(0.55), _FX(1)    },
    { "ease_in_cubic",    lv_anim_path_custom_bezier3, true, _FX(0.32), _FX(0),    _FX(0.67), _FX(0)    },
    { "ease_out_cubic",   lv_anim_path_custom_bezier3, true, _FX(0.33), _FX(1),    _FX(0.68), _FX(1)    },
    { "ease_in_out_cubic",lv_anim_path_custom_bezier3, true, _FX(0.65), _FX(0),    _FX(0.35), _FX(1)    },
    { "ease_in_quart",    lv_anim_path_custom_bezier3, true, _FX(0.5),  _FX(0),    _FX(0.75), _FX(0)    },
    { "ease_out_quart",   lv_anim_path_custom_bezier3, true, _FX(0.25), _FX(1),    _FX(0.5),  _FX(1)    },
    { "ease_in_out_quart",lv_anim_path_custom_bezier3, true, _FX(0.76), _FX(0),    _FX(0.24), _FX(1)    },
    { "ease_in_quint",    lv_anim_path_custom_bezier3, true, _FX(0.64), _FX(0),    _FX(0.78), _FX(0)    },
    { "ease_out_quint",   lv_anim_path_custom_bezier3, true, _FX(0.22), _FX(1),    _FX(0.36), _FX(1)    },
    { "ease_in_out_quint",lv_anim_path_custom_bezier3, true, _FX(0.83), _FX(0),    _FX(0.17), _FX(1)    },
    { "ease_in_expo",     lv_anim_path_custom_bezier3, true, _FX(0.7),  _FX(0),    _FX(0.84), _FX(0)    },
    { "ease_out_expo",    lv_anim_path_custom_bezier3, true, _FX(0.16), _FX(1),    _FX(0.3),  _FX(1)    },
    { "ease_in_out_expo", lv_anim_path_custom_bezier3, true, _FX(0.87), _FX(0),    _FX(0.13), _FX(1)    },
    { "ease_in_circ",     lv_anim_path_custom_bezier3, true, _FX(0.55), _FX(0),    _FX(1),    _FX(0.45) },
    { "ease_out_circ",    lv_anim_path_custom_bezier3, true, _FX(0),    _FX(0.55), _FX(0.45), _FX(1)    },
    { "ease_in_out_circ", lv_anim_path_custom_bezier3, true, _FX(0.85), _FX(0),    _FX(0.15), _FX(1)    },
    { "ease_in_back",     lv_anim_path_custom_bezier3, true, _FX(0.36), _FX(0),    _FX(0.66), _FX(-0.56)},
    { "ease_out_back",    lv_anim_path_custom_bezier3, true, _FX(0.34), _FX(1.56), _FX(0.64), _FX(1)    },
    { "ease_in_out_back", lv_anim_path_custom_bezier3, true, _FX(0.68), _FX(-0.6), _FX(0.32), _FX(1.6)  },
};

#define _CURVE_COUNT (sizeof(_curve_table) / sizeof(_curve_table[0]))

/* ===================================================================
 * Tuning state
 * =================================================================== */

static eos_debug_anim_state_t _state = {
    .enabled              = true,
    .speed_multiplier     = 1.0f,
    .duration_override_ms = -1,
    .curve_index          = -1,
    .total_intercepted    = 0,
};

/* ===================================================================
 * Internal helpers — duration scaling per animation type
 * =================================================================== */

static uint32_t _scale_dur(uint32_t dur, float mult)
{
    if (mult <= 0.0f) return dur;
    uint32_t v = (uint32_t)((float)dur / mult);
    return v < 1 ? 1 : v;
}

static void _apply_multiplier_to_lv_anim(lv_anim_t *a, float mult)
{
    if (!a || a->duration == 0) return;
    a->duration = _scale_dur(a->duration, mult);
}

static void _scale_durations(eos_anim_t *anim, float mult)
{
    switch (anim->type) {
    case EOS_ANIM_SCALE:
        _apply_multiplier_to_lv_anim(&anim->anim.scale.a_width,  mult);
        _apply_multiplier_to_lv_anim(&anim->anim.scale.a_height, mult);
        break;
    case EOS_ANIM_FADE:
        _apply_multiplier_to_lv_anim(&anim->anim.fade.a_opa, mult);
        break;
    case EOS_ANIM_MOVE:
        if (!anim->cfg.move.disable_x)
            _apply_multiplier_to_lv_anim(&anim->anim.move.a_x, mult);
        if (!anim->cfg.move.disable_y)
            _apply_multiplier_to_lv_anim(&anim->anim.move.a_y, mult);
        break;
    case EOS_ANIM_TRANSFORM_SCALE:
        _apply_multiplier_to_lv_anim(&anim->anim.transform_scale.a_scale, mult);
        break;
    case EOS_ANIM_IMAGE_SCALE:
        _apply_multiplier_to_lv_anim(&anim->anim.image_scale.a_scale, mult);
        break;
    case EOS_ANIM_RESIZE:
        if (!anim->cfg.resize.disable_w)
            _apply_multiplier_to_lv_anim(&anim->anim.resize.a_w, mult);
        if (!anim->cfg.resize.disable_h)
            _apply_multiplier_to_lv_anim(&anim->anim.resize.a_h, mult);
        break;
    default:
        break;
    }
}

static void _apply_override_to_lv_anim(lv_anim_t *a, int32_t ms)
{
    if (!a) return;
    a->duration = (uint32_t)ms;
}

static void _override_durations(eos_anim_t *anim, int32_t ms)
{
    switch (anim->type) {
    case EOS_ANIM_SCALE:
        _apply_override_to_lv_anim(&anim->anim.scale.a_width,  ms);
        _apply_override_to_lv_anim(&anim->anim.scale.a_height, ms);
        break;
    case EOS_ANIM_FADE:
        _apply_override_to_lv_anim(&anim->anim.fade.a_opa, ms);
        break;
    case EOS_ANIM_MOVE:
        if (!anim->cfg.move.disable_x)
            _apply_override_to_lv_anim(&anim->anim.move.a_x, ms);
        if (!anim->cfg.move.disable_y)
            _apply_override_to_lv_anim(&anim->anim.move.a_y, ms);
        break;
    case EOS_ANIM_TRANSFORM_SCALE:
        _apply_override_to_lv_anim(&anim->anim.transform_scale.a_scale, ms);
        break;
    case EOS_ANIM_IMAGE_SCALE:
        _apply_override_to_lv_anim(&anim->anim.image_scale.a_scale, ms);
        break;
    case EOS_ANIM_RESIZE:
        if (!anim->cfg.resize.disable_w)
            _apply_override_to_lv_anim(&anim->anim.resize.a_w, ms);
        if (!anim->cfg.resize.disable_h)
            _apply_override_to_lv_anim(&anim->anim.resize.a_h, ms);
        break;
    default:
        break;
    }
}

/** Apply bezier3 params to one sub-animation's lv_anim_t. */
static void _apply_bezier3_to_lv_anim(lv_anim_t *a,
                                      int16_t bx1, int16_t by1,
                                      int16_t bx2, int16_t by2)
{
    if (!a) return;
    lv_anim_set_bezier3_param(a, bx1, by1, bx2, by2);
}

/** For bezier3 curves: set path + bezier3 params on all sub-animations. */
static void _apply_curve_with_bezier3(eos_anim_t *anim,
                                      const eos_debug_anim_curve_entry_t *entry)
{
    /* First override the path callback */
    eos_anim_set_path(anim, entry->cb);

    /* Then apply bezier3 params directly to every sub-animation's lv_anim_t */
    int16_t bx1 = entry->bx1, by1 = entry->by1;
    int16_t bx2 = entry->bx2, by2 = entry->by2;

    switch (anim->type) {
    case EOS_ANIM_SCALE:
        _apply_bezier3_to_lv_anim(&anim->anim.scale.a_width,  bx1, by1, bx2, by2);
        _apply_bezier3_to_lv_anim(&anim->anim.scale.a_height, bx1, by1, bx2, by2);
        break;
    case EOS_ANIM_FADE:
        _apply_bezier3_to_lv_anim(&anim->anim.fade.a_opa, bx1, by1, bx2, by2);
        break;
    case EOS_ANIM_MOVE:
        if (!anim->cfg.move.disable_x)
            _apply_bezier3_to_lv_anim(&anim->anim.move.a_x, bx1, by1, bx2, by2);
        if (!anim->cfg.move.disable_y)
            _apply_bezier3_to_lv_anim(&anim->anim.move.a_y, bx1, by1, bx2, by2);
        break;
    case EOS_ANIM_TRANSFORM_SCALE:
        _apply_bezier3_to_lv_anim(&anim->anim.transform_scale.a_scale, bx1, by1, bx2, by2);
        break;
    case EOS_ANIM_IMAGE_SCALE:
        _apply_bezier3_to_lv_anim(&anim->anim.image_scale.a_scale, bx1, by1, bx2, by2);
        break;
    case EOS_ANIM_RESIZE:
        if (!anim->cfg.resize.disable_w)
            _apply_bezier3_to_lv_anim(&anim->anim.resize.a_w, bx1, by1, bx2, by2);
        if (!anim->cfg.resize.disable_h)
            _apply_bezier3_to_lv_anim(&anim->anim.resize.a_h, bx1, by1, bx2, by2);
        break;
    default:
        break;
    }
}

/* ===================================================================
 * Intercept callback
 * =================================================================== */

static void _intercept_cb(eos_anim_t *anim)
{
    if (!_state.enabled) return;
    _state.total_intercepted++;

    /* 1. Speed multiplier */
    if (_state.speed_multiplier != 1.0f) {
        _scale_durations(anim, _state.speed_multiplier);
    }

    /* 2. Duration override */
    if (_state.duration_override_ms > 0) {
        _override_durations(anim, _state.duration_override_ms);
    }

    /* 3. Curve override */
    if (_state.use_custom_bezier) {
        /* Custom bezier mode: apply user-defined control points */
        eos_anim_set_path_bezier3(anim,
                                   _state.custom_bezier.bx1, _state.custom_bezier.by1,
                                   _state.custom_bezier.bx2, _state.custom_bezier.by2);
    } else if (_state.curve_index >= 0 && _state.curve_index < (int)_CURVE_COUNT) {
        const eos_debug_anim_curve_entry_t *entry = &_curve_table[_state.curve_index];
        if (entry->is_bezier3) {
            _apply_curve_with_bezier3(anim, entry);
        } else {
            eos_anim_set_path(anim, entry->cb);
        }
    }
}

/* ===================================================================
 * Public API
 * =================================================================== */

void eos_debug_anim_init(void)
{
    eos_anim_set_intercept_cb(_intercept_cb);
}

eos_debug_anim_state_t *eos_debug_anim_get_state(void)
{
    return &_state;
}

void eos_debug_anim_set_enabled(bool en)
{
    _state.enabled = en;
}

void eos_debug_anim_set_speed(float multiplier)
{
    if (multiplier < 0.05f) multiplier = 0.05f;
    if (multiplier > 5.0f)  multiplier = 5.0f;
    _state.speed_multiplier = multiplier;
}

void eos_debug_anim_set_duration_override(int32_t ms)
{
    _state.duration_override_ms = ms;
}

void eos_debug_anim_set_curve_index(int idx)
{
    _state.curve_index = idx;
}

void eos_debug_anim_set_use_custom_bezier(bool use)
{
    _state.use_custom_bezier = use;
}

void eos_debug_anim_set_custom_bezier3(int16_t bx1, int16_t by1,
                                       int16_t bx2, int16_t by2)
{
    _state.custom_bezier.bx1 = bx1;
    _state.custom_bezier.by1 = by1;
    _state.custom_bezier.bx2 = bx2;
    _state.custom_bezier.by2 = by2;
}

void eos_debug_anim_reset(void)
{
    _state.enabled              = true;
    _state.speed_multiplier     = 1.0f;
    _state.duration_override_ms = -1;
    _state.curve_index          = -1;
    _state.use_custom_bezier    = false;
    _state.custom_bezier.bx1   = 0;
    _state.custom_bezier.by1   = 0;
    _state.custom_bezier.bx2   = 0;
    _state.custom_bezier.by2   = 0;
}

const eos_debug_anim_curve_entry_t *eos_debug_anim_curve_table(int *out_count)
{
    if (out_count) *out_count = (int)_CURVE_COUNT;
    return _curve_table;
}

int eos_debug_anim_curve_count(void)
{
    return (int)_CURVE_COUNT;
}

const char *eos_debug_anim_curve_name_by_index(int idx)
{
    if (idx < 0 || idx >= (int)_CURVE_COUNT) return "None";
    return _curve_table[idx].name;
}
