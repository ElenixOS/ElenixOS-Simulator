/**
 * @file eos_debug_sensor_panel.c
 * @brief Sensor debug panel — real-time display of all sensor values.
 *
 * Created by eos_debug_sensor_panel_create(parent).  The panel uses a
 * vertical flex layout and contains one card per registered sensor:
 *  - Accelerometer (ACCE)  — X / Y / Z
 *  - Gyroscope (GYRO)       — X / Y / Z
 *  - Magnetometer (MAG)     — X / Y / Z
 *  - Heart Rate (HR)        — bpm
 *  - SpO2 (SPO2)            — %
 *  - Light (LIGHT)          — lux
 *  - Temperature (TEMP)     — °C
 *  - Barometer (BARO)       — Pa
 *  - Proximity (PROXIMITY)  — mm
 *  - ECG (ECG)              — raw
 *  - Capacitance (CAP)      — raw
 *  - Step Counter (STEP)    — steps
 *
 * Each card displays the sensor name, current value(s), a random-data
 * switch, and fixed-value inputs.  A 100 ms timer refreshes all values.
 */

#include "eos_debug_sensor_panel.h"
#include "eos_service_sensor.h"
#include "eos_dev_sensor.h"
#include "eos_port_sensor.h"
#include "eos_error.h"
#include "lvgl.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ===================================================================
 * Constants — dimensions
 * =================================================================== */

#define PAD 10
#define CARD_PAD 8
#define CARD_RADIUS 8
#define DEBUG_SAMPLE_INTERVAL_MS 100

/* Number of registered sensors in the simulator */
#define SENSOR_COUNT 12

/* ===================================================================
 * Constants — colour palette (modern dark theme, same as anim panel)
 * =================================================================== */

#define COL_BG 0x0f0f1a
#define COL_CARD 0x1a1a2e
#define COL_CARD_BORDER 0x2d2d44
#define COL_TITLE 0xe8e8f5
#define COL_LABEL 0x8888aa
#define COL_VALUE 0x00d4aa
#define COL_INPUT_BG 0x101020
#define COL_BTN_BG 0x2d2d44
#define COL_BTN_BG_PR 0x3a3a55

/* Category accent colours */
#define COL_IMU 0x00d4aa
#define COL_HEALTH 0xff6644
#define COL_ENV 0x7c5cff
#define COL_MOTION 0xffaa00

/* ===================================================================
 * Sensor info table
 * =================================================================== */

typedef struct
{
    const char *name;
    const char *short_name;
    eos_sensor_type_t type;
    uint32_t color;
    bool is_triple_axis;
} _sensor_info_t;

static const _sensor_info_t _sensor_table[SENSOR_COUNT] = {
    {"Accelerometer", "ACCE", EOS_SENSOR_TYPE_ACCE, COL_IMU, true},
    {"Gyroscope", "GYRO", EOS_SENSOR_TYPE_GYRO, COL_IMU, true},
    {"Magnetometer", "MAG", EOS_SENSOR_TYPE_MAG, COL_IMU, true},
    {"Heart Rate", "HR", EOS_SENSOR_TYPE_HR, COL_HEALTH, false},
    {"SpO2", "SPO2", EOS_SENSOR_TYPE_SPO2, COL_HEALTH, false},
    {"Light", "LIGHT", EOS_SENSOR_TYPE_LIGHT, COL_ENV, false},
    {"Proximity", "PROX", EOS_SENSOR_TYPE_PROXIMITY, COL_ENV, false},
    {"ECG", "ECG", EOS_SENSOR_TYPE_ECG, COL_HEALTH, false},
    {"Temperature", "TEMP", EOS_SENSOR_TYPE_TEMP, COL_ENV, false},
    {"Barometer", "BARO", EOS_SENSOR_TYPE_BARO, COL_ENV, false},
    {"Capacitance", "CAP", EOS_SENSOR_TYPE_CAP, COL_ENV, false},
    {"Step Counter", "STEP", EOS_SENSOR_TYPE_STEP, COL_MOTION, false},
};

/* ===================================================================
 * Per-card widget references
 * =================================================================== */

