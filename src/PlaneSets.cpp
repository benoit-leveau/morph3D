/*
GROUPING NEARLY COPLANAR POLYGONS INTO COPLANAR SETS

David Salesin
Filippo Tampieri

Cornell University
*/

/*
This code partitions a given set of arbitrary 3D polygons into
subsets of coplanar polygons.
The input polygons are organized in a linked list and the
resulting subsets of coplanar polygons are returned in the form
of a binary tree; each node of the binary tree contains a
different subset (once again stored as a linked list) and its
plane equation.  An inorder traversal of the binary tree will
return the sets of coplanar polygons sorted by plane equations
according to the total ordering imposed by the relation
implemented by the routine 'comparePlaneEqs'.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <vector>

#include "PlaneSets.h"

#ifndef M_PI
#  define M_PI		3.14159265358979323846f
#endif

#define X   0
#define Y   1
#define Z   2
#define D   3

#define VZERO(v)    (v[X] = v[Y] = v[Z] = 0.0)
#define VNORM(v)    (sqrt(v[X] * v[X] + v[Y] * v[Y] + v[Z] * v[Z]))
#define VDOT(u, v)  (u[0] * v[0] + u[1] * v[1] + u[2] * v[2])
#define VINCR(u, v) (u[X] += v[X], u[Y] += v[Y], u[Z] += v[Z])

static float linEps;  /* tolerances used by 'comparePlaneEq' to */
static float cosEps;  /* account for numerical errors           */

/* declare local routines */
void computePlaneEq(Poly *polygon, Plane plane);
bool insertPlaneEq(std::vector<Node *> &listOfNodes, Node ***tree, const Plane &plane);
Node *insertPlaneEq_Rec(Node *tree, Plane plane, Node **pset);
int comparePlaneEqs(const Plane &p1, const Plane &p2);

/*
coplanarSets--takes as input a linked list of polygons 'plist',
and two tolerances 'linearEps' and 'angularEps' and returns a
binary tree of sets of coplanar polygons.
The tolerances are used to deal with floating-point arithmetic
and numerical errors when comparing plane equations; two plane
equations are considered equal if the angle between their
respective normals is less than or equal to angularEps (in
degrees) and the distance between the two planes is less than
or equal to linearEps.
*/
Node *coplanarSets(std::vector<Node *> &listOfNodes, Poly *plist, float linearEps, float angularEps)
{
	Node *tree;
	Plane plane;
	void computePlaneEq();
	Poly *polygon, *nextPolygon;

	/* initialize the tolerances used by comparePlaneEqs() */
	linEps = linearEps;
	cosEps = cosf(angularEps * M_PI / 180.0f);

	/* place each input polygon in the appropriate set
	of coplanar polygons
	*/
	tree = NULL;
	polygon = plist;
	while(polygon != NULL) {
		nextPolygon = polygon->next;
		/* first, compute the plane equation of the polygon */
		computePlaneEq(polygon, plane);
		/* then, find the set of coplanar polygons with this plane
		equation or create a new, empty one if none exist
		*/
		Node **treeToGo = &tree;
		while (insertPlaneEq(listOfNodes, &treeToGo, plane)){}
		if (tree==NULL) tree = *treeToGo;

		/* finally, add the polygon to the selected set of
		coplanar polygons
		*/
		polygon->next = (*treeToGo)->plist;
		(*treeToGo)->plist = polygon;
		/* go to the next polygon in the input list. Note that the
		'next' field in the polygon structure is reused to
		assemble lists of coplanar polygons; thus the necessity
		for 'nextPolygon'
		*/
		polygon = nextPolygon;
	}

	return(tree);
}

