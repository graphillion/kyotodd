CtoI Operations
===============

CtoI (Combination-to-Integer) represents an integer-valued function
that maps combinations (subsets of items) to integer values. Internally
it uses a single ZBDD where user variables represent items and system
variables (1..BDDV_SysVarTop) encode binary digit positions.

Header: ``#include "ctoi.h"``

Requires ``BDDV_Init()`` to have been called.

.. note:: No Python binding is provided for this class.

CtoI Class
----------

.. cpp:class:: CtoI

   A mapping from combinations to integers, stored as a single ZBDD.

   **Constructors**

   .. cpp:function:: CtoI()

      Default constructor. Empty CtoI (no combinations assigned).

   .. cpp:function:: CtoI(const CtoI& a)

      Copy constructor.

   .. cpp:function:: CtoI(const ZDD& f)

      Construct from ZBDD directly.

   .. cpp:function:: CtoI(int n)

      Construct from int value. Creates a constant function:
      empty combination maps to value *n*.

   **Arithmetic Operators**

   .. cpp:function:: friend CtoI operator+(const CtoI& a, const CtoI& b)
                     friend CtoI operator-(const CtoI& a, const CtoI& b)
                     friend CtoI operator*(const CtoI& a, const CtoI& b)
                     friend CtoI operator/(const CtoI& a, const CtoI& b)
                     friend CtoI operator%(const CtoI& a, const CtoI& b)

      Arithmetic on integer-valued combination functions.

   .. cpp:function:: CtoI& operator+=(const CtoI& a)
                     CtoI& operator-=(const CtoI& a)
                     CtoI& operator*=(const CtoI& a)
                     CtoI& operator/=(const CtoI& a)
                     CtoI& operator%=(const CtoI& a)

      Compound assignment operators.

   .. cpp:function:: CtoI operator-() const

      Arithmetic negation.

   **Comparison**

   .. cpp:function:: friend int operator==(const CtoI& a, const CtoI& b)

      Equality.

   .. cpp:function:: friend int operator!=(const CtoI& a, const CtoI& b)

      Inequality.

   **Access / Predicates**

   .. cpp:function:: int Top() const

      Top variable ID of internal ZBDD.

   .. cpp:function:: int TopItem() const

      Top user variable (item) ID. 0 if no items.

   .. cpp:function:: int TopDigit() const

      Top digit index. 0 if boolean.

   .. cpp:function:: int IsBool() const

      True if boolean (items only, values 0 or 1).

   .. cpp:function:: int IsConst() const

      True if constant (no item dependency).

   **Factor Decomposition**

   .. cpp:function:: CtoI Factor0(int v) const

      0-factor by variable *v* (sets NOT containing *v*).

   .. cpp:function:: CtoI Factor1(int v) const

      1-factor by variable *v* (sets containing *v*, *v* removed).

   .. cpp:function:: CtoI AffixVar(int v) const

      Merge Factor0 and Factor1, then add *v* to all combinations.

   **Filter Operations**

   .. cpp:function:: CtoI FilterThen(const CtoI& a) const

      Keep combinations where condition *a* is non-zero.

   .. cpp:function:: CtoI FilterElse(const CtoI& a) const

      Keep combinations where condition *a* is zero.

   .. cpp:function:: CtoI FilterRestrict(const CtoI& a) const

      Restrict filter.

   .. cpp:function:: CtoI FilterPermit(const CtoI& a) const

      Permit filter.

   .. cpp:function:: CtoI FilterPermitSym(int n) const

      Symmetric permit filter.

   **Set Operations**

   .. cpp:function:: CtoI NonZero() const

      Boolean CtoI of combinations with non-zero value.

   .. cpp:function:: CtoI Support() const

      Support (set of variables appearing).

   .. cpp:function:: CtoI ConstTerm() const

      Constant term (value for empty combination only).

   **Digit Operations**

   .. cpp:function:: CtoI Digit(int index) const

      Extract boolean CtoI for given digit index.

   .. cpp:function:: CtoI TimesSysVar(int v) const

      Multiply by weight of system variable *v*.

   .. cpp:function:: CtoI DivBySysVar(int v) const

      Divide by weight of system variable *v*.

   .. cpp:function:: CtoI ShiftDigit(int pow) const

      Shift digits: multiply value by 2^pow.

   **Constant Comparisons**

   .. cpp:function:: CtoI EQ_Const(const CtoI& a) const
                     CtoI NE_Const(const CtoI& a) const
                     CtoI GT_Const(const CtoI& a) const
                     CtoI GE_Const(const CtoI& a) const
                     CtoI LT_Const(const CtoI& a) const
                     CtoI LE_Const(const CtoI& a) const

      Return boolean CtoI of combinations where this value satisfies
      the comparison against constant *a*.

   **Max / Min**

   .. cpp:function:: CtoI MaxVal() const

      Maximum value across all combinations.

   .. cpp:function:: CtoI MinVal() const

      Minimum value across all combinations.

   **Aggregation**

   .. cpp:function:: CtoI CountTerms() const

      Number of combinations (count of non-zero entries).

   .. cpp:function:: CtoI TotalVal() const

      Sum of values across all combinations.

   .. cpp:function:: CtoI TotalValItems() const

      Sum of (value x item count) across all combinations.

   **Absolute Value / Sign**

   .. cpp:function:: CtoI Abs() const

      Absolute value for each combination.

   .. cpp:function:: CtoI Sign() const

      Sign function: +1, -1, or 0 for each combination.

   **Conversion / Output**

   .. cpp:function:: ZDD GetZBDD() const

      Return the internal ZBDD.

   .. cpp:function:: uint64_t Size() const

      BDD node count.

   .. cpp:function:: int GetInt() const

      Convert constant CtoI to int.

   .. cpp:function:: int StrNum10(char* s) const

      Convert to decimal string. Returns 0 on success, 1 on error.

   .. cpp:function:: int StrNum16(char* s) const

      Convert to hex string. Returns 0 on success, 1 on error.

   .. cpp:function:: int PutForm() const

      Print in polynomial form. Returns 0 on success, 1 on error.

   .. cpp:function:: void Print() const

      Print internal ZBDD.

   **Frequent Pattern Mining**

   .. cpp:function:: CtoI ReduceItems(const CtoI& b) const

      Reduce items by filter *b*.

   .. cpp:function:: CtoI FreqPatA(int Val) const

      Frequent patterns (anti-monotone, all occurrences).

   .. cpp:function:: CtoI FreqPatA2(int Val) const

      Frequent patterns (improved version with pruning).

   .. cpp:function:: CtoI FreqPatAV(int Val) const

      Frequent patterns with values (anti-monotone).

   .. cpp:function:: CtoI FreqPatM(int Val) const

      Frequent patterns (maximal, monotone).

   .. cpp:function:: CtoI FreqPatC(int Val) const

      Frequent patterns (closed).

