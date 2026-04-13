#ifndef KYOTODD_SEQBDD_H
#define KYOTODD_SEQBDD_H

#include "bdd.h"

struct SvgParams;

/** @brief Cache operation code for SeqBDD concatenation. */
static const uint8_t BDD_OP_SEQBDD_CONCAT = 66;
/** @brief Cache operation code for SeqBDD left quotient. */
static const uint8_t BDD_OP_SEQBDD_LQUOT  = 67;

/**
 * @brief A Sequence BDD representing sets of sequences (strings).
 *
 * SeqBDD uses composition, wrapping a ZDD internally. Each variable
 * represents a symbol, and paths from root to terminal encode ordered
 * sequences where the same symbol may appear multiple times.
 *
 * Does NOT inherit from DDBase.
 */
class SeqBDD {
    ZDD zdd_;

    friend SeqBDD operator&(const SeqBDD& f, const SeqBDD& g);
    friend SeqBDD operator+(const SeqBDD& f, const SeqBDD& g);
    friend SeqBDD operator-(const SeqBDD& f, const SeqBDD& g);
    friend SeqBDD operator*(const SeqBDD& f, const SeqBDD& g);
    friend SeqBDD operator/(const SeqBDD& f, const SeqBDD& p);
    friend bool operator==(const SeqBDD& f, const SeqBDD& g);

public:
    /** @brief Default constructor. Constructs an empty SeqBDD (no sequences). */
    SeqBDD() : zdd_(0) {}
    /**
     * @brief Construct from an integer value.
     * @param val 0 for empty set, 1 for {epsilon} (single empty sequence), negative for null.
     */
    explicit SeqBDD(int val) : zdd_(val) {}
    /** @brief Copy constructor. */
    SeqBDD(const SeqBDD& f) : zdd_(f.zdd_) {}
    /** @brief Move constructor. */
    SeqBDD(SeqBDD&& f) : zdd_(std::move(f.zdd_)) {}
    /**
     * @brief Construct from an existing ZDD.
     * @param zbdd A ZDD to interpret as a sequence set.
     */
    explicit SeqBDD(const ZDD& zbdd) : zdd_(zbdd) {}
    /** @brief Destructor. */
    ~SeqBDD() {}

    /** @brief Copy assignment operator. */
    SeqBDD& operator=(const SeqBDD& f) { zdd_ = f.zdd_; return *this; }
    /** @brief Move assignment operator. */
    SeqBDD& operator=(SeqBDD&& f) { zdd_ = std::move(f.zdd_); return *this; }

    /** @brief In-place intersection. */
    SeqBDD& operator&=(const SeqBDD& f) { *this = *this & f; return *this; }
    /** @brief In-place union. */
    SeqBDD& operator+=(const SeqBDD& f) { *this = *this + f; return *this; }
    /** @brief In-place difference. */
    SeqBDD& operator-=(const SeqBDD& f) { *this = *this - f; return *this; }
    /** @brief In-place concatenation. */
    SeqBDD& operator*=(const SeqBDD& f) { *this = *this * f; return *this; }
    /** @brief In-place left quotient. */
    SeqBDD& operator/=(const SeqBDD& f) { *this = *this / f; return *this; }
    /** @brief In-place left remainder. */
    SeqBDD& operator%=(const SeqBDD& f);

    /**
     * @brief Get the top variable number.
     * @return The variable number at the root of the internal ZDD.
     */
    int top() const { return static_cast<int>(zdd_.Top()); }
    /** @deprecated Use top() instead. */
    int Top() const { return top(); }
    /**
     * @brief Get the internal ZDD representation.
     * @return A copy of the internal ZDD.
     */
    ZDD get_zdd() const { return zdd_; }
    /** @deprecated Use get_zdd() instead. */
    ZDD GetZDD() const { return get_zdd(); }
    /**
     * @brief Get the number of nodes in the internal ZDD.
     * @return The node count.
     */
    uint64_t size() const { return zdd_.Size(); }
    /** @deprecated Use size() instead. */
    uint64_t Size() const { return size(); }
    /**
     * @brief Get the number of sequences in the set.
     * @return The cardinality (number of sequences).
     */
    uint64_t card() const { return zdd_.Card(); }
    /** @deprecated Use card() instead. */
    uint64_t Card() const { return card(); }
    /**
     * @brief Get the total symbol count across all sequences.
     * @return The sum of lengths of all sequences.
     */
    uint64_t lit() const { return zdd_.Lit(); }
    /** @deprecated Use lit() instead. */
    uint64_t Lit() const { return lit(); }
    /**
     * @brief Get the length of the longest sequence.
     * @return The maximum sequence length.
     */
    uint64_t len() const { return zdd_.Len(); }
    /** @deprecated Use len() instead. */
    uint64_t Len() const { return len(); }