/*
computePlaneEq--takes as input a pointer 'polygon' to an
arbitrary 3D polygon and returns in 'plane' the normalized
(unit normal) plane equation of the polygon.
Newell's method (see "Newell's Method for Computing the Plane
Equation of a Polygon" in this volume) is used for the
computation.
*/
void computePlaneEq(Poly *polygon, Plane plane)
{
	int i;
	Point refpt;
	Normal normal;
	float *u, *v, len;

	/* first, compute the normal of the input polygon */
	VZERO(normal);
	VZERO(refpt);
	for(i = 0; i <3; i++) {
		u = polygon->vertices[i];
		v = polygon->vertices[(i + 1) % 3];
		normal[X] += (u[Y] - v[Y]) * (u[Z] + v[Z]);
		normal[Y] += (u[Z] - v[Z]) * (u[X] + v[X]);
		normal[Z] += (u[X] - v[X]) * (u[Y] + v[Y]);
		VINCR(refpt, u);
	}
	/* then, compute the normalized plane equation using the
	arithmetic average of the vertices of the input polygon to
	determine its last coefficient. Note that, for efficiency,
	'refpt' stores the sum of the vertices rather than the
	actual average; the division by 'numVertices' is
	carried out together with the normalization when computing
	'plane[D]'.
	*/
	len = (float)VNORM(normal);
	plane[X] = normal[X] / len;
	plane[Y] = normal[Y] / len;
	plane[Z] = normal[Z] / len;
	len *= 3;
	plane[D] = -VDOT(refpt, normal) / len;
}

/*
insertPlaneEq--takes as input a binary tree 'tree' of sets of
coplanar polygons, and a plane equation 'plane', and returns
a pointer 'pset' to a set of coplanar polygons with the given
plane equation. A new, empty set is created if none is found.
*/

Node *insertPlaneEq_Rec(Node *tree, const Plane &plane, Node **pset)
{
	int i, c;

	if(tree == NULL) {
		/* the input plane is not in the tree.
		Create a new set for it
		*/
		tree = new Node();
		for(i = 0; i < 4; i++)
			tree->plane[i] = plane[i];
		tree->plist = NULL; /* no polygons in this set for now */
		tree->left = tree->right = NULL;
		*pset = tree;
	} else {
		/* compare the input plane equation with that
		of the visited node
		*/
		c = comparePlaneEqs(plane, tree->plane);
		if(c < 0)
			tree->left = insertPlaneEq_Rec(tree->left, plane, pset);
		else if(c > 0)
			tree->right = insertPlaneEq_Rec(tree->right, plane, pset);
		else
			/* the input plane is approximately equal to that
			of this node
			*/
			*pset = tree;
	}

	return(tree);
}

// removed recursion from insertPlaneEq
bool insertPlaneEq(std::vector<Node *> &listOfNodes, Node ***tree, const Plane &plane)
{
	int i, c;
	if((**tree) == NULL) {
		// the input plane is not in the tree, create a new set for it
		(**tree) = new Node();
		for(i = 0; i < 4; i++)
			(**tree)->plane[i] = plane[i];
		(**tree)->plist = NULL; /* no polygons in this set for now */
		(**tree)->left = (**tree)->right = NULL;
		listOfNodes.push_back(**tree);
		return false; // stop browsing
	} else {
		// compare the input plane equation with that of the visited node
		c = comparePlaneEqs(plane, (**tree)->plane);
		if(c < 0){
			(*tree) = &(**tree)->left;
			return true; // continue browsing using the left child
		}
		else if(c > 0){
			(*tree) = &(**tree)->right;
			return true; // continue browsing using the left child
		}
		else{
			// the input plane is approximately equal to that of this node
			return false; // stop browsing
		}
	}
}

/*
comparePlaneEqs--compares two plane equations, 'p1' and 'p2',
and returns an integer less than, equal to, or greater than
zero, depending on whether 'p1' is less than, equal to, or
greater than 'p2'.  The two global variables, 'linEps' and
'cosEps' are tolerances used to account for numerical errors.
*/
int comparePlaneEqs(const Plane &p1, const Plane &p2)
{
	int ret;
	float cosTheta;

	/* compute the cosine of the angle between the normals
	of the two planes
	*/
	cosTheta = VDOT(p1, p2);

	if(cosTheta < 0.0)
		ret = -1;
	else if(cosTheta < cosEps)
		ret = 1;
	else {
		/* the two planes are parallel and oriented in
		the same direction
		*/
		if(p1[3] + linEps < p2[3])
			ret = -1;
		else if(p2[3] + linEps < p1[3])
			ret = 1;
		else
			/* the two planes are equal */
			ret = 0;
	}

	return ret;
}

