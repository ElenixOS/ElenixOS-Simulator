/**
 * eos Stress / Attack-surface test
 *
 * Probes system defensive capabilities against:
 *  - SNI bridge abuse (oversized inputs)
 *  - JS heap exhaustion (arrays, strings, objects)
 *  - Stack overflow (recursion)
 *  - VM halt / timeout (infinite loop)
 *  - Promise / callback flooding
 *
 * Each test targets a specific threat model.
 * If a defense layer is missing, the test FAILs gracefully
 * (the _t wrapper catches exceptions and continues).
 *
 * Log rules: no Chinese characters. Each entry is [PASS] or [FAIL].
 */
import { test, log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose, runSuite } from './framework.mjs';

export function suite() {
    runSuite('stress', () => {
    log("");
    log("=== Stress / Attack Surface Test Suite ===");
    log("");

    /* Layer 1: SNI bridge guards */
    log("--- SNI input guard ---");
    test("check(10KB) guard",                  test_sni_string_guard_check);
    test("request(10KB) guard",                test_sni_string_guard_request);
    test("check(object) guard",                test_sni_type_guard_check);
    test("request(null) guard",                test_sni_null_request);
    test("request() 50x flood",                test_sni_request_flood);

    /* Layer 2/3: Heap exhaustion */
    log("");
    log("--- Heap exhaustion ---");
    test("Array growth stress",                test_heap_array_stress);
    test("String doubling stress",             test_heap_string_stress);
    test("Object 10k properties",              test_heap_object_stress);
    test("Heap fragmentation 10x5k",           test_heap_fragmentation);

    /* Stack overflow */
    log("");
    log("--- Stack overflow ---");
    test("Recursion depth 50000",              test_stack_recursion);
    test("Mutual recursion",                   test_stack_mutual_recursion);

    /* VM halt / timeout */
    log("");
    log("--- VM execution timeout ---");
    test("Infinite for-loop escape",           test_vm_halt_infinite_for);
    test("While-true escape",                  test_vm_halt_while_true);

    /* Promise flooding */
    log("");
    log("--- Promise / async flooding ---");
    test("1000 pending promises",              test_promise_flood);
    test("100 .then() chain",                  test_promise_chain);

    /* JSON / parser */
    log("");
    log("--- JSON nesting ---");
    test("JSON depth 100",                     test_json_nesting);

    /* Summary */
    log("");
    log("=== Stress Test Summary ===");
    log("Total: " + (_pass + _fail));
    log("Pass:  " + _pass);
    log("Fail:  " + _fail);
    });
}
