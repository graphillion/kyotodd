ZDD Operations
==============

Selection Operations
--------------------

.. doxygenfunction:: bddoffset
.. doxygenfunction:: bddonset
.. doxygenfunction:: bddonset0
.. doxygenfunction:: bddchange

Set Operations
--------------

.. doxygenfunction:: bddunion
.. doxygenfunction:: bddintersec
.. doxygenfunction:: bddsubtract
.. doxygenfunction:: bddsymdiff

Algebraic Operations
--------------------

.. doxygenfunction:: bdddiv
.. doxygenfunction:: bddremainder

Cross-Product Operations
------------------------

.. doxygenfunction:: bddjoin
.. doxygenfunction:: bddmeet
.. doxygenfunction:: bdddelta
.. doxygenfunction:: bdddisjoin
.. doxygenfunction:: bddjointjoin

Filtering Operations
--------------------

.. doxygenfunction:: bddrestrict
.. doxygenfunction:: bddpermit
.. doxygenfunction:: bddnonsup
.. doxygenfunction:: bddnonsub

Unary Operations
----------------

.. doxygenfunction:: bddmaximal
.. doxygenfunction:: bddminimal
.. doxygenfunction:: bddminhit
.. doxygenfunction:: bddclosure

Push
----

.. doxygenfunction:: bddpush

Variable Shifting
-----------------

.. doxygenfunction:: bddlshiftz
.. doxygenfunction:: bddrshiftz

Counting
--------

.. doxygenfunction:: bddcard
.. doxygenfunction:: bddlit
.. doxygenfunction:: bddlen
.. doxygenfunction:: bddexactcount
.. doxygenfunction:: bddcardmp16

Random Generation
-----------------

.. doxygenfunction:: ZDD_Random

Variable Analysis
-----------------

.. doxygenfunction:: bddispoly
.. doxygenfunction:: bddswapz
.. doxygenfunction:: bddimplychk
.. doxygenfunction:: bddcoimplychk
.. doxygenfunction:: bddpermitsym
.. doxygenfunction:: bddalways
.. doxygenfunction:: bddsymchk
.. doxygenfunction:: bddimplyset
.. doxygenfunction:: bddsymgrp
.. doxygenfunction:: bddsymgrpnaive
.. doxygenfunction:: bddsymset
.. doxygenfunction:: bddcoimplyset
.. doxygenfunction:: bdddivisor

LCM Algorithms
--------------

.. doxygenfunction:: ZDD_LCM_A
.. doxygenfunction:: ZDD_LCM_C
.. doxygenfunction:: ZDD_LCM_M
