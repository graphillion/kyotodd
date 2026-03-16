#ifndef BIGINT_BIGINT_HPP
#define BIGINT_BIGINT_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace bigint {

class BigInt {
public:
    // ---------------------------------------------------------------
    // Constructors
    // ---------------------------------------------------------------
    BigInt() : limbs_(), negative_(false) {}

    BigInt(int value) : limbs_(), negative_(false) {
        if (value == 0) return;
        if (value < 0) {
            negative_ = true;
            // handle INT_MIN: cast to unsigned first
            uint32_t abs_val = static_cast<uint32_t>(
                -static_cast<int64_t>(value));
            limbs_.push_back(abs_val);
        } else {
            limbs_.push_back(static_cast<uint32_t>(value));
        }
    }

    BigInt(unsigned int value) : limbs_(), negative_(false) {
        if (value != 0) {
            limbs_.push_back(static_cast<uint32_t>(value));
        }
    }

    BigInt(long value) : BigInt(static_cast<long long>(value)) {}
    BigInt(unsigned long value) : BigInt(static_cast<unsigned long long>(value)) {}

    BigInt(long long value) : limbs_(), negative_(false) {
        if (value == 0) return;
        unsigned long long abs_val;
        if (value < 0) {
            negative_ = true;
            abs_val = static_cast<unsigned long long>(
                -(static_cast<long long>(value + 1)) ) + 1ULL;
        } else {
            abs_val = static_cast<unsigned long long>(value);
        }
        limbs_.push_back(static_cast<uint32_t>(abs_val & 0xFFFFFFFFULL));
        uint32_t hi = static_cast<uint32_t>(abs_val >> 32);
        if (hi != 0) {
            limbs_.push_back(hi);
        }
    }

    BigInt(unsigned long long value) : limbs_(), negative_(false) {
        if (value == 0) return;
        limbs_.push_back(static_cast<uint32_t>(value & 0xFFFFFFFFULL));
        uint32_t hi = static_cast<uint32_t>(value >> 32);
        if (hi != 0) {
            limbs_.push_back(hi);
        }
    }

    explicit BigInt(const std::string& str) : limbs_(), negative_(false) {
        if (str.empty()) {
            throw std::invalid_argument("BigInt: empty string");
        }

        std::size_t pos = 0;
        bool neg = false;
        if (str[pos] == '-') {
            neg = true;
            ++pos;
        } else if (str[pos] == '+') {
            ++pos;
        }

        if (pos >= str.size()) {
            throw std::invalid_argument("BigInt: no digits in string");
        }

        // Skip leading zeros
        std::size_t first_nonzero = pos;
        while (first_nonzero < str.size() && str[first_nonzero] == '0') {
            ++first_nonzero;
        }
        if (first_nonzero == str.size()) {
            // All zeros → value is 0
            return;
        }

        // Validate all digits
        for (std::size_t i = pos; i < str.size(); ++i) {
            if (str[i] < '0' || str[i] > '9') {
                throw std::invalid_argument(
                    "BigInt: invalid character in string");
            }
        }

        // Process 9 digits at a time (10^9 base)
        // We build up the number by: result = result * 10^k + chunk
        const uint32_t base9 = 1000000000U; // 10^9
        std::size_t len = str.size() - first_nonzero;
        const char* digits = str.c_str() + first_nonzero;

        for (std::size_t i = 0; i < len; ) {
            // Determine chunk size: first chunk may be shorter
            std::size_t remaining = len - i;
            std::size_t chunk_size = remaining % 9;
            if (chunk_size == 0) chunk_size = 9;

            // Parse the chunk
            uint32_t chunk = 0;
            for (std::size_t j = 0; j < chunk_size; ++j) {
                chunk = chunk * 10 + static_cast<uint32_t>(digits[i + j] - '0');
            }

            // Compute multiplier: 10^chunk_size
            uint32_t multiplier = 1;
            for (std::size_t j = 0; j < chunk_size; ++j) {
                multiplier *= 10;
            }

            // result = result * multiplier + chunk
            mul_single_add(multiplier, chunk);

            i += chunk_size;
        }

        negative_ = neg;
        normalize();
    }

