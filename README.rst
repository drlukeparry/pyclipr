Pyclipr - Python Polygon and Offsetting Library (Clipper2 Bindings)
========================================================================

.. image:: https://github.com/drlukeparry/pyclipr/actions/workflows/pythonpublish.yml/badge.svg
    :target: https://github.com/drlukeparry/pyclipr/actions
.. image:: https://badge.fury.io/py/pyclipr.svg
    :target: https://badge.fury.io/py/pyclipr
.. image:: https://static.pepy.tech/personalized-badge/pyclipr?period=total&units=international_system&left_color=black&right_color=orange&left_text=Downloads
 :target: https://pepy.tech/project/pyclipr


Pyclipr is a Python library offering the functionality of the `Clipper2 <http://www.angusj.com/clipper2/Docs/Overview.htm>`_
polygon clipping and offsetting library and are built upon `pybind <https://pybind11.readthedocs.io/en/stable/basics.html>`_ .
The underlying Clipper2 library performs intersection, union, difference and XOR boolean operations on both simple and
complex polygons and also performs offsetting of polygons and inflation of paths.

Unlike `pyclipper <https://pypi.org/project/pyclipper/>`_, this library is not built using cython. Instead the full use of
capability pybind is exploited. This library aims to provide convenient access to the Clipper2 library for Python users,
especially with its usage in 3D Printing and computer graphics applications.

For further information, see the latest `release notes <https://github.com/drlukeparry/pycork/blob/master/CHANGELOG.md>`_.

Installation
*************

Installation is currently supported on Windows, Mac. No special requirements or prerequeisites are necessary.

.. code:: bash

    conda install -c numpy
    pip install numpy

Installation of pycork can then be performed using pre-built python packages using the PyPi repository.

.. code:: bash

    pip install pyclipr

Alternatively, pyclipr may be compiled directly from source within the python environment.
Currently the prerequisites are the a compliant c++ build environment include CMake build system (>v3.15) and the
availability of a compiler with c++17 compatibility.  Currently the package has been tested on Windows 10, using VS2019. E

Ensure that you perform the recurisve submodule when initialising the repoistory.

.. code:: bash

    git clone https://github.com/drlukeparry/pyclipr.git && cd ./pyclipr
    git submodule update --init --recursive

    python setup.py install

Usage
******

The pyclipr library follows similar structure to that documented in `Clipper2 <http://www.angusj.com/clipper2/Docs/Overview.htm>`_ library.
Although for consistency most methods are implemented using camelcase naming convention and more generic functions
are provided for the addition of paths.

The library assumes that coordinates are provided and scaled by a ``scaleFactor``  (*default = 1e3*), set within
the ``Clipper`` and ``ClipperOffset`` classes to ensure correct numerical robustness outlined in the underlying Clipper library.
The coordinates for the paths may be provided as a list of tuples or a numpy array.

Both ``Path64`` and ``PolyTree64`` structures are supported from the clipping and offseting operations, which are enacted
by using either `execute` or `execute2` methods, respectively.

.. code:: python

    import numpy as np
    import pyclipr

    # Tuple definition of a path
    path = [(0.0, 0.), (0, 105.1234), (100, 105.1234), (100, 0), (0, 0)]
    path2 = [(0, 0), (0, 50), (100, 50), (100, 0), (0,0)]


    # Create an offsetting object
    po = pyclipr.ClipperOffset()

    # Set the scale factor to convert to internal integer representation
    pc.scaleFactor = int(1000)

    # add the path - ensuring to use Polygon for the endType argument
    po.addPath(np.array(path), pyclipr.Miter, pyclipr.Polygon)

    # Apply the offsetting operation using a delta.
    offsetSquare = po.execute(10.0)

    # Create a clipping object
    pc = pyclipr.Clipper()
    pc.scaleFactor = int(1000)

    # Add the paths to the clipping object. Ensure the subject and clip arguments are set to differentiate
    # the paths during the Boolean operation. The final argument specifies if the path is
    # open.
    pc.addPaths(offsetSquare, pyclipr.Subject, False)
    pc.addPath(np.array(path2), pyclipr.Clip, False)

    """ Test Polygon Clipping """
    # Below returns paths
    out  = pc.execute(pyclipr.Intersection, pyclipr.EvenOdd)
    out2 = pc.execute(pyclipr.Union, pyclipr.EvenOdd)
    out3 = pc.execute(pyclipr.Difference, pyclipr.EvenOdd)
    out4 = pc.execute(pyclipr.Xor, pyclipr.EvenOdd)

    # Using execute2 returns a PolyTree structure that provides hierarchical information inflormation
    # if the paths are interior or exterior
    outB = pc.execute2(pyclipr.Intersection, pyclipr.EvenOdd)

    """ Test Open Path Clipping """
    # Pyclipr can be used for clipping open paths.  This remains simple to complete using the Clipper2 library

    pc2 = pyclipr.Clipper()
    pc2.scaleFactor = int(1e5)

    # The open path is added as a subject (note the final argument is set to True)
    pc2.addPath( ((50,-10),(50,110)), pyclipr.Subject, True)

    # The clipping object is usually set to the Polygon
    pc2.addPaths(offsetSquare, pyclipr.Clip, False)

    """ Test the return types for open path clipping with option enabled"""
    # The returnOpenPaths argument is set to True to return the open paths. Note this function only works
    # well using the Boolean intersection option
    outC = pc2.execute(pyclipr.Intersection, pyclipr.NonZero)
    outC2, openPathsC = pc2.execute(pyclipr.Intersection, pyclipr.NonZero, returnOpenPaths=True)

    outD = pc2.execute2(pyclipr.Intersection,  pyclipr.NonZero)
    outD2, openPathsD = pc2.execute2(pyclipr.Intersection,  pyclipr.NonZero, returnOpenPaths=True)




