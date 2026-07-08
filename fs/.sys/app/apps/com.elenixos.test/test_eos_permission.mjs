/**
 * eos.permission coverage test
 *
 * Tests eos.permission.check() and eos.permission.request() APIs
 * against all declared permissions plus boundary / error cases.
 *
 * Log rules: no Chinese characters. Each entry is [PASS] or [FAIL].
 */

/* ---- test harness ---- */

/** Test that should NOT throw */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose, runSuite } from './framework.mjs';

export function suite() {
    runSuite('permission', () => {
    log("================================================================");
    log("Starting eos.permission coverage test");
    log("================================================================");

    /* --- check() tests --- */
    test_check_arg_validation();
    test_check_valid_perms();
    test_check_edge_cases();
    test_check_stress();
    test_check_return_types();

    /* --- request() tests --- */
    test_request_arg_validation();
    test_request_valid_perms();
    test_request_manifest_check();
    test_request_concurrent();
    test_request_callback_not_sync();

    /* --- cross-API & boundary tests --- */
    test_cross_api_consistency();
    test_long_string();
    test_lookalike_strings();

    /* --- summary --- */
    log("================================================================");
    log("RESULTS: " + _pass + " passed, " + _fail + " failed, " +
         (_pass + _fail) + " total");
    if (_fail === 0) {
        log("ALL TESTS PASSED");
    } else {
        log("SOME TESTS FAILED (" + _fail + " failures)");
    }
    log("================================================================");
    });
}
