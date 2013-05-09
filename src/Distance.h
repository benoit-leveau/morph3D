/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing object plugin for 3DSMax)

	FILE: Distance.h

	DESCRIPTION: Header of the Maths Distance Functions

	CREATED BY: Benoît Leveau

	HISTORY: 16/09/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#pragma once

#include "Morph3DObj.h"
#include "TriBox.h"

enum ENormalType{
	NT_Face,
	NT_Edge1,
	NT_Edge2,
	NT_Edge3,
	NT_Vertex1,
	NT_Vertex2,
	NT_Vertex3
};

inline bool GetIntersection(const Box3 &bbox, const Point3 &p1, const Point3 &p2, const Point3 &p3)
{
	Point3 boxCenter = bbox.Center();
	Point3 boxHalfSize = 0.5f*bbox.Width();
	float boxcenter[3];
	float boxhalf[3];
	float triverts[3][3];
	boxcenter[0]=boxCenter[0];	boxcenter[1]=boxCenter[1];	boxcenter[2]=boxCenter[2];
	boxhalf[0]=boxHalfSize[0];	boxhalf[1]=boxHalfSize[1];	boxhalf[2]=boxHalfSize[2];
	triverts[0][0]=p1[0];		triverts[0][1]=p1[1];		triverts[0][2]=p1[2];
	triverts[1][0]=p2[0];		triverts[1][1]=p2[1];		triverts[1][2]=p2[2];
	triverts[2][0]=p3[0];		triverts[2][1]=p3[1];		triverts[2][2]=p3[2];
	return (IsTriangleIntersectBox(boxcenter, 0.5f*bbox.Width(), triverts)!=0);
}

Point3 GetClosestDistance(const Point3 &p, const Point3 &p1, const Point3 &p2, const Point3 &p3, const Point3 &normale, ENormalType &type);

inline float signedSqrt(float v)
{
	return (v>0.f) ? sqrt(v) : -sqrt(-v);
}