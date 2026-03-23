#include "btoi.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <stdexcept>
#include <vector>

/* ================================================================ */
/*  Private helpers                                                  */
/* ================================================================ */

BtoI BtoI::Shift(int power) const
{
    if (power == 0) return *this;

    int len = Len();
    if (power > 0) {
        /* Left shift: insert 'power' zero bits at bottom */
        if (_bddv.GetMetaBDD().GetID() == bddempty) return *this;  // 0 << n = 0
        BDDV zeros(BDD(0), power);
        return BtoI(zeros || _bddv);
    } else {
        /* Arithmetic right shift: drop bottom bits, keep sign */
        int p = -power;
        if (p >= len) p = len - 1;
        return BtoI(_bddv.Part(p, len - p));
    }
}

BtoI BtoI::Sup() const
{
    int len = Len();
    while (len > 1) {
        BDD top = _bddv.GetBDD(len - 1);
        BDD below = _bddv.GetBDD(len - 2);
        if (top == below) {
            len--;
        } else {
            break;
        }
    }
    if (len == Len()) return *this;
    return BtoI(_bddv.Part(0, len));
}

/* ================================================================ */
/*  Constructors / destructor / assignment                           */
/* ================================================================ */

BtoI::BtoI() : _bddv(BDD(0)) {}

BtoI::BtoI(const BtoI& fv) : _bddv(fv._bddv) {}

BtoI::BtoI(const BDDV& fv) : _bddv(fv) {}

BtoI::BtoI(const BDD& f)
{
    if (f.GetID() == bddempty || f.GetID() == bddnull) {
        /* BDD(0) → value 0, BDD(-1) → overflow */
        _bddv = BDDV(f);
    } else {
        /* BDD(1) or variable-dependent: 0-or-1 value, sign bit = 0 */
        _bddv = BDDV(f) || BDDV(BDD(0));
    }
}

BtoI::BtoI(int n)
{
    if (n == 0) {
        _bddv = BDDV(BDD(0));
        return;
    }

    bool neg = (n < 0);
    unsigned int k;
    if (neg) {
        if (n == INT_MIN) {
            throw std::overflow_error("BtoI(int): INT_MIN overflow");
        }
        k = static_cast<unsigned int>(-n);
    } else {
        k = static_cast<unsigned int>(n);
    }

    /* Build positive value bit by bit */
    _bddv = BDDV(BDD(static_cast<int>(k & 1)));
    k >>= 1;
    while (k > 0) {
        _bddv = _bddv || BDDV(BDD(static_cast<int>(k & 1)));
        k >>= 1;
    }
    /* Append sign bit = 0 (positive) */
    _bddv = _bddv || BDDV(BDD(0));

    if (neg) {
        *this = -(*this);
    }
}

BtoI::~BtoI() {}

BtoI& BtoI::operator=(const BtoI& fv)
{
    _bddv = fv._bddv;
    return *this;
}

/* ================================================================ */
/*  Compound assignment operators                                    */
/* ================================================================ */

BtoI& BtoI::operator+=(const BtoI& fv) { *this = *this + fv; return *this; }
BtoI& BtoI::operator-=(const BtoI& fv) { *this = *this - fv; return *this; }
BtoI& BtoI::operator*=(const BtoI& fv) { *this = *this * fv; return *this; }
BtoI& BtoI::operator/=(const BtoI& fv) { *this = *this / fv; return *this; }
BtoI& BtoI::operator%=(const BtoI& fv) { *this = *this % fv; return *this; }
BtoI& BtoI::operator&=(const BtoI& fv) { *this = *this & fv; return *this; }
BtoI& BtoI::operator|=(const BtoI& fv) { *this = *this | fv; return *this; }
BtoI& BtoI::operator^=(const BtoI& fv) { *this = *this ^ fv; return *this; }
BtoI& BtoI::operator<<=(const BtoI& fv) { *this = *this << fv; return *this; }
BtoI& BtoI::operator>>=(const BtoI& fv) { *this = *this >> fv; return *this; }

