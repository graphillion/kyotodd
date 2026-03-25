MVBDD / MVZDD Classes
=====================

Multi-Valued Decision Diagrams represent functions and families over
multi-valued variables (each taking values 0..k-1). Internally they are
emulated by standard BDDs/ZDDs using a one-hot style encoding.

MVDDVarInfo
-----------

.. py:class:: kyotodd.MVDDVarInfo

   Information about a single MVDD variable. Read-only.

   .. py:property:: mvdd_var
      :type: int

      MVDD variable number (1-indexed).

   .. py:property:: k
      :type: int

      Value domain size.

   .. py:property:: dd_vars
      :type: list[int]

      Internal DD variable numbers (size k-1).

MVDDVarTable
------------

.. py:class:: kyotodd.MVDDVarTable(k)

   Manages the mapping between MVDD variables and internal DD variables.
   All MVDD variables in a table share the same value domain size *k*.

   :param int k: Value domain size (>= 2, <= 65536).
   :raises ValueError: If k is out of range.

   .. py:property:: k
      :type: int

      Value domain size.

   .. py:property:: mvdd_var_count
      :type: int

      Number of registered MVDD variables.

   .. py:method:: register_var(dd_vars)

      Register a new MVDD variable with the given internal DD variables.

      :param list[int] dd_vars: Internal DD variable numbers (must have size k-1).
      :return: The new MVDD variable number (1-indexed).
      :rtype: int
      :raises ValueError: If dd_vars size != k-1 or invalid.

   .. py:method:: dd_vars_of(mv)

      Return internal DD variable numbers for MVDD variable *mv*.

      :param int mv: MVDD variable number.
      :rtype: list[int]
      :raises IndexError: If mv is out of range.

   .. py:method:: mvdd_var_of(dv)

      Return the MVDD variable number for internal DD variable *dv*.
      Returns 0 if not found.

      :param int dv: Internal DD variable number.
      :rtype: int

   .. py:method:: dd_var_index(dv)

      Return the index (0 to k-2) of DD variable *dv* within its MVDD
      variable. Returns -1 if not found.

      :param int dv: Internal DD variable number.
      :rtype: int

   .. py:method:: var_info(mv)

      Return info for MVDD variable *mv*.

      :param int mv: MVDD variable number.
      :rtype: MVDDVarInfo
      :raises IndexError: If mv is out of range.

   .. py:method:: get_top_dd_var(mv)

      Return the first (lowest-level) DD variable for MVDD variable *mv*.

      :param int mv: MVDD variable number.
      :rtype: int
      :raises IndexError: If mv is out of range.

MVBDD Class
-----------

