/**
 * @file eos_port_display.c
 * @brief Display port implementation for PC Simulator
 */

#include "eos_port_display.h"
#include "eos_dev_display.h"
#include "eos_headless_display.h"

#include <stdio.h>
#include "lvgl/lvgl.h"

extern lv_obj_t *brightness_mask;

static void _set_brightness(uint8_t brightness)
{
    float b = (float)((100.0 - (float)brightness) / 100.0 * 255.0);
    printf("[PortDisplay] brightness: %d\n", brightness);
    if (!brightness_mask || !lv_obj_is_valid(brightness_mask))
    {
        eos_headless_display_set_brightness(brightness);
        return;
    }
    if (b > 200)
    {
        b = 200;
    }
    lv_obj_set_style_opa(brightness_mask, b, 0);
}

static void _power_on(void)
{
    printf("[PortDisplay] Power on\n");
    if (brightness_mask && lv_obj_is_valid(brightness_mask))
    {
        lv_obj_remove_flag(brightness_mask, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        eos_headless_display_set_power(true);
    }
}

static void _power_off(void)
{
    printf("[PortDisplay] Power off\n");
    if (brightness_mask && lv_obj_is_valid(brightness_mask))
    {
        lv_obj_add_flag(brightness_mask, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        eos_headless_display_set_power(false);
    }
}

static const eos_dev_display_ops_t _ops = {
    .set_brightness = _set_brightness,
    .power_on = _power_on,
    .power_off = _power_off,
};

void eos_port_display_init(void)
{
    eos_dev_display_register(&_ops);
}
