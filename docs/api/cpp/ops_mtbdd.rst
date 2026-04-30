MTBDD / MTZDD Operations
========================

Multi-Terminal Decision Diagrams map Boolean variable assignments to
arbitrary terminal values (not just 0/1). ``MTBDD<T>`` uses BDD cofactoring
semantics; ``MTZDD<T>`` uses ZDD zero-suppression semantics.
``MTBDD<T>`` is also known as ADD (Algebraic Decision Diagram).

Header: ``#include "mtbdd.h"``

MTBDDTerminalTable
------------------

.. cpp:class:: template<typename T> MTBDDTerminalTable : public MTBDDTerminalTableBase

   Per-type singleton that maps terminal values to indices.
   Shared by both ``MTBDD<T>`` and ``MTZDD<T>``.

   .. cpp:function:: static MTBDDTerminalTable<T>& instance()

      Return the singleton instance for type *T*. Auto-registers with
      the global table list on first access.

   .. cpp:function:: uint64_t get_or_insert(const T& value)

      Return the index for *value*, inserting it if not already present.

   .. cpp:function:: const T& get_value(uint64_t index) const

      Return the terminal value at *index*.

      :throws std\:\:out_of_range: if index is out of range.

   .. cpp:function:: bool contains(const T& value) const

      Check whether *value* is already in the table.

   .. cpp:function:: uint64_t size() const

      Return the number of distinct terminal values.

   .. cpp:function:: uint64_t zero_index() const

      Return the index of the zero value ``T{}`` (always 0).

   .. cpp:function:: void clear()

      Reset the table to contain only the zero value at index 0.

   .. cpp:function:: static bddp make_terminal(uint64_t index)

      Create a terminal node ID from a table index.

   .. cpp:function:: static uint64_t terminal_index(bddp p)

      Extract the table index from a terminal node ID.

MTBDD Class
-----------

