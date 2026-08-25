/**
 * ElenixOS LVGL API Test Runner — Interactive List UI
 *
 * Shows a scrollable list of test suites. Tap any suite to run it.
 * Results are shown per-suite with pass / fail indicators.
 *
 * This app runs inside the system-created Activity for the test app.
 * Each test suite runs inside its own off-stack Activity for perfect
 * widget isolation — created/destroyed by framework.mjs.
 */

import {
    createSuiteRunner, getAllResults, clearAllResults,
    onSuiteStart, onSuiteDone, report, log
} from './framework.mjs';

// ---- Import test suites ----
import { suite as suite_obj } from './test_lv_obj.mjs';
import { suite as suite_button } from './test_lv_button.mjs';
import { suite as suite_label } from './test_lv_label.mjs';
import { suite as suite_arc } from './test_lv_arc.mjs';
import { suite as suite_bar } from './test_lv_bar.mjs';

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
import { suite as suite_port } from './test_port.mjs';
import { suite as suite_cleanup } from './test_cleanup_integrity.mjs';
import { suite as suite_activity } from './test_lv_activity.mjs';

// ---- Suite registry ----
const SUITES = [
    { key: 'obj',          name: 'lv.obj',           fn: suite_obj },
    { key: 'button',       name: 'lv.button',        fn: suite_button },
    { key: 'label',        name: 'lv.label',         fn: suite_label },
    { key: 'arc',          name: 'lv.arc',           fn: suite_arc },
    { key: 'bar',          name: 'lv.bar',           fn: suite_bar },

    { key: 'color',        name: 'lv.color',         fn: suite_color },
    { key: 'timer',        name: 'lv.timer',         fn: suite_timer },
    { key: 'anim',         name: 'lv.anim',          fn: suite_anim },
    { key: 'buttonmatrix', name: 'lv.buttonmatrix',  fn: suite_buttonmatrix },
    { key: 'calendar',     name: 'lv.calendar',      fn: suite_calendar },
    { key: 'chart',        name: 'lv.chart',         fn: suite_chart },
    { key: 'canvas',       name: 'lv.canvas',        fn: suite_canvas },
    { key: 'checkbox',     name: 'lv.checkbox',      fn: suite_checkbox },
    { key: 'dropdown',     name: 'lv.dropdown',      fn: suite_dropdown },
    { key: 'image',        name: 'lv.image',         fn: suite_image },
    { key: 'imagebutton',  name: 'lv.imagebutton',   fn: suite_imagebutton },
    { key: 'port',         name: 'port (time + fs)', fn: suite_port },
    { key: 'cleanup',      name: 'cleanup integrity',fn: suite_cleanup },
    { key: 'activity',     name: 'eos.activity',     fn: suite_activity },
];

// ---- Build runners ----
const RUNNERS = {};
for (let i = 0; i < SUITES.length; i++) {
    RUNNERS[SUITES[i].key] = createSuiteRunner(SUITES[i].key, SUITES[i].fn);
}

// ---- UI constants ----
const SW = eos.DISPLAY_WIDTH;   // 390
const SH = eos.DISPLAY_HEIGHT;  // 450
const HEADER_H = 52;
const PAD = 14;
const GAP = 8;
const ROW_H = 52;
const BTN_H = 46;
const STATS_H = 24;

// Vertical layout: header → run-all button → stats → scroll list
const BTN_Y    = HEADER_H + PAD;
const STATS_Y  = BTN_Y + BTN_H + 6;
const LIST_TOP = STATS_Y + STATS_H + PAD;
const LIST_H   = SH - LIST_TOP - PAD;

// ---- Colors (refined dark theme) ----
const C_BG       = lv.color.hex(0x000000);
const C_SURFACE  = lv.color.hex(0x1C1C1E);
const C_SURFACE_H= lv.color.hex(0x2C2C2E);
const C_PRESSED  = lv.color.hex(0x3A3A3C);
const C_ACCENT   = lv.color.hex(0x0A84FF);
const C_ACCENT_H = lv.color.hex(0x409CFF);
const C_TEXT     = lv.color.hex(0xFFFFFF);
const C_SUBTLE   = lv.color.hex(0x8E8E93);
const C_PASS     = lv.color.hex(0x30D158);
const C_FAIL     = lv.color.hex(0xFF453A);
const C_IDLE     = lv.color.hex(0x48484A);
const C_RUN      = lv.color.hex(0xFFD60A);
const C_BORDER   = lv.color.hex(0x38383C);

// ---- UI widget references ----
let runAllBtn = null;
let runAllLabel = null;
let statsLabel = null;
let rowWidgets = {};   // key → { container, nameLabel, statusDot, countLabel }
let isBusy = false;
let _lastClickTime = 0;
const DEBOUNCE_MS = 300;

