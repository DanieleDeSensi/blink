/*
 * test_stencil.cpp — correctness tests for stencil_2d_nb.
 *
 * Strategy: spawn stencil_2d_nb with -debug and parse
 * "DEBUG rank=X nprocs=Y north=N south=S west=W east=E check=OK|FAIL" lines.
 * Absent neighbours (boundary ranks, non-periodic) are printed as MPI_PROC_NULL
 * which is negative; the test treats any negative value as "no neighbour".
 *
 * Properties checked:
 *   Coverage      — every rank 0..N-1 appears exactly once.
 *   Nprocs        — every rank reports the correct communicator size.
 *   DataIntegrity — every rank reports check=OK.
 *                   For each present neighbour d, recv_buf[d*msg_size..]
 *                   must equal neighbour's rank byte throughout.
 *   Consistency   — the neighbour graph is symmetric:
 *                   if rank A's north = B then rank B's south = A, etc.
 *
 * Requires 8 MPI ranks (auto grid via MPI_Dims_create).
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

static void check_consistency(const DebugMap& m, const std::string& ctx)
{
    for (const auto& [r, fields] : m) {
        int north = get_int(m, r, "north");
        int south = get_int(m, r, "south");
        int west  = get_int(m, r, "west");
        int east  = get_int(m, r, "east");

        if (north >= 0) {
            ASSERT_TRUE(m.count(north)) << ctx << ": rank " << north << " missing";
            int n_south = get_int(m, north, "south");
            EXPECT_EQ(n_south, r)
                << ctx << ": rank " << r << " north=" << north
                << " but rank " << north << " south=" << n_south;
        }
        if (south >= 0) {
            ASSERT_TRUE(m.count(south)) << ctx << ": rank " << south << " missing";
            int s_north = get_int(m, south, "north");
            EXPECT_EQ(s_north, r)
                << ctx << ": rank " << r << " south=" << south
                << " but rank " << south << " north=" << s_north;
        }
        if (west >= 0) {
            ASSERT_TRUE(m.count(west)) << ctx << ": rank " << west << " missing";
            int w_east = get_int(m, west, "east");
            EXPECT_EQ(w_east, r)
                << ctx << ": rank " << r << " west=" << west
                << " but rank " << west << " east=" << w_east;
        }
        if (east >= 0) {
            ASSERT_TRUE(m.count(east)) << ctx << ": rank " << east << " missing";
            int e_west = get_int(m, east, "west");
            EXPECT_EQ(e_west, r)
                << ctx << ": rank " << r << " east=" << east
                << " but rank " << east << " west=" << e_west;
        }
    }
}

/* ── tests ──────────────────────────────────────────────────────────────────── */

class StencilTest : public ::testing::Test {
protected:
    static constexpr int N = 8;
};

TEST_F(StencilTest, Coverage) {
    auto m = run_debug("stencil_2d_nb", N);
    check_coverage(m, N, "stencil default");
}
TEST_F(StencilTest, Nprocs) {
    auto m = run_debug("stencil_2d_nb", N);
    check_nprocs(m, N, "stencil default");
}
TEST_F(StencilTest, DataIntegrity) {
    auto m = run_debug("stencil_2d_nb", N);
    check_coverage(m, N, "stencil default");
    check_all_ok(m, "stencil default");
}
TEST_F(StencilTest, Consistency) {
    auto m = run_debug("stencil_2d_nb", N);
    check_coverage(m, N, "stencil default");
    check_consistency(m, "stencil default");
}
TEST_F(StencilTest, DataIntegrity_periodic) {
    auto m = run_debug("stencil_2d_nb", N, "-periodic");
    check_coverage(m, N, "stencil periodic");
    check_all_ok(m, "stencil periodic");
}
TEST_F(StencilTest, Consistency_periodic) {
    auto m = run_debug("stencil_2d_nb", N, "-periodic");
    check_coverage(m, N, "stencil periodic");
    check_consistency(m, "stencil periodic");
}
TEST_F(StencilTest, DataIntegrity_burst) {
    auto m = run_debug("stencil_2d_nb", N, "-blength 0.001");
    check_coverage(m, N, "stencil burst");
    check_all_ok(m, "stencil burst");
}
TEST_F(StencilTest, Consistency_burst) {
    auto m = run_debug("stencil_2d_nb", N, "-blength 0.001");
    check_coverage(m, N, "stencil burst");
    check_consistency(m, "stencil burst");
}
/*
 * Periodic vs non-periodic distinction: non-periodic must produce at least
 * one boundary rank with an MPI_PROC_NULL (negative) neighbour, and periodic
 * must produce none.  A silently-ignored -periodic flag would fail this.
 */
TEST_F(StencilTest, PeriodicityDistinct) {
    auto plain    = run_debug("stencil_2d_nb", N);
    auto periodic = run_debug("stencil_2d_nb", N, "-periodic");

    int plain_nulls = 0, periodic_nulls = 0;
    for (const auto& [r, fields] : plain) {
        for (const char *d : {"north","south","west","east"})
            if (get_int(plain, r, d) < 0) plain_nulls++;
    }
    for (const auto& [r, fields] : periodic) {
        for (const char *d : {"north","south","west","east"})
            if (get_int(periodic, r, d) < 0) periodic_nulls++;
    }
    EXPECT_GT(plain_nulls, 0)
        << "non-periodic stencil should have at least one boundary neighbour=MPI_PROC_NULL";
    EXPECT_EQ(periodic_nulls, 0)
        << "periodic stencil should have no MPI_PROC_NULL neighbours (toroidal grid), "
        << "got " << periodic_nulls << " — is -periodic silently ignored?";
}
