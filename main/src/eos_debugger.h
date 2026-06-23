#ifndef EOS_DEBUGGER_H
#define EOS_DEBUGGER_H

#include <stdint.h>
#include <stdbool.h>
#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void eos_debugger_init(lv_display_t *main_disp, lv_obj_t *outer_container, lv_obj_t *watch_area);
void eos_debugger_toggle(void);
void eos_debugger_update(void);
bool eos_debugger_is_expanded(void);
void eos_debugger_deinit(void);

#ifdef __cplusplus
}
#endif
#endif
