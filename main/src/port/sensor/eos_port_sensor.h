/**
 * @file eos_port_sensor.h
 * @brief Sensor port header for PC Simulator
 */

#ifndef EOS_PORT_SENSOR_H
#define EOS_PORT_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "eos_dev_sensor.h"

void eos_port_sensor_init(void);
bool eos_port_sensor_set_debug_fixed(eos_sensor_type_t type, const eos_sensor_data_t *data);
void eos_port_sensor_set_debug_random(eos_sensor_type_t type);
bool eos_port_sensor_get_debug_fixed(eos_sensor_type_t type, eos_sensor_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* EOS_PORT_SENSOR_H */
