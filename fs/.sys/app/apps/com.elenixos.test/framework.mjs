/**
 * ElenixOS JS Unit Test Framework
 *
 * Usage:
 *   import { runSuite, test, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose, log }
 *     from './framework.mjs';
 *
 *   export function suite() {
 *     runSuite('MyModule', () => {
 *       test('should do something', () => {
 *         let v = 42;
 *         assertEqual(v, 42);
 *         assertOk(v > 0);
 *         assertType(v, 'number');
 *       });
 *     });
 *   }
 */

let _suiteName = '';
let _pass = 0;
let _fail = 0;
let _error = 0;
let _totalSuites = 0;
let _totalPass = 0;
let _totalFail = 0;
let _totalError = 0;

function log(msg) {
    eos.console.log(msg);
}

function _reset() {
    _pass = 0;
    _fail = 0;
    _error = 0;
}

// ---- Assertions ----

function assertEqual(actual, expected, msg) {
    if (actual !== expected) {
        throw new Error(msg || ('expected ' + JSON.stringify(expected) + ', got ' + JSON.stringify(actual)));
    }
}

function assertOk(value, msg) {
    if (!value) {
        throw new Error(msg || ('expected truthy, got ' + JSON.stringify(value)));
    }
}

function assertType(value, type, msg) {
    if (typeof value !== type) {
        throw new Error(msg || ('expected type ' + type + ', got ' + typeof value));
    }
}

function assertNotNull(value, msg) {
    if (value === null || value === undefined) {
        throw new Error(msg || ('expected non-null, got ' + value));
    }
}

function assertThrows(fn, msg) {
    let threw = false;
    try {
        fn();
    } catch (e) {
        threw = true;
    }
    if (!threw) {
        throw new Error(msg || 'expected exception was not thrown');
    }
}

function assertClose(actual, expected, delta, msg) {
    delta = delta || 1;
    if (Math.abs(actual - expected) > delta) {
        throw new Error(msg || ('expected ' + expected + ' +/- ' + delta + ', got ' + actual));
    }
}

// ---- Runner ----

function runSuite(name, suiteFn) {
    _suiteName = name;
    _reset();
    log('\n========== ' + name + ' ==========');
    try {
        suiteFn();
    } catch (e) {
        _error++;
        log('  [ERROR] Suite crashed: ' + (e.message || e));
    }
    let total = _pass + _fail + _error;
    _totalSuites++;
    _totalPass += _pass;
    _totalFail += _fail;
    _totalError += _error;
    log('---------- ' + _pass + '/' + total + ' passed' +
        (_fail > 0 ? ', ' + _fail + ' failed' : '') +
        (_error > 0 ? ', ' + _error + ' errors' : '') + ' ----------');
}

function test(name, fn) {
    try {
        fn();
        _pass++;
        log('  [PASS] ' + name);
    } catch (e) {
        _fail++;
        log('  [FAIL] ' + name + ' => ' + (e.message || e));
    }
}

// Short alias
function it(name, fn) { test(name, fn); }

// ---- Final Report ----

function report() {
    let total = _totalPass + _totalFail + _totalError;
    log('\n========================================');
    log('  ALL TESTS COMPLETE');
    log('  Suites: ' + _totalSuites);
    log('  Passed: ' + _totalPass);
    log('  Failed: ' + _totalFail);
    log('  Errors: ' + _totalError);
    log('  Total:  ' + total);
    log('========================================');
}

// Compatibility wrappers for ported existing tests
function _t(name, fn) { test(name, fn); }
function _log(msg) { log('[legacy] ' + msg); }

export { log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose,
         runSuite, test, it, report, _t, _log };