/* ================================================================ */
/*  Unary operators                                                  */
/* ================================================================ */

BtoI BtoI::operator-() const
{
    return BtoI(0) - *this;
}

BtoI BtoI::operator~() const
{
    return BtoI(~_bddv);
}

BtoI BtoI::operator!() const
{
    return BtoI_EQ(*this, BtoI(0));
}

/* ================================================================ */
/*  Equality / inequality                                            */
/* ================================================================ */

int operator==(const BtoI& a, const BtoI& b)
{
    BtoI sa = a.Sup();
    BtoI sb = b.Sup();
    return (sa._bddv == sb._bddv) ? 1 : 0;
}

int operator!=(const BtoI& a, const BtoI& b)
{
    return !(a == b);
}

/* ================================================================ */
/*  Addition                                                         */
/* ================================================================ */

BtoI operator+(const BtoI& a, const BtoI& b)
{
    /* Terminal cases */
    if (a._bddv.GetMetaBDD().GetID() == bddempty) return b;  // 0 + b = b (a is BDDV(BDD(0)) len 1)
    if (b._bddv.GetMetaBDD().GetID() == bddempty) return a;

    /* Check for actual zero: all bits are BDD(0) */
    bool a_is_zero = (a.Len() == 1 && a._bddv.GetBDD(0).GetID() == bddempty);
    bool b_is_zero = (b.Len() == 1 && b._bddv.GetBDD(0).GetID() == bddempty);
    if (a_is_zero) return b;
    if (b_is_zero) return a;

    /* Check overflow */
    if (a._bddv.GetBDD(0).GetID() == bddnull) return a;
    if (b._bddv.GetBDD(0).GetID() == bddnull) return b;

    int len = (a.Len() > b.Len()) ? a.Len() : b.Len();

    BDD carry(0);
    BDDV result(BDD(0));  // placeholder, will be replaced
    bool first = true;

    BDD prev_carry(0);
    BDD a0, b0;

    for (int i = 0; i < len; i++) {
        a0 = a.GetBDD(i);
        b0 = b.GetBDD(i);
        BDD sum = a0 ^ b0 ^ carry;
        if (first) {
            result = BDDV(sum);
            first = false;
        } else {
            result = result || BDDV(sum);
        }
        prev_carry = carry;
        carry = (a0 & b0) | (b0 & carry) | (carry & a0);
    }

    /* Check if we need an extra bit for sign extension */
    if (carry != prev_carry) {
        BDD extra = a0 ^ b0 ^ carry;
        result = result || BDDV(extra);
    }

    return BtoI(result).Sup();
}

/* ================================================================ */
/*  Subtraction                                                      */
/* ================================================================ */

BtoI operator-(const BtoI& a, const BtoI& b)
{
    /* Terminal cases */
    bool b_is_zero = (b.Len() == 1 && b._bddv.GetBDD(0).GetID() == bddempty);
    if (b_is_zero) return a;
    if (b._bddv.GetBDD(0).GetID() == bddnull) return b;
    if (a == b) return BtoI(0);
    bool a_is_zero = (a.Len() == 1 && a._bddv.GetBDD(0).GetID() == bddempty);
    /* a == 0 is handled by generic subtraction below */
    if (a._bddv.GetBDD(0).GetID() == bddnull) return a;
    (void)a_is_zero;

    int len = (a.Len() > b.Len()) ? a.Len() : b.Len();

    BDD borrow(0);
    BDDV result(BDD(0));
    bool first = true;

    BDD prev_borrow(0);
    BDD a0, b0;

    for (int i = 0; i < len; i++) {
        a0 = a.GetBDD(i);
        b0 = b.GetBDD(i);
        BDD diff = a0 ^ b0 ^ borrow;
        if (first) {
            result = BDDV(diff);
            first = false;
        } else {
            result = result || BDDV(diff);
        }
        prev_borrow = borrow;
        borrow = (~a0 & b0) | (b0 & borrow) | (borrow & ~a0);
    }

    /* Check if we need an extra bit */
    if (borrow != prev_borrow) {
        BDD extra = a0 ^ b0 ^ borrow;
        result = result || BDDV(extra);
    }

    return BtoI(result).Sup();
}

