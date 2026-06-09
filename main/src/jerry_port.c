/**
 * @file jerry_port.c
 * @brief JerryScript Port for ElenixOS PC Simulator
 */

#include <stdint.h>
#include "jerryscript-port.h"

double jerry_port_current_time(void)
{
    return (double)0;
}

int32_t jerry_port_local_tza(double unix_ms)
{
    return 480 * (int32_t)unix_ms;
}
