#include "mlzbddv.h"
#include <gtest/gtest.h>

class MLZBDDVTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDDV_Init(256, 256);
    }
};

// Helper: compute pin from a ZBDDV (same as auto-detect constructor)
static int compute_pin(const ZBDDV& zv) {
    int top = zv.Top();
    return (top > 0) ? static_cast<int>(BDD_LevOfVar(
                            static_cast<bddvar>(top)))
                     : 0;
}

// ============================================================
// Default constructor
// ============================================================

TEST_F(MLZBDDVTest, DefaultConstructor) {
    MLZBDDV ml;
    EXPECT_EQ(ml.N_pin(), 0);
    EXPECT_EQ(ml.N_out(), 0);
    EXPECT_EQ(ml.N_sin(), 0);
}

// ============================================================
// Empty ZBDDV
// ============================================================

TEST_F(MLZBDDVTest, EmptyZBDDV) {
    ZBDDV zv;
    MLZBDDV ml(zv, 0, 0);
    EXPECT_EQ(ml.N_pin(), 0);
    EXPECT_EQ(ml.N_out(), 0);
    EXPECT_EQ(ml.N_sin(), 0);
}

// ============================================================
// Single output, no factorization possible
// ============================================================

TEST_F(MLZBDDVTest, SingleOutputNoFactor) {
    bddvar v1 = static_cast<bddvar>(BDDV_NewVar());
    ZDD f0 = ZDD::Single.Change(v1);

    ZBDDV zv(f0, 0);
    int pin = compute_pin(zv);
    MLZBDDV ml(zv, pin, 1);

    EXPECT_EQ(ml.N_pin(), pin);
    EXPECT_EQ(ml.N_out(), 1);
    EXPECT_EQ(ml.N_sin(), 0);

    // Output should still represent the same function
    ZDD out0 = ml.GetZBDDV().GetZBDD(0);
    EXPECT_EQ(out0, f0);
}

// ============================================================
// Two identical outputs — phase 1 should extract
// ============================================================

TEST_F(MLZBDDVTest, TwoIdenticalOutputs) {
    bddvar v1 = static_cast<bddvar>(BDDV_NewVar());
    bddvar v2 = static_cast<bddvar>(BDDV_NewVar());

    ZDD f = ZDD::Single.Change(v1).Change(v2);

    ZBDDV zv(f, 0);
    zv += ZBDDV(f, 1);

    int pin = compute_pin(zv);
    MLZBDDV ml(zv, pin, 2);

    EXPECT_EQ(ml.N_pin(), pin);
    EXPECT_EQ(ml.N_out(), 2);
}

// ============================================================
// Auto-detect constructor
// ============================================================

TEST_F(MLZBDDVTest, AutoDetectConstructor) {
    bddvar v1 = static_cast<bddvar>(BDDV_NewVar());
    bddvar v2 = static_cast<bddvar>(BDDV_NewVar());

    ZDD f0 = ZDD::Single.Change(v1);
    ZDD f1 = ZDD::Single.Change(v2);

    ZBDDV zv(f0, 0);
    zv += ZBDDV(f1, 1);

    MLZBDDV ml(zv);

    int expected_pin = static_cast<int>(BDD_LevOfVar(
        static_cast<bddvar>(zv.Top())));
    EXPECT_EQ(ml.N_pin(), expected_pin);
    EXPECT_EQ(ml.N_out(), 2);
}

// ============================================================
// Copy assignment
// ============================================================

TEST_F(MLZBDDVTest, CopyAssignment) {
    ZBDDV zv;
    MLZBDDV ml1(zv, 0, 0);
    MLZBDDV ml2;
    ml2 = ml1;

    EXPECT_EQ(ml2.N_pin(), ml1.N_pin());
    EXPECT_EQ(ml2.N_out(), ml1.N_out());
    EXPECT_EQ(ml2.N_sin(), ml1.N_sin());
}

// ============================================================
// Error value (null ZBDDV)
// ============================================================

TEST_F(MLZBDDVTest, ErrorValue) {
    ZBDDV zv(ZDD(-1));
    MLZBDDV ml(zv, 3, 2);

    EXPECT_EQ(ml.N_pin(), 0);
    EXPECT_EQ(ml.N_out(), 0);
    EXPECT_EQ(ml.N_sin(), 0);
}

// ============================================================
// Phase 1: one output divides another
// ============================================================

