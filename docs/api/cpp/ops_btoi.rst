BtoI Operations
===============

BtoI (Binary-to-Integer) represents an integer-valued function that
depends on BDD variables. Internally it uses a BDDV in two's complement
representation. Each bit can be a constant BDD or a variable-dependent
function, allowing BtoI to represent functions whose integer value
varies depending on BDD variable assignments.

Header: ``#include "btoi.h"``

Requires ``BDDV_Init()`` to have been called.

.. note:: No Python binding is provided for this class.

BtoI Class
----------

.. cpp:class:: BtoI

   An integer-valued function on BDD variables, stored as a BDDV in
   two's complement.

   **Constructors**

   .. cpp:function:: BtoI()

      Default constructor. Represents integer 0.

   .. cpp:function:: BtoI(const BtoI& fv)

      Copy constructor.

   .. cpp:function:: BtoI(const BDDV& fv)

      Construct from BDDV. The caller must ensure valid two's complement
      encoding.

   .. cpp:function:: BtoI(const BDD& f)

      Construct from a single BDD. ``BDD(0)`` gives integer 0;
      ``BDD(1)`` or a variable-dependent BDD gives a 0-or-1 integer;
      ``BDD(-1)`` gives overflow.

   .. cpp:function:: BtoI(int n)

      Construct from a C++ int value.

   **Arithmetic Operators**

   .. cpp:function:: friend BtoI operator+(const BtoI& a, const BtoI& b)
                     friend BtoI operator-(const BtoI& a, const BtoI& b)
                     friend BtoI operator*(const BtoI& a, const BtoI& b)
                     friend BtoI operator/(const BtoI& a, const BtoI& b)
                     friend BtoI operator%(const BtoI& a, const BtoI& b)

      Arithmetic: addition, subtraction, multiplication, division,
      modulo.

   .. cpp:function:: friend BtoI operator&(const BtoI& a, const BtoI& b)
                     friend BtoI operator|(const BtoI& a, const BtoI& b)
                     friend BtoI operator^(const BtoI& a, const BtoI& b)

      Bitwise AND, OR, XOR.

   .. cpp:function:: BtoI& operator+=(const BtoI& fv)
                     BtoI& operator-=(const BtoI& fv)
                     BtoI& operator*=(const BtoI& fv)
                     BtoI& operator/=(const BtoI& fv)
                     BtoI& operator%=(const BtoI& fv)
                     BtoI& operator&=(const BtoI& fv)
                     BtoI& operator|=(const BtoI& fv)
                     BtoI& operator^=(const BtoI& fv)
                     BtoI& operator<<=(const BtoI& fv)
                     BtoI& operator>>=(const BtoI& fv)

      Compound assignment operators.

   **Unary Operators**

   .. cpp:function:: BtoI operator-() const

      Arithmetic negation.

   .. cpp:function:: BtoI operator~() const

      Bitwise NOT.

   .. cpp:function:: BtoI operator!() const

      Logical NOT (1 if zero, 0 otherwise).

   **Shift Operators**

   .. cpp:function:: BtoI operator<<(const BtoI& fv) const
                     BtoI operator>>(const BtoI& fv) const

      Left shift / arithmetic right shift.

   **Comparison**

   .. cpp:function:: friend int operator==(const BtoI& a, const BtoI& b)

      Equality.

   .. cpp:function:: friend int operator!=(const BtoI& a, const BtoI& b)

      Inequality.

   **Bounds**

   .. cpp:function:: BtoI UpperBound() const
                     BtoI UpperBound(const BDD& f) const

      Upper bound of the integer value (over all variable assignments,
      or restricted to assignments satisfying *f*).

   .. cpp:function:: BtoI LowerBound() const
                     BtoI LowerBound(const BDD& f) const

      Lower bound of the integer value.

   **Cofactor / Expansion**

   .. cpp:function:: BtoI At0(int v) const

      Restrict variable *v* to 0.

   .. cpp:function:: BtoI At1(int v) const

      Restrict variable *v* to 1.

   .. cpp:function:: BtoI Cofact(const BtoI& fv) const

      Generalized cofactor.

   .. cpp:function:: BtoI Spread(int k) const

      Apply Spread operation.

   **Access**

   .. cpp:function:: int Top() const

      Top (highest-level) user variable across all bits.

   .. cpp:function:: BDD GetSignBDD() const

      Return the sign bit (MSB) as a BDD.

   .. cpp:function:: BDD GetBDD(int i) const

      Return the BDD at bit position *i* (0 = LSB).

   .. cpp:function:: BDDV GetMetaBDDV() const

      Return the internal BDDV.

   .. cpp:function:: int Len() const

      Number of bits in the two's complement representation.

   **Conversion**

   .. cpp:function:: int GetInt() const

      Convert a constant BtoI to int.

   .. cpp:function:: int StrNum10(char* s) const

      Convert to decimal string. Returns 0 on success, 1 on error.

   .. cpp:function:: int StrNum16(char* s) const

      Convert to hex string. Returns 0 on success, 1 on error.

   **Other**

   .. cpp:function:: uint64_t Size() const

      BDD node count.

   .. cpp:function:: void Print() const

      Print internal representation to stdout.

Free Functions
--------------

.. cpp:function:: BtoI BtoI_ITE(const BDD& f, const BtoI& a, const BtoI& b)

   If-then-else with BDD condition: ``f ? a : b``.

.. cpp:function:: BtoI BtoI_ITE(const BtoI& a, const BtoI& b, const BtoI& c)

   If-then-else with BtoI condition: ``a != 0 ? b : c``.

.. cpp:function:: BtoI BtoI_EQ(const BtoI& a, const BtoI& b)

   Returns 1 where ``a == b``, 0 elsewhere.

.. cpp:function:: BtoI BtoI_GT(const BtoI& a, const BtoI& b)

   Returns 1 where ``a > b``.

.. cpp:function:: BtoI BtoI_LT(const BtoI& a, const BtoI& b)

   Returns 1 where ``a < b``.

.. cpp:function:: BtoI BtoI_GE(const BtoI& a, const BtoI& b)

   Returns 1 where ``a >= b``.

.. cpp:function:: BtoI BtoI_LE(const BtoI& a, const BtoI& b)

   Returns 1 where ``a <= b``.

.. cpp:function:: BtoI BtoI_NE(const BtoI& a, const BtoI& b)

   Returns 1 where ``a != b``.

.. cpp:function:: BtoI BtoI_atoi(const char* s)

   Parse string to BtoI. Supports decimal, hex (``0x...``), and binary
   (``0b...``) formats.
