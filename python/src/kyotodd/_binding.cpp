#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <sstream>
#include <fstream>
#include <random>
#include <iostream>
#include "bdd.h"
#include "qdd.h"
#include "unreduced_dd.h"
#include "pidd.h"
#include "rotpidd.h"
#include "seqbdd.h"

namespace py = pybind11;

struct CoutRedirectGuard {
    std::streambuf* old;
    CoutRedirectGuard(std::streambuf* buf) : old(std::cout.rdbuf(buf)) {}
    ~CoutRedirectGuard() { std::cout.rdbuf(old); }
};

static bool g_initialized = false;

static void reset_pidd_globals() {
    if (PiDD_XOfLev) { delete[] PiDD_XOfLev; PiDD_XOfLev = 0; }
    PiDD_TopVar = 0;
    PiDD_VarTableSize = 16;
}

static void reset_rotpidd_globals() {
    if (RotPiDD_XOfLev) { delete[] RotPiDD_XOfLev; RotPiDD_XOfLev = 0; }
    RotPiDD_TopVar = 0;
    RotPiDD_VarTableSize = 16;
}

static void ensure_init() {
    if (!g_initialized) {
        bddinit(256, UINT64_MAX);
        reset_pidd_globals();
        reset_rotpidd_globals();
        g_initialized = true;
    }
}

