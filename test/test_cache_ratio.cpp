// Tests for the node-table-to-cache ratio (bddcache_setratio) and the
// automatic resizing of the operation cache when the node table grows.
#include <gtest/gtest.h>
#include <stdexcept>
#include "bdd.h"

using namespace kyotodd;

namespace {

// Create @p n fresh variables and one node per variable so that the node
// table is forced through node_array_grow(). Returns the created nodes so
// the caller keeps them alive (and reachable) for the duration of the test.
std::vector<BDD> make_nodes(int n) {
    std::vector<BDD> keep;
    keep.reserve(n);
    for (int i = 0; i < n; i++) {
        bddvar v = bddnewvar();
        keep.push_back(BDD_ID(bddprime(v)));
    }
    return keep;
}

// Parity of n variables: with complement edges the BDD has exactly n
// nodes and 2^(n-1) satisfying assignments. Building it allocates nodes
// steadily, so with a small initial table the cache is resized several
// times mid-operation.
BDD parity(int n) {
    BDD f = BDD::False;
    for (int i = 0; i < n; i++) {
        bddvar v = bddnewvar();
        f = f ^ BDD_ID(bddprime(v));
    }
    return f;
}

}  // namespace

class CacheRatioTest : public ::testing::Test {
protected:
    void TearDown() override {
        bddfinal();
    }
};

TEST_F(CacheRatioTest, DefaultRatioIsOne) {
    bddinit(1000, UINT64_MAX);
    EXPECT_EQ(bddcache_getratio(), 1);
    // One entry per node slot, rounded up to a power of 2.
    EXPECT_EQ(bddcache_size(), UINT64_C(1024));
}

TEST_F(CacheRatioTest, CacheSizeIsZeroBeforeInit) {
    bddfinal();
    EXPECT_EQ(bddcache_size(), UINT64_C(0));
}

TEST_F(CacheRatioTest, CacheGrowsWithNodeTable) {
    bddinit(256, UINT64_MAX);
    ASSERT_EQ(bddcache_size(), UINT64_C(256));
    std::vector<BDD> keep = make_nodes(300);  // > 256 nodes -> one doubling
    EXPECT_GE(bdd_node_count, UINT64_C(300));
    EXPECT_EQ(bddcache_size(), bdd_node_count);  // ratio 1
    EXPECT_EQ(bddcache_size(), UINT64_C(512));
}

TEST_F(CacheRatioTest, CacheGrowsThroughSeveralDoublings) {
    bddinit(256, UINT64_MAX);
    std::vector<BDD> keep = make_nodes(3000);  // 256 -> 512 -> ... -> 4096
    EXPECT_EQ(bdd_node_count, UINT64_C(4096));
    EXPECT_EQ(bddcache_size(), UINT64_C(4096));
}

TEST_F(CacheRatioTest, RatioZeroFreezesCache) {
    bddinit(256, UINT64_MAX);
    EXPECT_EQ(bddcache_setratio(0), 1);  // returns the previous ratio
    EXPECT_EQ(bddcache_getratio(), 0);
    std::vector<BDD> keep = make_nodes(3000);
    EXPECT_EQ(bdd_node_count, UINT64_C(4096));
    EXPECT_EQ(bddcache_size(), UINT64_C(256));  // unchanged
}

TEST_F(CacheRatioTest, SetRatioResizesImmediately) {
    bddinit(1024, UINT64_MAX);
    ASSERT_EQ(bddcache_size(), UINT64_C(1024));
    EXPECT_EQ(bddcache_setratio(4), 1);
    EXPECT_EQ(bddcache_getratio(), 4);
    EXPECT_EQ(bddcache_size(), UINT64_C(256));  // 1024 / 4
    // Growth keeps following the new ratio.
    std::vector<BDD> keep = make_nodes(1500);  // 1024 -> 2048
    EXPECT_EQ(bdd_node_count, UINT64_C(2048));
    EXPECT_EQ(bddcache_size(), UINT64_C(512));  // 2048 / 4
    // Back to ratio 1 grows the cache right away.
    EXPECT_EQ(bddcache_setratio(1), 4);
    EXPECT_EQ(bddcache_size(), UINT64_C(2048));
}

TEST_F(CacheRatioTest, RatioRoundsUpToPowerOfTwo) {
    bddinit(1000, UINT64_MAX);
    bddcache_setratio(3);  // 1000 / 3 = 333 -> 512
    EXPECT_EQ(bddcache_size(), UINT64_C(512));
}