.. cpp:class:: template<typename T> MTBDD : public DDBase

   Multi-Terminal BDD. Maps Boolean variable assignments to terminal
   values of type *T*. Uses BDD cofactoring (missing variable → pass through
   both children).

   Also available as the alias ``ADD<T>``.

   **Constructors**

   .. cpp:function:: MTBDD()

      Default constructor. Creates a zero-terminal (``T{}``) MTBDD.

   .. cpp:function:: MTBDD(const MTBDD& other)

      Copy constructor.

   .. cpp:function:: MTBDD(MTBDD&& other)

      Move constructor.

   **Static Factory Methods**

   .. cpp:function:: static MTBDD terminal(const T& value)

      Create a terminal node with the given value.

   .. cpp:function:: static MTBDD ite(bddvar v, const MTBDD& high, const MTBDD& low)

      Create a node: if *v* then *high* else *low*.

      :param v: Variable number.
      :param high: High (1-edge) child.
      :param low: Low (0-edge) child.

   .. cpp:function:: static MTBDD from_bdd(const BDD& bdd, const T& zero_val = T{}, const T& one_val = T{1})

      Convert a BDD to MTBDD. BDD false maps to *zero_val*, true maps to *one_val*.

      :param bdd: Source BDD.
      :param zero_val: Terminal value for BDD false.
      :param one_val: Terminal value for BDD true.

   .. cpp:function:: static bddp zero_terminal()

      Return the node ID of the zero terminal.

   **Query**

   .. cpp:function:: T terminal_value() const

      Return the terminal value.

      :throws std\:\:logic_error: if not a terminal node.

   .. cpp:function:: bool is_one() const

      Check if the terminal value equals ``T{1}``. Hides ``DDBase::is_one()``.

   **Evaluation**

   .. cpp:function:: T evaluate(const std::vector<int>& assignment) const

      Evaluate the MTBDD for the given assignment. Traverses from root
      following lo/hi edges based on assignment values.

      :param assignment: assignment[v] is the value (0/1) for variable *v*.
      :throws std\:\:invalid_argument: if assignment is too short.

   **Binary Operations (apply)**

   .. cpp:function:: template<typename BinOp> MTBDD apply(const MTBDD& other, BinOp op) const

      Apply a binary operation element-wise. Each pair of terminal
      values is combined via ``op(a, b)``. Uses BDD cofactoring with
      operation cache.

   .. cpp:function:: MTBDD operator+(const MTBDD& other) const

      Element-wise addition.

   .. cpp:function:: MTBDD operator-(const MTBDD& other) const

      Element-wise subtraction.

   .. cpp:function:: MTBDD operator*(const MTBDD& other) const

      Element-wise multiplication.

   .. cpp:function:: MTBDD& operator+=(const MTBDD& other)
   .. cpp:function:: MTBDD& operator-=(const MTBDD& other)
   .. cpp:function:: MTBDD& operator*=(const MTBDD& other)

   .. cpp:function:: static MTBDD min(const MTBDD& a, const MTBDD& b)

      Element-wise minimum.

   .. cpp:function:: static MTBDD max(const MTBDD& a, const MTBDD& b)

      Element-wise maximum.

   **ITE (3-operand)**

   .. cpp:function:: MTBDD ite(const MTBDD& then_case, const MTBDD& else_case) const

      ``this`` is the condition. If the condition's terminal value is
      non-zero, return *then_case*; if zero, return *else_case*.

   **Terminal Table Access**

   .. cpp:function:: static MTBDDTerminalTable<T>& terminals()

      Return the terminal table for type *T*.

   **Comparison**

   .. cpp:function:: bool operator==(const MTBDD& other) const
   .. cpp:function:: bool operator!=(const MTBDD& other) const

   **Binary I/O**

   .. cpp:function:: void export_binary(std::ostream& strm) const
   .. cpp:function:: void export_binary(const char* filename) const

      Serialize this MTBDD to a binary stream/file (dd_type=4). The
      template argument *T* must be trivially copyable and at most 8
      bytes; this is enforced at compile time.

   .. cpp:function:: static MTBDD import_binary(std::istream& strm)
   .. cpp:function:: static MTBDD import_binary(const char* filename)

      Reconstruct an MTBDD from a binary stream/file. Malformed input
      (invalid magic, mismatched ``dd_type``, unsupported
      ``bits_for_id``, or out-of-order child references) is rejected
      with ``std::runtime_error`` / ``std::invalid_argument``.

   **SVG export**

   .. cpp:function:: void save_svg(const char* filename, const SvgParams& params = SvgParams()) const
   .. cpp:function:: void save_svg(std::ostream& strm, const SvgParams& params = SvgParams()) const
   .. cpp:function:: std::string save_svg(const SvgParams& params = SvgParams()) const

      Write or return an SVG visualization. Terminal values are stringified
      and used as terminal labels.

MTZDD Class
-----------

