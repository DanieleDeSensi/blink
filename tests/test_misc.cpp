/*
 * test_misc.cpp — miscellaneous correctness tests:
 *   - ring-buffer wrap (iter > maxsamples)
 *   - pretty-print formatter doesn't crash and produces a recognisable table
 *   - -mrand picks a deterministic master rank for a given seed
 *   - sampler/dist_test runs (validates the burst distribution implementation)
 *
 * All tests use 8 ranks except where noted.
 */
#include <gtest/gtest.h>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "popen_helpers.h"

using blink::run_debug_capture;
using blink::DebugRun;

/* Helper: count lines in s containing a substring */
static int count_lines_containing(const std::string& s, const std::string& needle)
{
    int n = 0;
    size_t p = 0;
    while ((p = s.find(needle, p)) != std::string::npos) { n++; p += needle.size(); }
    return n;
}

/* ── ring-buffer wrap ───────────────────────────────────────────────────────── */

/*
 * Configure max_samples to be smaller than the iteration count, ensuring the
 * ring buffer wraps.  The benchmark should still print exactly max_samples
 * "Average,..." data lines (one per recorded sample), and the summary line
 * should report "Measured <max_samples>" iterations.
 */
TEST(RingBufferWrap, BarrierWrapsCorrectly)
{
    /* run barrier_nb (cheap) with iter=50 maxsamples=10 warmup=0 — buffer
     * should hold the last 10 samples.                                    */
    blink::DebugRun r = run_debug_capture("barrier_nb", 8,
        "-iter 50 -maxsamples 10 -warmup 0");
    /* run_debug_capture does NOT assert; do the assertions explicitly */
    ASSERT_TRUE(r.normal_exit) << "stderr:\n" << r.stderr_raw;
    ASSERT_EQ(r.exit_code, 0)  << "stderr:\n" << r.stderr_raw;

    /* the master rank's CSV body has lines of the form "%.9f,%.9f,..." */
    int data_lines = 0;
    size_t p = 0;
    while ((p = r.stdout_raw.find('\n', p)) != std::string::npos) {
        /* heuristic: a data line starts with a digit and has 4 commas */
        size_t line_start = r.stdout_raw.rfind('\n', p > 0 ? p - 1 : 0);
        size_t s = (line_start == std::string::npos) ? 0 : line_start + 1;
        std::string line = r.stdout_raw.substr(s, p - s);
        if (!line.empty() && isdigit((unsigned char)line[0]))
            if (std::count(line.begin(), line.end(), ',') == 4)
                data_lines++;
        p++;
    }
    EXPECT_EQ(data_lines, 10)
        << "ring buffer wrap: expected 10 measured samples (maxsamples), got "
        << data_lines << "\nstdout:\n" << r.stdout_raw;

    EXPECT_NE(r.stdout_raw.find("Ran 50 iterations. Measured 10 iterations."),
              std::string::npos)
        << "summary line mismatch.\nstdout:\n" << r.stdout_raw;
}

/* ── pretty-print formatter ─────────────────────────────────────────────────── */

TEST(PrettyPrint, FormatterDoesNotCrash)
{
    DebugRun r = run_debug_capture("barrier_nb", 8, "-iter 3 -pretty-print");
    ASSERT_TRUE(r.normal_exit) << "stderr:\n" << r.stderr_raw;
    ASSERT_EQ(r.exit_code, 0);
    /* pretty-print uses a column-header row and a separator line */
    EXPECT_NE(r.stdout_raw.find("avg"),    std::string::npos);
    EXPECT_NE(r.stdout_raw.find("median"), std::string::npos);
    EXPECT_NE(r.stdout_raw.find("samples"),std::string::npos);
}

/* ── -mrand determinism ─────────────────────────────────────────────────────── */

TEST(MasterRand, SameSeedSameMaster)
{
    /* with -mrand and a fixed seed, the chosen master_rank should be
     * deterministic across runs.  We grep the "receiver rank: R" line from
     * incast_b's own startup banner.                                       */
    DebugRun r1 = run_debug_capture("incast_b", 8, "-mrand -seed 42");
    DebugRun r2 = run_debug_capture("incast_b", 8, "-mrand -seed 42");
    ASSERT_TRUE(r1.normal_exit);
    ASSERT_TRUE(r2.normal_exit);
    auto p1 = r1.stdout_raw.find("receiver rank:");
    auto p2 = r2.stdout_raw.find("receiver rank:");
    ASSERT_NE(p1, std::string::npos) << r1.stdout_raw;
    ASSERT_NE(p2, std::string::npos) << r2.stdout_raw;
    int m1, m2;
    sscanf(r1.stdout_raw.c_str() + p1, "receiver rank: %d", &m1);
    sscanf(r2.stdout_raw.c_str() + p2, "receiver rank: %d", &m2);
    EXPECT_EQ(m1, m2)
        << "same -seed should give same -mrand master, got " << m1 << " vs " << m2;
}

/* ── dist_test sampler validation ───────────────────────────────────────────── */

TEST(SamplerValidation, DistTestRuns)
{
    /* dist_test is single-rank.  If it exits 0 the sampler implementations
     * are internally consistent (mean / KS test).                          */
    DebugRun r = run_debug_capture("dist_test", 1, "");
    /* dist_test may not accept -debug; treat absence of failure as success */
    ASSERT_EQ(r.exit_code, 0)
        << "dist_test failed.\nstderr:\n" << r.stderr_raw
        << "\nstdout:\n" << r.stdout_raw;
}

