/**
 * lv.imagebutton coverage test
 *
 * Log rules: no Chinese characters. Each entry is [PASS] or [FAIL].
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose, runSuite } from './framework.mjs';

export function suite() {
    runSuite('imagebutton', () => {
    let scr = eos.view.active();
    let host;
    let title;
    let btn;
    let clicked = false;

    test("constructor host new lv.obj(scr)", () => {
        host = new lv.obj(scr);
        host.setSize(360, 240);
        host.align(lv.ALIGN_CENTER, 0, 0);
        if (!host) throw new Error("null host");
    });

    test("title new lv.label(host)", () => {
        title = new lv.label(host);
        title.setText("lv.imagebutton test");
        title.align(lv.ALIGN_TOP_MID, 0, 10);
    });

    test("constructor new lv.imagebutton(host)", () => {
        btn = new lv.imagebutton(host);
        btn.align(lv.ALIGN_TOP_MID, 0, 54);
        if (!btn) throw new Error("null imagebutton");
    });

    test("getSrcMiddle(released)", () => {
        let src = btn.getSrcMiddle(lv.IMAGEBUTTON_STATE_RELEASED);
        if (src === undefined) throw new Error("undefined middle src");
    });

    test("getSrcMiddle(pressed)", () => {
        let src = btn.getSrcMiddle(lv.IMAGEBUTTON_STATE_PRESSED);
        if (src === undefined) throw new Error("undefined pressed src");
    });

    test("getSrcLeft/getSrcRight allow null", () => {
        let left = btn.getSrcLeft(lv.IMAGEBUTTON_STATE_RELEASED);
        let right = btn.getSrcRight(lv.IMAGEBUTTON_STATE_RELEASED);
        if (left === undefined) throw new Error("undefined left src");
        if (right === undefined) throw new Error("undefined right src");
    });

    test("setState(method)", () => {
        btn.setState(lv.IMAGEBUTTON_STATE_PRESSED);
    });

    test("state property set", () => {
        btn.state = lv.IMAGEBUTTON_STATE_RELEASED;
    });

    let cb = function (e) {
        clicked = true;
        eos.console.log("[imagebutton-test] callback fired");
    };

    test("addEventCb(CLICKED)", () => {
        let dsc = btn.addEventCb(cb, lv.EVENT_CLICKED, null);
        if (!dsc) throw new Error("null dsc");
    });

    test("sendEvent(CLICKED) fires callback", () => {
        clicked = false;
        btn.sendEvent(lv.EVENT_CLICKED, null);
        if (!clicked) throw new Error("callback not fired");
    });

    test("removeEventCb(cb)", () => {
        let r = btn.removeEventCb(cb);
        if (typeof r !== "boolean") throw new Error("type=" + typeof r);
    });

    });
}
