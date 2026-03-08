#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <sstream>
#include <fstream>
#include "bdd.h"

namespace py = pybind11;

static bool g_initialized = false;

static void ensure_init() {
    if (!g_initialized) {
        bddinit(256, UINT64_MAX);
        g_initialized = true;
    }
}

PYBIND11_MODULE(_core, m) {
    m.doc() = "KyotoDD BDD/ZDD library";

    // Exception translation
    py::register_exception_translator([](std::exception_ptr p) {
        try {
            if (p) std::rethrow_exception(p);
        } catch (const std::overflow_error& e) {
            PyErr_SetString(PyExc_MemoryError, e.what());
        } catch (const std::invalid_argument& e) {
            PyErr_SetString(PyExc_ValueError, e.what());
        } catch (const std::out_of_range& e) {
            PyErr_SetString(PyExc_IndexError, e.what());
        }
    });

    // Initialization
    m.def("init", [](uint64_t node_count, uint64_t node_max) {
        bddinit(node_count, node_max);
        g_initialized = true;
    }, py::arg("node_count") = 256, py::arg("node_max") = UINT64_MAX,
       "Initialize the BDD library.\n\n"
       "Args:\n"
       "    node_count: Initial number of node slots to allocate (default: 256).\n"
       "    node_max: Maximum number of node slots allowed (default: unlimited).\n");

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
       "Return the total number of nodes currently allocated.\n\n"
       "Returns:\n"
       "    The node count.\n");

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
            return std::hash<uint64_t>()(a.root);
        }, "Hash based on node ID.")
        .def("__repr__", [](const BDD& a) {
            return "BDD(node_id=" + std::to_string(a.root) + ")";
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
             "Return the support set as a BDD (conjunction of variables).")
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
            bddp p = b.root;
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
            bddp p = b.root;
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

        .def_property_readonly("node_id", [](const BDD& b) { return b.root; },
             "The raw node ID of this BDD.")
        .def_property_readonly("size", &BDD::Size,
             "The number of nodes in the DAG of this BDD.")
        .def_property_readonly("top_var", [](const BDD& b) -> bddvar {
            return bddtop(b.root);
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
            return std::hash<uint64_t>()(a.root);
        }, "Hash based on node ID.")
        .def("__repr__", [](const ZDD& a) {
            return "ZDD(node_id=" + std::to_string(a.root) + ")";
        }, "Return string representation: ZDD(node_id=...).")

        // Operators
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
             "Remainder: self %% other.")
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
             "Select sets containing variable v, then remove v.\n\n"
             "Args:\n"
             "    v: Variable number.\n\n"
             "Returns:\n"
             "    The resulting ZDD.\n")
        .def("onset0", &ZDD::OnSet0, py::arg("v"),
             "Select sets NOT containing variable v (same as offset).\n\n"
             "Args:\n"
             "    v: Variable number.\n\n"
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
             "For each pair (A, B), include A | B in the result\n"
             "regardless of overlap.\n\n"
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

        // I/O
        .def("export_str", [](const ZDD& z) -> std::string {
            std::ostringstream oss;
            bddp p = z.root;
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
            ZDD z(0);
            z.root = p;
            return z;
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
            bddp p = z.root;
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
            ZDD z(0);
            z.root = p;
            return z;
        }, py::arg("path"),
           "Import a ZDD from a file.\n\n"
           "Args:\n"
           "    path: File path to read from.\n\n"
           "Returns:\n"
           "    The reconstructed ZDD.\n\n"
           "Raises:\n"
           "    RuntimeError: If import fails or file cannot be opened.\n")

        .def_property_readonly("card", &ZDD::Card,
             "The number of sets in the family (cardinality).")
        .def_property_readonly("node_id", [](const ZDD& z) { return z.root; },
             "The raw node ID of this ZDD.")
        .def_property_readonly("top_var", [](const ZDD& z) -> bddvar {
            return bddtop(z.root);
        }, "The top (root) variable number of this ZDD.")
    ;
}
