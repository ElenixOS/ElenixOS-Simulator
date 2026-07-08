/**
 * lv.msgbox coverage test
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, runSuite } from './framework.mjs';

export function suite() {
    runSuite('msgbox', () => {
    let scr = eos.view.active();

    let mbox;
    test('constructor new lv.msgbox(scr)', () => {
        mbox = new lv.msgbox(scr);
        assertNotNull(mbox);
    });

    test('addTitle returns object', () => {
        let t = mbox.addTitle('Test Title');
        assertNotNull(t);
    });

    test('addText returns object', () => {
        let t = mbox.addText('This is the message body text.');
        assertNotNull(t);
    });

    test('addFooterButton returns object', () => {
        let btn = mbox.addFooterButton('OK');
        assertNotNull(btn);
    });

    test('addCloseButton returns object', () => {
        let btn = mbox.addCloseButton();
        assertNotNull(btn);
    });

    test('addHeaderButton returns object', () => {
        let btn = mbox.addHeaderButton(null);
        assertNotNull(btn);
    });

    test('getHeader returns object', () => {
        let h = mbox.getHeader();
        assertNotNull(h);
    });

    test('getFooter returns object', () => {
        let f = mbox.getFooter();
        assertNotNull(f);
    });

    test('getContent returns object', () => {
        let c = mbox.getContent();
        assertNotNull(c);
    });

    test('getTitle returns object', () => {
        let t = mbox.getTitle();
        assertNotNull(t);
    });

    test('close', () => {
        mbox.close();
        assertOk(true);
    });
    });
}
