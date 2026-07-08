/**
 * lv.switch coverage test
 *
 * lv.switch has no widget-exclusive methods; state is controlled via
 * inherited lv.obj state/addState/removeState APIs.
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, runSuite } from './framework.mjs';

export function suite() {
    runSuite('switch', () => {
    let scr = eos.view.active();

    let sw;
    test('constructor new lv.switch(scr)', () => {
        sw = new lv.switch(scr);
        assertNotNull(sw);
    });

    test('setSize and align', () => {
        sw.setSize(80, 40);
        sw.align(lv.ALIGN_CENTER, 0, 0);
        assertOk(true);
    });

    test('addFlag CLICKABLE', () => {
        sw.addFlag(lv.OBJ_FLAG_CLICKABLE);
        assertOk(sw.hasFlag(lv.OBJ_FLAG_CLICKABLE));
    });

    test('addFlag CHECKABLE', () => {
        sw.addFlag(lv.OBJ_FLAG_CHECKABLE);
        assertOk(sw.hasFlag(lv.OBJ_FLAG_CHECKABLE));
    });

    test('addState CHECKED', () => {
        sw.addState(lv.STATE_CHECKED);
        assertOk(sw.hasState(lv.STATE_CHECKED));
    });

    test('removeState CHECKED', () => {
        sw.removeState(lv.STATE_CHECKED);
        assertOk(!sw.hasState(lv.STATE_CHECKED));
    });

    test('getState returns number', () => {
        let state = sw.getState();
        assertType(state, 'number');
    });

    test('sendEvent VALUE_CHANGED', () => {
        sw.addState(lv.STATE_CHECKED);
        let fired = false;
        let cb = function(e) { fired = true; };
        sw.addEventCb(cb, lv.EVENT_VALUE_CHANGED, null);
        sw.sendEvent(lv.EVENT_VALUE_CHANGED, null);
        assertOk(fired);
        sw.removeEventCb(cb);
    });

    test('setStyleBgColor via obj API', () => {
        let color = lv.color.hex(0x00FF00);
        sw.setStyleBgColor(color, lv.PART_MAIN);
        assertOk(true);
    });

    test('setStyleRadius via obj API', () => {
        sw.setStyleRadius(10, lv.PART_MAIN);
        assertOk(true);
    });
    });
}
