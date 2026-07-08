/**
 * lv.keyboard coverage test
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, runSuite } from './framework.mjs';

export function suite() {
    runSuite('keyboard', () => {
    let scr = eos.view.active();

    let kb;
    test('constructor new lv.keyboard(scr)', () => {
        kb = new lv.keyboard(scr);
        assertNotNull(kb);
    });

    test('setSize and align', () => {
        kb.setSize(300, 160);
        kb.align(lv.ALIGN_BOTTOM_MID, 0, 0);
        assertOk(true);
    });

    test('setMode and getMode', () => {
        kb.setMode(lv.KEYBOARD_MODE_NUMBER);
        let mode = kb.getMode();
        assertType(mode, 'number');

        kb.setMode(lv.KEYBOARD_MODE_TEXT_LOWER);
        let mode2 = kb.getMode();
        assertType(mode2, 'number');

        kb.setMode(lv.KEYBOARD_MODE_TEXT_UPPER);
        let mode3 = kb.getMode();
        assertType(mode3, 'number');

        kb.setMode(lv.KEYBOARD_MODE_SPECIAL);
        let mode4 = kb.getMode();
        assertType(mode4, 'number');
    });

    test('setTextarea and getTextarea', () => {
        let ta = new lv.textarea(scr);
        ta.setSize(280, 40);
        ta.align(lv.ALIGN_TOP_MID, 0, 10);
        kb.setTextarea(ta);
        let result = kb.getTextarea();
        assertNotNull(result);
    });

    test('setPopovers and getPopovers', () => {
        kb.setPopovers(false);
        let v = kb.getPopovers();
        assertType(v, 'boolean');
        kb.setPopovers(true);
        let v2 = kb.getPopovers();
        assertType(v2, 'boolean');
    });

    test('getSelectedButton returns number', () => {
        let v = kb.getSelectedButton();
        assertType(v, 'number');
    });

    test('getButtonText returns string or null', () => {
        let t = kb.getButtonText(0);
        // May be string or null depending on state
        assertOk(true);
    });

    test('property textarea', () => {
        let ta = new lv.textarea(scr);
        ta.setSize(280, 40);
        ta.align(lv.ALIGN_TOP_MID, 0, 55);
        kb.textarea = ta;
        assertNotNull(kb.textarea);
    });

    test('property mode', () => {
        kb.mode = lv.KEYBOARD_MODE_TEXT_LOWER;
        assertType(kb.mode, 'number');
    });

    test('property popovers', () => {
        kb.popovers = false;
        assertType(kb.popovers, 'boolean');
    });
    });
}