    /**
     * @brief Remove sequences starting with variable v (offset).
     * @param v Variable number.
     * @return A SeqBDD without sequences starting with v.
     */
    SeqBDD offset(int v) const;
    /** @deprecated Use offset() instead. */
    SeqBDD OffSet(int v) const { return offset(v); }
    /**
     * @brief Extract suffixes of sequences starting with v (onset with strip).
     *
     * Selects sequences whose first symbol is v, then removes the leading v.
     *
     * @param v Variable number.
     * @return A SeqBDD of suffixes after stripping the leading v.
     */
    SeqBDD onset0(int v) const;
    /** @deprecated Use onset0() instead. */
    SeqBDD OnSet0(int v) const { return onset0(v); }
    /**
     * @brief Extract sequences starting with variable v (onset).
     * @param v Variable number.
     * @return A SeqBDD of sequences that start with v.
     */
    SeqBDD onset(int v) const;
    /** @deprecated Use onset() instead. */
    SeqBDD OnSet(int v) const { return onset(v); }
    /**
     * @brief Prepend variable v to all sequences.
     * @param v Variable number to prepend.
     * @return A SeqBDD with v prepended to every sequence.
     */
    SeqBDD push(int v) const;
    /** @deprecated Use push() instead. */
    SeqBDD Push(int v) const { return push(v); }

    /** @brief Print all sequences to stdout.
     * @note C++ only. Not available in the Python binding.
     *       Use print_seq() or seq_str() instead. */
    void print() const;
    /** @deprecated Use print() instead. */
    void Print() const { print(); }
    /**
     * @brief Export the internal ZDD in Sapporo format to a FILE stream.
     * @param strm FILE pointer (default: stdout).
     */
    void export_to(FILE* strm = stdout) const;
    /** @deprecated Use export_to() instead. */
    void Export(FILE* strm = stdout) const { export_to(strm); }
    /**
     * @brief Export the internal ZDD in Sapporo format to an output stream.
     * @param strm Output stream.
     */
    void export_to(std::ostream& strm) const;
    /** @deprecated Use export_to() instead. */
    void Export(std::ostream& strm) const { export_to(strm); }
    /** @brief Print all sequences in a human-readable format. */
    void print_seq() const;
    /** @deprecated Use print_seq() instead. */
    void PrintSeq() const { print_seq(); }
    /**
     * @brief Get all sequences as a string.
     * @return A string representation of all sequences.
     */
    std::string seq_str() const;

    /**
     * @brief Export the internal ZDD as SVG to a file.
     * @param filename Output file path.
     * @param params SVG rendering parameters.
     */
    void save_svg(const char* filename, const SvgParams& params) const;
    /** @overload */
    void save_svg(const char* filename) const;
    /**
     * @brief Export the internal ZDD as SVG to a stream.
     * @param strm Output stream.
     * @param params SVG rendering parameters.
     */
    void save_svg(std::ostream& strm, const SvgParams& params) const;
    /** @overload */
    void save_svg(std::ostream& strm) const;
    /**
     * @brief Export the internal ZDD as an SVG string.
     * @param params SVG rendering parameters.
     * @return An SVG format string.
     */
    std::string save_svg(const SvgParams& params) const;
    /** @overload */
    std::string save_svg() const;
};

/** @brief Intersection of two sequence sets. */
inline SeqBDD operator&(const SeqBDD& f, const SeqBDD& g) { return SeqBDD(f.zdd_ & g.zdd_); }
/** @brief Union of two sequence sets. */
inline SeqBDD operator+(const SeqBDD& f, const SeqBDD& g) { return SeqBDD(f.zdd_ + g.zdd_); }
/** @brief Difference of two sequence sets. */
inline SeqBDD operator-(const SeqBDD& f, const SeqBDD& g) { return SeqBDD(f.zdd_ - g.zdd_); }
/** @brief Equality comparison. */
inline bool operator==(const SeqBDD& f, const SeqBDD& g) { return f.zdd_ == g.zdd_; }
/** @brief Inequality comparison. */
inline bool operator!=(const SeqBDD& f, const SeqBDD& g) { return !(f == g); }

/** @brief Left remainder: f - (f / p) * p. */
inline SeqBDD operator%(const SeqBDD& f, const SeqBDD& p) { return f - (f / p) * p; }

inline SeqBDD& SeqBDD::operator%=(const SeqBDD& f) { *this = *this % f; return *this; }

/**
 * @brief Create a SeqBDD from a raw node ID.
 * @param p Raw bddp node ID.
 * @return A SeqBDD wrapping the node.
 */
inline SeqBDD SeqBDD_ID(bddp p) { return SeqBDD(ZDD_ID(p)); }

// --- SeqBDD save_svg inline implementations ---
#include "svg_export.h"

inline void SeqBDD::save_svg(const char* filename, const SvgParams& params) const {
    zdd_save_svg(filename, zdd_.id(), params);
}
inline void SeqBDD::save_svg(const char* filename) const {
    save_svg(filename, SvgParams());
}
inline void SeqBDD::save_svg(std::ostream& strm, const SvgParams& params) const {
    zdd_save_svg(strm, zdd_.id(), params);
}
inline void SeqBDD::save_svg(std::ostream& strm) const {
    save_svg(strm, SvgParams());
}
inline std::string SeqBDD::save_svg(const SvgParams& params) const {
    return zdd_save_svg(zdd_.id(), params);
}
inline std::string SeqBDD::save_svg() const {
    return save_svg(SvgParams());
}

#endif
