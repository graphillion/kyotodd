SeqBDD Operations
=================

SeqBDD (Sequence BDD) represents sets of sequences using a ZDD,
where each path from root to terminal encodes a sequence. Unlike
standard ZDDs, the same variable may appear multiple times in a path,
allowing sequences with repeated symbols.

Header: ``#include "seqbdd.h"``

SeqBDD Class
------------

.. cpp:class:: SeqBDD

   A set of sequences represented on a ZDD.

   **Constructors**

   .. cpp:function:: SeqBDD()

      Default constructor. Creates an empty set (no sequences).

   .. cpp:function:: explicit SeqBDD(int val)

      Construct from integer. 0 = empty set, 1 = {epsilon}, -1 = null.

   .. cpp:function:: SeqBDD(const SeqBDD& f)

      Copy constructor.

   .. cpp:function:: SeqBDD(SeqBDD&& f)

      Move constructor.

   .. cpp:function:: explicit SeqBDD(const ZDD& zbdd)

      Construct from a ZDD directly.

   **Set Operations (inline friends)**

   .. cpp:function:: friend SeqBDD operator&(const SeqBDD& f, const SeqBDD& g)

      Intersection of two sequence sets.

   .. cpp:function:: friend SeqBDD operator+(const SeqBDD& f, const SeqBDD& g)

      Union of two sequence sets.

   .. cpp:function:: friend SeqBDD operator-(const SeqBDD& f, const SeqBDD& g)

      Difference: sequences in *f* but not in *g*.

   **Concatenation**

   .. cpp:function:: friend SeqBDD operator*(const SeqBDD& f, const SeqBDD& g)

      Concatenation of two sequence sets. The result contains every
      sequence formed by appending a sequence from *g* to a sequence
      from *f*.

      Uses operation cache (code 66) for memoization.

   **Left Quotient and Remainder**

   .. cpp:function:: friend SeqBDD operator/(const SeqBDD& f, const SeqBDD& p)

      Left quotient. For every prefix *s* in *p*, strips *s* from the
      front of matching sequences in *f*. When *p* contains multiple
      sequences, the result is the intersection of individual quotients.

      Throws ``std::invalid_argument`` if *p* is the empty set.

      Uses operation cache (code 67) for memoization.

   .. cpp:function:: friend SeqBDD operator%(const SeqBDD& f, const SeqBDD& p)

      Left remainder: ``f - p * (f / p)``.

   **Compound Assignment**

   .. cpp:function:: SeqBDD& operator&=(const SeqBDD& f)
                     SeqBDD& operator+=(const SeqBDD& f)
                     SeqBDD& operator-=(const SeqBDD& f)
                     SeqBDD& operator*=(const SeqBDD& f)
                     SeqBDD& operator/=(const SeqBDD& f)
                     SeqBDD& operator%=(const SeqBDD& f)

      In-place variants of the above operators.

   **Comparison**

   .. cpp:function:: friend bool operator==(const SeqBDD& f, const SeqBDD& g)

      Equality comparison.

   .. cpp:function:: friend bool operator!=(const SeqBDD& f, const SeqBDD& g)

      Inequality comparison.

   **Node Operations**

   .. cpp:function:: SeqBDD off_set(int v) const

      Remove all sequences whose first element is *v*.

      :param v: Variable number.
      :return: A SeqBDD without sequences starting with *v*.

   .. cpp:function:: SeqBDD on_set0(int v) const

      Extract sequences starting with *v* and strip the leading *v*.

      Walks from the root past variables with higher level than *v*,
      then extracts the onset.

      :param v: Variable number.
      :return: A SeqBDD of suffixes after removing the leading *v*.

   .. cpp:function:: SeqBDD on_set(int v) const

      Extract sequences starting with *v*, keeping *v*.
      Equivalent to ``on_set0(v).push(v)``.

      :param v: Variable number.
      :return: A SeqBDD of sequences that start with *v*.

   .. cpp:function:: SeqBDD push(int v) const

      Prepend variable *v* to every sequence in the set.

      :param v: Variable number to prepend.
      :return: A SeqBDD with *v* added to the front of every sequence.

   **Query Methods**

   .. cpp:function:: int top() const

      The variable number of the root node. Returns 0 for terminals.

   .. cpp:function:: ZDD get_zdd() const

      Return the internal ZDD.

   .. cpp:function:: uint64_t size() const

      The number of ZDD nodes.

   .. cpp:function:: uint64_t card() const

      The number of sequences in the set.

   .. cpp:function:: uint64_t lit() const

      Total symbol count across all sequences.

   .. cpp:function:: uint64_t len() const

      Length of the longest sequence.

   **Output**

   .. cpp:function:: void print() const

      Print the internal ZDD structure to stdout.

   .. cpp:function:: void export_to(FILE* strm = stdout) const

      Export the internal ZDD to a file stream.

   .. cpp:function:: void export_to(std::ostream& strm) const

      Export the internal ZDD to a C++ output stream.

   .. cpp:function:: void print_seq() const

      Print all sequences in human-readable form to stdout.
      Sequences are separated by ``" + "``. The empty sequence is
      shown as ``e``. Each symbol is displayed as its level value.

   .. cpp:function:: std::string seq_str() const

      Return the same output as ``print_seq()`` as a string.

Friend Functions
----------------

.. cpp:function:: SeqBDD SeqBDD_ID(bddp p)

   Create a SeqBDD from a raw node ID.
