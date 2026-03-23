#include "zbddv.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

// ============================================================
// Constructors, destructor, assignment
// ============================================================

ZBDDV::ZBDDV() : _zbdd(0) {}

ZBDDV::ZBDDV(const ZBDDV& fv) : _zbdd(fv._zbdd) {}

ZBDDV::ZBDDV(const ZDD& f, int location) : _zbdd(0) {
    if (location < 0) {
        throw std::invalid_argument("ZBDDV(ZDD, location): negative location");
    }
    if (location >= BDDV_MaxLen) {
        throw std::invalid_argument("ZBDDV(ZDD, location): location >= BDDV_MaxLen");
    }
    if (f.GetID() != bddnull && f.GetID() != bddempty) {
        bddvar top = f.Top();
        if (top > 0 && top <= static_cast<bddvar>(BDDV_SysVarTop)) {
            throw std::invalid_argument(
                "ZBDDV(ZDD, location): ZDD contains system variables");
        }
    }
    _zbdd = f;
    bddvar var = 1;
    int loc = location;
    while (loc > 0) {
        if (loc & 1) {
            _zbdd = _zbdd.Change(var);
        }
        loc >>= 1;
        var++;
    }
}

ZBDDV::~ZBDDV() {}

ZBDDV& ZBDDV::operator=(const ZBDDV& fv) {
    _zbdd = fv._zbdd;
    return *this;
}

// ============================================================
// Set operators
// ============================================================

ZBDDV operator&(const ZBDDV& fv, const ZBDDV& gv) {
    ZBDDV result;
    result._zbdd = fv._zbdd & gv._zbdd;
    return result;
}

ZBDDV operator+(const ZBDDV& fv, const ZBDDV& gv) {
    ZBDDV result;
    result._zbdd = fv._zbdd + gv._zbdd;
    return result;
}

ZBDDV operator-(const ZBDDV& fv, const ZBDDV& gv) {
    ZBDDV result;
    result._zbdd = fv._zbdd - gv._zbdd;
    return result;
}

ZBDDV& ZBDDV::operator&=(const ZBDDV& fv) {
    _zbdd &= fv._zbdd;
    return *this;
}

ZBDDV& ZBDDV::operator+=(const ZBDDV& fv) {
    _zbdd += fv._zbdd;
    return *this;
}

ZBDDV& ZBDDV::operator-=(const ZBDDV& fv) {
    _zbdd -= fv._zbdd;
    return *this;
}

// ============================================================
// Comparison operators
// ============================================================

int operator==(const ZBDDV& fv, const ZBDDV& gv) {
    return (fv._zbdd.GetID() == gv._zbdd.GetID()) ? 1 : 0;
}

int operator!=(const ZBDDV& fv, const ZBDDV& gv) {
    return !(fv == gv);
}

// ============================================================
// Shift operators
// ============================================================

ZBDDV ZBDDV::operator<<(int s) const {
    ZBDDV fv1 = *this;
    ZBDDV fv2;
    while (fv1._zbdd.GetID() != bddempty) {
        if (fv1._zbdd.GetID() == bddnull) {
            ZBDDV r;
            r._zbdd = ZDD(-1);
            return r;
        }
        int last = fv1.Last();
        ZDD elem = fv1.GetZBDD(last);
        ZDD shifted = elem << static_cast<bddvar>(s);
        fv2 += ZBDDV(shifted, last);
        fv1 -= fv1.Mask(last);
    }
    return fv2;
}

ZBDDV ZBDDV::operator>>(int s) const {
    ZBDDV fv1 = *this;
    ZBDDV fv2;
    while (fv1._zbdd.GetID() != bddempty) {
        if (fv1._zbdd.GetID() == bddnull) {
            ZBDDV r;
            r._zbdd = ZDD(-1);
            return r;
        }
        int last = fv1.Last();
        ZDD elem = fv1.GetZBDD(last);
        ZDD shifted = elem >> static_cast<bddvar>(s);
        fv2 += ZBDDV(shifted, last);
        fv1 -= fv1.Mask(last);
    }
    return fv2;
}

ZBDDV& ZBDDV::operator<<=(int s) {
    *this = *this << s;
    return *this;
}

ZBDDV& ZBDDV::operator>>=(int s) {
    *this = *this >> s;
    return *this;
}

// ============================================================
// Member functions
// ============================================================

