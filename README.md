morph3D
=======

C++ plugin for 3dsMax to create animated "3D mophing" between two 3D models
Written in 2006.

Original Paper
==============

The code is based on <i>Three-Dimensional Distance Field Metamorphosis</i> by Daniel Cohem-Or, David Levin and Amira Solomovici..

The paper can be consulted there: http://diyhpl.us/~bryan/papers2/frey/morphing/Cohen-Or%20D.,%20Three%20dimensional%20distance%20field%20metamorphosis.pdf

Rationale
=========

Initially started as a project during my studies, I picked it up a few years after to see if I could improve it and make a 3dsMax plugin version of it,

Additions
=========

I extended the original algorithm by using an Adaptative Distance Field Octree (class ADFOCtree), so that we don't have to store distances in each cell of the  3d grid.
It works by storing min and max distances in each cell of the octree, refining if necessary (ie. if  the ranges of the original object's cell and the destination object's cell  will produce a surface cell (distance of 0)).


