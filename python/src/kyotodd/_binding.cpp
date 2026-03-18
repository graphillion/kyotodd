#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <sstream>
#include <fstream>
#include "bdd.h"
#include "pidd.h"
#include "rotpidd.h"

namespace py = pybind11;

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
        bddinit(node_count, node_max);
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
        g_initialized = false;
    }, "Finalize the BDD library and release all resources.\n\n"
       "Raises RuntimeError if called while BDD/ZDD objects exist.\n"
       "Safe to call multiple times when no objects are alive.\n");

    // Variable management
    m.def("newvar", []() -> bddvar {
        ensure_init();
        return bddnewvar();
    }, "Create a new variable and return its variable number.\n\n"
       "Returns:\n"
       "    The variable number of the newly created variable.\n");

    m.def("newvar_of_level", [](bddvar lev) -> bddvar {
        ensure_init();
        return bddnewvaroflev(lev);
    }, py::arg("lev"),
       "Create a new variable at the specified level.\n\n"
       "Args:\n"
       "    lev: The level to insert the new variable at.\n\n"
       "Returns:\n"
       "    The variable number of the newly created variable.\n");

    m.def("var_count", &bddvarused,
       "Return the number of variables created so far.\n\n"
       "Returns:\n"
       "    The count of variables.\n");

    m.def("level_of_var", &bddlevofvar, py::arg("var"),
       "Return the level of the given variable.\n\n"
       "Args:\n"
       "    var: Variable number.\n\n"
       "Returns:\n"
       "    The level of the variable.\n");

    m.def("var_of_level", &bddvaroflev, py::arg("level"),
       "Return the variable at the given level.\n\n"
       "Args:\n"
       "    level: Level number.\n\n"
       "Returns:\n"
       "    The variable number at that level.\n");

    // Node stats
    m.def("node_count", &bddused,
       "Return the number of used node slots (including dead nodes awaiting GC).\n\n"
       "This is not the initial capacity or the array size -- it is the\n"
       "count of node slots that have been occupied at least once.\n\n"
       "Returns:\n"
       "    The used node slot count.\n");

    // GC API
    m.def("gc", &bddgc,
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

    m.def("zdd_random", [](int lev, int density) -> ZDD {
        ensure_init();
        return ZDD_Random(lev, density);
    }, py::arg("lev"), py::arg("density") = 50,
       "Generate a random ZDD over the lowest lev levels.\n\n"
       "Variables for levels 1..lev must have been created (via newvar())\n"
       "before calling this function.\n\n"
       "Args:\n"
       "    lev: Number of variable levels to use.\n"
       "    density: Probability (0-100) for each terminal to be 1 (default: 50).\n\n"
       "Returns:\n"
       "    A random ZDD.\n");

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
             py::arg("cube"), "Existential quantification by cube BDD.")
        .def("exist", [](const BDD& f, const std::vector<bddvar>& vars) { return f.Exist(vars); },
             py::arg("vars"), "Existential quantification by variable list.")
        .def("exist", [](const BDD& f, bddvar var) { return f.Exist(var); },
             py::arg("var"), "Existential quantification of a single variable.")
        .def("univ", [](const BDD& f, const BDD& cube) { return f.Univ(cube); },
             py::arg("cube"), "Universal quantification by cube BDD.")
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
        .def("export_file", [](const BDD& b, const std::string& path) {
            std::ofstream ofs(path);
            if (!ofs) throw std::runtime_error("Cannot open file: " + path);
            bddp p = b.GetID();
            bddexport(ofs, &p, 1);
        }, py::arg("path"),
           "Export this BDD to a file.\n\n"
           "Args:\n"
           "    path: File path to write to.\n")
        .def_static("import_file", [](const std::string& path) -> BDD {
            ensure_init();
            std::ifstream ifs(path);
            if (!ifs) throw std::runtime_error("Cannot open file: " + path);
            bddp p;
            int ret = bddimport(ifs, &p, 1);
            if (ret < 1) throw std::runtime_error("BDD import failed");
            return BDD_ID(p);
        }, py::arg("path"),
           "Import a BDD from a file.\n\n"
           "Args:\n"
           "    path: File path to read from.\n\n"
           "Returns:\n"
           "    The reconstructed BDD.\n\n"
           "Raises:\n"
           "    RuntimeError: If import fails or file cannot be opened.\n")

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
        .def("__lshift__", [](const ZDD& a, int s) { return a << s; },
             "Left shift: increase variable numbers by s.")
        .def("__rshift__", [](const ZDD& a, int s) { return a >> s; },
             "Right shift: decrease variable numbers by s.")
        .def("__ilshift__", [](ZDD& a, int s) -> ZDD& { a <<= s; return a; },
             py::return_value_policy::reference_internal,
             "In-place left shift.")
        .def("__irshift__", [](ZDD& a, int s) -> ZDD& { a >>= s; return a; },
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
        .def("export_file", [](const ZDD& z, const std::string& path) {
            std::ofstream ofs(path);
            if (!ofs) throw std::runtime_error("Cannot open file: " + path);
            bddp p = z.GetID();
            bddexport(ofs, &p, 1);
        }, py::arg("path"),
           "Export this ZDD to a file.\n\n"
           "Args:\n"
           "    path: File path to write to.\n")
        .def_static("import_file", [](const std::string& path) -> ZDD {
            ensure_init();
            std::ifstream ifs(path);
            if (!ifs) throw std::runtime_error("Cannot open file: " + path);
            bddp p;
            int ret = bddimportz(ifs, &p, 1);
            if (ret < 1) throw std::runtime_error("ZDD import failed");
            return ZDD_ID(p);
        }, py::arg("path"),
           "Import a ZDD from a file.\n\n"
           "Args:\n"
           "    path: File path to read from.\n\n"
           "Returns:\n"
           "    The reconstructed ZDD.\n\n"
           "Raises:\n"
           "    RuntimeError: If import fails or file cannot be opened.\n")

        .def_property_readonly("card", &ZDD::Card,
             "The number of sets in the family (cardinality).\n\n"
             ".. deprecated:: Use count or exact_count instead.")
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

        .def("__eq__", [](const PiDD& a, const PiDD& b) { return a == b; },
             "Equality comparison.")
        .def("__ne__", [](const PiDD& a, const PiDD& b) { return a != b; },
             "Inequality comparison.")
        .def("__hash__", [](const PiDD& a) {
            return std::hash<uint64_t>()(a.GetZDD().GetID());
        }, "Hash based on internal ZDD node ID.")
        .def("__repr__", [](const PiDD& a) {
            return "PiDD(card=" + std::to_string(a.Card()) + ")";
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
        .def_property_readonly("card", &PiDD::Card,
             "The number of permutations in the set.")
        .def_property_readonly("zdd", &PiDD::GetZDD,
             "The internal ZDD representation.")
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

        .def("__eq__", [](const RotPiDD& a, const RotPiDD& b) { return a == b; },
             "Equality comparison.")
        .def("__ne__", [](const RotPiDD& a, const RotPiDD& b) { return a != b; },
             "Inequality comparison.")
        .def("__hash__", [](const RotPiDD& a) {
            return std::hash<uint64_t>()(a.GetZDD().GetID());
        }, "Hash based on internal ZDD node ID.")
        .def("__repr__", [](const RotPiDD& a) {
            return "RotPiDD(card=" + std::to_string(a.Card()) + ")";
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
        .def_property_readonly("card", &RotPiDD::Card,
             "The number of permutations in the set.")
        .def_property_readonly("zdd", &RotPiDD::GetZDD,
             "The internal ZDD representation.")
    ;
}
