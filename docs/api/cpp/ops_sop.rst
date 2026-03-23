SOP / SOPV Operations
=====================

SOP (Sum-of-Products) represents a Boolean function in two-level form
using a ZBDD. Each logic variable uses a pair of ZBDD variables: an
even VarID for the positive literal and the preceding odd VarID for the
negative literal.

SOPV is a vector of SOPs stored in a single ZBDDV, useful for
multi-output logic functions.

Header: ``#include "sop.h"``

Requires ``BDDV_Init()`` to have been called before using SOPV.

.. note:: No Python binding is provided for these classes.

Variable Management
-------------------

.. cpp:function:: int SOP_NewVar()

   Create a new SOP logic variable. Internally calls ``BDD_NewVar()``
   twice to create a pair of ZBDD variables (odd VarID for negative
   literal, even VarID for positive literal).

   :return: The even VarID.

.. cpp:function:: int SOP_NewVarOfLev(int lev)

   Create a new SOP variable at the given level.

   :param lev: Must be even. Creates ZBDD variables at levels
      ``lev-1`` and ``lev``.
   :return: The even VarID.

SOP Class
---------

.. cpp:class:: SOP

   Sum-of-Products representation using a ZBDD.

   **Constructors**

   .. cpp:function:: SOP()

      Default constructor. Empty SOP (constant 0).

   .. cpp:function:: SOP(int val)

      Construct from integer. 0 = constant false (no cubes),
      1 = tautology (one empty cube).

   .. cpp:function:: SOP(const SOP& f)

      Copy constructor.

   .. cpp:function:: SOP(const ZDD& zbdd)

      Construct from a ZBDD directly. The caller must ensure that the
      ZBDD follows SOP variable encoding.

   **Set Operators**

   .. cpp:function:: friend SOP operator&(const SOP& f, const SOP& g)

      Intersection.

   .. cpp:function:: friend SOP operator+(const SOP& f, const SOP& g)

      Union.

   .. cpp:function:: friend SOP operator-(const SOP& f, const SOP& g)

      Difference.

   **Algebraic Operators**

   .. cpp:function:: friend SOP operator*(const SOP& f, const SOP& g)

      Algebraic product (AND of all cubes pairwise).

   .. cpp:function:: friend SOP operator/(const SOP& f, const SOP& g)

      Algebraic division.

   .. cpp:function:: friend SOP operator%(const SOP& f, const SOP& g)

      Algebraic remainder.

   .. cpp:function:: SOP& operator&=(const SOP& f)
                     SOP& operator+=(const SOP& f)
                     SOP& operator-=(const SOP& f)
                     SOP& operator*=(const SOP& f)
                     SOP& operator/=(const SOP& f)
                     SOP& operator%=(const SOP& f)

      Compound assignment operators.

   **Comparison**

   .. cpp:function:: friend int operator==(const SOP& f, const SOP& g)

      Equality.

   .. cpp:function:: friend int operator!=(const SOP& f, const SOP& g)

      Inequality.

   **Factor Operations**

   .. cpp:function:: SOP Factor1(int v) const

      Extract cubes containing positive literal x_v (with x_v removed).

      :param v: Even VarID of the logic variable.

   .. cpp:function:: SOP Factor0(int v) const

      Extract cubes containing negative literal ~x_v (with ~x_v
      removed).

      :param v: Even VarID of the logic variable.

   .. cpp:function:: SOP FactorD(int v) const

      Extract cubes not depending on variable *v*.

      :param v: Even VarID of the logic variable.

   **And Operations**

   .. cpp:function:: SOP And1(int v) const

      AND positive literal x_v to all cubes.

   .. cpp:function:: SOP And0(int v) const

      AND negative literal ~x_v to all cubes.

   **Shift Operators**

   .. cpp:function:: SOP operator<<(int n) const
                     SOP operator>>(int n) const

      Shift all variables up/down by *n* levels. *n* must be even.

   .. cpp:function:: SOP& operator<<=(int n)
                     SOP& operator>>=(int n)

      In-place shift.

   **Query**

   .. cpp:function:: int Top() const

      Top logic variable VarID (even).

   .. cpp:function:: uint64_t Size() const

      Node count of internal ZBDD.

   .. cpp:function:: uint64_t Cube() const

      Number of cubes (product terms).

   .. cpp:function:: uint64_t Lit() const

      Total number of literals across all cubes.

   .. cpp:function:: ZDD GetZBDD() const

      Return the internal ZBDD.

   .. cpp:function:: BDD GetBDD() const

      Convert SOP to BDD via Shannon expansion.

   **Predicates**

   .. cpp:function:: int IsPolyCube() const

      1 if multiple cubes, 0 otherwise.

   .. cpp:function:: int IsPolyLit() const

      0 if SOP is a single literal, 1 otherwise.

   **Advanced Operations**

   .. cpp:function:: SOP Support() const

      Set of all logic variables appearing in this SOP.

   .. cpp:function:: SOP Divisor() const

      Find a non-trivial algebraic divisor.

   .. cpp:function:: SOP Implicants(BDD f) const

      Select cubes that are implicants of BDD *f*.

   .. cpp:function:: SOP Swap(int v1, int v2) const

      Swap two logic variables.

      :param v1: Even VarID of first variable.
      :param v2: Even VarID of second variable.

   .. cpp:function:: SOP InvISOP() const

      Compute ISOP of the negation of this SOP's function.

   **Output**

   .. cpp:function:: void Print() const

      Print debug info to stdout.

   .. cpp:function:: void PrintPla() const

      Print in PLA format.

