/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing object plugin for 3DSMax)

	FILE: MarchingCubes.h

	DESCRIPTION: Header of the Marching Cubes algorithm

	CREATED BY: Benoît Leveau

	HISTORY: 01/10/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#pragma once
	
typedef struct poly Poly;
typedef struct node Node;

class MarchingCube
{
private:
	void ComputeMCInCell(ADFOctree::Cell *cell, const Box3 &curBbox, Poly *&plist_ptr) const;

public:
	MarchingCube(){}
	~MarchingCube(){}

	void GetMeshFromOctree(ADFOctree::Cell *root, const Box3 &curBbox, Mesh *&mesh);
	void ComputeTree(Node *node, Poly *&plist_ptr, bool bRecursive=true) const;
};