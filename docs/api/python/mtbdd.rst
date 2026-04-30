MTBDD / MTZDD Classes
=====================

Multi-Terminal Decision Diagrams map Boolean variable assignments to
arbitrary terminal values (not just 0/1). Also known as ADDs (Algebraic
Decision Diagrams).

Four concrete classes are provided:

- **MTBDDFloat** — ``MTBDD<double>``, floating-point terminals
- **MTBDDInt** — ``MTBDD<int64_t>``, integer terminals
- **MTZDDFloat** — ``MTZDD<double>``, floating-point terminals with ZDD semantics
- **MTZDDInt** — ``MTZDD<int64_t>``, integer terminals with ZDD semantics

MTBDDFloat Class
----------------

.. py:class:: kyotodd.MTBDDFloat()

   Multi-Terminal BDD with double (float) terminal values.

   The default constructor creates a zero-terminal MTBDD.

   Static Factory Methods
   ~~~~~~~~~~~~~~~~~~~~~~

   .. py:staticmethod:: terminal(value)

      Create a terminal node with the given value.

      :param float value: Terminal value.
      :rtype: MTBDDFloat

   .. py:staticmethod:: ite(var, high, low)

      Create a node: if *var* then *high* else *low*.

      :param int var: Variable number.
      :param MTBDDFloat high: High (1-edge) child.
      :param MTBDDFloat low: Low (0-edge) child.
      :rtype: MTBDDFloat

   .. py:staticmethod:: from_bdd(bdd, zero_val=0.0, one_val=1.0)

      Convert a BDD to MTBDD. BDD false maps to *zero_val*, true maps to *one_val*.

      :param BDD bdd: Source BDD.
      :param float zero_val: Terminal value for BDD false (default: 0.0).
      :param float one_val: Terminal value for BDD true (default: 1.0).
      :rtype: MTBDDFloat

   .. py:staticmethod:: zero_terminal()

      Return the zero-terminal MTBDD.

      :returns: Zero-terminal MTBDD.
      :rtype: MTBDDFloat

   .. py:staticmethod:: min(a, b)

      Element-wise minimum.

      :param MTBDDFloat a: First operand.
      :param MTBDDFloat b: Second operand.
      :returns: Element-wise minimum.
      :rtype: MTBDDFloat

   .. py:staticmethod:: max(a, b)

      Element-wise maximum.

      :param MTBDDFloat a: First operand.
      :param MTBDDFloat b: Second operand.
      :returns: Element-wise maximum.
      :rtype: MTBDDFloat

   Query
   ~~~~~

   .. py:method:: terminal_value()

      Return the terminal value.

      :returns: The terminal value.
      :rtype: float
      :raises RuntimeError: If not a terminal node.

   .. py:method:: evaluate(assignment)

      Evaluate the MTBDD for the given assignment.

      :param list[int] assignment: Values (0/1), indexed by variable number.
      :returns: The evaluated terminal value.
      :rtype: float

   .. py:method:: ite_cond(then_case, else_case)

      ITE: if this (condition) then *then_case* else *else_case*.
      The condition is this MTBDD — non-zero selects *then_case*,
      zero selects *else_case*.

      :param MTBDDFloat then_case: Result when condition is non-zero.
      :param MTBDDFloat else_case: Result when condition is zero.
      :returns: The resulting MTBDD.
      :rtype: MTBDDFloat

   Operators
   ~~~~~~~~~

   .. py:method:: __add__(other)

      Element-wise addition: ``self + other``.

      :rtype: MTBDDFloat

   .. py:method:: __sub__(other)

      Element-wise subtraction: ``self - other``.

      :rtype: MTBDDFloat

   .. py:method:: __mul__(other)

      Element-wise multiplication: ``self * other``.

      :rtype: MTBDDFloat

   .. py:method:: __iadd__(other)
   .. py:method:: __isub__(other)
   .. py:method:: __imul__(other)

      In-place arithmetic operations.

   .. py:method:: __eq__(other)
   .. py:method:: __ne__(other)
   .. py:method:: __hash__()

   .. py:method:: __bool__()

      Always raises :exc:`TypeError`. Use ``is_zero`` or ``is_one``.

   Properties
   ~~~~~~~~~~

   .. py:property:: node_id
      :type: int

      Raw node ID.

   .. py:property:: is_terminal
      :type: bool

      True if terminal node.

   .. py:property:: is_zero
      :type: bool

      True if zero terminal.

   .. py:property:: is_one
      :type: bool

      True if terminal with value 1.0.

   .. py:property:: top_var
      :type: int

      Top variable number (0 for terminals).

   .. py:property:: raw_size
      :type: int

      DAG node count.

   I/O
   ~~~

   .. py:method:: save_svg_str(draw_zero=True)

      Export this MTBDD as an SVG string.

      :param bool draw_zero: If ``True``, draw the zero-value terminal.
      :rtype: str

   .. py:method:: save_svg_file(stream, draw_zero=True)

      Export this MTBDD as SVG to a text stream.

      :param IO stream: A text stream to write SVG to.
      :param bool draw_zero: If ``True``, draw the zero-value terminal.

   .. py:method:: export_binary_bytes()

      Export this MTBDD as binary bytes. Only float (``MTBDDFloat``) and
      int64 (``MTBDDInt``) terminal types are supported; the underlying
      C++ I/O requires a trivially-copyable terminal type fitting in 8
      bytes.

      :rtype: bytes

   .. py:staticmethod:: import_binary_bytes(data)

      Import an MTBDD from binary bytes.

      :param bytes data: Binary data produced by ``export_binary_bytes``.
      :rtype: MTBDDFloat