.. py:class:: kyotodd.MVBDD(k, value=False)
              kyotodd.MVBDD(table, bdd)

   Multi-Valued BDD. Represents a Boolean function over multi-valued
   variables (each taking values 0..k-1).

   :param int k: Value domain size (creates a new variable table).
   :param bool value: Initial Boolean value (default: False).
   :param MVDDVarTable table: Shared variable table.
   :param BDD bdd: Internal BDD representation.

   Static Factory Methods
   ~~~~~~~~~~~~~~~~~~~~~~

   .. py:staticmethod:: zero(table)

      Constant false, sharing the given table.

      :param MVDDVarTable table: Variable table.
      :rtype: MVBDD

   .. py:staticmethod:: one(table)

      Constant true, sharing the given table.

      :param MVDDVarTable table: Variable table.
      :rtype: MVBDD

   .. py:staticmethod:: singleton(base, mv, value)

      Create a literal: MVDD variable *mv* equals *value*.

      :param MVBDD base: An MVBDD providing the variable table.
      :param int mv: MVDD variable number (1-indexed).
      :param int value: The value (0 to k-1).
      :rtype: MVBDD

   .. py:staticmethod:: ite(base, mv, children)

      Build an MVBDD by specifying a child for each value of variable *mv*.

      :param MVBDD base: An MVBDD providing the variable table.
      :param int mv: MVDD variable number (1-indexed).
      :param list[MVBDD] children: One MVBDD per value (size must be k).
      :rtype: MVBDD

   .. py:staticmethod:: from_bdd(base, bdd)

      Wrap a BDD as an MVBDD using base's variable table.

      :param MVBDD base: An MVBDD providing the variable table.
      :param BDD bdd: The BDD to wrap.
      :rtype: MVBDD

   Variable Management
   ~~~~~~~~~~~~~~~~~~~

   .. py:method:: new_var()

      Create a new MVDD variable. Allocates k-1 internal DD variables
      and registers them.

      :return: The new MVDD variable number (1-indexed).
      :rtype: int

   Child Access
   ~~~~~~~~~~~~

   .. py:method:: child(value)

      Return the cofactor when the top MVDD variable takes the given value.

      :param int value: The value (0 to k-1).
      :rtype: MVBDD
      :raises ValueError: On terminal or invalid value.

   Evaluation
   ~~~~~~~~~~

   .. py:method:: evaluate(assignment)

      Evaluate the Boolean function for the given MVDD assignment.

      :param list[int] assignment: 0-indexed. assignment[i] is the value
         of MVDD variable i+1.
      :rtype: bool
      :raises ValueError: If assignment size is wrong.

   Conversion
   ~~~~~~~~~~

   .. py:method:: to_bdd()

      Return the internal BDD.

      :rtype: BDD

   Operators
   ~~~~~~~~~

   .. py:method:: __and__(other)

      Boolean AND: ``self & other``.

      :rtype: MVBDD

   .. py:method:: __or__(other)

      Boolean OR: ``self | other``.

      :rtype: MVBDD

   .. py:method:: __xor__(other)

      Boolean XOR: ``self ^ other``.

      :rtype: MVBDD

   .. py:method:: __invert__()

      Boolean NOT: ``~self``.

      :rtype: MVBDD

   .. py:method:: __iand__(other)
   .. py:method:: __ior__(other)
   .. py:method:: __ixor__(other)

      In-place Boolean operations.

   .. py:method:: __eq__(other)
   .. py:method:: __ne__(other)
   .. py:method:: __hash__()

   .. py:method:: __bool__()

      Always raises :exc:`TypeError`. Use ``is_zero`` or ``is_one``.

   Properties
   ~~~~~~~~~~

   .. py:property:: k
      :type: int

      Value domain size.

   .. py:property:: var_table
      :type: MVDDVarTable

      Shared variable table.

   .. py:property:: node_id
      :type: int

      Raw node ID.

   .. py:property:: top_var
      :type: int

      Top MVDD variable number (0 for terminals).

   .. py:property:: is_zero
      :type: bool

      True if constant false.

   .. py:property:: is_one
      :type: bool

      True if constant true.

   .. py:property:: is_terminal
      :type: bool

      True if terminal node.

   .. py:property:: mvbdd_node_count
      :type: int

      MVDD-level logical node count.

   .. py:property:: size
      :type: int

      Internal BDD node count.

MVZDD Class
-----------