// ---- Helpers ----

function styleRow(btn) {
    btn.setStyleBgColor(C_SURFACE, lv.PART_MAIN);
    btn.setStyleBorderWidth(0, lv.PART_MAIN);
    btn.setStyleRadius(14, lv.PART_MAIN);
    btn.setStyleShadowWidth(0, lv.PART_MAIN);
    btn.setStylePadAll(0, lv.PART_MAIN);
}

// ---- Update a single row's display ----

function refreshRow(key) {
    let w = rowWidgets[key];
    if (!w) return;
    let r = getAllResults()[key];
    let statusDot = w.statusDot;
    let countLabel = w.countLabel;

    if (!r) {
        // idle
        statusDot.setStyleBgColor(C_IDLE, lv.PART_MAIN);
        countLabel.setText('');
        // Reset row background
        w.container.setStyleBgColor(C_SURFACE, lv.PART_MAIN);
    } else if (r.status === 'running') {
        statusDot.setStyleBgColor(C_RUN, lv.PART_MAIN);
        countLabel.setText('...');
        w.container.setStyleBgColor(C_PRESSED, lv.PART_MAIN);
    } else if (r.status === 'pass') {
        statusDot.setStyleBgColor(C_PASS, lv.PART_MAIN);
        countLabel.setText(r.pass + '/' + r.total);
        w.container.setStyleBgColor(C_SURFACE, lv.PART_MAIN);
    } else {
        statusDot.setStyleBgColor(C_FAIL, lv.PART_MAIN);
        countLabel.setText(r.pass + '/' + r.total + ' (' + r.fail + 'F)');
        w.container.setStyleBgColor(C_SURFACE, lv.PART_MAIN);
    }
}

function refreshAllRows() {
    for (let i = 0; i < SUITES.length; i++) {
        refreshRow(SUITES[i].key);
    }
}

// ---- Visual tap feedback ----

function _flashRow(key) {
    let w = rowWidgets[key];
    if (!w) return;
    w.container.setStyleBgColor(C_PRESSED, lv.PART_MAIN);
}

// ---- Run one suite ----

function runOne(key) {
    if (isBusy) return;

    // Debounce
    let now = Date.now ? Date.now() : 0;
    if (now - _lastClickTime < DEBOUNCE_MS) return;
    _lastClickTime = now;

    let runner = RUNNERS[key];
    if (!runner) return;

    isBusy = true;
    _flashRow(key);

    // Set to running
    let r = getAllResults();
    r[key] = { name: key, status: 'running', pass: 0, fail: 0, error: 0, total: 0 };
    refreshRow(key);
    refreshRunAllState();

    // Use lv.timer to yield to LVGL so the UI refreshes before we block on tests.
    // The C layer auto-deletes one-shot timers (auto_delete=true) after the
    // callback returns — no explicit timer.delete() needed.
    // The runner's `done` callback fires AFTER the deferred test-container
    // cleanup (CLEANUP_DELAY_MS), so UI updates happen on a clean screen.
    let timer = new lv.timer(function () {
        try {
            runner(function () {
                try {
                    refreshRow(key);
                    refreshRunAllState();
                } finally {
                    isBusy = false;
                }
            });
        } catch (e) {
            log('[runner] Suite crash: ' + key + ' => ' + (e.message || e));
            refreshRow(key);
            refreshRunAllState();
            isBusy = false;
        }
    }, 10, null);
}

// ---- Run all suites sequentially ----

function runAll() {
    if (isBusy) return;

    // Debounce
    let now = Date.now ? Date.now() : 0;
    if (now - _lastClickTime < DEBOUNCE_MS) return;
    _lastClickTime = now;

    isBusy = true;
    clearAllResults();
    refreshAllRows();
    runAllLabel.setText('Running...');
    refreshRunAllState();

    let keys = [];
    for (let i = 0; i < SUITES.length; i++) keys.push(SUITES[i].key);
    let idx = 0;

    function next() {
        if (idx >= keys.length) {
            // Done — all suites completed
            runAllLabel.setText('Run All');
            refreshAllRows();
            refreshRunAllState();
            report();
            isBusy = false;
            return;
        }

        try {
            let key = keys[idx];
            let runner = RUNNERS[key];
            idx++;

            // Mark running
            let r = getAllResults();
            r[key] = { name: key, status: 'running', pass: 0, fail: 0, error: 0, total: 0 };
            refreshRow(key);
            refreshRunAllState();

            // Yield to LVGL so the "running" indicator renders.
            // The C layer auto-deletes one-shot timers after the callback returns.
            // The runner's `done` callback fires AFTER deferred cleanup
            // (CLEANUP_DELAY_MS), so the next suite starts on a clean screen.
            let timer = new lv.timer(function () {
                try {
                    runner(function () {
                        try {
                            refreshRow(key);
                            refreshRunAllState();
                        } finally {
                            // Schedule next suite after cleanup completes
                            let t2 = new lv.timer(function () {
                                next();
                            }, 10, null);
                        }
                    });
                } catch (e) {
                    log('[runner] Suite crash: ' + key + ' => ' + (e.message || e));
                    refreshRow(key);
                    refreshRunAllState();
                    let t2 = new lv.timer(function () {
                        next();
                    }, 10, null);
                }
            }, 10, null);
        } catch (e) {
            log('[runner] Failed to start suite: ' + (e.message || e));
            isBusy = false;
        }
    }

    next();
}

