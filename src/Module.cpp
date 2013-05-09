/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing modifier plugin for 3DSMax)

	FILE: Module.cpp

	DESCRIPTION: Module Interface

	CREATED BY: Benoît Leveau

	HISTORY: 12/06/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#include "StdAfx.h"
#include "Morph3DObj.h"
#include "buildver.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

HINSTANCE hInst3DSMax = NULL;

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved)
{
	hInst3DSMax = hinstDLL;
	static bool controlsInit = FALSE;

	if (!controlsInit){
		controlsInit = TRUE;

		// jaguar controls
		InitCustomControls(hInst3DSMax);

#ifdef OLD3DCONTROLS
		// initialize 3D controls
		Ctl3dRegister(hinstDLL);
		Ctl3dAutoSubclass(hinstDLL);
#endif
		
		// initialize Chicago controls
		InitCommonControls();
	}

	switch(fdwReason) {
		case DLL_PROCESS_ATTACH:
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return(TRUE);
}


//------------------------------------------------------
// This is the interface to Jaguar:
//------------------------------------------------------

__declspec( dllexport ) const TCHAR * LibDescription()
{
	return GetString(IDS_MPH_DESCRIPTION); 
}


__declspec( dllexport ) int LibNumberClasses() 
{
	return 1; // Morph3DObj class
}

__declspec( dllexport ) ClassDesc* LibClassDesc(int i) 
{
	switch(i){
		/*case 0:		return Morph3DClassDesc::Instance();*/
		case 0:		return Morph3DObjClassDesc::Instance();
		default:	return NULL;
	}
}

// Return version so 3DSMAX can detect obsolete DLLs
__declspec( dllexport ) ULONG LibVersion() 
{ 
	return VERSION_3DSMAX; 
}

// Let the plug-in register itself for deferred loading
__declspec( dllexport ) ULONG CanAutoDefer()
{
	return 1;
}

TCHAR *GetString(int id)
{
	static TCHAR buf[256];
	if (hInst3DSMax)
		return LoadString(hInst3DSMax, id, buf, sizeof(buf)) ? buf : NULL;
	return NULL;
}
