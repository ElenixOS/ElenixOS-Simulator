/**
 * lv.timer coverage test
 *
 * Log rules: no Chinese characters. Each entry is [PASS] or [FAIL].
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose, runSuite } from './framework.mjs';

export function suite() {
    runSuite('timer', () => {
    let fireCount = 0;
    let cb1 = function () {
        fireCount++;
        eos.console.log("[timer-test] cb1 fired count=" + fireCount);
    };
    let cb2 = function () {
        fireCount++;
        eos.console.log("[timer-test] cb2 fired count=" + fireCount);
    };

    let timer;

    test("constructor new lv.timer(cb, period, null)", () => {
        timer = new lv.timer(cb1, 1000, null);
        if (!timer) throw new Error("null timer");
    });

    test("pause + getPaused", () => {
        timer.pause();
        if (typeof timer.getPaused() !== "boolean") throw new Error("type");
    });

    test("resume + getPaused", () => {
        timer.resume();
        if (typeof timer.getPaused() !== "boolean") throw new Error("type");
    });

    test("setPeriod", () => {
        timer.setPeriod(500);
    });

    test("setRepeatCount", () => {
        timer.setRepeatCount(2);
    });

    test("setAutoDelete", () => {
        timer.setAutoDelete(false);
    });

    test("reset", () => {
        timer.reset();
    });

    test("ready", () => {
        timer.ready();
    });

    test("getNext", () => {
        timer.getNext();
    });

    test("prop cb", () => {
        timer.cb = cb2;
    });

    test("prop period", () => {
        timer.period = 250;
    });

    test("prop repeatCount", () => {
        timer.repeatCount = 1;
    });

    test("prop autoDelete", () => {
        timer.autoDelete = false;
    });

    test("prop paused/next", () => {
        if (typeof timer.paused !== "boolean") throw new Error("paused");
        timer.next;
    });

    test("delete", () => {
        timer.delete();
    });

    });
}
