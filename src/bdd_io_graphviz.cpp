#include "bdd_io_common.h"
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace kyotodd {

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
