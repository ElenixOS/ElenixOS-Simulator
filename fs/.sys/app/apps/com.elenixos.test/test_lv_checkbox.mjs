/**
 * lv.checkbox coverage test
 *
 * Log rules: no Chinese characters. Each entry is [PASS] or [FAIL].
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose, runSuite, getTestView } from './framework.mjs';

export function suite() {
    runSuite('checkbox', () => {
    let scr = getTestView();
    let host;
    let title;
    let checkbox;
    let cbFired = false;

    test("constructor host new lv.obj(scr)", () => {
        host = new lv.obj(scr);
        host.setSize(360, 220);
        host.align(lv.ALIGN_CENTER, 0, 0);
        if (!host) throw new Error("null host");
    });

    test("title new lv.label(host)", () => {
        title = new lv.label(host);
        title.setText("lv.checkbox test");
        title.align(lv.ALIGN_TOP_MID, 0, 10);
    });

    test("constructor new lv.checkbox(host)", () => {
        checkbox = new lv.checkbox(host);
        checkbox.align(lv.ALIGN_TOP_LEFT, 24, 56);
        if (!checkbox) throw new Error("null checkbox");
    });

    test("setText + getText", () => {
        checkbox.setText("Enable feature");
        let text = checkbox.getText();
        if (typeof text !== "string") throw new Error("type=" + typeof text);
        if (text.indexOf("Enable") < 0) throw new Error("unexpected text=" + text);
    });

    test("prop text get/set", () => {
        checkbox.text = "Remember me";
        let text = checkbox.text;
        if (typeof text !== "string") throw new Error("type=" + typeof text);
        if (text !== "Remember me") throw new Error("unexpected text=" + text);
    });

    test("checked state add/remove", () => {
        checkbox.addState(lv.STATE_CHECKED);
        if (!checkbox.hasState(lv.STATE_CHECKED)) {
            throw new Error("expected checked=true");
        }

        checkbox.removeState(lv.STATE_CHECKED);
        if (checkbox.hasState(lv.STATE_CHECKED)) {
            throw new Error("expected checked=false");
        }
    });

    let cb = function (e) {
        cbFired = true;
        eos.console.log("[checkbox-test] callback fired");
    };

    test("addEventCb(VALUE_CHANGED) -> handle", () => {
        let dsc = checkbox.addEventCb(cb, lv.EVENT_VALUE_CHANGED, null);
        if (!dsc) throw new Error("null dsc");
    });

    test("sendEvent(VALUE_CHANGED) fires callback", () => {
        cbFired = false;
        checkbox.sendEvent(lv.EVENT_VALUE_CHANGED, null);
        if (!cbFired) throw new Error("callback not fired");
    });

    test("removeEventCb(cb) -> bool", () => {
        let r = checkbox.removeEventCb(cb);
        if (typeof r !== "boolean") throw new Error("type=" + typeof r);
    });

    });
}
