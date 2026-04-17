#include "bddv.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace kyotodd {


int BDDV_Active = 0;

// ============================================================
// Free functions: initialization and variable management
// ============================================================

int BDDV_Init(uint64_t init, uint64_t limit) {
    int err = bddinit(init, limit);
    if (err) return 1;
    for (int i = 0; i < BDDV_SysVarTop; i++) {
        bddnewvar();
    }
    BDDV_Active = 1;
    return 0;
}

int BDDV_UserTopLev() {
    return static_cast<int>(BDD_TopLev()) - BDDV_SysVarTop;
}

int BDDV_NewVar() {
    return static_cast<int>(bddnewvar());
}

int BDDV_NewVarOfLev(int lev) {
    return static_cast<int>(BDD_NewVarOfLev(static_cast<bddvar>(lev)));
}

// ============================================================
// Private helper
// ============================================================

int BDDV::GetLev(int len) const {
    if (len <= 1) return 0;
    int lev = 0;
    int v = len - 1;
    while (v > 0) {
        v >>= 1;
        lev++;
    }
    return lev;
}

// Helper: create a length-1 null BDDV
static BDDV make_null_bddv() {
    return BDDV(BDD(-1), 1);
}

// ============================================================
// Constructors, destructor, assignment
// ============================================================

BDDV::BDDV() : _bdd(0), _len(0), _lev(0) {}

BDDV::BDDV(const BDDV& fv) : _bdd(fv._bdd), _len(fv._len), _lev(fv._lev) {}

BDDV::BDDV(const BDD& f) : _bdd(f), _len(1), _lev(0) {
    if (f.GetID() == bddnull) return;
    bddvar top = f.Top();
    if (top > 0 && top <= static_cast<bddvar>(BDDV_SysVarTop)) {
        throw std::invalid_argument("BDDV(BDD): BDD contains system variables");
    }
}

BDDV::BDDV(const BDD& f, int len) : _bdd(0), _len(0), _lev(0) {
    if (len < 0) {
        throw std::invalid_argument("BDDV(BDD, len): negative length");
    }
    if (len > BDDV_MaxLen) {
        throw std::invalid_argument("BDDV(BDD, len): length exceeds BDDV_MaxLen");
    }
    if (f.GetID() != bddnull) {
        bddvar top = f.Top();
        if (top > 0 && top <= static_cast<bddvar>(BDDV_SysVarTop)) {
            throw std::invalid_argument("BDDV(BDD, len): BDD contains system variables");
        }
    }
    if (f.GetID() == bddnull) {
        _bdd = f;
        _len = 1;
        _lev = 0;
    } else if (len == 0) {
        _bdd = BDD(0);
        _len = 0;
        _lev = 0;
    } else {
        _bdd = f;
        _len = len;
        _lev = GetLev(len);
    }
}

BDDV::~BDDV() {}

BDDV& BDDV::operator=(const BDDV& fv) {
    _bdd = fv._bdd;
    _len = fv._len;
    _lev = fv._lev;
    return *this;
}

// ============================================================
// Basic accessors
// ============================================================

BDD BDDV::GetBDD(int index) const {
    if (index < 0 || index >= _len) {
        throw std::invalid_argument("BDDV::GetBDD: index out of range");
    }
    if (_len == 1) return _bdd;

    BDD f = _bdd;
    for (int i = _lev - 1; i >= 0; i--) {
        if (index & (1 << i))
            f = f.At1(static_cast<bddvar>(i + 1));
        else
            f = f.At0(static_cast<bddvar>(i + 1));
    }
    return f;
}

BDD BDDV::GetMetaBDD() const {
    return _bdd;
}

int BDDV::Uniform() const {
    if (_bdd.is_terminal()) return 1;
    if (_lev == 0) return 1;
    // Check if the meta-BDD depends on any system variable (1.._lev).
    // In a reduced BDD, variable v is present iff At0(v) != At1(v).
    // Check from the highest system variable used (_lev) down to 1.
    BDD cur = _bdd;
    for (int i = _lev; i >= 1; i--) {
        BDD a0 = cur.At0(static_cast<bddvar>(i));
        BDD a1 = cur.At1(static_cast<bddvar>(i));
        if (a0.GetID() != a1.GetID()) return 0;
        cur = a0;
    }
    return 1;
}

