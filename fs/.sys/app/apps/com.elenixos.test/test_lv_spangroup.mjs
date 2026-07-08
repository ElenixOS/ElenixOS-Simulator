/**
 * lv.spangroup coverage test
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, runSuite } from './framework.mjs';

export function suite() {
    runSuite('spangroup', () => {
    let scr = eos.view.active();

    let sg;
    test('constructor new lv.spangroup(scr)', () => {
        sg = new lv.spangroup(scr);
        assertNotNull(sg);
    });

    test('setSize and align', () => {
        sg.setSize(300, 150);
        sg.align(lv.ALIGN_CENTER, 0, 0);
        assertOk(true);
    });

    test('newSpan returns handle', () => {
        let span = sg.newSpan();
        assertNotNull(span);
    });

    test('span setText', () => {
        let span = sg.newSpan();
        span.setText('Hello span');
        assertOk(true);
    });

    test('deleteSpan', () => {
        let span = sg.newSpan();
        span.setText('temp');
        sg.deleteSpan(span);
        assertOk(true);
    });

    test('setAlign and getAlign', () => {
        sg.setAlign(lv.TEXT_ALIGN_LEFT);
        let align = sg.getAlign();
        assertType(align, 'number');
    });

    test('setOverflow and getOverflow', () => {
        sg.setOverflow(lv.SPAN_OVERFLOW_CLIP);
        let ov = sg.getOverflow();
        assertType(ov, 'number');
        sg.setOverflow(lv.SPAN_OVERFLOW_ELLIPSIS);
        assertType(sg.getOverflow(), 'number');
    });

    test('setIndent and getIndent', () => {
        sg.setIndent(10);
        let v = sg.getIndent();
        assertType(v, 'number');
    });

    test('setMode and getMode', () => {
        sg.setMode(lv.SPAN_MODE_FIXED);
        let mode = sg.getMode();
        assertType(mode, 'number');
        sg.setMode(lv.SPAN_MODE_EXPAND);
        assertType(sg.getMode(), 'number');
    });

    test('setMaxLines and getMaxLines', () => {
        sg.setMaxLines(5);
        let v = sg.getMaxLines();
        assertType(v, 'number');
    });

    test('getChild returns handle', () => {
        let span = sg.newSpan();
        span.setText('child span');
        let child = sg.getChild(0);
        assertNotNull(child);
    });

    test('getSpanCount returns number', () => {
        let count = sg.getSpanCount();
        assertType(count, 'number');
    });

    test('getMaxLineHeight returns number', () => {
        let v = sg.getMaxLineHeight();
        assertType(v, 'number');
    });

    test('getExpandWidth returns number', () => {
        sg.setText(''); // clear spans
        let sp = sg.newSpan();
        sp.setText('test width');
        let w = sg.getExpandWidth(300);
        assertType(w, 'number');
    });

    test('getExpandHeight returns number', () => {
        let h = sg.getExpandHeight(200);
        assertType(h, 'number');
    });

    test('refrMode', () => {
        sg.refrMode();
        assertOk(true);
    });

    test('property align', () => { sg.align = lv.TEXT_ALIGN_CENTER; assertType(sg.align, 'number'); });
    test('property overflow', () => { sg.overflow = lv.SPAN_OVERFLOW_CLIP; assertType(sg.overflow, 'number'); });
    test('property indent', () => { sg.indent = 5; assertType(sg.indent, 'number'); });
    test('property mode', () => { sg.mode = lv.SPAN_MODE_EXPAND; assertType(sg.mode, 'number'); });
    test('property maxLines', () => { sg.maxLines = 3; assertType(sg.maxLines, 'number'); });
    });
}