    BigInt(const BigInt& other) = default;
    BigInt(BigInt&& other) noexcept
        : limbs_(std::move(other.limbs_)), negative_(other.negative_) {
        other.negative_ = false;
    }

    // ---------------------------------------------------------------
    // Assignment
    // ---------------------------------------------------------------
    BigInt& operator=(const BigInt&) = default;
    BigInt& operator=(BigInt&& other) noexcept {
        limbs_ = std::move(other.limbs_);
        negative_ = other.negative_;
        other.negative_ = false;
        return *this;
    }

    // ---------------------------------------------------------------
    // Unary operators
    // ---------------------------------------------------------------
    BigInt operator+() const { return *this; }
    BigInt operator-() const {
        BigInt result(*this);
        if (!result.is_zero()) {
            result.negative_ = !result.negative_;
        }
        return result;
    }

    // ---------------------------------------------------------------
    // Arithmetic: += -= *=
    // ---------------------------------------------------------------
    BigInt& operator+=(const BigInt& rhs) {
        if (negative_ == rhs.negative_) {
            // Same sign: add magnitudes
            BigInt sum = add_abs(*this, rhs);
            limbs_ = std::move(sum.limbs_);
            // keep negative_ unchanged
        } else {
            // Different signs: subtract magnitudes
            int cmp = compare_abs(*this, rhs);
            if (cmp == 0) {
                limbs_.clear();
                negative_ = false;
            } else if (cmp > 0) {
                BigInt diff = sub_abs(*this, rhs);
                limbs_ = std::move(diff.limbs_);
                // keep negative_ unchanged
            } else {
                BigInt diff = sub_abs(rhs, *this);
                limbs_ = std::move(diff.limbs_);
                negative_ = rhs.negative_;
            }
        }
        normalize();
        return *this;
    }

    BigInt& operator-=(const BigInt& rhs) {
        if (negative_ != rhs.negative_) {
            // Different signs: add magnitudes
            BigInt sum = add_abs(*this, rhs);
            limbs_ = std::move(sum.limbs_);
            // keep negative_ unchanged
        } else {
            // Same sign: subtract magnitudes
            int cmp = compare_abs(*this, rhs);
            if (cmp == 0) {
                limbs_.clear();
                negative_ = false;
            } else if (cmp > 0) {
                BigInt diff = sub_abs(*this, rhs);
                limbs_ = std::move(diff.limbs_);
                // keep negative_ unchanged
            } else {
                BigInt diff = sub_abs(rhs, *this);
                limbs_ = std::move(diff.limbs_);
                negative_ = !negative_;
            }
        }
        normalize();
        return *this;
    }

    BigInt& operator*=(const BigInt& rhs) {
        if (is_zero() || rhs.is_zero()) {
            limbs_.clear();
            negative_ = false;
            return *this;
        }
        bool result_neg = (negative_ != rhs.negative_);
        BigInt product = mul_abs(*this, rhs);
        limbs_ = std::move(product.limbs_);
        negative_ = result_neg;
        normalize();
        return *this;
    }

    // ---------------------------------------------------------------
    // Arithmetic: binary operators
    // ---------------------------------------------------------------
    friend BigInt operator+(const BigInt& a, const BigInt& b) {
        BigInt result(a);
        result += b;
        return result;
    }

    friend BigInt operator-(const BigInt& a, const BigInt& b) {
        BigInt result(a);
        result -= b;
        return result;
    }

    friend BigInt operator*(const BigInt& a, const BigInt& b) {
        if (a.is_zero() || b.is_zero()) return BigInt();
        BigInt result = mul_abs(a, b);
        result.negative_ = (a.negative_ != b.negative_);
        result.normalize();
        return result;
    }

    // ---------------------------------------------------------------
    // Comparison
    // ---------------------------------------------------------------
    friend bool operator==(const BigInt& a, const BigInt& b) {
        if (a.negative_ != b.negative_) return false;
        return a.limbs_ == b.limbs_;
    }

    friend bool operator!=(const BigInt& a, const BigInt& b) {
        return !(a == b);
    }

