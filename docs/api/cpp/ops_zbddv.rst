ZBDDV Operations
================

.. deprecated::
   ZBDDV is obsolete and retained only for backward compatibility.
   For new code, use ``std::vector<ZDD>`` and
   ``ZDD::raw_size(const std::vector<ZDD>&)``.

ZBDDV encodes a sparse array of ZBDDs into a single meta-ZBDD using
system variables (1..BDDV_SysVarTop) for array index encoding. Elements
that are empty (ZDD 0) contribute nothing to the meta-ZBDD, so sparse
arrays are efficient. Unlike BDDV, array length is implicit — determined
by the highest non-empty index.

Header: ``#include "zbddv.h"``

Requires ``BDDV_Init()`` to have been called.

.. note:: No Python binding is provided for this class.

ZBDDV Class
------------

.. cpp:class:: ZBDDV

   A sparse array of ZBDDs encoded into a single meta-ZBDD.

   **Constructors**

   .. cpp:function:: ZBDDV()

      Default constructor. All elements are empty (ZDD 0).

   .. cpp:function:: ZBDDV(const ZBDDV& fv)

      Copy constructor.

   .. cpp:function:: ZBDDV(const ZDD& f, int location = 0)

      Construct a ZBDDV with element *f* at *location*. All other
      elements are empty.

      :param f: A ZBDD (must not contain system variables).
      :param location: Array index (0-based, must be < BDDV_MaxLen).

   **Set Operators**

   .. cpp:function:: friend ZBDDV operator&(const ZBDDV& fv, const ZBDDV& gv)

      Element-wise intersection.

   .. cpp:function:: friend ZBDDV operator+(const ZBDDV& fv, const ZBDDV& gv)

      Element-wise union.

   .. cpp:function:: friend ZBDDV operator-(const ZBDDV& fv, const ZBDDV& gv)

      Element-wise subtract.

   .. cpp:function:: ZBDDV& operator&=(const ZBDDV& fv)
                     ZBDDV& operator+=(const ZBDDV& fv)
                     ZBDDV& operator-=(const ZBDDV& fv)

      In-place set operators.

   **Comparison**

   .. cpp:function:: friend int operator==(const ZBDDV& fv, const ZBDDV& gv)

      Equality.

   .. cpp:function:: friend int operator!=(const ZBDDV& fv, const ZBDDV& gv)

      Inequality.

   **Shift Operators**

   .. cpp:function:: ZBDDV operator<<(int s) const
                     ZBDDV operator>>(int s) const

      Shift all item variable levels up/down by *s*.

   .. cpp:function:: ZBDDV& operator<<=(int s)
                     ZBDDV& operator>>=(int s)

      In-place shift.

   **Set Operations on Items**

   .. cpp:function:: ZBDDV OffSet(int v) const

      Keep only sets NOT containing item *v* in each element.

   .. cpp:function:: ZBDDV OnSet(int v) const

      Keep only sets containing item *v* in each element.

   .. cpp:function:: ZBDDV OnSet0(int v) const

      Like ``OnSet(v)`` then remove item *v*.

   .. cpp:function:: ZBDDV Change(int v) const

      Toggle item *v* in all sets of each element.

   .. cpp:function:: ZBDDV Swap(int v1, int v2) const

      Swap items *v1* and *v2* in all elements.

   **Array Access**

   .. cpp:function:: ZDD GetZBDD(int index) const

      Return the ZBDD at the given 0-based index.

   .. cpp:function:: ZDD GetMetaZBDD() const

      Return the internal meta-ZBDD (includes system variables).

   .. cpp:function:: int Top() const

      Return the top (highest-level) user variable across all elements.
      Returns 0 if all elements are empty.

   .. cpp:function:: int Last() const

      Return the highest index with a non-empty element.

   .. cpp:function:: ZBDDV Mask(int start, int length = 1) const

      Extract a sub-array from *start* to ``start + length - 1``.

   **Output / Export**

   .. cpp:function:: uint64_t Size() const

      Total shared node count across all elements.

   .. cpp:function:: void Print() const

      Print internal info for each element to stdout.

   .. cpp:function:: void Export(FILE* strm = stdout) const

      Export the ZBDDV structure to a file.

   .. cpp:function:: int PrintPla() const

      Print the ZBDDV as PLA format to stdout.

      :return: 0 on success, 1 on error.

Free Functions
--------------

.. cpp:function:: ZBDDV ZBDDV_Import(FILE* strm = stdin)

   Import a ZBDDV from a file.

   :return: The imported ZBDDV, or a null ZBDDV on error.
