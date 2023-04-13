Pyclipr - Python Polygon and Offsetting Library (Python Bindings for Clipper2 Library)
========================================================================================

.. image:: https://github.com/drlukeparry/pyclipr/actions/workflows/pythonpublish.yml/badge.svg
    :target: https://github.com/drlukeparry/pyclipr/actions
.. image:: https://badge.fury.io/py/pyclipr.svg
    :target: https://badge.fury.io/py/pyclipr
.. image:: https://static.pepy.tech/personalized-badge/pyclipr?period=total&units=international_system&left_color=black&right_color=orange&left_text=Downloads
 :target: https://pepy.tech/project/pyclipr


Pyclipr is a Python library offering the functionality of the Clipper2 polygon clipping and offsetting library based
on the `pybind <https://pybind11.readthedocs.io/en/stable/basics.html>`_ library. The package aims to provide access to the Clipper2 library for Python users. 

For further information, see the latest `release notes <https://github.com/drlukeparry/pycork/blob/master/CHANGELOG.md>`_.

Installation
*************

Installation is currently supported on Windows. No special requiremnets are necessary for using pycork, except having the numpy library available. It is recommend to also install the `trimesh <https://github.com/mikedh/trimesh>`_ library to provide an interface to processing meshes as input for pycork.

.. code:: bash

    conda install -c numpy
    pip install numpy

Installation of pycork can then be performed using pre-built python packages using the PyPi repository.

.. code:: bash

    pip install pyclipr

Alternatively, pyclipr may be compiled directly from source. Currently the prerequisites are the a compliant c++ build environment, include CMake build system. Currently the package has been tested on Windows 10, using VS2019.

.. code:: bash

    git clone https://github.com/drlukeparry/pyclipr.git && cd ./pyclipr
    git submodule update --init --recursive

    python setup.py install

Usage
******

The PyClipr library follows similar structure to that documented in `Clipper2 <http://www.angusj.com/clipper2/Docs/Overview.htm>`_ library. 

.. code:: python

    import numpy as np

    import pycork



