Installation
============

C++ Library
-----------

KyotoDD uses CMake as its build system and requires a C++11 compiler.

.. code-block:: bash

   git clone --recursive https://github.com/graphillion/kyotodd.git
   cd kyotodd
   mkdir build && cd build
   cmake ..
   cmake --build .

``--recursive`` fetches the `TdZdd <https://github.com/kunisura/TdZdd>`_
submodule (``third_party/TdZdd``), which is needed only by the TdZdd
bridge (``UnreducedDD::construct(spec)``, ``UnreducedDD::zdd_subset(spec)``,
``ZDD::subset(spec)``). If you cloned without it, run
``git submodule update --init``.

This produces:

- ``libkyotodd_lib.a`` — the static library
- ``kyotodd_test`` — the test executable

To run the tests:

.. code-block:: bash

   ctest

To use KyotoDD in your project, add the ``include/`` directory to your
include path and link against the static library. Include the umbrella
header:

.. code-block:: cpp

   #include "bdd.h"

Python Package
--------------

The Python package uses `scikit-build-core <https://scikit-build-core.readthedocs.io/>`_
with pybind11. Install directly from GitHub:

.. code-block:: bash

   pip install git+https://github.com/graphillion/kyotodd.git#subdirectory=python

Or install from a local clone in editable (development) mode:

.. code-block:: bash

   git clone https://github.com/graphillion/kyotodd.git
   cd kyotodd/python
   pip install -e .

Requirements:

- Python >= 3.7
- CMake >= 3.14
- A C++11-capable compiler

Verify the installation:

.. code-block:: python

   import kyotodd
   x = kyotodd.new_var()
   print(f"Created variable {x}")