MTBDDInt Class
--------------

.. py:class:: kyotodd.MTBDDInt()

   Multi-Terminal BDD with int64 terminal values.

   The API is identical to :py:class:`MTBDDFloat` except that terminal
   values are integers instead of floats.

   Static Factory Methods
   ~~~~~~~~~~~~~~~~~~~~~~

   .. py:staticmethod:: terminal(value)

      :param int value: Terminal value.
      :rtype: MTBDDInt

   .. py:staticmethod:: ite(var, high, low)

      :param int var: Variable number.
      :param MTBDDInt high: High child.
      :param MTBDDInt low: Low child.
      :rtype: MTBDDInt

   .. py:staticmethod:: from_bdd(bdd, zero_val=0, one_val=1)

      :param BDD bdd: Source BDD.
      :param int zero_val: Terminal value for BDD false.
      :param int one_val: Terminal value for BDD true.
      :rtype: MTBDDInt

   .. py:staticmethod:: zero_terminal()

      Return the zero-terminal MTBDD.

      :returns: Zero-terminal MTBDD.
      :rtype: MTBDDInt

   .. py:staticmethod:: min(a, b)

      Element-wise minimum.

      :returns: Element-wise minimum.
      :rtype: MTBDDInt

   .. py:staticmethod:: max(a, b)

      Element-wise maximum.

      :returns: Element-wise maximum.
      :rtype: MTBDDInt

   Query
   ~~~~~

   .. py:method:: terminal_value()

      Return the terminal value.

      :returns: The terminal value.
      :rtype: int

   .. py:method:: evaluate(assignment)

      Evaluate the MTBDD for the given assignment.

      :returns: The evaluated terminal value.
      :rtype: int

   .. py:method:: ite_cond(then_case, else_case)

      ITE: if this (condition) then *then_case* else *else_case*.

      :returns: The resulting MTBDD.
      :rtype: MTBDDInt

   Operators
   ~~~~~~~~~

   ``+``, ``-``, ``*``, ``+=``, ``-=``, ``*=``, ``==``, ``!=``, ``hash()``
   — same semantics as :py:class:`MTBDDFloat`.

   Properties
   ~~~~~~~~~~

   ``node_id``, ``is_terminal``, ``is_zero``, ``is_one``, ``top_var``, ``raw_size``
   — same as :py:class:`MTBDDFloat`.

   I/O
   ~~~

   ``save_svg_str``, ``save_svg_file``, ``export_binary_bytes``,
   ``import_binary_bytes`` — same semantics as :py:class:`MTBDDFloat`.

MTZDDFloat Class
----------------

.. py:class:: kyotodd.MTZDDFloat()

   Multi-Terminal ZDD with double (float) terminal values.
   Uses ZDD zero-suppression semantics.

   The API is identical to :py:class:`MTBDDFloat` except:

   - ``from_zdd(zdd, zero_val, one_val)`` replaces ``from_bdd``
   - Evaluation uses ZDD semantics (zero-suppressed variables set to 1
     return zero)

   Static Factory Methods
   ~~~~~~~~~~~~~~~~~~~~~~

   .. py:staticmethod:: terminal(value)

      :param float value: Terminal value.
      :rtype: MTZDDFloat

   .. py:staticmethod:: ite(var, high, low)

      :param int var: Variable number.
      :param MTZDDFloat high: High child.
      :param MTZDDFloat low: Low child.
      :rtype: MTZDDFloat

   .. py:staticmethod:: from_zdd(zdd, zero_val=0.0, one_val=1.0)

      Convert a ZDD to MTZDD.

      :param ZDD zdd: Source ZDD.
      :param float zero_val: Terminal value for ZDD empty (default: 0.0).
      :param float one_val: Terminal value for ZDD single (default: 1.0).
      :rtype: MTZDDFloat

   .. py:staticmethod:: zero_terminal()

      Return the zero-terminal MTZDD.

      :returns: Zero-terminal MTZDD.
      :rtype: MTZDDFloat

   .. py:staticmethod:: min(a, b)

      Element-wise minimum.

      :returns: Element-wise minimum.
      :rtype: MTZDDFloat

   .. py:staticmethod:: max(a, b)

      Element-wise maximum.

      :returns: Element-wise maximum.
      :rtype: MTZDDFloat

   Query
   ~~~~~

   .. py:method:: terminal_value()

      Return the terminal value.

      :returns: The terminal value.
      :rtype: float

   .. py:method:: evaluate(assignment)

      Evaluate the MTZDD for the given assignment.

      :returns: The evaluated terminal value.
      :rtype: float

   .. py:method:: ite_cond(then_case, else_case)

      ITE: if this (condition) then *then_case* else *else_case*.

      :returns: The resulting MTZDD.
      :rtype: MTZDDFloat

   Operators
   ~~~~~~~~~

   ``+``, ``-``, ``*``, ``+=``, ``-=``, ``*=``, ``==``, ``!=``, ``hash()``
   — same semantics as :py:class:`MTBDDFloat`.

   Properties
   ~~~~~~~~~~

   ``node_id``, ``is_terminal``, ``is_zero``, ``is_one``, ``top_var``, ``raw_size``
   — same as :py:class:`MTBDDFloat`.

   I/O
   ~~~

   ``save_svg_str``, ``save_svg_file``, ``export_binary_bytes``,
   ``import_binary_bytes`` — same semantics as :py:class:`MTBDDFloat`.

