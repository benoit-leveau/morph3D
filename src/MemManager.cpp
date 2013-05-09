/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing modifier plugin for 3DSMax)

	FILE: MemoryManager.cpp

	DESCRIPTION: Implementation of MemoryManager

	CREATED BY: Benoît Leveau

	HISTORY: 12/06/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#include "StdAfx.h"

#define _MEM_MANAGER_CPP_
#include "MemoryManager.h"
#undef _MEM_MANAGER_CPP_

#ifdef _DEBUG

// Redefinition of new/delete operators
void* operator new(size_t nSize, const char * lpszFileName, int nLine)
{
	return MemManager::Instance()->op_new(nSize, lpszFileName, nLine);
}

void operator delete(void* pMem, const char* lpszFileName, int nLine)
{
	MemManager::Instance()->op_delete(pMem, lpszFileName, nLine);
}

// MemManager Member Functions
void *MemManager::op_new(size_t nSize, const char * lpszFileName, int nLine)
{
	return ::operator new(nSize, 1, lpszFileName, nLine);
}

void MemManager::op_delete(void* pMem, const char * lpszFileName, int nLine)
{
	::operator delete(pMem);
}

void *MemManager::op_malloc(size_t nSize, const char * lpszFileName, int nLine)
{
	return _malloc_dbg(nSize, 1, lpszFileName, nLine);
}

void MemManager::op_free(void* pMem, const char * lpszFileName, int nLine)
{
	return _free_dbg(pMem, 1);
}

#endif // _DEBUG
