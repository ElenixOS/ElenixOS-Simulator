/**
 * @file eos_debug_anim_panel.h
 * @brief Visual animation debug panel — lives in a parent container
 *        provided by the simulator layout (right-side panel).
 */

#ifndef EOS_DEBUG_ANIM_PANEL_H
#define EOS_DEBUG_ANIM_PANEL_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Create the debug panel as a child of @p parent.
     *
     * The panel fills the parent entirely (LV_PCT(100) × LV_PCT(100))
     * and lays out its controls in a vertical flex column.
     *
     * @param parent  Container that owns the panel (non-NULL).
     * @return        The panel object (lv_obj_t *), or NULL on failure.
     */
    lv_obj_t *eos_debug_anim_panel_create(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif

#endif /* EOS_DEBUG_ANIM_PANEL_H */
