/**
 * @file eos_debug_anim_panel.c
 * @brief Visual animation debug panel — fills a parent container.
 *
 * Created by eos_debug_anim_panel_create(parent).  The panel uses a
 * vertical flex layout and contains:
 *  - enabled switch
 *  - speed multiplier slider
 *  - duration override slider
 *  - curve dropdown (lv_dropdown, ~30 entries + "Custom")
 *  - canvas-based curve preview with grid + reference line
 *  - [when Custom selected] 4 bezier control-point sliders (x1/y1/x2/y2)
 *  - interception counter
 *  - reset button
 */

#include "eos_debug_anim_panel.h"
#include "eos_debug_anim.h"
#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

/* ===================================================================
 * Constants — dimensions
 * =================================================================== */

#define PAD          10
#define CARD_PAD      8
#define CARD_RADIUS  10
#define CV_W         320
#define CV_H         130

/* Speed slider: value / 100 = multiplier (10 → 0.10x, 500 → 5.00x) */
#define SPEED_MIN    10
#define SPEED_MAX    500
#define SPEED_DFL    100

/* Duration slider: 0 → Auto, 50–2000 → ms */
#define DUR_MIN      0
#define DUR_MAX      2000
#define DUR_DFL      0

/* Custom bezier slider range: fixed-point, LV_BEZIER_VAL_MAX = 1024.
 * Allow overshoot: -500 .. 1500 ≈ -0.49 .. 1.46 */
#define BEZ_MIN      (-500)
#define BEZ_MAX      1500

/* ===================================================================
 * Constants — colour palette (modern dark theme)
 * =================================================================== */

#define COL_BG           0x0f0f1a
#define COL_CARD         0x1a1a2e
#define COL_CARD_BORDER  0x2d2d44
#define COL_TITLE        0xe8e8f5
#define COL_LABEL        0x8888aa
#define COL_VALUE        0x00d4aa
#define COL_ACCENT       0x00d4aa
#define COL_ACCENT2      0x7c5cff
#define COL_BTN_BG       0x7c5cff
#define COL_BTN_BG_PR    0x6a4ce0
#define COL_CV_BG        0x0a0a14
#define COL_CV_GRID      0x1c1c33
#define COL_CV_REF       0x3a3a55
#define COL_CV_CURVE     0x00d4aa
#define COL_CV_DIM       0x555566
#define COL_CV_BORDER    0x2d2d44
#define COL_CV_CP        0xff6644   /* control-point marker colour */

/* ===================================================================
 * Static widget references (one panel instance)
 * =================================================================== */

static lv_obj_t *_panel            = NULL;
static lv_obj_t *_cv               = NULL;   /* curve-preview canvas       */
static lv_obj_t *_curve_dd         = NULL;   /* curve dropdown             */
static lv_obj_t *_speed_label      = NULL;
static lv_obj_t *_duration_label   = NULL;
static lv_obj_t *_count_label      = NULL;
static lv_obj_t *_enabled_sw       = NULL;
static lv_obj_t *_speed_slider     = NULL;
static lv_obj_t *_duration_slider  = NULL;

/* Custom bezier sliders + labels */
static lv_obj_t *_bez_card         = NULL;   /* container for the 4 sliders */
static lv_obj_t *_bez_x1_slider    = NULL;
static lv_obj_t *_bez_y1_slider    = NULL;
static lv_obj_t *_bez_x2_slider    = NULL;
static lv_obj_t *_bez_y2_slider    = NULL;
static lv_obj_t *_bez_x1_label     = NULL;
static lv_obj_t *_bez_y1_label     = NULL;
static lv_obj_t *_bez_x2_label     = NULL;
static lv_obj_t *_bez_y2_label     = NULL;

/* Default custom bezier params (ease_out) */
#define BEZ_X1_DFL  0
#define BEZ_Y1_DFL  0
#define BEZ_X2_DFL  584   /* 0.58 * 1024 */
#define BEZ_Y2_DFL  1024

static LV_ATTRIBUTE_LARGE_RAM_ARRAY uint8_t _cv_buf[CV_W * CV_H * 2]; /* RGB565 */

static lv_timer_t *_refresh_timer  = NULL;

/* ===================================================================
 * Canvas drawing
 * =================================================================== */

