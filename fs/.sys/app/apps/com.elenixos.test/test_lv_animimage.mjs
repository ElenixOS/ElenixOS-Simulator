/**
 * lv.animimage coverage test
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, runSuite } from './framework.mjs';

export function suite() {
    runSuite('animimage', () => {
    let scr = eos.view.active();

    let aimg;
    test('constructor new lv.animimage(scr)', () => {
        aimg = new lv.animimage(scr);
        assertNotNull(aimg);
    });

    test('setSize and align', () => {
        aimg.setSize(100, 100);
        aimg.align(lv.ALIGN_CENTER, 0, 0);
        assertOk(true);
    });

    test('setDuration and getDuration', () => {
        aimg.setDuration(1000);
        let d = aimg.getDuration();
        assertType(d, 'number');
    });

    test('setRepeatCount and getRepeatCount', () => {
        aimg.setRepeatCount(3);
        let r = aimg.getRepeatCount();
        assertType(r, 'number');
    });

    test('start', () => {
        aimg.start();
        assertOk(true);
    });

    test('getSrcCount returns number', () => {
        let c = aimg.getSrcCount();
        assertType(c, 'number');
    });

    test('getAnim returns handle', () => {
        let anim = aimg.getAnim();
        assertNotNull(anim);
    });

    test('property duration', () => { aimg.duration = 500; assertType(aimg.duration, 'number'); });
    test('property repeatCount', () => { aimg.repeatCount = 0; assertType(aimg.repeatCount, 'number'); });
    });
}
