/**
 * lv.slider coverage test
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, runSuite } from './framework.mjs';

export function suite() {
    runSuite('slider', () => {
    let scr = eos.view.active();

    let slider;
    test('constructor new lv.slider(scr)', () => {
        slider = new lv.slider(scr);
        assertNotNull(slider);
    });

    test('setSize and align', () => {
        slider.setSize(200, 30);
        slider.align(lv.ALIGN_CENTER, 0, 0);
        assertOk(true);
    });

    test('setValue and getValue', () => {
        slider.setValue(50, 0);
        let v = slider.getValue();
        assertType(v, 'number');
    });

    test('setRange and getMin/MaxValue', () => {
        slider.setRange(0, 100);
        let minVal = slider.getMinValue();
        let maxVal = slider.getMaxValue();
        assertType(minVal, 'number');
        assertType(maxVal, 'number');
    });

    test('setLeftValue and getLeftValue', () => {
        slider.setLeftValue(20, 0);
        let v = slider.getLeftValue();
        assertType(v, 'number');
    });

    test('setMode and getMode', () => {
        slider.setMode(lv.SLIDER_MODE_NORMAL);
        let mode = slider.getMode();
        assertType(mode, 'number');
        slider.setMode(lv.SLIDER_MODE_SYMMETRICAL);
        let mode2 = slider.getMode();
        assertType(mode2, 'number');
        slider.setMode(lv.SLIDER_MODE_RANGE);
        let mode3 = slider.getMode();
        assertType(mode3, 'number');
    });

    test('isDragged returns boolean', () => {
        let v = slider.isDragged();
        assertType(v, 'boolean');
    });

    test('isSymmetrical returns boolean', () => {
        slider.setMode(lv.SLIDER_MODE_SYMMETRICAL);
        let v = slider.isSymmetrical();
        assertType(v, 'boolean');
    });

    test('property value', () => {
        slider.value = 30;
        assertType(slider.value, 'number');
    });

    test('property mode', () => {
        slider.mode = lv.SLIDER_MODE_NORMAL;
        assertType(slider.mode, 'number');
    });

    test('property minValue', () => {
        slider.minValue = 0;
        assertType(slider.minValue, 'number');
    });

    test('property maxValue', () => {
        slider.maxValue = 255;
        assertType(slider.maxValue, 'number');
    });

    test('property leftValue', () => {
        slider.leftValue = 10;
        assertType(slider.leftValue, 'number');
    });
    });
}