static void _cv_set_px_safe(int x, int y, lv_color_t col, lv_opa_t opa)
{
    if (x < 0 || x >= CV_W || y < 0 || y >= CV_H) return;
    lv_canvas_set_px(_cv, x, y, col, opa);
}

static void _cv_draw_hline(int y, lv_color_t col, lv_opa_t opa)
{
    for (int x = 0; x < CV_W; x++)
        _cv_set_px_safe(x, y, col, opa);
}

static void _cv_draw_vline(int x, lv_color_t col, lv_opa_t opa)
{
    for (int y = 0; y < CV_H; y++)
        _cv_set_px_safe(x, y, col, opa);
}

static void _cv_draw_dot(int cx, int cy, lv_color_t col)
{
    for (int dy = -2; dy <= 2; dy++)
        for (int dx = -2; dx <= 2; dx++)
            if (dx * dx + dy * dy <= 4)
                _cv_set_px_safe(cx + dx, cy + dy, col, LV_OPA_COVER);
}

/**
 * Draw the curve preview onto the canvas.
 *
 * When curve_idx == CUSTOM_BEZIER_IDX, reads slider values for control
 * points; otherwise uses the preset curve table (as before).
 */
static void _cv_draw_curve(int curve_idx)
{
    if (!_cv) return;

    /* ---- Background ---- */
    lv_canvas_fill_bg(_cv, lv_color_hex(COL_CV_BG), LV_OPA_COVER);

    /* ---- Grid lines (4×4) ---- */
    lv_color_t grid_col = lv_color_hex(COL_CV_GRID);
    for (int i = 1; i < 4; i++) {
        int gx = CV_W * i / 4;
        int gy = CV_H * i / 4;
        _cv_draw_vline(gx, grid_col, LV_OPA_60);
        _cv_draw_hline(gy, grid_col, LV_OPA_60);
    }

    /* ---- Border ---- */
    lv_color_t border_col = lv_color_hex(COL_CV_BORDER);
    _cv_draw_hline(0, border_col, LV_OPA_COVER);
    _cv_draw_hline(CV_H - 1, border_col, LV_OPA_COVER);
    _cv_draw_vline(0, border_col, LV_OPA_COVER);
    _cv_draw_vline(CV_W - 1, border_col, LV_OPA_COVER);

    /* ---- Reference diagonal (linear) ---- */
    lv_color_t ref_col = lv_color_hex(COL_CV_REF);
    for (int x = 1; x < CV_W - 1; x++) {
        int y = CV_H - 2 - (int64_t)(x - 1) * (CV_H - 3) / (CV_W - 3);
        _cv_set_px_safe(x, y, ref_col, LV_OPA_COVER);
    }

    /* ---- Set up a dummy anim for sampling ---- */
    lv_anim_t a;
    lv_anim_init(&a);
    a.duration    = 1024;
    a.start_value = 0;
    a.end_value   = 1024;

    /* Determine if we use custom bezier or preset */
    int curve_count = 0;
    eos_debug_anim_curve_table(&curve_count);
    bool is_custom = (curve_idx == curve_count);  /* last entry = Custom */

    if (is_custom) {
        int16_t bx1 = (int16_t)lv_slider_get_value(_bez_x1_slider);
        int16_t by1 = (int16_t)lv_slider_get_value(_bez_y1_slider);
        int16_t bx2 = (int16_t)lv_slider_get_value(_bez_x2_slider);
        int16_t by2 = (int16_t)lv_slider_get_value(_bez_y2_slider);
        lv_anim_set_bezier3_param(&a, bx1, by1, bx2, by2);

        /* Draw control points */
        int plot_x1 = 1 + (int64_t)bx1 * (CV_W - 3) / 1024;
        int plot_y1 = CV_H - 2 - (int64_t)by1 * (CV_H - 3) / 1024;
        int plot_x2 = 1 + (int64_t)bx2 * (CV_W - 3) / 1024;
        int plot_y2 = CV_H - 2 - (int64_t)by2 * (CV_H - 3) / 1024;
        lv_color_t cp_col = lv_color_hex(COL_CV_CP);
        _cv_draw_dot(plot_x1, plot_y1, cp_col);
        _cv_draw_dot(plot_x2, plot_y2, cp_col);

        /* Draw control handle lines (P0→CP1 and CP2→P3) */
        for (int x = 1; x <= plot_x1 && x < CV_W - 1; x++) {
            int y = CV_H - 2 - (int64_t)(x - 1) * (plot_y1 - (CV_H - 2)) / (plot_x1 > 1 ? (plot_x1 - 1) : 1);
            _cv_set_px_safe(x, y, cp_col, LV_OPA_80);
        }
        for (int x = plot_x2; x < CV_W - 1; x++) {
            int y = plot_y2 + (int64_t)(x - plot_x2) * ((CV_H - 2) - plot_y2) / ((CV_W - 2) > plot_x2 ? (CV_W - 2 - plot_x2) : 1);
            _cv_set_px_safe(x, y, cp_col, LV_OPA_80);
        }
    } else {
        const eos_debug_anim_curve_entry_t *table =
            eos_debug_anim_curve_table(&curve_count);
        const eos_debug_anim_curve_entry_t *entry = NULL;
        lv_anim_path_cb_t path_cb = NULL;

        if (curve_idx >= 0 && curve_idx < curve_count) {
            path_cb = table[curve_idx].cb;
            entry   = &table[curve_idx];
        }
        if (!path_cb)
            path_cb = lv_anim_path_linear;

        if (entry && entry->is_bezier3) {
            lv_anim_set_bezier3_param(&a,
                                      entry->bx1, entry->by1,
                                      entry->bx2, entry->by2);
        }

        /* ---- Draw the actual curve ---- */
        lv_color_t curve_col = (curve_idx < 0)
            ? lv_color_hex(COL_CV_DIM) : lv_color_hex(COL_CV_CURVE);

        int prev_y = -1;
        for (int x = 1; x < CV_W - 1; x++) {
            a.act_time = (int32_t)((int64_t)(x - 1) * 1024 / (CV_W - 3));
            int32_t val = path_cb(&a);
            if (val < 0)    val = 0;
            if (val > 1024) val = 1024;
            int y = CV_H - 2 - (int64_t)val * (CV_H - 3) / 1024;

            for (int dy = -1; dy <= 1; dy++)
                _cv_set_px_safe(x, y + dy, curve_col, LV_OPA_COVER);

            if (prev_y >= 0) {
                int dy_abs = y - prev_y;
                if (dy_abs > 1 || dy_abs < -1) {
                    int step = (dy_abs > 0) ? 1 : -1;
                    for (int fy = prev_y; fy != y; fy += step) {
                        for (int dx = 0; dx <= 1; dx++)
                            _cv_set_px_safe(x - dx, fy, curve_col, LV_OPA_COVER);
                    }
                }
            }
            prev_y = y;
        }
        return; /* preset curve done */
    }

    /* ---- Draw the custom bezier curve ---- */
    lv_color_t curve_col = lv_color_hex(COL_CV_CURVE);
    lv_anim_path_cb_t custom_cb = lv_anim_path_custom_bezier3;

    int prev_y = -1;
    for (int x = 1; x < CV_W - 1; x++) {
        a.act_time = (int32_t)((int64_t)(x - 1) * 1024 / (CV_W - 3));
        int32_t val = custom_cb(&a);
        if (val < 0)    val = 0;
        if (val > 1024) val = 1024;
        int y = CV_H - 2 - (int64_t)val * (CV_H - 3) / 1024;

        for (int dy = -1; dy <= 1; dy++)
            _cv_set_px_safe(x, y + dy, curve_col, LV_OPA_COVER);

        if (prev_y >= 0) {
            int dy_abs = y - prev_y;
            if (dy_abs > 1 || dy_abs < -1) {
                int step = (dy_abs > 0) ? 1 : -1;
                for (int fy = prev_y; fy != y; fy += step) {
                    for (int dx = 0; dx <= 1; dx++)
                        _cv_set_px_safe(x - dx, fy, curve_col, LV_OPA_COVER);
                }
            }
        }
        prev_y = y;
    }
}

