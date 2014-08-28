# Python Code to generate random positions within areas in a hexagon.
# Slightly modified version implemented by Greg Kuperberg.
#
# http://stackoverflow.com/users/225554/greg-kuperberg
#
# 2014/08/28


from math import sqrt

from random import randrange, random
from matplotlib import pyplot

size = 100 

vectors = [(0,-1),(-sqrt(3.)/2.,.5),(sqrt(3.)/2.,.5)]

sides = [] 

def randinunithex():
    #if not randrange(3*size*size+1): return (0,0)

    t = sides.pop()

    (v1,v2) = (vectors[t], vectors[(t+1)%3])
    (x,y) = (randrange(0,size),randrange(1,size))
    return (x*v1[0]+y*v2[0],x*v1[1]+y*v2[1])

# Plot 6 random points in the hexagon
total = 6

perzone = total /3

for i in xrange(perzone):
    for j in xrange(3):
        sides.append(j)

for n in xrange(total):
    v = randinunithex()
    pyplot.plot([v[0]],[v[1]],'ro')

# Show the trimmed rhombuses
for t in xrange(3):
    (v1,v2) = (vectors[t], vectors[(t+1)%3])
    corners = [(0,1),(0,size-1),(size-1,size-1),(size-1,1),(0,1)]
    corners = [(x*v1[0]+y*v2[0],x*v1[1]+y*v2[1]) for (x,y) in corners]
    pyplot.plot([x for (x,y) in corners],[y for (x,y) in corners],'b')

pyplot.show()
