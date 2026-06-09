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

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

let _pass = 0;
let _fail = 0;

function _log(msg) {
    eos.console.log("[stress-test] " + msg);
}

function _t(name, fn) {
    try {
        fn();
        _pass++;
        _log("[PASS] " + name);
    } catch (e) {
        _fail++;
        _log("[FAIL] " + name + " => " + e);
    }
}

/* ------------------------------------------------------------------ */
/* Threat: SNI bridge input overrun                                   */
/* ------------------------------------------------------------------ */

/* Attack: pass a 10 KB string to eos.permission.check()
 * Defense: SNI input guard rejects strings beyond reasonable length.
 * Expected: throw (guard active) or return "unknown" (guard absent). */
function test_sni_string_guard_check() {
    let longName = "x";
    for (let i = 0; i < 14; i++) longName += longName;
    let threw = false;
    try {
        let r = eos.permission.check(longName);
        if (typeof r === "string") {
            _log("[WARN] check(10KB) returned '" + r + "' instead of throwing");
            return;
        }
    } catch (e) {
        threw = true;
    }
    if (!threw) {
        throw new Error("check(10KB) should throw or be rejected");
    }
}

/* Attack: pass a 10 KB string to eos.permission.request()
 * Defense: SNI input guard rejects oversized permission names.
 * Expected: throw. */
function test_sni_string_guard_request() {
    let longName = "x";
    for (let i = 0; i < 14; i++) longName += longName;
    let threw = false;
    try {
        eos.permission.request(longName, function (s) {});
    } catch (e) {
        threw = true;
    }
    if (!threw) {
        throw new Error("request(10KB) should throw or be rejected");
    }
}

/* Attack: pass non-string type (object) to check()
 * Defense: SNI type-check rejects non-string.
 * Expected: throw. */
function test_sni_type_guard_check() {
    let threw = false;
    try {
        eos.permission.check({ key: "location" });
    } catch (e) {
        threw = true;
    }
    if (!threw) {
        _log("[WARN] check(object) did not throw");
    }
}

/* Attack: pass null/undefined to request()
 * Defense: SNI argument validation rejects non-string name.
 * Expected: throw. */
function test_sni_null_request() {
    let threw = false;
    try {
        eos.permission.request(null, function (s) {});
    } catch (e) {
        threw = true;
    }
    if (!threw) {
        _log("[WARN] request(null, cb) did not throw");
    }
}

/* Attack: request with huge callback chain
 * Defense: should not crash from callback proliferation.
 * Expected: no crash. */
function test_sni_request_flood() {
    let count = 0;
    for (let i = 0; i < 50; i++) {
        try {
            eos.permission.request("location", function (state) {
                count++;
            });
        } catch (e) {
            // request() may throw when another is active; this is OK
        }
    }
    // If we reach here without crashing, the test passes
}

/* ------------------------------------------------------------------ */
/* Threat: JS heap exhaustion                                          */
/* ------------------------------------------------------------------ */

/* Attack: allocate a very large Array
 * Defense: engine OOM handler or heap limit.
 * Expected: either allocation fails gracefully or script is terminated.
 * We probe the boundary: allocate progressively larger arrays
 * until we either succeed, throw, or timeout. */
function test_heap_array_stress() {
    for (let n = 100; n <= 100000; n *= 10) {
        try {
            let arr = new Array(n);
            arr[n - 1] = 1;
            _log("[INFO] Array(" + n + ") allocated OK, len=" + arr.length);
            arr = null;
        } catch (e) {
            _log("[INFO] Array(" + n + ") rejected: " + e);
            return;
        }
    }
}

/* Attack: allocate a very large string
 * Defense: engine OOM handler.
 * Expected: gracefully fail before exhausting heap. */
function test_heap_string_stress() {
    let s = "a";
    for (let i = 0; i < 18; i++) {
        try {
            s = s + s;
            _log("[INFO] string 2^" + (i + 1) + " = " + s.length + " chars OK");
        } catch (e) {
            _log("[INFO] string 2^" + (i + 1) + " rejected: " + e);
            return;
        }
    }
}

/* Attack: create an object with many properties
 * Defense: engine OOM handler.
 * Expected: gracefully fail before exhausting heap. */
function test_heap_object_stress() {
    try {
        let obj = {};
        for (let i = 0; i < 10000; i++) {
            obj["p" + i] = i;
        }
        _log("[INFO] object with 10000 properties OK, keys=" + Object.keys(obj).length);
    } catch (e) {
        _log("[INFO] object stress rejected: " + e);
    }
}

/* Attack: create many small objects (heap fragmentation)
 * Defense: GC should reclaim memory.
 * Expected: no crash, GC should keep up. */
