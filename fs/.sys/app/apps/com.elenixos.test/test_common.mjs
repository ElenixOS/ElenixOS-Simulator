/**
 * Common test framework for JavaScript unit tests.
 *
 * Usage:
 *   import { $t, $log, $report } from './test_common.mjs';
 *
 *   export function run_xxx_test() {
 *     $t("test name", () => { ... });
 *     $report("xxx");
 *   }
 */

function $log(msg) {
    try { eos.console.log("[js-test] " + msg); } catch (e) { }
}

function $t(name, fn) {
    try {
        fn();
        $log("[PASS] " + name);
    } catch (e) {
        $log("[FAIL] " + name + " => " + e);
    }
}

function $report(module_name) {
    let total = _pass + _fail;
    $log("[" + module_name + "] Complete: " + _pass + "/" + total + " passed, " + _fail + " failed");
    return { pass: _pass, fail: _fail, total: total };
}

function $reset() {
}

function $get() {
    return { pass: _pass, fail: _fail, total: _pass + _fail };
}

export { $t, $log, $report, $reset, $get };