typedef struct
{
    lv_obj_t *value_labels[3]; /* X/Y/Z for triple-axis, [0] for single */
    lv_obj_t *random_switch;
    lv_obj_t *fixed_inputs[3];
    int num_values;
} _card_t;

static lv_obj_t *_panel = NULL;
static _card_t _cards[SENSOR_COUNT];
static lv_timer_t *_refresh_timer = NULL;

/* ===================================================================
 * Helpers — widget factories
 * =================================================================== */

static lv_obj_t *_make_card(lv_obj_t *parent, uint32_t accent)
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
    lv_obj_set_style_pad_row(card, 6, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    /* Coloured left border accent strip */
    lv_obj_set_style_border_side(card, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(accent), 0);
    lv_obj_set_style_border_width(card, 3, 0);
    return card;
}

static lv_obj_t *_make_header(lv_obj_t *parent, const _sensor_info_t *info)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    char name_buf[48];
    snprintf(name_buf, sizeof(name_buf), "%s  %s", info->name, info->short_name);
    lv_obj_t *name = lv_label_create(row);
    lv_label_set_text(name, name_buf);
    lv_obj_set_style_text_color(name, lv_color_hex(info->color), 0);
    lv_obj_set_style_text_font(name, &lv_font_montserrat_14, 0);

    return name;
}

/* ===================================================================
 * Value formatting
 * =================================================================== */

static void _format_triple(int16_t x, int16_t y, int16_t z, char *xb, char *yb, char *zb, size_t cap)
{
    snprintf(xb, cap, "X: %d", x);
    snprintf(yb, cap, "Y: %d", y);
    snprintf(zb, cap, "Z: %d", z);
}

static void _format_single(const _sensor_info_t *info, const eos_sensor_data_t *data, char *buf, size_t cap)
{
    switch (info->type)
    {
        case EOS_SENSOR_TYPE_HR:
            snprintf(buf, cap, "%u bpm", (unsigned)data->hr.heart_rate);
            break;
        case EOS_SENSOR_TYPE_SPO2:
            snprintf(buf, cap, "%u%%", (unsigned)data->spo2.spo2);
            break;
        case EOS_SENSOR_TYPE_LIGHT:
            snprintf(buf, cap, "%u lux", (unsigned)data->light.lux);
            break;
        case EOS_SENSOR_TYPE_PROXIMITY:
            snprintf(buf, cap, "%u mm", (unsigned)data->proximity.distance_mm);
            break;
        case EOS_SENSOR_TYPE_ECG:
            snprintf(buf, cap, "%u raw", (unsigned)data->ecg.ecg);
            break;
        case EOS_SENSOR_TYPE_TEMP:
        {
            int32_t t = data->temp.temp;
            int32_t whole = t / 100;
            int32_t frac = t % 100;
            if (frac < 0)
                frac = -frac;
            snprintf(buf, cap, "%d.%02d C", (int)whole, (int)frac);
            break;
        }
        case EOS_SENSOR_TYPE_BARO:
            snprintf(buf, cap, "%d Pa", (int)data->baro.pressure);
            break;
        case EOS_SENSOR_TYPE_CAP:
            snprintf(buf, cap, "%u raw", (unsigned)data->cap.cap);
            break;
        case EOS_SENSOR_TYPE_STEP:
            snprintf(buf, cap, "%u steps", (unsigned)data->step.steps);
            break;
        default:
            snprintf(buf, cap, "---");
            break;
    }
}

static int16_t _clamp_i16(int value)
{
    if (value < INT16_MIN)
        return INT16_MIN;
    if (value > INT16_MAX)
        return INT16_MAX;
    return (int16_t)value;
}

static uint16_t _clamp_u16(int value)
{
    if (value < 0)
        return 0;
    if (value > UINT16_MAX)
        return UINT16_MAX;
    return (uint16_t)value;
}

static uint32_t _clamp_u32(int value)
{
    if (value < 0)
        return 0;
    return (uint32_t)value;
}

