#ifndef KYOTODD_MTBDD_IO_H
#define KYOTODD_MTBDD_IO_H

// Internal header: MTBDD/MTZDD binary export/import, SVG forward
// declarations, and operator templates.
//
// End users should include "mtbdd.h" (umbrella), not this header directly.

#include "mtbdd_class.h"

#include <type_traits>

namespace kyotodd {

// ========================================================================
//  MTBDD/MTZDD binary export/import inline implementations
// ========================================================================

namespace mtbdd_binary_detail {

inline void mb_encode_le16(uint8_t* buf, uint16_t v) {
    buf[0] = v & 0xFF; buf[1] = (v >> 8) & 0xFF;
}
inline void mb_encode_le32(uint8_t* buf, uint32_t v) {
    for (int i = 0; i < 4; i++) buf[i] = (v >> (8*i)) & 0xFF;
}
inline void mb_encode_le64(uint8_t* buf, uint64_t v) {
    for (int i = 0; i < 8; i++) buf[i] = (v >> (8*i)) & 0xFF;
}
inline uint32_t mb_decode_le32(const uint8_t* buf) {
    uint32_t v = 0;
    for (int i = 0; i < 4; i++) v |= static_cast<uint32_t>(buf[i]) << (8*i);
    return v;
}
inline uint64_t mb_decode_le64(const uint8_t* buf) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= static_cast<uint64_t>(buf[i]) << (8*i);
    return v;
}

inline void mb_write_bytes(std::ostream& strm, const void* data, size_t n) {
    strm.write(static_cast<const char*>(data), n);
}
inline bool mb_read_bytes(std::istream& strm, void* data, size_t n) {
    strm.read(static_cast<char*>(data), n);
    return !strm.fail();
}
inline void mb_write_id_le(std::ostream& strm, uint64_t id, int bytes) {
    uint8_t buf[8] = {0};
    for (int i = 0; i < bytes; i++) buf[i] = (id >> (8*i)) & 0xFF;
    mb_write_bytes(strm, buf, bytes);
}
inline uint64_t mb_read_id_le(std::istream& strm, int bytes) {
    // Defense-in-depth: writers only emit bytes ∈ {1, 2, 4, 8}, but a
    // crafted file could still set bits_for_id to an unsupported value.
    // The header validation already rejects such files, but guard the
    // 8-byte stack buffer here as well.
    if (bytes < 1 || bytes > 8)
        throw std::runtime_error("mtbdd binary import: invalid id byte width");
    uint8_t buf[8] = {0};
    if (!mb_read_bytes(strm, buf, bytes))
        throw std::runtime_error("mtbdd binary import: unexpected end of input");
    uint64_t id = 0;
    for (int i = 0; i < bytes; i++) id |= static_cast<uint64_t>(buf[i]) << (8*i);
    return id;
}
inline uint8_t mb_bits_for_max_id(uint64_t max_id) {
    if (max_id <= 0xFF) return 8;
    if (max_id <= 0xFFFF) return 16;
    if (max_id <= 0xFFFFFFFFULL) return 32;
    return 64;
}

// Binary I/O is restricted to trivially-copyable terminal types that fit in
// 8 bytes. Larger or non-trivial T (e.g. std::string) would overflow the
// 8-byte scratch buffer or copy invalid object representations.
template<typename T>
inline void mb_write_value_le(std::ostream& strm, const T& val) {
    static_assert(std::is_trivially_copyable<T>::value,
                  "MTBDD binary I/O requires a trivially-copyable terminal type");
    static_assert(sizeof(T) <= sizeof(uint64_t),
                  "MTBDD binary I/O requires sizeof(T) <= 8");
    uint64_t bits = 0;
    std::memcpy(&bits, &val, sizeof(T));
    uint8_t buf[8];
    for (size_t i = 0; i < sizeof(T); i++) buf[i] = static_cast<uint8_t>((bits >> (8*i)) & 0xFF);
    mb_write_bytes(strm, buf, sizeof(T));
}

template<typename T>
inline T mb_read_value_le(std::istream& strm) {
    static_assert(std::is_trivially_copyable<T>::value,
                  "MTBDD binary I/O requires a trivially-copyable terminal type");
    static_assert(sizeof(T) <= sizeof(uint64_t),
                  "MTBDD binary I/O requires sizeof(T) <= 8");
    uint8_t buf[8] = {0};
    if (!mb_read_bytes(strm, buf, sizeof(T)))
        throw std::runtime_error("mtbdd binary import: truncated terminal values");
    uint64_t bits = 0;
    for (size_t i = 0; i < sizeof(T); i++) bits |= static_cast<uint64_t>(buf[i]) << (8*i);
    T val;
    std::memcpy(&val, &bits, sizeof(T));
    return val;
}

// dd_type constants for MTBDD/MTZDD
static const uint8_t DD_TYPE_MTBDD = 4;
static const uint8_t DD_TYPE_MTZDD = 5;

template<typename T>
inline void mtbdd_export_binary_impl(std::ostream& strm, bddp f, uint8_t dd_type) {
    if (f == bddnull)
        throw std::invalid_argument("mtbdd binary export: null node");

    MTBDDTerminalTable<T>& table = MTBDDTerminalTable<T>::instance();

    // Collect all physical nodes and terminal bddp values via DFS
    std::unordered_set<bddp> visited;
    std::vector<bddp> all_nodes;
    std::unordered_set<bddp> terminal_set;

    std::vector<bddp> stack;
    if (f & BDD_CONST_FLAG) {
        terminal_set.insert(f);
    } else {
        stack.push_back(f);
        visited.insert(f);
    }

    while (!stack.empty()) {
        bddp node = stack.back();
        stack.pop_back();
        all_nodes.push_back(node);
        bddp lo = node_lo(node);
        bddp hi = node_hi(node);
        bddp children[2] = {lo, hi};
        for (int c = 0; c < 2; c++) {
            if (children[c] & BDD_CONST_FLAG) {
                terminal_set.insert(children[c]);
            } else if (visited.insert(children[c]).second) {
                stack.push_back(children[c]);
            }
        }
    }

    // Sort by level ascending
    std::sort(all_nodes.begin(), all_nodes.end(),
              [](bddp a, bddp b) {
                  return var2level[node_var(a)] < var2level[node_var(b)];
              });

    // Build terminal list: index 0 = zero terminal, rest sorted by index
    bddp zero_t = MTBDDTerminalTable<T>::make_terminal(0);
    std::vector<bddp> terminals;
    terminals.push_back(zero_t);
    terminal_set.erase(zero_t);
    // Sort remaining terminals by their terminal table index for determinism
    std::vector<bddp> rest(terminal_set.begin(), terminal_set.end());
    std::sort(rest.begin(), rest.end());
    for (size_t i = 0; i < rest.size(); i++) {
        terminals.push_back(rest[i]);
    }

    uint32_t t = static_cast<uint32_t>(terminals.size());
    std::unordered_map<bddp, uint64_t> terminal_id_map;
    for (uint32_t i = 0; i < t; i++) {
        terminal_id_map[terminals[i]] = i;
    }

    uint64_t total_nodes = all_nodes.size();
    bddvar max_level = 0;
    for (size_t i = 0; i < all_nodes.size(); i++) {
        bddvar lev = var2level[node_var(all_nodes[i])];
        if (lev > max_level) max_level = lev;
    }
    if (max_level == 0 && bddvarused() > 0) max_level = bddvarused();

    // Build non-terminal ID mapping
    std::unordered_map<bddp, uint64_t> id_map;
    for (uint64_t i = 0; i < total_nodes; i++) {
        id_map[all_nodes[i]] = t + i;
    }

    // Map any bddp to binary ID
    auto to_bin_id = [&](bddp edge) -> uint64_t {
        if (edge & BDD_CONST_FLAG) {
            typename std::unordered_map<bddp, uint64_t>::const_iterator it = terminal_id_map.find(edge);
            if (it == terminal_id_map.end())
                throw std::logic_error("mtbdd binary export: terminal not in map");
            return it->second;
        }
        typename std::unordered_map<bddp, uint64_t>::const_iterator it = id_map.find(edge);
        if (it == id_map.end())
            throw std::logic_error("mtbdd binary export: node not in id_map");
        return it->second;
    };

    uint64_t max_id = (t > 0) ? t - 1 : 0;
    if (total_nodes > 0) {
        uint64_t last_id = t + total_nodes - 1;
        if (last_id > max_id) max_id = last_id;
    }
    uint64_t root_bin_id = to_bin_id(f);
    if (root_bin_id > max_id) max_id = root_bin_id;

    uint8_t bits = mb_bits_for_max_id(max_id);
    int id_bytes = bits / 8;

    // --- Write magic ---
    uint8_t magic[3] = {0x42, 0x44, 0x44};
    mb_write_bytes(strm, magic, 3);

    // --- Write header (91 bytes) ---
    uint8_t header[91];
    std::memset(header, 0, sizeof(header));
    header[0] = 1;  // version
    header[1] = dd_type;
    mb_encode_le16(header + 2, 2);   // number_of_arcs
    mb_encode_le32(header + 4, t);   // number_of_terminals
    header[8] = 0;   // bits_for_level (unused)
    header[9] = bits; // bits_for_id
    header[10] = 0;  // use_negative_arcs = 0
    mb_encode_le64(header + 11, max_level);
    mb_encode_le64(header + 19, 1);  // number_of_roots
    header[27] = static_cast<uint8_t>(sizeof(T));  // terminal_value_size
    mb_write_bytes(strm, header, 91);

    // --- Write terminal values ---
    for (uint32_t i = 0; i < t; i++) {
        uint64_t idx = MTBDDTerminalTable<T>::terminal_index(terminals[i]);
        mb_write_value_le<T>(strm, table.get_value(idx));
    }

    // --- Write level counts ---
    std::vector<uint64_t> level_counts(max_level, 0);
    for (size_t i = 0; i < all_nodes.size(); i++) {
        bddvar lev = var2level[node_var(all_nodes[i])];
        level_counts[lev - 1]++;
    }
    for (bddvar l = 0; l < max_level; l++) {
        uint8_t buf[8];
        mb_encode_le64(buf, level_counts[l]);
        mb_write_bytes(strm, buf, 8);
    }

    // --- Write root ID ---
    mb_write_id_le(strm, root_bin_id, id_bytes);

    // --- Write nodes ---
    for (size_t i = 0; i < all_nodes.size(); i++) {
        bddp node = all_nodes[i];
        mb_write_id_le(strm, to_bin_id(node_lo(node)), id_bytes);
        mb_write_id_le(strm, to_bin_id(node_hi(node)), id_bytes);
    }

    if (strm.fail())
        throw std::runtime_error("mtbdd binary export: write error");
}

template<typename T>
inline bddp mtbdd_import_binary_impl(std::istream& strm,
                                      bddp (*make_node)(bddvar, bddp, bddp),
                                      uint8_t expected_type) {
    // --- Read magic ---
    uint8_t magic[3];
    if (!mb_read_bytes(strm, magic, 3) ||
        magic[0] != 0x42 || magic[1] != 0x44 || magic[2] != 0x44)
        throw std::runtime_error("mtbdd binary import: invalid magic bytes");

    // --- Read header ---
    uint8_t header[91];
    if (!mb_read_bytes(strm, header, 91))
        throw std::runtime_error("mtbdd binary import: truncated header");

    uint8_t version = header[0];
    if (version != 1)
        throw std::runtime_error("mtbdd binary import: unsupported version");

    uint8_t dd_type = header[1];
    if (expected_type != 0 && dd_type != expected_type)
        throw std::runtime_error("mtbdd binary import: type mismatch");

    uint32_t num_terminals = mb_decode_le32(header + 4);
    uint8_t bits_for_id = header[9];
    uint64_t max_level = mb_decode_le64(header + 11);
    uint64_t num_roots = mb_decode_le64(header + 19);
    uint8_t terminal_value_size = header[27];

    // Restrict to the writer-supported set {8, 16, 32, 64}. Other values
    // (including 72+) would otherwise overflow the 8-byte stack buffer
    // used by mb_read_id_le.
    if (bits_for_id != 8 && bits_for_id != 16 &&
        bits_for_id != 32 && bits_for_id != 64)
        throw std::runtime_error("mtbdd binary import: invalid bits_for_id");
    if (num_roots < 1)
        throw std::runtime_error("mtbdd binary import: number_of_roots < 1");
    if (terminal_value_size != 0 && terminal_value_size != sizeof(T))
        throw std::runtime_error("mtbdd binary import: terminal_value_size mismatch");

    int id_bytes = bits_for_id / 8;
    uint32_t t = num_terminals;

    // --- Read terminal values ---
    MTBDDTerminalTable<T>& table = MTBDDTerminalTable<T>::instance();
    std::vector<bddp> terminal_bddps(t);
    for (uint32_t i = 0; i < t; i++) {
        T val = mb_read_value_le<T>(strm);
        uint64_t idx = table.get_or_insert(val);
        terminal_bddps[i] = MTBDDTerminalTable<T>::make_terminal(idx);
    }

    // --- Read level counts ---
    std::vector<uint64_t> level_counts(max_level);
    for (uint64_t l = 0; l < max_level; l++) {
        uint8_t buf[8];
        if (!mb_read_bytes(strm, buf, 8))
            throw std::runtime_error("mtbdd binary import: truncated level counts");
        level_counts[l] = mb_decode_le64(buf);
    }

    uint64_t total_nodes = 0;
    for (uint64_t l = 0; l < max_level; l++) total_nodes += level_counts[l];

    // --- Read root ID ---
    uint64_t root_id = mb_read_id_le(strm, id_bytes);

    // --- Read nodes ---
    struct BinNode { uint64_t lo, hi; };
    std::vector<BinNode> bin_nodes(total_nodes);
    for (uint64_t i = 0; i < total_nodes; i++) {
        bin_nodes[i].lo = mb_read_id_le(strm, id_bytes);
        bin_nodes[i].hi = mb_read_id_le(strm, id_bytes);
    }

    // Ensure variables exist
    while (bddvarused() < max_level) bddnewvar();

    // --- Reconstruct nodes (topological sort) ---
    std::unordered_map<uint64_t, bddp> id_map;

    auto is_resolved = [&](uint64_t bin_id) -> bool {
        if (bin_id < t) return true;
        return id_map.count(bin_id) != 0;
    };

    auto resolve = [&](uint64_t bin_id) -> bddp {
        if (bin_id < t) {
            if (bin_id >= terminal_bddps.size())
                throw std::runtime_error("mtbdd binary import: terminal ID out of range");
            return terminal_bddps[static_cast<size_t>(bin_id)];
        }
        typename std::unordered_map<uint64_t, bddp>::const_iterator it = id_map.find(bin_id);
        if (it == id_map.end())
            throw std::runtime_error("mtbdd binary import: undefined node reference");
        return it->second;
    };

    // Pre-compute levels
    std::vector<bddvar> node_levels(total_nodes);
    {
        uint64_t idx = 0;
        for (uint64_t l = 0; l < max_level; l++) {
            for (uint64_t c = 0; c < level_counts[l]; c++) {
                node_levels[idx++] = static_cast<bddvar>(l + 1);
            }
        }
    }

    // Worklist-based topological sort
    std::vector<int> pending(total_nodes, 0);
    std::unordered_map<uint64_t, std::vector<uint64_t> > dependents;
    for (uint64_t i = 0; i < total_nodes; i++) {
        if (!is_resolved(bin_nodes[i].lo)) {
            pending[i]++;
            dependents[bin_nodes[i].lo].push_back(i);
        }
        if (!is_resolved(bin_nodes[i].hi)) {
            pending[i]++;
            dependents[bin_nodes[i].hi].push_back(i);
        }
    }

    std::vector<uint64_t> ready;
    for (uint64_t i = 0; i < total_nodes; i++) {
        if (pending[i] == 0) ready.push_back(i);
    }

    uint64_t created = 0;
    while (!ready.empty()) {
        uint64_t i = ready.back();
        ready.pop_back();

        bddvar var = bddvaroflev(node_levels[i]);
        bddp lo = resolve(bin_nodes[i].lo);
        bddp hi = resolve(bin_nodes[i].hi);
        uint64_t bin_id = t + i;
        id_map[bin_id] = make_node(var, lo, hi);
        created++;

        typename std::unordered_map<uint64_t, std::vector<uint64_t> >::iterator dit = dependents.find(bin_id);
        if (dit != dependents.end()) {
            for (size_t j = 0; j < dit->second.size(); j++) {
                uint64_t dep = dit->second[j];
                if (--pending[dep] == 0) ready.push_back(dep);
            }
        }
    }

    if (created < total_nodes)
        throw std::runtime_error("mtbdd binary import: circular dependency");

    return resolve(root_id);
}

} // namespace mtbdd_binary_detail

