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

      :param int v: Variable number (must have been created with :func:`~kyotodd.newvar`).
      :return: A BDD for the variable.
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

      Return the support set as a BDD (conjunction of variables).

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

   .. py:property:: size
      :type: int

      The number of nodes without complement edge sharing.

   .. py:property:: top_var
      :type: int

      The top (root) variable number of this BDD.