    friend bool operator<(const BigInt& a, const BigInt& b) {
        if (a.negative_ != b.negative_) {
            return a.negative_;
        }
        int cmp = compare_abs(a, b);
        return a.negative_ ? (cmp > 0) : (cmp < 0);
    }

    friend bool operator>(const BigInt& a, const BigInt& b) {
        return b < a;
    }

    friend bool operator<=(const BigInt& a, const BigInt& b) {
        return !(b < a);
    }

    friend bool operator>=(const BigInt& a, const BigInt& b) {
        return !(a < b);
    }

    // ---------------------------------------------------------------
    // Bit shift: <<= >>=
    // ---------------------------------------------------------------
    BigInt& operator<<=(std::size_t shift) {
        if (is_zero() || shift == 0) return *this;

        std::size_t limb_shift = shift / 32;
        std::size_t bit_shift = shift % 32;

        if (bit_shift == 0) {
            limbs_.insert(limbs_.begin(), limb_shift, 0);
        } else {
            std::size_t old_size = limbs_.size();
            limbs_.resize(old_size + limb_shift + 1, 0);

            // Shift bits within limbs (from high to low to avoid overlap)
            for (std::size_t i = old_size; i > 0; --i) {
                limbs_[i - 1 + limb_shift + 1] |=
                    limbs_[i - 1] >> (32 - bit_shift);
                limbs_[i - 1 + limb_shift] =
                    limbs_[i - 1] << bit_shift;
            }
            // Clear the low limbs
            for (std::size_t i = 0; i < limb_shift; ++i) {
                limbs_[i] = 0;
            }
        }
        normalize();
        return *this;
    }

    BigInt& operator>>=(std::size_t shift) {
        if (is_zero() || shift == 0) return *this;

        std::size_t limb_shift = shift / 32;
        std::size_t bit_shift = shift % 32;

        // For negative numbers: arithmetic right shift (round toward -inf)
        // We need to check if any shifted-out bits are non-zero
        bool round_down = false;
        if (negative_) {
            // Check if any of the shifted-out bits are set
            for (std::size_t i = 0; i < limb_shift && i < limbs_.size(); ++i) {
                if (limbs_[i] != 0) {
                    round_down = true;
                    break;
                }
            }
            if (!round_down && bit_shift > 0 && limb_shift < limbs_.size()) {
                uint32_t mask = (1U << bit_shift) - 1U;
                if (limbs_[limb_shift] & mask) {
                    round_down = true;
                }
            }
        }

        if (limb_shift >= limbs_.size()) {
            limbs_.clear();
            negative_ = false;
            if (round_down) {
                // Result is -1
                limbs_.push_back(1);
                negative_ = true;
            }
            return *this;
        }

        if (bit_shift == 0) {
            limbs_.erase(limbs_.begin(),
                         limbs_.begin() +
                             static_cast<std::ptrdiff_t>(limb_shift));
        } else {
            std::size_t new_size = limbs_.size() - limb_shift;
            for (std::size_t i = 0; i < new_size; ++i) {
                limbs_[i] = limbs_[i + limb_shift] >> bit_shift;
                if (i + limb_shift + 1 < limbs_.size()) {
                    limbs_[i] |=
                        limbs_[i + limb_shift + 1] << (32 - bit_shift);
                }
            }
            limbs_.resize(new_size);
        }
        normalize();

        if (round_down) {
            // Add 1 to magnitude for negative numbers
            // If magnitude became zero after shift, we need to restore negative flag
            if (is_zero()) {
                limbs_.push_back(1);
                negative_ = true;
            } else {
                add_one_to_magnitude();
            }
            normalize();
        }

        return *this;
    }

    friend BigInt operator<<(const BigInt& a, std::size_t shift) {
        BigInt result(a);
        result <<= shift;
        return result;
    }

    friend BigInt operator>>(const BigInt& a, std::size_t shift) {
        BigInt result(a);
        result >>= shift;
        return result;
    }

    // ---------------------------------------------------------------
    // Bitwise: ~ & | ^
    // ---------------------------------------------------------------

