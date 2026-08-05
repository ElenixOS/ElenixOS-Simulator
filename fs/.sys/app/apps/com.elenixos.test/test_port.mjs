/**
 * JerryScript Port coverage test
 *
 * Verifies that the ElenixOS kernel jerry_port.c functions work correctly
 * through the JS APIs that exercise them:
 *
 *   jerry_port_current_time()  →  Date.now(), new Date()
 *   jerry_port_local_tza()     →  new Date().getTimezoneOffset()
 *   jerry_port_path_normalize() → module imports, file I/O
 *   jerry_port_source_read()   → module imports (ES import)
 *   jerry_port_*fs*()          → eos.config.setStr/getStr
 *   jerry_port_log()           → (indirect, via engine messages)
 *
 * Log rules: no Chinese characters. Each entry is [PASS] or [FAIL].
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose, runSuite } from './framework.mjs';

export function suite() {
    runSuite('port (time + fs io)', () => {

    // ====================================================================
    // Time — jerry_port_current_time() + jerry_port_local_tza()
    // ====================================================================

    test("Date.now() returns a number > 0", () => {
        let now = Date.now();
        assertType(now, 'number');
        assertOk(now > 0, 'Date.now() should be > 0');
    });

    test("Date.now() is after 2020-01-01 (1577836800000 ms)", () => {
        let now = Date.now();
        let epoch2020 = 1577836800000;
        assertOk(now > epoch2020, 'Date.now() should be after 2020 (epoch ms too small: ' + now + ')');
    });

    test("Date.now() is within reasonable range (< 2100)", () => {
        let now = Date.now();
        let epoch2100 = 4102444800000;
        assertOk(now < epoch2100, 'Date.now() should be before 2100 (epoch ms too large: ' + now + ')');
    });

    test("new Date() returns valid Date object", () => {
        let d = new Date();
        assertNotNull(d, 'new Date() returned null');
    });

    test("new Date().getTime() is a number", () => {
        let d = new Date();
        assertType(d.getTime(), 'number');
    });

    test("new Date().getTime() matches Date.now() within 1000ms", () => {
        let before = Date.now();
        let d = new Date();
        let after = Date.now();
        let ts = d.getTime();
        assertOk(ts >= before - 100, 'timestamp should be >= before - 100ms');
        assertOk(ts <= after + 100, 'timestamp should be <= after + 100ms');
    });

    test("new Date().getFullYear() is current year (2025-2030)", () => {
        let year = new Date().getFullYear();
        assertType(year, 'number');
        assertOk(year >= 2025 && year <= 2030, 'year should be 2025-2030, got ' + year);
    });

    test("new Date().getMonth() is 0-11", () => {
        let month = new Date().getMonth();
        assertOk(month >= 0 && month <= 11, 'month should be 0-11, got ' + month);
    });

    test("new Date().getDate() is 1-31", () => {
        let day = new Date().getDate();
        assertOk(day >= 1 && day <= 31, 'day should be 1-31, got ' + day);
    });

    test("new Date().getHours() is 0-23", () => {
        let hour = new Date().getHours();
        assertOk(hour >= 0 && hour <= 23, 'hour should be 0-23, got ' + hour);
    });

    test("new Date().getMinutes() is 0-59", () => {
        let min = new Date().getMinutes();
        assertOk(min >= 0 && min <= 59, 'min should be 0-59, got ' + min);
    });

    test("new Date().getSeconds() is 0-59", () => {
        let sec = new Date().getSeconds();
        assertOk(sec >= 0 && sec <= 59, 'sec should be 0-59, got ' + sec);
    });

    test("new Date().getDay() is 0-6", () => {
        let dow = new Date().getDay();
        assertOk(dow >= 0 && dow <= 6, 'day of week should be 0-6, got ' + dow);
    });

    // -- jerry_port_local_tza() → getTimezoneOffset()

    test("getTimezoneOffset() returns a number", () => {
        let tzo = new Date().getTimezoneOffset();
        assertType(tzo, 'number');
    });

    test("getTimezoneOffset() is within range [-840, 840]", () => {
        // UTC+14 (Kiritimati) to UTC-12 (Baker Island) → -840 to 720
        // (getTimezoneOffset returns MINUTES to add to local to get UTC,
        //  so UTC+8 = -480, UTC-5 = 300)
        let tzo = new Date().getTimezoneOffset();
        assertOk(tzo >= -840 && tzo <= 840, 'timezone offset out of range: ' + tzo);
    });

    test("getTimezoneOffset() is an integer", () => {
        let tzo = new Date().getTimezoneOffset();
        assertEqual(tzo, Math.floor(tzo));
    });

    // -- Date from explicit UTC timestamp

    test("new Date(0) creates epoch (1970-01-01T00:00:00Z)", () => {
        let d = new Date(0);
        assertEqual(d.getUTCFullYear(), 1970);
        assertEqual(d.getUTCMonth(), 0);
        assertEqual(d.getUTCDate(), 1);
        assertEqual(d.getUTCHours(), 0);
        assertEqual(d.getUTCMinutes(), 0);
        assertEqual(d.getUTCSeconds(), 0);
    });

    test("new Date('2025-01-01T00:00:00Z').getTime() is valid", () => {
        let d = new Date('2025-01-01T00:00:00Z');
        assertType(d.getTime(), 'number');
        assertOk(d.getTime() > 0);
    });

    test("Date.UTC(2025, 0, 1) returns a valid number", () => {
        let ms = Date.UTC(2025, 0, 1);
        assertType(ms, 'number');
        assertOk(ms > 0);
    });

    // -- Two successive Date.now() calls are non-decreasing

    test("Date.now() is non-decreasing in quick succession", () => {
        let t1 = Date.now();
        let t2 = Date.now();
        assertOk(t2 >= t1, 't2(' + t2 + ') should be >= t1(' + t1 + ')');
    });

    // ====================================================================
    // File I/O — jerry_port_source_read(), jerry_port_path_normalize()
    // (tested implicitly via eos.config which uses eos_storage_* → eos_fs_*)
    // ====================================================================

    test("eos.config.setStr stores a key", () => {
        // Should not throw
        eos.config.setStr("test_port_key", "hello_port");
    });

    test("eos.config.getStr retrieves written key", () => {
        let val = eos.config.getStr("test_port_key");
        assertEqual(val, "hello_port");
    });

    test("eos.config.setBool stores a boolean", () => {
        eos.config.setBool("test_port_bool", true);
    });

    test("eos.config.getBool retrieves written boolean", () => {
        let val = eos.config.getBool("test_port_bool");
        assertOk(val === true, 'expected true, got ' + val);
    });

    test("eos.config.setNumber stores a number", () => {
        eos.config.setNumber("test_port_num", 12345.678);
    });

    test("eos.config.getNumber retrieves written number", () => {
        let val = eos.config.getNumber("test_port_num");
        assertClose(val, 12345.678, 0.001);
    });

    test("eos.config.getStr returns undefined for missing key", () => {
        let val = eos.config.getStr("test_port_nonexistent_key_xyz");
        assertEqual(val, undefined);
    });

    test("eos.config.getBool returns false for missing key", () => {
        let val = eos.config.getBool("test_port_nonexistent_bool_xyz");
        assertEqual(val, false);
    });

    test("eos.config.getNumber returns 0 for missing key", () => {
        let val = eos.config.getNumber("test_port_nonexistent_num_xyz");
        assertEqual(val, 0);
    });

    // -- Multiple writes to same key

    test("eos.config.setStr overwrites existing key", () => {
        eos.config.setStr("test_port_overwrite", "first");
        eos.config.setStr("test_port_overwrite", "second");
        let val = eos.config.getStr("test_port_overwrite");
        assertEqual(val, "second");
    });

    // -- Special characters in config value

    test("eos.config.setStr handles JSON special chars", () => {
        eos.config.setStr("test_port_special", '{\"key\": \"value\", \"arr\": [1,2,3]}');
        let val = eos.config.getStr("test_port_special");
        assertType(val, 'string');
        assertOk(val.length > 0);
    });

    // -- Module imports work (proves jerry_port_source_read works) --
    // All the ES imports at the top of this file go through
    // jerry_port_source_read → eos_fs_open_read.

    test("ES module imports resolved successfully (framework loaded)", () => {
        // If we can call log/assertEqual, the module import chain worked.
        // framework.mjs → test_common.mjs → all imported via jerry_port_source_read.
        assertOk(typeof log === 'function', 'log should be a function');
        assertOk(typeof assertEqual === 'function', 'assertEqual should be a function');
        assertOk(typeof runSuite === 'function', 'runSuite should be a function');
        assertOk(typeof test === 'function', 'test should be a function');
    });

    });
}