/* ================================================================ */
/*  Multiplication                                                   */
/* ================================================================ */

BtoI operator*(const BtoI& a, const BtoI& b)
{
    /* Terminal cases */
    if (a == BtoI(1)) return b;
    if (b == BtoI(1)) return a;
    if (a._bddv.GetBDD(0).GetID() == bddnull) return a;
    if (b._bddv.GetBDD(0).GetID() == bddnull) return b;
    if (a == BtoI(0)) return BtoI(0);
    if (b == BtoI(0)) return BtoI(0);

    /* Make b's sign non-negative: if b < 0, negate both */
    BDD bsign = b.GetSignBDD();
    BtoI a0 = BtoI_ITE(bsign, -a, a);
    BtoI b0 = BtoI_ITE(bsign, -b, b);

    /* Initial partial product: if b0.bit0 is 1, then a0, else 0 */
    BDD b0_bit0 = b0.GetBDD(0);
    BtoI s = BtoI_ITE(b0_bit0, a0, BtoI(0));

    /* Shift-and-add */
    b0 = b0.Shift(-1);
    while (b0 != BtoI(0)) {
        a0 = a0.Shift(1);
        BDD bit = b0.GetBDD(0);
        if (bit.GetID() != bddempty) {
            s = BtoI_ITE(bit, s + a0, s);
        }
        if (s._bddv.GetBDD(0).GetID() == bddnull) break;  // overflow
        b0 = b0.Shift(-1);
    }

    return s;
}

/* ================================================================ */
/*  Division                                                         */
/* ================================================================ */

BtoI operator/(const BtoI& a, const BtoI& b)
{
    /* Terminal cases */
    if (b == BtoI(1)) return a;
    if (a._bddv.GetBDD(0).GetID() == bddnull) return a;
    if (b._bddv.GetBDD(0).GetID() == bddnull) return b;
    if (a == BtoI(0)) return BtoI(0);
    if (a == b) return BtoI(1);

    /* Check division by zero */
    if (BtoI_EQ(b, BtoI(0)) != BtoI(0)) {
        throw std::invalid_argument("BtoI: division by zero");
    }

    /* Result sign */
    BDD rsign = a.GetSignBDD() ^ b.GetSignBDD();

    /* Work with absolute values */
    BDD asign = a.GetSignBDD();
    BDD bsign = b.GetSignBDD();
    BtoI a0 = BtoI_ITE(asign, -a, a);
    BtoI b0 = BtoI_ITE(bsign, -b, b);

    if (a0._bddv.GetBDD(0).GetID() == bddnull) return a0;
    if (b0._bddv.GetBDD(0).GetID() == bddnull) return b0;

    /* Align divisor to dividend */
    int alen = a0.Len();
    b0 = b0.Shift(alen - 2);

    BtoI s(0);
    for (int i = 0; i < alen - 1; i++) {
        s = s.Shift(1);
        BtoI ge = BtoI_GE(a0, b0);
        BDD cond = ge.GetBDD(0);
        a0 = BtoI_ITE(cond, a0 - b0, a0);
        s = s + BtoI(cond);
        b0 = b0.Shift(-1);
    }

    /* Apply sign */
    return BtoI_ITE(rsign, -s, s);
}

/* ================================================================ */
/*  Remainder                                                        */
/* ================================================================ */

BtoI operator%(const BtoI& a, const BtoI& b)
{
    return a - (a / b) * b;
}

/* ================================================================ */
/*  Bitwise operations                                               */
/* ================================================================ */

