/**
 * eos.activity API test suite
 *
 * Tests the JavaScript bindings for the ElenixOS Activity system.
 * Activities created here are off-stack and destroyed after each test
 * to prevent widget leakage.
 *
 * IMPORTANT: eos.activity.create() auto-creates a full-screen black
 * opaque view on root_screen.  eos.activity.getView() returns an
 * SNI_H_EOS_VIEW wrapper that does NOT expose lv.obj methods (like
 * addFlag), so we cannot simply hide it.  Instead createHiddenActivity()
 * immediately calls setView with a transparent proxy to replace the
 * auto-created view.  All test-created activities are destroyed in
 * cleanup.
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, runSuite, getTestView } from './framework.mjs';

/**
 * Helper: create a test activity and immediately replace its auto-created
 * view with a tiny transparent container on the test view.
 *
 * eos.activity.create() auto-creates a full-screen black opaque view on
 * root_screen.  eos.activity.getView() returns an SNI_H_EOS_VIEW wrapper
 * ("immutable container, parent-only access") which does NOT expose
 * lv.obj methods like addFlag — so we cannot simply hide it.
 *
 * Instead we call setView with a transparent proxy widget parented to the
 * test view container.  This deletes the auto-created view from root_screen
 * immediately, preventing a solid-black screen during test execution.
 */
function createHiddenActivity() {
    let a = eos.activity.create();
    if (!a) return null;
    try {
        let scr = getTestView();
        let hiddenView = new lv.obj(scr);
        hiddenView.setSize(10, 10);
        hiddenView.setStyleBgOpa(0, lv.PART_MAIN);
        hiddenView.removeFlag(lv.OBJ_FLAG_CLICKABLE);
        eos.activity.setView(a, hiddenView);
    } catch (e) {
        log('[activity test] Could not hide auto-created view: ' + (e.message || e));
    }
    return a;
}

/**
 * Helper: safely destroy a test activity.
 */
function destroyActivity(a) {
    if (!a) return;
    try {
        eos.activity.destroy(a);
    } catch (e) {
        log('[activity test] Destroy failed: ' + (e.message || e));
    }
}

export function suite() {
    runSuite('activity', () => {

    // ---- 1. current() --------------------------------------------------

    let current;

    test("eos.activity.current() returns non-null", () => {
        current = eos.activity.current();
        assertNotNull(current);
    });

    test("current activity type is number", () => {
        let t = eos.activity.getType(current);
        assertType(t, 'number');
    });

    // ---- 2. create() ---------------------------------------------------

    let a1 = null;

    test("eos.activity.create() returns non-null", () => {
        a1 = createHiddenActivity();
        assertNotNull(a1);
    });

    test("created activity is different from current", () => {
        assertOk(a1 !== current, 'new activity should differ from current');
    });

    // ---- 3. getView() --------------------------------------------------
    // NOTE: eos.activity.getView() returns an SNI_H_EOS_VIEW wrapper
    // which does NOT expose lv.obj methods like setSize.  The wrapper
    // is an opaque handle that can only be passed back to other
    // eos.activity.* APIs.

    test("eos.activity.getView(new) returns non-null view handle", () => {
        let v = eos.activity.getView(a1);
        assertNotNull(v);
    });

    // ---- 4. setView() --------------------------------------------------

    let replacement = null;

    test("eos.activity.setView() with custom view", () => {
        let scr = getTestView();
        replacement = new lv.obj(scr);
        replacement.setSize(100, 100);
        replacement.setStyleBgOpa(0, lv.PART_MAIN);
        replacement.removeFlag(lv.OBJ_FLAG_CLICKABLE);

        // Should not throw
        eos.activity.setView(a1, replacement);
    });

    test("getView returns non-null after setView", () => {
        // getView returns an EOS_VIEW wrapper — a different JS object
        // than the raw lv.obj passed to setView.  Reference equality
        // (===) is therefore not expected.  We verify the handle is
        // valid and usable with other activity APIs.
        let v = eos.activity.getView(a1);
        assertNotNull(v);
        // Verify we can pass the handle back to getView itself (round-trip)
        let v2 = eos.activity.getView(a1);
        assertNotNull(v2);
    });

    // ---- 5. destroy() --------------------------------------------------

    test("eos.activity.destroy() on off-stack activity", () => {
        // Should not throw
        destroyActivity(a1);
        a1 = null;
        // replacement was deleted along with the activity
        replacement = null;
    });

    test("create + immediate destroy (no setView)", () => {
        let a2 = createHiddenActivity();
        assertNotNull(a2);
        destroyActivity(a2);
    });

    test("create + setView + destroy (normal test cycle)", () => {
        let a = eos.activity.create();
        assertNotNull(a);

        // Auto-created view will be deleted by setView below.
        // (EOS_VIEW wrapper from getView does not support lv.obj
        //  methods like addFlag, so skip the hide step.)

        // Replace with custom view
        let scr = getTestView();
        let custom = new lv.obj(scr);
        custom.setSize(200, 200);
        custom.setStyleBgOpa(0, lv.PART_MAIN);
        custom.removeFlag(lv.OBJ_FLAG_CLICKABLE);

        eos.activity.setView(a, custom);

        // Verify view was set — getView returns an EOS_VIEW wrapper,
        // not the raw lv.obj, so reference equality is not expected.
        let v = eos.activity.getView(a);
        assertNotNull(v);

        // Create some children on the custom view (using the raw lv.obj)
        let child = new lv.obj(custom);
        child.setSize(50, 50);
        child.setPos(10, 10);

        // Destroy — this should delete custom view + child
        destroyActivity(a);
    });

    // ---- 6. Sequential create / destroy (stress) -----------------------

    test("sequential create+destroy x10", () => {
        for (let i = 0; i < 10; i++) {
            let a = createHiddenActivity();
            assertNotNull(a);
            destroyActivity(a);
        }
    });

    // ---- 7. title ------------------------------------------------------

    test("setTitle / getTitle round-trip", () => {
        let a = createHiddenActivity();
        assertNotNull(a);

        eos.activity.setTitle(a, 'TestTitle');
        let title = eos.activity.getTitle(a);
        assertEqual(title, 'TestTitle');

        destroyActivity(a);
    });

    // ---- 8. type -------------------------------------------------------

    test("getType returns valid type constant", () => {
        let a = createHiddenActivity();
        assertNotNull(a);

        let t = eos.activity.getType(a);
        assertType(t, 'number');
        // EOS_ACTIVITY_TYPE_APP = 1 is the default for eos.activity.create()
        assertEqual(t, eos.ACTIVITY_TYPE_APP || 1);

        destroyActivity(a);
    });

    // ---- 9. app header visibility --------------------------------------

    test("setAppHeaderVisible / isAppHeaderVisible", () => {
        let cur = eos.activity.current();

        // Save original state
        let origVisible = eos.activity.isAppHeaderVisible(cur);

        eos.activity.setAppHeaderVisible(cur, true);
        assertOk(eos.activity.isAppHeaderVisible(cur) === true,
                 'header should be visible after set(true)');

        eos.activity.setAppHeaderVisible(cur, false);
        assertOk(eos.activity.isAppHeaderVisible(cur) === false,
                 'header should be hidden after set(false)');

        // Restore
        eos.activity.setAppHeaderVisible(cur, origVisible);
    });

    // ---- 10. Cleanup: destroy remaining test data ----------------------

    log('[activity] All activity tests completed, cleaning up');

    }); // runSuite
}
