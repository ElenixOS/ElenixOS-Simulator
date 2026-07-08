/**
 * lv.arc coverage test
 *
 * Log rules: no Chinese characters. Each entry is [PASS] or [FAIL].
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose, runSuite } from './framework.mjs';

export function suite() {
    runSuite('arc', () => {
    let scr = eos.view.active();
    let arc;

    test("constructor new lv.arc(scr)", () => {
        arc = new lv.arc(scr);
        arc.setSize(180, 180);
        arc.center();
        if (!arc) throw new Error("null arc");
    });

    test("prop bgStartAngle", () => {
        arc.bgStartAngle = 15;
    });

    test("prop bgEndAngle", () => {
        arc.bgEndAngle = 300;
    });

    test("prop changeRate", () => {
        arc.changeRate = 720;
    });

    test("prop startAngle", () => {
        arc.startAngle = 30;
    });

    test("prop endAngle", () => {
        arc.endAngle = 240;
    });

    test("prop knobOffset", () => {
        arc.knobOffset = 6;
        if (typeof arc.knobOffset !== "number") throw new Error("type=" + typeof arc.knobOffset);
    });

    test("prop mode", () => {
        arc.mode = 0;
        if (typeof arc.mode !== "number") throw new Error("type=" + typeof arc.mode);
    });

    test("prop rotation", () => {
        arc.rotation = 90;
        if (typeof arc.rotation !== "number") throw new Error("type=" + typeof arc.rotation);
    });

    test("prop value", () => {
        arc.value = 50;
        if (typeof arc.value !== "number") throw new Error("type=" + typeof arc.value);
    });

    test("getters angle/value/range", () => {
        if (typeof arc.angleStart !== "number") throw new Error("angleStart");
        if (typeof arc.angleEnd !== "number") throw new Error("angleEnd");
        if (typeof arc.bgAngleStart !== "number") throw new Error("bgAngleStart");
        if (typeof arc.bgAngleEnd !== "number") throw new Error("bgAngleEnd");
        if (typeof arc.minValue !== "number") throw new Error("minValue");
        if (typeof arc.maxValue !== "number") throw new Error("maxValue");
    });

    });
}
