#include "bdd.h"
#include "bdd_internal.h"
#include <cinttypes>
#include <climits>
#include <cstdlib>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cstring>
#include <unordered_set>

namespace kyotodd {


// Max token length for arc string representations (node IDs, "F", "T", "N").
// ARC_BUF_SIZE includes the null terminator.
#define ARC_MAX_LEN 31
#define ARC_BUF_SIZE (ARC_MAX_LEN + 1)
#define XSTR_(x) #x
#define XSTR(x) XSTR_(x)

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

// Stream output helpers for FILE* and std::ostream.
static void write_str(FILE* strm, const char* s) { std::fputs(s, strm); }
static void write_str(std::ostream& strm, const char* s) { strm << s; }

static bool stream_valid(FILE* strm) { return strm != nullptr; }
static bool stream_valid(std::ostream&) { return true; }

static bool stream_has_error(FILE* strm) { return std::ferror(strm) != 0; }
static bool stream_has_error(std::ostream& strm) { return !strm.good(); }

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

typedef bddp (*import_nodefn_t)(bddvar, bddp, bddp);

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

static const unsigned IMPORT_MAX_LEVEL_LIMIT = 1000000;
static const unsigned IMPORT_MAX_NODE_COUNT = 10000000;
static const unsigned IMPORT_MAX_OUTPUT_COUNT = 1000000;

// Stream input helpers for FILE* and std::istream.
// Tags ("_i", "_o", "_n") are at most 2 chars; ARC_MAX_LEN is more than enough.
static bool read_tag_uint(FILE* strm, const char* tag, unsigned& val) {
    char buf[ARC_BUF_SIZE];
    if (std::fscanf(strm, " %" XSTR(ARC_MAX_LEN) "s %u", buf, &val) != 2) return false;
    return std::string(buf) == tag;
}
static bool read_tag_uint(std::istream& strm, const char* tag, unsigned& val) {
    std::string s;
    if (!(strm >> s >> val)) return false;
    return s == tag;
}

static bool read_node_line(FILE* strm, uint64_t& id, unsigned& level,
                            char* s0, char* s1) {
    return std::fscanf(strm, " %" SCNu64 " %u %" XSTR(ARC_MAX_LEN) "s %" XSTR(ARC_MAX_LEN) "s",
                       &id, &level, s0, s1) == 4;
}
static bool read_node_line(std::istream& strm, uint64_t& id, unsigned& level,
                            char* s0, char* s1) {
    std::string ss0, ss1;
    if (!(strm >> id >> level >> ss0 >> ss1)) return false;
    if (ss0.size() > ARC_MAX_LEN || ss1.size() > ARC_MAX_LEN) return false;
    std::snprintf(s0, ARC_BUF_SIZE, "%s", ss0.c_str());
    std::snprintf(s1, ARC_BUF_SIZE, "%s", ss1.c_str());
    return true;
}

static bool read_token(FILE* strm, char* buf) {
    return std::fscanf(strm, " %" XSTR(ARC_MAX_LEN) "s", buf) == 1;
}
static bool read_token(std::istream& strm, char* buf) {
    std::string s;
    if (!(strm >> s)) return false;
    if (s.size() > ARC_MAX_LEN) return false;
    std::snprintf(buf, ARC_BUF_SIZE, "%s", s.c_str());
    return true;
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
    dump_rec(f, visited);
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

    // Dump all nodes (shared nodes printed once)
    std::unordered_set<bddp> visited;
    for (int i = 0; i < lim; i++) {
        dump_rec(p[i], visited);
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

// --- Knuth format save/load (obsolete) ---

// Read one line from FILE* or istream. Returns false on EOF.
static bool read_line(FILE* strm, char* buf, size_t bufsize) {
    if (std::fgets(buf, static_cast<int>(bufsize), strm) == NULL) return false;
    // Strip trailing newline
    size_t len = std::strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    return true;
}
static bool read_line(std::istream& strm, char* buf, size_t bufsize) {
    std::string s;
    if (!std::getline(strm, s)) return false;
    std::snprintf(buf, bufsize, "%s", s.c_str());
    return true;
}

template<typename Stream>
static void knuth_export_core(Stream& strm, bddp f, bool is_hex, int offset, bool is_zdd) {
    if (f == bddnull)
        throw std::invalid_argument("knuth export: null node");

    // Terminal cases
    if (f == bddempty) { write_str(strm, "0\n"); return; }
    if (f == bddsingle) { write_str(strm, "1\n"); return; }

    // Complement-edge-expanding DFS (same as Graphillion export)
    std::unordered_map<bddp, uint64_t> id_map;
    std::vector<bddp> order;
    bddvar max_level = 0;

    std::vector<std::pair<bddp, bool>> stack;
    stack.push_back(std::make_pair(f, false));

    while (!stack.empty()) {
        bddp node = stack.back().first;
        bool expanded = stack.back().second;

        if (expanded) {
            stack.pop_back();
            if (id_map.count(node)) continue;
            uint64_t gid = order.size() + 2;  // IDs start at 2
            id_map[node] = gid;
            order.push_back(node);
            bddp raw = node & ~BDD_COMP_FLAG;
            bddvar v = node_var(raw);
            bddvar lev = var2level[v];
            if (lev > max_level) max_level = lev;
            continue;
        }
        stack.back().second = true;

        if (id_map.count(node)) { stack.pop_back(); continue; }

        bddp raw = node & ~BDD_COMP_FLAG;
        bool comp = (node & BDD_COMP_FLAG) != 0;
        bddp lo = node_lo(raw);
        bddp hi = node_hi(raw);
        if (comp) {
            lo = bddnot(lo);
            if (!is_zdd) hi = bddnot(hi);  // BDD: both children flipped
        }

        if (!(hi & BDD_CONST_FLAG) && !id_map.count(hi))
            stack.push_back(std::make_pair(hi, false));
        if (!(lo & BDD_CONST_FLAG) && !id_map.count(lo))
            stack.push_back(std::make_pair(lo, false));
    }

    // Sort by KyotoDD level DESCENDING (= knuth_level ascending)
    std::sort(order.begin(), order.end(),
              [](bddp a, bddp b) {
                  bddvar la = var2level[node_var(a & ~BDD_COMP_FLAG)];
                  bddvar lb = var2level[node_var(b & ~BDD_COMP_FLAG)];
                  return la > lb;
              });

    // Reassign IDs after sort (starting from 2)
    id_map.clear();
    for (size_t i = 0; i < order.size(); i++) {
        id_map[order[i]] = i + 2;
    }

    // Helper to convert a child bddp to its Knuth ID
    auto to_knuth_id = [&](bddp edge) -> uint64_t {
        if (edge == bddempty) return 0;
        if (edge == bddsingle) return 1;
        return id_map[edge];
    };

    bddvar N = max_level;
    char buf[128];
    const char* id_fmt = is_hex ? "%" PRIx64 ":%" PRIx64 ",%" PRIx64 "\n"
                                : "%" PRIu64 ":%" PRIu64 ",%" PRIu64 "\n";

    // Write nodes grouped by knuth_level
    bddvar prev_knuth_level = 0;
    for (size_t i = 0; i < order.size(); i++) {
        bddp node = order[i];
        bddp raw = node & ~BDD_COMP_FLAG;
        bool comp = (node & BDD_COMP_FLAG) != 0;
        bddvar lev = var2level[node_var(raw)];
        int knuth_lev = static_cast<int>(N) + 1 - static_cast<int>(lev) + offset;

        if (static_cast<bddvar>(knuth_lev) != prev_knuth_level) {
            std::snprintf(buf, sizeof(buf), "#%d\n", knuth_lev);
            write_str(strm, buf);
            prev_knuth_level = static_cast<bddvar>(knuth_lev);
        }

        bddp lo = node_lo(raw);
        bddp hi = node_hi(raw);
        if (comp) {
            lo = bddnot(lo);
            if (!is_zdd) hi = bddnot(hi);  // BDD: both children flipped
        }

        uint64_t nid = id_map[node];
        std::snprintf(buf, sizeof(buf), id_fmt, nid,
                     to_knuth_id(lo), to_knuth_id(hi));
        write_str(strm, buf);
    }

    if (stream_has_error(strm))
        throw std::runtime_error("knuth export: write error");
}

void bdd_export_knuth(FILE* strm, bddp f, bool is_hex, int offset) {
    knuth_export_core(strm, f, is_hex, offset, false);
}
void bdd_export_knuth(std::ostream& strm, bddp f, bool is_hex, int offset) {
    knuth_export_core(strm, f, is_hex, offset, false);
}
void zdd_export_knuth(FILE* strm, bddp f, bool is_hex, int offset) {
    knuth_export_core(strm, f, is_hex, offset, true);
}
void zdd_export_knuth(std::ostream& strm, bddp f, bool is_hex, int offset) {
    knuth_export_core(strm, f, is_hex, offset, true);
}

// --- Knuth format import ---

template<typename Stream>
static bddp knuth_import_core(Stream& strm, bool is_hex, int offset,
                               import_nodefn_t make_node) {
    char buf[256];

    // Read first line
    if (!read_line(strm, buf, sizeof(buf)))
        throw std::runtime_error("knuth import: empty input");

    // Terminal cases
    if (buf[0] == '0' && (buf[1] == '\0' || buf[1] == '\n'))
        return bddfalse;
    if (buf[0] == '1' && (buf[1] == '\0' || buf[1] == '\n'))
        return bddtrue;

    // First line should be a level header "#<n>"
    struct KnuthNode {
        uint64_t id;
        unsigned knuth_level;
        uint64_t lo_id;
        uint64_t hi_id;
    };

    std::vector<KnuthNode> nodes;
    unsigned current_knuth_level = 0;
    unsigned max_knuth_level = 0;

    // Process first line and continue
    bool first_line = true;
    do {
        if (buf[0] == '#') {
            int lev;
            if (std::sscanf(buf, "#%d", &lev) != 1)
                throw std::runtime_error("knuth import: invalid level header");
            current_knuth_level = static_cast<unsigned>(lev);
            if (current_knuth_level > max_knuth_level)
                max_knuth_level = current_knuth_level;
        } else if (buf[0] != '\0') {
            if (current_knuth_level == 0)
                throw std::runtime_error("knuth import: node line before any level header");
            KnuthNode kn;
            kn.knuth_level = current_knuth_level;
            uint64_t id_val, lo_val, hi_val;
            int c;
            if (is_hex) {
                c = std::sscanf(buf, "%" SCNx64 ":%" SCNx64 ",%" SCNx64,
                               &id_val, &lo_val, &hi_val);
            } else {
                c = std::sscanf(buf, "%" SCNu64 ":%" SCNu64 ",%" SCNu64,
                               &id_val, &lo_val, &hi_val);
            }
            if (c != 3)
                throw std::runtime_error("knuth import: invalid node line");
            kn.id = id_val;
            kn.lo_id = lo_val;
            kn.hi_id = hi_val;
            nodes.push_back(kn);
        }
        first_line = false;
    } while (read_line(strm, buf, sizeof(buf)));

    if (nodes.empty())
        throw std::runtime_error("knuth import: no nodes found");

    unsigned N = max_knuth_level;
    if (N > IMPORT_MAX_LEVEL_LIMIT)
        throw std::runtime_error("knuth import: max_level exceeds limit");

    // Determine max level needed and ensure variables exist
    int max_kyoto_level = static_cast<int>(N) + offset;
    if (max_kyoto_level < 1) max_kyoto_level = 1;
    while (bddvarused() < static_cast<bddvar>(max_kyoto_level))
        bddnewvar();

    // Build id_map with topological sort (worklist approach)
    std::unordered_map<uint64_t, bddp> id_map;
    id_map[0] = bddfalse;
    id_map[1] = bddtrue;

    // Count pending (unresolved) children per node
    std::vector<int> pending(nodes.size(), 0);
    // Map from child Knuth ID to parent node indices that depend on it
    std::unordered_map<uint64_t, std::vector<size_t>> dependents;
    for (size_t i = 0; i < nodes.size(); i++) {
        const KnuthNode& kn = nodes[i];
        if (!id_map.count(kn.lo_id)) {
            pending[i]++;
            dependents[kn.lo_id].push_back(i);
        }
        if (!id_map.count(kn.hi_id)) {
            pending[i]++;
            dependents[kn.hi_id].push_back(i);
        }
    }

    // Initialize ready queue with nodes that have all children resolved
    std::vector<size_t> ready;
    for (size_t i = 0; i < nodes.size(); i++) {
        if (pending[i] == 0) ready.push_back(i);
    }

    uint64_t created_count = 0;
    while (!ready.empty()) {
        size_t i = ready.back();
        ready.pop_back();
        const KnuthNode& kn = nodes[i];
        int level = static_cast<int>(N) + 1
                  - static_cast<int>(kn.knuth_level) + offset;
        if (level < 1)
            throw std::runtime_error("knuth import: computed level < 1");
        bddvar var = bddvaroflev(static_cast<bddvar>(level));
        bddp lo = id_map[kn.lo_id];
        bddp hi = id_map[kn.hi_id];
        id_map[kn.id] = make_node(var, lo, hi);
        created_count++;

        // Notify dependents
        std::unordered_map<uint64_t, std::vector<size_t>>::iterator dit = dependents.find(kn.id);
        if (dit != dependents.end()) {
            for (size_t j = 0; j < dit->second.size(); j++) {
                size_t dep = dit->second[j];
                if (--pending[dep] == 0) ready.push_back(dep);
            }
        }
    }

    if (created_count < nodes.size())
        throw std::runtime_error("knuth import: circular dependency");

    // The first non-terminal node (ID 2) is the root
    if (!id_map.count(2))
        throw std::runtime_error("knuth import: root node (ID 2) not found");
    return id_map[2];
}

bddp bdd_import_knuth(FILE* strm, bool is_hex, int offset) {
    return knuth_import_core(strm, is_hex, offset, BDD::getnode_raw);
}
bddp bdd_import_knuth(std::istream& strm, bool is_hex, int offset) {
    return knuth_import_core(strm, is_hex, offset, BDD::getnode_raw);
}
bddp zdd_import_knuth(FILE* strm, bool is_hex, int offset) {
    return knuth_import_core(strm, is_hex, offset, ZDD::getnode_raw);
}
bddp zdd_import_knuth(std::istream& strm, bool is_hex, int offset) {
    return knuth_import_core(strm, is_hex, offset, ZDD::getnode_raw);
}

// --- Binary format save/load ---

static void write_bytes(FILE* strm, const void* data, size_t n) {
    std::fwrite(data, 1, n, strm);
}
static void write_bytes(std::ostream& strm, const void* data, size_t n) {
    strm.write(static_cast<const char*>(data), n);
}
static bool read_bytes(FILE* strm, void* data, size_t n) {
    return std::fread(data, 1, n, strm) == n;
}
static bool read_bytes(std::istream& strm, void* data, size_t n) {
    strm.read(static_cast<char*>(data), n);
    return !strm.fail();
}

// Little-endian encode/decode
static void encode_le16(uint8_t* buf, uint16_t v) {
    buf[0] = v & 0xFF; buf[1] = (v >> 8) & 0xFF;
}
static void encode_le32(uint8_t* buf, uint32_t v) {
    for (int i = 0; i < 4; i++) buf[i] = (v >> (8*i)) & 0xFF;
}
static void encode_le64(uint8_t* buf, uint64_t v) {
    for (int i = 0; i < 8; i++) buf[i] = (v >> (8*i)) & 0xFF;
}
static uint16_t decode_le16(const uint8_t* buf) {
    return static_cast<uint16_t>(buf[0]) | (static_cast<uint16_t>(buf[1]) << 8);
}
static uint32_t decode_le32(const uint8_t* buf) {
    uint32_t v = 0;
    for (int i = 0; i < 4; i++) v |= static_cast<uint32_t>(buf[i]) << (8*i);
    return v;
}
static uint64_t decode_le64(const uint8_t* buf) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= static_cast<uint64_t>(buf[i]) << (8*i);
    return v;
}

// Write/read a single ID in LE with the given byte width
template<typename Stream>
static void write_id_le(Stream& strm, uint64_t id, int bytes) {
    uint8_t buf[8] = {0};
    for (int i = 0; i < bytes; i++) buf[i] = (id >> (8*i)) & 0xFF;
    write_bytes(strm, buf, bytes);
}
template<typename Stream>
static uint64_t read_id_le(Stream& strm, int bytes) {
    uint8_t buf[8] = {0};
    if (!read_bytes(strm, buf, bytes))
        throw std::runtime_error("binary import: unexpected end of input");
    uint64_t id = 0;
    for (int i = 0; i < bytes; i++) id |= static_cast<uint64_t>(buf[i]) << (8*i);
    return id;
}

// Determine minimum byte-aligned bits to represent max_id
static uint8_t bits_for_max_id(uint64_t max_id) {
    if (max_id <= 0xFF) return 8;
    if (max_id <= 0xFFFF) return 16;
    if (max_id <= 0xFFFFFFFFULL) return 32;
    return 64;
}

template<typename Stream>
static void binary_export_core(Stream& strm, bddp f, uint8_t dd_type) {
    if (f == bddnull)
        throw std::invalid_argument("binary export: null node");

    const uint32_t t = 2;  // number of terminals

    // Collect all physical nodes via DFS, grouped by level
    std::unordered_set<bddp> visited;
    std::vector<bddp> all_nodes;  // physical node bddps (even, no complement)

    if (!(f & BDD_CONST_FLAG)) {
        // Iterative DFS to collect physical nodes
        bddp root_phys = f & ~BDD_COMP_FLAG;
        std::vector<bddp> stack;
        stack.push_back(root_phys);
        visited.insert(root_phys);
        while (!stack.empty()) {
            bddp node = stack.back();
            stack.pop_back();
            all_nodes.push_back(node);
            bddp lo = node_lo(node);
            bddp hi = node_hi(node);
            bddp lo_phys = lo & ~BDD_COMP_FLAG;
            bddp hi_phys = hi & ~BDD_COMP_FLAG;
            if (!(hi_phys & BDD_CONST_FLAG) && visited.insert(hi_phys).second)
                stack.push_back(hi_phys);
            if (!(lo_phys & BDD_CONST_FLAG) && visited.insert(lo_phys).second)
                stack.push_back(lo_phys);
        }
    }

    // Sort by level (ascending: level 1 first)
    std::sort(all_nodes.begin(), all_nodes.end(),
              [](bddp a, bddp b) {
                  return var2level[node_var(a)] < var2level[node_var(b)];
              });

    uint64_t total_nodes = all_nodes.size();
    bddvar max_level = 0;
    for (size_t i = 0; i < all_nodes.size(); i++) {
        bddvar lev = var2level[node_var(all_nodes[i])];
        if (lev > max_level) max_level = lev;
    }
    if (max_level == 0 && bddvarused() > 0) max_level = bddvarused();

    // Build ID mapping: physical bddp -> binary ID (even)
    std::unordered_map<bddp, uint64_t> id_map;
    for (uint64_t i = 0; i < total_nodes; i++) {
        id_map[all_nodes[i]] = t + 2 * i;
    }

    // Helper to map any bddp edge to binary ID
    auto to_bin_id = [&](bddp edge) -> uint64_t {
        if (edge == bddfalse) return 0;
        if (edge == bddtrue) return 1;
        bddp phys = edge & ~BDD_COMP_FLAG;
        bool comp = (edge & BDD_COMP_FLAG) != 0;
        auto it = id_map.find(phys);
        if (it == id_map.end()) {
            throw std::logic_error("binary export: child node not in id_map");
        }
        uint64_t bin_id = it->second;
        return comp ? (bin_id | 1) : bin_id;
    };

    // Max binary ID
    uint64_t max_id = 1;  // at least terminal 1
    if (total_nodes > 0)
        max_id = t + 2 * (total_nodes - 1) + 1;  // max odd (negated last node)
    uint64_t root_bin_id = (f & BDD_CONST_FLAG) ?
        ((f == bddtrue) ? 1 : 0) : to_bin_id(f);
    if (root_bin_id > max_id) max_id = root_bin_id;

    uint8_t bits = bits_for_max_id(max_id);
    int id_bytes = bits / 8;

    // --- Write magic ---
    uint8_t magic[3] = {0x42, 0x44, 0x44};
    write_bytes(strm, magic, 3);

    // --- Write header (91 bytes) ---
    uint8_t header[91];
    std::memset(header, 0, sizeof(header));
    header[0] = 1;  // version
    header[1] = dd_type;  // type: 2=BDD, 3=ZDD
    encode_le16(header + 2, 2);   // number_of_arcs
    encode_le32(header + 4, t);   // number_of_terminals
    header[8] = 0;   // number_of_bits_for_level (unused v1)
    header[9] = bits; // number_of_bits_for_id
    header[10] = 1;  // use_negative_arcs
    encode_le64(header + 11, max_level);  // max_level
    encode_le64(header + 19, 1);  // number_of_roots
    // bytes 27-90: reserved (already zero)
    write_bytes(strm, header, 91);

    // --- Write node-count array (max_level * uint64_t) ---
    // Count nodes per level
    std::vector<uint64_t> level_counts(max_level, 0);
    for (size_t i = 0; i < all_nodes.size(); i++) {
        bddvar lev = var2level[node_var(all_nodes[i])];
        if (lev < 1 || lev > max_level) {
            throw std::logic_error("binary export: node has invalid level");
        }
        level_counts[lev - 1]++;
    }
    for (bddvar l = 0; l < max_level; l++) {
        uint8_t buf[8];
        encode_le64(buf, level_counts[l]);
        write_bytes(strm, buf, 8);
    }

    // --- Write root ID ---
    write_id_le(strm, root_bin_id, id_bytes);

    // --- Write node array (sorted by level, each: lo_id, hi_id) ---
    for (size_t i = 0; i < all_nodes.size(); i++) {
        bddp node = all_nodes[i];
        bddp lo = node_lo(node);
        bddp hi = node_hi(node);
        write_id_le(strm, to_bin_id(lo), id_bytes);
        write_id_le(strm, to_bin_id(hi), id_bytes);
    }

    if (stream_has_error(strm))
        throw std::runtime_error("binary export: write error");
}

void bdd_export_binary(FILE* strm, bddp f) { binary_export_core(strm, f, 2); }
void bdd_export_binary(std::ostream& strm, bddp f) { binary_export_core(strm, f, 2); }
void zdd_export_binary(FILE* strm, bddp f) { binary_export_core(strm, f, 3); }
void zdd_export_binary(std::ostream& strm, bddp f) { binary_export_core(strm, f, 3); }
void qdd_export_binary(FILE* strm, bddp f) { binary_export_core(strm, f, 1); }
void qdd_export_binary(std::ostream& strm, bddp f) { binary_export_core(strm, f, 1); }
void unreduced_export_binary(FILE* strm, bddp f) { binary_export_core(strm, f, 1); }
void unreduced_export_binary(std::ostream& strm, bddp f) { binary_export_core(strm, f, 1); }

// --- Multi-root binary format export ---

template<typename Stream>
static void binary_export_multi_core(Stream& strm, const bddp* roots, size_t num_roots, uint8_t dd_type) {
    if (num_roots > 0 && roots == nullptr) {
        throw std::invalid_argument("binary export: roots pointer is null");
    }
    for (size_t r = 0; r < num_roots; r++) {
        if (roots[r] == bddnull)
            throw std::invalid_argument("binary export: null node");
    }

    const uint32_t t = 2;  // number of terminals

    // Collect all physical nodes via DFS from all roots
    std::unordered_set<bddp> visited;
    std::vector<bddp> all_nodes;

    for (size_t r = 0; r < num_roots; r++) {
        bddp f = roots[r];
        if (f & BDD_CONST_FLAG) continue;  // terminal, skip
        bddp root_phys = f & ~BDD_COMP_FLAG;
        if (!visited.insert(root_phys).second) continue;
        std::vector<bddp> stack;
        stack.push_back(root_phys);
        while (!stack.empty()) {
            bddp node = stack.back();
            stack.pop_back();
            all_nodes.push_back(node);
            bddp lo = node_lo(node);
            bddp hi = node_hi(node);
            bddp lo_phys = lo & ~BDD_COMP_FLAG;
            bddp hi_phys = hi & ~BDD_COMP_FLAG;
            if (!(hi_phys & BDD_CONST_FLAG) && visited.insert(hi_phys).second)
                stack.push_back(hi_phys);
            if (!(lo_phys & BDD_CONST_FLAG) && visited.insert(lo_phys).second)
                stack.push_back(lo_phys);
        }
    }

    // Sort by level (ascending: level 1 first)
    std::sort(all_nodes.begin(), all_nodes.end(),
              [](bddp a, bddp b) {
                  return var2level[node_var(a)] < var2level[node_var(b)];
              });

    uint64_t total_nodes = all_nodes.size();
    bddvar max_level = 0;
    for (size_t i = 0; i < all_nodes.size(); i++) {
        bddvar lev = var2level[node_var(all_nodes[i])];
        if (lev > max_level) max_level = lev;
    }
    if (max_level == 0 && bddvarused() > 0) max_level = bddvarused();

    // Build ID mapping: physical bddp -> binary ID (even)
    std::unordered_map<bddp, uint64_t> id_map;
    for (uint64_t i = 0; i < total_nodes; i++) {
        id_map[all_nodes[i]] = t + 2 * i;
    }

    // Helper to map any bddp edge to binary ID
    auto to_bin_id = [&](bddp edge) -> uint64_t {
        if (edge == bddfalse) return 0;
        if (edge == bddtrue) return 1;
        bddp phys = edge & ~BDD_COMP_FLAG;
        bool comp = (edge & BDD_COMP_FLAG) != 0;
        auto it = id_map.find(phys);
        if (it == id_map.end()) {
            throw std::logic_error("binary export: child node not in id_map");
        }
        uint64_t bin_id = it->second;
        return comp ? (bin_id | 1) : bin_id;
    };

    // Max binary ID
    uint64_t max_id = 1;  // at least terminal 1
    if (total_nodes > 0)
        max_id = t + 2 * (total_nodes - 1) + 1;
    for (size_t r = 0; r < num_roots; r++) {
        bddp f = roots[r];
        uint64_t root_bin_id = (f & BDD_CONST_FLAG) ?
            ((f == bddtrue) ? 1 : 0) : to_bin_id(f);
        if (root_bin_id > max_id) max_id = root_bin_id;
    }

    uint8_t bits = bits_for_max_id(max_id);
    int id_bytes = bits / 8;

    // --- Write magic ---
    uint8_t magic[3] = {0x42, 0x44, 0x44};
    write_bytes(strm, magic, 3);

    // --- Write header (91 bytes) ---
    uint8_t header[91];
    std::memset(header, 0, sizeof(header));
    header[0] = 1;  // version
    header[1] = dd_type;
    encode_le16(header + 2, 2);   // number_of_arcs
    encode_le32(header + 4, t);   // number_of_terminals
    header[8] = 0;   // number_of_bits_for_level (unused v1)
    header[9] = bits; // number_of_bits_for_id
    header[10] = 1;  // use_negative_arcs
    encode_le64(header + 11, max_level);
    encode_le64(header + 19, static_cast<uint64_t>(num_roots));
    // bytes 27-90: reserved (already zero)
    write_bytes(strm, header, 91);

    // --- Write node-count array (max_level * uint64_t) ---
    std::vector<uint64_t> level_counts(max_level, 0);
    for (size_t i = 0; i < all_nodes.size(); i++) {
        bddvar lev = var2level[node_var(all_nodes[i])];
        if (lev < 1 || lev > max_level) {
            throw std::logic_error("binary export: node has invalid level");
        }
        level_counts[lev - 1]++;
    }
    for (bddvar l = 0; l < max_level; l++) {
        uint8_t buf[8];
        encode_le64(buf, level_counts[l]);
        write_bytes(strm, buf, 8);
    }

    // --- Write root IDs ---
    for (size_t r = 0; r < num_roots; r++) {
        bddp f = roots[r];
        uint64_t root_bin_id = (f & BDD_CONST_FLAG) ?
            ((f == bddtrue) ? 1 : 0) : to_bin_id(f);
        write_id_le(strm, root_bin_id, id_bytes);
    }

    // --- Write node array (sorted by level, each: lo_id, hi_id) ---
    for (size_t i = 0; i < all_nodes.size(); i++) {
        bddp node = all_nodes[i];
        bddp lo = node_lo(node);
        bddp hi = node_hi(node);
        write_id_le(strm, to_bin_id(lo), id_bytes);
        write_id_le(strm, to_bin_id(hi), id_bytes);
    }

    if (stream_has_error(strm))
        throw std::runtime_error("binary export: write error");
}

void bdd_export_binary_multi(FILE* strm, const bddp* roots, size_t n) { binary_export_multi_core(strm, roots, n, 2); }
void bdd_export_binary_multi(std::ostream& strm, const bddp* roots, size_t n) { binary_export_multi_core(strm, roots, n, 2); }
void zdd_export_binary_multi(FILE* strm, const bddp* roots, size_t n) { binary_export_multi_core(strm, roots, n, 3); }
void zdd_export_binary_multi(std::ostream& strm, const bddp* roots, size_t n) { binary_export_multi_core(strm, roots, n, 3); }
void qdd_export_binary_multi(FILE* strm, const bddp* roots, size_t n) { binary_export_multi_core(strm, roots, n, 1); }
void qdd_export_binary_multi(std::ostream& strm, const bddp* roots, size_t n) { binary_export_multi_core(strm, roots, n, 1); }

// --- Binary format import ---

static const char* dd_type_name(uint8_t t) {
    switch (t) {
        case 1: return "QDD";
        case 2: return "BDD";
        case 3: return "ZDD";
        case 4: return "MTBDD";
        case 5: return "MTZDD";
        default: return "Unknown";
    }
}

template<typename Stream>
static std::vector<bddp> binary_import_multi_core(Stream& strm, import_nodefn_t make_node, uint8_t expected_type) {
    // --- Read magic ---
    uint8_t magic[3];
    if (!read_bytes(strm, magic, 3) ||
        magic[0] != 0x42 || magic[1] != 0x44 || magic[2] != 0x44)
        throw std::runtime_error("binary import: invalid magic bytes");

    // --- Read header ---
    uint8_t header[91];
    if (!read_bytes(strm, header, 91))
        throw std::runtime_error("binary import: truncated header");

    uint8_t version = header[0];
    if (version != 1)
        throw std::runtime_error("binary import: unsupported version");

    uint8_t dd_type = header[1];
    if (expected_type != 0 && dd_type != expected_type) {
        throw std::runtime_error(
            std::string("binary import: type mismatch: expected ") +
            dd_type_name(expected_type) + " (type=" +
            std::to_string(static_cast<int>(expected_type)) +
            ") but file contains " + dd_type_name(dd_type) +
            " (type=" + std::to_string(static_cast<int>(dd_type)) + ")");
    }
    uint16_t num_arcs = decode_le16(header + 2);
    uint32_t num_terminals = decode_le32(header + 4);
    // header[8]: bits_for_level (unused)
    uint8_t bits_for_id = header[9];
    uint8_t use_neg = header[10];
    uint64_t max_level = decode_le64(header + 11);
    uint64_t num_roots = decode_le64(header + 19);

    if (num_arcs < 2)
        throw std::runtime_error("binary import: number_of_arcs < 2");
    if (bits_for_id == 0 || bits_for_id % 8 != 0)
        throw std::runtime_error("binary import: bits_for_id must be a multiple of 8");
    if (max_level > bdd_node_max)
        throw std::runtime_error("binary import: max_level exceeds node limit");
    if (num_roots > bdd_node_max)
        throw std::runtime_error("binary import: num_roots exceeds node limit");

    int id_bytes = bits_for_id / 8;
    uint32_t t = num_terminals;

    // --- Read node-count array ---
    std::vector<uint64_t> level_counts(max_level);
    for (uint64_t l = 0; l < max_level; l++) {
        uint8_t buf[8];
        if (!read_bytes(strm, buf, 8))
            throw std::runtime_error("binary import: truncated level-count array");
        level_counts[l] = decode_le64(buf);
    }

    uint64_t total_nodes = 0;
    for (uint64_t l = 0; l < max_level; l++) {
        if (total_nodes > UINT64_MAX - level_counts[l])
            throw std::runtime_error("binary import: total_nodes overflow");
        total_nodes += level_counts[l];
    }

    // --- Read root IDs ---
    std::vector<uint64_t> root_ids(num_roots);
    for (uint64_t r = 0; r < num_roots; r++) {
        root_ids[r] = read_id_le(strm, id_bytes);
    }

    // Handle empty case (num_roots == 0, total_nodes == 0)
    if (num_roots == 0) {
        // Ensure variables exist even for empty files
        while (bddvarused() < max_level) bddnewvar();
        return std::vector<bddp>();
    }

    // --- Read node array ---
    // Each node has num_arcs child IDs
    struct BinNode {
        std::vector<uint64_t> children;
    };
    std::vector<BinNode> bin_nodes(total_nodes);
    for (uint64_t i = 0; i < total_nodes; i++) {
        bin_nodes[i].children.resize(num_arcs);
        for (uint16_t a = 0; a < num_arcs; a++) {
            bin_nodes[i].children[a] = read_id_le(strm, id_bytes);
        }
    }

    // Ensure variables exist
    while (bddvarused() < max_level) bddnewvar();

    // --- Reconstruct nodes ---
    // Nodes are stored by level but may have children at any level
    // (e.g., ZDDs with non-standard variable ordering).
    // Use iterative topological sort: create nodes whose children
    // are all resolved (terminals or already created).
    std::unordered_map<uint64_t, bddp> id_map;

    // Helper to check if a binary ID is resolved
    auto is_resolved = [&](uint64_t bin_id) -> bool {
        if (bin_id < t) return true;  // terminal
        uint64_t even_id = (use_neg && (bin_id & 1)) ? bin_id - 1 : bin_id;
        return id_map.count(even_id) != 0;
    };

    // Helper to resolve a binary ID to internal bddp
    auto resolve = [&](uint64_t bin_id) -> bddp {
        if (bin_id < t) {
            if (bin_id == 0) return bddfalse;
            if (bin_id == 1) return bddtrue;
            throw std::runtime_error("binary import: unsupported terminal ID");
        }
        bool comp = false;
        if (use_neg && (bin_id & 1)) {
            comp = true;
            bin_id -= 1;
        }
        std::unordered_map<uint64_t, bddp>::const_iterator it = id_map.find(bin_id);
        if (it == id_map.end())
            throw std::runtime_error("binary import: undefined node reference");
        return comp ? bddnot(it->second) : it->second;
    };

    // Pre-compute level for each node index
    std::vector<bddvar> node_levels(total_nodes);
    {
        uint64_t idx = 0;
        for (uint64_t l = 0; l < max_level; l++) {
            for (uint64_t c = 0; c < level_counts[l]; c++) {
                if (idx >= total_nodes) {
                    throw std::runtime_error(
                        "binary import: level_counts sum exceeds total_nodes");
                }
                node_levels[idx++] = static_cast<bddvar>(l + 1);
            }
        }
    }

    // Worklist-based topological sort for node creation
    // Count pending (unresolved) children per node
    std::vector<int> pending(total_nodes, 0);
    // Map from binary ID (even, non-terminal) to node indices that depend on it
    std::unordered_map<uint64_t, std::vector<uint64_t>> dependents;
    for (uint64_t i = 0; i < total_nodes; i++) {
        const BinNode& bn = bin_nodes[i];
        for (uint16_t a = 0; a < num_arcs; a++) {
            if (!is_resolved(bn.children[a])) {
                pending[i]++;
                uint64_t child_even = (use_neg && (bn.children[a] & 1))
                    ? bn.children[a] - 1 : bn.children[a];
                dependents[child_even].push_back(i);
            }
        }
    }

    // Initialize ready queue
    std::vector<uint64_t> ready_queue;
    for (uint64_t i = 0; i < total_nodes; i++) {
        if (pending[i] == 0) ready_queue.push_back(i);
    }

    uint64_t created_count = 0;
    while (!ready_queue.empty()) {
        uint64_t i = ready_queue.back();
        ready_queue.pop_back();
        const BinNode& bn = bin_nodes[i];

        bddvar var = bddvaroflev(node_levels[i]);
        bddp lo = resolve(bn.children[0]);
        bddp hi = resolve(bn.children[1]);
        uint64_t bin_id = use_neg ? (t + 2 * i) : (t + i);
        id_map[bin_id] = make_node(var, lo, hi);
        created_count++;

        // Notify dependents
        std::unordered_map<uint64_t, std::vector<uint64_t>>::iterator dit = dependents.find(bin_id);
        if (dit != dependents.end()) {
            for (size_t j = 0; j < dit->second.size(); j++) {
                uint64_t dep = dit->second[j];
                if (--pending[dep] == 0) ready_queue.push_back(dep);
            }
        }
    }

    if (created_count < total_nodes)
        throw std::runtime_error("binary import: circular dependency in node references");

    // Resolve all roots
    std::vector<bddp> result;
    result.reserve(num_roots);
    for (uint64_t r = 0; r < num_roots; r++) {
        result.push_back(resolve(root_ids[r]));
    }
    return result;
}

template<typename Stream>
static bddp binary_import_core(Stream& strm, import_nodefn_t make_node, uint8_t expected_type) {
    std::vector<bddp> results = binary_import_multi_core(strm, make_node, expected_type);
    if (results.empty())
        throw std::runtime_error("binary import: number_of_roots < 1");
    return results[0];
}

bddp bdd_import_binary(FILE* strm, bool ignore_type) { return binary_import_core(strm, BDD::getnode_raw, ignore_type ? 0 : 2); }
bddp bdd_import_binary(std::istream& strm, bool ignore_type) { return binary_import_core(strm, BDD::getnode_raw, ignore_type ? 0 : 2); }
bddp zdd_import_binary(FILE* strm, bool ignore_type) { return binary_import_core(strm, ZDD::getnode_raw, ignore_type ? 0 : 3); }
bddp zdd_import_binary(std::istream& strm, bool ignore_type) { return binary_import_core(strm, ZDD::getnode_raw, ignore_type ? 0 : 3); }
bddp qdd_import_binary(FILE* strm, bool ignore_type) { return binary_import_core(strm, QDD::getnode_raw, ignore_type ? 0 : 1); }
bddp qdd_import_binary(std::istream& strm, bool ignore_type) { return binary_import_core(strm, QDD::getnode_raw, ignore_type ? 0 : 1); }
bddp unreduced_import_binary(FILE* strm) { return binary_import_core(strm, UnreducedDD::getnode_raw, 0); }
bddp unreduced_import_binary(std::istream& strm) { return binary_import_core(strm, UnreducedDD::getnode_raw, 0); }

std::vector<bddp> bdd_import_binary_multi(FILE* strm, bool ignore_type) { return binary_import_multi_core(strm, BDD::getnode_raw, ignore_type ? 0 : 2); }
std::vector<bddp> bdd_import_binary_multi(std::istream& strm, bool ignore_type) { return binary_import_multi_core(strm, BDD::getnode_raw, ignore_type ? 0 : 2); }
std::vector<bddp> zdd_import_binary_multi(FILE* strm, bool ignore_type) { return binary_import_multi_core(strm, ZDD::getnode_raw, ignore_type ? 0 : 3); }
std::vector<bddp> zdd_import_binary_multi(std::istream& strm, bool ignore_type) { return binary_import_multi_core(strm, ZDD::getnode_raw, ignore_type ? 0 : 3); }
std::vector<bddp> qdd_import_binary_multi(FILE* strm, bool ignore_type) { return binary_import_multi_core(strm, QDD::getnode_raw, ignore_type ? 0 : 1); }
std::vector<bddp> qdd_import_binary_multi(std::istream& strm, bool ignore_type) { return binary_import_multi_core(strm, QDD::getnode_raw, ignore_type ? 0 : 1); }

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

// --- Graphillion format save/load ---

// Write a Graphillion child reference: "B", "T", or the assigned node ID.
static void graphillion_arc_str(bddp arc,
                                 const std::unordered_map<bddp, uint64_t>& id_map,
                                 char* buf, size_t bufsize) {
    if (arc == bddempty) {
        buf[0] = 'B'; buf[1] = '\0';
    } else if (arc == bddsingle) {
        buf[0] = 'T'; buf[1] = '\0';
    } else {
        std::unordered_map<bddp, uint64_t>::const_iterator it = id_map.find(arc);
        std::snprintf(buf, bufsize, "%" PRIu64, it->second);
    }
}

template<typename Stream>
static void graphillion_export_core(Stream& strm, bddp f, int offset) {
    if (f == bddnull)
        throw std::invalid_argument("zdd_export_graphillion: null node");

    // Terminal cases
    if (f == bddempty) { write_str(strm, "B\n.\n"); return; }
    if (f == bddsingle) { write_str(strm, "T\n.\n"); return; }

    // Iterative post-order DFS with complement edge expansion.
    // Key: bddp value (including complement bit) → assigned Graphillion node ID.
    std::unordered_map<bddp, uint64_t> id_map;
    std::vector<bddp> order;  // post-order list of expanded non-terminal bddps
    bddvar max_level = 0;

    // Stack: (bddp, expanded_flag)
    std::vector<std::pair<bddp, bool>> stack;
    stack.push_back(std::make_pair(f, false));

    while (!stack.empty()) {
        bddp node = stack.back().first;
        bool expanded = stack.back().second;

        if (expanded) {
            stack.pop_back();
            if (id_map.count(node)) continue;
            uint64_t gid = order.size();
            id_map[node] = gid;
            order.push_back(node);
            bddp raw = node & ~BDD_COMP_FLAG;
            bddvar v = node_var(raw);
            bddvar lev = var2level[v];
            if (lev > max_level) max_level = lev;
            continue;
        }
        stack.back().second = true;

        if (id_map.count(node)) { stack.pop_back(); continue; }

        // Resolve children with ZDD complement semantics
        bddp raw = node & ~BDD_COMP_FLAG;
        bool comp = (node & BDD_COMP_FLAG) != 0;
        bddp lo = node_lo(raw);
        bddp hi = node_hi(raw);
        if (comp) lo = bddnot(lo);  // ZDD: only lo affected

        // Push hi first so lo is processed first (stack order)
        if (!(hi & BDD_CONST_FLAG) && !id_map.count(hi)) {
            stack.push_back(std::make_pair(hi, false));
        }
        if (!(lo & BDD_CONST_FLAG) && !id_map.count(lo)) {
            stack.push_back(std::make_pair(lo, false));
        }
    }

    // Write nodes in post-order
    bddvar N = max_level;
    char buf[128], buf0[ARC_BUF_SIZE], buf1[ARC_BUF_SIZE];
    for (size_t i = 0; i < order.size(); i++) {
        bddp node = order[i];
        bddp raw = node & ~BDD_COMP_FLAG;
        bool comp = (node & BDD_COMP_FLAG) != 0;

        bddvar v = node_var(raw);
        bddvar lev = var2level[v];
        int g_var = static_cast<int>(N) + 1 - static_cast<int>(lev) + offset;

        bddp lo = node_lo(raw);
        bddp hi = node_hi(raw);
        if (comp) lo = bddnot(lo);

        uint64_t gid = id_map[node];
        graphillion_arc_str(lo, id_map, buf0, sizeof(buf0));
        graphillion_arc_str(hi, id_map, buf1, sizeof(buf1));

        std::snprintf(buf, sizeof(buf), "%" PRIu64 " %d %s %s\n",
                     gid, g_var, buf0, buf1);
        write_str(strm, buf);
    }
    write_str(strm, ".\n");

    if (stream_has_error(strm)) {
        throw std::runtime_error("zdd_export_graphillion: write error");
    }
}

void zdd_export_graphillion(FILE* strm, bddp f, int offset) {
    graphillion_export_core(strm, f, offset);
}

void zdd_export_graphillion(std::ostream& strm, bddp f, int offset) {
    graphillion_export_core(strm, f, offset);
}

// --- Graphillion format import ---

template<typename Stream>
static bddp graphillion_import_core(Stream& strm, int offset) {
    struct NodeLine {
        uint64_t id;
        unsigned g_var;
        std::string lo_str;
        std::string hi_str;
    };

    std::vector<NodeLine> lines;
    bddp root = bddempty;  // default for terminal-only files
    unsigned max_gvar = 0;

    // Read tokens until "."
    while (true) {
        char tok[ARC_BUF_SIZE];
        if (!read_token(strm, tok))
            throw std::runtime_error("zdd_import_graphillion: unexpected end of input");

        std::string s(tok);
        if (s == ".") break;
        if (s == "B") { root = bddempty; continue; }
        if (s == "T") { root = bddsingle; continue; }

        // Non-terminal node: tok is the node ID
        NodeLine nl;
        char* endptr;
        nl.id = std::strtoull(tok, &endptr, 10);
        if (endptr == tok || *endptr != '\0')
            throw std::runtime_error("zdd_import_graphillion: invalid node ID");

        // Read variable number
        char vbuf[ARC_BUF_SIZE];
        if (!read_token(strm, vbuf))
            throw std::runtime_error("zdd_import_graphillion: missing variable number");
        nl.g_var = static_cast<unsigned>(std::strtoul(vbuf, &endptr, 10));
        if (endptr == vbuf || *endptr != '\0' || nl.g_var == 0)
            throw std::runtime_error("zdd_import_graphillion: invalid variable number");

        // Read lo child
        char lo_buf[ARC_BUF_SIZE];
        if (!read_token(strm, lo_buf))
            throw std::runtime_error("zdd_import_graphillion: missing lo child");
        nl.lo_str = lo_buf;

        // Read hi child
        char hi_buf[ARC_BUF_SIZE];
        if (!read_token(strm, hi_buf))
            throw std::runtime_error("zdd_import_graphillion: missing hi child");
        nl.hi_str = hi_buf;

        if (nl.g_var > max_gvar) max_gvar = nl.g_var;
        lines.push_back(nl);
    }

    if (lines.empty()) return root;  // terminal-only

    unsigned N = max_gvar;
    if (N > IMPORT_MAX_LEVEL_LIMIT)
        throw std::runtime_error("zdd_import_graphillion: max variable exceeds limit");

    // Determine max level needed and ensure variables exist
    // max level = N + 1 - 1 + offset = N + offset (for g_var=1, nearest root)
    int max_level_signed = static_cast<int>(N) + offset;
    if (max_level_signed < 1)
        throw std::runtime_error("zdd_import_graphillion: offset too negative (max_level < 1)");
    bddvar max_level = static_cast<bddvar>(max_level_signed);
    while (bddvarused() < max_level) bddnewvar();

    // Map: file node ID (string) → internal bddp
    std::unordered_map<uint64_t, bddp> id_map;

    for (size_t i = 0; i < lines.size(); i++) {
        const NodeLine& nl = lines[i];
        int level = static_cast<int>(N) + 1 - static_cast<int>(nl.g_var) + offset;
        if (level < 1)
            throw std::runtime_error("zdd_import_graphillion: computed level < 1");
        bddvar var = bddvaroflev(static_cast<bddvar>(level));

        // Resolve lo
        bddp lo;
        if (nl.lo_str == "B") {
            lo = bddempty;
        } else if (nl.lo_str == "T") {
            lo = bddsingle;
        } else {
            char* ep;
            uint64_t lo_id = std::strtoull(nl.lo_str.c_str(), &ep, 10);
            if (ep == nl.lo_str.c_str() || *ep != '\0')
                throw std::runtime_error("zdd_import_graphillion: invalid lo child ID");
            std::unordered_map<uint64_t, bddp>::const_iterator it = id_map.find(lo_id);
            if (it == id_map.end())
                throw std::runtime_error("zdd_import_graphillion: undefined lo node reference");
            lo = it->second;
        }

        // Resolve hi
        bddp hi;
        if (nl.hi_str == "B") {
            hi = bddempty;
        } else if (nl.hi_str == "T") {
            hi = bddsingle;
        } else {
            char* ep;
            uint64_t hi_id = std::strtoull(nl.hi_str.c_str(), &ep, 10);
            if (ep == nl.hi_str.c_str() || *ep != '\0')
                throw std::runtime_error("zdd_import_graphillion: invalid hi child ID");
            std::unordered_map<uint64_t, bddp>::const_iterator it = id_map.find(hi_id);
            if (it == id_map.end())
                throw std::runtime_error("zdd_import_graphillion: undefined hi node reference");
            hi = it->second;
        }

        if (id_map.count(nl.id))
            throw std::runtime_error("zdd_import_graphillion: duplicate node ID");
        bddp node = ZDD::getnode_raw(var, lo, hi);
        id_map[nl.id] = node;
        root = node;  // last node is root
    }

    return root;
}

bddp zdd_import_graphillion(FILE* strm, int offset) {
    return graphillion_import_core(strm, offset);
}

bddp zdd_import_graphillion(std::istream& strm, int offset) {
    return graphillion_import_core(strm, offset);
}

// ---------------------------------------------------------------------------
// Graphviz DOT export
// ---------------------------------------------------------------------------

struct BddChildResolver {
    void operator()(bddp raw, bool comp, bddp& lo, bddp& hi) const {
        lo = node_lo(raw);
        hi = node_hi(raw);
        if (comp) { lo = bddnot(lo); hi = bddnot(hi); }
    }
};

struct ZddChildResolver {
    void operator()(bddp raw, bool comp, bddp& lo, bddp& hi) const {
        lo = node_lo(raw);
        hi = node_hi(raw);
        if (comp) { lo = bddnot(lo); }
    }
};

// Node info collected during traversal.
struct GvNode {
    bddp key;    // node key (expanded: full bddp; raw: stripped bddp)
    bddvar var;  // variable number
    int level;   // level in DD
    bddp lo;     // lo child key (in the same key-space as key)
    bddp hi;     // hi child key
    bool lo_comp; // lo edge is complement (raw mode only)
    bool hi_comp; // hi edge is complement (raw mode only)
};

// Collect nodes for Expanded mode. Each unique bddp (including complement
// bit) is a separate logical node.
template<typename ChildResolver>
static void graphviz_collect_expanded(
    bddp f, ChildResolver resolve,
    std::vector<GvNode>& nodes,
    std::unordered_map<bddp, size_t>& id_map,
    bool& has_false, bool& has_true)
{
    // Iterative DFS
    std::vector<std::pair<bddp, bool>> stack;
    stack.push_back({f, false});
    while (!stack.empty()) {
        auto& top = stack.back();
        bddp p = top.first;
        bool expanded = top.second;

        if (p & BDD_CONST_FLAG) {
            stack.pop_back();
            if (p == bddfalse) has_false = true;
            else has_true = true;
            continue;
        }

        if (id_map.count(p)) {
            stack.pop_back();
            continue;
        }

        if (!expanded) {
            top.second = true;
            bddp raw = p & ~BDD_COMP_FLAG;
            bool comp = (p & BDD_COMP_FLAG) != 0;
            bddp lo, hi;
            resolve(raw, comp, lo, hi);
            // Push children first (they need to be visited before this node)
            if (!(hi & BDD_CONST_FLAG) && !id_map.count(hi))
                stack.push_back({hi, false});
            if (!(lo & BDD_CONST_FLAG) && !id_map.count(lo))
                stack.push_back({lo, false});
        } else {
            stack.pop_back();
            bddp raw = p & ~BDD_COMP_FLAG;
            bool comp = (p & BDD_COMP_FLAG) != 0;
            bddp lo, hi;
            resolve(raw, comp, lo, hi);

            size_t idx = nodes.size();
            id_map[p] = idx;
            GvNode gn;
            gn.key = p;
            gn.var = node_var(raw);
            gn.level = static_cast<int>(bddlevofvar(gn.var));
            gn.lo = lo;
            gn.hi = hi;
            gn.lo_comp = false;
            gn.hi_comp = false;
            nodes.push_back(gn);
        }
    }
}

// Collect nodes for Raw mode. Physical DAG nodes only (complement bit
// stripped). Complement edges tracked separately.
static void graphviz_collect_raw(
    bddp f,
    std::vector<GvNode>& nodes,
    std::unordered_map<bddp, size_t>& id_map,
    bool& has_false, bool& has_true)
{
    // Iterative DFS
    std::vector<std::pair<bddp, bool>> stack;
    bddp root = f & ~BDD_COMP_FLAG;
    stack.push_back({root, false});
    while (!stack.empty()) {
        auto& top = stack.back();
        bddp p = top.first; // always stripped (no complement bit)
        bool expanded = top.second;

        if (p & BDD_CONST_FLAG) {
            stack.pop_back();
            if (p == bddfalse) has_false = true;
            else has_true = true;
            continue;
        }

        if (id_map.count(p)) {
            stack.pop_back();
            continue;
        }

        if (!expanded) {
            top.second = true;
            bddp lo = node_lo(p);
            bddp hi = node_hi(p);
            // For terminals, bit 0 is value not complement; don't strip it
            bddp lo_raw = (lo & BDD_CONST_FLAG) ? lo : (lo & ~BDD_COMP_FLAG);
            bddp hi_raw = (hi & BDD_CONST_FLAG) ? hi : (hi & ~BDD_COMP_FLAG);
            if (!(hi_raw & BDD_CONST_FLAG) && !id_map.count(hi_raw))
                stack.push_back({hi_raw, false});
            if (!(lo_raw & BDD_CONST_FLAG) && !id_map.count(lo_raw))
                stack.push_back({lo_raw, false});
        } else {
            stack.pop_back();
            bddp lo = node_lo(p);
            bddp hi = node_hi(p);

            size_t idx = nodes.size();
            id_map[p] = idx;
            GvNode gn;
            gn.key = p;
            gn.var = node_var(p);
            gn.level = static_cast<int>(bddlevofvar(gn.var));
            // For terminals, bit 0 is part of the value, not complement flag
            bool lo_is_term = (lo & BDD_CONST_FLAG) != 0;
            bool hi_is_term = (hi & BDD_CONST_FLAG) != 0;
            gn.lo = lo_is_term ? lo : (lo & ~BDD_COMP_FLAG);
            gn.hi = hi_is_term ? hi : (hi & ~BDD_COMP_FLAG);
            gn.lo_comp = !lo_is_term && (lo & BDD_COMP_FLAG) != 0;
            gn.hi_comp = !hi_is_term && (hi & BDD_COMP_FLAG) != 0;
            nodes.push_back(gn);
        }
    }
}

template<typename Stream, typename ChildResolver>
static void graphviz_core(Stream& strm, bddp f, bool expanded,
                          ChildResolver resolve)
{
    if (f == bddnull) return;

    char buf[256];

    // Terminal-only case
    if (f & BDD_CONST_FLAG) {
        write_str(strm, "digraph {\n");
        if (f == bddfalse) {
            write_str(strm, "\tt0 [label = \"0\", shape = box, "
                "style = filled, color = \"#81B65D\", "
                "fillcolor = \"#F6FAF4\", penwidth = 2.5, "
                "width = 0.4, height = 0.6, fontsize = 24];\n");
        } else {
            write_str(strm, "\tt1 [label = \"1\", shape = box, "
                "style = filled, color = \"#81B65D\", "
                "fillcolor = \"#F6FAF4\", penwidth = 2.5, "
                "width = 0.4, height = 0.6, fontsize = 24];\n");
        }
        write_str(strm, "}\n");
        return;
    }

    // Collect nodes
    std::vector<GvNode> nodes;
    std::unordered_map<bddp, size_t> id_map;
    bool has_false = false, has_true = false;

    if (expanded) {
        graphviz_collect_expanded(f, resolve, nodes, id_map, has_false, has_true);
    } else {
        graphviz_collect_raw(f, nodes, id_map, has_false, has_true);
    }

    // Group nodes by level (descending)
    int max_level = 0;
    for (const auto& gn : nodes) {
        if (gn.level > max_level) max_level = gn.level;
    }
    std::vector<std::vector<size_t>> levels(max_level + 1);
    for (size_t i = 0; i < nodes.size(); ++i) {
        levels[nodes[i].level].push_back(i);
    }

    // Emit DOT
    write_str(strm, "digraph {\n");

    // Terminal nodes
    if (has_false) {
        write_str(strm, "\tt0 [label = \"0\", shape = box, "
            "style = filled, color = \"#81B65D\", "
            "fillcolor = \"#F6FAF4\", penwidth = 2.5, "
            "width = 0.4, height = 0.6, fontsize = 24];\n");
    }
    if (has_true) {
        write_str(strm, "\tt1 [label = \"1\", shape = box, "
            "style = filled, color = \"#81B65D\", "
            "fillcolor = \"#F6FAF4\", penwidth = 2.5, "
            "width = 0.4, height = 0.6, fontsize = 24];\n");
    }

    // Raw mode: entry point for complement root
    bool root_comp = !expanded && (f & BDD_COMP_FLAG) != 0;
    if (root_comp) {
        write_str(strm, "\tentry [shape = point];\n");
        bddp root_raw = f & ~BDD_COMP_FLAG;
        auto it = id_map.find(root_raw);
        if (it != id_map.end()) {
            std::snprintf(buf, sizeof(buf),
                "\tentry -> n%u [arrowtail = odot, dir = both, "
                "color = \"#81B65D\", penwidth = 2.5];\n",
                static_cast<unsigned>(it->second));
            write_str(strm, buf);
        }
    }

    // Non-terminal nodes and edges (level by level, top to bottom)
    for (int lev = max_level; lev >= 1; --lev) {
        for (size_t idx : levels[lev]) {
            const GvNode& gn = nodes[idx];
            // Node definition
            std::snprintf(buf, sizeof(buf),
                "\tn%u [shape = circle, style = filled, "
                "color = \"#81B65D\", fillcolor = \"#F6FAF4\", "
                "penwidth = 2.5, label = \"%u\", fontsize = 18];\n",
                static_cast<unsigned>(idx),
                static_cast<unsigned>(gn.var));
            write_str(strm, buf);

            // Lo edge (0-child): dotted
            if (gn.lo & BDD_CONST_FLAG) {
                const char* tgt = (gn.lo == bddfalse) ? "t0" : "t1";
                if (gn.lo_comp) {
                    std::snprintf(buf, sizeof(buf),
                        "\tn%u -> %s [style = dotted, color = \"#81B65D\", "
                        "penwidth = 2.5, arrowtail = odot, dir = both];\n",
                        static_cast<unsigned>(idx), tgt);
                } else {
                    std::snprintf(buf, sizeof(buf),
                        "\tn%u -> %s [style = dotted, color = \"#81B65D\", "
                        "penwidth = 2.5];\n",
                        static_cast<unsigned>(idx), tgt);
                }
            } else {
                auto it = id_map.find(gn.lo);
                if (it != id_map.end()) {
                    if (gn.lo_comp) {
                        std::snprintf(buf, sizeof(buf),
                            "\tn%u -> n%u [style = dotted, color = \"#81B65D\", "
                            "penwidth = 2.5, arrowtail = odot, dir = both];\n",
                            static_cast<unsigned>(idx),
                            static_cast<unsigned>(it->second));
                    } else {
                        std::snprintf(buf, sizeof(buf),
                            "\tn%u -> n%u [style = dotted, color = \"#81B65D\", "
                            "penwidth = 2.5];\n",
                            static_cast<unsigned>(idx),
                            static_cast<unsigned>(it->second));
                    }
                }
            }
            write_str(strm, buf);

            // Hi edge (1-child): solid
            if (gn.hi & BDD_CONST_FLAG) {
                const char* tgt = (gn.hi == bddfalse) ? "t0" : "t1";
                if (gn.hi_comp) {
                    std::snprintf(buf, sizeof(buf),
                        "\tn%u -> %s [color = \"#81B65D\", "
                        "penwidth = 2.5, arrowtail = odot, dir = both];\n",
                        static_cast<unsigned>(idx), tgt);
                } else {
                    std::snprintf(buf, sizeof(buf),
                        "\tn%u -> %s [color = \"#81B65D\", "
                        "penwidth = 2.5];\n",
                        static_cast<unsigned>(idx), tgt);
                }
            } else {
                auto it = id_map.find(gn.hi);
                if (it != id_map.end()) {
                    if (gn.hi_comp) {
                        std::snprintf(buf, sizeof(buf),
                            "\tn%u -> n%u [color = \"#81B65D\", "
                            "penwidth = 2.5, arrowtail = odot, dir = both];\n",
                            static_cast<unsigned>(idx),
                            static_cast<unsigned>(it->second));
                    } else {
                        std::snprintf(buf, sizeof(buf),
                            "\tn%u -> n%u [color = \"#81B65D\", "
                            "penwidth = 2.5];\n",
                            static_cast<unsigned>(idx),
                            static_cast<unsigned>(it->second));
                    }
                }
            }
            write_str(strm, buf);
        }

        // Rank constraint for this level
        if (!levels[lev].empty()) {
            std::string rank = "\t{rank = same; ";
            for (size_t i = 0; i < levels[lev].size(); ++i) {
                char nbuf[32];
                std::snprintf(nbuf, sizeof(nbuf), "n%u; ",
                    static_cast<unsigned>(levels[lev][i]));
                rank += nbuf;
            }
            rank += "}\n";
            write_str(strm, rank.c_str());
        }
    }

    // Terminal rank constraint
    if (has_false && has_true) {
        write_str(strm, "\t{rank = same; t0; t1; }\n");
    } else if (has_false) {
        write_str(strm, "\t{rank = same; t0; }\n");
    } else if (has_true) {
        write_str(strm, "\t{rank = same; t1; }\n");
    }

    write_str(strm, "}\n");
}

// Public API wrappers
void bdd_save_graphviz(FILE* strm, bddp f, DrawMode mode) {
    graphviz_core(strm, f, mode == DrawMode::Expanded, BddChildResolver{});
}

void bdd_save_graphviz(std::ostream& strm, bddp f, DrawMode mode) {
    graphviz_core(strm, f, mode == DrawMode::Expanded, BddChildResolver{});
}

void zdd_save_graphviz(FILE* strm, bddp f, DrawMode mode) {
    graphviz_core(strm, f, mode == DrawMode::Expanded, ZddChildResolver{});
}

void zdd_save_graphviz(std::ostream& strm, bddp f, DrawMode mode) {
    graphviz_core(strm, f, mode == DrawMode::Expanded, ZddChildResolver{});
}

// Obsolete graph functions: retained for API compatibility.
void bddgraph0(bddp f) {
    (void)f;
    throw std::logic_error("bddgraph0: obsolete — BDD/ZDD share the same node table");
}

void bddgraph(bddp f) {
    (void)f;
    throw std::logic_error("bddgraph: obsolete — BDD/ZDD share the same node table");
}

void bddvgraph0(bddp* ptr, int lim) {
    (void)ptr; (void)lim;
    throw std::logic_error("bddvgraph0: obsolete — BDD/ZDD share the same node table");
}

void bddvgraph(bddp* ptr, int lim) {
    (void)ptr; (void)lim;
    throw std::logic_error("bddvgraph: obsolete — BDD/ZDD share the same node table");
}

} // namespace kyotodd