BtoI operator&(const BtoI& a, const BtoI& b)
{
    int len = (a.Len() > b.Len()) ? a.Len() : b.Len();
    BDDV result(a.GetBDD(0) & b.GetBDD(0));
    for (int i = 1; i < len; i++) {
        result = result || BDDV(a.GetBDD(i) & b.GetBDD(i));
    }
    return BtoI(result).Sup();
}

BtoI operator|(const BtoI& a, const BtoI& b)
{
    int len = (a.Len() > b.Len()) ? a.Len() : b.Len();
    BDDV result(a.GetBDD(0) | b.GetBDD(0));
    for (int i = 1; i < len; i++) {
        result = result || BDDV(a.GetBDD(i) | b.GetBDD(i));
    }
    return BtoI(result).Sup();
}

BtoI operator^(const BtoI& a, const BtoI& b)
{
    int len = (a.Len() > b.Len()) ? a.Len() : b.Len();
    BDDV result(a.GetBDD(0) ^ b.GetBDD(0));
    for (int i = 1; i < len; i++) {
        result = result || BDDV(a.GetBDD(i) ^ b.GetBDD(i));
    }
    return BtoI(result).Sup();
}

/* ================================================================ */
/*  Shift operators                                                  */
/* ================================================================ */

BtoI BtoI::operator<<(const BtoI& fv) const
{
    /* Overflow checks */
    if (_bddv.GetBDD(0).GetID() == bddnull) return *this;
    if (fv._bddv.GetBDD(0).GetID() == bddnull) return fv;

    BDD sign = fv.GetSignBDD();

    /* Split into positive and negative shift cases */
    BtoI p1 = BtoI_ITE(sign, BtoI(0), *this);
    BtoI n1 = BtoI_ITE(sign, *this, BtoI(0));
    BtoI p2 = BtoI_ITE(sign, BtoI(0), fv);
    BtoI n2 = BtoI_ITE(sign, -fv, BtoI(0));

    int fv_len = fv.Len();
    for (int i = 0; i < fv_len; i++) {
        BDD pbit = p2.GetBDD(i);
        if (pbit.GetID() != bddempty) {
            BtoI shifted = p1.Shift(1 << i);
            p1 = BtoI_ITE(pbit, shifted, p1);
        }
        BDD nbit = n2.GetBDD(i);
        if (nbit.GetID() != bddempty) {
            BtoI shifted = n1.Shift(-(1 << i));
            n1 = BtoI_ITE(nbit, shifted, n1);
        }
    }

    return p1 | n1;
}

BtoI BtoI::operator>>(const BtoI& fv) const
{
    return *this << (-fv);
}

/* ================================================================ */
/*  Bounds                                                           */
/* ================================================================ */

BtoI BtoI::UpperBound() const
{
    if (_bddv.GetBDD(0).GetID() == bddnull) return *this;

    int len = Len();
    BDD d = _bddv.GetBDD(len - 1);  // sign bit

    /* Determine sign bit of result and initial condition */
    BDD sign_result;
    BDD cond;

    if (d.GetID() == bddempty) {
        /* Sign bit always 0: good for max */
        sign_result = BDD(0);
        cond = BDD(1);
    } else if (d.GetID() == bddsingle) {
        /* Sign bit always 1: constant negative, can't avoid */
        sign_result = BDD(1);
        cond = BDD(1);
    } else {
        /* Variable-dependent: prefer sign=0 */
        sign_result = BDD(0);
        cond = ~d;
    }

    /* Greedy from MSB-1 down to 0: try to set each bit to 1 */
    std::vector<BDD> bits(len);
    bits[len - 1] = sign_result;

    for (int i = len - 2; i >= 0; i--) {
        d = _bddv.GetBDD(i);
        BDD c0 = cond & d;

        if (c0.GetID() == bddnull) return BtoI(BDD(-1));

        bits[i] = (c0.GetID() != bddempty) ? BDD(1) : BDD(0);

        /* Narrow condition: if we chose 1, restrict to those assignments */
        if (c0.GetID() != bddempty) {
            cond = c0;
        }
        /* else: bit forced to 0, condition unchanged */
    }

    /* Build BDDV from bits array (index 0 = LSB) */
    BDDV result(bits[0]);
    for (int i = 1; i < len; i++) {
        result = result || BDDV(bits[i]);
    }

    return BtoI(result).Sup();
}