static void _set_fixed_data_value(eos_sensor_data_t *data, eos_sensor_type_t type, int value, int axis)
{
    switch (type)
    {
        case EOS_SENSOR_TYPE_ACCE:
            if (axis == 0)
                data->acce.x = _clamp_i16(value);
            else if (axis == 1)
                data->acce.y = _clamp_i16(value);
            else
                data->acce.z = _clamp_i16(value);
            break;
        case EOS_SENSOR_TYPE_GYRO:
            if (axis == 0)
                data->gyro.x = _clamp_i16(value);
            else if (axis == 1)
                data->gyro.y = _clamp_i16(value);
            else
                data->gyro.z = _clamp_i16(value);
            break;
        case EOS_SENSOR_TYPE_MAG:
            if (axis == 0)
                data->mag.x = _clamp_i16(value);
            else if (axis == 1)
                data->mag.y = _clamp_i16(value);
            else
                data->mag.z = _clamp_i16(value);
            break;
        case EOS_SENSOR_TYPE_HR:
            data->hr.heart_rate = _clamp_u16(value);
            break;
        case EOS_SENSOR_TYPE_SPO2:
            data->spo2.spo2 = _clamp_u16(value);
            break;
        case EOS_SENSOR_TYPE_LIGHT:
            data->light.lux = _clamp_u32(value);
            break;
        case EOS_SENSOR_TYPE_PROXIMITY:
            data->proximity.distance_mm = _clamp_u16(value);
            break;
        case EOS_SENSOR_TYPE_ECG:
            data->ecg.ecg = _clamp_u16(value);
            break;
        case EOS_SENSOR_TYPE_TEMP:
            data->temp.temp = (int32_t)value;
            break;
        case EOS_SENSOR_TYPE_BARO:
            data->baro.pressure = (int32_t)value;
            break;
        case EOS_SENSOR_TYPE_CAP:
            data->cap.cap = _clamp_u16(value);
            break;
        case EOS_SENSOR_TYPE_STEP:
            data->step.steps = _clamp_u32(value);
            break;
        default:
            break;
    }
}

static int _parse_input_value(lv_obj_t *ta)
{
    const char *txt = lv_textarea_get_text(ta);
    if (!txt || txt[0] == '\0' || (txt[0] == '-' && txt[1] == '\0'))
        return 0;

    return (int)strtol(txt, NULL, 10);
}

/* ===================================================================
 * Refresh timer — read all sensors and update labels
 * =================================================================== */

static void _refresh_timer_cb(lv_timer_t *t)
{
    (void)t;
    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        const _sensor_info_t *info = &_sensor_table[i];
        _card_t *card = &_cards[i];
        eos_sensor_raw_data_t raw;
        eos_result_t res = eos_sensor_read_latest(info->type, &raw);

        if (info->is_triple_axis)
        {
            char xb[16], yb[16], zb[16];
            if (res == EOS_OK)
            {
                int16_t x = 0, y = 0, z = 0;
                switch (info->type)
                {
                    case EOS_SENSOR_TYPE_ACCE:
                        x = raw.data.acce.x;
                        y = raw.data.acce.y;
                        z = raw.data.acce.z;
                        break;
                    case EOS_SENSOR_TYPE_GYRO:
                        x = raw.data.gyro.x;
                        y = raw.data.gyro.y;
                        z = raw.data.gyro.z;
                        break;
                    case EOS_SENSOR_TYPE_MAG:
                        x = raw.data.mag.x;
                        y = raw.data.mag.y;
                        z = raw.data.mag.z;
                        break;
                    default:
                        break;
                }
                _format_triple(x, y, z, xb, yb, zb, sizeof(xb));
            }
            else
            {
                snprintf(xb, sizeof(xb), "X: --");
                snprintf(yb, sizeof(yb), "Y: --");
                snprintf(zb, sizeof(zb), "Z: --");
            }
            if (card->value_labels[0])
                lv_label_set_text(card->value_labels[0], xb);
            if (card->value_labels[1])
                lv_label_set_text(card->value_labels[1], yb);
            if (card->value_labels[2])
                lv_label_set_text(card->value_labels[2], zb);
        }
        else
        {
            char buf[32];
            if (res == EOS_OK)
            {
                _format_single(info, &raw.data, buf, sizeof(buf));
            }
            else
            {
                snprintf(buf, sizeof(buf), "--");
            }
            if (card->value_labels[0])
                lv_label_set_text(card->value_labels[0], buf);
        }
    }
}

/* ===================================================================
 * Event handlers
 * =================================================================== */

