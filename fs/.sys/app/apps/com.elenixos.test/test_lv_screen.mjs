/**
 * lv.screen coverage test
 *
 * Log rules: no Chinese characters. Each entry is [PASS] or [FAIL].
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose, runSuite } from './framework.mjs';

export function suite() {
    runSuite('screen', () => {
    let scr;

    test("static active", () => {
        scr = lv.screen.active();
        if (!scr) throw new Error("null screen");
    });

    test("screen behaves as obj handle", () => {
        if (typeof scr.getWidth() !== "number") throw new Error("width");
        if (typeof scr.getHeight() !== "number") throw new Error("height");
    });

    test("screen child create", () => {
        let obj = new lv.obj(scr);
        obj.setSize(40, 40);
        obj.align(lv.ALIGN_TOP_LEFT, 0, 0);
        obj.delete();
    });

    });
}