BtoI BtoI::UpperBound(const BDD& f) const
{
    if (_bddv.GetBDD(0).GetID() == bddnull) return *this;
    if (f.GetID() == bddnull) return BtoI(BDD(-1));
    if (f.GetID() == bddempty) return BtoI(BDD(-1));

    int len = Len();
    BDD d = _bddv.GetBDD(len - 1);  // sign bit

    /* In 2's complement the sign bit has weight -2^(n-1).
       To maximise, prefer sign=0 (avoid large negative contribution).
       Restrict to f-satisfying assignments throughout. */
    BDD sign_result;
    BDD cond;

    BDD nonneg = f & ~d;  /* f-satisfying assignments where sign=0 */
    if (nonneg.GetID() != bddempty) {
        sign_result = BDD(0);
        cond = nonneg;
    } else {
        /* All f-satisfying assignments are negative */
        sign_result = BDD(1);
        cond = f;
    }

    std::vector<BDD> bits(len);
    bits[len - 1] = sign_result;

    for (int i = len - 2; i >= 0; i--) {
        d = _bddv.GetBDD(i);
        BDD c0 = cond & d;

        if (c0.GetID() == bddnull) return BtoI(BDD(-1));

        bits[i] = (c0.GetID() != bddempty) ? BDD(1) : BDD(0);

        if (c0.GetID() != bddempty) {
            cond = c0;
        }
    }

    BDDV result(bits[0]);
    for (int i = 1; i < len; i++) {
        result = result || BDDV(bits[i]);
    }

    return BtoI(result).Sup();
}

BtoI BtoI::LowerBound() const
{
    return -((-*this).UpperBound());
}

BtoI BtoI::LowerBound(const BDD& f) const
{
    return -((-*this).UpperBound(f));
}

/* ================================================================ */
/*  Cofactor / expansion                                             */
/* ================================================================ */

BtoI BtoI::At0(int v) const
{
    return BtoI(_bddv.At0(v)).Sup();
}

BtoI BtoI::At1(int v) const
{
    return BtoI(_bddv.At1(v)).Sup();
}

BtoI BtoI::Cofact(const BtoI& fv) const
{
    BDD cond = BtoI_NE(fv, BtoI(0)).GetBDD(0);
    BDDV a(cond, Len());
    return BtoI(_bddv.Cofact(a)).Sup();
}

BtoI BtoI::Spread(int k) const
{
    return BtoI(_bddv.Spread(k)).Sup();
}

/* ================================================================ */
/*  Access functions                                                 */
/* ================================================================ */

int BtoI::Top() const { return _bddv.Top(); }

BDD BtoI::GetSignBDD() const { return _bddv.GetBDD(Len() - 1); }

BDD BtoI::GetBDD(int i) const
{
    if (i >= Len()) return _bddv.GetBDD(Len() - 1);  // sign extension
    return _bddv.GetBDD(i);
}

BDDV BtoI::GetMetaBDDV() const { return _bddv; }

int BtoI::Len() const { return _bddv.Len(); }

/* ================================================================ */
/*  Conversion: GetInt                                               */
/* ================================================================ */

int BtoI::GetInt() const
{
    if (_bddv.GetBDD(0).GetID() == bddnull) return 0;
    if (Top() > 0) {
        throw std::invalid_argument("BtoI::GetInt: non-constant BtoI");
    }

    /* Check sign */
    BDD sign = GetSignBDD();
    if (sign.GetID() != bddempty) {
        /* Negative */
        return -((-*this).GetInt());
    }

    /* Non-negative constant */
    int len = Len();
    if (len > 32) len = 32;

    int val = 0;
    for (int i = len - 2; i >= 0; i--) {
        val <<= 1;
        if (_bddv.GetBDD(i).GetID() != bddempty) {
            val |= 1;
        }
    }
    return val;
}