MTZDDInt Class
--------------

.. py:class:: kyotodd.MTZDDInt()

   Multi-Terminal ZDD with int64 terminal values.
   Uses ZDD zero-suppression semantics.

   Static Factory Methods
   ~~~~~~~~~~~~~~~~~~~~~~

   .. py:staticmethod:: terminal(value)

      :param int value: Terminal value.
      :rtype: MTZDDInt

   .. py:staticmethod:: ite(var, high, low)

      :param int var: Variable number.
      :param MTZDDInt high: High child.
      :param MTZDDInt low: Low child.
      :rtype: MTZDDInt

   .. py:staticmethod:: from_zdd(zdd, zero_val=0, one_val=1)

      Convert a ZDD to MTZDD.

      :param ZDD zdd: Source ZDD.
      :param int zero_val: Terminal value for ZDD empty.
      :param int one_val: Terminal value for ZDD single.
      :rtype: MTZDDInt

   .. py:staticmethod:: zero_terminal()

      Return the zero-terminal MTZDD.

      :returns: Zero-terminal MTZDD.
      :rtype: MTZDDInt

   .. py:staticmethod:: min(a, b)

      Element-wise minimum.

      :returns: Element-wise minimum.
      :rtype: MTZDDInt

   .. py:staticmethod:: max(a, b)

      Element-wise maximum.

      :returns: Element-wise maximum.
      :rtype: MTZDDInt

   Query
   ~~~~~

   .. py:method:: terminal_value()

      Return the terminal value.

      :returns: The terminal value.
      :rtype: int

   .. py:method:: evaluate(assignment)

      Evaluate the MTZDD for the given assignment.

      :returns: The evaluated terminal value.
      :rtype: int

   .. py:method:: ite_cond(then_case, else_case)

      ITE: if this (condition) then *then_case* else *else_case*.

      :returns: The resulting MTZDD.
      :rtype: MTZDDInt

   Operators
   ~~~~~~~~~

   ``+``, ``-``, ``*``, ``+=``, ``-=``, ``*=``, ``==``, ``!=``, ``hash()``
   — same semantics as :py:class:`MTBDDFloat`.

   Properties
   ~~~~~~~~~~

   ``node_id``, ``is_terminal``, ``is_zero``, ``is_one``, ``top_var``, ``raw_size``
   — same as :py:class:`MTBDDFloat`.

   I/O
   ~~~

   ``save_svg_str``, ``save_svg_file``, ``export_binary_bytes``,
   ``import_binary_bytes`` — same semantics as :py:class:`MTBDDFloat`.

Example
-------

.. code-block:: python

   import kyotodd

   kyotodd.init()
   v1 = kyotodd.new_var()

   # Create an MTBDD: if v1 then 3.14 else 2.71
   hi = kyotodd.MTBDDFloat.terminal(3.14)
   lo = kyotodd.MTBDDFloat.terminal(2.71)
   f = kyotodd.MTBDDFloat.ite(v1, hi, lo)

   assert f.evaluate([0, 1]) == 3.14
   assert f.evaluate([0, 0]) == 2.71

   # Arithmetic on MTBDDs
   g = kyotodd.MTBDDFloat.terminal(1.0)
   result = f + g
   assert result.evaluate([0, 1]) == 4.14

   # Convert from BDD
   bdd = kyotodd.BDD.var(v1)
   mt = kyotodd.MTBDDInt.from_bdd(bdd, 0, 10)
   assert mt.evaluate([0, 1]) == 10
   assert mt.evaluate([0, 0]) == 0
