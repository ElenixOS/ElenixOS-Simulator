/**
 * Test entry switchboard
 *
 * Change TEST_TARGET to run a different test quickly.
 */

import { run_obj_test } from './test_lv_obj.mjs';
import { run_button_test } from './test_lv_button.mjs';
import { run_label_test } from './test_lv_label.mjs';
import { run_arc_test } from './test_lv_arc.mjs';
import { run_bar_test } from './test_lv_bar.mjs';
import { run_screen_test } from './test_lv_screen.mjs';
import { run_color_test } from './test_lv_color.mjs';
import { run_timer_test } from './test_lv_timer.mjs';
import { run_anim_test } from './test_lv_anim.mjs';
import { run_buttonmatrix_test } from './test_lv_buttonmatrix.mjs';
import { run_calendar_test } from './test_lv_calendar.mjs';
import { run_chart_test } from './test_lv_chart.mjs';
import { run_canvas_test } from './test_lv_canvas.mjs';
import { run_checkbox_test } from './test_lv_checkbox.mjs';
import { run_dropdown_test } from './test_lv_dropdown.mjs';
import { run_image_test } from './test_lv_image.mjs';
import { run_imagebutton_test } from './test_lv_imagebutton.mjs';
import { run_sni_exit_cleanup_test } from './test_sni_exit_cleanup.mjs';
import { run_permission_test } from './test_eos_permission.mjs';
import { run_stress_test } from './test_eos_stress.mjs';

const TEST_TARGET = 'stress';

const TESTS = {
	obj: run_obj_test,
	button: run_button_test,
	label: run_label_test,
	arc: run_arc_test,
	bar: run_bar_test,
	screen: run_screen_test,
	color: run_color_test,
	timer: run_timer_test,
	anim: run_anim_test,
	buttonmatrix: run_buttonmatrix_test,
	calendar: run_calendar_test,
	chart: run_chart_test,
	canvas: run_canvas_test,
	checkbox: run_checkbox_test,
	dropdown: run_dropdown_test,
	image: run_image_test,
	imagebutton: run_imagebutton_test,
	sni_exit_cleanup: run_sni_exit_cleanup_test,
	permission: run_permission_test,
	stress: run_stress_test,
};

function _log(msg) {
	eos.console.log('[test-entry] ' + msg);
}

function run_selected_test() {
	let runner = TESTS[TEST_TARGET];
	if (typeof runner !== 'function') {
		let names = Object.keys(TESTS).join(', ');
		_log('[FAIL] unknown TEST_TARGET=' + TEST_TARGET);
		_log('[INFO] available targets: ' + names);
		return;
	}

	_log('[INFO] running test: ' + TEST_TARGET);
	runner();
}

run_selected_test();
