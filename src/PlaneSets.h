/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing object plugin for 3DSMax)

	FILE: PlaneSets.h

	REFERENCES: GROUPING NEARLY COPLANAR POLYGONS INTO COPLANAR SETS

	URL: http://www.blitzbasic.com/codearcs/codearcs.php?code=1191

	DESCRIPTION: Implementation of the coplanarSets function

	CREATED BY: Benoît Leveau

	HISTORY: 05/10/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#pragma once

typedef float Vector[3];
typedef Vector Point;
typedef Vector Normal;
typedef float Plane[4];

/*
    Polygon--a polygon is stored as an array 'vertices' of size 3.
	Pointer 'next' is used to implement sets of polygons through
	linked lists.
*/
typedef struct poly {
    Point vertices[3];
    struct poly *next;
} Poly;

/*
    Node--each node stores a set of coplanar polygons. The set is
    implemented as a linked list pointed to by 'plist', and the
    plane equation of the set is stored in 'plane'. Pointers 'left'
    and 'right' point to the subtrees containing sets of coplanar
    polygons with plane equations respectively less than and
    greater than that stored in 'plane'.
*/
typedef struct node {
        Plane plane;
        Poly *plist;
        struct node *left, *right;
} Node;

Node *coplanarSets(std::vector<Node *> &listOfNodes, Poly *plist, float linearEps, float angularEps);
