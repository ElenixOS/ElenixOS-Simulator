/**
 * lv.canvas coverage test
 *
 * Log rules: no Chinese characters. Each entry is [PASS] or [FAIL].
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose, runSuite } from './framework.mjs';

export function suite() {
    runSuite('canvas', () => {
    // Keep buffer by default so the drawing stays visible on screen.
    // Set to true only when explicitly testing release logic.
    const AUTO_FREE_BUFFER = false;

    let scr = eos.view.active();
    let container;
    let canvas;

    test("constructor new lv.canvas(scr)", () => {
        container = new lv.obj(scr);
        container.setSize(200, 200);
        container.setStyleBorderWidth(2, lv.PART_MAIN);
        container.setStyleBorderColor(lv.color.hex(0x222222), lv.PART_MAIN);
        container.setStyleBgColor(lv.color.hex(0xFFFFFF), lv.PART_MAIN);
        container.setStyleBgOpa(255, lv.PART_MAIN);
        canvas = new lv.canvas(container);
        canvas.setSize(200, 200);
        canvas.center();
        container.center();
        if (!canvas) throw new Error("null canvas");
    });

    test("initBuffer", () => {
        canvas.initBuffer(200, 200, lv.COLOR_FORMAT_NATIVE);
    });

    test("fillBg", () => {
        canvas.fillBg(lv.color.hex(0x1E88E5), 255);
    });

    test("setPx + getPx", () => {
        canvas.setPx(2, 2, lv.color.hex(0xFFCC00), 255);
        let px = canvas.getPx(2, 2);
        if (!px || typeof px !== "object") throw new Error("invalid px");
    });

    if (AUTO_FREE_BUFFER) {
        test("freeBuffer", () => {
            canvas.freeBuffer();
        });
    } else {
        log("[INFO] keep draw buffer for visual check (AUTO_FREE_BUFFER=false)");
    }

    });
}
