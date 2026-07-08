/**
 * lv.spinbox coverage test
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, runSuite } from './framework.mjs';

export function suite() {
    runSuite('spinbox', () => {
    let scr = eos.view.active();

    let spin;
    test('constructor new lv.spinbox(scr)', () => {
        spin = new lv.spinbox(scr);
        assertNotNull(spin);
    });

    test('setSize and align', () => {
        spin.setSize(150, 50);
        spin.align(lv.ALIGN_CENTER, 0, 0);
        assertOk(true);
    });

    test('setValue and getValue', () => {
        spin.setValue(42);
        let v = spin.getValue();
        assertType(v, 'number');
    });

    test('setRollover and getRollover', () => {
        spin.setRollover(true);
        assertOk(spin.getRollover());
        spin.setRollover(false);
        assertOk(!spin.getRollover());
    });

    test('setDigitFormat', () => {
        spin.setDigitFormat(4, 0);
        assertOk(true);
    });

    test('setStep and getStep', () => {
        spin.setStep(5);
        let s = spin.getStep();
        assertType(s, 'number');
    });

    test('setRange', () => {
        spin.setRange(0, 9999);
        assertOk(true);
    });

    test('setCursorPos', () => {
        spin.setCursorPos(1);
        assertOk(true);
    });

    test('setDigitStepDirection', () => {
        spin.setDigitStepDirection(lv.DIR_TOP);
        assertOk(true);
    });

    test('stepNext', () => { spin.stepNext(); assertOk(true); });
    test('stepPrev', () => { spin.stepPrev(); assertOk(true); });
    test('increment', () => { spin.increment(); assertOk(true); });
    test('decrement', () => { spin.decrement(); assertOk(true); });

    test('property value', () => { spin.value = 100; assertType(spin.value, 'number'); });
    test('property rollover', () => { spin.rollover = true; assertType(spin.rollover, 'boolean'); });
    test('property step', () => { spin.step = 5; assertType(spin.step, 'number'); });
    });
}