TEST_F(MLZBDDVTest, Phase1InterOutputDivision) {
    bddvar v1 = static_cast<bddvar>(BDDV_NewVar());
    bddvar v2 = static_cast<bddvar>(BDDV_NewVar());
    bddvar v3 = static_cast<bddvar>(BDDV_NewVar());

    // f0 = {{v1}} (a factor)
    ZDD f0 = ZDD::Single.Change(v1);

    // f1 = {{v1, v2}, {v1, v3}} = f0 * {{v2}, {v3}}
    ZDD f1 = ZDD::Single.Change(v1).Change(v2)
           + ZDD::Single.Change(v1).Change(v3);

    ZBDDV zv(f0, 0);
    zv += ZBDDV(f1, 1);

    int pin = compute_pin(zv);
    MLZBDDV ml(zv, pin, 2);

    EXPECT_EQ(ml.N_out(), 2);

    // f0 should be unchanged
    ZDD out0 = ml.GetZBDDV().GetZBDD(0);
    EXPECT_EQ(out0, f0);
}

// ============================================================
// Phase 2: kernel extraction from common sub-expressions
// ============================================================

TEST_F(MLZBDDVTest, Phase2KernelExtraction) {
    bddvar v1 = static_cast<bddvar>(BDDV_NewVar());
    bddvar v2 = static_cast<bddvar>(BDDV_NewVar());
    bddvar v3 = static_cast<bddvar>(BDDV_NewVar());

    // f0 = {{v1, v2}, {v1, v3}} — has kernel {{v2}, {v3}}
    ZDD f0 = ZDD::Single.Change(v1).Change(v2)
           + ZDD::Single.Change(v1).Change(v3);

    ZBDDV zv(f0, 0);

    int pin = compute_pin(zv);
    MLZBDDV ml(zv, pin, 1);

    EXPECT_EQ(ml.N_out(), 1);
    // Phase 1 allocates 1, plus phase 2 should extract at least 1 kernel
    EXPECT_GE(ml.N_sin(), 1);
}

// ============================================================
// Constructor does not write to stdout
// ============================================================

TEST_F(MLZBDDVTest, ConstructorNoStdout) {
    bddvar v1 = static_cast<bddvar>(BDDV_NewVar());
    bddvar v2 = static_cast<bddvar>(BDDV_NewVar());
    bddvar v3 = static_cast<bddvar>(BDDV_NewVar());

    // f0 = {{v1, v2}, {v1, v3}} — triggers phase 2 kernel extraction
    ZDD f0 = ZDD::Single.Change(v1).Change(v2)
           + ZDD::Single.Change(v1).Change(v3);

    ZBDDV zv(f0, 0);
    int pin = compute_pin(zv);

    testing::internal::CaptureStdout();
    MLZBDDV ml(zv, pin, 1);
    std::string output = testing::internal::GetCapturedStdout();

    // Constructor must not produce any stdout output
    EXPECT_EQ(output, "");
}

// ============================================================
// N_sin counts phase 2 sub-expression extractions only
// ============================================================

TEST_F(MLZBDDVTest, SinCountsPhase2Only) {
    bddvar v1 = static_cast<bddvar>(BDDV_NewVar());
    bddvar v2 = static_cast<bddvar>(BDDV_NewVar());
    bddvar v3 = static_cast<bddvar>(BDDV_NewVar());
    bddvar v4 = static_cast<bddvar>(BDDV_NewVar());

    // f0 = {{v1, v2}, {v1, v3}}
    ZDD f0 = ZDD::Single.Change(v1).Change(v2)
           + ZDD::Single.Change(v1).Change(v3);

    // f1 = {{v1, v4}, {v1, v3}}
    ZDD f1 = ZDD::Single.Change(v1).Change(v4)
           + ZDD::Single.Change(v1).Change(v3);

    ZBDDV zv(f0, 0);
    zv += ZBDDV(f1, 1);

    int pin = compute_pin(zv);
    MLZBDDV ml(zv, pin, 2);

    EXPECT_EQ(ml.N_out(), 2);
    // Phase 2 should extract kernels from both outputs
    EXPECT_GE(ml.N_sin(), 2);
}

// ============================================================
// GetZBDDV returns valid ZBDDV
// ============================================================

TEST_F(MLZBDDVTest, GetZBDDVReturnsValidResult) {
    bddvar v1 = static_cast<bddvar>(BDDV_NewVar());
    ZDD f0 = ZDD::Single.Change(v1);

    ZBDDV zv(f0, 0);
    int pin = compute_pin(zv);
    MLZBDDV ml(zv, pin, 1);

    ZBDDV result = ml.GetZBDDV();
    ZDD out0 = result.GetZBDD(0);
    EXPECT_NE(out0.GetID(), bddempty);
}

