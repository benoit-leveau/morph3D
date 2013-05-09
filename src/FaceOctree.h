/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing object plugin for 3DSMax)

	FILE: FaceOctree.h

	DESCRIPTION: Header of the FaceOctree class

	CREATED BY: Benoît Leveau

	HISTORY: 20/09/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#pragma once

#include <vector>
#include <map>
#include "Octree.h"

struct FaceCellValue{
	// Data Members
	std::vector<int> faces;
#ifdef _FOCTREE_USE_BOOLEAN_SAMEASPARENT
	bool bSameAsParent;
#endif //_FOCTREE_USE_BOOLEAN_SAMEASPARENT

	// Ctor-Dtor
	inline FaceCellValue()
#ifdef _FOCTREE_USE_BOOLEAN_SAMEASPARENT
		:bSameAsParent(false)
#endif //_FOCTREE_USE_BOOLEAN_SAMEASPARENT
		{}
	inline FaceCellValue(const FaceCellValue &v):
#ifdef _FOCTREE_USE_BOOLEAN_SAMEASPARENT
		bSameAsParent(v.bSameAsParent), 
#endif // _FOCTREE_USE_BOOLEAN_SAMEASPARENT
		faces(v.faces){}
	inline FaceCellValue(std::vector<int> &faces):
#ifdef _FOCTREE_USE_BOOLEAN_SAMEASPARENT
		bSameAsParent(false),
#endif // _FOCTREE_USE_BOOLEAN_SAMEASPARENT
		faces(faces){}

	// Member Functions
#ifdef _FOCTREE_USE_BOOLEAN_SAMEASPARENT
	inline void SetSameAsParent(){
		faces.clear();
		bSameAsParent = true;
	}
	inline bool IsSameAsParent() const {return bSameAsParent;}
#endif //_FOCTREE_USE_BOOLEAN_SAMEASPARENT
	inline FaceCellValue &operator<<(const FaceCellValue &v){
		faces = v.faces;
#ifdef _FOCTREE_USE_BOOLEAN_SAMEASPARENT
		bSameAsParent = v.bSameAsParent;
#endif //_FOCTREE_USE_BOOLEAN_SAMEASPARENT
		return *this;
	}
	inline FaceCellValue &operator<<(const std::vector<int> &vec){
		faces = vec;
#ifdef _FOCTREE_USE_BOOLEAN_SAMEASPARENT
		bSameAsParent = false;
#endif //_FOCTREE_USE_BOOLEAN_SAMEASPARENT
		return *this;
	}
};

class FaceOctree: public Octree<FaceCellValue>
{
// Stats Data
	USE_TIMER

// Data
private:
	bool bUseBBToFillFaces;
	Mesh *mesh;
	int min_faces_for_subdivide;
	
// ctor
public:
	FaceOctree(const Box3 &bbox, int max_depth, int min_faces_for_subdivide) : Octree(bbox, max_depth), min_faces_for_subdivide(min_faces_for_subdivide){}
	~FaceOctree(){}

// Member Functions
private:
	virtual void Reset(FaceCellValue value){
#ifdef _FOCTREE_USE_BOOLEAN_SAMEASPARENT
		value.bSameAsParent = false;
#endif //_FOCTREE_USE_BOOLEAN_SAMEASPARENT
		value.faces.clear();
	}
public:
	void Fill(Mesh *mesh_);
	void Subdivide(Cell *cell, Box3 &bbox, int level);
	
	// Get the list of faces to process for the i-th corner of the octree
	const std::vector<int> &GetListOfFacesFromCorner(int index) const;

	// Get the list of faces to process for an arbitrary point in the octree:
	// go to the smallest bounding box surrounding the 'p' point that is sure to contains the closest point to p
	void GetListOfFaces(const Point3 &p, std::vector<int> &listOfFaces) const;

	// Get the list of faces in the specified cell
	const std::vector<int> &GetListFromCoord(Coordinate &c) const;

	// Get the coordinate of the box at the floor level of the octree at the i-th corner of the point p
	bool GetBoxCoordinate(Coordinate &c, const Point3 &p, int corner) const;

	// trim the coordinate c so that it refers to the deepest cell with a non-empty list of faces
	void GetDeepestCoordinate(Coordinate &c) const;

	bool HasFaces(const Coordinate &c) const;

	#ifdef DISPLAY_MORPH_ENGINE
		virtual void Display(GraphicsWindow *gw) const;
	#endif // DISPLAY_MORPH_ENGINE
};

extern std::ostream &operator<<(std::ostream &o, const FaceOctree&);
