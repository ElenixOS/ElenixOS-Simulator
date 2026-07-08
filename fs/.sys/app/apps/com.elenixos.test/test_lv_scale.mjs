/**
 * lv.scale coverage test
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, runSuite } from './framework.mjs';

export function suite() {
    runSuite('scale', () => {
    let scr = eos.view.active();

    let scale;
    test('constructor new lv.scale(scr)', () => {
        scale = new lv.scale(scr);
        assertNotNull(scale);
    });

    test('setSize and align', () => {
        scale.setSize(300, 300);
        scale.align(lv.ALIGN_CENTER, 0, 0);
        assertOk(true);
    });

    test('setMode and getMode', () => {
        scale.setMode(lv.SCALE_MODE_ROUND_INNER);
        let mode = scale.getMode();
        assertType(mode, 'number');

        scale.setMode(lv.SCALE_MODE_HORIZONTAL_TOP);
        assertType(scale.getMode(), 'number');
    });

    test('setTotalTickCount and getTotalTickCount', () => {
        scale.setTotalTickCount(12);
        let v = scale.getTotalTickCount();
        assertType(v, 'number');
    });

    test('setMajorTickEvery and getMajorTickEvery', () => {
        scale.setMajorTickEvery(3);
        let v = scale.getMajorTickEvery();
        assertType(v, 'number');
    });

    test('setLabelShow and getLabelShow', () => {
        scale.setLabelShow(true);
        assertOk(scale.getLabelShow());
        scale.setLabelShow(false);
        assertOk(!scale.getLabelShow());
    });

    test('setRange and getRangeMin/MaxValue', () => {
        scale.setRange(0, 100);
        let minVal = scale.getRangeMinValue();
        let maxVal = scale.getRangeMaxValue();
        assertType(minVal, 'number');
        assertType(maxVal, 'number');
    });

    test('setAngleRange and getAngleRange', () => {
        scale.setAngleRange(270);
        let v = scale.getAngleRange();
        assertType(v, 'number');
    });

    test('setRotation', () => {
        scale.setRotation(135);
        assertType(scale.getMode(), 'number');
    });

    test('setPostDraw', () => {
        scale.setPostDraw(true);
        assertOk(true);
    });

    test('setDrawTicksOnTop', () => {
        scale.setDrawTicksOnTop(true);
        assertOk(true);
    });

    test('addSection returns handle', () => {
        let section = scale.addSection();
        assertNotNull(section);
        if (section) {
            section.setRange(0, 50);
        }
    });

    test('property mode', () => { scale.mode = lv.SCALE_MODE_ROUND_OUTER; assertType(scale.mode, 'number'); });
    test('property totalTickCount', () => { scale.totalTickCount = 24; assertType(scale.totalTickCount, 'number'); });
    test('property majorTickEvery', () => { scale.majorTickEvery = 6; assertType(scale.majorTickEvery, 'number'); });
    test('property labelShow', () => { scale.labelShow = false; assertType(scale.labelShow, 'boolean'); });
    test('property angleRange', () => { scale.angleRange = 180; assertType(scale.angleRange, 'number'); });
    });
}
