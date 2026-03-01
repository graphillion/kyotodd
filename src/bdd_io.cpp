#include "bdd.h"
#include "bdd_internal.h"
#include <cinttypes>
#include <cstdlib>
#include <istream>
#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

static void export_collect(bddp f, std::unordered_set<bddp>& visited,
                           std::vector<bddp>& order) {
    if (f == bddnull) return;
    if (f & BDD_CONST_FLAG) return;
    bddp node = f & ~BDD_COMP_FLAG;
    if (!visited.insert(node).second) return;
    export_collect(node_lo(node), visited, order);
    export_collect(node_hi(node), visited, order);
    order.push_back(node);
}

// Format an arc value as a string for the export file.
// Terminal: "F" or "T". Non-terminal: node ID (even) or node ID | 1 (odd=negated).
static void export_arc_str(bddp arc, char* buf, size_t bufsize) {
    if (arc == bddfalse) {
        buf[0] = 'F'; buf[1] = '\0';
    } else if (arc == bddtrue) {
        buf[0] = 'T'; buf[1] = '\0';
    } else {
        // Non-terminal: strip the KyotoDD constant flag is not set here.
        // complement bit (bit 0) maps directly to odd/even in the output.
        bddp node = arc & ~BDD_COMP_FLAG;
        bool comp = (arc & BDD_COMP_FLAG) != 0;
        uint64_t id = comp ? (node | 1) : node;
        std::snprintf(buf, bufsize, "%" PRIu64, id);
    }
}

// Core export logic writing to FILE*.
static void export_core(FILE* strm, bddp* p, int lim) {
    if (lim <= 0 || !p || !strm) return;
    // Collect all nodes in post-order
    std::unordered_set<bddp> visited;
    std::vector<bddp> order;
    bddvar max_level = 0;
    for (int i = 0; i < lim; i++) {
        export_collect(p[i], visited, order);
        // Track max level
        if (p[i] != bddnull && !(p[i] & BDD_CONST_FLAG)) {
            bddvar v = node_var(p[i]);  // strips complement internally
            bddvar lev = var2level[v];
            if (lev > max_level) max_level = lev;
        }
    }

    // Also check max level among all collected nodes
    for (size_t i = 0; i < order.size(); i++) {
        bddvar v = node_var(order[i]);
        bddvar lev = var2level[v];
        if (lev > max_level) max_level = lev;
    }

    // Header
    std::fprintf(strm, "_i %u\n", static_cast<unsigned>(max_level));
    std::fprintf(strm, "_o %d\n", lim);
    std::fprintf(strm, "_n %u\n", static_cast<unsigned>(order.size()));

    // Node section
    char buf0[32], buf1[32];
    for (size_t i = 0; i < order.size(); i++) {
        bddp node = order[i];
        bddvar v = node_var(node);
        bddvar lev = var2level[v];
        bddp lo = node_lo(node);
        bddp hi = node_hi(node);
        export_arc_str(lo, buf0, sizeof(buf0));
        export_arc_str(hi, buf1, sizeof(buf1));
        std::fprintf(strm, "%" PRIu64 " %u %s %s\n",
                     static_cast<uint64_t>(node),
                     static_cast<unsigned>(lev), buf0, buf1);
    }

    // Root section
    for (int i = 0; i < lim; i++) {
        if (p[i] == bddnull) {
            std::fprintf(strm, "N\n");
        } else if (p[i] == bddfalse) {
            std::fprintf(strm, "F\n");
        } else if (p[i] == bddtrue) {
            std::fprintf(strm, "T\n");
        } else {
            bddp node = p[i] & ~BDD_COMP_FLAG;
            bool comp = (p[i] & BDD_COMP_FLAG) != 0;
            uint64_t id = comp ? (node | 1) : node;
            std::fprintf(strm, "%" PRIu64 "\n", id);
        }
    }
}