int BDDV::Len() const {
    return _len;
}

BDDV BDDV::Former() const {
    if (_len <= 1) return BDDV();
    BDD f = _bdd.At0(static_cast<bddvar>(_lev));
    if (f.GetID() == bddnull) return make_null_bddv();
    BDDV result;
    result._bdd = f;
    result._len = (1 << (_lev - 1));
    result._lev = _lev - 1;
    return result;
}

BDDV BDDV::Latter() const {
    if (_len == 0) return BDDV();
    if (_len == 1) return *this;
    BDD f = _bdd.At1(static_cast<bddvar>(_lev));
    if (f.GetID() == bddnull) return make_null_bddv();
    int latter_len = _len - (1 << (_lev - 1));
    BDDV result;
    result._bdd = f;
    result._len = latter_len;
    result._lev = result.GetLev(latter_len);
    return result;
}

// ============================================================
// Logic operators
// ============================================================

BDDV operator&(const BDDV& fv, const BDDV& gv) {
    if (fv._len != gv._len) {
        throw std::invalid_argument("BDDV operator&: length mismatch");
    }
    BDD r = fv._bdd & gv._bdd;
    if (r.GetID() == bddnull) return make_null_bddv();
    BDDV result;
    result._bdd = r;
    result._len = fv._len;
    result._lev = fv._lev;
    return result;
}

BDDV operator|(const BDDV& fv, const BDDV& gv) {
    if (fv._len != gv._len) {
        throw std::invalid_argument("BDDV operator|: length mismatch");
    }
    BDD r = fv._bdd | gv._bdd;
    if (r.GetID() == bddnull) return make_null_bddv();
    BDDV result;
    result._bdd = r;
    result._len = fv._len;
    result._lev = fv._lev;
    return result;
}

BDDV operator^(const BDDV& fv, const BDDV& gv) {
    if (fv._len != gv._len) {
        throw std::invalid_argument("BDDV operator^: length mismatch");
    }
    BDD r = fv._bdd ^ gv._bdd;
    if (r.GetID() == bddnull) return make_null_bddv();
    BDDV result;
    result._bdd = r;
    result._len = fv._len;
    result._lev = fv._lev;
    return result;
}

BDDV BDDV::operator~() const {
    BDD r = ~_bdd;
    if (r.GetID() == bddnull) return make_null_bddv();
    BDDV result;
    result._bdd = r;
    result._len = _len;
    result._lev = _lev;
    return result;
}

BDDV& BDDV::operator&=(const BDDV& fv) {
    *this = *this & fv;
    return *this;
}

BDDV& BDDV::operator|=(const BDDV& fv) {
    *this = *this | fv;
    return *this;
}

BDDV& BDDV::operator^=(const BDDV& fv) {
    *this = *this ^ fv;
    return *this;
}

// ============================================================
// Comparison operators
// ============================================================

int operator==(const BDDV& fv, const BDDV& gv) {
    return (fv._len == gv._len && fv._bdd.GetID() == gv._bdd.GetID()) ? 1 : 0;
}

int operator!=(const BDDV& fv, const BDDV& gv) {
    return !(fv == gv);
}

// ============================================================
// Concatenation operator ||
// ============================================================

BDDV operator||(const BDDV& fv, const BDDV& gv) {
    if (fv._len == 0) return gv;
    if (gv._len == 0) return fv;

    int total = fv._len + gv._len;
    if (total > BDDV_MaxLen) {
        throw std::invalid_argument("BDDV operator||: total length exceeds BDDV_MaxLen");
    }

    // fv is not power-of-two aligned
    if (fv._len != (1 << fv._lev)) {
        return fv.Former() || (fv.Latter() || gv);
    }

    // fv is smaller than gv
    if (fv._len < gv._len) {
        return (fv || gv.Former()) || gv.Latter();
    }

    // Base case: fv is power-of-two and fv._len >= gv._len
    BDD x = BDDvar(static_cast<bddvar>(fv._lev + 1));
    BDD r = (~x & fv._bdd) | (x & gv._bdd);
    if (r.GetID() == bddnull) return make_null_bddv();

    BDDV result;
    result._bdd = r;
    result._len = total;
    result._lev = fv._lev + 1;
    return result;
}

// ============================================================
// Shift operators
// ============================================================

