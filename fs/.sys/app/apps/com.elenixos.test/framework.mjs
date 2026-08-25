/**
 * ElenixOS JS Unit Test Framework
 *
 * Manages a transparent full-screen container on the host view for test
 * isolation.  The container is created before each suite runs and deleted
 * afterwards — all test widgets inside it are recursively removed.
 *
 * Usage:
 *   import { runSuite, test, assertEqual, assertOk, assertType, assertNotNull,
 *            assertThrows, assertClose, log, createSuiteRunner, getSuiteResult,
 *            getAllResults, clearAllResults, report, getTestView }
 *     from './framework.mjs';
 *
 *   export function suite() {
 *     runSuite('MyModule', () => {
 *       let scr = getTestView();     // <-- always use this as parent
 *       test('should do something', () => { ... });
 *     });
 *   }
 *
 *   const runner = createSuiteRunner('MyModule', mySuiteFn);
 *   runner();
 */

let _moduleId = Math.random().toString(36).substring(2, 8);
log('[runner] framework.mjs MODULE instance id=' + _moduleId);

let _suiteName = '';
let _pass = 0;
let _fail = 0;
let _error = 0;
let _totalSuites = 0;
let _totalPass = 0;
let _totalFail = 0;
let _totalError = 0;

/* ---- Per-suite result storage (for UI) ---- */

let _suiteResults = {};
let _onSuiteStart = null;
let _onSuiteDone  = null;

/* ---- Test view management ---- */

let _testView = null;

function _setTestView(v, caller) {
    if (_testView !== v) {
        log('[runner][' + _moduleId + '] _setTestView: ' + caller + ' changing _testView from ' + _testView + ' to ' + v);
    }
    _testView = v;
}

/**
 * Get the current test view.
 *
 * When a suite is running inside createSuiteRunner, this returns the
 * isolated test Activity's view.  Otherwise it falls back to the
 * currently active view so that legacy / manual calls still work.
 */
function getTestView() {
    if (_testView) {
        log('[runner][' + _moduleId + '] getTestView: returning _testView (valid)');
        return _testView;
    }
    log('[runner][' + _moduleId + '] getTestView: _testView is FALSY (' + _testView + '), falling back to eos.view.active()');
    return eos.view.active();
}

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

// ---- Runner (legacy console mode) ----

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
    /* NOTE: _totalSuites / _totalPass / _totalFail / _totalError are
     * incremented by createSuiteRunner (the primary path).  This
     * function is the legacy console-only path; duplicating the
     * increments here would double-count every suite. */
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

function it(name, fn) { test(name, fn); }

// ---- Test container lifecycle ----

/** Objects registered for deferred cleanup (fallback for when the test
 *  view container cannot be created).  Cleared at the start of each suite. */
let _cleanupList = [];

/**
 * Register an LVGL object for deferred cleanup.  Useful as a safety net
 * for suites that create top-level widgets — if the framework's container
 * deletion succeeds, these objects are already gone; if it fails, they are
 * individually deleted at the end of the suite.
 */
function trackForCleanup(obj) {
    if (obj && _cleanupList.indexOf(obj) < 0) {
        _cleanupList.push(obj);
    }
}

/** Delete every tracked object, newest first.  Errors are caught per-object
 *  so one failure does not block subsequent deletions. */
function _runCleanup() {
    log('[runner] _runCleanup: ' + _cleanupList.length + ' tracked objects');
    while (_cleanupList.length > 0) {
        let obj = _cleanupList.pop();
        try {
            log('[runner] _runCleanup: deleting tracked object, childCount=' + (obj.getChildCount ? obj.getChildCount() : 'N/A'));
            obj.delete();
            log('[runner] _runCleanup: tracked object deleted OK');
        } catch (e) {
            log('[runner] _runCleanup: failed to delete tracked object: ' + (e.message || e));
        }
    }
}

/**
 * Create a transparent full-screen container on the host view.
 *
 * Uses only the absolute minimum API calls (create, size, pos, bgOpa)
 * to maximise reliability.  If even these fail the engine is broken and
 * the per-suite `_cleanupList` fallback catches leftover widgets.
 */
function _createTestView() {
    try {
        let parent = eos.view.active();
        let tv = new lv.obj(parent);
        tv.setSize(480, 480);
        tv.setPos(0, 0);
        tv.setStyleBgOpa(0, lv.PART_MAIN);
        return tv;
    } catch (e) {
        log('[runner] _createTestView error: ' + (e.message || e));
        return null;
    }
}

