/**
 * lv.spinner coverage test
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, runSuite } from './framework.mjs';

export function suite() {
    runSuite('spinner', () => {
    let scr = eos.view.active();

    let spin;
    test('constructor new lv.spinner(scr)', () => {
        spin = new lv.spinner(scr);
        assertNotNull(spin);
    });

    test('setSize and align', () => {
        spin.setSize(100, 100);
        spin.align(lv.ALIGN_CENTER, 0, 0);
        assertOk(true);
    });

    test('setAnimParams', () => {
        spin.setAnimParams(2000, 60);
        assertOk(true);
    });

    test('setStyleArcColor', () => {
        let color = lv.color.hex(0x2196F3);
        spin.setStyleArcColor(color, lv.PART_MAIN);
        assertOk(true);
    });

    test('setStyleArcColor on indicator', () => {
        let color = lv.color.hex(0x00BCD4);
        spin.setStyleArcColor(color, lv.PART_INDICATOR);
        assertOk(true);
    });

    test('setStyleArcWidth', () => {
        spin.setStyleArcWidth(4, lv.PART_MAIN);
        spin.setStyleArcWidth(4, lv.PART_INDICATOR);
        assertOk(true);
    });

    test('inherited obj APIs work', () => {
        let w = spin.getWidth();
        assertType(w, 'number');
        spin.addFlag(lv.OBJ_FLAG_CLICKABLE);
        assertOk(true);
    });
    });
}
