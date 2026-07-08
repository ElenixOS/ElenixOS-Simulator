/**
 * ElenixOS LVGL API Test Runner
 *
 * Run all test suites. Set RUN_SELECTED to true and modify SUITE_SELECTION
 * to run a specific suite.
 */

import { runSuite, report, log } from './framework.mjs';

// ---- Import existing test suites (ported from cn.sab1e.app) ----
import { suite as suite_obj } from './test_lv_obj.mjs';
import { suite as suite_button } from './test_lv_button.mjs';
import { suite as suite_label } from './test_lv_label.mjs';
import { suite as suite_arc } from './test_lv_arc.mjs';
import { suite as suite_bar } from './test_lv_bar.mjs';
import { suite as suite_screen } from './test_lv_screen.mjs';
import { suite as suite_color } from './test_lv_color.mjs';
import { suite as suite_timer } from './test_lv_timer.mjs';
import { suite as suite_anim } from './test_lv_anim.mjs';
import { suite as suite_buttonmatrix } from './test_lv_buttonmatrix.mjs';
import { suite as suite_calendar } from './test_lv_calendar.mjs';
import { suite as suite_chart } from './test_lv_chart.mjs';
import { suite as suite_canvas } from './test_lv_canvas.mjs';
import { suite as suite_checkbox } from './test_lv_checkbox.mjs';
import { suite as suite_dropdown } from './test_lv_dropdown.mjs';
import { suite as suite_image } from './test_lv_image.mjs';
import { suite as suite_imagebutton } from './test_lv_imagebutton.mjs';
import { suite as suite_sni_cleanup } from './test_sni_exit_cleanup.mjs';
import { suite as suite_permission } from './test_eos_permission.mjs';
import { suite as suite_stress } from './test_eos_stress.mjs';

// ---- Cleanup integrity test ----
import { suite as suite_cleanup } from './test_cleanup_integrity.mjs';

// ---- Import new test suites (P0-P3 widgets) ----
import { suite as suite_slider } from './test_lv_slider.mjs';
import { suite as suite_switch } from './test_lv_switch.mjs';
import { suite as suite_textarea } from './test_lv_textarea.mjs';
import { suite as suite_keyboard } from './test_lv_keyboard.mjs';
import { suite as suite_msgbox } from './test_lv_msgbox.mjs';
import { suite as suite_list } from './test_lv_list.mjs';
import { suite as suite_scale } from './test_lv_scale.mjs';
import { suite as suite_line } from './test_lv_line.mjs';
import { suite as suite_roller } from './test_lv_roller.mjs';
import { suite as suite_spinbox } from './test_lv_spinbox.mjs';
import { suite as suite_led } from './test_lv_led.mjs';
import { suite as suite_tabview } from './test_lv_tabview.mjs';
import { suite as suite_win } from './test_lv_win.mjs';
import { suite as suite_spangroup } from './test_lv_spangroup.mjs';
import { suite as suite_animimage } from './test_lv_animimage.mjs';
import { suite as suite_spinner } from './test_lv_spinner.mjs';
import { suite as suite_menu } from './test_lv_menu.mjs';

// ---- Configuration ----
const RUN_SELECTED = false;
const SUITE_SELECTION = 'obj';

const ALL_SUITES = {
    obj: suite_obj,
    button: suite_button,
    label: suite_label,
    arc: suite_arc,
    bar: suite_bar,
    screen: suite_screen,
    color: suite_color,
    timer: suite_timer,
    anim: suite_anim,
    buttonmatrix: suite_buttonmatrix,
    calendar: suite_calendar,
    chart: suite_chart,
    canvas: suite_canvas,
    checkbox: suite_checkbox,
    dropdown: suite_dropdown,
    image: suite_image,
    imagebutton: suite_imagebutton,
    sni_cleanup: suite_sni_cleanup,
    permission: suite_permission,
    stress: suite_stress,
    cleanup: suite_cleanup,
    // New P0-P3 widgets
    slider: suite_slider,
    switch: suite_switch,
    textarea: suite_textarea,
    keyboard: suite_keyboard,
    msgbox: suite_msgbox,
    list: suite_list,
    scale: suite_scale,
    line: suite_line,
    roller: suite_roller,
    spinbox: suite_spinbox,
    led: suite_led,
    tabview: suite_tabview,
    win: suite_win,
    spangroup: suite_spangroup,
    animimage: suite_animimage,
    spinner: suite_spinner,
    menu: suite_menu,
};

function runAllSuites() {
    log('[runner] Starting all test suites...');
    for (let name of Object.keys(ALL_SUITES)) {
        try {
            ALL_SUITES[name]();
        } catch (e) {
            log('[runner] Suite CRASH: ' + name + ' => ' + (e.message || e));
        }
    }
    report();
}

function runSelectedSuite(name) {
    let fn = ALL_SUITES[name];
    if (typeof fn === 'function') {
        fn();
    } else {
        log('[runner] Unknown suite: ' + name);
        log('[runner] Available: ' + Object.keys(ALL_SUITES).join(', '));
    }
}

if (RUN_SELECTED) {
    runSelectedSuite(SUITE_SELECTION);
} else {
    runAllSuites();
}
