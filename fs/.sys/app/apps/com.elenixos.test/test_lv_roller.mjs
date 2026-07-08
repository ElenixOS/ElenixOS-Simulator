/**
 * lv.roller coverage test
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, runSuite } from './framework.mjs';

export function suite() {
    runSuite('roller', () => {
    let scr = eos.view.active();

    let roller;
    test('constructor new lv.roller(scr)', () => {
        roller = new lv.roller(scr);
        assertNotNull(roller);
    });

    test('setSize and align', () => {
        roller.setSize(200, 120);
        roller.align(lv.ALIGN_CENTER, 0, 0);
        assertOk(true);
    });

    test('setOptions and getOptions', () => {
        roller.setOptions('Mon\nTue\nWed\nThu\nFri\nSat\nSun', lv.ROLLER_MODE_NORMAL);
        let opts = roller.getOptions();
        assertType(opts, 'string');
    });

    test('setSelected and getSelected', () => {
        roller.setSelected(3, 0);
        let sel = roller.getSelected();
        assertType(sel, 'number');
    });

    test('setVisibleRowCount', () => {
        roller.setVisibleRowCount(3);
        assertOk(true);
    });

    test('getSelectedStr', () => {
        roller.setOptions('one\ntwo\nthree', lv.ROLLER_MODE_NORMAL);
        roller.setSelected(1, 0);
        let buf = {};
        roller.getSelectedStr(buf, 32);
        assertOk(true);
    });

    test('getOptionCount returns number', () => {
        let count = roller.getOptionCount();
        assertType(count, 'number');
    });

    test('mode INFINITE', () => {
        roller.setOptions('a\nb\nc', lv.ROLLER_MODE_INFINITE);
        assertOk(true);
    });

    test('property selected', () => { roller.selected = 2; assertType(roller.selected, 'number'); });
    test('property options', () => { roller.options = 'x\ny\nz'; assertType(roller.options, 'string'); });
    test('property visibleRowCount', () => { roller.visibleRowCount = 5; assertType(roller.visibleRowCount, 'number'); });
    });
}