    // ~x = -(x + 1) for two's complement semantics
    BigInt operator~() const {
        if (negative_) {
            // ~(-x) = x - 1
            BigInt result(*this);
            result.negative_ = false;
            result.sub_one_from_magnitude();
            result.normalize();
            return result;
        } else {
            // ~x = -(x + 1)
            BigInt result(*this);
            result.add_one_to_magnitude();
            result.negative_ = true;
            result.normalize();
            return result;
        }
    }

    BigInt& operator&=(const BigInt& rhs) {
        *this = *this & rhs;
        return *this;
    }

    BigInt& operator|=(const BigInt& rhs) {
        *this = *this | rhs;
        return *this;
    }

    BigInt& operator^=(const BigInt& rhs) {
        *this = *this ^ rhs;
        return *this;
    }

    friend BigInt operator&(const BigInt& a, const BigInt& b) {
        return bitwise_op(a, b, BitOp::AND);
    }

    friend BigInt operator|(const BigInt& a, const BigInt& b) {
        return bitwise_op(a, b, BitOp::OR);
    }

    friend BigInt operator^(const BigInt& a, const BigInt& b) {
        return bitwise_op(a, b, BitOp::XOR);
    }

    // ---------------------------------------------------------------
    // String conversion
    // ---------------------------------------------------------------
    std::string get_str() const { return to_string(); }

    std::string to_string() const {
        if (is_zero()) return "0";

        BigInt tmp(*this);
        tmp.negative_ = false;

        std::string result;
        const uint32_t base = 1000000000U; // 10^9

        while (!tmp.is_zero()) {
            uint32_t remainder = tmp.divmod_single(base);
            std::string chunk = std::to_string(remainder);
            // Pad to 9 digits (except for the last chunk which is the
            // most significant)
            while (chunk.size() < 9) {
                chunk = "0" + chunk;
            }
            result = chunk + result;
        }

        // Remove leading zeros
        std::size_t first_nonzero = result.find_first_not_of('0');
        if (first_nonzero != std::string::npos) {
            result = result.substr(first_nonzero);
        }

        if (negative_) {
            result = "-" + result;
        }
        return result;
    }

    std::string to_hex_upper() const {
        if (is_zero()) return "0";

        const char* hex_digits = "0123456789ABCDEF";
        std::string result;

        // limbs_ is little-endian uint32_t; iterate from MSB to LSB
        bool leading = true;
        for (std::size_t i = limbs_.size(); i > 0; --i) {
            uint32_t limb = limbs_[i - 1];
            for (int nibble = 7; nibble >= 0; --nibble) {
                uint32_t d = (limb >> (nibble * 4)) & 0xFU;
                if (leading && d == 0) continue;
                leading = false;
                result += hex_digits[d];
            }
        }

        if (negative_) {
            result = "-" + result;
        }
        return result;
    }

    // ---------------------------------------------------------------
    // Stream I/O
    // ---------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os, const BigInt& val) {
        return os << val.to_string();
    }

    friend std::istream& operator>>(std::istream& is, BigInt& val) {
        std::string s;
        is >> s;
        if (is) {
            try {
                val = BigInt(s);
            } catch (const std::invalid_argument&) {
                is.setstate(std::ios::failbit);
            }
        }
        return is;
    }

    // ---------------------------------------------------------------
    // Utility
    // ---------------------------------------------------------------
    friend BigInt abs(const BigInt& x) {
        BigInt result(x);
        result.negative_ = false;
        return result;
    }

    int sign() const {
        if (is_zero()) return 0;
        return negative_ ? -1 : 1;
    }

    bool is_zero() const {
        return limbs_.empty();
    }

    void swap(BigInt& other) noexcept {
        limbs_.swap(other.limbs_);
        std::swap(negative_, other.negative_);
    }

    explicit operator bool() const {
        return !is_zero();
    }