BDDV BDDV::operator<<(int s) const {
    if (Uniform()) {
        BDD r = _bdd << static_cast<bddvar>(s);
        if (r.GetID() == bddnull) return make_null_bddv();
        BDDV result;
        result._bdd = r;
        result._len = _len;
        result._lev = _lev;
        return result;
    }
    return Former() << s || Latter() << s;
}

BDDV BDDV::operator>>(int s) const {
    if (Uniform()) {
        BDD r = _bdd >> static_cast<bddvar>(s);
        if (r.GetID() == bddnull) return make_null_bddv();
        BDDV result;
        result._bdd = r;
        result._len = _len;
        result._lev = _lev;
        return result;
    }
    return Former() >> s || Latter() >> s;
}

BDDV& BDDV::operator<<=(int s) {
    *this = *this << s;
    return *this;
}

BDDV& BDDV::operator>>=(int s) {
    *this = *this >> s;
    return *this;
}

// ============================================================
// Member functions: At0, At1, Cofact, Swap, Spread, Top, Size
// ============================================================

BDDV BDDV::At0(int v) const {
    if (v > 0 && v <= BDDV_SysVarTop) {
        throw std::invalid_argument("BDDV::At0: variable is a system variable");
    }
    BDD r = _bdd.At0(static_cast<bddvar>(v));
    if (r.GetID() == bddnull) return make_null_bddv();
    BDDV result;
    result._bdd = r;
    result._len = _len;
    result._lev = _lev;
    return result;
}

BDDV BDDV::At1(int v) const {
    if (v > 0 && v <= BDDV_SysVarTop) {
        throw std::invalid_argument("BDDV::At1: variable is a system variable");
    }
    BDD r = _bdd.At1(static_cast<bddvar>(v));
    if (r.GetID() == bddnull) return make_null_bddv();
    BDDV result;
    result._bdd = r;
    result._len = _len;
    result._lev = _lev;
    return result;
}

BDDV BDDV::Cofact(const BDDV& fv) const {
    if (_len != fv._len) {
        throw std::invalid_argument("BDDV::Cofact: length mismatch");
    }
    if (_lev > 0) {
        BDDV f_former = Former().Cofact(fv.Former());
        BDDV f_latter = Latter().Cofact(fv.Latter());
        return f_former || f_latter;
    }
    BDD r = _bdd.Cofactor(fv._bdd);
    if (r.GetID() == bddnull) return make_null_bddv();
    BDDV result;
    result._bdd = r;
    result._len = _len;
    result._lev = _lev;
    return result;
}

BDDV BDDV::Swap(int v1, int v2) const {
    if (v1 > 0 && v1 <= BDDV_SysVarTop) {
        throw std::invalid_argument("BDDV::Swap: v1 is a system variable");
    }
    if (v2 > 0 && v2 <= BDDV_SysVarTop) {
        throw std::invalid_argument("BDDV::Swap: v2 is a system variable");
    }
    BDD r = _bdd.Swap(static_cast<bddvar>(v1), static_cast<bddvar>(v2));
    if (r.GetID() == bddnull) return make_null_bddv();
    BDDV result;
    result._bdd = r;
    result._len = _len;
    result._lev = _lev;
    return result;
}

BDDV BDDV::Spread(int k) const {
    if (Uniform()) {
        BDD r = _bdd.Spread(k);
        if (r.GetID() == bddnull) return make_null_bddv();
        BDDV result;
        result._bdd = r;
        result._len = _len;
        result._lev = _lev;
        return result;
    }
    return Former().Spread(k) || Latter().Spread(k);
}

int BDDV::Top() const {
    if (Uniform()) {
        return static_cast<int>(_bdd.Top());
    }
    int t1 = Former().Top();
    int t2 = Latter().Top();
    if (t1 == 0) return t2;
    if (t2 == 0) return t1;
    bddvar lev1 = BDD_LevOfVar(static_cast<bddvar>(t1));
    bddvar lev2 = BDD_LevOfVar(static_cast<bddvar>(t2));
    return (lev1 >= lev2) ? t1 : t2;
}

uint64_t BDDV::Size() const {
    if (_len == 0) return 0;
    std::vector<bddp> ids(static_cast<size_t>(_len));
    for (int i = 0; i < _len; i++) {
        ids[static_cast<size_t>(i)] = GetBDD(i).GetID();
    }
    return bddvsize(ids);
}

