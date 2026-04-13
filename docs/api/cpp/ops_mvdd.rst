MVBDD / MVZDD Operations
========================

Multi-Valued Decision Diagrams represent functions and families over
multi-valued variables (each taking values 0..k-1). Internally they are
emulated by standard BDDs/ZDDs using a one-hot style encoding: each MVDD
variable with domain size *k* uses *k-1* internal DD variables.

Header: ``#include "mvdd.h"``

MVDDVarInfo
-----------

.. cpp:struct:: MVDDVarInfo

   Information about a single MVDD variable.

   .. cpp:member:: bddvar mvdd_var

      MVDD variable number (1-indexed). 0 = invalid.

   .. cpp:member:: int k

      Value domain size.

   .. cpp:member:: std::vector<bddvar> dd_vars

      Internal DD variable numbers (size k-1).

   .. cpp:function:: MVDDVarInfo()

      Default constructor. mvdd_var=0, k=0, dd_vars empty.

   .. cpp:function:: MVDDVarInfo(bddvar mv, int kval, const std::vector<bddvar>& dvars)

      Construct with specified values.

MVDDVarTable
------------

.. cpp:class:: MVDDVarTable

   Manages the bidirectional mapping between MVDD variables and internal
   DD variables. All MVDD variables in a table share the same value domain
   size *k*.

   .. cpp:function:: explicit MVDDVarTable(int k)

      Construct a variable table with the given domain size.

      :param k: Value domain size. Must be >= 2 and <= 65536.
      :throws std\:\:invalid_argument: if k is out of range.

   .. cpp:function:: int k() const

      Return the value domain size.

   .. cpp:function:: bddvar mvdd_var_count() const

      Return the number of registered MVDD variables.

   .. cpp:function:: bddvar register_var(const std::vector<bddvar>& dd_vars)

      Register a new MVDD variable with the given internal DD variables.

      :param dd_vars: Internal DD variable numbers (must have size k-1).
         DD variables must be in increasing level order and not already used.
      :return: The new MVDD variable number (1-indexed).
      :throws std\:\:invalid_argument: if dd_vars.size() != k-1, or variables
         are duplicated or not in increasing level order.

   .. cpp:function:: const std::vector<bddvar>& dd_vars_of(bddvar mv) const

      Return the internal DD variable numbers for MVDD variable *mv*.

      :throws std\:\:out_of_range: if mv is out of range.

   .. cpp:function:: bddvar mvdd_var_of(bddvar dv) const

      Return the MVDD variable number for internal DD variable *dv*.
      Returns 0 if not found.

   .. cpp:function:: int dd_var_index(bddvar dv) const

      Return the index (0 to k-2) of internal DD variable *dv* within
      its MVDD variable. Returns -1 if not found.

   .. cpp:function:: const MVDDVarInfo& var_info(bddvar mv) const

      Return the info for MVDD variable *mv*.

      :throws std\:\:out_of_range: if mv is out of range.

   .. cpp:function:: bddvar get_top_dd_var(bddvar mv) const

      Return the first (lowest-level) DD variable for MVDD variable *mv*.

      :throws std\:\:out_of_range: if mv is out of range.

MVBDD Class
-----------

