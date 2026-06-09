/**
 * @file eos_port_power.c
 * @brief Power port implementation for PC Simulator
 */

#include "eos_port_power.h"
#include "eos_dev_power.h"

#include <stdio.h>
#include "lvgl/lvgl.h"

extern lv_obj_t *brightness_mask;

static int _set_power(dev_power_state_t state)
{
    printf("[PortPower] set_power: %d\n", state);
    switch (state)
    {
    case DEV_POWER_STATE_OFF:
    case DEV_POWER_STATE_SLEEP:
        if (brightness_mask && lv_obj_is_valid(brightness_mask))
        {
            lv_obj_remove_flag(brightness_mask, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_opa(brightness_mask, LV_OPA_COVER, 0);
        }
        break;
    case DEV_POWER_STATE_ON:
        if (brightness_mask && lv_obj_is_valid(brightness_mask))
        {
            lv_obj_remove_flag(brightness_mask, LV_OBJ_FLAG_HIDDEN);
            float b = (float)((100.0 - 100.0) / 100.0 * 255.0);
            if (b > 200)
            {
                b = 200;
            }
            lv_obj_set_style_opa(brightness_mask, b, 0);
        }
        break;
    case DEV_POWER_STATE_AOD:
        if (brightness_mask && lv_obj_is_valid(brightness_mask))
        {
            lv_obj_remove_flag(brightness_mask, LV_OBJ_FLAG_HIDDEN);
            float b = (float)((100.0 - 20.0) / 100.0 * 255.0);
            if (b > 200)
            {
                b = 200;
            }
            lv_obj_set_style_opa(brightness_mask, b, 0);
        }
        break;
    default:
        return -1;
    }
    return 0;
}

static const eos_dev_power_ops_t _ops = {
    .set_power = _set_power,
};

void eos_port_power_init(void)
{
    eos_dev_power_register(&_ops);
}
