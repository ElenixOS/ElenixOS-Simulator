/**
 * @file eos_debug_sensor_panel.h
 * @brief Sensor debug panel — displays real-time values for all sensors.
 *
 * Created by eos_debug_sensor_panel_create(parent).  The panel fills
 * its parent container and shows one card per registered sensor type
 * (accelerometer, gyroscope, magnetometer, heart rate, SpO2, light,
 * proximity, ECG, temperature, barometer, capacitance, step counter).
 * Each card displays the sensor name, current value(s), a random-data
 * switch, and fixed-value inputs.
 */

#ifndef EOS_DEBUG_SENSOR_PANEL_H
#define EOS_DEBUG_SENSOR_PANEL_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create the sensor debug panel as a child of @p parent.
 *
 * The panel fills the parent entirely (LV_PCT(100) x LV_PCT(100))
 * and lays out sensor cards in a vertical flex column with
 * active vertical scrolling.
 *
 * @param parent  Container that owns the panel (non-NULL).
 * @return        The panel object (lv_obj_t *), or NULL on failure.
 */
lv_obj_t *eos_debug_sensor_panel_create(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif

#endif /* EOS_DEBUG_SENSOR_PANEL_H */
