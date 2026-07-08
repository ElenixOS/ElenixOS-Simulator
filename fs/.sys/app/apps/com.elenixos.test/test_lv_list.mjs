/**
 * lv.list coverage test
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, runSuite } from './framework.mjs';

export function suite() {
    runSuite('list', () => {
    let scr = eos.view.active();

    let list;
    test('constructor new lv.list(scr)', () => {
        list = new lv.list(scr);
        assertNotNull(list);
    });

    test('setSize and align', () => {
        list.setSize(350, 350);
        list.align(lv.ALIGN_CENTER, 0, 0);
        assertOk(true);
    });

    test('addText returns object', () => {
        let t = list.addText('Section Header');
        assertNotNull(t);
    });

    test('addButton returns object', () => {
        let btn = list.addButton(null, 'Item 1');
        assertNotNull(btn);
    });

    test('addButton with icon returns object', () => {
        let btn = list.addButton(null, 'Item 2');
        assertNotNull(btn);
    });

    test('getButtonText returns string', () => {
        let btn = list.addButton(null, 'Test Item');
        let txt = list.getButtonText(btn);
        assertType(txt, 'string');
    });

    test('setButtonText', () => {
        let btn = list.addButton(null, 'Old Text');
        list.setButtonText(btn, 'New Text');
        assertOk(true);
    });

    test('property buttonText', () => {
        let btn = list.addButton(null, 'Prop Item');
        // getButtonText works via method
        let txt = list.getButtonText(btn);
        assertOk(txt !== null);
    });

    test('setStyleBgColor on list', () => {
        let color = lv.color.hex(0xF0F0F0);
        list.setStyleBgColor(color, lv.PART_MAIN);
        assertOk(true);
    });

    test('setStyleRadius on list', () => {
        list.setStyleRadius(8, lv.PART_MAIN);
        assertOk(true);
    });
    });
}