/* ================================================================ */
/*  Conversion: StrNum10                                             */
/* ================================================================ */

int BtoI::StrNum10(char* s) const
{
    if (_bddv.GetBDD(0).GetID() == bddnull) {
        std::strcpy(s, "0");
        return 1;
    }
    if (Top() > 0) {
        throw std::invalid_argument("BtoI::StrNum10: non-constant BtoI");
    }

    BtoI f = *this;

    /* Handle negative */
    bool neg = false;
    if (f.GetSignBDD().GetID() != bddempty) {
        neg = true;
        f = -f;
    }

    if (f.Len() <= 29) {
        int val = f.GetInt();
        if (neg) {
            std::sprintf(s, "-%d", val);
        } else {
            std::sprintf(s, "%d", val);
        }
        return 0;
    }

    /* Large number: divide by 10^9 repeatedly */
    BtoI divisor(1000000000);
    char buf[1024];
    int pos = 0;

    while (f != BtoI(0)) {
        BtoI rem = f % divisor;
        f = f / divisor;
        if (rem._bddv.GetBDD(0).GetID() == bddnull ||
            f._bddv.GetBDD(0).GetID() == bddnull) {
            std::strcpy(s, "0");
            return 1;
        }
        int r = rem.GetInt();
        if (f == BtoI(0)) {
            /* Last block: no leading zeros */
            int len = std::sprintf(buf + pos, "%d", r);
            pos += len;
        } else {
            /* Middle block: 9-digit with leading zeros */
            int len = std::sprintf(buf + pos, "%09d", r);
            pos += len;
        }
    }

    if (pos == 0) {
        std::strcpy(s, "0");
        return 0;
    }

    /* Reverse blocks: buf has blocks in reverse order (LSB first) */
    /* Actually we built blocks from least significant to most,
       so we need to reverse */
    int out = 0;
    if (neg) s[out++] = '-';

    /* The blocks in buf are: block0 (units), block1 (10^9), block2 (10^18)...
       We printed them left-to-right, so buf has units first.
       We need most significant first. */
    /* Count blocks */
    /* Actually, let's rebuild properly.
       Each block except the last is 9 chars. The last (most significant) is variable. */
    /* Simpler: just reverse the string in buf */
    /* Wait, each block was printed as a number, not reversed.
       block0 = rem0 as "%09d" (or "%d" for last)
       block1 = rem1 as "%09d" (or "%d" for last)
       The most significant block should come first in output. */

    /* We need to output blocks in reverse order */
    /* Find block boundaries: all blocks except possibly the last are 9 chars */
    /* The last written block (at highest position) has variable length */

    /* Let's redo: collect blocks in a vector-like manner */
    /* Actually, let's just do a simpler approach for the string reversal */
    /* The blocks are laid out as: [9-digit][9-digit]...[variable-length] in buf */
    /* We need to reverse the block order */

    /* Find the last block (variable length) */
    int last_start = pos;
    /* Walk backwards to find the start of the last block */
    /* All blocks before the last are 9 digits */
    int nblocks = 0;
    int remaining = pos;
    while (remaining > 9) {
        remaining -= 9;
        nblocks++;
    }
    /* remaining chars is the last (most significant) block */
    last_start = nblocks * 9;

    /* Output: last block first, then earlier blocks in reverse order */
    std::memcpy(s + out, buf + last_start, pos - last_start);
    out += (pos - last_start);
    for (int i = nblocks - 1; i >= 0; i--) {
        std::memcpy(s + out, buf + i * 9, 9);
        out += 9;
    }
    s[out] = '\0';
    return 0;
}