/* ===================================================================
 * Label updaters
 * =================================================================== */

static void _update_speed_label(void)
{
    if (!_speed_label) return;
    eos_debug_anim_state_t *st = eos_debug_anim_get_state();
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2fx", (double)st->speed_multiplier);
    lv_label_set_text(_speed_label, buf);
}

static void _update_duration_label(int32_t slider_val)
{
    if (!_duration_label) return;
    if (slider_val <= 0)
        lv_label_set_text(_duration_label, "Auto");
    else {
        char buf[16];
        snprintf(buf, sizeof(buf), "%" PRId32 "ms", slider_val);
        lv_label_set_text(_duration_label, buf);
    }
}

static void _update_count_label(void)
{
    if (!_count_label) return;
    eos_debug_anim_state_t *st = eos_debug_anim_get_state();
    char buf[48];
    snprintf(buf, sizeof(buf), "Intercepted: %d", st->total_intercepted);
    lv_label_set_text(_count_label, buf);
}

static void _update_bez_label(lv_obj_t *label, int32_t val)
{
    if (!label) return;
    /* Show as floating-point value (val / 1024.0) */
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2f", (double)val / 1024.0);
    lv_label_set_text(label, buf);
}

/* ===================================================================
 * Helpers — widget factories
 * =================================================================== */

