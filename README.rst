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

Installation using pre-built packages are currently supported on Windows, Mac but excludes Linux because pre-built
packages are unsupported via PyPi. Otherwise, no special requirements or prerequisites are necessary.

.. code:: bash

    conda install -c numpy
    pip install numpy

Installation of `pyclipr` can then be performed using the pre-built python packages using the PyPi repository.

.. code:: bash

    pip install pyclipr

Alternatively, pyclipr may be compiled directly from source within the python environment. Currently the prerequisites
are the a compliant c++ build environment include CMake build system (>v3.15) and the availability of a compiler with
c++17 compatibility.  Currently the package has been tested built using Windows 10, using VS2019 and Mac OSX Sonoma.

Firstly, clone the PyClipr repository whilst ensuring that you perform the recurisve submodule when initialising
the repoistory. This ensures that all dependencies (pybind, pyclipr, eigen, fmt) are downloaded into the source tree.

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
    path2 = [(1.0, 1.0), (1.0, 50), (100, 50), (100, 1.0), (1.0,1.0)]

    # Create an offsetting object
    po = pyclipr.ClipperOffset()

    # Set the scale factor to convert to internal integer representation
    po.scaleFactor = int(1000)

    # add the path - ensuring to use Polygon for the endType argument
    # addPaths is required when working with polygon - this is a list of correctly orientated paths for exterior
    # and interior holes
    po.addPaths([np.array(path)], pyclipr.JoinType.Miter, pyclipr.EndType.Polygon)

    # Apply the offsetting operation using a delta.
    offsetSquare = po.execute(10.0)

    # Create a clipping object
    pc = pyclipr.Clipper()
    pc.scaleFactor = int(1000)

    # Add the paths to the clipping object. Ensure the subject and clip arguments are set to differentiate
    # the paths during the Boolean operation. The final argument specifies if the path is
    # open.
    pc.addPaths(offsetSquare, pyclipr.Subject)
    pc.addPath(np.array(path2), pyclipr.Clip)

    """ Test Polygon Clipping """
    # Below returns paths
    out  = pc.execute(pyclipr.Intersection, pyclipr.FillRule.EvenOdd)
    out2 = pc.execute(pyclipr.Union, pyclipr.FillRule.EvenOdd)
    out3 = pc.execute(pyclipr.Difference, pyclipr.FillRule.EvenOdd)
    out4 = pc.execute(pyclipr.Xor, pyclipr.FillRule.EvenOdd)

    # Using execute2 returns a PolyTree structure that provides hierarchical information inflormation
    # if the paths are interior or exterior
    outB = pc.execute2(pyclipr.Intersection, pyclipr.FillRule.EvenOdd)

    # An alternative equivalent name is executeTree
    outB = pc.executeTree(pyclipr.Intersection, pyclipr.FillRule.EvenOdd)


    """ Test Open Path Clipping """
    # Pyclipr can be used for clipping open paths.  This remains simple to complete using the Clipper2 library

    pc2 = pyclipr.Clipper()
    pc2.scaleFactor = int(1e5)

    # The open path is added as a subject (note the final argument is set to True)
    pc2.addPath( ((40,-10),(50,130)), pyclipr.Subject, True)

    # The clipping object is usually set to the Polygon
    pc2.addPaths(offsetSquare, pyclipr.Clip, False)

    """ Test the return types for open path clipping with option enabled"""
    # The returnOpenPaths argument is set to True to return the open paths. Note this function only works
    # well using the Boolean intersection option
    outC = pc2.execute(pyclipr.Intersection, pyclipr.FillRule.NonZero)
    outC2, openPathsC = pc2.execute(pyclipr.Intersection, pyclipr.FillRule.NonZero, returnOpenPaths=True)

    outD = pc2.execute2(pyclipr.Intersection,  pyclipr.FillRule.NonZero)
    outD2, openPathsD = pc2.execute2(pyclipr.Intersection,  pyclipr.FillRule.NonZero, returnOpenPaths=True)

    # Plot the results
    pathPoly = np.array(path)

    import matplotlib.pyplot as plt
    plt.figure()
    plt.axis('equal')

    # Plot the original polygon
    plt.fill(pathPoly[:,0], pathPoly[:,1], 'b', alpha=0.1, linewidth=1.0, linestyle='dashed', edgecolor='#000')

    # Plot the offset square
    plt.fill(offsetSquare[0][:, 0], offsetSquare[0][:, 1], linewidth=1.0, linestyle='dashed', edgecolor='#333', facecolor='none')

    # Plot the intersection
    plt.fill(out[0][:, 0], out[0][:, 1],  facecolor='#75507b')

    # Plot the open path intersection
    plt.plot(openPathsC[0][:,0], openPathsC[0][:,1],color='#222', linewidth=1.0, linestyle='dashed', marker='.',markersize=20.0)
