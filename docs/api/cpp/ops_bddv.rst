BDDV Operations
===============

.. deprecated::
   BDDV is obsolete and retained only for backward compatibility.
   For new code, use ``std::vector<BDD>`` and
   ``BDD::raw_size(const std::vector<BDD>&)``.

BDDV encodes an array of BDDs into a single meta-BDD using
output-selection system variables (1..BDDV_SysVarTop). When all
elements are identical, the internal representation is compact
regardless of array length.

Header: ``#include "bddv.h"``

.. note:: No Python binding is provided for this class.

Initialization
--------------

.. cpp:function:: int BDDV_Init(uint64_t init = 256, uint64_t limit = BDD_MaxNode)

   Initialize the BDDV subsystem. Calls ``bddinit()`` and allocates
   ``BDDV_SysVarTop`` (20) output-selection variables. Must be called
   before using any BDDV, ZBDDV, BtoI, CtoI, SOP, SOPV, or MLZBDDV
   functionality.

   :param init: Initial node table capacity.
   :param limit: Maximum node table size.
   :return: 0 on success, 1 on failure.

.. cpp:function:: int BDDV_UserTopLev()

   Return the number of user variables (excluding system variables).

.. cpp:function:: int BDDV_NewVar()

   Create a new user input variable.

   :return: The variable number (VarID) of the new variable.

.. cpp:function:: int BDDV_NewVarOfLev(int lev)

   Create a new user input variable at a specific level.

   :param lev: The level at which to insert the variable.
   :return: The variable number (VarID) of the new variable.

Constants
---------

.. cpp:var:: const int BDDV_SysVarTop = 20

   Number of output-selection (system) variables.

.. cpp:var:: const int BDDV_MaxLen = (1 << BDDV_SysVarTop)

   Maximum array length (2^20 = 1,048,576).

.. cpp:var:: const int BDDV_MaxLenImport = 1000

   Maximum number of outputs when importing.

.. cpp:var:: extern int BDDV_Active

   Nonzero if BDDV has been initialized via ``BDDV_Init()``.

BDDV Class
----------

.. cpp:class:: BDDV

   An array of BDDs encoded into a single meta-BDD.

   **Constructors**

   .. cpp:function:: BDDV()

      Default constructor. Creates an empty (length 0) BDDV.

   .. cpp:function:: BDDV(const BDDV& fv)

      Copy constructor.

   .. cpp:function:: BDDV(const BDD& f)

      Construct a length-1 BDDV from a single BDD.

      :param f: A BDD (must not contain system variables).

   .. cpp:function:: BDDV(const BDD& f, int len)

      Construct a BDDV where all *len* elements are the same BDD *f*.

      :param f: A BDD (must not contain system variables).
      :param len: Array length (0..BDDV_MaxLen).

   **Logic Operators**

   .. cpp:function:: BDDV operator~() const

      Element-wise NOT.

   .. cpp:function:: friend BDDV operator&(const BDDV& fv, const BDDV& gv)

      Element-wise AND. Requires same length.

   .. cpp:function:: friend BDDV operator|(const BDDV& fv, const BDDV& gv)

      Element-wise OR. Requires same length.

   .. cpp:function:: friend BDDV operator^(const BDDV& fv, const BDDV& gv)

      Element-wise XOR. Requires same length.

   .. cpp:function:: friend BDDV operator||(const BDDV& fv, const BDDV& gv)

      Concatenation. The result has length ``fv.Len() + gv.Len()``.

   .. cpp:function:: BDDV& operator&=(const BDDV& fv)
                     BDDV& operator|=(const BDDV& fv)
                     BDDV& operator^=(const BDDV& fv)

      In-place element-wise logic operators.

   **Comparison**

   .. cpp:function:: friend int operator==(const BDDV& fv, const BDDV& gv)

      Equality (same elements and same length).

   .. cpp:function:: friend int operator!=(const BDDV& fv, const BDDV& gv)

      Inequality.

   **Shift Operators**

   .. cpp:function:: BDDV operator<<(int s) const
                     BDDV operator>>(int s) const

      Shift all user variable levels up/down by *s*.

   .. cpp:function:: BDDV& operator<<=(int s)
                     BDDV& operator>>=(int s)

      In-place shift.

   **Cofactor / Expansion**

   .. cpp:function:: BDDV At0(int v) const

      Cofactor: restrict variable *v* to 0 in all elements.

   .. cpp:function:: BDDV At1(int v) const

      Cofactor: restrict variable *v* to 1 in all elements.

   .. cpp:function:: BDDV Cofact(const BDDV& fv) const

      Generalized cofactor of each element with respect to *fv*.

   .. cpp:function:: BDDV Swap(int v1, int v2) const

      Swap variables *v1* and *v2* in all elements.

   .. cpp:function:: BDDV Spread(int k) const

      Apply Spread operation to all elements.

   **Array Access**

   .. cpp:function:: BDD GetBDD(int index) const

      Return the BDD at the given 0-based index.

   .. cpp:function:: BDD GetMetaBDD() const

      Return the internal meta-BDD (includes system variables).

   .. cpp:function:: int Len() const

      Return the array length.

   .. cpp:function:: int Uniform() const

      Return 1 if all elements are identical, 0 otherwise.

   .. cpp:function:: int Top() const

      Return the top (highest-level) user variable across all elements.

   .. cpp:function:: BDDV Former() const

      Return the first half of the array.

   .. cpp:function:: BDDV Latter() const

      Return the second half of the array.

   .. cpp:function:: BDDV Part(int start, int len) const

      Return a sub-array starting at *start* with length *len*.

   **Output / Export**

   .. cpp:function:: uint64_t Size() const

      Total shared node count across all elements.

   .. cpp:function:: void Print() const

      Print internal info for each element to stdout.

   .. cpp:function:: void Export(FILE* strm = stdout) const

      Export the BDDV structure to a file.

Free Functions
--------------

.. cpp:function:: int BDDV_Imply(const BDDV& fv, const BDDV& gv)

   Check implication: all elements of ``~fv | gv`` are tautologies.

   :return: 1 if *fv* implies *gv* element-wise, 0 otherwise.

.. cpp:function:: BDDV BDDV_Mask1(int index, int len)

   Create a BDDV where only element *index* is true, others false.

.. cpp:function:: BDDV BDDV_Mask2(int index, int len)

   Create a BDDV where elements 0..index-1 are false, index..len-1 are true.

.. cpp:function:: BDDV BDDV_Import(FILE* strm = stdin)

   Import a BDDV from a file.

.. cpp:function:: BDDV BDDV_ImportPla(FILE* strm = stdin, int sopf = 0)

   Import a BDDV from an ESPRESSO PLA file.

   :param sopf: If nonzero, use even-numbered levels for SOP compatibility.