ZBDDV ZBDDV::OffSet(int v) const {
    if (v > 0 && v <= BDDV_SysVarTop) {
        throw std::invalid_argument("ZBDDV::OffSet: variable is a system variable");
    }
    ZBDDV result;
    result._zbdd = _zbdd.Offset(static_cast<bddvar>(v));
    return result;
}

ZBDDV ZBDDV::OnSet(int v) const {
    if (v > 0 && v <= BDDV_SysVarTop) {
        throw std::invalid_argument("ZBDDV::OnSet: variable is a system variable");
    }
    ZBDDV result;
    result._zbdd = _zbdd.OnSet(static_cast<bddvar>(v));
    return result;
}

ZBDDV ZBDDV::OnSet0(int v) const {
    if (v > 0 && v <= BDDV_SysVarTop) {
        throw std::invalid_argument("ZBDDV::OnSet0: variable is a system variable");
    }
    ZBDDV result;
    result._zbdd = _zbdd.OnSet0(static_cast<bddvar>(v));
    return result;
}

ZBDDV ZBDDV::Change(int v) const {
    if (v > 0 && v <= BDDV_SysVarTop) {
        throw std::invalid_argument("ZBDDV::Change: variable is a system variable");
    }
    ZBDDV result;
    result._zbdd = _zbdd.Change(static_cast<bddvar>(v));
    return result;
}

ZBDDV ZBDDV::Swap(int v1, int v2) const {
    if (v1 > 0 && v1 <= BDDV_SysVarTop) {
        throw std::invalid_argument("ZBDDV::Swap: v1 is a system variable");
    }
    if (v2 > 0 && v2 <= BDDV_SysVarTop) {
        throw std::invalid_argument("ZBDDV::Swap: v2 is a system variable");
    }
    ZBDDV result;
    result._zbdd = _zbdd.Swap(static_cast<bddvar>(v1), static_cast<bddvar>(v2));
    return result;
}

int ZBDDV::Top() const {
    if (_zbdd.GetID() == bddnull) return 0;

    ZBDDV fv1 = *this;
    int top = 0;

    while (fv1._zbdd.GetID() != bddempty) {
        if (fv1._zbdd.GetID() == bddnull) return 0;
        int last = fv1.Last();
        ZDD elem = fv1.GetZBDD(last);
        bddvar t = elem.Top();
        if (t > 0) {
            if (top == 0 ||
                BDD_LevOfVar(t) >
                    BDD_LevOfVar(static_cast<bddvar>(top))) {
                top = static_cast<int>(t);
            }
        }
        fv1 -= fv1.Mask(last);
    }
    return top;
}

int ZBDDV::Last() const {
    if (_zbdd.GetID() == bddnull || _zbdd.GetID() == bddempty) return 0;

    int last = 0;
    ZDD f = _zbdd;

    // Check system variables from highest to lowest.
    // System variables 1..BDDV_SysVarTop encode the array index.
    for (int v = BDDV_SysVarTop; v >= 1; v--) {
        ZDD on = f.OnSet(static_cast<bddvar>(v));
        if (on.GetID() != bddempty) {
            last += (1 << (v - 1));
            f = f.OnSet0(static_cast<bddvar>(v));
        } else {
            f = f.Offset(static_cast<bddvar>(v));
        }
    }
    return last;
}

ZBDDV ZBDDV::Mask(int start, int length) const {
    if (start < 0 || start >= BDDV_MaxLen) {
        throw std::invalid_argument("ZBDDV::Mask: start out of range");
    }
    if (length <= 0 || start + length > BDDV_MaxLen) {
        throw std::invalid_argument("ZBDDV::Mask: length out of range");
    }
    ZBDDV tmp;
    for (int i = start; i < start + length; i++) {
        ZDD elem = GetZBDD(i);
        if (elem.GetID() != bddempty) {
            tmp += ZBDDV(elem, i);
        }
    }
    return tmp;
}

ZDD ZBDDV::GetZBDD(int index) const {
    if (index < 0 || index >= BDDV_MaxLen) {
        throw std::invalid_argument("ZBDDV::GetZBDD: index out of range");
    }

    // Compute number of bits needed for index
    int level = 0;
    for (int i = 1; i <= index; i <<= 1) {
        level++;
    }

    ZDD f = _zbdd;

    // Strip system variables above the needed level
    for (int v = BDDV_SysVarTop; v > level; v--) {
        f = f.Offset(static_cast<bddvar>(v));
    }

    // Traverse system variables from level down to 1
    for (int l = level; l >= 1; l--) {
        if (f.GetID() == bddempty) return ZDD(0);
        if (index & (1 << (l - 1))) {
            f = f.OnSet0(static_cast<bddvar>(l));
        } else {
            f = f.Offset(static_cast<bddvar>(l));
        }
    }
    return f;
}

