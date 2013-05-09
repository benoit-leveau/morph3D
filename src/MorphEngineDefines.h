/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing object plugin for 3DSMax)

	FILE: MorphEngineDefines.h

	DESCRIPTION: A place to set the MorphEngine defines

	CREATED BY: Benoît Leveau

	HISTORY: 20/09/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#define	MAX_DEPTH_DEBUG		5					// Max depth in the octree
#define	MAX_DEPTH_RELEASE	6					// Max depth in the octree
#define USE_BOUNDING_BOXES_IN_FACEOCTREE	0	// Should we use the simplified test for triangles
#define MIN_FACES_FOR_SUBDIVIDE		0			// Minimum number of triangles in a cell of the FaceOctree
#define MIN_ERROR	1e-5						// Min error check when filling the ADFOctree
//#define _FOCTREE_USE_BOOLEAN_SAMEASPARENT
#define DONT_DETECT_HOLES	1

#ifdef _DEBUG
#define MAX_DEPTH	MAX_DEPTH_DEBUG
#else
#define MAX_DEPTH	MAX_DEPTH_RELEASE
#endif