// Core export logic writing to std::ostream.
static void export_core(std::ostream& strm, bddp* p, int lim) {
    if (lim <= 0 || !p) return;
    std::unordered_set<bddp> visited;
    std::vector<bddp> order;
    bddvar max_level = 0;
    for (int i = 0; i < lim; i++) {
        export_collect(p[i], visited, order);
        if (p[i] != bddnull && !(p[i] & BDD_CONST_FLAG)) {
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

    // Header
    strm << "_i " << max_level << "\n";
    strm << "_o " << lim << "\n";
    strm << "_n " << order.size() << "\n";

    // Node section
    for (size_t i = 0; i < order.size(); i++) {
        bddp node = order[i];
        bddvar v = node_var(node);
        bddvar lev = var2level[v];
        bddp lo = node_lo(node);
        bddp hi = node_hi(node);

        strm << node << " " << lev << " ";
        if (lo == bddfalse) strm << "F";
        else if (lo == bddtrue) strm << "T";
        else strm << static_cast<uint64_t>(lo);  // lo is always non-complemented
        strm << " ";
        if (hi == bddfalse) strm << "F";
        else if (hi == bddtrue) strm << "T";
        else {
            bddp hn = hi & ~BDD_COMP_FLAG;
            bool hc = (hi & BDD_COMP_FLAG) != 0;
            strm << static_cast<uint64_t>(hc ? (hn | 1) : hn);
        }
        strm << "\n";
    }

    // Root section
    for (int i = 0; i < lim; i++) {
        if (p[i] == bddnull) {
            strm << "N\n";
        } else if (p[i] == bddfalse) {
            strm << "F\n";
        } else if (p[i] == bddtrue) {
            strm << "T\n";
        } else {
            bddp node = p[i] & ~BDD_COMP_FLAG;
            bool comp = (p[i] & BDD_COMP_FLAG) != 0;
            strm << static_cast<uint64_t>(comp ? (node | 1) : node) << "\n";
        }
    }
}

void bddexport(FILE* strm, bddp* p, int lim) {
    export_core(strm, p, lim);
}

void bddexport(FILE* strm, const std::vector<bddp>& v) {
    std::vector<bddp> tmp(v);
    export_core(strm, tmp.data(), static_cast<int>(tmp.size()));
}

void bddexport(std::ostream& strm, bddp* p, int lim) {
    export_core(strm, p, lim);
}

void bddexport(std::ostream& strm, const std::vector<bddp>& v) {
    std::vector<bddp> tmp(v);
    export_core(strm, tmp.data(), static_cast<int>(tmp.size()));
}

// --- bddimport / bddimportz ---

typedef bddp (*import_nodefn_t)(bddvar, bddp, bddp);

static bddp import_parse_arc(const char* s,
                              const std::unordered_map<uint64_t, bddp>& id_map) {
    if (s[0] == 'N') return bddnull;
    if (s[0] == 'F') return bddfalse;
    if (s[0] == 'T') return bddtrue;
    uint64_t val = std::strtoull(s, nullptr, 10);
    bool comp = (val & 1) != 0;
    uint64_t node_id = comp ? (val - 1) : val;
    std::unordered_map<uint64_t, bddp>::const_iterator it = id_map.find(node_id);
    if (it == id_map.end()) return bddnull;
    return comp ? bddnot(it->second) : it->second;
}

static int import_core(FILE* strm, std::vector<bddp>& result,
                       import_nodefn_t make_node) {
    unsigned max_level, output_count, node_count;
    if (std::fscanf(strm, " _i %u", &max_level) != 1) return -1;
    if (std::fscanf(strm, " _o %u", &output_count) != 1) return -1;
    if (std::fscanf(strm, " _n %u", &node_count) != 1) return -1;

    while (bddvarused() < max_level) bddnewvar();

    std::unordered_map<uint64_t, bddp> id_map;
    char buf0[32], buf1[32];
    for (unsigned i = 0; i < node_count; i++) {
        uint64_t old_id;
        unsigned level;
        if (std::fscanf(strm, " %" SCNu64 " %u %31s %31s",
                        &old_id, &level, buf0, buf1) != 4) return -1;
        bddp lo = import_parse_arc(buf0, id_map);
        bddp hi = import_parse_arc(buf1, id_map);
        if (lo == bddnull || hi == bddnull) return -1;
        bddvar var = bddvaroflev(level);
        bddp new_node = make_node(var, lo, hi);
        id_map[old_id] = new_node;
    }

    result.resize(output_count);
    for (unsigned i = 0; i < output_count; i++) {
        char ref[32];
        if (std::fscanf(strm, " %31s", ref) != 1) return -1;
        bddp r = import_parse_arc(ref, id_map);
        if (r == bddnull) return -1;
        result[i] = r;
    }

    return static_cast<int>(output_count);
}

static int import_core(std::istream& strm, std::vector<bddp>& result,
                       import_nodefn_t make_node) {
    std::string tag;
    unsigned max_level, output_count, node_count;
    if (!(strm >> tag >> max_level) || tag != "_i") return -1;
    if (!(strm >> tag >> output_count) || tag != "_o") return -1;
    if (!(strm >> tag >> node_count) || tag != "_n") return -1;

    while (bddvarused() < max_level) bddnewvar();

    std::unordered_map<uint64_t, bddp> id_map;
    for (unsigned i = 0; i < node_count; i++) {
        uint64_t old_id;
        unsigned level;
        std::string s0, s1;
        if (!(strm >> old_id >> level >> s0 >> s1)) return -1;
        bddp lo = import_parse_arc(s0.c_str(), id_map);
        bddp hi = import_parse_arc(s1.c_str(), id_map);
        if (lo == bddnull || hi == bddnull) return -1;
        bddvar var = bddvaroflev(level);
        bddp new_node = make_node(var, lo, hi);
        id_map[old_id] = new_node;
    }

    result.resize(output_count);
    for (unsigned i = 0; i < output_count; i++) {
        std::string ref;
        if (!(strm >> ref)) return -1;
        bddp r = import_parse_arc(ref.c_str(), id_map);
        if (r == bddnull) return -1;
        result[i] = r;
    }

    return static_cast<int>(output_count);
}

int bddimport(FILE* strm, bddp* p, int lim) {
    if (lim <= 0 || !p) return 0;
    std::vector<bddp> result;
    int ret = import_core(strm, result, getnode);
    if (ret < 0) return ret;
    int count = ret < lim ? ret : lim;
    for (int i = 0; i < count; i++) p[i] = result[i];
    return count;
}

int bddimport(FILE* strm, std::vector<bddp>& v) {
    return import_core(strm, v, getnode);
}

int bddimport(std::istream& strm, bddp* p, int lim) {
    if (lim <= 0 || !p) return 0;
    std::vector<bddp> result;
    int ret = import_core(strm, result, getnode);
    if (ret < 0) return ret;
    int count = ret < lim ? ret : lim;
    for (int i = 0; i < count; i++) p[i] = result[i];
    return count;
}

int bddimport(std::istream& strm, std::vector<bddp>& v) {
    return import_core(strm, v, getnode);
}

int bddimportz(FILE* strm, bddp* p, int lim) {
    if (lim <= 0 || !p) return 0;
    std::vector<bddp> result;
    int ret = import_core(strm, result, getznode);
    if (ret < 0) return ret;
    int count = ret < lim ? ret : lim;
    for (int i = 0; i < count; i++) p[i] = result[i];
    return count;
}

int bddimportz(FILE* strm, std::vector<bddp>& v) {
    return import_core(strm, v, getznode);
}

int bddimportz(std::istream& strm, bddp* p, int lim) {
    if (lim <= 0 || !p) return 0;
    std::vector<bddp> result;
    int ret = import_core(strm, result, getznode);
    if (ret < 0) return ret;
    int count = ret < lim ? ret : lim;
    for (int i = 0; i < count; i++) p[i] = result[i];
    return count;
}

int bddimportz(std::istream& strm, std::vector<bddp>& v) {
    return import_core(strm, v, getznode);
}
