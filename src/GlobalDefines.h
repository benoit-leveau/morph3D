/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing object plugin for 3DSMax)

	FILE: GlobalDefines.h

	DESCRIPTION: Place to set all the global defines

	CREATED BY: Benoît Leveau

	HISTORY: 16/09/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#pragma once

// uncomment this line to allow the viewport visualization of the octrees
//#define DISPLAY_MORPH_ENGINE

// uncomment this line to allow the computation of stats while computing the morphing data
//#define DO_STATS

//#define OPTIMIZATIONS_OCTREE
#ifdef OPTIMIZATIONS_OCTREE
	#define OPT_OCTREE_ARRAY_SIZE	100000
	#define OPT_OCTREE(x) x
#else
	#define OPT_OCTREE(x)
#endif

#define OPTIMIZATIONS_INLINE
#ifdef OPTIMIZATIONS_INLINE
	#define INLINE inline
#else
	#define INLINE
#endif

#define DEFAULT_DIMENSION		50
#define DEFAULT_NUMOP			1
#define DEFAULT_SMOOTH			FALSE
#define DEFAULT_COEFF1			0.33f
#define DEFAULT_COEFF2			-0.34f

#include "Stats.h"