/**
 * lv.calendar coverage test
 *
 * Log rules: no Chinese characters. Each entry is [PASS] or [FAIL].
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose, runSuite } from './framework.mjs';

export function suite() {
    runSuite('calendar', () => {
    let scr = eos.view.active();
    let calendar;

    let dayNames = ["日", "一", "二", "三", "四", "五", "六"];
    let highlighted = [
        { year: 2026, month: 3, day: 15 },
        { year: 2026, month: 3, day: 20 },
        { year: 2026, month: 3, day: 28 },
    ];

    test("constructor new lv.calendar(scr)", () => {
        calendar = new lv.calendar(scr);
        calendar.setSize(300, 220);
        calendar.align(lv.ALIGN_TOP_MID, 0, 8);
        if (!calendar) throw new Error("null calendar");
    });

    test("setTodayDate", () => {
        calendar.setTodayDate(2026, 3, 15);
    });

    test("setShowedDate", () => {
        calendar.setShowedDate(2026, 3);
    });

    test("special method setDayNames", () => {
        calendar.setDayNames(dayNames);
    });

    test("special method setHighlightedDates", () => {
        calendar.setHighlightedDates(highlighted, highlighted.length);
    });

    test("getBtnmatrix", () => {
        let btnm = calendar.getBtnmatrix();
        if (!btnm) throw new Error("null btnmatrix");
    });

    test("prop dayNames", () => {
        calendar.dayNames = dayNames;
    });

    test("prop btnmatrix getter", () => {
        let btnm = calendar.btnmatrix;
        if (!btnm) throw new Error("null btnmatrix prop");
    });

    test("prop highlightedDatesNum getter", () => {
        let count = calendar.highlightedDatesNum;
        if (typeof count !== "number") throw new Error("type=" + typeof count);
        if (count < 0) throw new Error("negative count");
    });

    test("prop chineseMode true/false with feature gate", () => {
        let threw = false;
        let errMsg = "";

        try {
            calendar.chineseMode = false;
            calendar.setFontSize(22);
            calendar.chineseMode = true;
        } catch (e) {
            threw = true;
            errMsg = String(e);
        }

        // Runtime behavior is the source of truth here.
        // Some builds may expose USE_CALENDAR_CHINESE constants that do not match the final linked path.
        if (threw && errMsg.indexOf("Calendar Chinese mode is disabled") < 0) {
            throw new Error("unexpected error: " + errMsg);
        }
    });

    });
}
