/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing object plugin for 3DSMax)

	FILE: FaceOctree.cpp

	DESCRIPTION: Implementation of the FaceOctree class

	CREATED BY: Benoît Leveau

	HISTORY: 20/09/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#include "StdAfx.h"
#include "FaceOctree.h"
#include "Distance.h"
#include <fstream>
#include "MemoryManager.h"

bool FaceOctree::HasFaces(const Coordinate &c) const
{
	const Cell *cell = &root;
	int level = 0;
	int levelToGo = c.GetLevel();
	while (level<levelToGo){
		cell = cell->GoDown(c.GetChildToGo(level++));
	}
#ifdef _FOCTREE_USE_BOOLEAN_SAMEASPARENT
	while (cell->value.IsSameAsParent()) cell = cell->parent;
#endif // _FOCTREE_USE_BOOLEAN_SAMEASPARENT
	return cell->value.faces.size()>0;
}

namespace{
	bool bAbort = false;
}

void FaceOctree::Subdivide(Cell *cell, Box3 &curBbox, int level)
{
	if (bAbort) return;
	if (GetAsyncKeyState(VK_ESCAPE)==1) {
		MessageBox(0,"FaceOctree filling aborted by user","Info",MB_OK);
		bAbort = true; 
		return;
	}
	if (level<max_depth && cell->value.faces.size()>min_faces_for_subdivide){
		Point3 p1, p2, p3;
		SUBDIVIDE(cell);
		Box3 childBoxes[8];
		for (int i=0;i<8;++i)
			GetChildBox(curBbox, childBoxes[i], i);
		for(std::vector<int>::iterator it=cell->value.faces.begin(); it!=cell->value.faces.end(); ++it){
			p1 =	mesh->verts[mesh->faces[*it].getVert(0)];
			p2 =	mesh->verts[mesh->faces[*it].getVert(1)];
			p3 =	mesh->verts[mesh->faces[*it].getVert(2)];
			for (int i=0;i<8;++i){
				if (GetIntersection(childBoxes[i],
									mesh->verts[mesh->faces[*it].getVert(0)],
									mesh->verts[mesh->faces[*it].getVert(1)],
									mesh->verts[mesh->faces[*it].getVert(2)]))
					cell->GetChildPointer(i)->value.faces.push_back(*it);
			}
		}
#ifdef _FOCTREE_USE_BOOLEAN_SAMEASPARENT
		if (level && cell->value.faces.size() == cell->parent->value.faces.size()){
			cell->value.SetSameAsParent();
		}
#endif //_FOCTREE_USE_BOOLEAN_SAMEASPARENT
		for (int i=0;i<8;++i)
			Subdivide(cell->GetChildPointer(i), childBoxes[i], level+1);
	}
#ifdef _FOCTREE_USE_BOOLEAN_SAMEASPARENT
	else{
		if (level && cell->value.faces.size() == cell->parent->value.faces.size()){
			cell->value.SetSameAsParent();
		}
	}
#endif //_FOCTREE_USE_BOOLEAN_SAMEASPARENT
}

void FaceOctree::Fill(Mesh *mesh_)
{
	TIMER(TM_TOTAL);
	
	mesh = mesh_;
	
	// Fill the list of faces for initialization of subdivide
	std::vector<int> listOfFaces;
	int numFaces = mesh->getNumFaces();
	for (int i=0;i<numFaces; ++i)
		listOfFaces.push_back(i);
	root<<FaceCellValue(listOfFaces);

	// Recursive call starting at the root node
	Subdivide(&root, bbox, 0);

	int a = 0;
	a+=2;

	OUTPUT_STATS("FaceOctree");
}

bool FaceOctree::GetBoxCoordinate(Coordinate &c, const Point3 &p, int corner) const
{
	Point3 coord((p-bbox.Min())/bbox.Width());
	coord*=(float)(2<<(max_depth-1));
	int cx = (int)floor(coord.x + ((corner%2) ? 0.5f : -0.5f));
	int cy = (int)floor(coord.y + ((corner%4)>1 ? 0.5f : -0.5f));
	int cz = (int)floor(coord.z + ((corner>3) ? 0.5f : -0.5f));
	if (cx>(2<<(max_depth-1)) || cy>(2<<(max_depth-1)) ||cz>(2<<(max_depth-1)) || cx<0 || cy<0 || cz<0)
		return false;
	int shift = 1<<(max_depth-1);
	for (int i=0;i<max_depth;++i){
		c.coord[i].flag = 1;
		c.coord[i].child = ((cx & shift)?1:0) + ((cy & shift)?2:0) + ((cz & shift)?4:0);
		shift = shift>>1;
	}
	return true;
}

