/**
 * lv.textarea coverage test
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, runSuite } from './framework.mjs';

export function suite() {
    runSuite('textarea', () => {
    let scr = eos.view.active();

    let ta;
    test('constructor new lv.textarea(scr)', () => {
        ta = new lv.textarea(scr);
        assertNotNull(ta);
    });

    test('setSize and align', () => {
        ta.setSize(300, 150);
        ta.align(lv.ALIGN_CENTER, 0, 0);
        assertOk(true);
    });

    test('setText and getText', () => {
        ta.setText('Hello world');
        let t = ta.getText();
        assertType(t, 'string');
    });

    test('setPlaceholderText and getPlaceholderText', () => {
        ta.setPlaceholderText('Type here...');
        let t = ta.getPlaceholderText();
        assertType(t, 'string');
    });

    test('setCursorPos and getCursorPos', () => {
        ta.setText('abcdef');
        ta.setCursorPos(3);
        let p = ta.getCursorPos();
        assertType(p, 'number');
    });

    test('setCursorClickPos and getCursorClickPos', () => {
        ta.setCursorClickPos(true);
        let v = ta.getCursorClickPos();
        assertType(v, 'boolean');
        ta.setCursorClickPos(false);
        let v2 = ta.getCursorClickPos();
        assertType(v2, 'boolean');
    });

    test('setPasswordMode and getPasswordMode', () => {
        ta.setPasswordMode(true);
        assertOk(ta.getPasswordMode());
        ta.setPasswordMode(false);
        assertOk(!ta.getPasswordMode());
    });

    test('setPasswordBullet and getPasswordBullet', () => {
        ta.setPasswordBullet('*');
        let b = ta.getPasswordBullet();
        assertType(b, 'string');
    });

    test('setOneLine and getOneLine', () => {
        ta.setOneLine(false);
        assertOk(!ta.getOneLine());
        ta.setOneLine(true);
        assertOk(ta.getOneLine());
    });

    test('setMaxLength and getMaxLength', () => {
        ta.setMaxLength(100);
        let v = ta.getMaxLength();
        assertType(v, 'number');
    });

    test('setAcceptedChars and getAcceptedChars', () => {
        ta.setAcceptedChars('0123456789');
        let c = ta.getAcceptedChars();
        assertType(c, 'string');
    });

    test('addChar', () => {
        ta.setText('');
        ta.addChar(65); // 'A'
        assertOk(true);
    });

    test('addText', () => {
        ta.setText('');
        ta.addText('Hello');
        assertOk(true);
    });

    test('deleteChar', () => {
        ta.setText('Hello');
        ta.deleteChar();
        assertOk(true);
    });

    test('deleteCharForward', () => {
        ta.setText('Hello');
        ta.setCursorPos(0);
        ta.deleteCharForward();
        assertOk(true);
    });

    test('setTextSelection and getTextSelection', () => {
        ta.setPasswordMode(false);
        ta.setOneLine(true);
        ta.setText('Hello world');
        ta.setTextSelection(true);
        assertOk(ta.getTextSelection());
        ta.setTextSelection(false);
        assertOk(!ta.getTextSelection());
    });

    test('setPasswordShowTime and getPasswordShowTime', () => {
        ta.setPasswordMode(true);
        ta.setPasswordShowTime(1500);
        let v = ta.getPasswordShowTime();
        assertType(v, 'number');
    });

    test('setInsertReplace', () => {
        ta.setOneLine(true);
        ta.setText('Hello');
        ta.setInsertReplace('X');
        assertOk(true);
    });

    test('getLabel returns object', () => {
        let lbl = ta.getLabel();
        assertNotNull(lbl);
    });

    test('textIsSelected returns boolean', () => {
        let v = ta.textIsSelected();
        assertType(v, 'boolean');
    });

    test('getCurrentChar returns number', () => {
        let c = ta.getCurrentChar();
        assertType(c, 'number');
    });

    test('clearSelection', () => {
        ta.setText('Hello world');
        ta.setTextSelection(true);
        ta.clearSelection();
        assertOk(true);
    });

    test('cursorRight', () => { ta.setText('abc'); ta.setCursorPos(0); ta.cursorRight(); assertOk(true); });
    test('cursorLeft', () => { ta.setText('abc'); ta.setCursorPos(2); ta.cursorLeft(); assertOk(true); });
    test('cursorDown', () => { ta.cursorDown(); assertOk(true); });
    test('cursorUp', () => { ta.cursorUp(); assertOk(true); });

    test('property text', () => { ta.text = 'Property text'; assertType(ta.text, 'string'); });
    test('property placeholderText', () => { ta.placeholderText = 'ph'; assertType(ta.placeholderText, 'string'); });
    test('property cursorPos', () => { ta.cursorPos = 1; assertType(ta.cursorPos, 'number'); });
    test('property maxLength', () => { ta.maxLength = 50; assertType(ta.maxLength, 'number'); });
    test('property oneLine', () => { ta.oneLine = true; assertType(ta.oneLine, 'boolean'); });
    test('property passwordMode', () => { ta.passwordMode = false; assertType(ta.passwordMode, 'boolean'); });
    test('property passwordBullet', () => { ta.passwordBullet = '.'; assertType(ta.passwordBullet, 'string'); });
    test('property acceptedChars', () => { ta.acceptedChars = 'abc'; assertType(ta.acceptedChars, 'string'); });
    });
}