/**
 * Delete the test container and all widgets inside it recursively.
 */
function _destroyTestView(view) {
    if (!view) { log('[runner] _destroyTestView: view is null, skipping'); return; }
    try {
        log('[runner] Deleting test view container... childCount=' + (view.getChildCount ? view.getChildCount() : 'N/A'));
        view.delete();
        log('[runner] Test view container deleted OK');
    } catch (e) {
        log('[runner] Failed to delete test view: ' + (e.message || e));
    }
}

// ---- UI-aware suite runner ----

/** Delay (ms) before destroying the test container.  Gives LVGL time to
 *  finish any pending animations (fadeIn/fadeOut) and rendering before
 *  widgets are torn down. */
const CLEANUP_DELAY_MS = 250;

/**
 * Create a UI-friendly suite runner.
 *
 * Each invocation: creates a transparent test container on the host
 * view → runs the suite (all widgets parented to the container via
 * getTestView()) → schedules deferred deletion of the container after
 * CLEANUP_DELAY_MS so that pending animations can complete and the
 * screen is clear before the next suite starts.
 *
 * @param {string} name
 * @param {Function} fn  suite function (e.g. the exported `suite`)
 * @param {Function} [done]  optional callback invoked AFTER the test
 *   container has been destroyed (i.e. after the cleanup timer fires).
 */
function createSuiteRunner(name, fn) {
    return function (done) {
        let testView = null;

        /* Mark as running so the UI can update before tests block the thread */
        _suiteResults[name] = { name: name, status: 'running', pass: 0, fail: 0, error: 0, total: 0 };
        if (_onSuiteStart) _onSuiteStart(name);

        _reset();
        _cleanupList = [];
        log('\n========== ' + name + ' ==========');

        try {
            testView = _createTestView();
            if (testView) {
                _setTestView(testView, 'createSuiteRunner');
                log('[runner] Test view container created OK');
            } else {
                log('[runner] WARNING: Could not create test view for ' + name +
                    ' — tests will run on host view (cleanup fallback active)');
            }
        } catch (e) {
            log('[runner] Failed to create test view: ' + (e.message || e));
        }

        try {
            fn();
        } catch (e) {
            _error++;
            log('  [ERROR] Suite crashed: ' + (e.message || e));
        }

        let total = _pass + _fail + _error;
        let result = {
            name: name,
            status: (_fail > 0 || _error > 0) ? 'fail' : 'pass',
            pass: _pass,
            fail: _fail,
            error: _error,
            total: total
        };

        _suiteResults[name] = result;
        _totalSuites++;
        _totalPass += _pass;
        _totalFail += _fail;
        _totalError += _error;

        log('---------- ' + _pass + '/' + total + ' passed' +
            (_fail > 0 ? ', ' + _fail + ' failed' : '') +
            (_error > 0 ? ', ' + _error + ' errors' : '') + ' ----------');

        _setTestView(null, 'createSuiteRunner cleanup');

        /* ALWAYS schedule deferred cleanup.  Gives LVGL time to process
         * pending animations before widgets are torn down.  Two-pronged:
         * 1. Delete the container (handles 99.9% of cases — all children
         *    are recursively removed)
         * 2. Run _cleanupList (safety net for objects that escaped the
         *    container, e.g. when _createTestView failed) */
        log('[runner] Scheduling cleanup in ' + CLEANUP_DELAY_MS + 'ms');
        let cleanupTimer = new lv.timer(function () {
            log('[runner] Cleanup timer fired');
            if (testView) {
                _destroyTestView(testView);
            }
            _runCleanup();
            if (done) done();
        }, CLEANUP_DELAY_MS, null);

        if (_onSuiteDone) _onSuiteDone(result);
        return result;
    };
}

function onSuiteStart(cb) { _onSuiteStart = cb; }
function onSuiteDone(cb)  { _onSuiteDone  = cb; }

function getSuiteResult(name) {
    return _suiteResults[name] || null;
}

function getAllResults() {
    return _suiteResults;
}

function clearAllResults() {
    _suiteResults = {};
    _totalSuites = 0;
    _totalPass = 0;
    _totalFail = 0;
    _totalError = 0;
}

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

// Compatibility wrappers
function _t(name, fn) { test(name, fn); }
function _log(msg) { log('[legacy] ' + msg); }

export { log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose,
         runSuite, test, it, report, _t, _log,
         createSuiteRunner, getSuiteResult, getAllResults, clearAllResults,
         onSuiteStart, onSuiteDone, getTestView, trackForCleanup };