.. cpp:class:: template<typename T> MTZDD : public DDBase

   Multi-Terminal ZDD. Maps variable assignments to terminal values of
   type *T* using ZDD zero-suppression semantics (missing variable →
   hi = zero terminal).

   **Constructors**

   .. cpp:function:: MTZDD()

      Default constructor. Creates a zero-terminal (``T{}``) MTZDD.

   .. cpp:function:: MTZDD(const MTZDD& other)

      Copy constructor.

   .. cpp:function:: MTZDD(MTZDD&& other)

      Move constructor.

   **Static Factory Methods**

   .. cpp:function:: static MTZDD terminal(const T& value)

      Create a terminal node with the given value.

   .. cpp:function:: static MTZDD ite(bddvar v, const MTZDD& high, const MTZDD& low)

      Create a node: if *v* then *high* else *low*.

   .. cpp:function:: static MTZDD from_zdd(const ZDD& zdd, const T& zero_val = T{}, const T& one_val = T{1})

      Convert a ZDD to MTZDD. ZDD empty maps to *zero_val*, single maps to *one_val*.

   .. cpp:function:: static bddp zero_terminal()

      Return the node ID of the zero terminal.

   **Query**

   .. cpp:function:: T terminal_value() const

      Return the terminal value.

      :throws std\:\:logic_error: if not a terminal node.

   .. cpp:function:: bool is_one() const

      Check if the terminal value equals ``T{1}``. Hides ``DDBase::is_one()``.

   **Evaluation**

   .. cpp:function:: T evaluate(const std::vector<int>& assignment) const

      Evaluate the MTZDD for the given assignment. Handles zero-suppressed
      variables: if a suppressed variable is set to 1, returns ``T{}``.

      :param assignment: assignment[v] is the value (0/1) for variable *v*.

   **Binary Operations (apply)**

   .. cpp:function:: template<typename BinOp> MTZDD apply(const MTZDD& other, BinOp op) const

      Apply a binary operation element-wise using ZDD cofactoring.

   .. cpp:function:: MTZDD operator+(const MTZDD& other) const

      Element-wise addition.

   .. cpp:function:: MTZDD operator-(const MTZDD& other) const

      Element-wise subtraction.

   .. cpp:function:: MTZDD operator*(const MTZDD& other) const

      Element-wise multiplication.

   .. cpp:function:: MTZDD& operator+=(const MTZDD& other)
   .. cpp:function:: MTZDD& operator-=(const MTZDD& other)
   .. cpp:function:: MTZDD& operator*=(const MTZDD& other)

   .. cpp:function:: static MTZDD min(const MTZDD& a, const MTZDD& b)

      Element-wise minimum.

   .. cpp:function:: static MTZDD max(const MTZDD& a, const MTZDD& b)

      Element-wise maximum.

   **ITE (3-operand)**

   .. cpp:function:: MTZDD ite(const MTZDD& then_case, const MTZDD& else_case) const

      ``this`` is the condition. Uses ZDD semantics for cofactoring.

   **Terminal Table Access**

   .. cpp:function:: static MTBDDTerminalTable<T>& terminals()

      Return the terminal table for type *T*.

   **Comparison**

   .. cpp:function:: bool operator==(const MTZDD& other) const
   .. cpp:function:: bool operator!=(const MTZDD& other) const

   **Binary I/O**

   .. cpp:function:: void export_binary(std::ostream& strm) const
   .. cpp:function:: void export_binary(const char* filename) const

      Serialize this MTZDD to a binary stream/file (dd_type=5). Terminal
      type *T* must be trivially copyable and at most 8 bytes; enforced
      at compile time.

   .. cpp:function:: static MTZDD import_binary(std::istream& strm)
   .. cpp:function:: static MTZDD import_binary(const char* filename)

      Reconstruct an MTZDD from a binary stream/file. Malformed input is
      rejected with ``std::runtime_error`` / ``std::invalid_argument``.

   **SVG export**

   .. cpp:function:: void save_svg(const char* filename, const SvgParams& params = SvgParams()) const
   .. cpp:function:: void save_svg(std::ostream& strm, const SvgParams& params = SvgParams()) const
   .. cpp:function:: std::string save_svg(const SvgParams& params = SvgParams()) const

      Write or return an SVG visualization with terminal labels.

Free Functions
--------------

.. cpp:function:: uint8_t mtbdd_alloc_op_code()

   Allocate a unique operation code for the MTBDD/MTZDD operation cache.
   Codes start at 70.

.. cpp:function:: bddp mtbdd_getnode(bddvar var, bddp lo, bddp hi)

   Create an MTBDD node with BDD reduction rule (lo == hi → lo).

.. cpp:function:: bddp mtzdd_getnode(bddvar var, bddp lo, bddp hi)

   Create an MTZDD node with ZDD zero-suppression rule (hi == zero_terminal → lo).

.. cpp:function:: bddp mtbdd_getnode_raw(bddvar var, bddp lo, bddp hi)

   Create an MTBDD node without reduction rules. Advanced use only.

.. cpp:function:: bddp mtzdd_getnode_raw(bddvar var, bddp lo, bddp hi)

   Create an MTZDD node without reduction rules. Advanced use only.

.. cpp:function:: void mtbdd_register_terminal_table(MTBDDTerminalTableBase* table)

   Register a terminal table for cleanup by ``bddfinal()``.

.. cpp:function:: void mtbdd_clear_all_terminal_tables()

   Clear all registered terminal tables.