// ============================================================
// Display functions
// ============================================================

void BDDV::Export(FILE* strm) const {
    if (_len == 0) return;
    std::vector<bddp> ids(static_cast<size_t>(_len));
    for (int i = 0; i < _len; i++) {
        ids[static_cast<size_t>(i)] = GetBDD(i).GetID();
    }
    bddexport(strm, ids.data(), _len);
}

void BDDV::Print() const {
    for (int i = 0; i < _len; i++) {
        std::cout << "f" << i << ": ";
        GetBDD(i).Print();
        std::cout << std::endl;
    }
    std::cout << "Size: " << Size() << std::endl;
}

void BDDV::XPrint0() const {
    if (_len == 0) return;
    std::vector<bddp> ids(static_cast<size_t>(_len));
    for (int i = 0; i < _len; i++) {
        ids[static_cast<size_t>(i)] = GetBDD(i).GetID();
    }
    bddvgraph0(ids.data(), _len);
}

void BDDV::XPrint() const {
    if (_len == 0) return;
    std::vector<bddp> ids(static_cast<size_t>(_len));
    for (int i = 0; i < _len; i++) {
        ids[static_cast<size_t>(i)] = GetBDD(i).GetID();
    }
    bddvgraph(ids.data(), _len);
}

// ============================================================
// Part
// ============================================================

BDDV BDDV::Part(int start, int len) const {
    if (_bdd.GetID() == bddnull) return *this;
    if (len == 0) return BDDV();
    if (start < 0 || start + len > _len) {
        throw std::invalid_argument("BDDV::Part: range out of bounds");
    }
    if (start == 0 && len == _len) return *this;

    int half = (1 << (_lev - 1));
    if (start + len <= half) {
        return Former().Part(start, len);
    }
    if (start >= half) {
        return Latter().Part(start - half, len);
    }
    return Former().Part(start, half - start) ||
           Latter().Part(0, start + len - half);
}

// ============================================================
// Free functions: BDDV_Imply, BDDV_Mask1, BDDV_Mask2
// ============================================================

int BDDV_Imply(const BDDV& fv, const BDDV& gv) {
    if (fv.Len() != gv.Len()) return 0;
    return bddimply(fv.GetMetaBDD().GetID(), gv.GetMetaBDD().GetID());
}

BDDV BDDV_Mask1(int index, int len) {
    if (len < 0) {
        throw std::invalid_argument("BDDV_Mask1: negative length");
    }
    if (index < 0 || index >= len) {
        throw std::invalid_argument("BDDV_Mask1: index out of range");
    }
    return BDDV(BDD(0), index) || BDDV(BDD(1), 1) ||
           BDDV(BDD(0), len - index - 1);
}

BDDV BDDV_Mask2(int index, int len) {
    if (len < 0) {
        throw std::invalid_argument("BDDV_Mask2: negative length");
    }
    if (index < 0 || index > len) {
        throw std::invalid_argument("BDDV_Mask2: index out of range");
    }
    return BDDV(BDD(0), index) || BDDV(BDD(1), len - index);
}

// ============================================================
// Import functions
// ============================================================

