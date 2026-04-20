#include "bdd_io_common.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kyotodd {

// --- Binary format save/load ---

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
    // BDD/ZDD/QDD always have exactly two terminals (0-term, 1-term).
    // Reject early so we do not allocate buffers sized from a malformed count.
    if ((dd_type == 1 || dd_type == 2 || dd_type == 3) && num_terminals != 2)
        throw std::runtime_error(
            "binary import: BDD/ZDD/QDD require num_terminals == 2");
    if (bits_for_id == 0 || bits_for_id % 8 != 0 || bits_for_id > 64)
        throw std::runtime_error(
            "binary import: bits_for_id must be a multiple of 8 and at most 64");
    if (max_level > bdd_node_max)
        throw std::runtime_error("binary import: max_level exceeds node limit");
    if (max_level > IMPORT_MAX_LEVEL_LIMIT)
        throw std::runtime_error(
            "binary import: max_level exceeds import safety limit");
    if (num_roots > bdd_node_max)
        throw std::runtime_error("binary import: num_roots exceeds node limit");
    if (num_roots > IMPORT_MAX_NODE_COUNT)
        throw std::runtime_error(
            "binary import: num_roots exceeds import safety limit");

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
    if (total_nodes > IMPORT_MAX_NODE_COUNT)
        throw std::runtime_error(
            "binary import: total_nodes exceeds import safety limit");

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

} // namespace kyotodd
