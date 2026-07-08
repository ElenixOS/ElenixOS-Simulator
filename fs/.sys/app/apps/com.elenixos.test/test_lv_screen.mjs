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

    test("screen child create", () => {
        let obj = new lv.obj(scr);
        obj.setSize(40, 40);
        obj.align(lv.ALIGN_TOP_LEFT, 0, 0);
        obj.delete();
    });

    });
}
