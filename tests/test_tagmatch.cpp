/*
 * test_tagmatch.cpp — correctness tests for tagmatch_nb.
 *
 * Strategy: spawn tagmatch_nb with -debug and parse
 *   "DEBUG rank=X nprocs=Y ntags=N sendorder=... recvorder=... prepost=0|1
 *    wildcard=0|1 check=OK|FAIL"
 * lines.
 *
 * Properties checked:
 *   Coverage      — both ranks (0 and 1) appear in the DEBUG output.
 *   Nprocs        — every rank reports nprocs=2.
 *   DataIntegrity — every rank reports check=OK across all
 *                   sendorder × recvorder × prepost combinations.
 *                   With non-wildcard, the receiver verifies that slot i
 *                   contains the tag it requested (recv_tags[i]).
 *   CliGuards     — bad order name, ntags < 1 each abort cleanly.
 *
 * Requires exactly 2 MPI ranks.
 *
 * Note: tests use small -ntags (8-32) to keep ctest fast.  The
 * O(N²) signal scales with ntags but is not exercised here; it's a
 * micro-benchmark property, not a correctness property.
 */
#include <gtest/gtest.h>
#include <string>
#include "popen_helpers.h"

using blink::DebugMap;
using blink::DebugRun;
using blink::run_debug;
using blink::run_debug_capture;
using blink::check_all_ok;
using blink::check_coverage;
using blink::check_nprocs;

/* ── tests ──────────────────────────────────────────────────────────────────── */

class TagmatchTest : public ::testing::Test {
protected:
    static constexpr int N = 2;       /* benchmark requires exactly 2 ranks */
    static constexpr int ntags = 16;  /* small for speed */
};

TEST_F(TagmatchTest, Coverage) {
    auto m = run_debug("tagmatch_nb", N, "-ntags 16");
    check_coverage(m, N, "tagmatch default");
}

TEST_F(TagmatchTest, Nprocs) {
    auto m = run_debug("tagmatch_nb", N, "-ntags 16");
    check_nprocs(m, N, "tagmatch default");
}

/* ── DataIntegrity matrix: sender × receiver × scheduling × wildcard ───────── */

TEST_F(TagmatchTest, Default_UMQ_inc_dec) {
    auto m = run_debug("tagmatch_nb", N, "-ntags 16");
    check_coverage(m, N, "UMQ inc/dec");
    check_all_ok(m, "UMQ inc/dec");
}

TEST_F(TagmatchTest, PRQ_inc_dec) {
    auto m = run_debug("tagmatch_nb", N, "-ntags 16 -prepost");
    check_coverage(m, N, "PRQ inc/dec");
    check_all_ok(m, "PRQ inc/dec");
}

TEST_F(TagmatchTest, Same_inc_inc) {
    auto m = run_debug("tagmatch_nb", N, "-ntags 16 -sendorder inc -recvorder inc");
    check_all_ok(m, "same inc/inc");
}

TEST_F(TagmatchTest, Same_dec_dec) {
    auto m = run_debug("tagmatch_nb", N, "-ntags 16 -sendorder dec -recvorder dec");
    check_all_ok(m, "same dec/dec");
}

TEST_F(TagmatchTest, SameAlias) {
    /* 'same' should behave like 'inc' on both sides */
    auto m = run_debug("tagmatch_nb", N, "-ntags 16 -sendorder same -recvorder same");
    check_all_ok(m, "same alias");
}

TEST_F(TagmatchTest, Random_Random) {
    /* both ranks share a seed → same permutation, just used differently */
    auto m = run_debug("tagmatch_nb", N, "-ntags 16 -sendorder random -recvorder random -seed 42");
    check_all_ok(m, "random/random");
}

TEST_F(TagmatchTest, Random_Inc_PRQ) {
    /* mixed pattern with prepost; harder for verification because some
     * messages may go through the racy interleaved state                 */
    auto m = run_debug("tagmatch_nb", N, "-ntags 32 -sendorder random -recvorder inc -prepost");
    check_all_ok(m, "random/inc PRQ");
}

TEST_F(TagmatchTest, Wildcard_UMQ) {
    /* MPI_ANY_TAG mode — verification is skipped on receiver, but the
     * benchmark must still complete cleanly and emit check=OK            */
    auto m = run_debug("tagmatch_nb", N, "-ntags 16 -wildcard");
    check_all_ok(m, "wildcard UMQ");
}

TEST_F(TagmatchTest, Wildcard_PRQ) {
    auto m = run_debug("tagmatch_nb", N, "-ntags 16 -wildcard -prepost");
    check_all_ok(m, "wildcard PRQ");
}

/* Larger ntags than the default 16, in opposite orders — the worst-case
 * pathology should still complete in milliseconds at this scale, but exercises
 * the multi-page allocation and request array path.                          */
TEST_F(TagmatchTest, Larger_ntags_opposite) {
    auto m = run_debug("tagmatch_nb", N, "-ntags 2048");
    check_all_ok(m, "ntags=2048 inc/dec");
}

/* Multiple iterations — verify no cross-iteration message contamination,
 * since each timed window starts with a barrier and ends with a Waitall. */
TEST_F(TagmatchTest, MultiIter) {
    auto m = run_debug("tagmatch_nb", N, "-ntags 16 -iter 5 -warmup 2");
    check_all_ok(m, "multi-iter");
}

/* ── CLI validation guards ──────────────────────────────────────────────────── */

TEST_F(TagmatchTest, BadSendorderAbortsCleanly) {
    DebugRun r = run_debug_capture("tagmatch_nb", N, "-ntags 8 -sendorder bogus");
    EXPECT_FALSE(r.normal_exit && r.exit_code == 0)
        << "bogus -sendorder should abort.\nstdout:\n" << r.stdout_raw;
    EXPECT_NE(r.stderr_raw.find("unknown order"), std::string::npos)
        << "expected an order-name diagnostic.\nstderr:\n" << r.stderr_raw;
}

TEST_F(TagmatchTest, BadRecvorderAbortsCleanly) {
    DebugRun r = run_debug_capture("tagmatch_nb", N, "-ntags 8 -recvorder bogus");
    EXPECT_FALSE(r.normal_exit && r.exit_code == 0)
        << "bogus -recvorder should abort.\nstdout:\n" << r.stdout_raw;
    EXPECT_NE(r.stderr_raw.find("unknown order"), std::string::npos)
        << "expected an order-name diagnostic.\nstderr:\n" << r.stderr_raw;
}

TEST_F(TagmatchTest, BadNtagsAbortsCleanly) {
    DebugRun r = run_debug_capture("tagmatch_nb", N, "-ntags 0");
    EXPECT_FALSE(r.normal_exit && r.exit_code == 0)
        << "-ntags 0 should abort.\nstdout:\n" << r.stdout_raw;
    EXPECT_NE(r.stderr_raw.find("must be >= 1"), std::string::npos)
        << "expected an ntags diagnostic.\nstderr:\n" << r.stderr_raw;
}

/* Wrong rank count must fail cleanly — benchmark requires exactly 2. */
TEST(TagmatchRankCount, WrongRankCountAbortsCleanly) {
    DebugRun r = run_debug_capture("tagmatch_nb", 4, "-ntags 8");
    EXPECT_FALSE(r.normal_exit && r.exit_code == 0)
        << "tagmatch_nb with 4 ranks should abort.\nstdout:\n" << r.stdout_raw;
    EXPECT_NE(r.stderr_raw.find("requires exactly 2 ranks"), std::string::npos)
        << "expected a rank-count diagnostic.\nstderr:\n" << r.stderr_raw;
}
