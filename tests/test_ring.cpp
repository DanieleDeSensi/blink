/*
 * test_ring.cpp — correctness tests for ring benchmarks.
 *
 * Strategy: spawn the actual benchmark binary with -debug and parse
 * "DEBUG rank=X nprocs=N left=L right=R check=OK|FAIL" lines.
 * No MPI in this binary.
 *
 * Properties checked:
 *   Coverage      — every rank 0..N-1 appears exactly once.
 *   Validity      — left and right are valid ranks in [0, N-1].
 *   Exact values  — sequential ring: rank r has left=(r-1+N)%N, right=(r+1)%N.
 *   Consistency   — if rank A says right=B then rank B must say left=A,
 *                   and vice-versa (the neighbour relation is coherent).
 *   Completeness  — every rank appears as someone's left exactly once AND
 *                   as someone's right exactly once (proper ring, no orphans).
 *   DataIntegrity — every rank reports check=OK.
 *
 * Requires 8 MPI ranks.
 */
#include <gtest/gtest.h>
#include <map>
#include <set>
#include <string>
#include "popen_helpers.h"

using blink::DebugMap;
using blink::run_debug;
using blink::get_int;
using blink::check_all_ok;
using blink::check_coverage;

/* ── shared assertion helpers ───────────────────────────────────────────────── */

static void check_validity(const DebugMap& m, int n, const std::string& ctx)
{
    for (const auto& [r, fields] : m) {
        int l = get_int(m, r, "left");
        int rr = get_int(m, r, "right");
        EXPECT_GE(l,  0) << ctx << ": rank " << r << " left=" << l;
        EXPECT_LT(l,  n) << ctx << ": rank " << r << " left=" << l;
        EXPECT_GE(rr, 0) << ctx << ": rank " << r << " right=" << rr;
        EXPECT_LT(rr, n) << ctx << ": rank " << r << " right=" << rr;
    }
}

/* If A.right = B then B.left must equal A (and A.left = C implies C.right = A). */
static void check_consistency(const DebugMap& m, const std::string& ctx)
{
    for (const auto& [r, fields] : m) {
        int l  = get_int(m, r, "left");
        int rr = get_int(m, r, "right");

        /* check right edge: r → rr */
        auto it = m.find(rr);
        if (it != m.end()) {
            int neighbour_left = get_int(m, rr, "left");
            EXPECT_EQ(neighbour_left, r)
                << ctx << ": rank " << r << " says right=" << rr
                << " but rank " << rr << " says left=" << neighbour_left;
        }

        /* check left edge: r → l */
        it = m.find(l);
        if (it != m.end()) {
            int neighbour_right = get_int(m, l, "right");
            EXPECT_EQ(neighbour_right, r)
                << ctx << ": rank " << r << " says left=" << l
                << " but rank " << l << " says right=" << neighbour_right;
        }
    }
}

/* Every rank appears as someone's left exactly once AND right exactly once. */
static void check_completeness(const DebugMap& m, const std::string& ctx)
{
    std::map<int,int> as_left, as_right;
    for (const auto& [r, fields] : m) {
        as_left[get_int(m, r, "left")]++;
        as_right[get_int(m, r, "right")]++;
    }
    for (const auto& [r, _] : m) {
        EXPECT_EQ(as_left.count(r)  ? as_left[r]  : 0, 1)
            << ctx << ": rank " << r << " appears as left neighbour "
            << (as_left.count(r) ? as_left[r] : 0) << " time(s) (expected 1)";
        EXPECT_EQ(as_right.count(r) ? as_right[r] : 0, 1)
            << ctx << ": rank " << r << " appears as right neighbour "
            << (as_right.count(r) ? as_right[r] : 0) << " time(s) (expected 1)";
    }
}

static void check_exact_sequential(const DebugMap& m, int n, const std::string& ctx)
{
    for (const auto& [r, fields] : m) {
        int l  = get_int(m, r, "left");
        int rr = get_int(m, r, "right");
        EXPECT_EQ(l,  (r - 1 + n) % n)
            << ctx << ": rank " << r << " expected left=" << (r-1+n)%n
            << " got " << l;
        EXPECT_EQ(rr, (r + 1) % n)
            << ctx << ": rank " << r << " expected right=" << (r+1)%n
            << " got " << rr;
    }
}

/* ── ring_nb ────────────────────────────────────────────────────────────────── */

class RingNbTest : public ::testing::Test {
protected:
    static constexpr int N = 8;
};

TEST_F(RingNbTest, Sequential_Coverage) {
    auto m = run_debug("ring_nb", N);
    check_coverage(m, N, "ring_nb sequential");
    check_all_ok(m, "ring_nb sequential");
}
TEST_F(RingNbTest, Sequential_Validity) {
    auto m = run_debug("ring_nb", N);
    check_validity(m, N, "ring_nb sequential");
}
TEST_F(RingNbTest, Sequential_ExactNeighbors) {
    auto m = run_debug("ring_nb", N);
    check_exact_sequential(m, N, "ring_nb sequential");
}
TEST_F(RingNbTest, Sequential_Consistency) {
    auto m = run_debug("ring_nb", N);
    check_consistency(m, "ring_nb sequential");
}
TEST_F(RingNbTest, Sequential_Completeness) {
    auto m = run_debug("ring_nb", N);
    check_completeness(m, "ring_nb sequential");
}