BDDV BDDV_Import(FILE* strm) {
    char keyword[256];
    int n_in = 0, n_out = 0, n_nd = 0;

    // Read header keywords: _i, _o, _n
    while (fscanf(strm, "%255s", keyword) == 1) {
        if (strcmp(keyword, "_i") == 0) {
            if (fscanf(strm, "%d", &n_in) != 1) return make_null_bddv();
        } else if (strcmp(keyword, "_o") == 0) {
            if (fscanf(strm, "%d", &n_out) != 1) return make_null_bddv();
        } else if (strcmp(keyword, "_n") == 0) {
            if (fscanf(strm, "%d", &n_nd) != 1) return make_null_bddv();
            break;
        }
    }

    if (n_in < 0 || n_out <= 0 || n_out > BDDV_MaxLenImport || n_nd < 0)
        return make_null_bddv();

    // Ensure enough input variables exist
    while (BDDV_UserTopLev() < n_in) {
        BDDV_NewVar();
    }

    // Hash table for node definitions: node_number -> BDD
    std::unordered_map<int, BDD> node_map;

    // Read node definitions
    for (int i = 0; i < n_nd; i++) {
        int node_num, level;
        char lo_str[256], hi_str[256];

        if (fscanf(strm, "%d %d %255s %255s", &node_num, &level, lo_str, hi_str) != 4) {
            return make_null_bddv();
        }

        // Parse lo child
        BDD f0(0);
        if (strcmp(lo_str, "F") == 0) {
            f0 = BDD(0);
        } else if (strcmp(lo_str, "T") == 0) {
            f0 = BDD(1);
        } else {
            char* endptr;
            long lo_num_l = strtol(lo_str, &endptr, 10);
            if (*endptr != '\0') return make_null_bddv();
            int lo_num = static_cast<int>(lo_num_l);
            auto it = node_map.find(lo_num);
            if (it == node_map.end()) return make_null_bddv();
            f0 = it->second;
        }

        // Parse hi child (odd number means negated)
        BDD f1(0);
        bool hi_neg = false;
        if (strcmp(hi_str, "F") == 0) {
            f1 = BDD(0);
        } else if (strcmp(hi_str, "T") == 0) {
            f1 = BDD(1);
        } else {
            char* endptr;
            long hi_num_l = strtol(hi_str, &endptr, 10);
            if (*endptr != '\0') return make_null_bddv();
            int hi_num = static_cast<int>(hi_num_l);
            if (hi_num % 2 != 0) {
                hi_neg = true;
                hi_num -= 1;
            }
            auto it = node_map.find(hi_num);
            if (it == node_map.end()) return make_null_bddv();
            f1 = it->second;
            if (hi_neg) f1 = ~f1;
        }

        // Validate level range (1-based user level)
        if (level < 1 || level > n_in) return make_null_bddv();

        // Reject duplicate node IDs
        if (node_map.count(node_num)) return make_null_bddv();

        // Build BDD node: (x & f1) | (~x & f0)
        // level in the import file is a user level (1-based);
        // map to actual variable number by offsetting past system variables
        bddvar actual_level = static_cast<bddvar>(level + BDDV_SysVarTop);
        bddvar var = bddvaroflev(actual_level);
        BDD x = BDDvar(var);
        BDD node_bdd = (x & f1) | (~x & f0);
        node_map[node_num] = node_bdd;
    }

    // Read output definitions
    BDDV result;
    for (int i = 0; i < n_out; i++) {
        char out_str[256];
        if (fscanf(strm, "%255s", out_str) != 1) return make_null_bddv();

        BDD out_bdd(0);
        if (strcmp(out_str, "F") == 0) {
            out_bdd = BDD(0);
        } else if (strcmp(out_str, "T") == 0) {
            out_bdd = BDD(1);
        } else {
            char* endptr;
            long out_num_l = strtol(out_str, &endptr, 10);
            if (*endptr != '\0') return make_null_bddv();
            int out_num = static_cast<int>(out_num_l);
            bool out_neg = false;
            if (out_num % 2 != 0) {
                out_neg = true;
                out_num -= 1;
            }
            auto it = node_map.find(out_num);
            if (it == node_map.end()) return make_null_bddv();
            out_bdd = it->second;
            if (out_neg) out_bdd = ~out_bdd;
        }

        result = result || BDDV(out_bdd);
    }

    return result;
}

