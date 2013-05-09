/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing object plugin for 3DSMax)

	FILE: Octree.cpp

	DESCRIPTION: Implementation of the Octree functions

	CREATED BY: Benoît Leveau

	HISTORY: 20/09/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#include "StdAfx.h"
#include "Octree.h"

void GetChildBox(const Box3 &b, Box3 &childBox, int i)
{
	childBox = Box3(b.Min(), b.Max());
	childBox.Scale(0.5f);
	Point3 t;			
	if (i%2)		t.x = b.Max().x-b.Min().x;
	else			t.x = b.Min().x-b.Max().x;
	if ((i%4)>=2)	t.y = b.Max().y-b.Min().y;
	else			t.y = b.Min().y-b.Max().y;
	if (i>=4)		t.z = b.Max().z-b.Min().z;
	else			t.z = b.Min().z-b.Max().z;
	childBox.Translate(0.25f*t);
}

#ifdef DISPLAY_MORPH_ENGINE
	void Draw(GraphicsWindow *gw, const Box3 &b)
	{
		Point3 seg[2];
		seg[0] = b[0]; seg[1] = b[1]; gw->segment(seg, 1);
		seg[0] = b[0]; seg[1] = b[2]; gw->segment(seg, 1);
		seg[0] = b[0]; seg[1] = b[4]; gw->segment(seg, 1);
		seg[0] = b[1]; seg[1] = b[3]; gw->segment(seg, 1);
		seg[0] = b[1]; seg[1] = b[5]; gw->segment(seg, 1);
		seg[0] = b[2]; seg[1] = b[3]; gw->segment(seg, 1);
		seg[0] = b[2]; seg[1] = b[6]; gw->segment(seg, 1);
		seg[0] = b[3]; seg[1] = b[7]; gw->segment(seg, 1);
		seg[0] = b[4]; seg[1] = b[5]; gw->segment(seg, 1);
		seg[0] = b[4]; seg[1] = b[6]; gw->segment(seg, 1);
		seg[0] = b[5]; seg[1] = b[7]; gw->segment(seg, 1);
		seg[0] = b[6]; seg[1] = b[7]; gw->segment(seg, 1);
	}
#endif // DISPLAY_MORPH_ENGINE
