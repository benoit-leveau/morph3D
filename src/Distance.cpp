/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing object plugin for 3DSMax)

	FILE: Distance.cpp

	DESCRIPTION: Implementation of the Maths Distance Functions

	CREATED BY: Benoît Leveau

	HISTORY: 16/09/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#include "StdAfx.h"
#include "MorphEngine.h"
#include "Distance.h"
#include "TriBox.h"
#include "MemoryManager.h"

namespace {
	Point3 temp;
	inline Point3 GetProjectOnLine(const Point3 &p, const Point3 &v1, const Point3 &v2)
	{
		temp = v2-v1;
		float c = ((temp%(p-v1))/(temp%temp));
		return ((c<=0) ? v1 : ((c>=1) ? v2 : v1+c*temp));
	}

	inline Point3 GetProjectOnPlane(const Point3 &p, const Point3 &p1, const Point3 &normale)
	{
		float d = (normale%p1)-(normale%p);
		return normale*d+p;
	}

	inline bool IsOnSameSide(const Point3 &p1, const Point3 &p2, const Point3 &v1, const Point3 &v2)
	{
		temp=v2-v1;
		return ((temp^(p1-v1))%(temp^(p2-v1))>=0);
	}

	inline bool IsInTriangle(const Point3 &p, const Point3 &a, const Point3 &b, const Point3 &c)
	{
		return (IsOnSameSide(p,a,b,c) && IsOnSameSide(p,b,a,c) && IsOnSameSide(p,c,a,b));
	}
	Point3 v,a,b;
}

Point3 GetClosestDistance(const Point3 &p, const Point3 &p1, const Point3 &p2, const Point3 &p3, const Point3 &normale, ENormalType &type)
{
	if (IsInTriangle(p,p1,p2,p3)){
		type = NT_Face;
		return GetProjectOnPlane(p, p1, normale);
	}
	else{
		v = GetProjectOnLine(p, p1, p2);
		float distance = (v-p).LengthSquared();
		type = NT_Edge1;

		a = GetProjectOnLine(p, p2, p3);
		float distance1 = (a-p).LengthSquared();
		if (distance1<distance){
			distance = distance1;
			v = a;
			type = NT_Edge2;
		}
		
		b = GetProjectOnLine(p, p3, p1);
		float distance2 = (b-p).LengthSquared();
		if (distance2<distance){
			distance = distance2;
			v = b;
			type = NT_Edge3;
		}
		if (v == p1)
			type = NT_Vertex1;
		else if (v == p2)
			type = NT_Vertex2;
		else if (v == p3)
			type = NT_Vertex3;
		return v;
	}
}
