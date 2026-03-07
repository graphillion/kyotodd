#include "bdd.h"
#include "bdd_internal.h"
#include <cinttypes>
#include <cstdlib>
#include <istream>
#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

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

template<typename Stream>
static void export_core(Stream& strm, const bddp* p, int lim) {
    if (lim <= 0 || !p || !stream_valid(strm)) return;
    // Collect all nodes in post-order
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

    char buf[128];
    // Header
    std::snprintf(buf, sizeof(buf), "_i %u\n", static_cast<unsigned>(max_level));
    write_str(strm, buf);
    std::snprintf(buf, sizeof(buf), "_o %d\n", lim);
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
    for (int i = 0; i < lim; i++) {
        if (p[i] == bddnull) {
            write_str(strm, "N\n");
        } else if (p[i] == bddfalse) {
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
}

void bddexport(FILE* strm, const bddp* p, int lim) {
    export_core(strm, p, lim);
}

void bddexport(FILE* strm, const std::vector<bddp>& v) {
    export_core(strm, v.data(), static_cast<int>(v.size()));
}

void bddexport(std::ostream& strm, const bddp* p, int lim) {
    export_core(strm, p, lim);
}

void bddexport(std::ostream& strm, const std::vector<bddp>& v) {
    export_core(strm, v.data(), static_cast<int>(v.size()));
}

// --- bddimport / bddimportz ---

typedef bddp (*import_nodefn_t)(bddvar, bddp, bddp);

static bddp import_parse_arc(const char* s,
                              const std::unordered_map<uint64_t, bddp>& id_map) {
    if (s[0] == 'N') return bddnull;
    if (s[0] == 'F') return bddfalse;
    if (s[0] == 'T') return bddtrue;
    char* endptr;
    uint64_t val = std::strtoull(s, &endptr, 10);
    if (endptr == s) return bddnull;  // no digits parsed
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
