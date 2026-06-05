/*
 * test_pingpong.cpp — correctness tests for pingpong benchmarks.
 *
 * Strategy: spawn the benchmark with -debug and parse
 * "DEBUG rank=X nprocs=Y [partner=Z] check=OK|FAIL" lines.
 *
 * Properties checked:
 *   Coverage      — every rank 0..N-1 appears exactly once.
 *   Nprocs        — every rank reports the correct communicator size.
 *   DataIntegrity — every rank reports check=OK.
 *                   Each rank fills send_buf with its own rank byte;
 *                   after the ping-pong it verifies recv_buf contains
 *                   the partner's rank byte throughout.
 *
 * pingpong_b          requires exactly 2 MPI ranks.
 * pingpong_pairwise_b requires 8 MPI ranks (even number).
 */
#include <gtest/gtest.h>
#include <string>
#include "popen_helpers.h"

using blink::run_debug;
using blink::check_all_ok;
using blink::check_coverage;
using blink::check_nprocs;

/* ── pingpong_b (2 ranks) ───────────────────────────────────────────────────── */

class PingpongBTest : public ::testing::Test {
protected:
    static constexpr int N = 2;
};

TEST_F(PingpongBTest, Coverage) {
    auto m = run_debug("pingpong_b", N);
    check_coverage(m, N, "pingpong_b");
}
TEST_F(PingpongBTest, Nprocs) {
    auto m = run_debug("pingpong_b", N);
    check_nprocs(m, N, "pingpong_b");
}
TEST_F(PingpongBTest, DataIntegrity) {
    auto m = run_debug("pingpong_b", N);
    check_coverage(m, N, "pingpong_b");
    check_all_ok(m, "pingpong_b");
}

TEST_F(PingpongBTest, DataIntegrity_burst) {
    auto m = run_debug("pingpong_b", N, "-blength 0.001");
    check_coverage(m, N, "pingpong_b burst");
    check_all_ok(m, "pingpong_b burst");
}

/* ── pingpong_pairwise_b (8 ranks) ─────────────────────────────────────────── */

class PingpongPairwiseBTest : public ::testing::Test {
protected:
    static constexpr int N = 8;
};

TEST_F(PingpongPairwiseBTest, Coverage) {
    auto m = run_debug("pingpong_pairwise_b", N);
    check_coverage(m, N, "pingpong_pairwise_b");
}
TEST_F(PingpongPairwiseBTest, Nprocs) {
    auto m = run_debug("pingpong_pairwise_b", N);
    check_nprocs(m, N, "pingpong_pairwise_b");
}
TEST_F(PingpongPairwiseBTest, DataIntegrity) {
    auto m = run_debug("pingpong_pairwise_b", N);
    check_coverage(m, N, "pingpong_pairwise_b");
    check_all_ok(m, "pingpong_pairwise_b");
}
TEST_F(PingpongPairwiseBTest, DataIntegrity_burst) {
    auto m = run_debug("pingpong_pairwise_b", N, "-blength 0.001");
    check_coverage(m, N, "pingpong_pairwise_b burst");
    check_all_ok(m, "pingpong_pairwise_b burst");
}