void FaceOctree::GetDeepestCoordinate(Coordinate &c) const
{
	Cell *cell = (Cell *)&root;
	Cell *child;
	int level = 0;
	while(level<max_depth){
		int childIndex = c.GetChildToGo(level++);
		child = cell->GetChildPointer(childIndex);
		if (child)
			cell = child;
		else
			break;
	}
#ifdef _FOCTREE_USE_BOOLEAN_SAMEASPARENT
	if (!cell->value.faces.size() && !cell->value.IsSameAsParent()) --level;
#endif //_FOCTREE_USE_BOOLEAN_SAMEASPARENT
	c.Trim(--level);
}

void FaceOctree::GetListOfFaces(const Point3 &p, std::vector<int> &listOfFaces) const
{	
	int listSize = 0;
	Coordinate *list[8];
	for (int i=0;i<8;++i)
		list[i] = new Coordinate(max_depth);
	Coordinate c(max_depth);
	int max_level=-1;
	for (int i=0;i<8;++i){
		if (!GetBoxCoordinate(c, p, i))
			continue;
		bool bIsParent = false;
		for (int j=0;j<listSize;++j)
			if (list[j]->IsParentOf(c)){
				bIsParent = true;
				break;
			}
		if (bIsParent) continue;
		GetDeepestCoordinate(c);
		*(list[listSize++]) = c;
		int level = c.GetLevel();
		if (level>max_level) max_level = level;
	}
	for (int i=0;i<listSize;++i){
		if (list[i] && list[i]->GetLevel() == max_level){
			list[i]->GoUp();
			for (int j=i+1;j<listSize;++j){
				if (list[j] && list[i]->IsParentOf(*list[j])){
					delete list[j];
					list[j] = NULL;
				}
			}
		}
	}
	for (int i=0;i<listSize;++i){
		if (list[i]){
			const std::vector<int> &vec = GetListFromCoord(*list[i]);
			listOfFaces.insert(listOfFaces.end(), vec.begin(), vec.end());
		}
	}
	for (int i=0;i<8;++i)
		if (list[i]) delete list[i];
}

const std::vector<int> &FaceOctree::GetListFromCoord(Coordinate &c) const
{
	Cell *cell = (Cell *)&root;
	int levelToGo = c.GetLevel();
	int level = 0;
	int childIndex;
	while(level<levelToGo)
	{
		childIndex = c.GetChildToGo(level++);
		cell = cell->GetChildPointer(childIndex); // can't be NULL normally so we don't test BE CAREFULL!!
	}
	while(level<levelToGo);
#ifdef _FOCTREE_USE_BOOLEAN_SAMEASPARENT
	while (cell->value.IsSameAsParent()) cell = cell->parent;
#endif //_FOCTREE_USE_BOOLEAN_SAMEASPARENT
	return cell->value.faces;
}

const std::vector<int> &FaceOctree::GetListOfFacesFromCorner(int index) const
{
	// go to the smallest cell at the i-th corner of the octree
	Cell *c;
	const Cell *cell = &root;
	while((c = cell->childs[index])!=NULL && cell->value.faces.size())
		cell = c;
#ifdef _FOCTREE_USE_BOOLEAN_SAMEASPARENT
	if (cell->value.faces.size()==0 && !cell->value.IsSameAsParent() && cell->parent) cell = cell->parent; // go up to the last non-empty cell
#else // !_FOCTREE_USE_BOOLEAN_SAMEASPARENT
	if (cell->value.faces.size()==0 && cell->parent) cell = cell->parent; // go up to the last non-empty cell
#endif // !_FOCTREE_USE_BOOLEAN_SAMEASPARENT
	if (cell->parent) cell = cell->parent; // go up so that we're sure this cell has the closest face
#ifdef _FOCTREE_USE_BOOLEAN_SAMEASPARENT
	while (cell->value.IsSameAsParent()) cell = cell->parent;
#endif //_FOCTREE_USE_BOOLEAN_SAMEASPARENT
	return cell->value.faces;
}

#ifdef DISPLAY_MORPH_ENGINE
void FaceOctree::Display(GraphicsWindow *gw) const
{
	gw->startSegments();
	DisplayCell(gw, &root, bbox);
	gw->endSegments();
}
#endif //DISPLAY_MORPH_ENGINE

extern std::ostream &operator<<(std::ostream &o, const FaceOctree&)
{
	return o;
}
