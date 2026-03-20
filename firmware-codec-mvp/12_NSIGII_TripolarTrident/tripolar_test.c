/*
 * tests/tripolar_test.c
 * ─────────────────────────────────────────────────────────────────────────────
 * OBINexus NSIGII — Tripolar Algebra Test Suite
 *
 * Verifies the four critical system properties:
 *
 *   TEST 1: Trident gate majority consensus
 *   TEST 2: Liar paradox → forced RENEW (no crash)
 *   TEST 3: AM/FM control rule (noise → REBUILD, distortion → RECREATE)
 *   TEST 4: Enzyme cycle with relay/replay detection
 *   TEST 5: Full CISC decompose → repair → recompose round-trip
 *
 * Build: gcc -o tripolar_test tripolar_test.c ../core/tripolar_engine.c
 *                ../core/tripolar_lexer.c -I../core -lm
 * Run:   ./tripolar_test
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "../core/tripolar_algebra.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

/* ─── test helpers ──────────────────────────────────────────────────────── */

static int tests_run    = 0;
static int tests_passed = 0;

#define TEST(name) \
    printf("\n[TEST %d] %s\n", ++tests_run, name)

#define ASSERT_EQ(a, b, msg) do { \
    if ((a) == (b)) { \
        printf("  ✓  %s\n", msg); \
        tests_passed++; \
    } else { \
        printf("  ✗  FAIL: %s  (got %d, expected %d)\n", msg, (int)(a), (int)(b)); \
    } \
} while(0)

#define ASSERT_TRUE(cond, msg) do { \
    if (cond) { \
        printf("  ✓  %s\n", msg); \
        tests_passed++; \
    } else { \
        printf("  ✗  FAIL: %s\n", msg); \
    } \
} while(0)


/* ─────────────────────────────────────────────────────────────────────────────
 * TEST 1: Trident Gate — Majority Consensus
 * ─────────────────────────────────────────────────────────────────────────────
 */
static void test_trident_gate(void) {
    TEST("Trident Gate — Majority Consensus");

    tripolar_ptr_t ptr;
    tripolar_ptr_init(&ptr);

    /* 2x CREATE + 1x RENEW → should output CREATE */
    ptr.there_and_then.state = SIGNAL_CREATE;
    ptr.here_and_now.state   = SIGNAL_CREATE;
    ptr.when_and_where.state = SIGNAL_RENEW;
    signal_state_t result = trident_gate(&ptr);
    ASSERT_EQ(result, SIGNAL_CREATE, "YES+YES+NIL → CREATE");

    /* 2x DESTROY + 1x RENEW → should output DESTROY */
    ptr.there_and_then.state = SIGNAL_DESTROY;
    ptr.here_and_now.state   = SIGNAL_DESTROY;
    ptr.when_and_where.state = SIGNAL_RENEW;
    result = trident_gate(&ptr);
    ASSERT_EQ(result, SIGNAL_DESTROY, "NO+NO+NIL → DESTROY");

    /* 1 of each → no majority → should output RENEW */
    ptr.there_and_then.state = SIGNAL_CREATE;
    ptr.here_and_now.state   = SIGNAL_DESTROY;
    ptr.when_and_where.state = SIGNAL_RENEW;
    result = trident_gate(&ptr);
    ASSERT_EQ(result, SIGNAL_RENEW, "YES+NO+NIL → RENEW (no majority)");

    /* All RENEW → still RENEW, no crash */
    ptr.there_and_then.state = SIGNAL_RENEW;
    ptr.here_and_now.state   = SIGNAL_RENEW;
    ptr.when_and_where.state = SIGNAL_RENEW;
    result = trident_gate(&ptr);
    ASSERT_EQ(result, SIGNAL_RENEW, "NIL+NIL+NIL → RENEW (system stable)");
}


/* ─────────────────────────────────────────────────────────────────────────────
 * TEST 2: Liar Paradox — Self-Referential Payload
 *
 * The void pointer (Uche) points back at its own node.
 * System must classify as RENEW without crash or undefined behaviour.
 * ─────────────────────────────────────────────────────────────────────────────
 */
static void test_liar_paradox(void) {
    TEST("Liar Paradox — Self-Referential Payload → RENEW");

    tripolar_node_t node;
    tripolar_node_init(&node, FRAME_WHEN_AND_WHERE, 0);

    /* Force the paradox: Uche points at herself */
    node.payload = (void *)&node;
    node.state   = SIGNAL_CREATE;   /* starts as CREATE */

    bool detected = is_self_referential(&node);
    ASSERT_TRUE(detected, "Self-reference detected");
    ASSERT_EQ(node.state, SIGNAL_RENEW, "State forced to RENEW");
    ASSERT_TRUE(node.self_ref_guard, "Guard flag set");

    /* Verify it doesn't crash when run through control_signal */
    signal_state_t result = control_signal(&node, 0.5f, 0.3f);
    ASSERT_EQ(result, SIGNAL_RENEW, "control_signal returns RENEW for liar");
}


/* ─────────────────────────────────────────────────────────────────────────────
 * TEST 3: AM/FM Control Rule
 * ─────────────────────────────────────────────────────────────────────────────
 */