static void _apply_fixed_value(int idx)
{
    if (idx < 0 || idx >= SENSOR_COUNT)
        return;

    const _sensor_info_t *info = &_sensor_table[idx];
    _card_t *card = &_cards[idx];
    eos_sensor_data_t data;
    memset(&data, 0, sizeof(data));

    for (int i = 0; i < card->num_values; i++)
    {
        _set_fixed_data_value(&data, info->type, _parse_input_value(card->fixed_inputs[i]), i);
    }

    eos_port_sensor_set_debug_fixed(info->type, &data);
}

static void _random_switch_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= SENSOR_COUNT)
        return;

    lv_obj_t *sw = lv_event_get_target(e);
    _card_t *card = &_cards[idx];
    bool random_enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);

    for (int i = 0; i < card->num_values; i++)
    {
        if (!card->fixed_inputs[i])
            continue;

        if (random_enabled)
            lv_obj_add_state(card->fixed_inputs[i], LV_STATE_DISABLED);
        else
            lv_obj_remove_state(card->fixed_inputs[i], LV_STATE_DISABLED);
    }

    if (random_enabled)
    {
        eos_port_sensor_set_debug_random(_sensor_table[idx].type);
    }
    else
    {
        _apply_fixed_value(idx);
    }
}

static void _apply_btn_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= SENSOR_COUNT)
        return;

    if (_cards[idx].random_switch)
        lv_obj_remove_state(_cards[idx].random_switch, LV_STATE_CHECKED);

    for (int i = 0; i < _cards[idx].num_values; i++)
    {
        if (_cards[idx].fixed_inputs[i])
            lv_obj_remove_state(_cards[idx].fixed_inputs[i], LV_STATE_DISABLED);
    }

    _apply_fixed_value(idx);
}

static void _sensor_debug_sub_cb(eos_sensor_type_t type, const eos_sensor_raw_data_t *data, void *user_data)
{
    (void)type;
    (void)data;
    (void)user_data;
}

/* ===================================================================
 * Card builder
 * =================================================================== */

