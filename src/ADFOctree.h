/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing object plugin for 3DSMax)

	FILE: ADFOctree.h

	DESCRIPTION: Header of the ADFOctree class

	CREATED BY: Benoît Leveau

	HISTORY: 20/09/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#pragma once

#include <vector>
#include <map>
#include "Octree.h"
#include "FaceOctree.h"

struct AveragedNormal{
	struct PairOfPoints{
		int p1, p2;
		inline PairOfPoints(int p1, int p2) : p1(p1), p2(p2){}
		bool operator<(const PairOfPoints &p) const {
			int myFirst = min(p1,p2);
			int hisFirst = min(p.p1,p.p2);
			if (myFirst==hisFirst)
				return (max(p1,p2) < max(p.p1,p.p2));
			else
				return (myFirst<hisFirst);
		}
	};
	std::map<PairOfPoints, Point3> edgeNormal; // [numEdges]
	Point3 *faceNormal; // [numFaces]
	Point3 *verticeNormal; // [numVertices]
};

class MyPoint3 : public Point3{
public:
	bool operator<(const MyPoint3 &p) const{
		if (p.x<x) return true;
		else if (p.x>x) return false;
		if (p.y<y) return true;
		else if (p.y>y) return false;
		if (p.z<z) return true;
		else return false;
	}
	explicit MyPoint3(const Point3 &c):Point3(c){}
};

extern std::ostream &operator<<(std::ostream &o, const FaceOctree&);

struct ADFCellValue
{
// Data
	float distances[8];
//	int nFace;
//	Point3 UVCoord;

// Member Functions
	ADFCellValue(){for (int i=0;i<8;++i) distances[i] = 0.f;}
	explicit ADFCellValue(const ADFCellValue &otherValue){for (int i=0;i<8;++i) distances[i] = otherValue.distances[i];}
	explicit ADFCellValue(float *distances_){for (int i=0;i<8;++i) distances[i] = distances_[i];}
};

class ADFOctree: public Octree<ADFCellValue>
{
// Stats Data
	USE_TIMER

// Data
private:
	float min_error;
	const Mesh *mesh;
	const FaceOctree *fOctree;
	const AveragedNormal *avgNormal;
	std::map<MyPoint3, float> mapDistances;
	std::vector<Coordinate>skippedCells;
	float maxDist;

// ctor
public:
	ADFOctree(const Box3 &bbox, int max_depth, double min_error) : min_error(min_error), Octree(bbox, max_depth){
		mesh = NULL;
		fOctree = NULL;
		avgNormal = NULL;
		maxDist = (bbox.Max() - bbox.Min()).LengthSquared();
	}
	~ADFOctree(){}

// Member Functions
private:
	virtual void Reset(ADFCellValue value){value = ADFCellValue();}
	float GetDistance(const Point3 &p, const std::vector<int> &vec) const;
public:
	void Subdivide(Cell *cell, Coordinate &c, const Box3 &curBbox, int level);
	void Fill(const Mesh *mesh_, const AveragedNormal *avgNormal_, const FaceOctree *fOctree_);
	void CreateMesh(Mesh &m) const;
	bool GetAndCheckInterpDistances(float distances[8], float distComp[19]) const;
	void Subdivide(Cell *cell, Coordinate &c, float *distances, const Box3 &curBbox, int level, bool bInit);
	#ifdef DISPLAY_MORPH_ENGINE
		virtual void Display(GraphicsWindow *gw) const;
	#endif // DISPLAY_MORPH_ENGINE
};

extern std::ostream &operator<<(std::ostream &o, const ADFOctree&);
