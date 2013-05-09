/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing object plugin for 3DSMax)

	FILE: ADFOctree.cpp

	DESCRIPTION: Implementation of the ADFOctree class

	CREATED BY: Benoît Leveau

	HISTORY: 20/09/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#include "StdAfx.h"
#include "ADFOctree.h"
#include "Distance.h"
#include <fstream>
#include "MemoryManager.h"

namespace{
	float GetInterpolatedDistance(float distances[8], int i)
	{
		switch(i){
			case 0: return (0.5f * (distances[0] + distances[1]));
			case 1: return (0.5f * (distances[0] + distances[2]));
			case 2: return (0.25f * (distances[0] + distances[1] + distances[2] + distances[3]));
			case 3: return (0.5f * (distances[1] + distances[3]));
			case 4: return (0.5f * (distances[2] + distances[3]));
			case 5: return (0.5f * (distances[0] + distances[4]));
			case 6: return (0.25f * (distances[0] + distances[1] + distances[4] + distances[5]));
			case 7: return (0.5f * (distances[1] + distances[5]));
			case 8: return (0.25f * (distances[0] + distances[2] + distances[4] + distances[6]));
			case 9: return (0.125f * (distances[0] + distances[1] + distances[2] + distances[3] + distances[4] + distances[5] + distances[6] + distances[7]));
			case 10: return (0.25f * (distances[1] + distances[3] + distances[5] + distances[7]));
			case 11: return (0.5f * (distances[2] + distances[6]));
			case 12: return (0.25f * (distances[2] + distances[3] + distances[6] + distances[7]));
			case 13: return (0.5f * (distances[3] + distances[7]));
			case 14: return (0.5f * (distances[4] + distances[5]));
			case 15: return (0.5f * (distances[4] + distances[6]));
			case 16: return (0.25f * (distances[4] + distances[5] + distances[6] + distances[7]));
			case 17: return (0.5f * (distances[5] + distances[7]));
			case 18: return (0.5f * (distances[6] + distances[7]));
			default: ASSERT(false); return 0.0f;
		}
	}
	void GetChildDist(float distances[8], float distComp[19], float childDist[8], int i)
	{
		switch(i){
			case 0:{childDist[0] = distances[0];	childDist[1] = distComp[0];		childDist[2] = distComp[1];		childDist[3] = distComp[2];		childDist[4] = distComp[5];		childDist[5] = distComp[6];		childDist[6] = distComp[8];		childDist[7] = distComp[9];		break;}
			case 1:{childDist[0] = distComp[0];		childDist[1] = distances[1];	childDist[2] = distComp[2];		childDist[3] = distComp[3];		childDist[4] = distComp[6];		childDist[5] = distComp[7];		childDist[6] = distComp[9];		childDist[7] = distComp[10];	break;}
			case 2:{childDist[0] = distComp[1];		childDist[1] = distComp[2];		childDist[2] = distances[2];	childDist[3] = distComp[4];		childDist[4] = distComp[8];		childDist[5] = distComp[9];		childDist[6] = distComp[11];	childDist[7] = distComp[12];	break;}
			case 3:{childDist[0] = distComp[2];		childDist[1] = distComp[3];		childDist[2] = distComp[4];		childDist[3] = distances[3];	childDist[4] = distComp[9];		childDist[5] = distComp[10];	childDist[6] = distComp[12];	childDist[7] = distComp[13];	break;}
			case 4:{childDist[0] = distComp[5];		childDist[1] = distComp[6];		childDist[2] = distComp[8];		childDist[3] = distComp[9];		childDist[4] = distances[4];	childDist[5] = distComp[14];	childDist[6] = distComp[15];	childDist[7] = distComp[16];	break;}
			case 5:{childDist[0] = distComp[6];		childDist[1] = distComp[7];		childDist[2] = distComp[9];		childDist[3] = distComp[10];	childDist[4] = distComp[14];	childDist[5] = distances[5];	childDist[6] = distComp[16];	childDist[7] = distComp[17];	break;}
			case 6:{childDist[0] = distComp[8];		childDist[1] = distComp[9];		childDist[2] = distComp[11];	childDist[3] = distComp[12];	childDist[4] = distComp[15];	childDist[5] = distComp[16];	childDist[6] = distances[6];	childDist[7] = distComp[18];	break;}
			case 7:{childDist[0] = distComp[9];		childDist[1] = distComp[10];	childDist[2] = distComp[12];	childDist[3] = distComp[13];	childDist[4] = distComp[16];	childDist[5] = distComp[17];	childDist[6] = distComp[18];	childDist[7] = distances[7];	break;}
			default: ASSERT(false);
		}
	}
	void GetPoint(const Point3 &minBox, const Point3 &maxBox, const Point3 &ctrBox, Point3 &p, int i)
	{
		switch (i){
			case 0:		p.x = ctrBox.x;	p.y = minBox.y;	p.z = minBox.z;	break;
			case 1:		p.x = minBox.x;	p.y = ctrBox.y;	p.z = minBox.z;	break;
			case 2:		p.x = ctrBox.x;	p.y = ctrBox.y;	p.z = minBox.z;	break;
			case 3:		p.x = maxBox.x;	p.y = ctrBox.y;	p.z = minBox.z;	break;
			case 4:		p.x = ctrBox.x;	p.y = maxBox.y;	p.z = minBox.z;	break;
			case 5:		p.x = minBox.x;	p.y = minBox.y;	p.z = ctrBox.z;	break;
			case 6:		p.x = ctrBox.x;	p.y = minBox.y;	p.z = ctrBox.z;	break;
			case 7:		p.x = maxBox.x;	p.y = minBox.y;	p.z = ctrBox.z;	break;
			case 8:		p.x = minBox.x;	p.y = ctrBox.y;	p.z = ctrBox.z;	break;
			case 9:		p = ctrBox;	break;
			case 10:	p.x = maxBox.x;	p.y = ctrBox.y;	p.z = ctrBox.z;	break;
			case 11:	p.x = minBox.x;	p.y = maxBox.y;	p.z = ctrBox.z;	break;
			case 12:	p.x = ctrBox.x;	p.y = maxBox.y;	p.z = ctrBox.z;	break;
			case 13:	p.x = maxBox.x;	p.y = maxBox.y;	p.z = ctrBox.z;	break;
			case 14:	p.x = ctrBox.x;	p.y = minBox.y;	p.z = maxBox.z;	break;
			case 15:	p.x = minBox.x;	p.y = ctrBox.y;	p.z = maxBox.z;	break;
			case 16:	p.x = ctrBox.x;	p.y = ctrBox.y;	p.z = maxBox.z;	break;
			case 17:	p.x = maxBox.x;	p.y = ctrBox.y;	p.z = maxBox.z;	break;
			case 18:	p.x = ctrBox.x;	p.y = maxBox.y;	p.z = maxBox.z;	break;
			default: ASSERT(false);
		}
	}
}