static void _build_sensor_card(lv_obj_t *parent, int idx)
{
    const _sensor_info_t *info = &_sensor_table[idx];
    _card_t *card = &_cards[idx];

    lv_obj_t *cv = _make_card(parent, info->color);

    /* Header — sensor name */
    _make_header(cv, info);

    /* Value display */
    if (info->is_triple_axis)
    {
        card->num_values = 3;

        lv_obj_t *vrow = lv_obj_create(cv);
        lv_obj_set_size(vrow, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(vrow, 0, 0);
        lv_obj_set_style_border_width(vrow, 0, 0);
        lv_obj_set_style_bg_opa(vrow, LV_OPA_TRANSP, 0);
        lv_obj_set_style_radius(vrow, 0, 0);
        lv_obj_set_flex_flow(vrow, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(vrow, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        const char *labels[] = {"X: --", "Y: --", "Z: --"};
        for (int j = 0; j < 3; j++)
        {
            card->value_labels[j] = lv_label_create(vrow);
            lv_label_set_text(card->value_labels[j], labels[j]);
            lv_obj_set_style_text_color(card->value_labels[j], lv_color_hex(info->color), 0);
            lv_obj_set_style_text_font(card->value_labels[j], &lv_font_montserrat_16, 0);
        }
    }
    else
    {
        card->num_values = 1;

        card->value_labels[0] = lv_label_create(cv);
        lv_label_set_text(card->value_labels[0], "--");
        lv_obj_set_width(card->value_labels[0], LV_PCT(100));
        lv_obj_set_style_text_align(card->value_labels[0], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(card->value_labels[0], lv_color_hex(info->color), 0);
        lv_obj_set_style_text_font(card->value_labels[0], &lv_font_montserrat_30, 0);
        lv_obj_set_style_pad_ver(card->value_labels[0], 4, 0);
    }

    lv_obj_t *mode_row = lv_obj_create(cv);
    lv_obj_set_size(mode_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(mode_row, 0, 0);
    lv_obj_set_style_border_width(mode_row, 0, 0);
    lv_obj_set_style_bg_opa(mode_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(mode_row, 0, 0);
    lv_obj_set_flex_flow(mode_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mode_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *random_label = lv_label_create(mode_row);
    lv_label_set_text(random_label, "Random");
    lv_obj_set_style_text_color(random_label, lv_color_hex(COL_LABEL), 0);
    lv_obj_set_style_text_font(random_label, &lv_font_montserrat_14, 0);

    card->random_switch = lv_switch_create(mode_row);
    lv_obj_add_state(card->random_switch, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(card->random_switch, lv_color_hex(COL_CARD_BORDER), 0);
    lv_obj_set_style_bg_color(card->random_switch, lv_color_hex(info->color), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(card->random_switch, lv_color_hex(0xffffff), LV_PART_KNOB);
    lv_obj_add_event_cb(card->random_switch, _random_switch_cb, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)idx);

    lv_obj_t *fixed_row = lv_obj_create(cv);
    lv_obj_set_size(fixed_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(fixed_row, 0, 0);
    lv_obj_set_style_border_width(fixed_row, 0, 0);
    lv_obj_set_style_bg_opa(fixed_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(fixed_row, 0, 0);
    lv_obj_set_flex_flow(fixed_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(fixed_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(fixed_row, 6, 0);

    const char *input_names[] = {"X", "Y", "Z"};
    for (int j = 0; j < card->num_values; j++)
    {
        lv_obj_t *input = lv_textarea_create(fixed_row);
        lv_obj_set_width(input, info->is_triple_axis ? 58 : 150);
        lv_obj_set_height(input, 32);
        lv_textarea_set_one_line(input, true);
        lv_textarea_set_accepted_chars(input, "-0123456789");
        lv_textarea_set_max_length(input, 8);
        lv_textarea_set_placeholder_text(input, info->is_triple_axis ? input_names[j] : "Value");
        lv_textarea_set_text(input, "0");
        lv_obj_add_state(input, LV_STATE_DISABLED);
        lv_obj_set_style_bg_color(input, lv_color_hex(COL_INPUT_BG), 0);
        lv_obj_set_style_bg_opa(input, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(input, lv_color_hex(COL_CARD_BORDER), 0);
        lv_obj_set_style_border_width(input, 1, 0);
        lv_obj_set_style_radius(input, 6, 0);
        lv_obj_set_style_text_color(input, lv_color_hex(COL_TITLE), 0);
        lv_obj_set_style_text_font(input, &lv_font_montserrat_14, 0);
        lv_obj_set_style_pad_all(input, 6, 0);
        card->fixed_inputs[j] = input;
    }

    lv_obj_t *apply_btn = lv_button_create(fixed_row);
    lv_obj_set_size(apply_btn, 58, 32);
    lv_obj_set_style_bg_color(apply_btn, lv_color_hex(COL_BTN_BG), 0);
    lv_obj_set_style_bg_color(apply_btn, lv_color_hex(COL_BTN_BG_PR), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(apply_btn, 0, 0);
    lv_obj_set_style_shadow_width(apply_btn, 0, 0);
    lv_obj_set_style_radius(apply_btn, 6, 0);
    lv_obj_add_event_cb(apply_btn, _apply_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)idx);

    lv_obj_t *apply_lbl = lv_label_create(apply_btn);
    lv_label_set_text(apply_lbl, "Apply");
    lv_obj_set_style_text_color(apply_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(apply_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(apply_lbl);
}

/* ===================================================================
 * Public API
 * =================================================================== */

lv_obj_t *eos_debug_sensor_panel_create(lv_obj_t *parent)
{
    if (!parent)
        return NULL;
    if (_panel)
        return _panel;

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
    lv_label_set_text(title, "Sensor Debugger");
    lv_obj_set_style_text_color(title, lv_color_hex(COL_TITLE), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_bottom(title, 4, 0);

    /* ---- Sensor cards ---- */
    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        _build_sensor_card(_panel, i);
        eos_sensor_subscribe(_sensor_table[i].type,
                             _sensor_debug_sub_cb,
                             (void *)(intptr_t)i,
                             DEBUG_SAMPLE_INTERVAL_MS);
    }

    /* ---- Periodic refresh (10 Hz) ---- */
    _refresh_timer = lv_timer_create(_refresh_timer_cb, 100, NULL);

    /* Initial read */
    _refresh_timer_cb(NULL);

    return _panel;
}