/* ---- Build dropdown option string from the curve table ---- */
static char *_build_curve_options(void)
{
    int cnt = 0;
    eos_debug_anim_curve_table(&cnt);

    /* "None" + 30 curves + "Custom", each up to ~20 chars + '\n' */
    size_t cap = 34 * 22 + 8;
    char *buf = malloc(cap);
    if (!buf) return NULL;

    size_t pos = 0;
    pos += (size_t)snprintf(buf + pos, cap - pos, "None");

    for (int i = 0; i < cnt; i++) {
        if (pos + 2 >= cap) break;
        buf[pos++] = '\n';
        pos += (size_t)snprintf(buf + pos, cap - pos, "%s",
                                eos_debug_anim_curve_name_by_index(i));
    }
    /* "Custom" is the last entry */
    if (pos + 2 < cap) {
        buf[pos++] = '\n';
        pos += (size_t)snprintf(buf + pos, cap - pos, "Custom");
    }
    return buf;
}

/* ---- Create a card container ---- */
static lv_obj_t *_make_card(lv_obj_t *parent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_width(card, LV_PCT(100));
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, lv_color_hex(COL_CARD), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(COL_CARD_BORDER), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, CARD_RADIUS, 0);
    lv_obj_set_style_pad_all(card, CARD_PAD, 0);
    lv_obj_set_style_pad_row(card, 8, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    return card;
}

/* ---- Create a small section label ---- */
static lv_obj_t *_make_section_label(lv_obj_t *parent, const char *text)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    return lbl;
}

