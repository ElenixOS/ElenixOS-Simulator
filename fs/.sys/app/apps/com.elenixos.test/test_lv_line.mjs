/**
 * lv.line coverage test
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, runSuite } from './framework.mjs';

export function suite() {
    runSuite('line', () => {
    let scr = eos.view.active();

    let line;
    test('constructor new lv.line(scr)', () => {
        line = new lv.line(scr);
        assertNotNull(line);
    });

    test('setSize and align', () => {
        line.setSize(300, 300);
        line.align(lv.ALIGN_CENTER, 0, 0);
        assertOk(true);
    });

    test('setYInvert and getYInvert', () => {
        line.setYInvert(true);
        assertOk(line.getYInvert());
        line.setYInvert(false);
        assertOk(!line.getYInvert());
    });

    test('getPointCount returns number', () => {
        let count = line.getPointCount();
        assertType(count, 'number');
    });

    test('isPointArrayMutable returns boolean', () => {
        let v = line.isPointArrayMutable();
        assertType(v, 'boolean');
    });

    test('setPoints with array', () => {
        let pts = [{ x: 0, y: 0 }, { x: 50, y: 50 }, { x: 100, y: 0 }];
        line.setPoints(pts);
        assertOk(true);
    });

    test('setPoints with single point', () => {
        let pts = [{ x: 0, y: 0 }];
        line.setPoints(pts);
        assertOk(true);
    });

    test('setStyleLineColor', () => {
        let color = lv.color.hex(0xFF0000);
        line.setStyleLineColor(color, lv.PART_MAIN);
        assertOk(true);
    });

    test('setStyleLineWidth', () => {
        line.setStyleLineWidth(3, lv.PART_MAIN);
        assertOk(true);
    });

    test('property yInvert', () => {
        line.yInvert = true;
        assertType(line.yInvert, 'boolean');
    });
    });
}