float ADFOctree::GetDistance(const Point3 &p, const std::vector<int> &vec) const
{
	int index1_, index2_, index3_;
	Point3 dir_, closest_, normal_;
	ENormalType type_;
	float min_dist = maxDist + 1.0f;

	Point3 normal, closest, dir, p1, p2, p3;
	ENormalType type;
	std::vector<int>::const_iterator it;

	for (it=vec.begin(); it!=vec.end(); ++it){
		int index1 = mesh->faces[*it].getVert(0);
		int index2 = mesh->faces[*it].getVert(1);
		int index3 = mesh->faces[*it].getVert(2);
		p1 =	mesh->verts[index1];
		p2 =	mesh->verts[index2];
		p3 =	mesh->verts[index3];
		normal = avgNormal->faceNormal[*it];
		closest = GetClosestDistance(p, p1, p2, p3, normal, type);
		dir = (p-closest);
		float dist = dir.LengthSquared();
		if (dist<min_dist){
			min_dist = dist;	normal_ = normal;	closest_ = closest;
			index1_ = index1;	index2_ = index2;	index3_ = index3;
			type_ = type;		dir_ = dir;
		}
	}
	
	// WARNING: 
	// IF the edgeNormal std::map has not been filled with all the possible pair of edges, the following lines will crash
	// we could use the operator[] instead of the lower_bound() function but this can't be used on a const std::map
	switch (type_){
		case NT_Edge1: normal_ = (*avgNormal->edgeNormal.lower_bound(AveragedNormal::PairOfPoints(index1_, index2_))).second; break;
		case NT_Edge2: normal_ = (*avgNormal->edgeNormal.lower_bound(AveragedNormal::PairOfPoints(index2_, index3_))).second; break;
		case NT_Edge3: normal_ = (*avgNormal->edgeNormal.lower_bound(AveragedNormal::PairOfPoints(index1_, index3_))).second; break;
		case NT_Vertex1: normal_ = avgNormal->verticeNormal[index1_]; break;
		case NT_Vertex2: normal_ = avgNormal->verticeNormal[index2_]; break;
		case NT_Vertex3: normal_ = avgNormal->verticeNormal[index3_]; break;
		case NT_Face: default: break;
	}
	return ((dir_%normal_)<0) ? -min_dist : min_dist;
}

