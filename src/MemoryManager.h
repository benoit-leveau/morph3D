/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing modifier plugin for 3DSMax)

	FILE: MemoryManager.cpp

	DESCRIPTION: Memory Manager

	CREATED BY: Benoît Leveau

	HISTORY: 12/06/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#pragma once

#include <crtdbg.h>

#ifdef _DEBUG
class MemManager{
public:

// Ctor
	MemManager(){}
	~MemManager(){
		_CrtDumpMemoryLeaks();
	}

// Member Functions
	void *op_new(size_t nSize, const char * lpszFileName, int nLine);
	void op_delete(void* pMem, const char * lpszFileName, int nLine);
	void *op_malloc(size_t nSize, const char * lpszFileName, int nLine);
	void op_free(void* pMem, const char * lpszFileName, int nLine);

// Global Instance
	static MemManager *Instance(){
		static MemManager instance;
		return &instance;
	}
};

#ifndef _MEM_MANAGER_CPP_
	// replace all the DEBUG_NEW calls to overrided 'new' operator with additional information
	void* operator new(size_t nSize, const char * lpszFileName, int nLine);
	#define new new(__FILE__, __LINE__)

	// replace all the DEBUG_DELETE calls to overrided 'delete' operator with additional information
	void operator delete(void* pMem, const char* pszFilename, int nLine);
	#define delete(x) delete(x, __FILE__, __LINE__)

	// replace all the FREE_ calls to overrided 'new' operator with additional information
	#define malloc(x) MemManager::Instance()->op_malloc(x, __FILE__, __LINE__)

	// replace all the FREE_ calls to overrided 'new' operator with additional information
	#define free(x) MemManager::Instance()->op_free(x, __FILE__, __LINE__)
#endif // !_MEM_MANAGER_CPP_

#endif // _DEBUG
