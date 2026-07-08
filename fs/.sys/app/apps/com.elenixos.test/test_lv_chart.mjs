/**
 * lv.chart coverage test
 *
 * Log rules: no Chinese characters. Each entry is [PASS] or [FAIL].
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose, runSuite } from './framework.mjs';

export function suite() {
    runSuite('chart', () => {
    let scr = eos.view.active();
    let chart;
    let ser;
    let cursor;

    test("constructor new lv.chart(scr)", () => {
        chart = new lv.chart(scr);
        chart.setSize(320, 180);
        chart.align(lv.ALIGN_TOP_MID, 0, 8);
        if (!chart) throw new Error("null chart");
    });

    test("setType", () => {
        chart.setType(lv.CHART_TYPE_LINE);
    });

    test("setPointCount", () => {
        chart.setPointCount(12);
    });

    test("setRange", () => {
        chart.setRange(lv.CHART_AXIS_PRIMARY_Y, 0, 100);
    });

    test("setUpdateMode", () => {
        chart.setUpdateMode(lv.CHART_UPDATE_MODE_SHIFT);
    });

    test("setDivLineCount", () => {
        chart.setDivLineCount(5, 5);
    });

    test("addSeries", () => {
        ser = chart.addSeries(lv.color.hex(0x33AA66), lv.CHART_AXIS_PRIMARY_Y);
        if (!ser) throw new Error("null series");
    });

    test("setAllValue", () => {
        chart.setAllValue(ser, 10);
    });

    test("setNextValue", () => {
        chart.setNextValue(ser, 40);
        chart.setNextValue(ser, 70);
    });

    test("setXStartPoint/getXStartPoint", () => {
        chart.setXStartPoint(ser, 2);
        let start = chart.getXStartPoint(ser);
        if (typeof start !== "number") throw new Error("type=" + typeof start);
    });

    test("addCursor", () => {
        cursor = chart.addCursor(lv.color.hex(0xFF6633), lv.DIR_TOP);
        if (!cursor) throw new Error("null cursor");
    });

    test("setNextValue2 (x,y)", () => {
        chart.setNextValue2(ser, 30, 60);
    });

    test("getType/getPointCount", () => {
        if (typeof chart.getType() !== "number") throw new Error("type getter");
        if (typeof chart.getPointCount() !== "number") throw new Error("count getter");
    });

    test("getPressedPoint/getFirstPointCenterOffset", () => {
        if (typeof chart.getPressedPoint() !== "number") throw new Error("pressedPoint");
        if (typeof chart.getFirstPointCenterOffset() !== "number") throw new Error("offset");
    });

    test("refresh", () => {
        chart.refresh();
    });

    test("prop pointCount", () => {
        chart.pointCount = 16;
        if (typeof chart.pointCount !== "number") throw new Error("type=" + typeof chart.pointCount);
    });

    test("prop type", () => {
        chart.type = lv.CHART_TYPE_BAR;
        if (typeof chart.type !== "number") throw new Error("type=" + typeof chart.type);
    });

    test("prop updateMode", () => {
        chart.updateMode = lv.CHART_UPDATE_MODE_CIRCULAR;
    });

    test("prop read-only values", () => {
        if (typeof chart.pressedPoint !== "number") throw new Error("pressedPoint");
        if (typeof chart.firstPointCenterOffset !== "number") throw new Error("firstPointCenterOffset");
    });

    test("removeSeries", () => {
        chart.removeSeries(ser);
    });

    });
}
