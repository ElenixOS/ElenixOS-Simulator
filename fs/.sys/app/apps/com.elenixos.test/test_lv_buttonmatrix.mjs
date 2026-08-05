/**
 * lv.buttonmatrix coverage test
 *
 * Log rules: no Chinese characters. Each entry is [PASS] or [FAIL].
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose, runSuite, getTestView } from './framework.mjs';

export function suite() {
    runSuite('buttonmatrix', () => {
    let scr = getTestView();
    let btnm;
    let rows = [
        ["1", "2", "3"],
        ["A", "B", "C"],
        ["OK", "Cancel"]
    ];
    let buttonCount = 8;
    let ctrlCheckable = 128;

    test("constructor new lv.buttonmatrix(scr)", () => {
        btnm = new lv.buttonmatrix(scr);
        btnm.setSize(300, 150);
        btnm.align(lv.ALIGN_TOP_MID, 0, 12);
        if (!btnm) throw new Error("null buttonmatrix");
    });

    test("setMap", () => {
        btnm.setMap(rows);
    });

    test("setCtrlMap", () => {
        let ctrlMap = [];
        for (let i = 0; i < buttonCount; i++) ctrlMap.push(0);
        btnm.setCtrlMap(ctrlMap);
    });

    test("setSelectedButton", () => {
        btnm.setSelectedButton(0);
    });

    test("setButtonCtrl + hasButtonCtrl", () => {
        btnm.setButtonCtrl(0, ctrlCheckable);
        let value = btnm.hasButtonCtrl(0, ctrlCheckable);
        if (typeof value !== "boolean") throw new Error("type=" + typeof value);
    });

    test("clearButtonCtrl", () => {
        btnm.clearButtonCtrl(0, ctrlCheckable);
    });

    test("setButtonCtrlAll + clearButtonCtrlAll", () => {
        btnm.setButtonCtrlAll(ctrlCheckable);
        btnm.clearButtonCtrlAll(ctrlCheckable);
    });

    test("setButtonWidth", () => {
        btnm.setButtonWidth(0, 2);
    });

    test("setOneChecked + getOneChecked", () => {
        btnm.setOneChecked(true);
        let value = btnm.getOneChecked();
        if (typeof value !== "boolean") throw new Error("type=" + typeof value);
    });

    test("getSelectedButton + getButtonText", () => {
        let id = btnm.getSelectedButton();
        if (typeof id !== "number") throw new Error("id");
        let text = btnm.getButtonText(0);
        if (typeof text !== "string") throw new Error("text");
    });

    test("prop map", () => {
        btnm.map = rows;
    });

    test("prop ctrlMap", () => {
        let ctrlMap = [];
        for (let i = 0; i < buttonCount; i++) ctrlMap.push(0);
        btnm.ctrlMap = ctrlMap;
    });

    test("prop selectedButton", () => {
        btnm.selectedButton = 1;
        if (typeof btnm.selectedButton !== "number") throw new Error("type=" + typeof btnm.selectedButton);
    });

    test("prop oneChecked", () => {
        btnm.oneChecked = true;
        if (typeof btnm.oneChecked !== "boolean") throw new Error("type=" + typeof btnm.oneChecked);
    });

    test("prop buttonCtrlAll", () => {
        btnm.buttonCtrlAll = ctrlCheckable;
    });

    test("callback registration", () => {
        let dsc = btnm.addEventCb(function () {
            eos.console.log("[buttonmatrix-test] callback fired");
        }, lv.EVENT_VALUE_CHANGED, null);
        if (!dsc) throw new Error("null dsc");
    });

    });
}