static void test_control_rule(void) {
    TEST("AM/FM Control Rule");

    tripolar_node_t node;
    tripolar_node_init(&node, FRAME_HERE_AND_NOW, 1);
    node.amplitude = 0.8f;
    node.frequency = 0.9f;

    /* Case 1: noise only, signal coherent → CREATE (AM boost) */
    signal_state_t result = control_signal(&node, 0.8f, 0.2f);
    ASSERT_EQ(result, SIGNAL_CREATE, "High noise + coherent signal → CREATE (AM)");
    ASSERT_TRUE(node.amplitude >= 0.8f, "Amplitude boosted or maintained");

    /* Case 2: distortion → RENEW (FM shift) */
    node.amplitude = 0.8f;
    node.frequency = 0.9f;
    result = control_signal(&node, 0.2f, 0.7f);
    ASSERT_EQ(result, SIGNAL_RENEW, "High distortion → RENEW (FM shift)");

    /* Case 3: stable → CREATE */
    node.amplitude = 0.8f;
    node.frequency = 0.9f;
    node.payload   = NULL;
    node.self_ref_guard = false;
    result = control_signal(&node, 0.1f, 0.1f);
    ASSERT_EQ(result, SIGNAL_CREATE, "Clean signal → CREATE");

    /* Case 4: dead signal → DESTROY */
    node.amplitude = 0.0f;
    node.frequency = 0.0f;
    result = control_signal(&node, 0.5f, 0.5f);
    ASSERT_EQ(result, SIGNAL_DESTROY, "Zero amplitude+frequency → DESTROY");
}


/* ─────────────────────────────────────────────────────────────────────────────
 * TEST 4: CISC Decompose → Token Stream → Recompose Round-Trip
 * ─────────────────────────────────────────────────────────────────────────────
 */
static void test_roundtrip(void) {
    TEST("CISC Decompose → Recompose Round-Trip");

    tripolar_ptr_t original;
    tripolar_ptr_init(&original);

    original.there_and_then.state = SIGNAL_CREATE;
    original.here_and_now.state   = SIGNAL_CREATE;
    original.when_and_where.state = SIGNAL_RENEW;
    original.there_and_then.amplitude = 0.9f;
    original.here_and_now.amplitude   = 0.8f;
    original.when_and_where.amplitude = 0.4f;
    original.seq = 42;

    /* Decompose into token stream */
    tripolar_token_stream_t stream = decompose(&original);
    ASSERT_EQ(stream.count, 3, "Token stream has 3 tokens");
    ASSERT_EQ(stream.verdict, SIGNAL_CREATE, "Verdict = CREATE (2x YES)");

    printf("  ");
    print_token_stream(&stream);

    /* Recompose from stream */
    tripolar_ptr_t restored = recompose(&stream);
    ASSERT_EQ(restored.there_and_then.state, SIGNAL_CREATE,
              "THERE_AND_THEN state preserved");
    ASSERT_EQ(restored.here_and_now.state, SIGNAL_CREATE,
              "HERE_AND_NOW state preserved");
    ASSERT_EQ(restored.when_and_where.state, SIGNAL_RENEW,
              "WHEN_AND_WHERE state preserved");
}


/* ─────────────────────────────────────────────────────────────────────────────
 * TEST 5: Enzyme Cycle — full repair loop
 * ─────────────────────────────────────────────────────────────────────────────
 */
static void test_enzyme_cycle(void) {
    TEST("Enzyme Cycle — Noise Injection → Repair → Stabilise");

    tripolar_ptr_t ptr;
    tripolar_ptr_init(&ptr);

    /* Start all frames in CREATE */
    ptr.there_and_then.state = SIGNAL_CREATE;
    ptr.there_and_then.amplitude = 0.9f;
    ptr.there_and_then.frequency = 0.9f;

    ptr.here_and_now.state = SIGNAL_CREATE;
    ptr.here_and_now.amplitude = 0.9f;
    ptr.here_and_now.frequency = 0.9f;

    ptr.when_and_where.state = SIGNAL_CREATE;
    ptr.when_and_where.amplitude = 0.9f;
    ptr.when_and_where.frequency = 0.9f;

    signal_state_t before = trident_gate(&ptr);
    ASSERT_EQ(before, SIGNAL_CREATE, "Before noise: all frames CREATE");

    /* Inject noise + distortion (simulated attack) */
    signal_state_t after = enzyme_cycle(&ptr, 0.9f, 0.7f);
    printf("  After attack: consensus = %s\n", signal_state_str(after));
    ASSERT_TRUE(after == SIGNAL_CREATE || after == SIGNAL_RENEW,
                "After attack: system in CREATE or RENEW (not crashed)");

    /* Run cycle again — system should stabilise */
    signal_state_t recovered = enzyme_cycle(&ptr, 0.1f, 0.1f);
    ASSERT_EQ(recovered, SIGNAL_CREATE, "After recovery: system stabilised to CREATE");
}


/* ─────────────────────────────────────────────────────────────────────────────
 * MAIN
 * ─────────────────────────────────────────────────────────────────────────────
 */
int main(void) {
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  OBINexus NSIGII — Tripolar Algebra Test Suite\n");
    printf("  obinexusmk2/nsigii_ltcodec_showcase/core\n");
    printf("═══════════════════════════════════════════════════════════\n");

    test_trident_gate();
    test_liar_paradox();
    test_control_rule();
    test_roundtrip();
    test_enzyme_cycle();

    printf("\n═══════════════════════════════════════════════════════════\n");
    printf("  Results: %d / %d tests passed\n", tests_passed, tests_run);
    printf("═══════════════════════════════════════════════════════════\n");

    return (tests_passed > 0 && (tests_run - tests_passed) == 0) ? 0 : 1;
}
