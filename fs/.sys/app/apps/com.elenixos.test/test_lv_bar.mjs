/**
 * lv.bar coverage test
 *
 * Log rules: no Chinese characters. Each entry is [PASS] or [FAIL].
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose, runSuite, getTestView } from './framework.mjs';

export function suite() {
    runSuite('bar', () => {
    let scr = getTestView();
    let bar;

    test("constructor new lv.bar(scr)", () => {
        bar = new lv.bar(scr);
        bar.setSize(220, 20);
        bar.align(lv.ALIGN_TOP_MID, 0, 12);
        if (!bar) throw new Error("null bar");
    });

    test("setRange", () => {
        bar.setRange(0, 100);
    });

    test("setValue", () => {
        bar.setValue(40, lv.ANIM_OFF);
    });

    test("setStartValue", () => {
        bar.setStartValue(10, lv.ANIM_OFF);
    });

    test("setMode", () => {
        bar.setMode(0);
    });

    test("setOrientation", () => {
        bar.setOrientation(0);
    });

    test("getValue/getStartValue", () => {
        if (typeof bar.getValue() !== "number") throw new Error("value");
        if (typeof bar.getStartValue() !== "number") throw new Error("startValue");
    });

    test("getRange getters", () => {
        if (typeof bar.getMinValue() !== "number") throw new Error("minValue");
        if (typeof bar.getMaxValue() !== "number") throw new Error("maxValue");
    });

    test("getMode/getOrientation/isSymmetrical", () => {
        if (typeof bar.getMode() !== "number") throw new Error("mode");
        if (typeof bar.getOrientation() !== "number") throw new Error("orientation");
        if (typeof bar.isSymmetrical() !== "boolean") throw new Error("symmetrical");
    });

    test("prop mode", () => {
        bar.mode = 0;
        if (typeof bar.mode !== "number") throw new Error("type=" + typeof bar.mode);
    });

    test("prop orientation", () => {
        bar.orientation = 0;
        if (typeof bar.orientation !== "number") throw new Error("type=" + typeof bar.orientation);
    });

    test("prop read-only values", () => {
        if (typeof bar.value !== "number") throw new Error("value");
        if (typeof bar.startValue !== "number") throw new Error("startValue");
        if (typeof bar.minValue !== "number") throw new Error("minValue");
        if (typeof bar.maxValue !== "number") throw new Error("maxValue");
    });

    });
}
