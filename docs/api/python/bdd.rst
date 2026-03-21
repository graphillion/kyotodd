BDD Class
=========

.. py:class:: kyotodd.BDD(val=0)

   A Binary Decision Diagram representing a Boolean function.

   Each BDD object holds a root node ID and is automatically protected
   from garbage collection during its lifetime.

   :param int val: Initial value. 0 for false, 1 for true, negative for null.

   Construction
   ------------

   .. py:staticmethod:: var(v)

      Create a BDD representing the given variable.

      :param int v: Variable number (1-based). If the variable has not been created yet, it is automatically created up to ``v``.
      :return: A BDD for the variable.
      :rtype: BDD

   .. py:staticmethod:: prime(v)

      Create a BDD for the positive literal of variable *v*.

      :param int v: Variable number.
      :rtype: BDD

   .. py:staticmethod:: prime_not(v)

      Create a BDD for the negative literal of variable *v*.

      :param int v: Variable number.
      :rtype: BDD

   .. py:staticmethod:: cube(lits)

      Create a BDD for the conjunction (AND) of literals.

      Uses DIMACS sign convention: positive int = variable, negative int = negated variable.

      :param list[int] lits: List of literals.
      :rtype: BDD

   .. py:staticmethod:: clause(lits)

      Create a BDD for the disjunction (OR) of literals.

      Uses DIMACS sign convention: positive int = variable, negative int = negated variable.

      :param list[int] lits: List of literals.
      :rtype: BDD

   .. py:staticmethod:: getnode(var, lo, hi)

      Create a BDD node with the given variable and children.

      Applies BDD reduction rules (jump rule, complement normalization).

      :param int var: Variable number.
      :param BDD lo: Low (0) child.
      :param BDD hi: High (1) child.
      :rtype: BDD

   .. py:staticmethod:: ite(f, g, h)

      If-then-else operation: ``(f & g) | (~f & h)``.

      :param BDD f: Condition.
      :param BDD g: Then-branch.
      :param BDD h: Else-branch.
      :return: The resulting BDD.
      :rtype: BDD

   Constants
   ---------

   .. py:attribute:: false_
      :type: BDD

      Constant false BDD (class attribute).

   .. py:attribute:: true_
      :type: BDD

      Constant true BDD (class attribute).

   .. py:attribute:: null
      :type: BDD

      Null (error) BDD (class attribute).

   Operators
   ---------

   .. py:method:: __and__(other)

      Logical AND: ``self & other``.

      :param BDD other: Right operand.
      :rtype: BDD

   .. py:method:: __or__(other)

      Logical OR: ``self | other``.

      :param BDD other: Right operand.
      :rtype: BDD

   .. py:method:: __xor__(other)

      Logical XOR: ``self ^ other``.

      :param BDD other: Right operand.
      :rtype: BDD

   .. py:method:: __invert__()

      Logical NOT: ``~self``.

      :rtype: BDD

   .. py:method:: __lshift__(shift)

      Left shift: rename variable *i* to *i+shift* for all variables.

      :param int shift: Number of positions to shift.
      :rtype: BDD

   .. py:method:: __rshift__(shift)

      Right shift: rename variable *i* to *i-shift* for all variables.

      :param int shift: Number of positions to shift.
      :rtype: BDD

   .. py:method:: __iand__(other)
                   __ior__(other)
                   __ixor__(other)
                   __ilshift__(shift)
                   __irshift__(shift)

      In-place variants of the above operators.

   .. py:method:: __eq__(other)

      Equality comparison by node ID.

   .. py:method:: __ne__(other)

      Inequality comparison by node ID.

   .. py:method:: __hash__()

      Hash based on node ID.

   .. py:method:: __repr__()

      Return string representation: ``BDD(node_id=...)``.

   Cofactoring
   -----------

   .. py:method:: at0(v)

      Cofactor: restrict variable *v* to 0.

      :param int v: Variable number.
      :return: The resulting BDD.
      :rtype: BDD

   .. py:method:: at1(v)

      Cofactor: restrict variable *v* to 1.

      :param int v: Variable number.
      :return: The resulting BDD.
      :rtype: BDD

   .. py:method:: cofactor(g)

      Generalized cofactor (constrain) by BDD *g*.

      :param BDD g: The constraining BDD (must not be false).
      :return: The generalized cofactor.
      :rtype: BDD

   Quantification
   --------------

   .. py:method:: exist(cube_or_vars_or_var)

      Existential quantification.

      Can be called with:

      - A BDD cube (conjunction of variables)
      - A list of variable numbers
      - A single variable number

      :param cube_or_vars_or_var: Variables to quantify out.
      :type cube_or_vars_or_var: BDD or list[int] or int
      :return: The resulting BDD.
      :rtype: BDD

   .. py:method:: univ(cube_or_vars_or_var)

      Universal quantification.

      Can be called with:

      - A BDD cube (conjunction of variables)
      - A list of variable numbers
      - A single variable number

      :param cube_or_vars_or_var: Variables to quantify.
      :type cube_or_vars_or_var: BDD or list[int] or int
      :return: The resulting BDD.
      :rtype: BDD

   Support Set
   -----------

   .. py:method:: support()

      Return the support set as a BDD (disjunction of variables).

      :rtype: BDD

   .. py:method:: support_vec()

      Return the support set as a list of variable numbers.

      :rtype: list[int]

   Implication
   -----------

   .. py:method:: imply(other)

      Check implication: whether this BDD implies *other*.

      :param BDD other: The BDD to check against.
      :return: 1 if self implies other, 0 otherwise.
      :rtype: int

   I/O
   ---

   .. py:method:: export_str()

      Export this BDD to a string representation.

      :return: The serialized BDD.
      :rtype: str

   .. py:method:: export_file(path)

      Export this BDD to a file.

      :param str path: File path to write to.

   .. py:staticmethod:: import_str(s)

      Import a BDD from a string.

      :param str s: The serialized BDD string.
      :return: The reconstructed BDD.
      :rtype: BDD
      :raises RuntimeError: If import fails.

   .. py:staticmethod:: import_file(path)

      Import a BDD from a file.

      :param str path: File path to read from.
      :return: The reconstructed BDD.
      :rtype: BDD
      :raises RuntimeError: If import fails or file cannot be opened.

   Variable Operations
   -------------------

   .. py:method:: swap(v1, v2)

      Swap variables *v1* and *v2* in the BDD.

      :param int v1: First variable number.
      :param int v2: Second variable number.
      :return: The BDD with *v1* and *v2* swapped.
      :rtype: BDD

   .. py:method:: smooth(v)

      Smooth (existential quantification) of variable *v*.

      :param int v: Variable number to quantify out.
      :return: The resulting BDD.
      :rtype: BDD

   .. py:method:: spread(k)

      Spread variable values to neighboring *k* levels.

      :param int k: Number of levels to spread (must be >= 0).
      :return: The resulting BDD.
      :rtype: BDD

   Properties
   ----------

   .. py:property:: node_id
      :type: int

      The raw node ID of this BDD.

   .. py:property:: raw_size
      :type: int

      The number of nodes in the DAG of this BDD.

   .. py:property:: plain_size
      :type: int

      The number of nodes without complement edge sharing.

   .. py:property:: top_var
      :type: int

      The top (root) variable number of this BDD.

   .. py:property:: is_terminal
      :type: bool

      True if this is a terminal node.

   .. py:property:: is_one
      :type: bool

      True if this is the 1-terminal (true).

   .. py:property:: is_zero
      :type: bool

      True if this is the 0-terminal (false).

   Counting and Sampling
   ---------------------

   .. py:method:: count(n)

      Count the number of satisfying assignments (floating-point).

      :param int n: Number of variables in the Boolean function.
      :rtype: float

   .. py:method:: exact_count(n)

      Count the number of satisfying assignments (arbitrary precision).

      :param int n: Number of variables in the Boolean function.
      :rtype: int

   .. py:method:: uniform_sample(n, seed=0)

      Sample a satisfying assignment uniformly at random.

      :param int n: Number of variables.
      :param int seed: Random seed (default 0).
      :return: A list of variable numbers set to 1 in the sample.
      :rtype: list[int]

   Conversion
   ----------

   .. py:method:: to_qdd()

      Convert to a Quasi-reduced Decision Diagram (QDD).

      :rtype: QDD

   Child Accessors
   ---------------

   .. py:method:: child0()

      Get the 0-child (lo) with complement edge resolution.

      :rtype: BDD

   .. py:method:: child1()

      Get the 1-child (hi) with complement edge resolution.

      :rtype: BDD

   .. py:method:: child(child)

      Get the child by index (0 or 1) with complement edge resolution.

      :param int child: 0 or 1.
      :rtype: BDD

   .. py:method:: raw_child0()

      Get the raw 0-child without complement resolution.

      :rtype: BDD

   .. py:method:: raw_child1()

      Get the raw 1-child without complement resolution.

      :rtype: BDD

   .. py:method:: raw_child(child)

      Get the raw child by index without complement resolution.

      :param int child: 0 or 1.
      :rtype: BDD

   Shared Size
   -----------

   .. py:staticmethod:: shared_size(bdds)

      Count the total number of shared nodes across multiple BDDs.

      :param list[BDD] bdds: List of BDDs.
      :rtype: int

   .. py:staticmethod:: shared_plain_size(bdds)

      Count the total number of nodes across multiple BDDs without complement sharing.

      :param list[BDD] bdds: List of BDDs.
      :rtype: int

   Binary I/O
   ----------

   .. py:method:: export_binary_str()

      Export this BDD in binary format to a bytes object.

      :rtype: bytes

   .. py:staticmethod:: import_binary_str(data, ignore_type=False)

      Import a BDD from binary format bytes.

      :param bytes data: Binary data.
      :param bool ignore_type: If True, skip type checking.
      :rtype: BDD

   .. py:method:: export_binary_file(path)

      Export this BDD in binary format to a file.

      :param str path: File path to write to.

   .. py:staticmethod:: import_binary_file(path, ignore_type=False)

      Import a BDD from a binary format file.

      :param str path: File path to read from.
      :param bool ignore_type: If True, skip type checking.
      :rtype: BDD

   .. py:staticmethod:: export_binary_multi_str(bdds)

      Export multiple BDDs in binary format to a bytes object.

      :param list[BDD] bdds: List of BDDs to export.
      :rtype: bytes

   .. py:staticmethod:: import_binary_multi_str(data, ignore_type=False)

      Import multiple BDDs from binary format bytes.

      :param bytes data: Binary data.
      :param bool ignore_type: If True, skip type checking.
      :rtype: list[BDD]

   .. py:staticmethod:: export_binary_multi_file(bdds, path)

      Export multiple BDDs in binary format to a file.

      :param list[BDD] bdds: List of BDDs to export.
      :param str path: File path to write to.

   .. py:staticmethod:: import_binary_multi_file(path, ignore_type=False)

      Import multiple BDDs from a binary format file.

      :param str path: File path to read from.
      :param bool ignore_type: If True, skip type checking.
      :rtype: list[BDD]

   Sapporo I/O
   -----------

   .. py:method:: export_sapporo_str()

      Export this BDD in Sapporo format to a string.

      :rtype: str

   .. py:staticmethod:: import_sapporo_str(s)

      Import a BDD from a Sapporo format string.

      :param str s: Sapporo format string.
      :rtype: BDD

   .. py:method:: export_sapporo_file(path)

      Export this BDD in Sapporo format to a file.

      :param str path: File path to write to.

   .. py:staticmethod:: import_sapporo_file(path)

      Import a BDD from a Sapporo format file.

      :param str path: File path to read from.
      :rtype: BDD

   Graphviz Visualization
   ----------------------

   .. py:method:: save_graphviz_str(raw=False)

      Export this BDD as a Graphviz DOT string.

      :param bool raw: If True, show physical DAG with complement markers. If False (default), expand complement edges.
      :rtype: str

   .. py:method:: save_graphviz_file(path, raw=False)

      Export this BDD as a Graphviz DOT file.

      :param str path: File path to write to.
      :param bool raw: If True, show physical DAG with complement markers. If False (default), expand complement edges.