.. cpp:class:: MVBDD : public DDBase

   Multi-Valued BDD. Represents a Boolean function over multi-valued
   variables (each taking values 0..k-1).

   **Constructors**

   .. cpp:function:: MVBDD()

      Default constructor. root=bddnull, var_table_=nullptr.

   .. cpp:function:: explicit MVBDD(int k, bool value = false)

      Construct with a new variable table.

      :param k: Value domain size (>= 2).
      :param value: Initial Boolean value.

   .. cpp:function:: MVBDD(std::shared_ptr<MVDDVarTable> table, const BDD& bdd)

      Construct from an existing variable table and a BDD.

      :param table: Shared variable table.
      :param bdd: Internal BDD representation.

   .. cpp:function:: MVBDD(const MVBDD& other)

      Copy constructor.

   .. cpp:function:: MVBDD(MVBDD&& other)

      Move constructor.

   **Static Factory Methods**

   .. cpp:function:: static MVBDD zero(std::shared_ptr<MVDDVarTable> table)

      Constant false, sharing the given table.

   .. cpp:function:: static MVBDD one(std::shared_ptr<MVDDVarTable> table)

      Constant true, sharing the given table.

   .. cpp:function:: static MVBDD singleton(const MVBDD& base, bddvar mv, int value)

      Create a literal: MVDD variable *mv* equals *value*.

      Returns a Boolean function that is true iff variable *mv* takes the
      specified value (don't care for other variables).

      :param base: An MVBDD providing the variable table.
      :param mv: MVDD variable number (1-indexed).
      :param value: The value (0 to k-1).

   .. cpp:function:: static MVBDD ite(const MVBDD& base, bddvar mv, const std::vector<MVBDD>& children)

      Build an MVBDD by specifying a child for each value of variable *mv*.

      :param base: An MVBDD providing the variable table.
      :param mv: MVDD variable number (1-indexed).
      :param children: children[i] is the sub-function when mv == i. Size must be k.
      :throws std\:\:invalid_argument: on invalid arguments.

   .. cpp:function:: static MVBDD from_bdd(const MVBDD& base, const BDD& bdd)

      Wrap a BDD as an MVBDD using base's variable table.

   **Variable Management**

   .. cpp:function:: bddvar new_var()

      Create a new MVDD variable. Allocates k-1 internal DD variables
      via ``bddnewvar()`` and registers them in the shared table.

      :return: The new MVDD variable number (1-indexed).

   .. cpp:function:: int k() const

      Return the value domain size.

   .. cpp:function:: std::shared_ptr<MVDDVarTable> var_table() const

      Return the shared variable table.

   **Node Access**

   .. cpp:function:: bddp id() const

      Return the raw node ID.

   .. cpp:function:: MVBDD child(int value) const

      Return the cofactor when the top MVDD variable takes the given value.

      :param value: The value (0 to k-1).
      :throws std\:\:invalid_argument: on terminal or invalid value.

   .. cpp:function:: bddvar top_var() const

      Return the top MVDD variable number, or 0 for terminals.

   **Boolean Operations**

   .. cpp:function:: MVBDD operator&(const MVBDD& other) const
   .. cpp:function:: MVBDD operator|(const MVBDD& other) const
   .. cpp:function:: MVBDD operator^(const MVBDD& other) const
   .. cpp:function:: MVBDD operator~() const
   .. cpp:function:: MVBDD& operator&=(const MVBDD& other)
   .. cpp:function:: MVBDD& operator|=(const MVBDD& other)
   .. cpp:function:: MVBDD& operator^=(const MVBDD& other)

   **Evaluation**

   .. cpp:function:: bool evaluate(const std::vector<int>& assignment) const

      Evaluate the Boolean function for the given MVDD assignment.

      :param assignment: 0-indexed. assignment[i] is the value of MVDD
         variable i+1. Size must equal the number of registered MVDD variables.
      :throws std\:\:invalid_argument: if assignment size is wrong.

   **Conversion**

   .. cpp:function:: BDD to_bdd() const

      Return the internal BDD (by value).

   **Terminal Checks**

   .. cpp:function:: bool is_zero() const
   .. cpp:function:: bool is_one() const
   .. cpp:function:: bool is_terminal() const

   **Node Count**

   .. cpp:function:: uint64_t mvbdd_node_count() const

      MVDD-level logical node count.

   .. cpp:function:: uint64_t size() const

      Internal BDD node count (same as ``raw_size()``).

   **Comparison**

   .. cpp:function:: bool operator==(const MVBDD& other) const
   .. cpp:function:: bool operator!=(const MVBDD& other) const

MVZDD Class
-----------

.. cpp:class:: MVZDD : public DDBase

   Multi-Valued ZDD. Represents a family of multi-valued assignments
   (each variable takes values 0..k-1).

   **Constructors**

   .. cpp:function:: MVZDD()

      Default constructor. root=bddnull, var_table_=nullptr.

   .. cpp:function:: explicit MVZDD(int k, bool value = false)

      Construct with a new variable table.

      :param k: Value domain size (>= 2).
      :param value: false = empty family, true = family containing the
         all-zero assignment.

   .. cpp:function:: MVZDD(std::shared_ptr<MVDDVarTable> table, const ZDD& zdd)

      Construct from an existing variable table and a ZDD.

   .. cpp:function:: MVZDD(const MVZDD& other)

      Copy constructor.

   .. cpp:function:: MVZDD(MVZDD&& other)

      Move constructor.

   **Static Factory Methods**

   .. cpp:function:: static MVZDD zero(std::shared_ptr<MVDDVarTable> table)

      Empty family, sharing the given table.

   .. cpp:function:: static MVZDD one(std::shared_ptr<MVDDVarTable> table)

      Family containing only the all-zero assignment, sharing the given table.

   .. cpp:function:: static MVZDD singleton(const MVZDD& base, bddvar mv, int value)

      Create a singleton family: one assignment where mv=value, all others=0.

      :param base: An MVZDD providing the variable table.
      :param mv: MVDD variable number (1-indexed).
      :param value: The value (0 to k-1).

   .. cpp:function:: static MVZDD ite(const MVZDD& base, bddvar mv, const std::vector<MVZDD>& children)

      Build an MVZDD by specifying a child for each value of variable *mv*.

      :param base: An MVZDD providing the variable table.
      :param mv: MVDD variable number (1-indexed).
      :param children: children[i] is the sub-family when mv == i. Size must be k.

   .. cpp:function:: static MVZDD from_zdd(const MVZDD& base, const ZDD& zdd)

      Wrap a ZDD as an MVZDD using base's variable table.

   **Variable Management**

   .. cpp:function:: bddvar new_var()

      Create a new MVDD variable (same as ``MVBDD::new_var()``).

   .. cpp:function:: int k() const

      Return the value domain size.

   .. cpp:function:: std::shared_ptr<MVDDVarTable> var_table() const

      Return the shared variable table.

   **Node Access**

   .. cpp:function:: bddp id() const

      Return the raw node ID.

   .. cpp:function:: MVZDD child(int value) const

      Return the sub-family when the top MVDD variable takes the given value.

      :param value: The value (0 to k-1).
      :throws std\:\:invalid_argument: on terminal or invalid value.

   .. cpp:function:: bddvar top_var() const

      Return the top MVDD variable number, or 0 for terminals.

   **Set Family Operations**

   .. cpp:function:: MVZDD operator+(const MVZDD& other) const

      Union.

   .. cpp:function:: MVZDD operator-(const MVZDD& other) const

      Difference.

   .. cpp:function:: MVZDD operator&(const MVZDD& other) const

      Intersection.

   .. cpp:function:: MVZDD& operator+=(const MVZDD& other)
   .. cpp:function:: MVZDD& operator-=(const MVZDD& other)
   .. cpp:function:: MVZDD& operator&=(const MVZDD& other)

   **Counting**

   .. cpp:function:: double count() const

      Count the number of MVDD assignments (double).

   .. cpp:function:: bigint::BigInt exact_count() const

      Count the number of MVDD assignments (exact, arbitrary precision).

   **Evaluation**

   .. cpp:function:: bool evaluate(const std::vector<int>& assignment) const

      Check if the given assignment is in the family.

      :param assignment: 0-indexed. assignment[i] is the value of MVDD
         variable i+1. Size must equal the number of registered MVDD variables.

   **Enumeration / Display**

   .. cpp:function:: std::vector<std::vector<int>> enumerate() const

      Enumerate all MVDD assignments in the family.

   .. cpp:function:: void print_sets(std::ostream& os) const

      Print all assignments to the stream.

   .. cpp:function:: void print_sets(std::ostream& os, const std::vector<std::string>& var_names) const

      Print all assignments with variable names.

   .. cpp:function:: std::string to_str() const

      Return a string representation of all assignments.

   **Conversion**

   .. cpp:function:: ZDD to_zdd() const

      Return the internal ZDD (by value).

   **Terminal Checks**

   .. cpp:function:: bool is_zero() const
   .. cpp:function:: bool is_one() const
   .. cpp:function:: bool is_terminal() const

   **Node Count**

   .. cpp:function:: uint64_t mvzdd_node_count() const

      MVDD-level logical node count.

   .. cpp:function:: uint64_t size() const

      Internal ZDD node count (same as ``raw_size()``).

   **Comparison**

   .. cpp:function:: bool operator==(const MVZDD& other) const
   .. cpp:function:: bool operator!=(const MVZDD& other) const
