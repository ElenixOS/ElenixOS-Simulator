/**
 * lv.dropdown coverage test
 *
 * Log rules: no Chinese characters. Each entry is [PASS] or [FAIL].
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose, runSuite } from './framework.mjs';

export function suite() {
    runSuite('dropdown', () => {
    let scr = eos.view.active();
    let host;
    let title;
    let dd;
    let cbFired = false;

    test("constructor host new lv.obj(scr)", () => {
        host = new lv.obj(scr);
        host.setSize(360, 240);
        host.align(lv.ALIGN_CENTER, 0, 0);
        if (!host) throw new Error("null host");
    });

    test("title new lv.label(host)", () => {
        title = new lv.label(host);
        title.setText("lv.dropdown test");
        title.align(lv.ALIGN_TOP_MID, 0, 10);
    });

    test("constructor new lv.dropdown(host)", () => {
        dd = new lv.dropdown(host);
        dd.setWidth(220);
        dd.align(lv.ALIGN_TOP_MID, 0, 44);
        if (!dd) throw new Error("null dropdown");
    });

    test("setText + getText", () => {
        dd.setText("Choose one");
        let text = dd.getText();
        if (typeof text !== "string") throw new Error("type=" + typeof text);
    });

    test("setOptions + getOptions", () => {
        dd.setOptions("Apple\nBanana\nCherry");
        let opts = dd.getOptions();
        if (typeof opts !== "string") throw new Error("type=" + typeof opts);
        if (opts.indexOf("Banana") < 0) throw new Error("missing option");
    });

    test("getOptionCount", () => {
        let n = dd.getOptionCount();
        if (typeof n !== "number") throw new Error("type=" + typeof n);
        if (n < 3) throw new Error("count=" + n);
    });

    test("setSelected + getSelected", () => {
        dd.setSelected(1);
        let idx = dd.getSelected();
        if (typeof idx !== "number") throw new Error("type=" + typeof idx);
        if (idx !== 1) throw new Error("selected=" + idx);
    });

    test("setDir + getDir", () => {
        dd.setDir(lv.DIR_BOTTOM);
        let dir = dd.getDir();
        if (typeof dir !== "number") throw new Error("type=" + typeof dir);
    });

    test("setSymbol + getSymbol", () => {
        dd.setSymbol(lv.SYMBOL_DOWN);
        let symbol = dd.getSymbol();
        if (typeof symbol !== "string") throw new Error("type=" + typeof symbol);
    });

    test("setSelectedHighlight + getSelectedHighlight", () => {
        dd.setSelectedHighlight(true);
        let en = dd.getSelectedHighlight();
        if (typeof en !== "boolean") throw new Error("type=" + typeof en);
    });

    test("property text", () => {
        dd.text = "Pick fruit";
        if (typeof dd.text !== "string") throw new Error("type=" + typeof dd.text);
    });

    test("property options", () => {
        dd.options = "Red\nGreen\nBlue";
        let opts = dd.options;
        if (typeof opts !== "string") throw new Error("type=" + typeof opts);
        if (opts.indexOf("Green") < 0) throw new Error("unexpected options");
    });

    test("property selected", () => {
        dd.selected = 2;
        if (typeof dd.selected !== "number") throw new Error("type=" + typeof dd.selected);
        if (dd.selected !== 2) throw new Error("selected=" + dd.selected);
    });

    test("property dir", () => {
        dd.dir = lv.DIR_BOTTOM;
        if (typeof dd.dir !== "number") throw new Error("type=" + typeof dd.dir);
    });

    test("property symbol", () => {
        dd.symbol = lv.SYMBOL_UP;
        if (typeof dd.symbol !== "string") throw new Error("type=" + typeof dd.symbol);
    });

    test("property selectedHighlight", () => {
        dd.selectedHighlight = true;
        if (typeof dd.selectedHighlight !== "boolean") throw new Error("type=" + typeof dd.selectedHighlight);
    });

    test("getList returns object or null", () => {
        let list = dd.getList();
        if (list !== null && typeof list !== "object") {
            throw new Error("type=" + typeof list);
        }
    });

    test("close + isOpen", () => {
        dd.close();
        let open = dd.isOpen();
        if (typeof open !== "boolean") throw new Error("type=" + typeof open);
    });

    let cb = function (e) {
        cbFired = true;
        eos.console.log("[dropdown-test] callback fired");
    };

    test("addEventCb(VALUE_CHANGED)", () => {
        let dsc = dd.addEventCb(cb, lv.EVENT_VALUE_CHANGED, null);
        if (!dsc) throw new Error("null dsc");
    });

    test("sendEvent(VALUE_CHANGED) fires callback", () => {
        cbFired = false;
        dd.sendEvent(lv.EVENT_VALUE_CHANGED, null);
        if (!cbFired) throw new Error("callback not fired");
    });

    test("removeEventCb(cb)", () => {
        let r = dd.removeEventCb(cb);
        if (typeof r !== "boolean") throw new Error("type=" + typeof r);
    });

    });
}
