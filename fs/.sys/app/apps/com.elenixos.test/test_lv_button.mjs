/**
 * lv.button coverage test
 *
 * Current binding note:
 * - lv.button has no button-exclusive instance methods.
 * - This file tests the button constructor and callback behavior on a button
 *   instance via inherited lv.obj event APIs.
 *
 * Log rules: no Chinese characters. Each entry is [PASS] or [FAIL].
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose, runSuite, getTestView } from './framework.mjs';

export function suite() {
    runSuite('button', () => {
    let scr = getTestView();

    let host;
    let btn;
    let title;
    let info;

    let cbFired = false;
    let eventCountBefore = 0;

    log("info: lv.button exposes constructor only in current binding");
    log("info: callback check uses inherited obj event APIs");

    test("constructor host new lv.obj(scr)", () => {
        host = new lv.obj(scr);
        host.setSize(360, 220);
        host.align(lv.ALIGN_CENTER, 0, 0);
        if (!host) throw new Error("null host");
    });

    test("title new lv.label(host)", () => {
        title = new lv.label(host);
        title.setText("lv.button test");
        title.align(lv.ALIGN_TOP_MID, 0, 10);
    });

    test("info new lv.label(host)", () => {
        info = new lv.label(host);
        info.setText("last: none");
        info.align(lv.ALIGN_TOP_MID, 0, 36);
    });

    test("constructor new lv.button(host)", () => {
        btn = new lv.button(host);
        btn.setSize(180, 64);
        btn.align(lv.ALIGN_CENTER, 0, 20);
        if (!btn) throw new Error("null button");
    });

    test("button child label", () => {
        let text = new lv.label(btn);
        text.setText("Click Me");
        text.center();
    });

    test("addFlag(CLICKABLE)", () => {
        btn.addFlag(lv.OBJ_FLAG_CLICKABLE);
    });

    test("hasFlag(CLICKABLE) -> true", () => {
        if (!btn.hasFlag(lv.OBJ_FLAG_CLICKABLE)) {
            throw new Error("expected clickable");
        }
    });

    test("addFlag(CHECKABLE)", () => {
        btn.addFlag(lv.OBJ_FLAG_CHECKABLE);
    });

    test("hasFlag(CHECKABLE) -> true", () => {
        if (!btn.hasFlag(lv.OBJ_FLAG_CHECKABLE)) {
            throw new Error("expected checkable");
        }
    });

    test("getEventCount before add -> number", () => {
        eventCountBefore = btn.getEventCount();
        if (typeof eventCountBefore !== "number") {
            throw new Error("type=" + typeof eventCountBefore);
        }
    });

    let cb = function (e) {
        cbFired = true;
        info.setText("last: clicked");
        eos.console.log("[button-test] callback fired");
    };

    test("addEventCb(CLICKED) -> handle", () => {
        let dsc = btn.addEventCb(cb, lv.EVENT_CLICKED, null);
        if (!dsc) throw new Error("null dsc");
    });

    test("getEventCount after add -> increased", () => {
        let n = btn.getEventCount();
        if (typeof n !== "number") throw new Error("type=" + typeof n);
        if (n < eventCountBefore + 1) {
            throw new Error("count=" + n + ", before=" + eventCountBefore);
        }
    });

    test("sendEvent(CLICKED) fires callback", () => {
        cbFired = false;
        btn.sendEvent(lv.EVENT_CLICKED, null);
        if (!cbFired) throw new Error("callback not fired");
    });

    test("removeEventCb(cb) -> bool", () => {
        let r = btn.removeEventCb(cb);
        if (typeof r !== "boolean") throw new Error("type=" + typeof r);
    });

    });
}