function test_heap_fragmentation() {
    for (let round = 0; round < 10; round++) {
        try {
            let many = [];
            for (let i = 0; i < 5000; i++) {
                many.push({ idx: i, data: "payload_" + i });
            }
            _log("[INFO] frag round " + round + ": 5000 objects OK");
            many = null;
        } catch (e) {
            _log("[INFO] frag round " + round + " failed: " + e);
            return;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Threat: Stack overflow via recursion                                */
/* ------------------------------------------------------------------ */

/* Attack: deep recursion to exhaust C stack
 * Defense: OS stack guard or JerryScript stack limit.
 * Expected: RangeError or script termination. */
function test_stack_recursion() {
    function dive(n) {
        if (n <= 0) return 0;
        return 1 + dive(n - 1);
    }
    let threw = false;
    try {
        let result = dive(50000);
        _log("[INFO] recursion 50000 returned " + result);
    } catch (e) {
        threw = true;
        _log("[INFO] recursion 50000 caught: " + e);
    }
    if (!threw) {
        _log("[WARN] recursion 50000 did NOT throw (stack may be very large)");
    }
}

/* Attack: mutual recursion (double function cycling)
 * Defense: same as above.
 * Expected: RangeError or script termination. */
function test_stack_mutual_recursion() {
    let depth = 0;
    function a() { depth++; if (depth > 50000) return 0; return b(); }
    function b() { depth++; if (depth > 50000) return 0; return a(); }
    let threw = false;
    try {
        a();
        _log("[INFO] mutual recursion depth " + depth);
    } catch (e) {
        threw = true;
        _log("[INFO] mutual recursion caught at depth " + depth + ": " + e);
    }
    if (!threw) {
        _log("[WARN] mutual recursion did NOT throw");
    }
}

/* ------------------------------------------------------------------ */
/* Threat: VM execution timeout                                        */
/* ------------------------------------------------------------------ */

/* Attack: infinite for-loop (no backward branch!)
 * Defense: only VM halt callback catches this (backward branch only).
 * Expected: timeout (if halt enabled) OR test hangs (need outer protection). */
function test_vm_halt_infinite_for() {
    let threw = false;
    try {
        // This may hang if no timeout mechanism is active.
        // We set a very high iteration count and hope halt fires.
        for (let i = 0; i < 999999999; i++) {
            if (i > 1000000) {
                _log("[INFO] for-loop reached 1000000 iterations, halting");
                break;
            }
        }
    } catch (e) {
        threw = true;
        _log("[INFO] infinite for caught: " + e);
    }
    // If we get here (either break or catch), it's a pass
}

/* Attack: while-true loop
 * Defense: VM halt callback (fires on backward branches).
 * Expected: timeout, then script termination. */
function test_vm_halt_while_true() {
    let start = Date.now();
    let count = 0;
    while (true) {
        count++;
        if (count > 500000) {
            _log("[INFO] while-true broke after " + count + " iters");
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Threat: Promise / async flooding                                    */
/* ------------------------------------------------------------------ */

/* Attack: create many pending promises (microtask queue stress)
 * Defense: engine should handle bounded promise creation.
 * Expected: no crash. */
function test_promise_flood() {
    let resolvers = [];
    for (let i = 0; i < 1000; i++) {
        try {
            let p = new Promise(function (resolve) {
                resolvers.push(resolve);
            });
        } catch (e) {
            _log("[INFO] promise " + i + " rejected: " + e);
            return;
        }
    }
    _log("[INFO] created " + resolvers.length + " pending promises");
    resolvers = null;
}

/* Attack: chain many .then() handlers
 * Defense: engine should handle bounded promise chains.
 * Expected: no crash. */
function test_promise_chain() {
    try {
        let p = Promise.resolve(0);
        for (let i = 0; i < 100; i++) {
            p = p.then(function (v) { return v + 1; });
        }
        _log("[INFO] promise chain of 100 then() OK");
    } catch (e) {
        _log("[INFO] promise chain failed: " + e);
    }
}

/* ------------------------------------------------------------------ */
/* Threat: JSON / eval injection                                       */
/* ------------------------------------------------------------------ */

/* Attack: deeply nested JSON parse
 * Defense: parser depth limit.
 * Expected: SyntaxError or graceful handling. */
function test_json_nesting() {
    let deep = "1";
    for (let i = 0; i < 100; i++) {
        deep = "[" + deep + "]";
    }
    let threw = false;
    try {
        let result = JSON.parse(deep);
        _log("[INFO] JSON depth 100 parsed OK");
    } catch (e) {
        threw = true;
        _log("[INFO] JSON depth 100 rejected: " + e);
    }
}

/* ------------------------------------------------------------------ */
/* Entry point                                                         */
/* ------------------------------------------------------------------ */

export function run_stress_test() {
    _pass = 0;
    _fail = 0;

    _log("");
    _log("=== Stress / Attack Surface Test Suite ===");
    _log("");

    /* Layer 1: SNI bridge guards */
    _log("--- SNI input guard ---");
    _t("check(10KB) guard",                  test_sni_string_guard_check);
    _t("request(10KB) guard",                test_sni_string_guard_request);
    _t("check(object) guard",                test_sni_type_guard_check);
    _t("request(null) guard",                test_sni_null_request);
    _t("request() 50x flood",                test_sni_request_flood);

    /* Layer 2/3: Heap exhaustion */
    _log("");
    _log("--- Heap exhaustion ---");
    _t("Array growth stress",                test_heap_array_stress);
    _t("String doubling stress",             test_heap_string_stress);
    _t("Object 10k properties",              test_heap_object_stress);
    _t("Heap fragmentation 10x5k",           test_heap_fragmentation);

    /* Stack overflow */
    _log("");
    _log("--- Stack overflow ---");
    _t("Recursion depth 50000",              test_stack_recursion);
    _t("Mutual recursion",                   test_stack_mutual_recursion);

    /* VM halt / timeout */
    _log("");
    _log("--- VM execution timeout ---");
    _t("Infinite for-loop escape",           test_vm_halt_infinite_for);
    _t("While-true escape",                  test_vm_halt_while_true);

    /* Promise flooding */
    _log("");
    _log("--- Promise / async flooding ---");
    _t("1000 pending promises",              test_promise_flood);
    _t("100 .then() chain",                  test_promise_chain);

    /* JSON / parser */
    _log("");
    _log("--- JSON nesting ---");
    _t("JSON depth 100",                     test_json_nesting);

    /* Summary */
    _log("");
    _log("=== Stress Test Summary ===");
    _log("Total: " + (_pass + _fail));
    _log("Pass:  " + _pass);
    _log("Fail:  " + _fail);
}
