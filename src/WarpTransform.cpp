/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing object plugin for 3DSMax)

	FILE: WarpTransform.cpp

	DESCRIPTION: Implementation of the WarpTransform Functions

	CREATED BY: Benoît Leveau

	HISTORY: 15/02/06

 *>	Copyright (c) 2007, All Rights Reserved.
 **********************************************************************/

#include "StdAfx.h"
#include "WarpTransform.h"
#include <boost/bind.hpp>

#define ID_RCUBE			1
#define ID_EXPONENTIAL		2

// Here we define the radial functions
namespace
{
	float g_r3_f(float p)
	{
		return 1.0f/(p*p*p);
	}
	RadialFunc g_r3(g_r3_f, ID_RCUBE);

	float g_exp_f(float sigma, float p)
	{
		return (exp(-(p*p)/(sigma*sigma)));
	}
	RadialFunc g_exp(boost::bind(g_exp_f,0.2,_1), ID_EXPONENTIAL);
}

// We add them to the list of default functions
namespace RadialFuncs
{
	ListOfFuncs::ListOfFuncs()
	{
		list.push_back(g_r3);
		list.push_back(g_exp);
	}
	RadialFunc DefaultFunc(g_r3);
	ListOfFuncs listOfFuncs;
};

IOResult RadialFunc::Save(ISave *isave) const
{
	ULONG nbWritten;
	if (isave->Write(&id, sizeof(int), &nbWritten)!=IO_OK) return IO_ERROR;
	return IO_OK;
}

IOResult RadialFunc::Load(ILoad *iload)
{
	ULONG nbRead;
	if (iload->Read(&id, sizeof(int), &nbRead)!=IO_OK) return IO_ERROR;
	for (std::vector<RadialFunc>::const_iterator it=RadialFuncs::listOfFuncs.list.begin(); it!=RadialFuncs::listOfFuncs.list.end(); ++it){
		if (id == (*it).GetID()){
			func = (*it).func;
			return IO_OK;
		}
	}
	return IO_ERROR;
}
