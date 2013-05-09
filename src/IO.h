/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing modifier plugin for 3DSMax)

	FILE: IO.h

	DESCRIPTION: List of the Input/Output functions

	CREATED BY: Benoît Leveau

	HISTORY: 25/03/07

 *>	Copyright (c) 2007, All Rights Reserved.
 **********************************************************************/

#include <vector>

template <class T> IOResult SaveVector(ISave *isave, const std::vector<T> &vec)
{
	ULONG nbWritten;
	size_t size = vec.size();
	if (isave->Write(&size, sizeof(size_t), &nbWritten)!=IO_OK) return IO_ERROR;
	for (std::vector<T>::const_iterator it=vec.begin(); it!=vec.end(); ++it)
		if (isave->Write(&(*it), sizeof(T), &nbWritten)!=IO_OK) return IO_ERROR;
	return IO_OK;
}

template <class T> IOResult LoadVector(ILoad *iload, std::vector<T> &vec)
{
	ULONG nbRead;
	size_t size;
	T trash;
	if (iload->Read(&size, sizeof(size_t), &nbRead)!=IO_OK) return IO_ERROR;
	vec.reserve(size);
	for (int i=0; i<size; ++i){
		if (iload->Read(&trash, sizeof(T), &nbRead)!=IO_OK) goto IOError;
		vec.push_back(trash);
	}
	return IO_OK;
IOError:
	vec.clear();
	return IO_ERROR;
}