int cpt = 0;

void ADFOctree::Fill(const Mesh *mesh_, const AveragedNormal *avgNormal_, const FaceOctree *fOctree_)
{
	TIMER(TM_TOTAL);

	mesh = mesh_;
	fOctree = fOctree_;
	avgNormal = avgNormal_;
	
	// fill the root distances values (distances to the mesh from the 8 corners of the bbox)
	float distances[8];
	for (int i=0;i<8;++i){
		const std::vector<int> &vec = fOctree->GetListOfFacesFromCorner(i);
		distances[i] = signedSqrt(GetDistance(bbox[i], vec));
		// no need to store this distance in the map as it's a corner one,
		// it will be passed along the octree to the childs
	}

	cpt = 0;

	// Recursive call starting at the root node
	Coordinate c(max_depth);
	Subdivide(&root, c, distances, bbox, 0, true);

	OUTPUT_STATS("FaceOctree");
}

namespace{
	bool bAbort=false;
}

void ADFOctree::Subdivide(Cell *cell, Coordinate &c, float *distances, const Box3 &curBbox, int level, bool bInit)
{
	++cpt;
	if (bAbort) return;
	if (GetAsyncKeyState(VK_ESCAPE)==1) {
		MessageBox(0,"ADFOCtree filling aborted by user","Info",MB_OK);
		bAbort = true;
		return;
	}

	float distComp[19];

	(*cell)<<ADFCellValue(distances);

	if (level==max_depth)
		return; // stop recursion

	if (!fOctree->HasFaces(c) && bInit){
		skippedCells.push_back(c);
		return; // no faces in current cell, so don't subdivide it during initialization
	}

	// compute the 19 distances at the center of the edges, faces and box
	Point3 p;
	Point3 centerBox = curBbox.Center();
	Point3 minBox = curBbox.Min();
	Point3 maxBox = curBbox.Max();
	for (int i=0; i<19; ++i){
		GetPoint(minBox, maxBox, centerBox, p, i);
		MyPoint3 myp(p);
		std::map<MyPoint3, float>::iterator it = mapDistances.find(myp);
		if (it==mapDistances.end()){
			std::vector<int>listOfFaces;
			fOctree->GetListOfFaces(p, listOfFaces);
			ASSERT(listOfFaces.size());
			distComp[i] = listOfFaces.size()>0 ? signedSqrt(GetDistance(p, listOfFaces)) : 0.f;
			mapDistances[myp] = distComp[i];
		}
		else 
			distComp[i] = it->second;
	}
	if (!GetAndCheckInterpDistances(distances, distComp)){
		SUBDIVIDE(cell);
		Box3 childBox;
		float childDist[8];
		for (int i=0;i<8;++i){
			GetChildBox(curBbox, childBox, i);
			GetChildDist(distances, distComp, childDist, i);
			c.GoDown(i);
			Subdivide(cell->GetChildPointer(i), c, childDist, childBox, level+1, bInit);
			c.GoUp();
		}
	}
}

bool ADFOctree::GetAndCheckInterpDistances(float distances[8], float distComp[19]) const
{
	for (int i=0;i<19;++i){
		if (abs(distComp[i] - GetInterpolatedDistance(distances, i))>min_error)
			return false;	
	}
	return true;
}

#ifdef DISPLAY_MORPH_ENGINE
void ADFOctree::Display(GraphicsWindow *gw) const
{
	gw->startSegments();
	DisplayCell(gw, &root, bbox);
	gw->endSegments();
}
#endif // DISPLAY_MORPH_ENGINE

extern std::ostream &operator<<(std::ostream &o, const ADFOctree&)
{
	return o;
}
