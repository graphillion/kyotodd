#include "bdd_io_common.h"
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace kyotodd {

// ---------------------------------------------------------------------------
// Sapporo format save/load (bddexport/bddimport/bddimportz) + dump helpers.
// Other formats live in:
//   - bdd_io_knuth.cpp       (Knuth format, deprecated)
//   - bdd_io_binary.cpp      (Binary format, single & multi-root)
//   - bdd_io_graphillion.cpp (Graphillion text format)
//   - bdd_io_graphviz.cpp    (Graphviz DOT export)
// Shared helpers (stream I/O, encode/decode LE, read_token/read_line, ...)
// live in src/bdd_io_common.h.
// ---------------------------------------------------------------------------

static void export_collect(bddp f, std::unordered_set<bddp>& visited,
                           std::vector<bddp>& order) {
    if (f == bddnull) return;
    if (f & BDD_CONST_FLAG) return;
    bddp root = f & ~BDD_COMP_FLAG;
    if (!visited.insert(root).second) return;
    // Iterative post-order DFS using explicit stack.
    // Stack entries: (node, expanded). When expanded=false, push children
    // first; when expanded=true, emit the node.
    std::vector<std::pair<bddp, bool>> stack;
    stack.push_back(std::make_pair(root, false));
    while (!stack.empty()) {
        auto& top = stack.back();
        if (top.second) {
            order.push_back(top.first);
            stack.pop_back();
            continue;
        }
        top.second = true;
        bddp node = top.first;
        bddp hi = node_hi(node);
        if (!(hi & BDD_CONST_FLAG) && hi != bddnull) {
            bddp hi_node = hi & ~BDD_COMP_FLAG;
            if (visited.insert(hi_node).second) {
                stack.push_back(std::make_pair(hi_node, false));
            }
        }
        bddp lo = node_lo(node);
        if (!(lo & BDD_CONST_FLAG) && lo != bddnull) {
            bddp lo_node = lo & ~BDD_COMP_FLAG;
            if (visited.insert(lo_node).second) {
                stack.push_back(std::make_pair(lo_node, false));
            }
        }
    }
}

// Format an arc value as a string for the export file.
// Terminal: "F" or "T". Non-terminal: node ID (even) or node ID | 1 (odd=negated).
static void export_arc_str(bddp arc, char* buf, size_t bufsize) {
    if (arc == bddfalse) {
        buf[0] = 'F'; buf[1] = '\0';
    } else if (arc == bddtrue) {
        buf[0] = 'T'; buf[1] = '\0';
    } else {
        // Non-terminal: complement bit (bit 0) maps directly to odd/even in the output.
        bddp node = arc & ~BDD_COMP_FLAG;
        bool comp = (arc & BDD_COMP_FLAG) != 0;
        uint64_t id = comp ? (node | 1) : node;
        std::snprintf(buf, bufsize, "%" PRIu64, id);
    }
}

template<typename Stream>
static void export_core(Stream& strm, const bddp* p, int lim) {
    if (lim <= 0 || !p || !stream_valid(strm)) return;

    // bddnull acts as a sentinel: stop at the first bddnull.
    int effective_lim = 0;
    while (effective_lim < lim && p[effective_lim] != bddnull) {
        effective_lim++;
    }
    if (effective_lim == 0) return;

    // Collect all nodes in post-order
    std::unordered_set<bddp> visited;
    std::vector<bddp> order;
    bddvar max_level = 0;
    for (int i = 0; i < effective_lim; i++) {
        export_collect(p[i], visited, order);
        if (!(p[i] & BDD_CONST_FLAG)) {
            bddvar v = node_var(p[i]);
            bddvar lev = var2level[v];
            if (lev > max_level) max_level = lev;
        }
    }
    for (size_t i = 0; i < order.size(); i++) {
        bddvar v = node_var(order[i]);
        bddvar lev = var2level[v];
        if (lev > max_level) max_level = lev;
    }

    char buf[128];
    // Header
    std::snprintf(buf, sizeof(buf), "_i %u\n", static_cast<unsigned>(max_level));
    write_str(strm, buf);
    std::snprintf(buf, sizeof(buf), "_o %d\n", effective_lim);
    write_str(strm, buf);
    std::snprintf(buf, sizeof(buf), "_n %u\n", static_cast<unsigned>(order.size()));
    write_str(strm, buf);

    // Node section
    char buf0[ARC_BUF_SIZE], buf1[ARC_BUF_SIZE];
    for (size_t i = 0; i < order.size(); i++) {
        bddp node = order[i];
        bddvar v = node_var(node);
        bddvar lev = var2level[v];
        bddp lo = node_lo(node);
        bddp hi = node_hi(node);
        export_arc_str(lo, buf0, sizeof(buf0));
        export_arc_str(hi, buf1, sizeof(buf1));
        std::snprintf(buf, sizeof(buf), "%" PRIu64 " %u %s %s\n",
                     static_cast<uint64_t>(node),
                     static_cast<unsigned>(lev), buf0, buf1);
        write_str(strm, buf);
    }

    // Root section
    for (int i = 0; i < effective_lim; i++) {
        if (p[i] == bddfalse) {
            write_str(strm, "F\n");
        } else if (p[i] == bddtrue) {
            write_str(strm, "T\n");
        } else {
            bddp node = p[i] & ~BDD_COMP_FLAG;
            bool comp = (p[i] & BDD_COMP_FLAG) != 0;
            uint64_t id = comp ? (node | 1) : node;
            std::snprintf(buf, sizeof(buf), "%" PRIu64 "\n", id);
            write_str(strm, buf);
        }
    }

    if (stream_has_error(strm)) {
        throw std::runtime_error("bddexport: write error");
    }
}

void bddexport(FILE* strm, const bddp* p, int lim) {
    export_core(strm, p, lim);
}

void bddexport(FILE* strm, const std::vector<bddp>& v) {
    int lim = v.size() > static_cast<size_t>(INT_MAX) ? INT_MAX : static_cast<int>(v.size());
    export_core(strm, v.data(), lim);
}

void bddexport(std::ostream& strm, const bddp* p, int lim) {
    export_core(strm, p, lim);
}

void bddexport(std::ostream& strm, const std::vector<bddp>& v) {
    int lim = v.size() > static_cast<size_t>(INT_MAX) ? INT_MAX : static_cast<int>(v.size());
    export_core(strm, v.data(), lim);
}

// --- bddimport / bddimportz ---

static bddp import_parse_arc(const char* s,
                              const std::unordered_map<uint64_t, bddp>& id_map) {
    if (s[0] == 'F' && s[1] == '\0') return bddfalse;
    if (s[0] == 'T' && s[1] == '\0') return bddtrue;
    char* endptr;
    uint64_t val = std::strtoull(s, &endptr, 10);
    if (endptr == s || *endptr != '\0') return bddnull;  // no digits or trailing garbage
    bool comp = (val & 1) != 0;
    uint64_t node_id = comp ? (val - 1) : val;
    std::unordered_map<uint64_t, bddp>::const_iterator it = id_map.find(node_id);
    if (it == id_map.end()) return bddnull;
    return comp ? bddnot(it->second) : it->second;
}

template<typename Stream>
static int import_core(Stream& strm, std::vector<bddp>& result,
                       import_nodefn_t make_node) {
    unsigned max_level, output_count, node_count;
    if (!read_tag_uint(strm, "_i", max_level)) return -1;
    if (!read_tag_uint(strm, "_o", output_count)) return -1;
    if (!read_tag_uint(strm, "_n", node_count)) return -1;

    if (max_level > IMPORT_MAX_LEVEL_LIMIT) return -1;
    if (node_count > IMPORT_MAX_NODE_COUNT) return -1;
    if (output_count > IMPORT_MAX_OUTPUT_COUNT) return -1;

    while (bddvarused() < max_level) bddnewvar();

    std::unordered_map<uint64_t, bddp> id_map;
    char buf0[ARC_BUF_SIZE], buf1[ARC_BUF_SIZE];
    for (unsigned i = 0; i < node_count; i++) {
        uint64_t old_id;
        unsigned level;
        if (!read_node_line(strm, old_id, level, buf0, buf1)) return -1;
        if (level < 1 || level > max_level) return -1;
        bddp lo = import_parse_arc(buf0, id_map);
        bddp hi = import_parse_arc(buf1, id_map);
        if (lo == bddnull || hi == bddnull) return -1;
        bddvar var = bddvaroflev(level);
        if (id_map.count(old_id)) return -1;  // duplicate node ID
        bddp new_node = make_node(var, lo, hi);
        id_map[old_id] = new_node;
    }

    result.resize(output_count);
    for (unsigned i = 0; i < output_count; i++) {
        char ref[ARC_BUF_SIZE];
        if (!read_token(strm, ref)) return -1;
        bddp r = import_parse_arc(ref, id_map);
        if (r == bddnull) return -1;
        result[i] = r;
    }

    return static_cast<int>(output_count);
}

int bddimport(FILE* strm, bddp* p, int lim) {
    if (lim <= 0 || !p) return 0;
    std::vector<bddp> result;
    int ret = import_core(strm, result, BDD::getnode_raw);
    if (ret < 0) return ret;
    int count = ret < lim ? ret : lim;
    for (int i = 0; i < count; i++) p[i] = result[i];
    return count;
}

int bddimport(FILE* strm, std::vector<bddp>& v) {
    return import_core(strm, v, BDD::getnode_raw);
}

int bddimport(std::istream& strm, bddp* p, int lim) {
    if (lim <= 0 || !p) return 0;
    std::vector<bddp> result;
    int ret = import_core(strm, result, BDD::getnode_raw);
    if (ret < 0) return ret;
    int count = ret < lim ? ret : lim;
    for (int i = 0; i < count; i++) p[i] = result[i];
    return count;
}

int bddimport(std::istream& strm, std::vector<bddp>& v) {
    return import_core(strm, v, BDD::getnode_raw);
}

int bddimportz(FILE* strm, bddp* p, int lim) {
    if (lim <= 0 || !p) return 0;
    std::vector<bddp> result;
    int ret = import_core(strm, result, ZDD::getnode_raw);
    if (ret < 0) return ret;
    int count = ret < lim ? ret : lim;
    for (int i = 0; i < count; i++) p[i] = result[i];
    return count;
}

int bddimportz(FILE* strm, std::vector<bddp>& v) {
    return import_core(strm, v, ZDD::getnode_raw);
}

int bddimportz(std::istream& strm, bddp* p, int lim) {
    if (lim <= 0 || !p) return 0;
    std::vector<bddp> result;
    int ret = import_core(strm, result, ZDD::getnode_raw);
    if (ret < 0) return ret;
    int count = ret < lim ? ret : lim;
    for (int i = 0; i < count; i++) p[i] = result[i];
    return count;
}

int bddimportz(std::istream& strm, std::vector<bddp>& v) {
    return import_core(strm, v, ZDD::getnode_raw);
}

// --- ZDD_Import ---

ZDD ZDD_Import(FILE* strm) {
    bddp p = bddnull;
    int ret = bddimportz(strm, &p, 1);
    if (ret < 0) {
        throw std::runtime_error("ZDD_Import: failed to parse input");
    }
    if (ret == 0) return ZDD_ID(bddempty);
    return ZDD_ID(p);
}

int ZDD_Import(FILE* strm, std::vector<ZDD>& v) {
    std::vector<bddp> raw;
    int ret = bddimportz(strm, raw);
    if (ret < 0) return ret;
    v.clear();
    for (int i = 0; i < ret; i++) v.push_back(ZDD_ID(raw[i]));
    return ret;
}

ZDD ZDD_Import(std::istream& strm) {
    bddp p = bddnull;
    int ret = bddimportz(strm, &p, 1);
    if (ret < 0) {
        throw std::runtime_error("ZDD_Import: failed to parse input");
    }
    if (ret == 0) return ZDD_ID(bddempty);
    return ZDD_ID(p);
}

int ZDD_Import(std::istream& strm, std::vector<ZDD>& v) {
    std::vector<bddp> raw;
    int ret = bddimportz(strm, raw);
    if (ret < 0) return ret;
    v.clear();
    for (int i = 0; i < ret; i++) v.push_back(ZDD_ID(raw[i]));
    return ret;
}

// --- bdddump / bddvdump ---

static void dump_print_node(bddp node) {
    uint64_t ndx = node / 2;
    bddvar v = node_var(node);
    bddvar lev = var2level[v];
    bddp f0 = node_lo(node);
    bddp f1 = node_hi(node);

    std::printf("N%" PRIu64 " = [V%u(%u), ", ndx,
                static_cast<unsigned>(v), static_cast<unsigned>(lev));

    if (f0 & BDD_CONST_FLAG) {
        std::printf("%" PRIu64, f0 & ~BDD_CONST_FLAG);
    } else {
        std::printf("N%" PRIu64, (f0 & ~BDD_COMP_FLAG) / 2);
    }

    std::printf(", ");

    bool neg1 = (f1 & BDD_COMP_FLAG) != 0;
    bddp f1abs = f1 & ~BDD_COMP_FLAG;
    if (f1abs & BDD_CONST_FLAG) {
        if (neg1) std::printf("~");
        std::printf("%" PRIu64, f1abs & ~BDD_CONST_FLAG);
    } else {
        if (neg1) std::printf("~");
        std::printf("N%" PRIu64, f1abs / 2);
    }

    std::printf("]\n");
}

// Explicit-stack rewrite of dump_rec. Post-order DFS: each node's children
// are emitted before the node itself. Shared nodes are visited once.
// PRECONDITION: caller holds a bdd_gc_guard scope.
static void dump_iter(bddp root_f, std::unordered_set<bddp>& visited) {
    enum class Phase : uint8_t { ENTER, AFTER_LO, AFTER_HI };
    struct Frame {
        bddp f;
        bddp node;
        bddp f1;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root_f;
    init.node = 0;
    init.f1 = 0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            bddp node = f & ~BDD_COMP_FLAG;
            if (node & BDD_CONST_FLAG) { stack.pop_back(); break; }
            if (!visited.insert(node).second) { stack.pop_back(); break; }

            uint64_t idx = node / 2 - 1;
            if (idx >= bdd_node_used) {
                std::printf("bdddump: Invalid bddp\n");
                stack.pop_back();
                break;
            }

            bddp f0 = node_lo(node);
            bddp f1 = node_hi(node);
            frame.node = node;
            frame.f1 = f1;
            frame.phase = Phase::AFTER_LO;

            Frame child;
            child.f = f0;
            child.node = 0; child.f1 = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::AFTER_LO: {
            frame.phase = Phase::AFTER_HI;
            Frame child;
            child.f = frame.f1 & ~BDD_COMP_FLAG;
            child.node = 0; child.f1 = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::AFTER_HI: {
            dump_print_node(frame.node);
            stack.pop_back();
            break;
        }
        }
    }
}

static void dump_rec(bddp f, std::unordered_set<bddp>& visited) {
    BDD_RecurGuard guard;
    bddp node = f & ~BDD_COMP_FLAG;
    if (node & BDD_CONST_FLAG) return;
    if (!visited.insert(node).second) return;

    uint64_t idx = node / 2 - 1;
    if (idx >= bdd_node_used) {
        std::printf("bdddump: Invalid bddp\n");
        return;
    }

    bddvar v = node_var(node);
    bddp f0 = node_lo(node);
    bddp f1 = node_hi(node);

    // Recurse children first (bottom-up order)
    dump_rec(f0, visited);
    dump_rec(f1 & ~BDD_COMP_FLAG, visited);

    uint64_t ndx = node / 2;
    bddvar lev = var2level[v];

    // Print: N<index> = [V<var>(<lev>), <f0>, <f1>]
    std::printf("N%" PRIu64 " = [V%u(%u), ", ndx, static_cast<unsigned>(v), static_cast<unsigned>(lev));

    // f0 (lo child) - always non-complemented due to normalization
    if (f0 & BDD_CONST_FLAG) {
        std::printf("%" PRIu64, f0 & ~BDD_CONST_FLAG);
    } else {
        std::printf("N%" PRIu64, (f0 & ~BDD_COMP_FLAG) / 2);
    }

    std::printf(", ");

    // f1 (hi child) - may have complement edge
    bool neg1 = (f1 & BDD_COMP_FLAG) != 0;
    bddp f1abs = f1 & ~BDD_COMP_FLAG;
    if (f1abs & BDD_CONST_FLAG) {
        if (neg1) std::printf("~");
        std::printf("%" PRIu64, f1abs & ~BDD_CONST_FLAG);
    } else {
        if (neg1) std::printf("~");
        std::printf("N%" PRIu64, f1abs / 2);
    }

    std::printf("]\n");
}

static void dump_root(const char* label, bddp f) {
    std::printf("%s = ", label);
    if (f == bddnull) {
        std::printf("NULL");
    } else if (f & BDD_CONST_FLAG) {
        bool neg = (f & BDD_COMP_FLAG) != 0;
        bddp val = (f & ~BDD_COMP_FLAG) & ~BDD_CONST_FLAG;
        if (neg) std::printf("~");
        std::printf("%" PRIu64, val);
    } else {
        bool neg = (f & BDD_COMP_FLAG) != 0;
        if (neg) std::printf("~");
        std::printf("N%" PRIu64, (f & ~BDD_COMP_FLAG) / 2);
    }
    std::printf("\n");
}

void bdddump(bddp f) {
    if (f == bddnull) {
        std::printf("RT = NULL\n\n");
        return;
    }
    if (!(f & BDD_CONST_FLAG)) {
        bddp node = f & ~BDD_COMP_FLAG;
        uint64_t idx = node / 2 - 1;
        if (idx >= bdd_node_used) {
            std::printf("bdddump: Invalid bddp\n");
            return;
        }
    }
    std::unordered_set<bddp> visited;
    if (use_iter_1op(f)) {
        dump_iter(f, visited);
    } else {
        dump_rec(f, visited);
    }
    dump_root("RT", f);
    std::printf("\n");
}

void bddvdump(bddp *p, int n) {
    if (!p || n <= 0) return;
    // Find effective limit (bddnull acts as sentinel)
    int lim = 0;
    for (int i = 0; i < n; i++) {
        if (p[i] == bddnull) break;
        lim = i + 1;
    }

    // Validate non-terminal nodes
    for (int i = 0; i < lim; i++) {
        if (!(p[i] & BDD_CONST_FLAG)) {
            bddp node = p[i] & ~BDD_COMP_FLAG;
            uint64_t idx = node / 2 - 1;
            if (idx >= bdd_node_used) {
                std::printf("bdddump: Invalid bddp\n");
                return;
            }
        }
    }

    // Dump all nodes (shared nodes printed once). Dispatch per-root to the
    // iterative implementation when the root's level exceeds BDD_RecurLimit.
    std::unordered_set<bddp> visited;
    for (int i = 0; i < lim; i++) {
        if (use_iter_1op(p[i])) {
            dump_iter(p[i], visited);
        } else {
            dump_rec(p[i], visited);
        }
    }

    // Print roots (up to and including the first bddnull)
    for (int i = 0; i < n; i++) {
        char label[32];
        std::snprintf(label, sizeof(label), "RT%d", i);
        dump_root(label, p[i]);
        if (p[i] == bddnull) break;
    }
    std::printf("\n");
}

// --- Sapporo format save/load wrappers ---

void bdd_export_sapporo(FILE* strm, bddp f) {
    bddexport(strm, &f, 1);
}

void bdd_export_sapporo(std::ostream& strm, bddp f) {
    bddexport(strm, &f, 1);
}

bddp bdd_import_sapporo(FILE* strm) {
    bddp p = bddnull;
    int ret = bddimport(strm, &p, 1);
    if (ret <= 0)
        throw std::runtime_error("sapporo import: parse error");
    return p;
}

bddp bdd_import_sapporo(std::istream& strm) {
    bddp p = bddnull;
    int ret = bddimport(strm, &p, 1);
    if (ret <= 0)
        throw std::runtime_error("sapporo import: parse error");
    return p;
}

void zdd_export_sapporo(FILE* strm, bddp f) {
    bddexport(strm, &f, 1);
}

void zdd_export_sapporo(std::ostream& strm, bddp f) {
    bddexport(strm, &f, 1);
}

bddp zdd_import_sapporo(FILE* strm) {
    bddp p = bddnull;
    int ret = bddimportz(strm, &p, 1);
    if (ret <= 0)
        throw std::runtime_error("sapporo import: parse error");
    return p;
}

bddp zdd_import_sapporo(std::istream& strm) {
    bddp p = bddnull;
    int ret = bddimportz(strm, &p, 1);
    if (ret <= 0)
        throw std::runtime_error("sapporo import: parse error");
    return p;
}

} // namespace kyotodd
