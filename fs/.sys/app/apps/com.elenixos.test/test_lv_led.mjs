/**
 * lv.led coverage test
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, runSuite } from './framework.mjs';

export function suite() {
    runSuite('led', () => {
    let scr = eos.view.active();

    let led;
    test('constructor new lv.led(scr)', () => {
        led = new lv.led(scr);
        assertNotNull(led);
    });

    test('setSize and align', () => {
        led.setSize(60, 60);
        led.align(lv.ALIGN_TOP_LEFT, 20, 20);
        assertOk(true);
    });

    test('setColor', () => {
        let c = lv.color.hex(0x00FF00);
        led.setColor(c);
        assertOk(true);
    });

    test('setBrightness and getBrightness', () => {
        led.setBrightness(200);
        let b = led.getBrightness();
        assertType(b, 'number');
    });

    test('on', () => { led.on(); assertOk(true); });
    test('off', () => { led.off(); assertOk(true); });
    test('toggle', () => { led.toggle(); assertOk(true); });

    test('property brightness', () => {
        led.brightness = 128;
        assertType(led.brightness, 'number');
    });

    test('property color', () => {
        let c = lv.color.hex(0xFF0000);
        led.color = c;
        assertOk(true);
    });
    });
}