TEST_F(RingNbTest, Random_Coverage) {
    auto m = run_debug("ring_nb", N, "-rring -seed 1");
    check_coverage(m, N, "ring_nb random");
    check_all_ok(m, "ring_nb random");
}
TEST_F(RingNbTest, Random_Validity) {
    auto m = run_debug("ring_nb", N, "-rring -seed 1");
    check_validity(m, N, "ring_nb random");
}
/* The random ring is built from an explicit position<->rank inverse map
 * (positions[targets[i]] = i in ring_nb.c), so the neighbour relation is fully
 * reciprocal (A.right=B  <=>  B.left=A) — the same consistency property the
 * sequential ring has.  Both consistency and completeness must hold.        */
TEST_F(RingNbTest, Random_Consistency) {
    auto m = run_debug("ring_nb", N, "-rring -seed 1");
    check_consistency(m, "ring_nb random");
}
TEST_F(RingNbTest, Random_Completeness) {
    auto m = run_debug("ring_nb", N, "-rring -seed 1");
    check_completeness(m, "ring_nb random");
}
TEST_F(RingNbTest, Burst) {
    auto m = run_debug("ring_nb", N, "-blength 0.001");
    check_coverage(m, N, "ring_nb burst");
    check_consistency(m, "ring_nb burst");
    check_all_ok(m, "ring_nb burst");
}
/* Random ring must differ from sequential (sanity: permutation actually happened). */
TEST_F(RingNbTest, Random_DiffersFromSequential) {
    auto seq = run_debug("ring_nb", N);
    auto rnd = run_debug("ring_nb", N, "-rring -seed 1");
    bool any_diff = false;
    for (const auto& [r, fields] : seq) {
        if (!rnd.count(r)) continue;
        if (get_int(rnd, r, "left")  != get_int(seq, r, "left") ||
            get_int(rnd, r, "right") != get_int(seq, r, "right"))
            any_diff = true;
    }
    EXPECT_TRUE(any_diff) << "random ring produced identical topology to sequential";
}

/* ── ring_bsnbr — same setup code, different MPI calls ─────────────────────── */

class RingBsnbrTest : public ::testing::Test {
protected:
    static constexpr int N = 8;
};

TEST_F(RingBsnbrTest, Sequential_ExactNeighbors) {
    auto m = run_debug("ring_bsnbr", N);
    check_exact_sequential(m, N, "ring_bsnbr sequential");
    check_all_ok(m, "ring_bsnbr sequential");
}
TEST_F(RingBsnbrTest, Sequential_Consistency) {
    auto m = run_debug("ring_bsnbr", N);
    check_consistency(m, "ring_bsnbr sequential");
}
TEST_F(RingBsnbrTest, Sequential_Completeness) {
    auto m = run_debug("ring_bsnbr", N);
    check_completeness(m, "ring_bsnbr sequential");
}
TEST_F(RingBsnbrTest, Random_Consistency) {
    auto m = run_debug("ring_bsnbr", N, "-rring -seed 1");
    check_consistency(m, "ring_bsnbr random");
}
TEST_F(RingBsnbrTest, Random_Completeness) {
    auto m = run_debug("ring_bsnbr", N, "-rring -seed 1");
    check_completeness(m, "ring_bsnbr random");
    check_all_ok(m, "ring_bsnbr random");
}
/* Both variants must agree on the same topology for the same seed. */
TEST_F(RingBsnbrTest, AgreesWith_RingNb_Sequential) {
    auto nb    = run_debug("ring_nb",    N);
    auto bsnbr = run_debug("ring_bsnbr", N);
    for (const auto& [r, fields] : nb) {
        if (!bsnbr.count(r)) continue;
        EXPECT_EQ(get_int(bsnbr, r, "left"),  get_int(nb, r, "left"))
            << "rank " << r << " left differs between ring_nb and ring_bsnbr";
        EXPECT_EQ(get_int(bsnbr, r, "right"), get_int(nb, r, "right"))
            << "rank " << r << " right differs between ring_nb and ring_bsnbr";
    }
}
TEST_F(RingBsnbrTest, Burst) {
    auto m = run_debug("ring_bsnbr", N, "-blength 0.001");
    check_coverage(m, N, "ring_bsnbr burst");
    check_consistency(m, "ring_bsnbr burst");
    check_all_ok(m, "ring_bsnbr burst");
}
TEST_F(RingBsnbrTest, AgreesWith_RingNb_Random) {
    auto nb    = run_debug("ring_nb",    N, "-rring -seed 42");
    auto bsnbr = run_debug("ring_bsnbr", N, "-rring -seed 42");
    for (const auto& [r, fields] : nb) {
        if (!bsnbr.count(r)) continue;
        EXPECT_EQ(get_int(bsnbr, r, "left"),  get_int(nb, r, "left"))
            << "rank " << r << " left differs (random, seed=42)";
        EXPECT_EQ(get_int(bsnbr, r, "right"), get_int(nb, r, "right"))
            << "rank " << r << " right differs (random, seed=42)";
    }
}
