/**
 * lv.color coverage test
 *
 * Log rules: no Chinese characters. Each entry is [PASS] or [FAIL].
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose, runSuite } from './framework.mjs';

export function suite() {
    runSuite('color', () => {
    let scr = eos.view.active();
    let obj;
    let color;

    test("constructor helper obj", () => {
        obj = new lv.obj(scr);
        obj.setSize(80, 80);
        obj.align(lv.ALIGN_TOP_MID, 0, 12);
    });

    test("static hex returns object", () => {
        color = lv.color.hex(0x336699);
        if (!color || typeof color !== "object") throw new Error("type=" + typeof color);
    });

    test("color usable in setStyleBgColor", () => {
        obj.setStyleBgColor(color, lv.PART_MAIN);
    });

    test("second hex color usable in setStyleBorderColor", () => {
        obj.setStyleBorderWidth(2, lv.PART_MAIN);
        obj.setStyleBorderColor(lv.color.hex(0xFF6600), lv.PART_MAIN);
    });

    });
}