private:
    std::vector<uint32_t> limbs_; // little-endian
    bool negative_;               // false for zero

    // ---------------------------------------------------------------
    // normalize: remove trailing zero limbs, fix zero sign
    // ---------------------------------------------------------------
    void normalize() {
        while (!limbs_.empty() && limbs_.back() == 0) {
            limbs_.pop_back();
        }
        if (limbs_.empty()) {
            negative_ = false;
        }
    }

    // ---------------------------------------------------------------
    // compare_abs: compare absolute values, returns -1, 0, +1
    // ---------------------------------------------------------------
    static int compare_abs(const BigInt& a, const BigInt& b) {
        if (a.limbs_.size() != b.limbs_.size()) {
            return a.limbs_.size() < b.limbs_.size() ? -1 : 1;
        }
        for (std::size_t i = a.limbs_.size(); i > 0; --i) {
            if (a.limbs_[i - 1] != b.limbs_[i - 1]) {
                return a.limbs_[i - 1] < b.limbs_[i - 1] ? -1 : 1;
            }
        }
        return 0;
    }

    // ---------------------------------------------------------------
    // add_abs: add absolute values
    // ---------------------------------------------------------------
    static BigInt add_abs(const BigInt& a, const BigInt& b) {
        BigInt result;
        std::size_t max_size = std::max(a.limbs_.size(), b.limbs_.size());
        result.limbs_.resize(max_size + 1, 0);

        uint64_t carry = 0;
        for (std::size_t i = 0; i < max_size || carry; ++i) {
            uint64_t sum = carry;
            if (i < a.limbs_.size()) sum += a.limbs_[i];
            if (i < b.limbs_.size()) sum += b.limbs_[i];
            if (i < result.limbs_.size()) {
                result.limbs_[i] = static_cast<uint32_t>(sum & 0xFFFFFFFFULL);
            } else {
                result.limbs_.push_back(
                    static_cast<uint32_t>(sum & 0xFFFFFFFFULL));
            }
            carry = sum >> 32;
        }
        result.normalize();
        return result;
    }

    // ---------------------------------------------------------------
    // sub_abs: subtract absolute values, |a| >= |b| required
    // ---------------------------------------------------------------
    static BigInt sub_abs(const BigInt& a, const BigInt& b) {
        BigInt result;
        result.limbs_.resize(a.limbs_.size(), 0);

        int64_t borrow = 0;
        for (std::size_t i = 0; i < a.limbs_.size(); ++i) {
            int64_t diff = static_cast<int64_t>(a.limbs_[i]) - borrow;
            if (i < b.limbs_.size()) {
                diff -= static_cast<int64_t>(b.limbs_[i]);
            }
            if (diff < 0) {
                diff += static_cast<int64_t>(1ULL << 32);
                borrow = 1;
            } else {
                borrow = 0;
            }
            result.limbs_[i] = static_cast<uint32_t>(diff);
        }
        result.normalize();
        return result;
    }

    // ---------------------------------------------------------------
    // mul_abs: multiply absolute values (schoolbook O(n*m))
    // ---------------------------------------------------------------
    static BigInt mul_abs(const BigInt& a, const BigInt& b) {
        if (a.is_zero() || b.is_zero()) return BigInt();

        BigInt result;
        result.limbs_.resize(a.limbs_.size() + b.limbs_.size(), 0);

        for (std::size_t i = 0; i < a.limbs_.size(); ++i) {
            uint64_t carry = 0;
            for (std::size_t j = 0; j < b.limbs_.size(); ++j) {
                uint64_t prod =
                    static_cast<uint64_t>(a.limbs_[i]) *
                    static_cast<uint64_t>(b.limbs_[j]) +
                    result.limbs_[i + j] + carry;
                result.limbs_[i + j] =
                    static_cast<uint32_t>(prod & 0xFFFFFFFFULL);
                carry = prod >> 32;
            }
            // Propagate remaining carry
            for (std::size_t k = i + b.limbs_.size();
                 carry && k < result.limbs_.size(); ++k) {
                uint64_t sum =
                    static_cast<uint64_t>(result.limbs_[k]) + carry;
                result.limbs_[k] =
                    static_cast<uint32_t>(sum & 0xFFFFFFFFULL);
                carry = sum >> 32;
            }
        }
        result.normalize();
        return result;
    }

    // ---------------------------------------------------------------
    // divmod_single: divide by single uint32_t, return remainder
    // Used internally for to_string()
    // ---------------------------------------------------------------
    uint32_t divmod_single(uint32_t divisor) {
        uint64_t remainder = 0;
        for (std::size_t i = limbs_.size(); i > 0; --i) {
            uint64_t cur = (remainder << 32) | limbs_[i - 1];
            limbs_[i - 1] = static_cast<uint32_t>(cur / divisor);
            remainder = cur % divisor;
        }
        normalize();
        return static_cast<uint32_t>(remainder);
    }

    // ---------------------------------------------------------------
    // mul_single_add: this = this * multiplier + addend
    // Used internally for string constructor
    // ---------------------------------------------------------------
    void mul_single_add(uint32_t multiplier, uint32_t addend) {
        uint64_t carry = addend;
        for (std::size_t i = 0; i < limbs_.size(); ++i) {
            uint64_t prod =
                static_cast<uint64_t>(limbs_[i]) * multiplier + carry;
            limbs_[i] = static_cast<uint32_t>(prod & 0xFFFFFFFFULL);
            carry = prod >> 32;
        }
        if (carry) {
            limbs_.push_back(static_cast<uint32_t>(carry));
        }
    }

    // ---------------------------------------------------------------
    // add_one_to_magnitude: |this| += 1
    // ---------------------------------------------------------------
    void add_one_to_magnitude() {
        uint64_t carry = 1;
        for (std::size_t i = 0; i < limbs_.size() && carry; ++i) {
            uint64_t sum = static_cast<uint64_t>(limbs_[i]) + carry;
            limbs_[i] = static_cast<uint32_t>(sum & 0xFFFFFFFFULL);
            carry = sum >> 32;
        }
        if (carry) {
            limbs_.push_back(static_cast<uint32_t>(carry));
        }
    }

    // ---------------------------------------------------------------
    // sub_one_from_magnitude: |this| -= 1 (assumes not zero)
    // ---------------------------------------------------------------
    void sub_one_from_magnitude() {
        for (std::size_t i = 0; i < limbs_.size(); ++i) {
            if (limbs_[i] > 0) {
                --limbs_[i];
                return;
            }
            limbs_[i] = 0xFFFFFFFFU;
        }
    }

    // ---------------------------------------------------------------
    // Bitwise operation helper
    // ---------------------------------------------------------------
    enum class BitOp { AND, OR, XOR };

    // Convert to two's complement representation for bitwise ops
    // Returns pair: (limbs in two's complement, is_negative)
    // For negative x: two's complement of magnitude
    struct TwosComplement {
        std::vector<uint32_t> digits;
        bool infinite_ones; // true if conceptually infinite 1s above
    };

    static TwosComplement to_twos_complement(const BigInt& x) {
        TwosComplement tc;
        if (!x.negative_) {
            tc.digits = x.limbs_;
            tc.infinite_ones = false;
        } else {
            // -x is stored as magnitude x. Two's complement = ~(x-1)
            // which is the same as complement of (magnitude - 1)
            tc.digits = x.limbs_;
            // Subtract 1
            for (std::size_t i = 0; i < tc.digits.size(); ++i) {
                if (tc.digits[i] > 0) {
                    --tc.digits[i];
                    break;
                }
                tc.digits[i] = 0xFFFFFFFFU;
            }
            // Complement all digits
            for (std::size_t i = 0; i < tc.digits.size(); ++i) {
                tc.digits[i] = ~tc.digits[i];
            }
            // Remove trailing zeros (which were trailing 0xFFFFFFFF before
            // complement)
            while (!tc.digits.empty() && tc.digits.back() == 0xFFFFFFFFU) {
                tc.digits.pop_back();
            }
            tc.infinite_ones = true;
        }
        return tc;
    }

    static BigInt from_twos_complement(
        const std::vector<uint32_t>& digits, bool negative) {
        BigInt result;
        if (!negative) {
            result.limbs_ = digits;
            result.negative_ = false;
        } else {
            // Convert back: complement, then add 1
            result.limbs_ = digits;
            for (std::size_t i = 0; i < result.limbs_.size(); ++i) {
                result.limbs_[i] = ~result.limbs_[i];
            }
            // Remove trailing 0xFFFFFFFF
            while (!result.limbs_.empty() &&
                   result.limbs_.back() == 0xFFFFFFFFU) {
                result.limbs_.pop_back();
            }
            // This was (magnitude - 1) complemented. So magnitude = complement + 1
            // Actually: we have ~(digits). The original number is -(~digits + 1)
            // So magnitude = ~digits + 1
            // We already complemented, now add 1
            result.add_one_to_magnitude();
            result.negative_ = true;
        }
        result.normalize();
        return result;
    }

    static BigInt bitwise_op(const BigInt& a, const BigInt& b, BitOp op) {
        TwosComplement ta = to_twos_complement(a);
        TwosComplement tb = to_twos_complement(b);

        bool result_negative = false;
        switch (op) {
            case BitOp::AND:
                result_negative =
                    ta.infinite_ones && tb.infinite_ones;
                break;
            case BitOp::OR:
                result_negative =
                    ta.infinite_ones || tb.infinite_ones;
                break;
            case BitOp::XOR:
                result_negative =
                    ta.infinite_ones != tb.infinite_ones;
                break;
        }

        std::size_t max_size =
            std::max(ta.digits.size(), tb.digits.size());
        // If result is negative, we might need one extra limb
        if (result_negative) {
            max_size = std::max(max_size, std::size_t(1));
        }

        std::vector<uint32_t> result_digits(max_size);

        for (std::size_t i = 0; i < max_size; ++i) {
            uint32_t da = (i < ta.digits.size()) ? ta.digits[i] :
                          (ta.infinite_ones ? 0xFFFFFFFFU : 0U);
            uint32_t db = (i < tb.digits.size()) ? tb.digits[i] :
                          (tb.infinite_ones ? 0xFFFFFFFFU : 0U);

            switch (op) {
                case BitOp::AND: result_digits[i] = da & db; break;
                case BitOp::OR:  result_digits[i] = da | db; break;
                case BitOp::XOR: result_digits[i] = da ^ db; break;
            }
        }

        return from_twos_complement(result_digits, result_negative);
    }

    // ---------------------------------------------------------------
    // bit_length: number of bits needed to represent abs value
    // ---------------------------------------------------------------
    std::size_t bit_length() const {
        if (is_zero()) return 0;
        std::size_t bits = (limbs_.size() - 1) * 32;
        uint32_t top = limbs_.back();
        while (top > 0) {
            ++bits;
            top >>= 1;
        }
        return bits;
    }

    // Make uniform_random a friend
    template<typename RNG>
    friend BigInt uniform_random(const BigInt& upper, RNG& rng);
};