BDDV BDDV_ImportPla(FILE* strm, int sopf) {
    char line[4096];
    int n_in = 0, n_out = 0;
    // type: 0=f, 1=fd, 2=fr, 3=fdr
    int type = 0;
    bool header_done = false;
    // Collect non-header lines encountered before header is complete
    std::vector<std::string> pending_lines;

    // Parse header and collect product term lines
    while (fgets(line, sizeof(line), strm) != NULL) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;

        if (line[0] == '.') {
            char keyword[64];
            sscanf(line, "%63s", keyword);

            if (strcmp(keyword, ".i") == 0) {
                sscanf(line + 2, "%d", &n_in);
            } else if (strcmp(keyword, ".o") == 0) {
                sscanf(line + 2, "%d", &n_out);
            } else if (strcmp(keyword, ".type") == 0) {
                char type_str[64];
                sscanf(line + 5, "%63s", type_str);
                if (strcmp(type_str, "f") == 0) type = 0;
                else if (strcmp(type_str, "fd") == 0) type = 1;
                else if (strcmp(type_str, "fr") == 0) type = 2;
                else if (strcmp(type_str, "fdr") == 0) type = 3;
            } else if (strcmp(keyword, ".e") == 0 ||
                       strcmp(keyword, ".end") == 0) {
                break;
            }
            continue;
        }

        if (n_in <= 0 || n_out <= 0) continue;

        if (!header_done) {
            int need = sopf ? n_in * 2 : n_in;
            while (BDDV_UserTopLev() < need) {
                BDDV_NewVar();
            }
            header_done = true;
        }

        // Save this product term line for processing below
        pending_lines.push_back(line);
    }

    if (n_in <= 0 || n_out <= 0) return make_null_bddv();

    if (!header_done) {
        int need = sopf ? n_in * 2 : n_in;
        while (BDDV_UserTopLev() < need) {
            BDDV_NewVar();
        }
    }

    std::vector<BDD> onset(static_cast<size_t>(n_out), BDD(0));
    std::vector<BDD> offset(static_cast<size_t>(n_out), BDD(0));
    std::vector<BDD> dcset(static_cast<size_t>(n_out), BDD(0));

    auto parse_product_term = [&](const char* ln) -> bool {
        const char* p = ln;

        BDD cube(1);
        for (int i = 0; i < n_in; i++) {
            while (*p == ' ' || *p == '\t') p++;
            if (*p == '\0' || *p == '\n' || *p == '\r') return false;
            char c = *p++;
            // Map input index i to user variable via level.
            // sopf=1: pairs (neg/pos) at user levels 1,2,3,4,...
            //   positive literal is at even user level 2*(i+1).
            // sopf=0: single variable at user level i+1.
            bddvar lev = sopf
                ? static_cast<bddvar>(BDDV_SysVarTop + 2 * (i + 1))
                : static_cast<bddvar>(BDDV_SysVarTop + 1 + i);
            bddvar var = bddvaroflev(lev);
            if (c == '0') {
                cube = cube & ~BDDvar(var);
            } else if (c == '1') {
                cube = cube & BDDvar(var);
            } else if (c == '-') {
                // don't care
            } else {
                return false;
            }
        }

        while (*p == ' ' || *p == '\t' || *p == '|') p++;

        for (int j = 0; j < n_out; j++) {
            if (*p == '\0' || *p == '\n' || *p == '\r') return false;
            char c = *p++;
            size_t jj = static_cast<size_t>(j);
            if (c == '1') {
                onset[jj] = onset[jj] | cube;
            } else if (c == '0') {
                offset[jj] = offset[jj] | cube;
            } else if (c == '-' || c == '~') {
                dcset[jj] = dcset[jj] | cube;
            }
        }
        return true;
    };

    // Process all collected product term lines
    for (size_t k = 0; k < pending_lines.size(); k++) {
        if (!parse_product_term(pending_lines[k].c_str())) {
            throw std::invalid_argument(
                "BDDV_ImportPla: malformed product term: " +
                pending_lines[k]);
        }
    }

    // Determine final onset and dcset based on type
    std::vector<BDD> final_onset(static_cast<size_t>(n_out));
    std::vector<BDD> final_dcset(static_cast<size_t>(n_out));

    for (int j = 0; j < n_out; j++) {
        size_t jj = static_cast<size_t>(j);
        switch (type) {
        case 0: // f: onset given, dcset = 0
            final_onset[jj] = onset[jj];
            final_dcset[jj] = BDD(0);
            break;
        case 1: // fd: onset = onset column, dcset = dcset column
            final_onset[jj] = onset[jj];
            final_dcset[jj] = dcset[jj];
            break;
        case 2: // fr: onset given, offset given; dcset = ~(onset | offset)
            final_onset[jj] = onset[jj];
            final_dcset[jj] = ~(onset[jj] | offset[jj]);
            break;
        case 3: // fdr: all given
            final_onset[jj] = onset[jj];
            final_dcset[jj] = dcset[jj];
            break;
        }
    }

    BDDV onset_bddv;
    for (int j = 0; j < n_out; j++) {
        onset_bddv = onset_bddv || BDDV(final_onset[static_cast<size_t>(j)]);
    }

    BDDV dcset_bddv;
    for (int j = 0; j < n_out; j++) {
        dcset_bddv = dcset_bddv || BDDV(final_dcset[static_cast<size_t>(j)]);
    }

    return onset_bddv || dcset_bddv;
}

} // namespace kyotodd