/* ── warm-up exclusion ──────────────────────────────────────────────────────── */

/*
 * Warm-up iterations must NOT be recorded.  With -iter 5 -warmup 3 the benchmark
 * runs 8 outer iterations but records only the last 5; the summary must report
 * "Ran 8 iterations. Measured 5 iterations." and emit exactly 5 CSV data lines.
 */
TEST(WarmupExclusion, WarmupSamplesNotRecorded)
{
    DebugRun r = run_debug_capture("barrier_nb", 8, "-iter 5 -warmup 3 -maxsamples 1000");
    ASSERT_TRUE(r.normal_exit) << "stderr:\n" << r.stderr_raw;
    ASSERT_EQ(r.exit_code, 0)  << "stderr:\n" << r.stderr_raw;

    int data_lines = 0;
    size_t p = 0;
    while ((p = r.stdout_raw.find('\n', p)) != std::string::npos) {
        size_t line_start = r.stdout_raw.rfind('\n', p > 0 ? p - 1 : 0);
        size_t s = (line_start == std::string::npos) ? 0 : line_start + 1;
        std::string line = r.stdout_raw.substr(s, p - s);
        if (!line.empty() && isdigit((unsigned char)line[0]))
            if (std::count(line.begin(), line.end(), ',') == 4)
                data_lines++;
        p++;
    }
    EXPECT_EQ(data_lines, 5)
        << "expected 5 recorded samples (3 warmup excluded), got " << data_lines
        << "\nstdout:\n" << r.stdout_raw;
    EXPECT_NE(r.stdout_raw.find("Ran 8 iterations. Measured 5 iterations."),
              std::string::npos)
        << "summary should report 8 run / 5 measured.\nstdout:\n" << r.stdout_raw;
}

/* ── SIGUSR1 clean shutdown ─────────────────────────────────────────────────── */

/*
 * Regression test for the endless-mode SIGUSR1 shutdown.  The shutdown flag is
 * set only on the rank that receives the signal, so all ranks must agree
 * collectively (check_shutdown) to leave the measurement loop on the same
 * iteration.  Sending SIGUSR1 to exactly ONE rank of an endless run must shut
 * the whole job down cleanly (no deadlock) and still print collected results.
 *
 * Orchestrated in POSIX sh via popen: launch the endless run in the background,
 * signal one rank (a process whose comm is exactly "alltoall_nb" — i.e. a rank,
 * not the launcher), then wait up to 30s for a clean exit, killing it if it
 * hangs so the test itself can never block indefinitely.
 */
TEST(Shutdown, Sigusr1ToSingleRankShutsDownCleanly)
{
    char outtmpl[] = "/tmp/blink_endl_XXXXXX";
    int ofd = mkstemp(outtmpl);
    ASSERT_GE(ofd, 0);
    close(ofd);
    std::string outfile = outtmpl;
    std::string bin = blink::bin_dir() + "/alltoall_nb";

    std::string cmd =
        "\"" + blink::mpiexec() + "\" " + blink::nproc_flag() + " 4 \"" + bin + "\""
        " -endl -msgsize 1024 -seed 999983 > \"" + outfile + "\" 2>&1 &\n"
        "MPIPID=$!\n"
        "sleep 3\n"
        "RP=\"\"\n"
        "for q in $(pgrep -f 'alltoall_nb -endl'); do\n"
        "  if [ -r /proc/$q/comm ] && [ \"$(cat /proc/$q/comm)\" = alltoall_nb ]; then RP=$q; break; fi\n"
        "done\n"
        "[ -n \"$RP\" ] && kill -USR1 \"$RP\"\n"
        "CLEAN=HANG\n"
        "for i in $(seq 1 30); do kill -0 \"$MPIPID\" 2>/dev/null || { CLEAN=CLEAN; break; }; sleep 1; done\n"
        "if [ \"$CLEAN\" = HANG ]; then kill -9 \"$MPIPID\" 2>/dev/null; pkill -9 -f 'alltoall_nb -endl' 2>/dev/null; fi\n"
        "echo SIGNALED=$RP\n"
        "echo RESULT=$CLEAN\n";

    FILE *pp = popen(cmd.c_str(), "r");
    ASSERT_NE(pp, nullptr);
    std::string out;
    char buf[512];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), pp)) > 0) out.append(buf, n);
    pclose(pp);

    std::string bench;
    FILE *of = fopen(outfile.c_str(), "r");
    if (of) { while ((n = fread(buf, 1, sizeof(buf), of)) > 0) bench.append(buf, n); fclose(of); }
    unlink(outfile.c_str());

    EXPECT_EQ(out.find("SIGNALED=\n"), std::string::npos)
        << "no rank process was found to signal.\norchestration:\n" << out;
    EXPECT_NE(out.find("RESULT=CLEAN"), std::string::npos)
        << "endless job did not shut down after SIGUSR1 to one rank (deadlock?).\n"
        << "orchestration:\n" << out << "\nbenchmark output:\n" << bench;
    EXPECT_NE(bench.find("Measured"), std::string::npos)
        << "no results footer after clean shutdown.\nbenchmark output:\n" << bench;
}