/* ================================================================ */
/*  Conversion: StrNum16                                             */
/* ================================================================ */

int BtoI::StrNum16(char* s) const
{
    if (_bddv.GetBDD(0).GetID() == bddnull) {
        std::strcpy(s, "0");
        return 1;
    }
    if (Top() > 0) {
        throw std::invalid_argument("BtoI::StrNum16: non-constant BtoI");
    }

    BtoI f = *this;

    bool neg = false;
    if (f.GetSignBDD().GetID() != bddempty) {
        neg = true;
        f = -f;
    }

    if (f.Len() <= 27) {
        int val = f.GetInt();
        if (neg) {
            std::sprintf(s, "-%x", val);
        } else {
            std::sprintf(s, "%x", val);
        }
        return 0;
    }

    /* Large number: extract 28-bit (7 hex digit) blocks */
    BtoI mask = BtoI((1 << 28) - 1);
    BtoI fv0 = f;
    char buf[1024];
    int pos = 0;

    while (fv0 != BtoI(0)) {
        BtoI block = fv0 & mask;
        fv0 = fv0 >> BtoI(28);
        if (block._bddv.GetBDD(0).GetID() == bddnull ||
            fv0._bddv.GetBDD(0).GetID() == bddnull) {
            std::strcpy(s, "0");
            return 1;
        }
        int r = block.GetInt();
        if (fv0 == BtoI(0)) {
            int len = std::sprintf(buf + pos, "%x", r);
            pos += len;
        } else {
            int len = std::sprintf(buf + pos, "%07x", r);
            pos += len;
        }
    }

    if (pos == 0) {
        std::strcpy(s, "0");
        return 0;
    }

    /* Reverse block order (same approach as StrNum10) */
    int out = 0;
    if (neg) s[out++] = '-';

    int nblocks = 0;
    int remaining = pos;
    while (remaining > 7) {
        remaining -= 7;
        nblocks++;
    }
    int last_start = nblocks * 7;
    std::memcpy(s + out, buf + last_start, pos - last_start);
    out += (pos - last_start);
    for (int i = nblocks - 1; i >= 0; i--) {
        std::memcpy(s + out, buf + i * 7, 7);
        out += 7;
    }
    s[out] = '\0';
    return 0;
}

/* ================================================================ */
/*  Other member functions                                           */
/* ================================================================ */

uint64_t BtoI::Size() const { return _bddv.Size(); }

void BtoI::Print() const { _bddv.Print(); }

/* ================================================================ */
/*  BtoI_ITE (BDD condition)                                         */
/* ================================================================ */

BtoI BtoI_ITE(const BDD& f, const BtoI& a, const BtoI& b)
{
    if (a == b) return a;

    int len = (a.Len() > b.Len()) ? a.Len() : b.Len();
    BDDV result(( f & a.GetBDD(0)) | (~f & b.GetBDD(0)));
    for (int i = 1; i < len; i++) {
        result = result || BDDV((f & a.GetBDD(i)) | (~f & b.GetBDD(i)));
    }
    return BtoI(result).Sup();
}

/* ================================================================ */
/*  BtoI_ITE (BtoI condition)                                        */
/* ================================================================ */

BtoI BtoI_ITE(const BtoI& a, const BtoI& b, const BtoI& c)
{
    BDD cond = ~BtoI_EQ(a, BtoI(0)).GetBDD(0);
    return BtoI_ITE(cond, b, c);
}

/* ================================================================ */
/*  Comparison functions                                             */
/* ================================================================ */

BtoI BtoI_EQ(const BtoI& a, const BtoI& b)
{
    BtoI c = a - b;
    int len = c.Len();
    BDD cond(0);

    for (int i = 0; i < len; i++) {
        cond = cond | c.GetBDD(i);
        if (cond.GetID() == bddsingle) break;  // always nonzero
    }

    return BtoI(~cond);
}

