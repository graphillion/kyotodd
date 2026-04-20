#include "bdd_io_common.h"
#include <unordered_map>
#include <vector>

namespace kyotodd {

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
        if (g_var <= 0) {
            // Graphillion reserves non-positive IDs (0 marks terminals at
            // import time); guarantee exported files are round-trippable.
            throw std::invalid_argument(
                "zdd_export_graphillion: offset produces non-positive g_var");
        }

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

} // namespace kyotodd