ZDD ZBDDV::GetMetaZBDD() const {
    return _zbdd;
}

uint64_t ZBDDV::Size() const {
    if (_zbdd.GetID() == bddnull) return 0;
    int count = Last() + 1;
    std::vector<bddp> ids(static_cast<size_t>(count));
    for (int i = 0; i < count; i++) {
        ids[static_cast<size_t>(i)] = GetZBDD(i).GetID();
    }
    return bddvsize(ids);
}

// ============================================================
// Display functions
// ============================================================

void ZBDDV::Print() const {
    int count = Last() + 1;
    for (int i = 0; i < count; i++) {
        std::cout << "f" << i << ": ";
        GetZBDD(i).Print();
    }
    std::cout << "Size: " << Size() << std::endl;
}

void ZBDDV::Export(FILE* strm) const {
    int count = Last() + 1;
    std::vector<bddp> ids(static_cast<size_t>(count));
    for (int i = 0; i < count; i++) {
        ids[static_cast<size_t>(i)] = GetZBDD(i).GetID();
    }
    bddexport(strm, ids.data(), count);
}

// Internal recursive helper for PrintPla
static int PrintPla_rec(const ZBDDV& fv, int top_var, int n_out,
                        std::string& pattern) {
    if (top_var == 0) {
        // Terminal: output the product term
        bool all_empty = true;
        for (int j = 0; j < n_out; j++) {
            ZDD elem = fv.GetZBDD(j);
            if (elem.GetID() != bddempty) {
                all_empty = false;
                break;
            }
        }
        if (all_empty) return 0;

        std::cout << pattern << " ";
        for (int j = 0; j < n_out; j++) {
            ZDD elem = fv.GetZBDD(j);
            std::cout << ((elem.GetID() != bddempty) ? "1" : "0");
        }
        std::cout << std::endl;
        return 0;
    }

    bddvar var = static_cast<bddvar>(top_var);
    bddvar lev = BDD_LevOfVar(var);

    // Find the next user variable below this one
    int next_var = 0;
    if (lev > static_cast<bddvar>(BDDV_SysVarTop) + 1) {
        next_var = static_cast<int>(bddvaroflev(lev - 1));
    }

    // 1-branch (OnSet0)
    ZBDDV on = fv.OnSet0(top_var);
    if (on.GetMetaZBDD().GetID() != bddempty) {
        pattern.push_back('1');
        PrintPla_rec(on, next_var, n_out, pattern);
        pattern.pop_back();
    }

    // 0-branch (OffSet)
    ZBDDV off = fv.OffSet(top_var);
    if (off.GetMetaZBDD().GetID() != bddempty) {
        pattern.push_back('0');
        PrintPla_rec(off, next_var, n_out, pattern);
        pattern.pop_back();
    }

    return 0;
}

int ZBDDV::PrintPla() const {
    if (_zbdd.GetID() == bddnull) return 1;

    int top = Top();
    int n_levels = (top > 0) ? static_cast<int>(BDD_LevOfVar(
                                   static_cast<bddvar>(top))) -
                                   BDDV_SysVarTop
                             : 0;
    int n_out = Last() + 1;

    std::cout << ".i " << n_levels << std::endl;
    std::cout << ".o " << n_out << std::endl;

    if (n_levels == 0) {
        // Constant outputs
        std::cout << " ";
        for (int j = 0; j < n_out; j++) {
            ZDD elem = GetZBDD(j);
            std::cout << ((elem.GetID() != bddempty) ? "1" : "0");
        }
        std::cout << std::endl;
    } else {
        std::string pattern;
        PrintPla_rec(*this, top, n_out, pattern);
    }

    std::cout << ".e" << std::endl;
    return 0;
}

void ZBDDV::XPrint() const {
    int count = Last() + 1;
    std::vector<bddp> ids(static_cast<size_t>(count));
    for (int i = 0; i < count; i++) {
        ids[static_cast<size_t>(i)] = GetZBDD(i).GetID();
    }
    bddvgraph(ids.data(), count);
}

// ============================================================
// ZBDDV_Import
// ============================================================

