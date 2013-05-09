/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing modifier plugin for 3DSMax)

	FILE: StdAfx.cpp

	DESCRIPTION: Precompiled Header File

	CREATED BY: Beno�t Leveau

	HISTORY: 12/06/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600	// Change this to the appropriate value to target other versions of IE.
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#include <cassert>
#define ASSERT assert

#include "GlobalDefines.h"

#include "Max.h"

#include "resource.h"		// main symbols

#include "IO.h"

#include "ChunkID.h"