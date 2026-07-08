/**
 * lv.label coverage test
 *
 * Log rules: no Chinese characters. Each entry is [PASS] or [FAIL].
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose, runSuite } from './framework.mjs';

export function suite() {
    runSuite('label', () => {
    let scr = eos.view.active();
    let label;

    test("constructor new lv.label(scr)", () => {
        label = new lv.label(scr);
        label.setWidth(220);
        label.align(lv.ALIGN_TOP_MID, 0, 8);
        if (!label) throw new Error("null label");
    });

    test("setText + getText", () => {
        label.setText("Hello label");
        let text = label.getText();
        if (typeof text !== "string") throw new Error("type=" + typeof text);
    });

    test("setLongMode + getLongMode", () => {
        label.setLongMode(lv.LABEL_LONG_WRAP);
        let mode = label.getLongMode();
        if (typeof mode !== "number") throw new Error("type=" + typeof mode);
    });

    test("setTextSelectionStart + getTextSelectionStart", () => {
        label.setText("Selectable text");
        label.setTextSelectionStart(0);
        let value = label.getTextSelectionStart();
        if (typeof value !== "number") throw new Error("type=" + typeof value);
    });

    test("setTextSelectionEnd + getTextSelectionEnd", () => {
        label.setTextSelectionEnd(4);
        let value = label.getTextSelectionEnd();
        if (typeof value !== "number") throw new Error("type=" + typeof value);
    });

    test("insText", () => {
        label.setText("Helo");
        label.insText(2, "l");
    });

    test("cutText", () => {
        label.setText("123456");
        label.cutText(2, 2);
    });

    test("getLetterPos", () => {
        label.setText("abcdef");
        let pos = {};
        label.getLetterPos(2, pos);
        if (typeof pos.x !== "number" || typeof pos.y !== "number") {
            throw new Error("invalid point");
        }
    });

    test("getLetterOn", () => {
        let pos = { x: 1, y: 1 };
        let index = label.getLetterOn(pos, false);
        if (typeof index !== "number") throw new Error("type=" + typeof index);
    });

    test("isCharUnderPos", () => {
        let pos = { x: 1, y: 1 };
        let value = label.isCharUnderPos(pos);
        if (typeof value !== "boolean") throw new Error("type=" + typeof value);
    });

    test("prop text", () => {
        label.text = "Text property";
        if (typeof label.text !== "string") throw new Error("type=" + typeof label.text);
    });

    test("prop longMode", () => {
        label.longMode = lv.LABEL_LONG_CLIP;
        if (typeof label.longMode !== "number") throw new Error("type=" + typeof label.longMode);
    });

    test("prop textSelectionStart", () => {
        label.textSelectionStart = 1;
        if (typeof label.textSelectionStart !== "number") throw new Error("type=" + typeof label.textSelectionStart);
    });

    test("prop textSelectionEnd", () => {
        label.textSelectionEnd = 3;
        if (typeof label.textSelectionEnd !== "number") throw new Error("type=" + typeof label.textSelectionEnd);
    });

    });
}
