#include "bdd_io_common.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace kyotodd {

// --- Knuth format save/load (obsolete) ---

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

} // namespace kyotodd
