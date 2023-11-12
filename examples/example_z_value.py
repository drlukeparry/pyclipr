import matplotlib.pyplot as plt
import numpy as np

import pyclipr

""" 
Test Open Path Clipping with Z-Attribute:
Construct a path (nx3 list) with an additional z-attribute. The Z-attribute is currently set as a floating point number
and the 'maximum' value across the intersecting edges is returned by default. In future releases, the z-attribute will
be customisable at computational expense. In this example, the single line has a z-value of 3.0 and 4.0, therefore,
the maximum Z value will be assigned to the new vertices in the resultant clipped line. 
"""
openPathPolyClipper = [(-1.0, -5.,0.1),
                       (-1.0, 105.1234, 0.1),
                       (100, 105.1234,0.1),
                       (100., -5.,0.1)]

""" Construct a Clipping Tool Object"""
pc2 = pyclipr.Clipper()
pc2.scaleFactor = int(1e6)

""" Add the existing nx3 path to the clipping tool """
pc2.addPath(openPathPolyClipper, pyclipr.Clip, False)

""" Add a second open path (line) to the clipping tool for intersection """
pc2.addPath( ((50,-20,3.0),(49.9,150,4.0)), pyclipr.Subject, isOpen = True)

""" Test the return types for open path clipping with different options selected """

# The first option will return only the paths without return the additional Z attribute.
# Essentially this will clip only the paths
closedPolys, openPaths, = pc2.execute(pyclipr.Intersection, pyclipr.FillRule.EvenOdd, returnOpenPaths=True)

# Including the returnZ=True argument will return both open and closed paths with the Z attribute
closedPolys, openPaths, closedPolyZVals, openPathZVals = pc2.execute(pyclipr.Intersection, pyclipr.FillRule.EvenOdd,
                                                                       returnOpenPaths=True, returnZ=True)

# Print the open path with the Z attribute included
for i, pnt in enumerate(openPaths[0]):
    print('Open Path with Z: x={:.3f}, y={:.3f} {:f}'.format(pnt[0], pnt[1], openPathZVals[0][i]))

""" Test the return types for open path clipping with option enabled """
outD = pc2.execute2(pyclipr.Intersection,  pyclipr.FillRule.NonZero)
outD2 = pc2.execute2(pyclipr.Intersection,  pyclipr.FillRule.NonZero, returnOpenPaths=True)
outD2withZ = pc2.execute2(pyclipr.Intersection,  pyclipr.FillRule.NonZero, returnOpenPaths=True, returnZ=True)


"""
Plot the results from the examples
"""
pathPoly = np.array(openPathPolyClipper)[:,:2]

plt.figure()
plt.axis('equal')

# Plot the original polygon
plt.fill(pathPoly[:, 0], pathPoly[:, 1], 'b', linewidth=1.0, linestyle='dashed', edgecolor='#333', facecolor='#b08fb6')

# Plot the open path intersection
plt.plot(openPaths[0][:, 0], openPaths[0][:, 1],
         color='#222', linewidth=1.0, linestyle='dashed', marker='.', markersize=20.0)