/* ---- Create a labelled slider row ---- */
static lv_obj_t *_make_slider_row(lv_obj_t *parent, const char *title,
                                   int32_t min, int32_t max, int32_t dfl,
                                   lv_event_cb_t cb,
                                   lv_obj_t **out_slider)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 8, 0);

    lv_obj_t *label = lv_label_create(row);
    lv_label_set_text(label, title);
    lv_obj_set_width(label, 64);
    lv_obj_set_style_text_color(label, lv_color_hex(COL_LABEL), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);

    lv_obj_t *slider = lv_slider_create(row);
    lv_obj_set_flex_grow(slider, 1);
    lv_slider_set_range(slider, min, max);
    lv_slider_set_value(slider, dfl, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, lv_color_hex(COL_CARD_BORDER), 0);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(slider, lv_color_hex(COL_ACCENT), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(COL_ACCENT), LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider, 2, 0);
    lv_obj_add_event_cb(slider, cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *val_label = lv_label_create(row);
    lv_obj_set_width(val_label, 56);
    lv_obj_set_style_text_align(val_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(val_label, lv_color_hex(COL_VALUE), 0);
    lv_obj_set_style_text_font(val_label, &lv_font_montserrat_14, 0);

    if (out_slider) *out_slider = slider;
    return val_label;
}

/* ===================================================================
 * Event handlers
 * =================================================================== */

static void _enabled_cb(lv_event_t *e)
{
    bool on = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
    eos_debug_anim_set_enabled(on);
}

static void _speed_cb(lv_event_t *e)
{
    int32_t v = (int32_t)lv_slider_get_value(lv_event_get_target(e));
    eos_debug_anim_set_speed((float)v / 100.0f);
    _update_speed_label();
}

static void _duration_cb(lv_event_t *e)
{
    int32_t v = (int32_t)lv_slider_get_value(lv_event_get_target(e));
    eos_debug_anim_set_duration_override((v <= 0) ? -1 : v);
    _update_duration_label(v);
}

static void _show_hide_bez_card(bool show)
{
    if (!_bez_card) return;
    if (show)
        lv_obj_remove_flag(_bez_card, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(_bez_card, LV_OBJ_FLAG_HIDDEN);
}

static void _sync_custom_bezier_to_state(void)
{
    int16_t bx1 = (int16_t)lv_slider_get_value(_bez_x1_slider);
    int16_t by1 = (int16_t)lv_slider_get_value(_bez_y1_slider);
    int16_t bx2 = (int16_t)lv_slider_get_value(_bez_x2_slider);
    int16_t by2 = (int16_t)lv_slider_get_value(_bez_y2_slider);
    eos_debug_anim_set_custom_bezier3(bx1, by1, bx2, by2);
}

static void _dropdown_cb(lv_event_t *e)
{
    int sel = (int)lv_dropdown_get_selected(lv_event_get_target(e));
    /* Index 0 = "None" → curve_index = -1 */
    /* Last  = "Custom" → use_custom_bezier = true */
    int curve_count = eos_debug_anim_curve_count();
    int custom_idx = curve_count;  /* the index after the last preset */

    if (sel == custom_idx + 1) {
        /* "Custom" selected (dropdown 1-indexed: None=0, presets 1..N, Custom=N+1) */
        eos_debug_anim_set_curve_index(-1);
        eos_debug_anim_set_use_custom_bezier(true);
        _sync_custom_bezier_to_state();
        _show_hide_bez_card(true);
        _cv_draw_curve(custom_idx);
    } else {
        int idx = sel - 1;
        eos_debug_anim_set_curve_index(idx);
        eos_debug_anim_set_use_custom_bezier(false);
        _show_hide_bez_card(false);
        _cv_draw_curve(idx);
    }
}

static void _bez_slider_cb(lv_event_t *e)
{
    _sync_custom_bezier_to_state();
    /* Redraw canvas with custom bezier */
    int curve_count = eos_debug_anim_curve_count();
    _cv_draw_curve(curve_count);

    /* Update value labels */
    _update_bez_label(_bez_x1_label, lv_slider_get_value(_bez_x1_slider));
    _update_bez_label(_bez_y1_label, lv_slider_get_value(_bez_y1_slider));
    _update_bez_label(_bez_x2_label, lv_slider_get_value(_bez_x2_slider));
    _update_bez_label(_bez_y2_label, lv_slider_get_value(_bez_y2_slider));
}

static void _reset_cb(lv_event_t *e)
{
    eos_debug_anim_reset();
    if (_speed_slider)
        lv_slider_set_value(_speed_slider, SPEED_DFL, LV_ANIM_OFF);
    if (_duration_slider)
        lv_slider_set_value(_duration_slider, DUR_DFL, LV_ANIM_OFF);
    if (_enabled_sw)
        lv_obj_add_state(_enabled_sw, LV_STATE_CHECKED);
    if (_curve_dd)
        lv_dropdown_set_selected(_curve_dd, 0);  /* "None" */
    if (_bez_x1_slider) lv_slider_set_value(_bez_x1_slider, BEZ_X1_DFL, LV_ANIM_OFF);
    if (_bez_y1_slider) lv_slider_set_value(_bez_y1_slider, BEZ_Y1_DFL, LV_ANIM_OFF);
    if (_bez_x2_slider) lv_slider_set_value(_bez_x2_slider, BEZ_X2_DFL, LV_ANIM_OFF);
    if (_bez_y2_slider) lv_slider_set_value(_bez_y2_slider, BEZ_Y2_DFL, LV_ANIM_OFF);
    _show_hide_bez_card(false);
    _update_speed_label();
    _update_duration_label(DUR_DFL);
    _update_bez_label(_bez_x1_label, BEZ_X1_DFL);
    _update_bez_label(_bez_y1_label, BEZ_Y1_DFL);
    _update_bez_label(_bez_x2_label, BEZ_X2_DFL);
    _update_bez_label(_bez_y2_label, BEZ_Y2_DFL);
    _cv_draw_curve(-1);
}

static void _refresh_timer_cb(lv_timer_t *t)
{
    _update_count_label();
}

/* ===================================================================
 * Public API
 * =================================================================== */

lv_obj_t *eos_debug_anim_panel_create(lv_obj_t *parent)
{
    if (!parent) return NULL;
    if (_panel) return _panel;

    /* ---- Main panel fills parent ---- */
    _panel = lv_obj_create(parent);
    lv_obj_set_size(_panel, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(_panel, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_border_width(_panel, 0, 0);
    lv_obj_set_style_radius(_panel, 0, 0);
    lv_obj_set_style_pad_all(_panel, PAD, 0);
    lv_obj_set_flex_flow(_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(_panel, 8, 0);
    lv_obj_set_scroll_dir(_panel, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(_panel, LV_SCROLLBAR_MODE_ACTIVE);

    /* ---- Title ---- */
    lv_obj_t *title = lv_label_create(_panel);
    lv_label_set_text(title, "Animation Debugger");
    lv_obj_set_style_text_color(title, lv_color_hex(COL_TITLE), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_bottom(title, 4, 0);

    /* ===================================================
     * Card 1 — Controls
     * =================================================== */
    lv_obj_t *ctrl_card = _make_card(_panel);

    /* ---- Enabled switch row ---- */
    lv_obj_t *en_row = lv_obj_create(ctrl_card);
    lv_obj_set_size(en_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(en_row, 0, 0);
    lv_obj_set_style_border_width(en_row, 0, 0);
    lv_obj_set_style_bg_opa(en_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(en_row, 0, 0);
    lv_obj_set_flex_flow(en_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(en_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *en_label = lv_label_create(en_row);
    lv_label_set_text(en_label, "Enabled");
    lv_obj_set_style_text_color(en_label, lv_color_hex(COL_LABEL), 0);
    lv_obj_set_style_text_font(en_label, &lv_font_montserrat_14, 0);

    _enabled_sw = lv_switch_create(en_row);
    lv_obj_add_state(_enabled_sw, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(_enabled_sw, lv_color_hex(COL_CARD_BORDER), 0);
    lv_obj_set_style_bg_color(_enabled_sw, lv_color_hex(COL_ACCENT), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(_enabled_sw, lv_color_hex(0xffffff), LV_PART_KNOB);
    lv_obj_add_event_cb(_enabled_sw, _enabled_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* ---- Speed slider ---- */
    _speed_label = _make_slider_row(ctrl_card, "Speed",
                                    SPEED_MIN, SPEED_MAX, SPEED_DFL,
                                    _speed_cb, &_speed_slider);
    _update_speed_label();

    /* ---- Duration slider ---- */
    _duration_label = _make_slider_row(ctrl_card, "Duration",
                                       DUR_MIN, DUR_MAX, DUR_DFL,
                                       _duration_cb, &_duration_slider);
    _update_duration_label(DUR_DFL);

    /* ---- Curve dropdown row ---- */
    lv_obj_t *dd_row = lv_obj_create(ctrl_card);
    lv_obj_set_size(dd_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(dd_row, 0, 0);
    lv_obj_set_style_border_width(dd_row, 0, 0);
    lv_obj_set_style_bg_opa(dd_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(dd_row, 0, 0);
    lv_obj_set_flex_flow(dd_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dd_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(dd_row, 8, 0);

    lv_obj_t *dd_label = lv_label_create(dd_row);
    lv_label_set_text(dd_label, "Curve");
    lv_obj_set_width(dd_label, 64);
    lv_obj_set_style_text_color(dd_label, lv_color_hex(COL_LABEL), 0);
    lv_obj_set_style_text_font(dd_label, &lv_font_montserrat_14, 0);

    _curve_dd = lv_dropdown_create(dd_row);
    lv_obj_set_flex_grow(_curve_dd, 1);
    lv_obj_set_style_bg_color(_curve_dd, lv_color_hex(COL_CARD_BORDER), 0);
    lv_obj_set_style_bg_opa(_curve_dd, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(_curve_dd, lv_color_hex(COL_CARD_BORDER), 0);
    lv_obj_set_style_text_color(_curve_dd, lv_color_hex(COL_TITLE), 0);
    lv_obj_set_style_text_font(_curve_dd, &lv_font_montserrat_14, 0);
    lv_obj_set_style_radius(_curve_dd, 6, 0);
    char *opts = _build_curve_options();
    if (opts) {
        lv_dropdown_set_options(_curve_dd, opts);
        free(opts);
    }
    lv_dropdown_set_selected(_curve_dd, 0);
    lv_dropdown_set_dir(_curve_dd, LV_DIR_TOP);
    lv_dropdown_set_symbol(_curve_dd, LV_SYMBOL_DOWN);
    lv_obj_add_event_cb(_curve_dd, _dropdown_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* ===================================================
     * Card 2 — Custom Bezier (hidden by default)
     * =================================================== */
    _bez_card = _make_card(_panel);
    lv_obj_add_flag(_bez_card, LV_OBJ_FLAG_HIDDEN);

    _make_section_label(_bez_card, "Custom Bezier");

    _bez_x1_label = _make_slider_row(_bez_card, "x1",
                                     BEZ_MIN, BEZ_MAX, BEZ_X1_DFL,
                                     _bez_slider_cb, &_bez_x1_slider);
    _bez_y1_label = _make_slider_row(_bez_card, "y1",
                                     BEZ_MIN, BEZ_MAX, BEZ_Y1_DFL,
                                     _bez_slider_cb, &_bez_y1_slider);
    _bez_x2_label = _make_slider_row(_bez_card, "x2",
                                     BEZ_MIN, BEZ_MAX, BEZ_X2_DFL,
                                     _bez_slider_cb, &_bez_x2_slider);
    _bez_y2_label = _make_slider_row(_bez_card, "y2",
                                     BEZ_MIN, BEZ_MAX, BEZ_Y2_DFL,
                                     _bez_slider_cb, &_bez_y2_slider);

    /* Initialize labels */
    _update_bez_label(_bez_x1_label, BEZ_X1_DFL);
    _update_bez_label(_bez_y1_label, BEZ_Y1_DFL);
    _update_bez_label(_bez_x2_label, BEZ_X2_DFL);
    _update_bez_label(_bez_y2_label, BEZ_Y2_DFL);

    /* ===================================================
     * Card 3 — Curve preview
     * =================================================== */
    lv_obj_t *prev_card = _make_card(_panel);

    _make_section_label(prev_card, "Curve Preview");

    _cv = lv_canvas_create(prev_card);
    lv_obj_set_size(_cv, CV_W, CV_H);
    lv_obj_set_style_radius(_cv, 6, 0);
    lv_obj_set_style_border_color(_cv, lv_color_hex(COL_CV_BORDER), 0);
    lv_obj_set_style_border_width(_cv, 1, 0);
    lv_obj_set_style_border_opa(_cv, LV_OPA_COVER, 0);
    lv_canvas_set_buffer(_cv, _cv_buf, CV_W, CV_H, LV_COLOR_FORMAT_RGB565);
    _cv_draw_curve(-1);

    /* ===================================================
     * Footer — counter + reset
     * =================================================== */
    _count_label = lv_label_create(_panel);
    lv_obj_set_style_text_color(_count_label, lv_color_hex(COL_LABEL), 0);
    lv_obj_set_style_text_font(_count_label, &lv_font_montserrat_14, 0);
    _update_count_label();

    lv_obj_t *reset_btn = lv_button_create(_panel);
    lv_obj_set_size(reset_btn, LV_PCT(100), 36);
    lv_obj_set_style_bg_color(reset_btn, lv_color_hex(COL_BTN_BG), 0);
    lv_obj_set_style_bg_color(reset_btn, lv_color_hex(COL_BTN_BG_PR), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(reset_btn, 8, 0);
    lv_obj_set_style_border_width(reset_btn, 0, 0);
    lv_obj_set_style_shadow_width(reset_btn, 0, 0);
    lv_obj_t *reset_lbl = lv_label_create(reset_btn);
    lv_label_set_text(reset_lbl, "Reset Defaults");
    lv_obj_set_style_text_color(reset_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(reset_lbl);
    lv_obj_add_event_cb(reset_btn, _reset_cb, LV_EVENT_CLICKED, NULL);

    /* ---- Periodic counter refresh ---- */
    _refresh_timer = lv_timer_create(_refresh_timer_cb, 300, NULL);

    return _panel;
}