Free Functions (SOP)
--------------------

.. cpp:function:: SOP SOP_ISOP(BDD f)

   Generate ISOP (Irredundant Sum-of-Products) from a BDD.

.. cpp:function:: SOP SOP_ISOP(BDD on, BDD dc)

   Generate ISOP with don't-care.

   :param on: The onset BDD (must be covered).
   :param dc: The don't-care BDD (may or may not be covered).

SOPV Class
----------

.. cpp:class:: SOPV

   Vector of SOPs stored in a ZBDDV.

   **Constructors**

   .. cpp:function:: SOPV()

      Default constructor.

   .. cpp:function:: SOPV(const SOPV& v)

      Copy constructor.

   .. cpp:function:: SOPV(const ZBDDV& zbddv)

      Construct from a ZBDDV.

   .. cpp:function:: SOPV(const SOP& f, int loc = 0)

      Construct from a single SOP at given index.

   **Set Operators**

   .. cpp:function:: friend SOPV operator&(const SOPV& f, const SOPV& g)

      Element-wise intersection.

   .. cpp:function:: friend SOPV operator+(const SOPV& f, const SOPV& g)

      Element-wise union.

   .. cpp:function:: friend SOPV operator-(const SOPV& f, const SOPV& g)

      Element-wise difference.

   .. cpp:function:: SOPV& operator&=(const SOPV& v)
                     SOPV& operator+=(const SOPV& v)
                     SOPV& operator-=(const SOPV& v)

      Compound assignment operators.

   **Comparison**

   .. cpp:function:: friend int operator==(const SOPV& v1, const SOPV& v2)

      Equality.

   .. cpp:function:: friend int operator!=(const SOPV& v1, const SOPV& v2)

      Inequality.

   **Shift Operators**

   .. cpp:function:: SOPV operator<<(int n) const
                     SOPV operator>>(int n) const

      Shift all variables up/down by *n* logic variables (internally
      shifts ZBDDV by ``n*2`` levels).

   **Factor / And Operations**

   .. cpp:function:: SOPV Factor1(int v) const
                     SOPV Factor0(int v) const
                     SOPV FactorD(int v) const
                     SOPV And1(int v) const
                     SOPV And0(int v) const

      Element-wise factor and literal-AND operations.

   **Query**

   .. cpp:function:: int Top() const

      Top logic variable VarID (even).

   .. cpp:function:: uint64_t Size() const

      Node count of internal ZBDDV.

   .. cpp:function:: uint64_t Cube() const

      Total cube count across all elements.

   .. cpp:function:: uint64_t Lit() const

      Total literal count across all elements.

   .. cpp:function:: int Last() const

      Highest index with a non-empty element.

   .. cpp:function:: SOP GetSOP(int index) const

      Get the SOP at the given 0-based index.

   .. cpp:function:: ZBDDV GetZBDDV() const

      Return the internal ZBDDV.

   .. cpp:function:: SOPV Mask(int start, int length = 1) const

      Mask elements from *start* to ``start + length - 1``, zeroing all
      other indices. Original indices are preserved (not rebased to 0).

   .. cpp:function:: SOPV Swap(int v1, int v2) const

      Swap two logic variables in all elements.

   **Output**

   .. cpp:function:: void Print() const

      Print debug info for each element.

   .. cpp:function:: int PrintPla() const

      Print in PLA format. Returns 0 on success, 1 on error.

Free Functions (SOPV)
---------------------

.. cpp:function:: int SOPV_NewVar()

   Alias for ``SOP_NewVar()``.

.. cpp:function:: int SOPV_NewVarOfLev(int lev)

   Alias for ``SOP_NewVarOfLev()``.

.. cpp:function:: SOPV SOPV_ISOP(BDDV v)

   Generate ISOP for each element of a BDDV.

.. cpp:function:: SOPV SOPV_ISOP(BDDV on, BDDV dc)

   Generate ISOP for each element with don't-care.

.. cpp:function:: SOPV SOPV_ISOP2(BDDV v)

   Generate ISOP with output polarity optimization.

.. cpp:function:: SOPV SOPV_ISOP2(BDDV on, BDDV dc)

   Generate ISOP with output polarity optimization and don't-care.
