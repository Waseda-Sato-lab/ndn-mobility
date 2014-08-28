# Python Code to generate random positions within areas in a hexagon.
# Slightly modified version implemented by Greg Kuperberg.
#
# http://stackoverflow.com/users/225554/greg-kuperberg
#
# 2014/08/28
# Jairo Eduardo Lopez

from math import sqrt
from random import randrange, random
from matplotlib import pyplot

# Plot total random points in the hexagon
total = 6
center = (0,0)

# Size of the regular hexagon
size = 100 

vectors = [(0,-1),(-sqrt(3.)/2.,.5),(sqrt(3.)/2.,.5)]

sides = [] 

# Puts a point within the hexagon.
# center gives the center of the hexagon
# limit [0,1,2] 0 - rhombus 1 will return points on both sides of the X axis
#               1 - rhombus 1 will return points on the negative side
#               2 - rhombus 1 will return points on the positive side
def randinunithex(center, limit=0):
    #if not randrange(3*size*size+1): return (0,0)

    if sides:
        t = sides.pop()

        (v1,v2) = (vectors[t], vectors[(t+1)%3])
        (x,y) = (randrange(0,size),randrange(1,size))
        resX = x*v1[0]+y*v2[0]
        resY = x*v1[1]+y*v2[1] + center[1]

        if t == 1:
            if limit == 1:
                if resX > 0:
                    resX = -1 * resX
            elif limit == 2:
                resX = abs(resX) 

        resX += center[0]
        return (resX,resY)
    else:
        return None

def addT(xs,ys):
    return (xs[0] + ys[0], xs[1] + ys[1])


# Always divided in 3 zones which correspond to 3 rhombus of regular hexagon
# 0 is left rhombus
# 1 is top rhombus - divided by X axis into negative and positive part
# 2 is right rhombus 
perzone = total/3

for i in xrange(perzone):
    for j in xrange(3):
        sides.append(j)

print sides

for n in xrange(total):
    v = randinunithex(center)
    if v != None:
        pyplot.plot([v[0]],[v[1]],'ro')

# Show the trimmed rhombuses
for t in xrange(3):
    (v1,v2) = (vectors[t], vectors[(t+1)%3])
    corners = [(0,1),(0,size-1),(size-1,size-1),(size-1,1),(0,1)]
    corners = [(x*v1[0]+y*v2[0],x*v1[1]+y*v2[1]) for (x,y) in corners]
    corners = [addT(x,center) for x in corners]
    pyplot.plot([x for (x,y) in corners],[y for (x,y) in corners],'b')

pyplot.show()
