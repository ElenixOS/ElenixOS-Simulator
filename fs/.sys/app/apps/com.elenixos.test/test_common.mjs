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

let _pass = 0;
let _fail = 0;

function $log(msg) {
    try { eos.console.log("[js-test] " + msg); } catch (e) { }
}

function $t(name, fn) {
    try {
        fn();
        _pass++;
        $log("[PASS] " + name);
    } catch (e) {
        _fail++;
        $log("[FAIL] " + name + " => " + e);
    }
}

function $report(module_name) {
    let total = _pass + _fail;
    $log("[" + module_name + "] Complete: " + _pass + "/" + total + " passed, " + _fail + " failed");
    return { pass: _pass, fail: _fail, total: total };
}

function $reset() {
    _pass = 0;
    _fail = 0;
}

function $get() {
    return { pass: _pass, fail: _fail, total: _pass + _fail };
}

export { $t, $log, $report, $reset, $get };