Free Functions
--------------

.. cpp:function:: CtoI CtoI_Intsec(const CtoI& a, const CtoI& b)

   Element-wise intersection.

.. cpp:function:: CtoI CtoI_Union(const CtoI& a, const CtoI& b)

   Element-wise union.

.. cpp:function:: CtoI CtoI_Diff(const CtoI& a, const CtoI& b)

   Element-wise difference.

.. cpp:function:: CtoI CtoI_GT(const CtoI& a, const CtoI& b)
                  CtoI CtoI_GE(const CtoI& a, const CtoI& b)
                  CtoI CtoI_LT(const CtoI& a, const CtoI& b)
                  CtoI CtoI_LE(const CtoI& a, const CtoI& b)
                  CtoI CtoI_NE(const CtoI& a, const CtoI& b)
                  CtoI CtoI_EQ(const CtoI& a, const CtoI& b)

   Element-wise comparison. Returns boolean CtoI.

.. cpp:function:: CtoI CtoI_ITE(const CtoI& a, const CtoI& b, const CtoI& c)

   If-then-else: ``a != 0 ? b : c``.

.. cpp:function:: CtoI CtoI_Max(const CtoI& a, const CtoI& b)

   Element-wise maximum.

.. cpp:function:: CtoI CtoI_Min(const CtoI& a, const CtoI& b)

   Element-wise minimum.

.. cpp:function:: CtoI CtoI_Null()

   Return a null (error) CtoI.

.. cpp:function:: CtoI CtoI_Meet(const CtoI& a, const CtoI& b)

   Meet operation.

.. cpp:function:: CtoI CtoI_atoi(const char* s)

   Parse string to CtoI constant.
