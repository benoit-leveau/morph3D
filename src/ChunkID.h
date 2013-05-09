/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing modifier plugin for 3DSMax)

	FILE: ChunkID.cpp

	DESCRIPTION: List of the ChunkIDs used for I/O operations

	CREATED BY: Benoît Leveau

	HISTORY: 12/06/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

// Morph3D Object
#define MORPH_VERSION_CHUNK				0x0001
#define MORPH_FLAGS_CHUNK				0x0002
#define MORPH_OPANAME_CHUNK				0x0003
#define MORPH_OPBNAME_CHUNK				0x0004
#define MORPH_OFFSET_CHUNK				0x0005
#define MORPH_ENGINE_CHUNK				0x0006

// MorphEngine
#define MORPHENGINE_VERSION_CHUNK		0x0001
#define MORPHENGINE_MPHTYPE_CHUNK		0x0002
#define MORPHENGINE_RIGID_CHUNK			0x0003
#define MORPHENGINE_ELASTIC_CHUNK		0x0004
#define MORPHENGINE_ANCHORLIST_CHUNK	0x0005

// Elastic Transformation
#define MORPHENGINE_RIGID_ORIGIN		0x0001
#define MORPHENGINE_RIGID_OT			0x0002
#define MORPHENGINE_RIGID_T				0x0003
#define MORPHENGINE_RIGID_R				0x0004

// Elastic Transformation
#define MORPH_ELASTIC_A_CHUNK			0x0001
#define MORPH_ELASTIC_AI_CHUNK			0x0002
#define MORPH_ELASTIC_ALPHA_CHUNK		0x0003
#define MORPH_ELASTIC_G_CHUNK			0x0004
#define MORPH_ELASTIC_QI_CHUNK			0x0005