function refreshRunAllState() {
    if (!runAllLabel) return;
    let results = getAllResults();
    let keys = Object.keys(results);
    let runCount = 0;
    let passCount = 0;
    let failCount = 0;
    let totalTests = 0;
    for (let i = 0; i < keys.length; i++) {
        let r = results[keys[i]];
        if (r && r.status !== 'idle') {
            runCount++;
            passCount += r.pass || 0;
            failCount += r.fail || 0;
            totalTests += r.total || 0;
        }
    }
    if (statsLabel) {
        if (runCount === 0) {
            statsLabel.setText('Ready · ' + SUITES.length + ' suites');
        } else {
            statsLabel.setText(passCount + ' passed · ' + failCount + ' failed · ' + runCount + '/' + SUITES.length + ' suites');
        }
    }
}

// ---- Build the UI ----

function buildUi() {
    let view = eos.view.active();

    // Hide app header — we draw our own
    let activity = eos.activity.current();
    eos.activity.setAppHeaderVisible(activity, false);

    // Root background
    let root = new lv.obj(view);
    root.setSize(SW, SH);
    root.setPos(0, 0);
    root.setStyleBgColor(C_BG, lv.PART_MAIN);
    root.setStyleBorderWidth(0, lv.PART_MAIN);
    root.setStyleRadius(0, lv.PART_MAIN);
    root.setStylePadAll(0, lv.PART_MAIN);
    root.setStyleLayout(lv.LAYOUT_NONE, lv.PART_MAIN);
    root.removeFlag(lv.OBJ_FLAG_SCROLLABLE);

    // ---- Title bar ----
    let titleBar = new lv.obj(root);
    titleBar.setSize(SW, HEADER_H);
    titleBar.setPos(0, 0);
    titleBar.setStyleBgColor(C_BG, lv.PART_MAIN);
    titleBar.setStyleBorderWidth(0, lv.PART_MAIN);
    titleBar.setStyleRadius(0, lv.PART_MAIN);
    titleBar.setStylePadAll(0, lv.PART_MAIN);
    titleBar.setStyleBorderSide(0x04, lv.PART_MAIN);   // bottom
    titleBar.setStyleBorderColor(C_BORDER, lv.PART_MAIN);
    titleBar.setStyleBorderWidth(1, lv.PART_MAIN);
    titleBar.removeFlag(lv.OBJ_FLAG_SCROLLABLE);

    let title = new lv.label(titleBar);
    title.setText('LVGL Test Suite');
    title.setStyleTextColor(C_TEXT, lv.PART_MAIN);
    title.setFontSize(eos.FONT_SIZE_MEDIUM);
    title.align(lv.ALIGN_LEFT_MID, PAD, 0);

    let suiteCount = new lv.label(titleBar);
    suiteCount.setText(SUITES.length + ' suites');
    suiteCount.setStyleTextColor(C_SUBTLE, lv.PART_MAIN);
    suiteCount.setFontSize(eos.FONT_SIZE_SMALL);
    suiteCount.align(lv.ALIGN_RIGHT_MID, -PAD, 0);

    // ---- Run All button ----
    let btnW = SW - PAD * 2;

    runAllBtn = new lv.button(root);
    runAllBtn.setSize(btnW, BTN_H);
    runAllBtn.setPos(PAD, BTN_Y);
    runAllBtn.setStyleBgColor(C_ACCENT, lv.PART_MAIN);
    runAllBtn.setStyleBorderWidth(0, lv.PART_MAIN);
    runAllBtn.setStyleRadius(14, lv.PART_MAIN);
    runAllBtn.setStyleShadowWidth(0, lv.PART_MAIN);
    runAllBtn.setStylePadAll(0, lv.PART_MAIN);

    runAllLabel = new lv.label(runAllBtn);
    runAllLabel.setText('Run All');
    runAllLabel.setStyleTextColor(C_TEXT, lv.PART_MAIN);
    runAllLabel.setFontSize(eos.FONT_SIZE_MEDIUM);
    runAllLabel.center();

    runAllBtn.addEventCb(function () {
        runAll();
    }, lv.EVENT_CLICKED, null);

    // ---- Stats bar (positioned above the scroll list, NOT behind it) ----
    statsLabel = new lv.label(root);
    statsLabel.setSize(btnW, STATS_H);
    statsLabel.setPos(PAD, STATS_Y);
    statsLabel.setText('Ready · ' + SUITES.length + ' suites');
    statsLabel.setStyleTextColor(C_SUBTLE, lv.PART_MAIN);
    statsLabel.setFontSize(eos.FONT_SIZE_SMALL);
    statsLabel.removeFlag(lv.OBJ_FLAG_SCROLLABLE);

    // ---- Scrollable list area (below stats) ----
    let contentH = SUITES.length * (ROW_H + GAP);

    let scroll = new lv.obj(root);
    scroll.setSize(SW - PAD * 2, LIST_H);
    scroll.setPos(PAD, LIST_TOP);
    scroll.setStyleBgColor(C_BG, lv.PART_MAIN);
    scroll.setStyleBorderWidth(0, lv.PART_MAIN);
    scroll.setStyleRadius(0, lv.PART_MAIN);
    scroll.setStylePadAll(0, lv.PART_MAIN);
    scroll.setStylePadBottom(PAD, lv.PART_MAIN);
    scroll.addFlag(lv.OBJ_FLAG_SCROLLABLE);
    scroll.setScrollbarMode(lv.SCROLLBAR_MODE_OFF);

    // ---- Create suite rows inside scroll ----
    let dotSize = 10;
    for (let i = 0; i < SUITES.length; i++) {
        let s = SUITES[i];
        let rowY = i * (ROW_H + GAP);
        let rowW = SW - PAD * 2;

        let row = new lv.button(scroll);
        row.setSize(rowW, ROW_H);
        row.setPos(0, rowY);
        styleRow(row);
        // Explicitly ensure the row is clickable — critical inside scrollable parent
        row.addFlag(lv.OBJ_FLAG_CLICKABLE);

        // Suite name label
        let nameLabel = new lv.label(row);
        nameLabel.setText(s.name);
        nameLabel.setStyleTextColor(C_TEXT, lv.PART_MAIN);
        nameLabel.setFontSize(eos.FONT_SIZE_SMALL);
        nameLabel.align(lv.ALIGN_LEFT_MID, 14, 0);
        // Label should not intercept clicks on the row button
        nameLabel.removeFlag(lv.OBJ_FLAG_CLICKABLE);

        // Pass/fail count label (right side)
        let countLabel = new lv.label(row);
        countLabel.setText('');
        countLabel.setStyleTextColor(C_SUBTLE, lv.PART_MAIN);
        countLabel.setFontSize(eos.FONT_SIZE_SMALL - 2);
        countLabel.align(lv.ALIGN_RIGHT_MID, -(dotSize + 18), 0);
        countLabel.removeFlag(lv.OBJ_FLAG_CLICKABLE);

        // Status dot (rightmost)
        let dot = new lv.obj(row);
        dot.setSize(dotSize, dotSize);
        dot.setStyleBgColor(C_IDLE, lv.PART_MAIN);
        dot.setStyleBorderWidth(0, lv.PART_MAIN);
        dot.setStyleRadius(dotSize / 2, lv.PART_MAIN);
        dot.align(lv.ALIGN_RIGHT_MID, -12, 0);
        dot.removeFlag(lv.OBJ_FLAG_CLICKABLE);

        // Store references
        rowWidgets[s.key] = {
            container: row,
            nameLabel: nameLabel,
            statusDot: dot,
            countLabel: countLabel,
        };

        // Click handler — register on both CLICKED and RELEASED to
        // work reliably inside scrollable containers
        (function (k) {
            function handleClick() {
                if (!isBusy) {
                    let r = getAllResults();
                    delete r[k];
                    runOne(k);
                }
            }
            row.addEventCb(handleClick, lv.EVENT_CLICKED, null);
        })(s.key);
    }

    // Force scroll area to know about all content
    let spacer = new lv.obj(scroll);
    spacer.setSize(1, contentH);
    spacer.setPos(0, 0);
    spacer.setStyleBgOpa(0, lv.PART_MAIN);
    spacer.setStyleBorderWidth(0, lv.PART_MAIN);
    spacer.removeFlag(lv.OBJ_FLAG_CLICKABLE);

    refreshRunAllState();
}

// ---- Start ----

log('[test-ui] Building UI...');
try {
    buildUi();
    log('[test-ui] Ready. ' + SUITES.length + ' suites available.');
} catch (e) {
    log('[test-ui] FATAL: Failed to build UI: ' + (e.message || e));
}