BtoI BtoI_GT(const BtoI& a, const BtoI& b)
{
    BtoI diff = b - a;
    return BtoI(diff.GetSignBDD());
}

BtoI BtoI_LT(const BtoI& a, const BtoI& b)
{
    BtoI diff = a - b;
    return BtoI(diff.GetSignBDD());
}

BtoI BtoI_GE(const BtoI& a, const BtoI& b)
{
    return !BtoI_LT(a, b);
}

BtoI BtoI_LE(const BtoI& a, const BtoI& b)
{
    return !BtoI_GT(a, b);
}

BtoI BtoI_NE(const BtoI& a, const BtoI& b)
{
    return !BtoI_EQ(a, b);
}

/* ================================================================ */
/*  BtoI_atoi                                                        */
/* ================================================================ */

static bool is_hex_digit(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

static BtoI atoi16(const char* s)
{
    int slen = static_cast<int>(std::strlen(s));
    if (slen == 0) {
        throw std::invalid_argument("BtoI_atoi: empty hex string");
    }
    for (int k = 0; k < slen; k++) {
        if (!is_hex_digit(s[k])) {
            throw std::invalid_argument(
                std::string("BtoI_atoi: invalid hex character '") +
                s[k] + "'");
        }
    }
    BtoI result(0);
    int i = 0;
    while (i < slen) {
        int block_len = (slen - i > 6) ? 6 : (slen - i);
        char buf[8];
        std::memcpy(buf, s + i, block_len);
        buf[block_len] = '\0';
        long val = std::strtol(buf, NULL, 16);
        if (i == 0) {
            result = BtoI(static_cast<int>(val));
        } else {
            result = (result << BtoI(block_len * 4)) + BtoI(static_cast<int>(val));
        }
        i += block_len;
    }
    return result;
}

static BtoI atoi2(const char* s)
{
    if (s[0] == '\0') {
        throw std::invalid_argument("BtoI_atoi: empty binary string");
    }
    for (int k = 0; s[k] != '\0'; k++) {
        if (s[k] != '0' && s[k] != '1') {
            throw std::invalid_argument(
                std::string("BtoI_atoi: invalid binary character '") +
                s[k] + "'");
        }
    }
    BtoI result(0);
    for (int i = 0; s[i] != '\0'; i++) {
        result = (result << BtoI(1)) + BtoI(s[i] == '1' ? 1 : 0);
    }
    return result;
}

static BtoI atoi10(const char* s)
{
    int slen = static_cast<int>(std::strlen(s));
    if (slen == 0) {
        throw std::invalid_argument("BtoI_atoi: empty decimal string");
    }
    for (int k = 0; k < slen; k++) {
        if (s[k] < '0' || s[k] > '9') {
            throw std::invalid_argument(
                std::string("BtoI_atoi: invalid decimal character '") +
                s[k] + "'");
        }
    }
    BtoI result(0);
    int i = 0;
    while (i < slen) {
        int block_len = (slen - i > 9) ? 9 : (slen - i);
        char buf[16];
        std::memcpy(buf, s + i, block_len);
        buf[block_len] = '\0';
        long val = std::strtol(buf, NULL, 10);
        if (i == 0) {
            result = BtoI(static_cast<int>(val));
        } else {
            /* Compute 10^block_len */
            int pow10 = 1;
            for (int j = 0; j < block_len; j++) pow10 *= 10;
            result = result * BtoI(pow10) + BtoI(static_cast<int>(val));
        }
        i += block_len;
    }
    return result;
}

BtoI BtoI_atoi(const char* s)
{
    bool neg = false;
    if (*s == '-') {
        neg = true;
        s++;
    } else if (*s == '+') {
        s++;
    }

    BtoI result;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        result = atoi16(s + 2);
    } else if (s[0] == '0' && (s[1] == 'b' || s[1] == 'B')) {
        result = atoi2(s + 2);
    } else {
        result = atoi10(s);
    }

    if (neg) result = -result;
    return result;
}