// --- MTBDD<T> binary export/import ---

template<typename T>
inline void MTBDD<T>::export_binary(std::ostream& strm) const {
    mtbdd_binary_detail::mtbdd_export_binary_impl<T>(
        strm, root, mtbdd_binary_detail::DD_TYPE_MTBDD);
}

template<typename T>
inline void MTBDD<T>::export_binary(const char* filename) const {
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs)
        throw std::runtime_error("MTBDD::export_binary: cannot open file");
    export_binary(ofs);
}

template<typename T>
inline MTBDD<T> MTBDD<T>::import_binary(std::istream& strm) {
    // Use the validating getnode (not _raw) so that a malformed binary
    // referencing a child at >= the parent's level is rejected instead of
    // creating a non-canonical node.
    bddp p = mtbdd_binary_detail::mtbdd_import_binary_impl<T>(
        strm, mtbdd_getnode, mtbdd_binary_detail::DD_TYPE_MTBDD);
    MTBDD result;
    result.root = p;
    return result;
}

template<typename T>
inline MTBDD<T> MTBDD<T>::import_binary(const char* filename) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs)
        throw std::runtime_error("MTBDD::import_binary: cannot open file");
    return import_binary(ifs);
}

// --- MTZDD<T> binary export/import ---

template<typename T>
inline void MTZDD<T>::export_binary(std::ostream& strm) const {
    mtbdd_binary_detail::mtbdd_export_binary_impl<T>(
        strm, root, mtbdd_binary_detail::DD_TYPE_MTZDD);
}

