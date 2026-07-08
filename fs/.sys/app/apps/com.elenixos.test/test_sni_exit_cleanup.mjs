/**
 * SNI cleanup on script exit manual verification.
 *
 * This test intentionally keeps resources active so the app can be exited
 * while callbacks, timers, and animations are still registered.
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose, runSuite } from './framework.mjs';

export function suite() {
    runSuite('sni_exit_cleanup', () => {
    let scr = eos.view.active();
    let panel = new lv.obj(scr);
    panel.setSize(120, 120);
    panel.align(lv.ALIGN_CENTER, 0, 0);

    let label = new lv.label(panel);
    label.setText("Exit app now");
    label.center();

    let clickCount = 0;
    panel.addEventCb(function () {
        clickCount++;
        log("event callback fired count=" + clickCount);
    }, lv.EVENT_CLICKED, null);

    let anim = new lv.anim();
    anim.init();
    anim.setVar(panel);
    anim.setValues(-20, 20);
    anim.setDuration(600);
    anim.setRepeatCount(lv.ANIM_REPEAT_INFINITE);
    anim.setCustomExecCb(function (_, value) {
        panel.setX(value);
    });
    anim.setDeletedCb(function () {
        log("deleted callback fired during cleanup");
    });
    anim.start();

    let timerCount = 0;
    let timer = new lv.timer(function () {
        timerCount++;
        log("timer callback fired count=" + timerCount);
    }, 300, null);
    timer.setRepeatCount(-1);

    log("resources created: event callback, active anim, active timer");
    log("exit the app while this screen is active and verify no crash or stale callbacks remain");
    });
}
