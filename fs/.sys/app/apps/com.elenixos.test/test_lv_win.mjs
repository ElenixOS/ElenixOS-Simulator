/**
 * lv.win coverage test
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, runSuite } from './framework.mjs';

export function suite() {
    runSuite('win', () => {
    let scr = eos.view.active();

    let win;
    test('constructor new lv.win(scr)', () => {
        win = new lv.win(scr);
        assertNotNull(win);
    });

    test('setSize and align', () => {
        win.setSize(340, 340);
        win.align(lv.ALIGN_CENTER, 0, 0);
        assertOk(true);
    });

    test('addTitle returns object', () => {
        let t = win.addTitle('Window Title');
        assertNotNull(t);
    });

    test('addButton returns object', () => {
        let btn = win.addButton(null, 30);
        assertNotNull(btn);
    });

    test('getHeader returns object', () => {
        let h = win.getHeader();
        assertNotNull(h);
    });

    test('getContent returns object', () => {
        let c = win.getContent();
        assertNotNull(c);
    });

    test('add content to window', () => {
        let content = win.getContent();
        let label = new lv.label(content);
        label.setText('Window Content');
        label.center();
        assertOk(true);
    });

    test('setStyleBgColor on win', () => {
        let color = lv.color.hex(0xFFFFFF);
        win.setStyleBgColor(color, lv.PART_MAIN);
        assertOk(true);
    });
    });
}