template<typename T>
inline void MTZDD<T>::export_binary(const char* filename) const {
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs)
        throw std::runtime_error("MTZDD::export_binary: cannot open file");
    export_binary(ofs);
}

template<typename T>
inline MTZDD<T> MTZDD<T>::import_binary(std::istream& strm) {
    // Use the validating getnode (not _raw) so that a malformed binary
    // referencing a child at >= the parent's level is rejected instead of
    // creating a non-canonical node.
    bddp p = mtbdd_binary_detail::mtbdd_import_binary_impl<T>(
        strm, mtzdd_getnode, mtbdd_binary_detail::DD_TYPE_MTZDD);
    MTZDD result;
    result.root = p;
    return result;
}

template<typename T>
inline MTZDD<T> MTZDD<T>::import_binary(const char* filename) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs)
        throw std::runtime_error("MTZDD::import_binary: cannot open file");
    return import_binary(ifs);
}

// ========================================================================
//  MTBDD/MTZDD save_svg inline implementations
// ========================================================================

// Helper: build terminal_name_map by DFS from root.
template<typename T>
static SvgParams mtbdd_build_svg_params(bddp root, const SvgParams& params) {
    SvgParams p = params;
    std::vector<bddp> stack;
    std::unordered_set<bddp> visited;
    stack.push_back(root);
    while (!stack.empty()) {
        bddp f = stack.back();
        stack.pop_back();
        if (f & BDD_CONST_FLAG) {
            if (p.terminal_name_map.count(f) == 0) {
                uint64_t idx = MTBDDTerminalTable<T>::terminal_index(f);
                T val = MTBDDTerminalTable<T>::instance().get_value(idx);
                std::ostringstream oss;
                oss << val;
                p.terminal_name_map[f] = oss.str();
            }
            continue;
        }
        if (!visited.insert(f).second) continue;
        stack.push_back(node_lo(f));
        stack.push_back(node_hi(f));
    }
    return p;
}

