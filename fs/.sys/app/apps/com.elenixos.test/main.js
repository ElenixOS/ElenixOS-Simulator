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

// ---- Cleanup integrity test ----
import { suite as suite_cleanup } from './test_cleanup_integrity.mjs';

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
    cleanup: suite_cleanup,
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