// ---------------------------------------------------------------
// Free functions
// ---------------------------------------------------------------
inline void swap(BigInt& a, BigInt& b) noexcept { a.swap(b); }
// abs() is defined as a friend inside the class body (found via ADL)

// ---------------------------------------------------------------
// uniform_random: generate uniform random BigInt in [0, upper)
// ---------------------------------------------------------------
template<typename RNG>
BigInt uniform_random(const BigInt& upper, RNG& rng) {
    if (upper.sign() <= 0) {
        throw std::invalid_argument(
            "uniform_random: upper bound must be positive");
    }

    std::size_t bits = upper.bit_length();
    std::size_t num_limbs = (bits + 31) / 32;
    std::size_t top_bits = bits % 32;
    uint32_t top_mask = (top_bits == 0) ? 0xFFFFFFFFU :
                        ((1U << top_bits) - 1U);

    // Use a uniform distribution for each 32-bit limb
    std::uniform_int_distribution<uint32_t> dist(
        0, std::numeric_limits<uint32_t>::max());

    BigInt result;
    result.limbs_.resize(num_limbs);

    // Rejection sampling
    while (true) {
        for (std::size_t i = 0; i < num_limbs; ++i) {
            result.limbs_[i] = dist(rng);
        }
        // Mask the top limb
        if (!result.limbs_.empty()) {
            result.limbs_.back() &= top_mask;
        }
        result.normalize();

        if (result < upper) {
            return result;
        }
    }
}

} // namespace bigint

#endif // BIGINT_BIGINT_HPP