PYBIND11_MODULE(_core, m) {
    m.doc() = "KyotoDD BDD/ZDD library";

    // Exception translation
    py::register_exception_translator([](std::exception_ptr p) {
        try {
            if (p) std::rethrow_exception(p);
        } catch (const py::builtin_exception&) {
            throw;  // Let pybind11 handle its own exception types
        } catch (const py::error_already_set&) {
            throw;  // Let pybind11 handle Python errors
        } catch (const std::overflow_error& e) {
            PyErr_SetString(PyExc_MemoryError, e.what());
        } catch (const std::invalid_argument& e) {
            PyErr_SetString(PyExc_ValueError, e.what());
        } catch (const std::out_of_range& e) {
            PyErr_SetString(PyExc_IndexError, e.what());
        } catch (const std::logic_error& e) {
            PyErr_SetString(PyExc_RuntimeError, e.what());
        } catch (const std::bad_alloc& e) {
            PyErr_SetString(PyExc_MemoryError, e.what());
        } catch (const std::exception& e) {
            PyErr_SetString(PyExc_RuntimeError, e.what());
        }
    });

    // Initialization
    m.def("init", [](uint64_t node_count, uint64_t node_max) {
        if (g_initialized && bddgc_rootcount() > 0) {
            throw std::runtime_error(
                "init(): cannot re-initialize while BDD/ZDD objects exist. "
                "Delete all BDD/ZDD objects first.");
        }
        DDBase::init(node_count, node_max);
        reset_pidd_globals();
        reset_rotpidd_globals();
        g_initialized = true;
    }, py::arg("node_count") = 256, py::arg("node_max") = UINT64_MAX,
       "Initialize the BDD library.\n\n"
       "If not called explicitly, the library is auto-initialized with default\n"
       "parameters (node_count=256, node_max=unlimited) on first use. Call this\n"
       "function before any operations if you need to set custom limits.\n\n"
       "Raises RuntimeError if called while BDD/ZDD objects exist.\n\n"
       "Args:\n"
       "    node_count: Initial number of node slots to allocate (default: 256).\n"
       "    node_max: Maximum number of node slots allowed (default: unlimited).\n");

    m.def("finalize", []() {
        if (g_initialized && bddgc_rootcount() > 0) {
            throw std::runtime_error(
                "finalize(): cannot finalize while BDD/ZDD objects exist. "
                "Delete all BDD/ZDD objects first.");
        }
        bddfinal();
        reset_pidd_globals();
        reset_rotpidd_globals();
        g_initialized = false;
    }, "Finalize the BDD library and release all resources.\n\n"
       "Raises RuntimeError if called while BDD/ZDD objects exist.\n"
       "Safe to call multiple times when no objects are alive.\n");

    // Variable management
    m.def("new_var", []() -> bddvar {
        ensure_init();
        return DDBase::new_var();
    }, "Create a new variable and return its variable number.\n\n"
       "Returns:\n"
       "    The variable number of the newly created variable.\n");

    m.def("new_var_of_level", [](bddvar lev) -> bddvar {
        ensure_init();
        return bddnewvaroflev(lev);
    }, py::arg("lev"),
       "Create a new variable at the specified level.\n\n"
       "Args:\n"
       "    lev: The level to insert the new variable at.\n\n"
       "Returns:\n"
       "    The variable number of the newly created variable.\n");

    m.def("var_count", &DDBase::var_used,
       "Return the number of variables created so far.\n\n"
       "Returns:\n"
       "    The count of variables.\n");

    m.def("to_level", &DDBase::to_level, py::arg("var"),
       "Convert a variable number to its level.\n\n"
       "Args:\n"
       "    var: Variable number.\n\n"
       "Returns:\n"
       "    The level of the variable.\n");

    m.def("to_var", &DDBase::to_var, py::arg("level"),
       "Convert a level to its variable number.\n\n"
       "Args:\n"
       "    level: Level number.\n\n"
       "Returns:\n"
       "    The variable number at that level.\n");

    // Node stats
    m.def("node_count", &DDBase::node_count,
       "Return the number of used node slots (including dead nodes awaiting GC).\n\n"
       "This is not the initial capacity or the array size -- it is the\n"
       "count of node slots that have been occupied at least once.\n\n"
       "Returns:\n"
       "    The used node slot count.\n");

    // GC API
    m.def("gc", &DDBase::gc,
       "Manually invoke garbage collection.\n\n"
       "Reclaims dead nodes that are no longer referenced.\n");

    m.def("live_nodes", &bddlive,
       "Return the number of live (non-garbage) nodes.\n\n"
       "Returns:\n"
       "    The live node count.\n");

    m.def("gc_set_threshold", &bddgc_setthreshold, py::arg("threshold"),
       "Set the GC threshold.\n\n"
       "Args:\n"
       "    threshold: A value between 0.0 and 1.0 (default: 0.9).\n");

    m.def("gc_get_threshold", &bddgc_getthreshold,
       "Get the current GC threshold.\n\n"
       "Returns:\n"
       "    The current threshold value.\n");

    m.def("new_vars", [](int n, bool reverse) -> std::vector<bddvar> {
        ensure_init();
        return DDBase::new_var(n, reverse);
    }, py::arg("n"), py::arg("reverse") = false,
       "Create n new variables at once.\n\n"
       "Args:\n"
       "    n: Number of variables to create.\n"
       "    reverse: If True, insert each at level 1 (reverses var/level ordering).\n\n"
       "Returns:\n"
       "    A list of the newly created variable numbers.\n");

    m.def("gc_rootcount", &bddgc_rootcount,
       "Return the number of registered GC roots.\n\n"
       "Returns:\n"
       "    The GC root count.\n");

    m.def("top_level", &bddtoplev,
       "Return the highest level currently in use.\n\n"
       "Returns:\n"
       "    The top level number.\n");

    // ZDD_Random: no Python binding (use ZDD.random_family instead)
    // bdd_check_reduced: no Python binding (internal debugging utility)

    // Memo classes
    py::class_<BddCountMemo>(m, "BddCountMemo",
        "Memo for BDD exact counting.\n\n"
        "Caches exact_count results so they can be reused across\n"
        "multiple exact_count and uniform_sample calls on the same BDD.")
        .def(py::init([](const BDD& f, bddvar n) {
            return BddCountMemo(f, n);
        }), py::arg("bdd"), py::arg("n"),
           "Create a BDD count memo.\n\n"
           "Args:\n"
           "    bdd: The BDD to count.\n"
           "    n: Number of variables.\n")
        .def_property_readonly("stored", &BddCountMemo::stored,
             "Whether the memo has been populated.");

    py::class_<ZddCountMemo>(m, "ZddCountMemo",
        "Memo for ZDD exact counting.\n\n"
        "Caches exact_count results so they can be reused across\n"
        "multiple exact_count and uniform_sample calls on the same ZDD.")
        .def(py::init([](const ZDD& f) {
            return ZddCountMemo(f);
        }), py::arg("zdd"),
           "Create a ZDD count memo.\n\n"
           "Args:\n"
           "    zdd: The ZDD to count.\n")
        .def_property_readonly("stored", &ZddCountMemo::stored,
             "Whether the memo has been populated.");

    // CostBoundMemo class
    py::class_<CostBoundMemo>(m, "CostBoundMemo",
        "Interval-memoization table for cost-bounded enumeration.\n\n"
        "Caches intermediate results using the interval-memoizing technique\n"
        "(BkTrk-IntervalMemo) so that repeated cost_bound calls with\n"
        "different bounds on the same ZDD and weights are efficient.\n\n"
        "A single CostBoundMemo must only be used with one weights vector.\n"
        "Passing a different weights vector raises ValueError.\n")
        .def(py::init<>(),
           "Create an empty cost-bound memo.\n")
        .def("clear", &CostBoundMemo::clear,
             "Clear all cached entries.\n");

    // BDD class
    py::class_<BDD>(m, "BDD",
        "A Binary Decision Diagram representing a Boolean function.\n\n"
        "Each BDD object holds a root node ID and is automatically\n"
        "protected from garbage collection during its lifetime.")
        .def(py::init([](int val) {
            ensure_init();
            return BDD(val);
        }), py::arg("val") = 0,
           "Construct a BDD from an integer value.\n\n"
           "Args:\n"
           "    val: 0 for false, 1 for true, negative for null.\n")

        .def_static("var", [](bddvar v) -> BDD {
            ensure_init();
            return BDDvar(v);
        }, py::arg("v"),
           "Create a BDD representing the given variable.")

        .def_property_readonly_static("false_", [](py::object) -> BDD {
            return BDD::False;
        })
        .def_property_readonly_static("true_", [](py::object) -> BDD {
            return BDD::True;
        })
        .def_property_readonly_static("null", [](py::object) -> BDD {
            return BDD::Null;
        })

        .def("__eq__", [](const BDD& a, const BDD& b) { return a == b; },
             "Equality comparison by node ID.")
        .def("__ne__", [](const BDD& a, const BDD& b) { return a != b; },
             "Inequality comparison by node ID.")
        .def("__hash__", [](const BDD& a) {
            return std::hash<uint64_t>()(a.GetID());
        }, "Hash based on node ID.")
        .def("__repr__", [](const BDD& a) {
            return "BDD(node_id=" + std::to_string(a.GetID()) + ")";
        }, "Return string representation: BDD(node_id=...).")
        .def("__bool__", [](const BDD&) -> bool {
            throw py::type_error(
                "BDD cannot be converted to bool. "
                "Use == BDD.false_ or == BDD.true_ instead.");
        })

        // Operators
        .def("__and__",    [](const BDD& a, const BDD& b) { return a & b; },
             "Logical AND: self & other.")
        .def("__or__",     [](const BDD& a, const BDD& b) { return a | b; },
             "Logical OR: self | other.")
        .def("__xor__",    [](const BDD& a, const BDD& b) { return a ^ b; },
             "Logical XOR: self ^ other.")
        .def("__invert__", [](const BDD& a) { return ~a; },
             "Logical NOT: ~self.")
        .def("__lshift__", [](const BDD& a, bddvar s) { return a << s; },
             "Left shift: rename variable i to i+shift.")
        .def("__rshift__", [](const BDD& a, bddvar s) { return a >> s; },
             "Right shift: rename variable i to i-shift.")
        .def("__iand__",   [](BDD& a, const BDD& b) -> BDD& { a &= b; return a; },
             py::return_value_policy::reference_internal,
             "In-place logical AND.")
        .def("__ior__",    [](BDD& a, const BDD& b) -> BDD& { a |= b; return a; },
             py::return_value_policy::reference_internal,
             "In-place logical OR.")
        .def("__ixor__",   [](BDD& a, const BDD& b) -> BDD& { a ^= b; return a; },
             py::return_value_policy::reference_internal,
             "In-place logical XOR.")
        .def("__ilshift__",[](BDD& a, bddvar s) -> BDD& { a <<= s; return a; },
             py::return_value_policy::reference_internal,
             "In-place left shift.")
        .def("__irshift__",[](BDD& a, bddvar s) -> BDD& { a >>= s; return a; },
             py::return_value_policy::reference_internal,
             "In-place right shift.")

        // Named binary operations
        .def("nand", [](const BDD& a, const BDD& b) {
            return BDD_ID(bddnand(a.GetID(), b.GetID()));
        }, py::arg("other"),
             "Logical NAND: ~(self & other).")
        .def("nor", [](const BDD& a, const BDD& b) {
            return BDD_ID(bddnor(a.GetID(), b.GetID()));
        }, py::arg("other"),
             "Logical NOR: ~(self | other).")
        .def("xnor", [](const BDD& a, const BDD& b) {
            return BDD_ID(bddxnor(a.GetID(), b.GetID()));
        }, py::arg("other"),
             "Logical XNOR: ~(self ^ other).")

        // Methods
        .def("at0", &BDD::At0, py::arg("v"),
             "Cofactor: restrict variable v to 0.")
        .def("at1", &BDD::At1, py::arg("v"),
             "Cofactor: restrict variable v to 1.")
        .def("cofactor", &BDD::Cofactor, py::arg("g"),
             "Generalized cofactor by BDD g.")
        .def("support", &BDD::Support,
             "Return the support set as a BDD (disjunction of variables).")
        .def("support_vec", &BDD::SupportVec,
             "Return the support set as a list of variable numbers.")
        .def("imply", &BDD::Imply, py::arg("other"),
             "Check implication: return 1 if self => other, 0 otherwise.")

        // Quantification (lambdas to avoid overload_cast requiring C++14)
        .def("exist", [](const BDD& f, const BDD& cube) { return f.Exist(cube); },
             py::arg("cube"), "Existential quantification by variable-set BDD (as returned by support()).")
        .def("exist", [](const BDD& f, const std::vector<bddvar>& vars) { return f.Exist(vars); },
             py::arg("vars"), "Existential quantification by variable list.")
        .def("exist", [](const BDD& f, bddvar var) { return f.Exist(var); },
             py::arg("var"), "Existential quantification of a single variable.")
        .def("univ", [](const BDD& f, const BDD& cube) { return f.Univ(cube); },
             py::arg("cube"), "Universal quantification by variable-set BDD (as returned by support()).")
        .def("univ", [](const BDD& f, const std::vector<bddvar>& vars) { return f.Univ(vars); },
             py::arg("vars"), "Universal quantification by variable list.")
        .def("univ", [](const BDD& f, bddvar var) { return f.Univ(var); },
             py::arg("var"), "Universal quantification of a single variable.")

        // Static methods
        .def_static("ite", &BDD::Ite, py::arg("f"), py::arg("g"), py::arg("h"),
             "If-then-else: returns (f & g) | (~f & h).")

        // I/O
        .def("export_str", [](const BDD& b) -> std::string {
            std::ostringstream oss;
            bddp p = b.GetID();
            bddexport(oss, &p, 1);
            return oss.str();
        }, "Export this BDD to a string representation.\n\n"
           "Returns:\n"
           "    The serialized BDD.\n")
        .def_static("import_str", [](const std::string& s) -> BDD {
            ensure_init();
            std::istringstream iss(s);
            bddp p;
            int ret = bddimport(iss, &p, 1);
            if (ret < 1) throw std::runtime_error("BDD import failed");
            return BDD_ID(p);
        }, py::arg("s"),
           "Import a BDD from a string.\n\n"
           "Args:\n"
           "    s: The serialized BDD string.\n\n"
           "Returns:\n"
           "    The reconstructed BDD.\n\n"
           "Raises:\n"
           "    RuntimeError: If import fails.\n")
        .def("export_file", [](const BDD& b, py::object stream) {
            std::ostringstream oss;
            bddp p = b.GetID();
            bddexport(oss, &p, 1);
            stream.attr("write")(oss.str());
        }, py::arg("stream"),
           "Export this BDD to a text stream.\n\n"
           "Args:\n"
           "    stream: A writable text stream (e.g. open('f.txt', 'w')).\n")
        .def_static("import_file", [](py::object stream) -> BDD {
            ensure_init();
            std::string s = py::cast<std::string>(stream.attr("read")());
            std::istringstream iss(s);
            bddp p;
            int ret = bddimport(iss, &p, 1);
            if (ret < 1) throw std::runtime_error("BDD import failed");
            return BDD_ID(p);
        }, py::arg("stream"),
           "Import a BDD from a text stream.\n\n"
           "Args:\n"
           "    stream: A readable text stream (e.g. open('f.txt', 'r')).\n\n"
           "Returns:\n"
           "    The reconstructed BDD.\n")

        // Multi-root legacy I/O
        .def_static("export_multi_str", [](const std::vector<BDD>& bdds) -> std::string {
            std::ostringstream oss;
            std::vector<bddp> v;
            v.reserve(bdds.size());
            for (auto& b : bdds) v.push_back(b.GetID());
            bddexport(oss, v);
            return oss.str();
        }, py::arg("bdds"),
           "Export multiple BDDs to a string.\n\n"
           "Args:\n"
           "    bdds: List of BDD objects.\n\n"
           "Returns:\n"
           "    The serialized string.\n")
        .def_static("import_multi_str", [](const std::string& s) -> std::vector<BDD> {
            ensure_init();
            if (s.empty()) return {};
            std::istringstream iss(s);
            std::vector<bddp> v;
            int ret = bddimport(iss, v);
            if (ret < 0) throw std::runtime_error("BDD multi import failed");
            std::vector<BDD> result;
            result.reserve(v.size());
            for (auto p : v) result.push_back(BDD_ID(p));
            return result;
        }, py::arg("s"),
           "Import multiple BDDs from a string.\n\n"
           "Args:\n"
           "    s: The serialized string.\n\n"
           "Returns:\n"
           "    A list of BDD objects.\n")
        .def_static("export_multi_file", [](const std::vector<BDD>& bdds, py::object stream) {
            std::ostringstream oss;
            std::vector<bddp> v;
            v.reserve(bdds.size());
            for (auto& b : bdds) v.push_back(b.GetID());
            bddexport(oss, v);
            stream.attr("write")(oss.str());
        }, py::arg("bdds"), py::arg("stream"),
           "Export multiple BDDs to a text stream.\n\n"
           "Args:\n"
           "    bdds: List of BDD objects.\n"
           "    stream: A writable text stream.\n")
        .def_static("import_multi_file", [](py::object stream) -> std::vector<BDD> {
            ensure_init();
            std::string s = py::cast<std::string>(stream.attr("read")());
            if (s.empty()) return {};
            std::istringstream iss(s);
            std::vector<bddp> v;
            int ret = bddimport(iss, v);
            if (ret < 0) throw std::runtime_error("BDD multi import failed");
            std::vector<BDD> result;
            result.reserve(v.size());
            for (auto p : v) result.push_back(BDD_ID(p));
            return result;
        }, py::arg("stream"),
           "Import multiple BDDs from a text stream.\n\n"
           "Args:\n"
           "    stream: A readable text stream.\n\n"
           "Returns:\n"
           "    A list of BDD objects.\n")

        .def("swap", &BDD::Swap, py::arg("v1"), py::arg("v2"),
             "Swap variables v1 and v2 in the BDD.\n\n"
             "Args:\n"
             "    v1: First variable number.\n"
             "    v2: Second variable number.\n\n"
             "Returns:\n"
             "    The BDD with v1 and v2 swapped.\n")
        .def("smooth", &BDD::Smooth, py::arg("v"),
             "Smooth (existential quantification) of variable v.\n\n"
             "Args:\n"
             "    v: Variable number to quantify out.\n\n"
             "Returns:\n"
             "    The resulting BDD.\n")
        .def("spread", &BDD::Spread, py::arg("k"),
             "Spread variable values to neighboring k levels.\n\n"
             "Args:\n"
             "    k: Number of levels to spread (must be >= 0).\n\n"
             "Returns:\n"
             "    The resulting BDD.\n")

        .def_property_readonly("node_id", [](const BDD& b) { return b.GetID(); },
             "The raw node ID of this BDD.")
        .def_property_readonly("raw_size", [](const BDD& b) { return b.raw_size(); },
             "The number of nodes in the DAG of this BDD.")
        .def_property_readonly("plain_size", [](const BDD& b) { return b.plain_size(); },
             "The number of nodes without complement edge sharing.")
        .def_property_readonly("top_var", [](const BDD& b) -> bddvar {
            return bddtop(b.GetID());
        }, "The top (root) variable number of this BDD.")
        .def_property_readonly("is_terminal", &BDD::is_terminal,
             "True if this is a terminal node.")
        .def_property_readonly("is_one", &BDD::is_one,
             "True if this is the 1-terminal (true).")
        .def_property_readonly("is_zero", &BDD::is_zero,
             "True if this is the 0-terminal (false).")

        // Conversion
        .def("to_qdd", &BDD::to_qdd,
             "Convert to a Quasi-reduced Decision Diagram (QDD).\n\n"
             "Returns:\n"
             "    The QDD representation.\n")

        // Counting
        .def("count", &BDD::count, py::arg("n"),
             "Count the number of satisfying assignments (floating-point).\n\n"
             "Args:\n"
             "    n: Number of variables in the Boolean function.\n\n"
             "Returns:\n"
             "    The number of satisfying assignments as a float.\n")
        .def("exact_count", [](const BDD& b, bddvar n) -> py::int_ {
            bigint::BigInt bi = b.exact_count(n);
            std::string s = bi.to_string();
            return py::int_(py::str(s));
        }, py::arg("n"),
           "Count the number of satisfying assignments (arbitrary precision).\n\n"
           "Args:\n"
           "    n: Number of variables in the Boolean function.\n\n"
           "Returns:\n"
           "    The number of satisfying assignments as a Python int.\n")
        .def("exact_count_with_memo", [](const BDD& b, bddvar n, BddCountMemo& memo) -> py::int_ {
            bigint::BigInt bi = b.exact_count(n, memo);
            std::string s = bi.to_string();
            return py::int_(py::str(s));
        }, py::arg("n"), py::arg("memo"),
           "Count satisfying assignments using a memo for caching.\n\n"
           "Args:\n"
           "    n: Number of variables in the Boolean function.\n"
           "    memo: A BddCountMemo object for caching.\n\n"
           "Returns:\n"
           "    The number of satisfying assignments as a Python int.\n")
        .def("uniform_sample", [](BDD& b, bddvar n, uint64_t seed) -> std::vector<bddvar> {
            std::mt19937_64 rng(seed);
            BddCountMemo memo(b, n);
            return b.uniform_sample(rng, n, memo);
        }, py::arg("n"), py::arg("seed") = 0,
           "Sample a satisfying assignment uniformly at random.\n\n"
           "Args:\n"
           "    n: Number of variables in the Boolean function.\n"
           "    seed: Random seed (default: 0).\n\n"
           "Returns:\n"
           "    A list of variable numbers set to 1 in the sampled assignment.\n")
        .def("uniform_sample_with_memo", [](BDD& b, bddvar n, BddCountMemo& memo, uint64_t seed) -> std::vector<bddvar> {
            std::mt19937_64 rng(seed);
            return b.uniform_sample(rng, n, memo);
        }, py::arg("n"), py::arg("memo"), py::arg("seed") = 0,
           "Sample a satisfying assignment using a memo for caching.\n\n"
           "Args:\n"
           "    n: Number of variables in the Boolean function.\n"
           "    memo: A BddCountMemo object for caching.\n"
           "    seed: Random seed (default: 0).\n\n"
           "Returns:\n"
           "    A list of variable numbers set to 1 in the sampled assignment.\n")

        // Static factory methods
        .def_static("prime", [](bddvar v) -> BDD {
            ensure_init();
            return BDD::prime(v);
        }, py::arg("v"),
           "Create a BDD for the positive literal of variable v.\n\n"
           "Args:\n"
           "    v: Variable number.\n\n"
           "Returns:\n"
           "    A BDD representing the variable v.\n")
        .def_static("prime_not", [](bddvar v) -> BDD {
            ensure_init();
            return BDD::prime_not(v);
        }, py::arg("v"),
           "Create a BDD for the negative literal of variable v.\n\n"
           "Args:\n"
           "    v: Variable number.\n\n"
           "Returns:\n"
           "    A BDD representing NOT v.\n")
        .def_static("cube", [](const std::vector<int>& lits) -> BDD {
            ensure_init();
            return BDD::cube(lits);
        }, py::arg("lits"),
           "Create a BDD for the conjunction (AND) of literals.\n\n"
           "Uses DIMACS convention: positive int = variable,\n"
           "negative int = negated variable.\n\n"
           "Args:\n"
           "    lits: List of literals (e.g. [1, -2, 3] means x1 & ~x2 & x3).\n\n"
           "Returns:\n"
           "    A BDD representing the cube.\n")
        .def_static("clause", [](const std::vector<int>& lits) -> BDD {
            ensure_init();
            return BDD::clause(lits);
        }, py::arg("lits"),
           "Create a BDD for the disjunction (OR) of literals.\n\n"
           "Uses DIMACS convention: positive int = variable,\n"
           "negative int = negated variable.\n\n"
           "Args:\n"
           "    lits: List of literals (e.g. [1, -2, 3] means x1 | ~x2 | x3).\n\n"
           "Returns:\n"
           "    A BDD representing the clause.\n")
        .def_static("getnode", [](bddvar var, const BDD& lo, const BDD& hi) -> BDD {
            ensure_init();
            return BDD::getnode(var, lo, hi);
        }, py::arg("var"), py::arg("lo"), py::arg("hi"),
           "Create a BDD node with the given variable and children.\n\n"
           "Applies BDD reduction rules (jump rule, complement normalization).\n\n"
           "Args:\n"
           "    var: Variable number.\n"
           "    lo: The low (0-edge) child.\n"
           "    hi: The high (1-edge) child.\n\n"
           "Returns:\n"
           "    The created BDD node.\n")
        .def_static("cache_get", [](uint8_t op, const BDD& f, const BDD& g) -> BDD {
            return BDD::cache_get(op, f, g);
        }, py::arg("op"), py::arg("f"), py::arg("g"),
           "Read a 2-operand cache entry.\n\n"
           "Args:\n"
           "    op: Operation code (0-255).\n"
           "    f: First operand BDD.\n"
           "    g: Second operand BDD.\n\n"
           "Returns:\n"
           "    The cached BDD result, or BDD.null on miss.\n")
        .def_static("cache_put", [](uint8_t op, const BDD& f, const BDD& g, const BDD& result) {
            BDD::cache_put(op, f, g, result);
        }, py::arg("op"), py::arg("f"), py::arg("g"), py::arg("result"),
           "Write a 2-operand cache entry.\n\n"
           "Args:\n"
           "    op: Operation code (0-255).\n"
           "    f: First operand BDD.\n"
           "    g: Second operand BDD.\n"
           "    result: The result BDD to cache.\n")
        .def_static("shared_size", [](const std::vector<BDD>& v) -> uint64_t {
            return BDD::raw_size(v);
        }, py::arg("bdds"),
           "Count the total number of shared nodes across multiple BDDs.\n\n"
           "Args:\n"
           "    bdds: List of BDD objects.\n\n"
           "Returns:\n"
           "    The number of distinct nodes (with complement sharing).\n")
        .def_static("shared_plain_size", [](const std::vector<BDD>& v) -> uint64_t {
            return BDD::plain_size(v);
        }, py::arg("bdds"),
           "Count the total number of nodes across multiple BDDs without complement sharing.\n\n"
           "Args:\n"
           "    bdds: List of BDD objects.\n\n"
           "Returns:\n"
           "    The number of nodes.\n")

        // Child accessors
        .def("child0", [](const BDD& b) { return b.child0(); },
             "Get the 0-child (lo) with complement edge resolution.")
        .def("child1", [](const BDD& b) { return b.child1(); },
             "Get the 1-child (hi) with complement edge resolution.")
        .def("child", [](const BDD& b, int c) { return b.child(c); },
             py::arg("child"),
             "Get the child by index (0 or 1) with complement edge resolution.")
        .def("raw_child0", [](const BDD& b) { return b.raw_child0(); },
             "Get the raw 0-child (lo) without complement resolution.")
        .def("raw_child1", [](const BDD& b) { return b.raw_child1(); },
             "Get the raw 1-child (hi) without complement resolution.")
        .def("raw_child", [](const BDD& b, int c) { return b.raw_child(c); },
             py::arg("child"),
             "Get the raw child by index (0 or 1) without complement resolution.")

        // Binary I/O
        .def("export_binary_str", [](const BDD& b) -> py::bytes {
            std::ostringstream oss;
            b.export_binary(oss);
            return py::bytes(oss.str());
        }, "Export this BDD in binary format to a bytes object.")
        .def_static("import_binary_str", [](const std::string& data, bool ignore_type) -> BDD {
            ensure_init();
            std::istringstream iss(data);
            return BDD::import_binary(iss, ignore_type);
        }, py::arg("data"), py::arg("ignore_type") = false,
           "Import a BDD from binary format bytes.")
        .def("export_binary_file", [](const BDD& b, py::object stream) {
            std::ostringstream oss;
            b.export_binary(oss);
            stream.attr("write")(py::bytes(oss.str()));
        }, py::arg("stream"),
           "Export this BDD in binary format to a binary stream.")
        .def_static("import_binary_file", [](py::object stream, bool ignore_type) -> BDD {
            ensure_init();
            std::string data = py::cast<std::string>(stream.attr("read")());
            std::istringstream iss(data);
            return BDD::import_binary(iss, ignore_type);
        }, py::arg("stream"), py::arg("ignore_type") = false,
           "Import a BDD from a binary stream.")

        // Multi-root binary I/O
        .def_static("export_binary_multi_str", [](const std::vector<BDD>& bdds) -> py::bytes {
            std::ostringstream oss;
            BDD::export_binary_multi(oss, bdds);
            return py::bytes(oss.str());
        }, py::arg("bdds"),
           "Export multiple BDDs in binary format to a bytes object.")
        .def_static("import_binary_multi_str", [](const std::string& data, bool ignore_type) -> std::vector<BDD> {
            ensure_init();
            std::istringstream iss(data);
            return BDD::import_binary_multi(iss, ignore_type);
        }, py::arg("data"), py::arg("ignore_type") = false,
           "Import multiple BDDs from binary format bytes.")
        .def_static("export_binary_multi_file", [](const std::vector<BDD>& bdds, py::object stream) {
            std::ostringstream oss;
            BDD::export_binary_multi(oss, bdds);
            stream.attr("write")(py::bytes(oss.str()));
        }, py::arg("bdds"), py::arg("stream"),
           "Export multiple BDDs in binary format to a binary stream.")
        .def_static("import_binary_multi_file", [](py::object stream, bool ignore_type) -> std::vector<BDD> {
            ensure_init();
            std::string data = py::cast<std::string>(stream.attr("read")());
            std::istringstream iss(data);
            return BDD::import_binary_multi(iss, ignore_type);
        }, py::arg("stream"), py::arg("ignore_type") = false,
           "Import multiple BDDs from a binary stream.")

        // Sapporo I/O
        .def("export_sapporo_str", [](const BDD& b) -> std::string {
            std::ostringstream oss;
            b.export_sapporo(oss);
            return oss.str();
        }, "Export this BDD in Sapporo format to a string.")
        .def_static("import_sapporo_str", [](const std::string& s) -> BDD {
            ensure_init();
            std::istringstream iss(s);
            return BDD::import_sapporo(iss);
        }, py::arg("s"),
           "Import a BDD from a Sapporo format string.")
        .def("export_sapporo_file", [](const BDD& b, py::object stream) {
            std::ostringstream oss;
            b.export_sapporo(oss);
            stream.attr("write")(oss.str());
        }, py::arg("stream"),
           "Export this BDD in Sapporo format to a text stream.")
        .def_static("import_sapporo_file", [](py::object stream) -> BDD {
            ensure_init();
            std::string s = py::cast<std::string>(stream.attr("read")());
            std::istringstream iss(s);
            return BDD::import_sapporo(iss);
        }, py::arg("stream"),
           "Import a BDD from a Sapporo format text stream.")

        // Knuth I/O (deprecated)
        .def("export_knuth_str", [](const BDD& b, bool is_hex, int offset) -> std::string {
            std::ostringstream oss;
            b.export_knuth(oss, is_hex, offset);
            return oss.str();
        }, py::arg("is_hex") = false, py::arg("offset") = 0,
           "Export this BDD in Knuth format to a string (deprecated).\n\n"
           "Args:\n"
           "    is_hex: If True, use hexadecimal node IDs.\n"
           "    offset: Variable number offset (default: 0).\n\n"
           "Returns:\n"
           "    A Knuth format string.\n\n"
           ".. deprecated:: Use export_sapporo_str() or export_binary_str() instead.\n")
        .def_static("import_knuth_str", [](const std::string& s, bool is_hex, int offset) -> BDD {
            ensure_init();
            std::istringstream iss(s);
            return BDD::import_knuth(iss, is_hex, offset);
        }, py::arg("s"), py::arg("is_hex") = false, py::arg("offset") = 0,
           "Import a BDD from a Knuth format string (deprecated).\n\n"
           "Args:\n"
           "    s: The Knuth format string.\n"
           "    is_hex: If True, use hexadecimal node IDs.\n"
           "    offset: Variable number offset (default: 0).\n\n"
           "Returns:\n"
           "    The reconstructed BDD.\n\n"
           ".. deprecated:: Use import_sapporo_str() or import_binary_str() instead.\n")
        .def("export_knuth_file", [](const BDD& b, py::object stream, bool is_hex, int offset) {
            std::ostringstream oss;
            b.export_knuth(oss, is_hex, offset);
            stream.attr("write")(oss.str());
        }, py::arg("stream"), py::arg("is_hex") = false, py::arg("offset") = 0,
           "Export this BDD in Knuth format to a text stream (deprecated).\n\n"
           "Args:\n"
           "    stream: A writable text stream.\n"
           "    is_hex: If True, use hexadecimal node IDs.\n"
           "    offset: Variable number offset (default: 0).\n\n"
           ".. deprecated:: Use export_sapporo_file() or export_binary_file() instead.\n")
        .def_static("import_knuth_file", [](py::object stream, bool is_hex, int offset) -> BDD {
            ensure_init();
            std::string s = py::cast<std::string>(stream.attr("read")());
            std::istringstream iss(s);
            return BDD::import_knuth(iss, is_hex, offset);
        }, py::arg("stream"), py::arg("is_hex") = false, py::arg("offset") = 0,
           "Import a BDD from a Knuth format text stream (deprecated).\n\n"
           "Args:\n"
           "    stream: A readable text stream.\n"
           "    is_hex: If True, use hexadecimal node IDs.\n"
           "    offset: Variable number offset (default: 0).\n\n"
           "Returns:\n"
           "    The reconstructed BDD.\n\n"
           ".. deprecated:: Use import_sapporo_file() or import_binary_file() instead.\n")

        // Graphviz
        .def("save_graphviz_str", [](const BDD& b, bool raw) -> std::string {
            std::ostringstream oss;
            b.save_graphviz(oss, raw ? GraphvizMode::Raw : GraphvizMode::Expanded);
            return oss.str();
        }, py::arg("raw") = false,
           "Export this BDD as a Graphviz DOT string.\n\n"
           "Args:\n"
           "    raw: If True, show physical DAG with complement markers.\n"
           "         If False (default), expand complement edges into full nodes.\n\n"
           "Returns:\n"
           "    A DOT format string.\n")
        .def("save_graphviz_file", [](const BDD& b, py::object stream, bool raw) {
            std::ostringstream oss;
            b.save_graphviz(oss, raw ? GraphvizMode::Raw : GraphvizMode::Expanded);
            stream.attr("write")(oss.str());
        }, py::arg("stream"), py::arg("raw") = false,
           "Export this BDD as a Graphviz DOT to a text stream.\n\n"
           "Args:\n"
           "    stream: A writable text stream.\n"
           "    raw: If True, show physical DAG with complement markers.\n"
           "         If False (default), expand complement edges into full nodes.\n")

        .def("print", [](const BDD& b) -> std::string {
            std::ostringstream oss;
            CoutRedirectGuard guard(oss.rdbuf());
            b.Print();
            return oss.str();
        }, "Print BDD summary (ID, Var, Level, Size) and return as string.")
    ;

    // ZDD class
    py::class_<ZDD>(m, "ZDD",
        "A Zero-suppressed Decision Diagram representing a family of sets.\n\n"
        "Each ZDD object holds a root node ID and is automatically\n"
        "protected from garbage collection during its lifetime.")
        .def(py::init([](int val) {
            ensure_init();
            return ZDD(val);
        }), py::arg("val") = 0,
           "Construct a ZDD from an integer value.\n\n"
           "Args:\n"
           "    val: 0 for empty family, 1 for unit family, negative for null.\n")

        .def_property_readonly_static("empty", [](py::object) -> ZDD {
            return ZDD::Empty;
        })
        .def_property_readonly_static("single", [](py::object) -> ZDD {
            return ZDD::Single;
        })
        .def_property_readonly_static("null", [](py::object) -> ZDD {
            return ZDD::Null;
        })

        .def("__eq__", [](const ZDD& a, const ZDD& b) { return a == b; },
             "Equality comparison by node ID.")
        .def("__ne__", [](const ZDD& a, const ZDD& b) { return a != b; },
             "Inequality comparison by node ID.")
        .def("__hash__", [](const ZDD& a) {
            return std::hash<uint64_t>()(a.GetID());
        }, "Hash based on node ID.")
        .def("__repr__", [](const ZDD& a) {
            return "ZDD(node_id=" + std::to_string(a.GetID()) + ")";
        }, "Return string representation: ZDD(node_id=...).")

        // Operators
        .def("__invert__", [](const ZDD& a) { return ~a; },
             "Complement: toggle empty set membership (~self).")
        .def("__add__",      [](const ZDD& a, const ZDD& b) { return a + b; },
             "Union: self + other.")
        .def("__sub__",      [](const ZDD& a, const ZDD& b) { return a - b; },
             "Subtraction (set difference): self - other.")
        .def("__and__",      [](const ZDD& a, const ZDD& b) { return a & b; },
             "Intersection: self & other.")
        .def("__xor__",      [](const ZDD& a, const ZDD& b) { return a ^ b; },
             "Symmetric difference: self ^ other.")
        .def("__mul__",      [](const ZDD& a, const ZDD& b) { return a * b; },
             "Join (cross product with union): self * other.")
        .def("__truediv__",  [](const ZDD& a, const ZDD& b) { return a / b; },
             "Division (quotient): self / other.")
        .def("__mod__",      [](const ZDD& a, const ZDD& b) { return a % b; },
             "Remainder: self % other.")
        .def("__iadd__",     [](ZDD& a, const ZDD& b) -> ZDD& { a += b; return a; },
             py::return_value_policy::reference_internal,
             "In-place union.")
        .def("__isub__",     [](ZDD& a, const ZDD& b) -> ZDD& { a -= b; return a; },
             py::return_value_policy::reference_internal,
             "In-place subtraction.")
        .def("__iand__",     [](ZDD& a, const ZDD& b) -> ZDD& { a &= b; return a; },
             py::return_value_policy::reference_internal,
             "In-place intersection.")
        .def("__ixor__",     [](ZDD& a, const ZDD& b) -> ZDD& { a ^= b; return a; },
             py::return_value_policy::reference_internal,
             "In-place symmetric difference.")
        .def("__imul__",     [](ZDD& a, const ZDD& b) -> ZDD& { a *= b; return a; },
             py::return_value_policy::reference_internal,
             "In-place join.")
        .def("__itruediv__", [](ZDD& a, const ZDD& b) -> ZDD& { a /= b; return a; },
             py::return_value_policy::reference_internal,
             "In-place division.")
        .def("__imod__",     [](ZDD& a, const ZDD& b) -> ZDD& { a %= b; return a; },
             py::return_value_policy::reference_internal,
             "In-place remainder.")

        // Methods
        .def("change", &ZDD::Change, py::arg("v"),
             "Toggle membership of variable v in all sets.\n\n"
             "For each set S in the family, if v is in S then remove it,\n"
             "otherwise add it.\n\n"
             "Args:\n"
             "    v: Variable number.\n\n"
             "Returns:\n"
             "    The resulting ZDD.\n")
        .def("offset", &ZDD::Offset, py::arg("v"),
             "Select sets NOT containing variable v.\n\n"
             "Args:\n"
             "    v: Variable number.\n\n"
             "Returns:\n"
             "    The resulting ZDD.\n")
        .def("onset", &ZDD::OnSet, py::arg("v"),
             "Select sets containing variable v (v is kept in the result).\n\n"
             "Args:\n"
             "    v: Variable number.\n\n"
             "Returns:\n"
             "    The resulting ZDD.\n")
        .def("onset0", &ZDD::OnSet0, py::arg("v"),
             "Select sets containing variable v, then remove v (1-cofactor).\n\n"
             "Args:\n"
             "    v: Variable number.\n\n"
             "Returns:\n"
             "    The resulting ZDD.\n")
        .def("meet", &ZDD::Meet, py::arg("other"),
             "Meet operation (intersection of all element pairs).\n\n"
             "For each pair of sets (one from this family, one from other),\n"
             "compute their intersection and collect all results.\n\n"
             "Args:\n"
             "    other: Another ZDD family.\n\n"
             "Returns:\n"
             "    The resulting ZDD.\n")
        .def("maximal", &ZDD::Maximal,
             "Extract maximal sets (no proper superset in the family).\n\n"
             "Returns:\n"
             "    A ZDD containing only the maximal sets.\n")
        .def("minimal", &ZDD::Minimal,
             "Extract minimal sets (no proper subset in the family).\n\n"
             "Returns:\n"
             "    A ZDD containing only the minimal sets.\n")
        .def("minhit", &ZDD::Minhit,
             "Compute minimum hitting sets.\n\n"
             "A hitting set intersects every set in the family.\n\n"
             "Returns:\n"
             "    A ZDD of minimal hitting sets.\n")
        .def("closure", &ZDD::Closure,
             "Compute the downward closure.\n\n"
             "Returns all subsets of sets in the family.\n\n"
             "Returns:\n"
             "    A ZDD representing the downward closure.\n")
        .def("restrict", &ZDD::Restrict, py::arg("g"),
             "Restrict to sets that are subsets of some set in g.\n\n"
             "Args:\n"
             "    g: The constraining family.\n\n"
             "Returns:\n"
             "    The restricted ZDD.\n")
        .def("permit", &ZDD::Permit, py::arg("g"),
             "Keep sets whose elements are all permitted by g.\n\n"
             "Args:\n"
             "    g: The permitting family.\n\n"
             "Returns:\n"
             "    The resulting ZDD.\n")
        .def("nonsup", &ZDD::Nonsup, py::arg("g"),
             "Remove sets that are supersets of some set in g.\n\n"
             "Args:\n"
             "    g: The constraining family.\n\n"
             "Returns:\n"
             "    The resulting ZDD.\n")
        .def("nonsub", &ZDD::Nonsub, py::arg("g"),
             "Remove sets that are subsets of some set in g.\n\n"
             "Args:\n"
             "    g: The constraining family.\n\n"
             "Returns:\n"
             "    The resulting ZDD.\n")
        .def("disjoin", &ZDD::Disjoin, py::arg("g"),
             "Disjoint product of two families.\n\n"
             "For each pair (A, B) where A is in this family and B is in g,\n"
             "if A and B are disjoint, include A | B in the result.\n\n"
             "Args:\n"
             "    g: The other family.\n\n"
             "Returns:\n"
             "    The resulting ZDD.\n")
        .def("jointjoin", &ZDD::Jointjoin, py::arg("g"),
             "Joint join of two families.\n\n"
             "For each pair (A, B) with A & B non-empty,\n"
             "include A | B in the result.\n"
             "Pairs with no overlap are excluded.\n\n"
             "Args:\n"
             "    g: The other family.\n\n"
             "Returns:\n"
             "    The resulting ZDD.\n")
        .def("delta", &ZDD::Delta, py::arg("g"),
             "Delta operation (symmetric difference of elements within pairs).\n\n"
             "Args:\n"
             "    g: The other family.\n\n"
             "Returns:\n"
             "    The resulting ZDD.\n")
        .def("always", &ZDD::Always,
             "Find elements common to ALL sets in the family.\n\n"
             "Returns:\n"
             "    A ZDD family of singletons, one for each always-present variable.\n"
             "    For example, {{1,2,3},{1,2}}.always() returns {{1},{2}}.\n")
        .def("permit_sym", &ZDD::PermitSym, py::arg("n"),
             "Symmetric permit: keep sets with at most n elements.\n\n"
             "Args:\n"
             "    n: Maximum number of elements.\n\n"
             "Returns:\n"
             "    A ZDD containing only sets with <= n elements.\n")
        .def("swap", &ZDD::Swap, py::arg("v1"), py::arg("v2"),
             "Swap two variables in the family.\n\n"
             "Args:\n"
             "    v1: First variable number.\n"
             "    v2: Second variable number.\n\n"
             "Returns:\n"
             "    A ZDD with v1 and v2 swapped.\n")
        .def("imply_chk", &ZDD::ImplyChk, py::arg("v1"), py::arg("v2"),
             "Check if v1 implies v2 in the family.\n\n"
             "Args:\n"
             "    v1: First variable number.\n"
             "    v2: Second variable number.\n\n"
             "Returns:\n"
             "    1 if every set containing v1 also contains v2, 0 otherwise.\n")
        .def("coimply_chk", &ZDD::CoImplyChk, py::arg("v1"), py::arg("v2"),
             "Check co-implication between v1 and v2 in the family.\n\n"
             "Args:\n"
             "    v1: First variable number.\n"
             "    v2: Second variable number.\n\n"
             "Returns:\n"
             "    1 if co-implication holds, 0 otherwise.\n")
        .def("sym_chk", &ZDD::SymChk, py::arg("v1"), py::arg("v2"),
             "Check if two variables are symmetric in the family.\n\n"
             "Args:\n"
             "    v1: First variable number.\n"
             "    v2: Second variable number.\n\n"
             "Returns:\n"
             "    1 if symmetric, 0 if not.\n")
        .def("imply_set", &ZDD::ImplySet, py::arg("v"),
             "Find all variables implied by v in the family.\n\n"
             "Args:\n"
             "    v: Variable number.\n\n"
             "Returns:\n"
             "    A ZDD (family of singletons) of variables that v implies.\n")
        .def("sym_grp", &ZDD::SymGrp,
             "Find all symmetry groups (size >= 2) in the family.\n\n"
             "Returns:\n"
             "    A ZDD family where each set is a symmetry group.\n")
        .def("sym_grp_naive", &ZDD::SymGrpNaive,
             "Find all symmetry groups (naive method, includes size 1).\n\n"
             "Returns:\n"
             "    A ZDD family where each set is a symmetry group.\n")
        .def("sym_set", &ZDD::SymSet, py::arg("v"),
             "Find all variables symmetric with v in the family.\n\n"
             "Args:\n"
             "    v: Variable number.\n\n"
             "Returns:\n"
             "    A ZDD (single set) of variables symmetric with v.\n")
        .def("coimply_set", &ZDD::CoImplySet, py::arg("v"),
             "Find all variables in co-implication relation with v.\n\n"
             "Args:\n"
             "    v: Variable number.\n\n"
             "Returns:\n"
             "    A ZDD (single set) of variables co-implied by v.\n")
        .def("divisor", &ZDD::Divisor,
             "Find a non-trivial divisor of the family (as polynomial).\n\n"
             "Returns:\n"
             "    A ZDD representing a divisor.\n")
        .def("__bool__", [](const ZDD&) -> bool {
            throw py::type_error(
                "ZDD cannot be converted to bool. "
                "Use == ZDD.empty or == ZDD.single instead.");
        })
        .def("__lshift__", [](const ZDD& a, int s) {
                 if (s < 0) throw py::value_error("shift amount must be non-negative");
                 return a << static_cast<bddvar>(s);
             },
             "Left shift: increase variable numbers by s.")
        .def("__rshift__", [](const ZDD& a, int s) {
                 if (s < 0) throw py::value_error("shift amount must be non-negative");
                 return a >> static_cast<bddvar>(s);
             },
             "Right shift: decrease variable numbers by s.")
        .def("__ilshift__", [](ZDD& a, int s) -> ZDD& {
                 if (s < 0) throw py::value_error("shift amount must be non-negative");
                 a <<= static_cast<bddvar>(s); return a;
             },
             py::return_value_policy::reference_internal,
             "In-place left shift.")
        .def("__irshift__", [](ZDD& a, int s) -> ZDD& {
                 if (s < 0) throw py::value_error("shift amount must be non-negative");
                 a >>= static_cast<bddvar>(s); return a;
             },
             py::return_value_policy::reference_internal,
             "In-place right shift.")

        // I/O
        .def("export_str", [](const ZDD& z) -> std::string {
            std::ostringstream oss;
            bddp p = z.GetID();
            bddexport(oss, &p, 1);
            return oss.str();
        }, "Export this ZDD to a string representation.\n\n"
           "Returns:\n"
           "    The serialized ZDD.\n")
        .def_static("import_str", [](const std::string& s) -> ZDD {
            ensure_init();
            std::istringstream iss(s);
            bddp p;
            int ret = bddimportz(iss, &p, 1);
            if (ret < 1) throw std::runtime_error("ZDD import failed");
            return ZDD_ID(p);
        }, py::arg("s"),
           "Import a ZDD from a string.\n\n"
           "Args:\n"
           "    s: The serialized ZDD string.\n\n"
           "Returns:\n"
           "    The reconstructed ZDD.\n\n"
           "Raises:\n"
           "    RuntimeError: If import fails.\n")
        .def("export_file", [](const ZDD& z, py::object stream) {
            std::ostringstream oss;
            bddp p = z.GetID();
            bddexport(oss, &p, 1);
            stream.attr("write")(oss.str());
        }, py::arg("stream"),
           "Export this ZDD to a text stream.\n\n"
           "Args:\n"
           "    stream: A writable text stream (e.g. open('f.txt', 'w')).\n")
        .def_static("import_file", [](py::object stream) -> ZDD {
            ensure_init();
            std::string s = py::cast<std::string>(stream.attr("read")());
            std::istringstream iss(s);
            bddp p;
            int ret = bddimportz(iss, &p, 1);
            if (ret < 1) throw std::runtime_error("ZDD import failed");
            return ZDD_ID(p);
        }, py::arg("stream"),
           "Import a ZDD from a text stream.\n\n"
           "Args:\n"
           "    stream: A readable text stream (e.g. open('f.txt', 'r')).\n\n"
           "Returns:\n"
           "    The reconstructed ZDD.\n")
        // Multi-root legacy I/O
        .def_static("export_multi_str", [](const std::vector<ZDD>& zdds) -> std::string {
            std::ostringstream oss;
            std::vector<bddp> v;
            v.reserve(zdds.size());
            for (auto& z : zdds) v.push_back(z.GetID());
            bddexport(oss, v);
            return oss.str();
        }, py::arg("zdds"),
           "Export multiple ZDDs to a string.\n\n"
           "Args:\n"
           "    zdds: List of ZDD objects.\n\n"
           "Returns:\n"
           "    The serialized string.\n")
        .def_static("import_multi_str", [](const std::string& s) -> std::vector<ZDD> {
            ensure_init();
            if (s.empty()) return {};
            std::istringstream iss(s);
            std::vector<bddp> v;
            int ret = bddimportz(iss, v);
            if (ret < 0) throw std::runtime_error("ZDD multi import failed");
            std::vector<ZDD> result;
            result.reserve(v.size());
            for (auto p : v) result.push_back(ZDD_ID(p));
            return result;
        }, py::arg("s"),
           "Import multiple ZDDs from a string.\n\n"
           "Args:\n"
           "    s: The serialized string.\n\n"
           "Returns:\n"
           "    A list of ZDD objects.\n")
        .def_static("export_multi_file", [](const std::vector<ZDD>& zdds, py::object stream) {
            std::ostringstream oss;
            std::vector<bddp> v;
            v.reserve(zdds.size());
            for (auto& z : zdds) v.push_back(z.GetID());
            bddexport(oss, v);
            stream.attr("write")(oss.str());
        }, py::arg("zdds"), py::arg("stream"),
           "Export multiple ZDDs to a text stream.\n\n"
           "Args:\n"
           "    zdds: List of ZDD objects.\n"
           "    stream: A writable text stream.\n")
        .def_static("import_multi_file", [](py::object stream) -> std::vector<ZDD> {
            ensure_init();
            std::string s = py::cast<std::string>(stream.attr("read")());
            if (s.empty()) return {};
            std::istringstream iss(s);
            std::vector<bddp> v;
            int ret = bddimportz(iss, v);
            if (ret < 0) throw std::runtime_error("ZDD multi import failed");
            std::vector<ZDD> result;
            result.reserve(v.size());
            for (auto p : v) result.push_back(ZDD_ID(p));
            return result;
        }, py::arg("stream"),
           "Import multiple ZDDs from a text stream.\n\n"
           "Args:\n"
           "    stream: A readable text stream.\n\n"
           "Returns:\n"
           "    A list of ZDD objects.\n")

        .def("print_sets", [](const ZDD& z,
                              const std::string& delim1,
                              const std::string& delim2,
                              const std::vector<std::string>& var_name_map) -> std::string {
            std::ostringstream oss;
            z.print_sets(oss, delim1, delim2, var_name_map);
            return oss.str();
        }, py::arg("delim1") = "},{"
         , py::arg("delim2") = ","
         , py::arg("var_name_map") = std::vector<std::string>(),
           "Print the family of sets as a string with custom delimiters.\n\n"
           "When called with default arguments, sets are separated by '},{'\n"
           "and elements by ','. The output contains no outer braces;\n"
           "for example: '};{1};{2};{2,1'.\n\n"
           "Special cases:\n"
           "    - null ZDD: returns 'N'\n"
           "    - empty ZDD: returns 'E'\n\n"
           "Args:\n"
           "    delim1: Delimiter between sets (default: '},{').\n"
           "    delim2: Delimiter between elements within a set (default: ',').\n"
           "    var_name_map: List indexed by variable number for display names.\n"
           "        If provided and var_name_map[v] is non-empty, it is used\n"
           "        instead of the variable number.\n\n"
           "Returns:\n"
           "    The formatted string.\n")
        .def("to_str", [](const ZDD& z) -> std::string {
            std::ostringstream oss;
            z.print_sets(oss);
            return oss.str();
        }, "Print the family of sets in default format.\n\n"
           "Each set is enclosed in braces, elements separated by commas.\n"
           "Example: '{},{1},{2},{2,1}'\n\n"
           "Special cases:\n"
           "    - null ZDD: returns 'N'\n"
           "    - empty ZDD: returns 'E'\n\n"
           "Returns:\n"
           "    The formatted string.\n")

        .def_property_readonly("exact_count", [](const ZDD& z) -> py::int_ {
            bigint::BigInt bi = z.exact_count();
            std::string s = bi.to_string();
            return py::int_(py::str(s));
        }, "The number of sets in the family (arbitrary precision Python int).")
        .def_property_readonly("node_id", [](const ZDD& z) { return z.GetID(); },
             "The raw node ID of this ZDD.")
        .def_property_readonly("raw_size", [](const ZDD& z) { return z.raw_size(); },
             "The number of nodes in the DAG of this ZDD.")
        .def_property_readonly("plain_size", [](const ZDD& z) { return z.plain_size(); },
             "The number of nodes without complement edge sharing.")
        .def_property_readonly("lit", &ZDD::Lit,
             "The total literal count across all sets in the family.")
        .def_property_readonly("max_set_size", &ZDD::Len,
             "The maximum set size in the family.")
        .def_property_readonly("is_poly", &ZDD::IsPoly,
             "1 if the family has >= 2 sets, 0 otherwise.")
        .def_property_readonly("support", [](const ZDD& z) { return z.Support(); },
             "The support set as a ZDD.")
        .def_property_readonly("top_var", [](const ZDD& z) -> bddvar {
            return bddtop(z.GetID());
        }, "The top (root) variable number of this ZDD.")
        .def_property_readonly("is_terminal", &ZDD::is_terminal,
             "True if this is a terminal node.")
        .def_property_readonly("is_one", &ZDD::is_one,
             "True if this is the 1-terminal ({empty set}).")
        .def_property_readonly("is_zero", &ZDD::is_zero,
             "True if this is the 0-terminal (empty family).")

        // Conversion
        .def("to_qdd", &ZDD::to_qdd,
             "Convert to a Quasi-reduced Decision Diagram (QDD).\n\n"
             "Returns:\n"
             "    The QDD representation.\n")

        // Counting
        .def("count", &ZDD::count,
             "Count the number of sets in the family (floating-point).\n\n"
             "Returns:\n"
             "    The number of sets as a float.\n")
        .def("exact_count_with_memo", [](const ZDD& z, ZddCountMemo& memo) -> py::int_ {
            bigint::BigInt bi = z.exact_count(memo);
            std::string s = bi.to_string();
            return py::int_(py::str(s));
        }, py::arg("memo"),
           "Count the number of sets using a memo for caching.\n\n"
           "Args:\n"
           "    memo: A ZddCountMemo object for caching.\n\n"
           "Returns:\n"
           "    The number of sets as a Python int.\n")
        .def("uniform_sample", [](ZDD& z, uint64_t seed) -> std::vector<bddvar> {
            std::mt19937_64 rng(seed);
            ZddCountMemo memo(z);
            return z.uniform_sample(rng, memo);
        }, py::arg("seed") = 0,
           "Sample a set uniformly at random from the family.\n\n"
           "Args:\n"
           "    seed: Random seed (default: 0).\n\n"
           "Returns:\n"
           "    A list of variable numbers in the sampled set.\n")
        .def("uniform_sample_with_memo", [](ZDD& z, ZddCountMemo& memo, uint64_t seed) -> std::vector<bddvar> {
            std::mt19937_64 rng(seed);
            return z.uniform_sample(rng, memo);
        }, py::arg("memo"), py::arg("seed") = 0,
           "Sample a set using a memo for caching.\n\n"
           "Args:\n"
           "    memo: A ZddCountMemo object for caching.\n"
           "    seed: Random seed (default: 0).\n\n"
           "Returns:\n"
           "    A list of variable numbers in the sampled set.\n")

        // Enumeration
        .def("enumerate", &ZDD::enumerate,
             "Enumerate all sets in the family.\n\n"
             "Returns:\n"
             "    A list of sets, each a list of variable numbers.\n")
        .def("has_empty", &ZDD::has_empty,
             "Check if the empty set is a member of the family.\n\n"
             "Returns:\n"
             "    True if the family contains the empty set.\n")

        // Weight optimization
        .def("min_weight", &ZDD::min_weight, py::arg("weights"),
             "Find the minimum weight sum among all sets in the family.\n\n"
             "Args:\n"
             "    weights: A list of integer weights indexed by variable number.\n"
             "             Size must be > the top variable number of the ZDD.\n\n"
             "Returns:\n"
             "    The minimum weight sum (int).\n\n"
             "Raises:\n"
             "    ValueError: If the ZDD is null, the family is empty, or weights is too small.\n")
        .def("max_weight", &ZDD::max_weight, py::arg("weights"),
             "Find the maximum weight sum among all sets in the family.\n\n"
             "Args:\n"
             "    weights: A list of integer weights indexed by variable number.\n"
             "             Size must be > the top variable number of the ZDD.\n\n"
             "Returns:\n"
             "    The maximum weight sum (int).\n\n"
             "Raises:\n"
             "    ValueError: If the ZDD is null, the family is empty, or weights is too small.\n")
        .def("min_weight_set", &ZDD::min_weight_set, py::arg("weights"),
             "Find a set with the minimum weight sum.\n\n"
             "Args:\n"
             "    weights: A list of integer weights indexed by variable number.\n"
             "             Size must be > the top variable number of the ZDD.\n\n"
             "Returns:\n"
             "    A list of variable numbers forming a set with minimum weight sum.\n\n"
             "Raises:\n"
             "    ValueError: If the ZDD is null, the family is empty, or weights is too small.\n")
        .def("max_weight_set", &ZDD::max_weight_set, py::arg("weights"),
             "Find a set with the maximum weight sum.\n\n"
             "Args:\n"
             "    weights: A list of integer weights indexed by variable number.\n"
             "             Size must be > the top variable number of the ZDD.\n\n"
             "Returns:\n"
             "    A list of variable numbers forming a set with maximum weight sum.\n\n"
             "Raises:\n"
             "    ValueError: If the ZDD is null, the family is empty, or weights is too small.\n")

        // Weight iteration
        .def("iter_min_weight", [](const ZDD& z, const std::vector<int>& weights) {
            return ZddMinWeightRange(z, weights);
        }, py::arg("weights"), py::keep_alive<0, 1>(),
           "Iterate over sets in ascending weight order.\n\n"
           "Args:\n"
           "    weights: A list of integer weights indexed by variable number.\n"
           "             Size must be > the top variable number of the ZDD.\n\n"
           "Returns:\n"
           "    An iterator yielding (weight, set) pairs.\n\n"
           "Example:\n"
           "    for weight, s in zdd.iter_min_weight(weights):\n"
           "        print(weight, s)\n"
           "        if weight > threshold:\n"
           "            break\n")

        // Cost-bounded enumeration
        .def("cost_bound", [](const ZDD& z, const std::vector<int>& weights, long long b) -> ZDD {
            return z.cost_bound(weights, b);
        }, py::arg("weights"), py::arg("b"),
           "Extract all sets whose total cost is at most b.\n\n"
           "Returns a ZDD representing {X in F | Cost(X) <= b}, where\n"
           "Cost(X) = sum of weights[v] for v in X.\n\n"
           "Uses the BkTrk-IntervalMemo algorithm internally.\n\n"
           "Args:\n"
           "    weights: A list of integer costs indexed by variable number.\n"
           "             Size must be > the number of variables (var_used()).\n"
           "    b: Cost bound. Sets with total cost <= b are included.\n\n"
           "Returns:\n"
           "    A ZDD containing all cost-bounded sets.\n\n"
           "Raises:\n"
           "    ValueError: If the ZDD is null or weights is too small.\n")
        .def("cost_bound_with_memo", [](const ZDD& z, const std::vector<int>& weights,
                                         long long b, CostBoundMemo& memo) -> ZDD {
            return z.cost_bound(weights, b, memo);
        }, py::arg("weights"), py::arg("b"), py::arg("memo"),
           "Extract cost-bounded sets, reusing a memo for efficiency.\n\n"
           "The memo caches intermediate results using interval-memoizing.\n"
           "When calling cost_bound repeatedly with different bounds on\n"
           "the same ZDD and weights, passing a CostBoundMemo can be\n"
           "significantly faster.\n\n"
           "Args:\n"
           "    weights: A list of integer costs indexed by variable number.\n"
           "             Size must be > the number of variables (var_used()).\n"
           "    b: Cost bound. Sets with total cost <= b are included.\n"
           "    memo: A CostBoundMemo object for caching across calls.\n\n"
           "Returns:\n"
           "    A ZDD containing all cost-bounded sets.\n\n"
           "Raises:\n"
           "    ValueError: If the ZDD is null, weights is too small,\n"
           "                or a different weights vector was used with this memo.\n")

        // Static factory methods
        .def_static("singleton", [](bddvar v) -> ZDD {
            ensure_init();
            return ZDD::singleton(v);
        }, py::arg("v"),
           "Create the ZDD {{v}} (a family with one singleton set).\n\n"
           "Args:\n"
           "    v: Variable number.\n\n"
           "Returns:\n"
           "    A ZDD representing {{v}}.\n")
        .def_static("single_set", [](const std::vector<bddvar>& vars) -> ZDD {
            ensure_init();
            return ZDD::single_set(vars);
        }, py::arg("vars"),
           "Create the ZDD {{v1, v2, ...}} (a family with one set).\n\n"
           "Args:\n"
           "    vars: List of variable numbers.\n\n"
           "Returns:\n"
           "    A ZDD representing the single set.\n")
        .def_static("power_set", [](bddvar n) -> ZDD {
            ensure_init();
            return ZDD::power_set(n);
        }, py::arg("n"),
           "Create the power set of {1, ..., n}.\n\n"
           "Args:\n"
           "    n: Universe size.\n\n"
           "Returns:\n"
           "    A ZDD representing 2^{1,...,n}.\n")
        .def_static("power_set_vars", [](const std::vector<bddvar>& vars) -> ZDD {
            ensure_init();
            return ZDD::power_set(vars);
        }, py::arg("vars"),
           "Create the power set of the given variables.\n\n"
           "Args:\n"
           "    vars: List of variable numbers.\n\n"
           "Returns:\n"
           "    A ZDD representing the power set.\n")
        .def_static("from_sets", [](const std::vector<std::vector<bddvar>>& sets) -> ZDD {
            ensure_init();
            return ZDD::from_sets(sets);
        }, py::arg("sets"),
           "Construct a ZDD from a list of sets.\n\n"
           "Args:\n"
           "    sets: List of sets, each a list of variable numbers.\n\n"
           "Returns:\n"
           "    A ZDD representing the family of sets.\n")
        .def_static("combination", [](bddvar n, bddvar k) -> ZDD {
            ensure_init();
            return ZDD::combination(n, k);
        }, py::arg("n"), py::arg("k"),
           "Create the ZDD of all k-element subsets of {1, ..., n}.\n\n"
           "Args:\n"
           "    n: Universe size.\n"
           "    k: Subset size.\n\n"
           "Returns:\n"
           "    A ZDD representing C(n,k).\n")
        .def_static("random_family", [](bddvar n, uint64_t seed) -> ZDD {
            ensure_init();
            std::mt19937_64 rng(seed);
            return ZDD::random_family(n, rng);
        }, py::arg("n"), py::arg("seed") = 0,
           "Generate a uniformly random family over {1, ..., n}.\n\n"
           "Selects one of the 2^(2^n) possible families uniformly at random.\n\n"
           "Args:\n"
           "    n: Universe size.\n"
           "    seed: Random seed (default: 0).\n\n"
           "Returns:\n"
           "    A ZDD representing the randomly chosen family.\n")
        .def_static("getnode", [](bddvar var, const ZDD& lo, const ZDD& hi) -> ZDD {
            ensure_init();
            return ZDD::getnode(var, lo, hi);
        }, py::arg("var"), py::arg("lo"), py::arg("hi"),
           "Create a ZDD node with the given variable and children.\n\n"
           "Applies ZDD reduction rules (zero-suppression, complement normalization).\n\n"
           "Args:\n"
           "    var: Variable number.\n"
           "    lo: The low (0-edge) child.\n"
           "    hi: The high (1-edge) child.\n\n"
           "Returns:\n"
           "    The created ZDD node.\n")
        .def_static("cache_get", [](uint8_t op, const ZDD& f, const ZDD& g) -> ZDD {
            return ZDD::cache_get(op, f, g);
        }, py::arg("op"), py::arg("f"), py::arg("g"),
           "Read a 2-operand cache entry.\n\n"
           "Args:\n"
           "    op: Operation code (0-255).\n"
           "    f: First operand ZDD.\n"
           "    g: Second operand ZDD.\n\n"
           "Returns:\n"
           "    The cached ZDD result, or ZDD.null on miss.\n")
        .def_static("cache_put", [](uint8_t op, const ZDD& f, const ZDD& g, const ZDD& result) {
            ZDD::cache_put(op, f, g, result);
        }, py::arg("op"), py::arg("f"), py::arg("g"), py::arg("result"),
           "Write a 2-operand cache entry.\n\n"
           "Args:\n"
           "    op: Operation code (0-255).\n"
           "    f: First operand ZDD.\n"
           "    g: Second operand ZDD.\n"
           "    result: The result ZDD to cache.\n")
        .def_static("shared_size", [](const std::vector<ZDD>& v) -> uint64_t {
            return ZDD::raw_size(v);
        }, py::arg("zdds"),
           "Count the total number of shared nodes across multiple ZDDs.\n\n"
           "Args:\n"
           "    zdds: List of ZDD objects.\n\n"
           "Returns:\n"
           "    The number of distinct nodes (with complement sharing).\n")
        .def_static("shared_plain_size", [](const std::vector<ZDD>& v) -> uint64_t {
            return ZDD::plain_size(v);
        }, py::arg("zdds"),
           "Count the total number of nodes across multiple ZDDs without complement sharing.\n\n"
           "Args:\n"
           "    zdds: List of ZDD objects.\n\n"
           "Returns:\n"
           "    The number of nodes.\n")

        // Child accessors
        .def("child0", [](const ZDD& z) { return z.child0(); },
             "Get the 0-child (lo) with complement edge resolution (ZDD semantics).")
        .def("child1", [](const ZDD& z) { return z.child1(); },
             "Get the 1-child (hi) with complement edge resolution (ZDD semantics).")
        .def("child", [](const ZDD& z, int c) { return z.child(c); },
             py::arg("child"),
             "Get the child by index (0 or 1) with complement edge resolution (ZDD semantics).")
        .def("raw_child0", [](const ZDD& z) { return z.raw_child0(); },
             "Get the raw 0-child (lo) without complement resolution.")
        .def("raw_child1", [](const ZDD& z) { return z.raw_child1(); },
             "Get the raw 1-child (hi) without complement resolution.")
        .def("raw_child", [](const ZDD& z, int c) { return z.raw_child(c); },
             py::arg("child"),
             "Get the raw child by index (0 or 1) without complement resolution.")

        // Binary I/O
        .def("export_binary_str", [](const ZDD& z) -> py::bytes {
            std::ostringstream oss;
            z.export_binary(oss);
            return py::bytes(oss.str());
        }, "Export this ZDD in binary format to a bytes object.")
        .def_static("import_binary_str", [](const std::string& data, bool ignore_type) -> ZDD {
            ensure_init();
            std::istringstream iss(data);
            return ZDD::import_binary(iss, ignore_type);
        }, py::arg("data"), py::arg("ignore_type") = false,
           "Import a ZDD from binary format bytes.")
        .def("export_binary_file", [](const ZDD& z, py::object stream) {
            std::ostringstream oss;
            z.export_binary(oss);
            stream.attr("write")(py::bytes(oss.str()));
        }, py::arg("stream"),
           "Export this ZDD in binary format to a binary stream.")
        .def_static("import_binary_file", [](py::object stream, bool ignore_type) -> ZDD {
            ensure_init();
            std::string data = py::cast<std::string>(stream.attr("read")());
            std::istringstream iss(data);
            return ZDD::import_binary(iss, ignore_type);
        }, py::arg("stream"), py::arg("ignore_type") = false,
           "Import a ZDD from a binary stream.")

        // Multi-root binary I/O
        .def_static("export_binary_multi_str", [](const std::vector<ZDD>& zdds) -> py::bytes {
            std::ostringstream oss;
            ZDD::export_binary_multi(oss, zdds);
            return py::bytes(oss.str());
        }, py::arg("zdds"),
           "Export multiple ZDDs in binary format to a bytes object.")
        .def_static("import_binary_multi_str", [](const std::string& data, bool ignore_type) -> std::vector<ZDD> {
            ensure_init();
            std::istringstream iss(data);
            return ZDD::import_binary_multi(iss, ignore_type);
        }, py::arg("data"), py::arg("ignore_type") = false,
           "Import multiple ZDDs from binary format bytes.")
        .def_static("export_binary_multi_file", [](const std::vector<ZDD>& zdds, py::object stream) {
            std::ostringstream oss;
            ZDD::export_binary_multi(oss, zdds);
            stream.attr("write")(py::bytes(oss.str()));
        }, py::arg("zdds"), py::arg("stream"),
           "Export multiple ZDDs in binary format to a binary stream.")
        .def_static("import_binary_multi_file", [](py::object stream, bool ignore_type) -> std::vector<ZDD> {
            ensure_init();
            std::string data = py::cast<std::string>(stream.attr("read")());
            std::istringstream iss(data);
            return ZDD::import_binary_multi(iss, ignore_type);
        }, py::arg("stream"), py::arg("ignore_type") = false,
           "Import multiple ZDDs from a binary stream.")

        // Sapporo I/O
        .def("export_sapporo_str", [](const ZDD& z) -> std::string {
            std::ostringstream oss;
            z.export_sapporo(oss);
            return oss.str();
        }, "Export this ZDD in Sapporo format to a string.")
        .def_static("import_sapporo_str", [](const std::string& s) -> ZDD {
            ensure_init();
            std::istringstream iss(s);
            return ZDD::import_sapporo(iss);
        }, py::arg("s"),
           "Import a ZDD from a Sapporo format string.")
        .def("export_sapporo_file", [](const ZDD& z, py::object stream) {
            std::ostringstream oss;
            z.export_sapporo(oss);
            stream.attr("write")(oss.str());
        }, py::arg("stream"),
           "Export this ZDD in Sapporo format to a text stream.")
        .def_static("import_sapporo_file", [](py::object stream) -> ZDD {
            ensure_init();
            std::string s = py::cast<std::string>(stream.attr("read")());
            std::istringstream iss(s);
            return ZDD::import_sapporo(iss);
        }, py::arg("stream"),
           "Import a ZDD from a Sapporo format text stream.")

        // Graphillion I/O
        .def("export_graphillion_str", [](const ZDD& z, int offset) -> std::string {
            std::ostringstream oss;
            z.export_graphillion(oss, offset);
            return oss.str();
        }, py::arg("offset") = 0,
           "Export this ZDD in Graphillion format to a string.\n\n"
           "Args:\n"
           "    offset: Variable number offset (default: 0).\n\n"
           "Returns:\n"
           "    A Graphillion format string.\n")
        .def_static("import_graphillion_str", [](const std::string& s, int offset) -> ZDD {
            ensure_init();
            std::istringstream iss(s);
            return ZDD::import_graphillion(iss, offset);
        }, py::arg("s"), py::arg("offset") = 0,
           "Import a ZDD from a Graphillion format string.\n\n"
           "Args:\n"
           "    s: The Graphillion format string.\n"
           "    offset: Variable number offset (default: 0).\n\n"
           "Returns:\n"
           "    The reconstructed ZDD.\n")
        .def("export_graphillion_file", [](const ZDD& z, py::object stream, int offset) {
            std::ostringstream oss;
            z.export_graphillion(oss, offset);
            stream.attr("write")(oss.str());
        }, py::arg("stream"), py::arg("offset") = 0,
           "Export this ZDD in Graphillion format to a text stream.\n\n"
           "Args:\n"
           "    stream: A writable text stream.\n"
           "    offset: Variable number offset (default: 0).\n")
        .def_static("import_graphillion_file", [](py::object stream, int offset) -> ZDD {
            ensure_init();
            std::string s = py::cast<std::string>(stream.attr("read")());
            std::istringstream iss(s);
            return ZDD::import_graphillion(iss, offset);
        }, py::arg("stream"), py::arg("offset") = 0,
           "Import a ZDD from a Graphillion format text stream.\n\n"
           "Args:\n"
           "    stream: A readable text stream.\n"
           "    offset: Variable number offset (default: 0).\n\n"
           "Returns:\n"
           "    The reconstructed ZDD.\n")

        // Knuth I/O (deprecated)
        .def("export_knuth_str", [](const ZDD& z, bool is_hex, int offset) -> std::string {
            std::ostringstream oss;
            z.export_knuth(oss, is_hex, offset);
            return oss.str();
        }, py::arg("is_hex") = false, py::arg("offset") = 0,
           "Export this ZDD in Knuth format to a string (deprecated).\n\n"
           "Args:\n"
           "    is_hex: If True, use hexadecimal node IDs.\n"
           "    offset: Variable number offset (default: 0).\n\n"
           "Returns:\n"
           "    A Knuth format string.\n\n"
           ".. deprecated:: Use export_sapporo_str() or export_binary_str() instead.\n")
        .def_static("import_knuth_str", [](const std::string& s, bool is_hex, int offset) -> ZDD {
            ensure_init();
            std::istringstream iss(s);
            return ZDD::import_knuth(iss, is_hex, offset);
        }, py::arg("s"), py::arg("is_hex") = false, py::arg("offset") = 0,
           "Import a ZDD from a Knuth format string (deprecated).\n\n"
           "Args:\n"
           "    s: The Knuth format string.\n"
           "    is_hex: If True, use hexadecimal node IDs.\n"
           "    offset: Variable number offset (default: 0).\n\n"
           "Returns:\n"
           "    The reconstructed ZDD.\n\n"
           ".. deprecated:: Use import_sapporo_str() or import_binary_str() instead.\n")
        .def("export_knuth_file", [](const ZDD& z, py::object stream, bool is_hex, int offset) {
            std::ostringstream oss;
            z.export_knuth(oss, is_hex, offset);
            stream.attr("write")(oss.str());
        }, py::arg("stream"), py::arg("is_hex") = false, py::arg("offset") = 0,
           "Export this ZDD in Knuth format to a text stream (deprecated).\n\n"
           "Args:\n"
           "    stream: A writable text stream.\n"
           "    is_hex: If True, use hexadecimal node IDs.\n"
           "    offset: Variable number offset (default: 0).\n\n"
           ".. deprecated:: Use export_sapporo_file() or export_binary_file() instead.\n")
        .def_static("import_knuth_file", [](py::object stream, bool is_hex, int offset) -> ZDD {
            ensure_init();
            std::string s = py::cast<std::string>(stream.attr("read")());
            std::istringstream iss(s);
            return ZDD::import_knuth(iss, is_hex, offset);
        }, py::arg("stream"), py::arg("is_hex") = false, py::arg("offset") = 0,
           "Import a ZDD from a Knuth format text stream (deprecated).\n\n"
           "Args:\n"
           "    stream: A readable text stream.\n"
           "    is_hex: If True, use hexadecimal node IDs.\n"
           "    offset: Variable number offset (default: 0).\n\n"
           "Returns:\n"
           "    The reconstructed ZDD.\n\n"
           ".. deprecated:: Use import_sapporo_file() or import_binary_file() instead.\n")

        // Graphviz
        .def("save_graphviz_str", [](const ZDD& z, bool raw) -> std::string {
            std::ostringstream oss;
            z.save_graphviz(oss, raw ? GraphvizMode::Raw : GraphvizMode::Expanded);
            return oss.str();
        }, py::arg("raw") = false,
           "Export this ZDD as a Graphviz DOT string.\n\n"
           "Args:\n"
           "    raw: If True, show physical DAG with complement markers.\n"
           "         If False (default), expand complement edges into full nodes.\n\n"
           "Returns:\n"
           "    A DOT format string.\n")
        .def("save_graphviz_file", [](const ZDD& z, py::object stream, bool raw) {
            std::ostringstream oss;
            z.save_graphviz(oss, raw ? GraphvizMode::Raw : GraphvizMode::Expanded);
            stream.attr("write")(oss.str());
        }, py::arg("stream"), py::arg("raw") = false,
           "Export this ZDD as a Graphviz DOT to a text stream.\n\n"
           "Args:\n"
           "    stream: A writable text stream.\n"
           "    raw: If True, show physical DAG with complement markers.\n"
           "         If False (default), expand complement edges into full nodes.\n")

        .def("print", [](const ZDD& z) -> std::string {
            std::ostringstream oss;
            CoutRedirectGuard guard(oss.rdbuf());
            z.Print();
            return oss.str();
        }, "Print ZDD statistics (ID, Var, Size, Card, Lit, Len) and return as string.")
    ;

    // ================================================================
    // PiDD class
    // ================================================================

    m.def("pidd_newvar", []() -> int {
        ensure_init();
        return PiDD_NewVar();
    }, "Create a new PiDD variable (extend permutation size by 1).\n\n"
       "Returns:\n"
       "    The new permutation size.\n");

    m.def("pidd_var_used", &PiDD_VarUsed,
       "Return the current PiDD permutation size.\n\n"
       "Returns:\n"
       "    The number of PiDD variables created so far.\n");

    py::class_<PiDD>(m, "PiDD",
        "A Permutation Decision Diagram based on adjacent transpositions.\n\n"
        "Represents a set of permutations using a ZDD, where each variable\n"
        "corresponds to an adjacent transposition Swap(x, y).")
        .def(py::init([](int val) {
            ensure_init();
            return PiDD(val);
        }), py::arg("val") = 0,
           "Construct a PiDD from an integer value.\n\n"
           "Args:\n"
           "    val: 0 for empty set, 1 for {identity}, negative for null.\n")
        .def(py::init([](const ZDD& z) {
            return PiDD(z);
        }), py::arg("zdd"),
           "Construct a PiDD from an existing ZDD.\n\n"
           "Args:\n"
           "    zdd: A ZDD to interpret as a permutation set.\n")

        .def("__eq__", [](const PiDD& a, const PiDD& b) { return a == b; },
             "Equality comparison.")
        .def("__ne__", [](const PiDD& a, const PiDD& b) { return a != b; },
             "Inequality comparison.")
        .def("__hash__", [](const PiDD& a) {
            return std::hash<uint64_t>()(a.GetZDD().GetID());
        }, "Hash based on internal ZDD node ID.")
        .def("__repr__", [](const PiDD& a) {
            return "PiDD(node_id=" + std::to_string(a.GetZDD().GetID()) + ")";
        }, "Return string representation.")
        .def("__bool__", [](const PiDD&) -> bool {
            throw py::type_error(
                "PiDD cannot be converted to bool. "
                "Use == PiDD(0) or == PiDD(1) instead.");
        })

        // Set operations
        .def("__and__", [](const PiDD& a, const PiDD& b) { return a & b; },
             "Intersection: self & other.")
        .def("__add__", [](const PiDD& a, const PiDD& b) { return a + b; },
             "Union: self + other.")
        .def("__sub__", [](const PiDD& a, const PiDD& b) { return a - b; },
             "Difference: self - other.")
        .def("__mul__", [](const PiDD& a, const PiDD& b) { return a * b; },
             "Composition: self * other.")
        .def("__truediv__", [](const PiDD& a, const PiDD& b) { return a / b; },
             "Division: self / other.")
        .def("__mod__", [](const PiDD& a, const PiDD& b) { return a % b; },
             "Remainder: self %% other.")
        .def("__iand__", [](PiDD& a, const PiDD& b) -> PiDD& { a &= b; return a; },
             py::return_value_policy::reference_internal,
             "In-place intersection.")
        .def("__iadd__", [](PiDD& a, const PiDD& b) -> PiDD& { a += b; return a; },
             py::return_value_policy::reference_internal,
             "In-place union.")
        .def("__isub__", [](PiDD& a, const PiDD& b) -> PiDD& { a -= b; return a; },
             py::return_value_policy::reference_internal,
             "In-place difference.")
        .def("__imul__", [](PiDD& a, const PiDD& b) -> PiDD& { a *= b; return a; },
             py::return_value_policy::reference_internal,
             "In-place composition.")

        // Core operations
        .def("swap", &PiDD::Swap, py::arg("u"), py::arg("v"),
             "Apply transposition Swap(u, v) to all permutations in the set.\n\n"
             "Args:\n"
             "    u: First position.\n"
             "    v: Second position.\n\n"
             "Returns:\n"
             "    A new PiDD with the transposition applied.\n")
        .def("cofact", &PiDD::Cofact, py::arg("u"), py::arg("v"),
             "Extract permutations where position u has value v, removing position u.\n\n"
             "Args:\n"
             "    u: Position to check.\n"
             "    v: Required value at position u.\n\n"
             "Returns:\n"
             "    A PiDD of sub-permutations satisfying the condition.\n")
        .def("odd", &PiDD::Odd,
             "Extract odd permutations from the set.\n\n"
             "Returns:\n"
             "    A PiDD containing only odd permutations.\n")
        .def("even", &PiDD::Even,
             "Extract even permutations from the set.\n\n"
             "Returns:\n"
             "    A PiDD containing only even permutations.\n")
        .def("swap_bound", &PiDD::SwapBound, py::arg("n"),
             "Apply symmetric constraint (PermitSym) to the internal ZDD.\n\n"
             "Args:\n"
             "    n: Constraint parameter.\n\n"
             "Returns:\n"
             "    A PiDD with the constraint applied.\n")

        // Properties
        .def_property_readonly("top_x", &PiDD::TopX,
             "The x coordinate of the top variable.")
        .def_property_readonly("top_y", &PiDD::TopY,
             "The y coordinate of the top variable.")
        .def_property_readonly("top_lev", &PiDD::TopLev,
             "The BDD level of the top variable.")
        .def_property_readonly("size", &PiDD::Size,
             "The number of nodes in the internal ZDD.")
        .def_property_readonly("exact_count", [](const PiDD& p) -> py::int_ {
            bigint::BigInt bi = p.GetZDD().exact_count();
            std::string s = bi.to_string();
            return py::int_(py::str(s));
        }, "The number of permutations in the set (arbitrary precision Python int).")
        .def_property_readonly("zdd", &PiDD::GetZDD,
             "The internal ZDD representation.")

        .def("__itruediv__", [](PiDD& a, const PiDD& b) -> PiDD& { a /= b; return a; },
             py::return_value_policy::reference_internal,
             "In-place division.")
        .def("__imod__", [](PiDD& a, const PiDD& b) -> PiDD& { a %= b; return a; },
             py::return_value_policy::reference_internal,
             "In-place remainder.")

        .def("print", [](const PiDD& p) -> std::string {
            std::ostringstream oss;
            CoutRedirectGuard guard(oss.rdbuf());
            p.Print();
            return oss.str();
        }, "Print PiDD statistics and return as string.")
        .def("enum", [](const PiDD& p) -> std::string {
            std::ostringstream oss;
            CoutRedirectGuard guard(oss.rdbuf());
            p.Enum();
            return oss.str();
        }, "Enumerate all permutations and return as string.")
        .def("enum2", [](const PiDD& p) -> std::string {
            std::ostringstream oss;
            CoutRedirectGuard guard(oss.rdbuf());
            p.Enum2();
            return oss.str();
        }, "Enumerate all permutations (form 2) and return as string.")
    ;

    // ================================================================
    // RotPiDD class
    // ================================================================

    m.def("rotpidd_newvar", []() -> int {
        ensure_init();
        return RotPiDD_NewVar();
    }, "Create a new RotPiDD variable (extend permutation size by 1).\n\n"
       "Returns:\n"
       "    The new permutation size.\n");

    m.def("rotpidd_var_used", &RotPiDD_VarUsed,
       "Return the current RotPiDD permutation size.\n\n"
       "Returns:\n"
       "    The number of RotPiDD variables created so far.\n");

    m.def("rotpidd_from_perm", [](std::vector<int> v) -> RotPiDD {
        ensure_init();
        return RotPiDD::VECtoRotPiDD(v);
    }, py::arg("perm"),
       "Create a RotPiDD from a permutation vector.\n\n"
       "The vector is automatically normalized to [1..n].\n\n"
       "Args:\n"
       "    perm: A list of integers representing a permutation.\n\n"
       "Returns:\n"
       "    A RotPiDD containing the single permutation.\n");

    py::class_<RotPiDD>(m, "RotPiDD",
        "A Rotational Permutation Decision Diagram.\n\n"
        "Represents a set of permutations using a ZDD, where each variable\n"
        "corresponds to a left rotation LeftRot(x, y).")
        .def(py::init([](int val) {
            ensure_init();
            return RotPiDD(val);
        }), py::arg("val") = 0,
           "Construct a RotPiDD from an integer value.\n\n"
           "Args:\n"
           "    val: 0 for empty set, 1 for {identity}, negative for null.\n")
        .def(py::init([](const ZDD& z) {
            return RotPiDD(z);
        }), py::arg("zdd"),
           "Construct a RotPiDD from an existing ZDD.\n\n"
           "Args:\n"
           "    zdd: A ZDD to interpret as a permutation set.\n")

        .def("__eq__", [](const RotPiDD& a, const RotPiDD& b) { return a == b; },
             "Equality comparison.")
        .def("__ne__", [](const RotPiDD& a, const RotPiDD& b) { return a != b; },
             "Inequality comparison.")
        .def("__hash__", [](const RotPiDD& a) {
            return std::hash<uint64_t>()(a.GetZDD().GetID());
        }, "Hash based on internal ZDD node ID.")
        .def("__repr__", [](const RotPiDD& a) {
            return "RotPiDD(node_id=" + std::to_string(a.GetZDD().GetID()) + ")";
        }, "Return string representation.")
        .def("__bool__", [](const RotPiDD&) -> bool {
            throw py::type_error(
                "RotPiDD cannot be converted to bool. "
                "Use == RotPiDD(0) or == RotPiDD(1) instead.");
        })

        // Set operations
        .def("__and__", [](const RotPiDD& a, const RotPiDD& b) { return a & b; },
             "Intersection: self & other.")
        .def("__add__", [](const RotPiDD& a, const RotPiDD& b) { return a + b; },
             "Union: self + other.")
        .def("__sub__", [](const RotPiDD& a, const RotPiDD& b) { return a - b; },
             "Difference: self - other.")
        .def("__mul__", [](const RotPiDD& a, const RotPiDD& b) { return a * b; },
             "Composition: self * other.")
        .def("__iand__", [](RotPiDD& a, const RotPiDD& b) -> RotPiDD& { a &= b; return a; },
             py::return_value_policy::reference_internal,
             "In-place intersection.")
        .def("__iadd__", [](RotPiDD& a, const RotPiDD& b) -> RotPiDD& { a += b; return a; },
             py::return_value_policy::reference_internal,
             "In-place union.")
        .def("__isub__", [](RotPiDD& a, const RotPiDD& b) -> RotPiDD& { a -= b; return a; },
             py::return_value_policy::reference_internal,
             "In-place difference.")
        .def("__imul__", [](RotPiDD& a, const RotPiDD& b) -> RotPiDD& { a *= b; return a; },
             py::return_value_policy::reference_internal,
             "In-place composition.")

        // Core operations
        .def("left_rot", &RotPiDD::LeftRot, py::arg("u"), py::arg("v"),
             "Apply left rotation LeftRot(u, v) to all permutations.\n\n"
             "Left-rotates positions v, v+1, ..., u cyclically.\n\n"
             "Args:\n"
             "    u: Upper position (u > v for non-trivial rotation).\n"
             "    v: Lower position.\n\n"
             "Returns:\n"
             "    A new RotPiDD with the rotation applied.\n")
        .def("swap", &RotPiDD::Swap, py::arg("a"), py::arg("b"),
             "Swap positions a and b in all permutations.\n\n"
             "Args:\n"
             "    a: First position.\n"
             "    b: Second position.\n\n"
             "Returns:\n"
             "    A new RotPiDD with the swap applied.\n")
        .def("reverse", &RotPiDD::Reverse, py::arg("l"), py::arg("r"),
             "Reverse positions l..r in all permutations.\n\n"
             "Args:\n"
             "    l: Left position.\n"
             "    r: Right position.\n\n"
             "Returns:\n"
             "    A new RotPiDD with the reversal applied.\n")
        .def("cofact", &RotPiDD::Cofact, py::arg("u"), py::arg("v"),
             "Extract permutations where position u has value v.\n\n"
             "Args:\n"
             "    u: Position to check.\n"
             "    v: Required value at position u.\n\n"
             "Returns:\n"
             "    A RotPiDD of sub-permutations satisfying the condition.\n")
        .def("odd", &RotPiDD::Odd,
             "Extract odd permutations from the set.\n\n"
             "Returns:\n"
             "    A RotPiDD containing only odd permutations.\n")
        .def("even", &RotPiDD::Even,
             "Extract even permutations from the set.\n\n"
             "Returns:\n"
             "    A RotPiDD containing only even permutations.\n")
        .def("rot_bound", &RotPiDD::RotBound, py::arg("n"),
             "Apply symmetric constraint (PermitSym) to the internal ZDD.\n\n"
             "Args:\n"
             "    n: Constraint parameter.\n\n"
             "Returns:\n"
             "    A RotPiDD with the constraint applied.\n")
        .def("order", &RotPiDD::Order, py::arg("a"), py::arg("b"),
             "Extract permutations where pi(a) < pi(b).\n\n"
             "Args:\n"
             "    a: First position.\n"
             "    b: Second position.\n\n"
             "Returns:\n"
             "    A RotPiDD containing only permutations satisfying the order.\n")
        .def("inverse", &RotPiDD::Inverse,
             "Compute the inverse of each permutation in the set.\n\n"
             "Returns:\n"
             "    A RotPiDD containing the inverse permutations.\n")
        .def("insert", &RotPiDD::Insert, py::arg("p"), py::arg("v"),
             "Insert value v at position p in each permutation.\n\n"
             "Args:\n"
             "    p: Insertion position.\n"
             "    v: Value to insert.\n\n"
             "Returns:\n"
             "    A RotPiDD with the element inserted.\n")
        .def("remove_max", &RotPiDD::RemoveMax, py::arg("k"),
             "Remove variables with size >= k from each permutation.\n\n"
             "Args:\n"
             "    k: Threshold.\n\n"
             "Returns:\n"
             "    A RotPiDD with large variables removed.\n")
        .def("normalize", &RotPiDD::normalizeRotPiDD, py::arg("k"),
             "Remove variables with x > k by projecting them out.\n\n"
             "Args:\n"
             "    k: Upper bound for retained variables.\n\n"
             "Returns:\n"
             "    A normalized RotPiDD.\n")
        .def("extract_one", &RotPiDD::Extract_One,
             "Extract a single permutation from the set.\n\n"
             "Returns:\n"
             "    A RotPiDD containing exactly one permutation.\n")
        .def("to_perms", &RotPiDD::RotPiDDToVectorOfPerms,
             "Convert to a list of permutation vectors.\n\n"
             "Returns:\n"
             "    A list of lists, each representing a permutation as [1..n].\n")

        // Properties
        .def_property_readonly("top_x", &RotPiDD::TopX,
             "The x coordinate of the top variable.")
        .def_property_readonly("top_y", &RotPiDD::TopY,
             "The y coordinate of the top variable.")
        .def_property_readonly("top_lev", &RotPiDD::TopLev,
             "The BDD level of the top variable.")
        .def_property_readonly("size", &RotPiDD::Size,
             "The number of nodes in the internal ZDD.")
        .def_property_readonly("exact_count", [](const RotPiDD& p) -> py::int_ {
            bigint::BigInt bi = p.GetZDD().exact_count();
            std::string s = bi.to_string();
            return py::int_(py::str(s));
        }, "The number of permutations in the set (arbitrary precision Python int).")
        .def_property_readonly("zdd", &RotPiDD::GetZDD,
             "The internal ZDD representation.")

        .def("print", [](const RotPiDD& p) -> std::string {
            std::ostringstream oss;
            CoutRedirectGuard guard(oss.rdbuf());
            p.Print();
            return oss.str();
        }, "Print RotPiDD statistics and return as string.")
        .def("enum", [](const RotPiDD& p) -> std::string {
            std::ostringstream oss;
            CoutRedirectGuard guard(oss.rdbuf());
            p.Enum();
            return oss.str();
        }, "Enumerate all permutations and return as string.")
        .def("enum2", [](const RotPiDD& p) -> std::string {
            std::ostringstream oss;
            CoutRedirectGuard guard(oss.rdbuf());
            p.Enum2();
            return oss.str();
        }, "Enumerate all permutations (form 2) and return as string.")
        .def("contradiction_maximization",
            [](const RotPiDD& p, int n,
               const std::vector<std::vector<int>>& w) -> long long int {
            if (n < 0)
                throw std::invalid_argument(
                    "contradiction_maximization: n must be non-negative");
            size_t required = static_cast<size_t>(n) + 1;
            if (w.size() < required)
                throw std::invalid_argument(
                    "contradiction_maximization: w must have at least n+1 rows");
            for (size_t i = 0; i < required; ++i) {
                if (w[i].size() < required)
                    throw std::invalid_argument(
                        "contradiction_maximization: each row of w must have at least n+1 elements");
            }
            unsigned long long int used_set = 0;
            std::vector<int> unused_list;
            for (int i = 1; i <= n; ++i) {
                unused_list.push_back(i);
            }
            std::unordered_map<
                std::pair<bddp, unsigned long long int>,
                long long int,
                RotPiDD::hash_func
            > hash;
            return p.contradictionMaximization(used_set, unused_list, n, hash, w);
        }, py::arg("n"), py::arg("w"),
           "Run contradiction maximization algorithm.\n\n"
           "Args:\n"
           "    n: Permutation size.\n"
           "    w: Weight matrix (n+1 x n+1, 1-indexed).\n\n"
           "Returns:\n"
           "    The maximum contradiction value.\n")
    ;

    // ================================================================
    // QDD class
    // ================================================================

    py::class_<QDD>(m, "QDD",
        "A Quasi-reduced Decision Diagram.\n\n"
        "QDD does not apply the jump rule (lo == hi nodes are preserved).\n"
        "Every path from root to terminal visits every variable level exactly once.\n"
        "Uses BDD complement edge semantics.")
        .def(py::init([](int val) {
            ensure_init();
            return QDD(val);
        }), py::arg("val") = 0,
           "Construct a QDD from an integer value.\n\n"
           "Args:\n"
           "    val: 0 for false, 1 for true, negative for null.\n")

        .def_property_readonly_static("false_", [](py::object) -> QDD {
            return QDD::False;
        })
        .def_property_readonly_static("true_", [](py::object) -> QDD {
            return QDD::True;
        })
        .def_property_readonly_static("null", [](py::object) -> QDD {
            return QDD::Null;
        })

        .def("__eq__", [](const QDD& a, const QDD& b) { return a == b; },
             "Equality comparison by node ID.")
        .def("__ne__", [](const QDD& a, const QDD& b) { return a != b; },
             "Inequality comparison by node ID.")
        .def("__hash__", [](const QDD& a) {
            return std::hash<uint64_t>()(a.get_id());
        }, "Hash based on node ID.")
        .def("__repr__", [](const QDD& a) {
            return "QDD(node_id=" + std::to_string(a.get_id()) + ")";
        }, "Return string representation: QDD(node_id=...).")
        .def("__bool__", [](const QDD&) -> bool {
            throw py::type_error(
                "QDD cannot be converted to bool. "
                "Use == QDD.false_ or == QDD.true_ instead.");
        })

        // Operators
        .def("__invert__", [](const QDD& a) { return ~a; },
             "Complement (negate): ~self.")

        // Node creation
        .def_static("getnode", [](bddvar var, const QDD& lo, const QDD& hi) -> QDD {
            ensure_init();
            return QDD::getnode(var, lo, hi);
        }, py::arg("var"), py::arg("lo"), py::arg("hi"),
           "Create a QDD node with level validation.\n\n"
           "Children must be at the expected level (var's level - 1).\n\n"
           "Args:\n"
           "    var: Variable number.\n"
           "    lo: The low (0-edge) child.\n"
           "    hi: The high (1-edge) child.\n\n"
           "Returns:\n"
           "    The created QDD node.\n")
        .def_static("cache_get", [](uint8_t op, const QDD& f, const QDD& g) -> QDD {
            return QDD::cache_get(op, f, g);
        }, py::arg("op"), py::arg("f"), py::arg("g"),
           "Read a 2-operand cache entry.\n\n"
           "Args:\n"
           "    op: Operation code (0-255).\n"
           "    f: First operand QDD.\n"
           "    g: Second operand QDD.\n\n"
           "Returns:\n"
           "    The cached QDD result, or QDD.null on miss.\n")
        .def_static("cache_put", [](uint8_t op, const QDD& f, const QDD& g, const QDD& result) {
            QDD::cache_put(op, f, g, result);
        }, py::arg("op"), py::arg("f"), py::arg("g"), py::arg("result"),
           "Write a 2-operand cache entry.\n\n"
           "Args:\n"
           "    op: Operation code (0-255).\n"
           "    f: First operand QDD.\n"
           "    g: Second operand QDD.\n"
           "    result: The result QDD to cache.\n")

        // Child accessors
        .def("child0", [](const QDD& q) { return q.child0(); },
             "Get the 0-child (lo) with complement resolution.")
        .def("child1", [](const QDD& q) { return q.child1(); },
             "Get the 1-child (hi) with complement resolution.")
        .def("child", [](const QDD& q, int c) { return q.child(c); },
             py::arg("child"),
             "Get the child by index (0 or 1) with complement resolution.")
        .def("raw_child0", [](const QDD& q) { return q.raw_child0(); },
             "Get the raw 0-child (lo) without complement resolution.")
        .def("raw_child1", [](const QDD& q) { return q.raw_child1(); },
             "Get the raw 1-child (hi) without complement resolution.")
        .def("raw_child", [](const QDD& q, int c) { return q.raw_child(c); },
             py::arg("child"),
             "Get the raw child by index (0 or 1) without complement resolution.")

        // Conversion
        .def("to_bdd", &QDD::to_bdd,
             "Convert to a canonical BDD by applying jump rule.")
        .def("to_zdd", &QDD::to_zdd,
             "Convert to a canonical ZDD.")

        // Properties
        .def_property_readonly("node_id", [](const QDD& q) { return q.get_id(); },
             "The raw node ID of this QDD.")
        .def_property_readonly("is_terminal", &QDD::is_terminal,
             "True if this is a terminal node.")
        .def_property_readonly("is_one", &QDD::is_one,
             "True if this is the 1-terminal.")
        .def_property_readonly("is_zero", &QDD::is_zero,
             "True if this is the 0-terminal.")
        .def_property_readonly("top_var", [](const QDD& q) -> bddvar {
            return q.top();
        }, "The top (root) variable number of this QDD.")
        .def_property_readonly("raw_size", [](const QDD& q) { return q.raw_size(); },
             "The number of nodes in the DAG of this QDD.")

        // Binary I/O
        .def("export_binary_str", [](const QDD& q) -> py::bytes {
            std::ostringstream oss;
            q.export_binary(oss);
            return py::bytes(oss.str());
        }, "Export this QDD in binary format to a bytes object.")
        .def_static("import_binary_str", [](const std::string& data, bool ignore_type) -> QDD {
            ensure_init();
            std::istringstream iss(data);
            return QDD::import_binary(iss, ignore_type);
        }, py::arg("data"), py::arg("ignore_type") = false,
           "Import a QDD from binary format bytes.")
        .def("export_binary_file", [](const QDD& q, py::object stream) {
            std::ostringstream oss;
            q.export_binary(oss);
            stream.attr("write")(py::bytes(oss.str()));
        }, py::arg("stream"),
           "Export this QDD in binary format to a binary stream.")
        .def_static("import_binary_file", [](py::object stream, bool ignore_type) -> QDD {
            ensure_init();
            std::string data = py::cast<std::string>(stream.attr("read")());
            std::istringstream iss(data);
            return QDD::import_binary(iss, ignore_type);
        }, py::arg("stream"), py::arg("ignore_type") = false,
           "Import a QDD from a binary stream.")

        // Multi-root binary I/O
        .def_static("export_binary_multi_str", [](const std::vector<QDD>& qdds) -> py::bytes {
            std::ostringstream oss;
            QDD::export_binary_multi(oss, qdds);
            return py::bytes(oss.str());
        }, py::arg("qdds"),
           "Export multiple QDDs in binary format to a bytes object.")
        .def_static("import_binary_multi_str", [](const std::string& data, bool ignore_type) -> std::vector<QDD> {
            ensure_init();
            std::istringstream iss(data);
            return QDD::import_binary_multi(iss, ignore_type);
        }, py::arg("data"), py::arg("ignore_type") = false,
           "Import multiple QDDs from binary format bytes.")
        .def_static("export_binary_multi_file", [](const std::vector<QDD>& qdds, py::object stream) {
            std::ostringstream oss;
            QDD::export_binary_multi(oss, qdds);
            stream.attr("write")(py::bytes(oss.str()));
        }, py::arg("qdds"), py::arg("stream"),
           "Export multiple QDDs in binary format to a binary stream.")
        .def_static("import_binary_multi_file", [](py::object stream, bool ignore_type) -> std::vector<QDD> {
            ensure_init();
            std::string data = py::cast<std::string>(stream.attr("read")());
            std::istringstream iss(data);
            return QDD::import_binary_multi(iss, ignore_type);
        }, py::arg("stream"), py::arg("ignore_type") = false,
           "Import multiple QDDs from a binary stream.")
    ;

    // ================================================================
    // UnreducedDD class
    // ================================================================

    py::class_<UnreducedDD>(m, "UnreducedDD",
        "A type-agnostic unreduced Decision Diagram.\n\n"
        "Does NOT apply any reduction rules at node creation time.\n"
        "Complement edges are stored raw and only gain meaning\n"
        "when reduce_as_bdd(), reduce_as_zdd(), or reduce_as_qdd() is called.")
        .def(py::init([](int val) {
            ensure_init();
            return UnreducedDD(val);
        }), py::arg("val") = 0,
           "Construct an UnreducedDD from an integer value.\n\n"
           "Args:\n"
           "    val: 0 for 0-terminal, 1 for 1-terminal, negative for null.\n")

        // Constructors from other DD types (complement expansion)
        .def(py::init([](const BDD& bdd) {
            return UnreducedDD(bdd);
        }), py::arg("bdd"),
           "Convert a BDD to an UnreducedDD with complement expansion.\n\n"
           "Recursively expands all complement edges using BDD semantics.\n")
        .def(py::init([](const ZDD& zdd) {
            return UnreducedDD(zdd);
        }), py::arg("zdd"),
           "Convert a ZDD to an UnreducedDD with complement expansion.\n\n"
           "Recursively expands all complement edges using ZDD semantics.\n")
        .def(py::init([](const QDD& qdd) {
            return UnreducedDD(qdd);
        }), py::arg("qdd"),
           "Convert a QDD to an UnreducedDD with complement expansion.\n\n"
           "Uses BDD complement semantics.\n")

        .def("__eq__", [](const UnreducedDD& a, const UnreducedDD& b) { return a == b; },
             "Equality comparison by node ID (not semantic equality).")
        .def("__ne__", [](const UnreducedDD& a, const UnreducedDD& b) { return a != b; },
             "Inequality comparison by node ID.")
        .def("__lt__", [](const UnreducedDD& a, const UnreducedDD& b) { return a < b; },
             "Less-than by node ID (for ordered containers).")
        .def("__hash__", [](const UnreducedDD& a) {
            return std::hash<uint64_t>()(a.get_id());
        }, "Hash based on node ID.")
        .def("__repr__", [](const UnreducedDD& a) {
            return "UnreducedDD(node_id=" + std::to_string(a.get_id()) + ")";
        }, "Return string representation: UnreducedDD(node_id=...).")
        .def("__bool__", [](const UnreducedDD&) -> bool {
            throw py::type_error(
                "UnreducedDD cannot be converted to bool. "
                "Use == UnreducedDD(0) or == UnreducedDD(1) instead.");
        })

        // Operators
        .def("__invert__", [](const UnreducedDD& a) { return ~a; },
             "Toggle complement bit (bit 0). O(1).\n\n"
             "The complement bit has no semantics in UnreducedDD;\n"
             "it is only interpreted at reduce time.")

        // Static factory functions
        .def_static("zero", []() -> UnreducedDD {
            ensure_init();
            return UnreducedDD::zero();
        }, "Return a 0-terminal UnreducedDD.")
        .def_static("one", []() -> UnreducedDD {
            ensure_init();
            return UnreducedDD::one();
        }, "Return a 1-terminal UnreducedDD.")

        // Node creation
        .def_static("getnode", [](bddvar var, const UnreducedDD& lo,
                                  const UnreducedDD& hi) -> UnreducedDD {
            ensure_init();
            return UnreducedDD::getnode(var, lo, hi);
        }, py::arg("var"), py::arg("lo"), py::arg("hi"),
           "Create an unreduced DD node.\n\n"
           "Always allocates a new node. No complement normalization,\n"
           "no reduction rules, no unique table insertion.\n\n"
           "Args:\n"
           "    var: Variable number.\n"
           "    lo: The low (0-edge) child.\n"
           "    hi: The high (1-edge) child.\n\n"
           "Returns:\n"
           "    The created UnreducedDD node.\n")

        // Raw wrap (complement edges preserved)
        .def_static("wrap_raw_bdd", [](const BDD& bdd) -> UnreducedDD {
            return UnreducedDD::wrap_raw(bdd);
        }, py::arg("bdd"),
           "Wrap a BDD's bddp directly without complement expansion.\n\n"
           "Only use reduce_as_bdd() on the result.\n")
        .def_static("wrap_raw_zdd", [](const ZDD& zdd) -> UnreducedDD {
            return UnreducedDD::wrap_raw(zdd);
        }, py::arg("zdd"),
           "Wrap a ZDD's bddp directly without complement expansion.\n\n"
           "Only use reduce_as_zdd() on the result.\n")
        .def_static("wrap_raw_qdd", [](const QDD& qdd) -> UnreducedDD {
            return UnreducedDD::wrap_raw(qdd);
        }, py::arg("qdd"),
           "Wrap a QDD's bddp directly without complement expansion.\n\n"
           "Only use reduce_as_qdd() on the result.\n")

        // Child accessors (raw only)
        .def("raw_child0", [](const UnreducedDD& u) { return u.raw_child0(); },
             "Get the raw 0-child (lo) as an UnreducedDD.")
        .def("raw_child1", [](const UnreducedDD& u) { return u.raw_child1(); },
             "Get the raw 1-child (hi) as an UnreducedDD.")
        .def("raw_child", [](const UnreducedDD& u, int c) { return u.raw_child(c); },
             py::arg("child"),
             "Get the raw child by index (0 or 1) as an UnreducedDD.")

        // Child mutation (top-down construction)
        .def("set_child0", &UnreducedDD::set_child0, py::arg("child"),
             "Set the 0-child (lo) of this unreduced node.\n\n"
             "Only valid on unreduced, non-terminal, non-complemented nodes.\n")
        .def("set_child1", &UnreducedDD::set_child1, py::arg("child"),
             "Set the 1-child (hi) of this unreduced node.\n\n"
             "Only valid on unreduced, non-terminal, non-complemented nodes.\n")

        // Reduce
        .def("reduce_as_bdd", &UnreducedDD::reduce_as_bdd,
             "Reduce to a canonical BDD.\n\n"
             "Applies BDD complement semantics and jump rule.\n")
        .def("reduce_as_zdd", &UnreducedDD::reduce_as_zdd,
             "Reduce to a canonical ZDD.\n\n"
             "Applies ZDD complement semantics and zero-suppression rule.\n")
        .def("reduce_as_qdd", &UnreducedDD::reduce_as_qdd,
             "Reduce to a canonical QDD.\n\n"
             "Equivalent to reduce_as_bdd().to_qdd().\n")

        // Properties
        .def_property_readonly("node_id", [](const UnreducedDD& u) { return u.get_id(); },
             "The raw node ID of this UnreducedDD.")
        .def_property_readonly("is_terminal", &UnreducedDD::is_terminal,
             "True if this is a terminal node.")
        .def_property_readonly("is_one", &UnreducedDD::is_one,
             "True if this is the 1-terminal.")
        .def_property_readonly("is_zero", &UnreducedDD::is_zero,
             "True if this is the 0-terminal.")
        .def_property_readonly("is_reduced", &UnreducedDD::is_reduced,
             "True if this DD is fully reduced (canonical).")
        .def_property_readonly("top_var", [](const UnreducedDD& u) -> bddvar {
            return u.top();
        }, "The top (root) variable number.")
        .def_property_readonly("raw_size", [](const UnreducedDD& u) { return u.raw_size(); },
             "The number of nodes in the DAG.")

        // Binary I/O
        .def("export_binary_str", [](const UnreducedDD& u) -> py::bytes {
            std::ostringstream oss;
            u.export_binary(oss);
            return py::bytes(oss.str());
        }, "Export this UnreducedDD in binary format to a bytes object.")
        .def_static("import_binary_str", [](const std::string& data) -> UnreducedDD {
            ensure_init();
            std::istringstream iss(data);
            return UnreducedDD::import_binary(iss);
        }, py::arg("data"),
           "Import an UnreducedDD from binary format bytes.")
        .def("export_binary_file", [](const UnreducedDD& u, py::object stream) {
            std::ostringstream oss;
            u.export_binary(oss);
            stream.attr("write")(py::bytes(oss.str()));
        }, py::arg("stream"),
           "Export this UnreducedDD in binary format to a binary stream.")
        .def_static("import_binary_file", [](py::object stream) -> UnreducedDD {
            ensure_init();
            std::string data = py::cast<std::string>(stream.attr("read")());
            std::istringstream iss(data);
            return UnreducedDD::import_binary(iss);
        }, py::arg("stream"),
           "Import an UnreducedDD from a binary stream.")
    ;

    // ================================================================
    // SeqBDD class
    // ================================================================

    py::class_<SeqBDD>(m, "SeqBDD",
        "A Sequence BDD representing sets of sequences.\n\n"
        "Uses ZDD internally to compactly represent ordered sequences\n"
        "where the same symbol may appear multiple times.")
        .def(py::init([](int val) {
            ensure_init();
            return SeqBDD(val);
        }), py::arg("val") = 0,
           "Construct a SeqBDD.\n\n"
           "Args:\n"
           "    val: 0 for empty set, 1 for {epsilon}, negative for null.\n")
        .def(py::init([](const ZDD& z) {
            return SeqBDD(z);
        }), py::arg("zdd"),
           "Construct a SeqBDD from an existing ZDD.\n\n"
           "Args:\n"
           "    zdd: A ZDD to interpret as a sequence set.\n")

        .def("__eq__", [](const SeqBDD& a, const SeqBDD& b) { return a == b; })
        .def("__ne__", [](const SeqBDD& a, const SeqBDD& b) { return a != b; })
        .def("__hash__", [](const SeqBDD& a) {
            return std::hash<uint64_t>()(a.get_zdd().GetID());
        })
        .def("__repr__", [](const SeqBDD& a) {
            return "SeqBDD(node_id=" + std::to_string(a.get_zdd().GetID()) + ")";
        })
        .def("__str__", [](const SeqBDD& a) {
            return a.seq_str();
        })
        .def("__bool__", [](const SeqBDD&) -> bool {
            throw py::type_error(
                "SeqBDD cannot be converted to bool. "
                "Use == SeqBDD(0) or == SeqBDD(1) instead.");
        })

        // Set operations
        .def("__and__", [](const SeqBDD& a, const SeqBDD& b) { return a & b; },
             "Intersection: ``self & other``.")
        .def("__add__", [](const SeqBDD& a, const SeqBDD& b) { return a + b; },
             "Union: ``self + other``.")
        .def("__sub__", [](const SeqBDD& a, const SeqBDD& b) { return a - b; },
             "Difference: ``self - other``.")
        .def("__mul__", [](const SeqBDD& a, const SeqBDD& b) { return a * b; },
             "Concatenation: ``self * other``.")
        .def("__truediv__", [](const SeqBDD& a, const SeqBDD& b) { return a / b; },
             "Left quotient: ``self / other``.")
        .def("__mod__", [](const SeqBDD& a, const SeqBDD& b) { return a % b; },
             "Left remainder: ``self % other``.")

        // In-place operations
        .def("__iand__", [](SeqBDD& a, const SeqBDD& b) -> SeqBDD& { a &= b; return a; },
             py::return_value_policy::reference_internal)
        .def("__iadd__", [](SeqBDD& a, const SeqBDD& b) -> SeqBDD& { a += b; return a; },
             py::return_value_policy::reference_internal)
        .def("__isub__", [](SeqBDD& a, const SeqBDD& b) -> SeqBDD& { a -= b; return a; },
             py::return_value_policy::reference_internal)
        .def("__imul__", [](SeqBDD& a, const SeqBDD& b) -> SeqBDD& { a *= b; return a; },
             py::return_value_policy::reference_internal)
        .def("__itruediv__", [](SeqBDD& a, const SeqBDD& b) -> SeqBDD& { a /= b; return a; },
             py::return_value_policy::reference_internal)
        .def("__imod__", [](SeqBDD& a, const SeqBDD& b) -> SeqBDD& { a %= b; return a; },
             py::return_value_policy::reference_internal)

        // Node operations
        .def("off_set", &SeqBDD::off_set, py::arg("v"),
             "Remove sequences starting with variable v.\n\n"
             "Args:\n"
             "    v: Variable number.\n\n"
             "Returns:\n"
             "    A SeqBDD without sequences starting with v.\n")
        .def("on_set0", &SeqBDD::on_set0, py::arg("v"),
             "Extract sequences starting with v, removing the leading v.\n\n"
             "Args:\n"
             "    v: Variable number.\n\n"
             "Returns:\n"
             "    A SeqBDD of suffixes after stripping the leading v.\n")
        .def("on_set", &SeqBDD::on_set, py::arg("v"),
             "Extract sequences starting with v, keeping v.\n\n"
             "Args:\n"
             "    v: Variable number.\n\n"
             "Returns:\n"
             "    A SeqBDD of sequences that start with v.\n")
        .def("push", &SeqBDD::push, py::arg("v"),
             "Prepend variable v to all sequences.\n\n"
             "Args:\n"
             "    v: Variable number to prepend.\n\n"
             "Returns:\n"
             "    A SeqBDD with v prepended to every sequence.\n")

        // Properties
        .def_property_readonly("top", &SeqBDD::top,
             "The variable number of the root node.")
        .def_property_readonly("size", &SeqBDD::size,
             "The number of nodes in the internal ZDD.")
        .def_property_readonly("lit", &SeqBDD::lit,
             "Total symbol count across all sequences.")
        .def_property_readonly("len", &SeqBDD::len,
             "Length of the longest sequence.")
        .def_property_readonly("exact_count", [](const SeqBDD& s) -> py::int_ {
            bigint::BigInt bi = s.get_zdd().exact_count();
            std::string s_str = bi.to_string();
            return py::int_(py::str(s_str));
        }, "The number of sequences in the set (arbitrary precision Python int).")
        .def_property_readonly("zdd", &SeqBDD::get_zdd,
             "The internal ZDD representation.")

        // Static methods
        .def_static("from_list", [](std::vector<int> vars) -> SeqBDD {
            ensure_init();
            SeqBDD result(1);
            for (auto it = vars.rbegin(); it != vars.rend(); ++it) {
                result = result.push(*it);
            }
            return result;
        }, py::arg("vars"),
           "Create a SeqBDD representing a single sequence.\n\n"
           "Args:\n"
           "    vars: List of variable numbers for the sequence.\n\n"
           "Returns:\n"
           "    A SeqBDD containing the single sequence.\n")

        .def("export_str", [](const SeqBDD& s) -> std::string {
            std::ostringstream oss;
            s.export_to(oss);
            return oss.str();
        }, "Export the internal ZDD in Sapporo format to a string.")
        .def("export_file", [](const SeqBDD& s, py::object stream) {
            std::ostringstream oss;
            s.export_to(oss);
            stream.attr("write")(oss.str());
        }, py::arg("stream"),
           "Export the internal ZDD in Sapporo format to a text stream.\n\n"
           "Args:\n"
           "    stream: A writable text stream.\n")
        .def("print_seq", [](const SeqBDD& s) -> std::string {
            std::ostringstream oss;
            CoutRedirectGuard guard(oss.rdbuf());
            s.print_seq();
            return oss.str();
        }, "Print all sequences and return as string.")
        .def("seq_str", [](const SeqBDD& s) -> std::string {
            return s.seq_str();
        }, "Get all sequences as a string.\n\n"
           "Equivalent to str(self).\n\n"
           "Returns:\n"
           "    A string representation of all sequences.\n")
    ;

    // ZddMinWeightRange Python iterator
    py::class_<ZddMinWeightRange>(m, "_ZddMinWeightRange")
        .def("__iter__", [](ZddMinWeightRange& r) {
            return r.begin();
        })
    ;

    py::class_<ZddMinWeightIterator>(m, "_ZddMinWeightIterator")
        .def("__iter__", [](ZddMinWeightIterator& it) -> ZddMinWeightIterator& {
            return it;
        })
        .def("__next__", [](ZddMinWeightIterator& it) {
            ZddMinWeightIterator end;
            if (it == end) throw py::stop_iteration();
            auto val = *it;
            ++it;
            return val;
        })
    ;
}
