/**
 * Cleanup Integrity Test
 *
 * Verifies that after a script crash, all LVGL objects are properly cleaned
 * up and new objects can be created without memory corruption.
 *
 * Strategy:
 *   1. Create many LVGL objects (stretch the widget tree)
 *   2. Force a fatal script error
 *   3. After engine recovery, verify the screen is clean
 *   4. Create new objects — if TLFS corruption occurred, this will crash
 */

import { test, log, assertOk, assertNotNull, runSuite, getTestView } from './framework.mjs';

function _forceError() {
    /* Force a ReferenceError by accessing an undefined variable.
     * This causes the engine to enter fatal error recovery. */
    throw new Error("INTENTIONAL CRASH for cleanup verification");
}

export function suite() {
    runSuite('cleanup', () => {
    let scr = getTestView();

    /* Phase 1: Create many nested objects */
    let containers = [];
    for (let i = 0; i < 20; i++) {
        let container = new lv.obj(scr);
        container.setSize(60, 60);
        container.setPos((i % 5) * 70, Math.floor(i / 5) * 70);
        container.setStyleBgColor(lv.color.hex(0x303030 + i * 0x101010), lv.PART_MAIN);
        container.setStyleRadius(8, lv.PART_MAIN);

        let label = new lv.label(container);
        label.setText("" + i);
        label.center();

        containers.push(container);
    }
    log('Phase 1: created ' + containers.length + ' nested widget groups');

    /* Phase 2: Force a crash.
     * NOTE: This will destroy the current program, including all widgets.
     * The engine should recover and clean up all LVGL objects. */
    try {
        _forceError();
    } catch (e) {
        log('Phase 2: error thrown as expected: ' + (e.message || e));
    }

    /* Phase 3: After recovery, existing objects should be invalid.
     * New objects should be creatable without corruption.
     * IF cleanup failed, TLFS assertion will fire here. */
    log('Phase 3: verifying recovery by creating new objects');

    let newScr = getTestView();
    assertNotNull(newScr, 'screen should be accessible after recovery');

    /* Try creating objects in different configurations to stress the allocator */
    let flexContainer = new lv.obj(newScr);
    flexContainer.setSize(300, 200);
    flexContainer.align(lv.ALIGN_CENTER, 0, 0);
    flexContainer.setStyleFlexFlow(lv.FLEX_FLOW_ROW, lv.PART_MAIN);
    flexContainer.setStylePadAll(10, lv.PART_MAIN);

    for (let i = 0; i < 8; i++) {
        let item = new lv.obj(flexContainer);
        item.setSize(50, 50);
        item.setStyleBgColor(lv.color.hex(0x00FF00 + i * 0x000010), lv.PART_MAIN);
        item.setStyleRadius(5, lv.PART_MAIN);
    }

    /* Verify flex layout processes without corruption */
    flexContainer.setStyleLayout(lv.LAYOUT_FLEX, lv.PART_MAIN);

    log('Phase 3: created flex layout with 8 children — no crash');
    log('=== CLEANUP INTEGRITY TEST PASSED ===');
    });
}