ZBDDV ZBDDV_Import(FILE* strm) {
    char keyword[256];
    int n_in = 0, n_out = 0, n_nd = 0;

    while (fscanf(strm, "%255s", keyword) == 1) {
        if (strcmp(keyword, "_i") == 0) {
            if (fscanf(strm, "%d", &n_in) != 1) {
                ZBDDV r;
                r = ZBDDV(ZDD(-1));
                return r;
            }
        } else if (strcmp(keyword, "_o") == 0) {
            if (fscanf(strm, "%d", &n_out) != 1) {
                ZBDDV r;
                r = ZBDDV(ZDD(-1));
                return r;
            }
        } else if (strcmp(keyword, "_n") == 0) {
            if (fscanf(strm, "%d", &n_nd) != 1) {
                ZBDDV r;
                r = ZBDDV(ZDD(-1));
                return r;
            }
            break;
        }
    }

    if (n_out <= 0 || n_out > BDDV_MaxLenImport) {
        ZBDDV r;
        r = ZBDDV(ZDD(-1));
        return r;
    }

    // Ensure enough user variables exist
    while (BDDV_UserTopLev() < n_in) {
        BDDV_NewVar();
    }

    // Hash table for node definitions: node_number -> ZDD
    std::unordered_map<int, ZDD> node_map;

    // Read node definitions
    for (int i = 0; i < n_nd; i++) {
        int node_num, level;
        char lo_str[256], hi_str[256];

        if (fscanf(strm, "%d %d %255s %255s", &node_num, &level, lo_str,
                   hi_str) != 4) {
            return ZBDDV(ZDD(-1));
        }

        // Parse lo child (0-child)
        ZDD f0(0);
        if (strcmp(lo_str, "F") == 0) {
            f0 = ZDD(0);
        } else if (strcmp(lo_str, "T") == 0) {
            f0 = ZDD(1);
        } else {
            char* endptr;
            long lo_num_l = strtol(lo_str, &endptr, 10);
            if (*endptr != '\0') return ZBDDV(ZDD(-1));
            int lo_num = static_cast<int>(lo_num_l);
            auto it = node_map.find(lo_num);
            if (it == node_map.end()) return ZBDDV(ZDD(-1));
            f0 = it->second;
        }

        // Parse hi child (1-child); odd number means union with {empty set}
        ZDD f1(0);
        if (strcmp(hi_str, "F") == 0) {
            f1 = ZDD(0);
        } else if (strcmp(hi_str, "T") == 0) {
            f1 = ZDD(1);
        } else {
            char* endptr;
            long hi_num_l = strtol(hi_str, &endptr, 10);
            if (*endptr != '\0') return ZBDDV(ZDD(-1));
            int hi_num = static_cast<int>(hi_num_l);
            bool hi_union_single = false;
            if (hi_num % 2 != 0) {
                hi_union_single = true;
                hi_num -= 1;
            }
            auto it = node_map.find(hi_num);
            if (it == node_map.end()) return ZBDDV(ZDD(-1));
            f1 = it->second;
            if (hi_union_single) f1 = f1 + ZDD(1);
        }

        // Build ZDD node: f1.Change(var) + f0
        // Level in file is user level; offset past system variables
        bddvar actual_level = static_cast<bddvar>(level + BDDV_SysVarTop);
        bddvar var = bddvaroflev(actual_level);
        ZDD node_zdd = f1.Change(var) + f0;
        node_map[node_num] = node_zdd;
    }

    // Read output definitions
    ZBDDV result;
    for (int i = 0; i < n_out; i++) {
        char out_str[256];
        if (fscanf(strm, "%255s", out_str) != 1) return ZBDDV(ZDD(-1));

        ZDD out_zdd(0);
        if (strcmp(out_str, "F") == 0) {
            out_zdd = ZDD(0);
        } else if (strcmp(out_str, "T") == 0) {
            out_zdd = ZDD(1);
        } else {
            char* endptr;
            long out_num_l = strtol(out_str, &endptr, 10);
            if (*endptr != '\0') return ZBDDV(ZDD(-1));
            int out_num = static_cast<int>(out_num_l);
            bool out_union_single = false;
            if (out_num % 2 != 0) {
                out_union_single = true;
                out_num -= 1;
            }
            auto it = node_map.find(out_num);
            if (it == node_map.end()) return ZBDDV(ZDD(-1));
            out_zdd = it->second;
            if (out_union_single) out_zdd = out_zdd + ZDD(1);
        }

        result += ZBDDV(out_zdd, i);
    }

    return result;
}