// ============================================================
// Print does not crash
// ============================================================

TEST_F(MLZBDDVTest, PrintDoesNotCrash) {
    bddvar v1 = static_cast<bddvar>(BDDV_NewVar());
    ZDD f0 = ZDD::Single.Change(v1);

    ZBDDV zv(f0, 0);
    int pin = compute_pin(zv);
    MLZBDDV ml(zv, pin, 1);

    testing::internal::CaptureStdout();
    ml.Print();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("pin:"), std::string::npos);
    EXPECT_NE(output.find("out:1"), std::string::npos);
    EXPECT_NE(output.find("sin:"), std::string::npos);
}

// ============================================================
// Constant outputs (no items)
// ============================================================

TEST_F(MLZBDDVTest, ConstantOutputs) {
    ZDD f0 = ZDD::Single;
    ZDD f1(0);

    ZBDDV zv(f0, 0);
    zv += ZBDDV(f1, 1);

    MLZBDDV ml(zv, 0, 2);

    EXPECT_EQ(ml.N_pin(), 0);
    EXPECT_EQ(ml.N_out(), 2);
}

// ============================================================
// Phase 1 with no successful divisions
// ============================================================

TEST_F(MLZBDDVTest, Phase1NoDivision) {
    bddvar v1 = static_cast<bddvar>(BDDV_NewVar());
    bddvar v2 = static_cast<bddvar>(BDDV_NewVar());

    ZDD f0 = ZDD::Single.Change(v1);
    ZDD f1 = ZDD::Single.Change(v2);

    ZBDDV zv(f0, 0);
    zv += ZBDDV(f1, 1);

    int pin = compute_pin(zv);
    MLZBDDV ml(zv, pin, 2);

    // Outputs should remain unchanged
    ZDD out0 = ml.GetZBDDV().GetZBDD(0);
    ZDD out1 = ml.GetZBDDV().GetZBDD(1);
    EXPECT_EQ(out0, f0);
    EXPECT_EQ(out1, f1);
}

// ============================================================
// Multiple outputs sharing a common factor
// ============================================================

TEST_F(MLZBDDVTest, MultipleOutputsCommonFactor) {
    bddvar v1 = static_cast<bddvar>(BDDV_NewVar());
    bddvar v2 = static_cast<bddvar>(BDDV_NewVar());
    bddvar v3 = static_cast<bddvar>(BDDV_NewVar());
    bddvar v4 = static_cast<bddvar>(BDDV_NewVar());

    // f0 = {{v1, v2, v3}}
    ZDD f0 = ZDD::Single.Change(v1).Change(v2).Change(v3);
    // f1 = {{v1, v2, v4}}
    ZDD f1 = ZDD::Single.Change(v1).Change(v2).Change(v4);
    // f2 = {{v3}}
    ZDD f2 = ZDD::Single.Change(v3);

    ZBDDV zv(f0, 0);
    zv += ZBDDV(f1, 1);
    zv += ZBDDV(f2, 2);

    int pin = compute_pin(zv);
    MLZBDDV ml(zv, pin, 3);

    EXPECT_EQ(ml.N_out(), 3);
}

// ============================================================
// Phase 1: identical output divides and rewrites
// ============================================================

TEST_F(MLZBDDVTest, Phase1RewriteVerification) {
    bddvar v1 = static_cast<bddvar>(BDDV_NewVar());
    bddvar v2 = static_cast<bddvar>(BDDV_NewVar());
    bddvar v3 = static_cast<bddvar>(BDDV_NewVar());

    // f0 = {{v2}, {v3}} (a factor)
    ZDD f0 = ZDD::Single.Change(v2) + ZDD::Single.Change(v3);
    // f1 = {{v1, v2}, {v1, v3}} = {{v1}} * f0
    ZDD f1 = ZDD::Single.Change(v1).Change(v2)
           + ZDD::Single.Change(v1).Change(v3);

    ZBDDV zv(f0, 0);
    zv += ZBDDV(f1, 1);

    int pin = compute_pin(zv);
    MLZBDDV ml(zv, pin, 2);

    EXPECT_EQ(ml.N_out(), 2);
    // f0 should be unchanged (it's the divisor, not the dividend)
    ZDD out0 = ml.GetZBDDV().GetZBDD(0);
    EXPECT_EQ(out0, f0);
    // f1 should have been rewritten (contains sub-expression variable)
    ZDD out1 = ml.GetZBDDV().GetZBDD(1);
    // out1 != f1 means rewriting happened
    EXPECT_NE(out1, f1);
}