template<typename T>
inline void MTBDD<T>::save_svg(const char* filename, const SvgParams& params) const {
    mtbdd_save_svg(filename, root, mtbdd_build_svg_params<T>(root, params));
}
template<typename T>
inline void MTBDD<T>::save_svg(const char* filename) const {
    save_svg(filename, SvgParams());
}
template<typename T>
inline void MTBDD<T>::save_svg(std::ostream& strm, const SvgParams& params) const {
    mtbdd_save_svg(strm, root, mtbdd_build_svg_params<T>(root, params));
}
template<typename T>
inline void MTBDD<T>::save_svg(std::ostream& strm) const {
    save_svg(strm, SvgParams());
}
template<typename T>
inline std::string MTBDD<T>::save_svg(const SvgParams& params) const {
    return mtbdd_save_svg(root, mtbdd_build_svg_params<T>(root, params));
}
template<typename T>
inline std::string MTBDD<T>::save_svg() const {
    return save_svg(SvgParams());
}

template<typename T>
inline void MTZDD<T>::save_svg(const char* filename, const SvgParams& params) const {
    mtzdd_save_svg(filename, root, mtbdd_build_svg_params<T>(root, params));
}
template<typename T>
inline void MTZDD<T>::save_svg(const char* filename) const {
    save_svg(filename, SvgParams());
}
template<typename T>
inline void MTZDD<T>::save_svg(std::ostream& strm, const SvgParams& params) const {
    mtzdd_save_svg(strm, root, mtbdd_build_svg_params<T>(root, params));
}
template<typename T>
inline void MTZDD<T>::save_svg(std::ostream& strm) const {
    save_svg(strm, SvgParams());
}
template<typename T>
inline std::string MTZDD<T>::save_svg(const SvgParams& params) const {
    return mtzdd_save_svg(root, mtbdd_build_svg_params<T>(root, params));
}
template<typename T>
inline std::string MTZDD<T>::save_svg() const {
    return save_svg(SvgParams());
}