.. py:class:: kyotodd.MVZDD(k, value=False)
              kyotodd.MVZDD(table, zdd)

   Multi-Valued ZDD. Represents a family of multi-valued assignments
   (each variable takes values 0..k-1).

   :param int k: Value domain size (creates a new variable table).
   :param bool value: False = empty family, True = family with all-zero assignment.
   :param MVDDVarTable table: Shared variable table.
   :param ZDD zdd: Internal ZDD representation.

   Static Factory Methods
   ~~~~~~~~~~~~~~~~~~~~~~

   .. py:staticmethod:: zero(table)

      Empty family, sharing the given table.

      :param MVDDVarTable table: Variable table.
      :rtype: MVZDD

   .. py:staticmethod:: one(table)

      Family containing only the all-zero assignment.

      :param MVDDVarTable table: Variable table.
      :rtype: MVZDD

   .. py:staticmethod:: singleton(base, mv, value)

      Create a singleton family: one assignment where mv=value, all others=0.

      :param MVZDD base: An MVZDD providing the variable table.
      :param int mv: MVDD variable number (1-indexed).
      :param int value: The value (0 to k-1).
      :rtype: MVZDD

   .. py:staticmethod:: ite(base, mv, children)

      Build an MVZDD by specifying a child for each value of variable *mv*.

      :param MVZDD base: An MVZDD providing the variable table.
      :param int mv: MVDD variable number (1-indexed).
      :param list[MVZDD] children: One MVZDD per value (size must be k).
      :rtype: MVZDD

   .. py:staticmethod:: from_zdd(base, zdd)

      Wrap a ZDD as an MVZDD using base's variable table.

      :param MVZDD base: An MVZDD providing the variable table.
      :param ZDD zdd: The ZDD to wrap.
      :rtype: MVZDD

   Variable Management
   ~~~~~~~~~~~~~~~~~~~

   .. py:method:: new_var()

      Create a new MVDD variable.

      :return: The new MVDD variable number (1-indexed).
      :rtype: int

   Child Access
   ~~~~~~~~~~~~

   .. py:method:: child(value)

      Return the sub-family when the top MVDD variable takes the given value.

      :param int value: The value (0 to k-1).
      :rtype: MVZDD
      :raises ValueError: On terminal or invalid value.

   Evaluation
   ~~~~~~~~~~

   .. py:method:: evaluate(assignment)

      Check if the given assignment is in the family.

      :param list[int] assignment: 0-indexed. assignment[i] is the value
         of MVDD variable i+1.
      :rtype: bool
      :raises ValueError: If assignment size is wrong.

   Enumeration / Display
   ~~~~~~~~~~~~~~~~~~~~~

   .. py:method:: enumerate()

      Enumerate all MVDD assignments in the family.

      :rtype: list[list[int]]

   .. py:method:: to_str()

      Return a string representation of all assignments.

      :rtype: str

   .. py:method:: print_sets()
                  print_sets(var_names)

      Return all assignments as a string. Optionally with variable names.

      :param list[str] var_names: Optional variable name list.
      :rtype: str

   Conversion
   ~~~~~~~~~~

   .. py:method:: to_zdd()

      Return the internal ZDD.

      :rtype: ZDD

   Operators
   ~~~~~~~~~

   .. py:method:: __add__(other)

      Union: ``self + other``.

      :rtype: MVZDD

   .. py:method:: __sub__(other)

      Difference: ``self - other``.

      :rtype: MVZDD

   .. py:method:: __and__(other)

      Intersection: ``self & other``.

      :rtype: MVZDD

   .. py:method:: __iadd__(other)
   .. py:method:: __isub__(other)
   .. py:method:: __iand__(other)

      In-place set operations.

   .. py:method:: __eq__(other)
   .. py:method:: __ne__(other)
   .. py:method:: __hash__()

   .. py:method:: __str__()

      Return string representation (same as ``to_str()``).

   .. py:method:: __bool__()

      Always raises :exc:`TypeError`. Use ``is_zero`` or ``is_one``.

   Properties
   ~~~~~~~~~~

   .. py:property:: count
      :type: float

      Number of assignments (double).

   .. py:property:: exact_count
      :type: int

      Number of assignments (arbitrary precision).

   .. py:property:: k
      :type: int

      Value domain size.

   .. py:property:: var_table
      :type: MVDDVarTable

      Shared variable table.

   .. py:property:: node_id
      :type: int

      Raw node ID.

   .. py:property:: top_var
      :type: int

      Top MVDD variable number (0 for terminals).

   .. py:property:: is_zero
      :type: bool

      True if empty family.

   .. py:property:: is_one
      :type: bool

      True if family contains only the all-zero assignment.

   .. py:property:: is_terminal
      :type: bool

      True if terminal node.

   .. py:property:: mvzdd_node_count
      :type: int

      MVDD-level logical node count.

   .. py:property:: size
      :type: int

      Internal ZDD node count.

Example
-------

.. code-block:: python

   import kyotodd

   kyotodd.init()

   # Create a ternary (k=3) MVBDD
   m = kyotodd.MVBDD(3)
   v1 = m.new_var()  # MVDD variable 1 (values 0, 1, 2)
   v2 = m.new_var()  # MVDD variable 2

   # Literal: v1 == 1
   lit = kyotodd.MVBDD.singleton(m, v1, 1)
   assert lit.evaluate([1, 0]) is True
   assert lit.evaluate([0, 0]) is False

   # MVZDD: family of multi-valued assignments
   z = kyotodd.MVZDD(3)
   v1 = z.new_var()
   s0 = kyotodd.MVZDD.singleton(z, v1, 0)
   s1 = kyotodd.MVZDD.singleton(z, v1, 1)
   union = s0 + s1  # {(0,), (1,)}
   assert union.count == 2.0
   assert union.exact_count == 2
