/*
 * test_kpartners.cpp — correctness tests for kpartners_nb.
 *
 * Strategy: spawn kpartners_nb with -debug and parse
 * "DEBUG rank=X nprocs=Y k=K check=OK|FAIL" lines.
 *
 * Properties checked:
 *   Coverage      — every rank 0..N-1 appears exactly once.
 *   Nprocs        — every rank reports the correct communicator size.
 *   K             — every rank reports the expected number of partners.
 *   DataIntegrity — every rank reports check=OK.  In the new schema this is
 *                   a meaningful check that partner identity was honoured.
 *
 * Requires 8 MPI ranks.
 */
#include <gtest/gtest.h>
#include <string>
#include "popen_helpers.h"

using blink::DebugMap;
using blink::run_debug;
using blink::get_int;
using blink::check_all_ok;
using blink::check_coverage;
using blink::check_nprocs;

/* ── helpers ────────────────────────────────────────────────────────────────── */

static void check_k(const DebugMap& m, int expected_k, const std::string& ctx)
{
    for (const auto& [r, fields] : m) {
        int k = get_int(m, r, "k");
        EXPECT_EQ(k, expected_k)
            << ctx << ": rank " << r << " reports k=" << k;
    }
}

/* ── tests ──────────────────────────────────────────────────────────────────── */

class KpartnersTest : public ::testing::Test {
protected:
    static constexpr int N = 8;
};

TEST_F(KpartnersTest, Coverage) {
    auto m = run_debug("kpartners_nb", N);
    check_coverage(m, N, "k=1");
}
TEST_F(KpartnersTest, Nprocs) {
    auto m = run_debug("kpartners_nb", N);
    check_nprocs(m, N, "k=1");
}
TEST_F(KpartnersTest, K_default) {
    auto m = run_debug("kpartners_nb", N);
    check_k(m, 1, "k=1");
}
TEST_F(KpartnersTest, DataIntegrity_k1) {
    auto m = run_debug("kpartners_nb", N);
    check_coverage(m, N, "k=1");
    check_all_ok(m, "k=1");
}
TEST_F(KpartnersTest, DataIntegrity_k3) {
    auto m = run_debug("kpartners_nb", N, "-k 3");
    check_coverage(m, N, "k=3");
    check_k(m, 3, "k=3");
    check_all_ok(m, "k=3");
}
TEST_F(KpartnersTest, DataIntegrity_burst) {
    auto m = run_debug("kpartners_nb", N, "-blength 0.001");
    check_coverage(m, N, "k=1 burst");
    check_all_ok(m, "k=1 burst");
}