// ========================================================================
//  MTZDD print_sets / to_str inline implementations
// ========================================================================

template<typename T>
inline void MTZDD<T>::print_sets(std::ostream& os) const {
    std::vector<std::pair<std::vector<bddvar>, T> > paths = enumerate();
    for (size_t i = 0; i < paths.size(); ++i) {
        if (i > 0) os << ", ";
        os << "{";
        for (size_t j = 0; j < paths[i].first.size(); ++j) {
            if (j > 0) os << ",";
            os << paths[i].first[j];
        }
        os << "} -> " << paths[i].second;
    }
}

template<typename T>
inline void MTZDD<T>::print_sets(
    std::ostream& os,
    const std::vector<std::string>& var_name_map) const
{
    std::vector<std::pair<std::vector<bddvar>, T> > paths = enumerate();
    for (size_t i = 0; i < paths.size(); ++i) {
        if (i > 0) os << ", ";
        os << "{";
        for (size_t j = 0; j < paths[i].first.size(); ++j) {
            if (j > 0) os << ",";
            bddvar v = paths[i].first[j];
            if (v < var_name_map.size() && !var_name_map[v].empty()) {
                os << var_name_map[v];
            } else {
                os << v;
            }
        }
        os << "} -> " << paths[i].second;
    }
}

template<typename T>
inline std::string MTZDD<T>::to_str() const {
    std::ostringstream oss;
    print_sets(oss);
    return oss.str();
}


} // namespace kyotodd

#endif
