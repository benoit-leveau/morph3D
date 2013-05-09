/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing object plugin for 3DSMax)

	FILE: TriBox.h

	REFERENCES: AABB-triangle overlap test code by Tomas Möller

	URL: http://www.blitzbasic.com/codearcs/codearcs.php?code=1191

	DESCRIPTION: Implementation of the IsTriangleInBox function

	CREATED BY: Benoît Leveau

	HISTORY: 20/09/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#pragma once

extern "C" int IsTriangleIntersectBox(float boxcenter[3], float boxhalf[3], float triverts[3][3]);