TEST_F(CacheRatioTest, NegativeRatioThrows) {
    bddinit(256, UINT64_MAX);
    EXPECT_THROW(bddcache_setratio(-1), std::invalid_argument);
    EXPECT_EQ(bddcache_getratio(), 1);  // unchanged
}

TEST_F(CacheRatioTest, InitAndFinalResetRatio) {
    bddinit(256, UINT64_MAX);
    bddcache_setratio(8);
    bddinit(256, UINT64_MAX);  // re-init goes through bddfinal
    EXPECT_EQ(bddcache_getratio(), 1);
    EXPECT_EQ(bddcache_size(), UINT64_C(256));
}

TEST_F(CacheRatioTest, GrowthRehashesExistingEntries) {
    bddinit(256, UINT64_MAX);
    // Terminal operands keep the keys valid regardless of node allocation.
    const uint8_t op2 = BDD_OP_USER_MIN;
    const uint8_t op3 = BDD_OP_USER_MIN + 1;
    bddwcache(op2, bddtrue, bddfalse, bddtrue);
    bddwcache(op2, bddfalse, bddtrue, bddfalse);
    bddwcache3(op3, bddtrue, bddfalse, bddtrue, bddfalse);
    ASSERT_EQ(bddrcache(op2, bddtrue, bddfalse), bddtrue);

    std::vector<BDD> keep = make_nodes(3000);  // several doublings
    ASSERT_EQ(bddcache_size(), UINT64_C(4096));

    // Growing never drops entries: distinct slots in the small table map
    // to distinct slots in the larger one.
    EXPECT_EQ(bddrcache(op2, bddtrue, bddfalse), bddtrue);
    EXPECT_EQ(bddrcache(op2, bddfalse, bddtrue), bddfalse);
    EXPECT_EQ(bddrcache3(op3, bddtrue, bddfalse, bddtrue), bddfalse);
    // A 2-operand key must not be confused with the 3-operand entry.
    EXPECT_EQ(bddrcache(op3, bddtrue, bddfalse), bddnull);
}

TEST_F(CacheRatioTest, ShrinkKeepsEntryWhenSlotIsFree) {
    bddinit(4096, UINT64_MAX);
    const uint8_t op = BDD_OP_USER_MIN;
    bddwcache(op, bddtrue, bddfalse, bddtrue);
    bddcache_setratio(16);  // 4096 -> 256: a single entry always survives
    ASSERT_EQ(bddcache_size(), UINT64_C(256));
    EXPECT_EQ(bddrcache(op, bddtrue, bddfalse), bddtrue);
}

TEST_F(CacheRatioTest, ResultsStayCorrectAcrossMidOperationResize) {
    bddinit(16, UINT64_MAX);  // tiny table: resizes happen inside operators
    {
        BDD f = parity(60);
        EXPECT_GT(bdd_node_count, UINT64_C(16));
        EXPECT_EQ(bddcache_size(), bdd_node_count);
        EXPECT_DOUBLE_EQ(f.count(60), 576460752303423488.0);  // 2^59
        EXPECT_EQ(f.raw_size(), UINT64_C(60));
    }
    // Rebuild from scratch with a frozen cache and compare the results.
    bddfinal();
    bddinit(16, UINT64_MAX);
    bddcache_setratio(0);
    BDD g = parity(60);
    EXPECT_EQ(bddcache_size(), UINT64_C(16));
    EXPECT_DOUBLE_EQ(g.count(60), 576460752303423488.0);
    EXPECT_EQ(g.raw_size(), UINT64_C(60));
    // Same session, same variables: the fully cached rebuild must reach the
    // same canonical node.
    bddcache_setratio(1);
    BDD h = BDD::False;
    for (bddvar v = 1; v <= 60; v++) h = h ^ BDD_ID(bddprime(v));
    EXPECT_EQ(g, h);
}

TEST_F(CacheRatioTest, GcClearsCacheButKeepsSize) {
    bddinit(256, UINT64_MAX);
    std::vector<BDD> keep = make_nodes(300);
    ASSERT_EQ(bddcache_size(), UINT64_C(512));
    const uint8_t op = BDD_OP_USER_MIN;
    bddwcache(op, bddtrue, bddfalse, bddtrue);
    bddgc();
    EXPECT_EQ(bddcache_size(), UINT64_C(512));
    EXPECT_EQ(bddrcache(op, bddtrue, bddfalse), bddnull);
}
