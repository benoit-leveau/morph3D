  /**********************************************************************
 *<
	PROJECT: Morph3D (Morphing object plugin for 3DSMax)

	FILE: Distance.h

	DESCRIPTION: Header of the Maths Distance Functions

	CREATED BY: Benoît Leveau

	HISTORY: 16/09/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#pragma once

#include <boost/function.hpp>
#include <vector>

struct RigidTransformation
{
	Matrix3 R;
	Point3 T;
	Point3 OT;
	Point3 origin;
	RigidTransformation(Matrix3 R, Point3 T, Point3 OT, Point3 origin):R(R),T(T),OT(OT),origin(origin){}
	RigidTransformation(const RigidTransformation &r):R(r.R),T(r.T),OT(r.OT),origin(r.origin){}
	RigidTransformation():R(true),T(),OT(),origin(){}
	Point3 operator()(Point3 &q){
		return Point3(origin+OT+R*(q-origin)+T);
	}
	void Interpolate(float coeff){
		Quat quat(R);
		quat.Normalize();
		quat.w *= coeff;
		quat.MakeMatrix(R);
		R.IdentityMatrix();
		T *= coeff;
		OT *= coeff;
	}
	IOResult Save(ISave *isave){
		ULONG nbWritten;
		isave->BeginChunk (MORPHENGINE_RIGID_ORIGIN);
		if (isave->Write(&origin, sizeof(Point3), &nbWritten)!=IO_OK) return IO_ERROR;
		isave->EndChunk();

		isave->BeginChunk (MORPHENGINE_RIGID_OT);
		if (isave->Write(&OT, sizeof(Point3), &nbWritten)!=IO_OK) return IO_ERROR;
		isave->EndChunk();

		isave->BeginChunk (MORPHENGINE_RIGID_T);
		if (isave->Write(&T, sizeof(Point3), &nbWritten)!=IO_OK) return IO_ERROR;
		isave->EndChunk();

		isave->BeginChunk (MORPHENGINE_RIGID_R);
		if (R.Save(isave)!=IO_OK) return IO_ERROR;
		isave->EndChunk();

		return IO_OK;
	}
	IOResult Load(ILoad *iload){
		ULONG nbRead;
		IOResult res = IO_OK;
		while (IO_OK==(res=iload->OpenChunk())) {
			switch (iload->CurChunkID()) {
				case MORPHENGINE_VERSION_CHUNK:
					res = iload->Read(&origin, sizeof(Point3), &nbRead);
					break;
				case MORPHENGINE_RIGID_OT:
					res = iload->Read(&OT, sizeof(Point3), &nbRead);
					break;
				case MORPHENGINE_RIGID_T:
					res = iload->Read(&T, sizeof(Point3), &nbRead);
					break;
				case MORPHENGINE_RIGID_R:
					res = R.Load(iload);
					break;
			}
			iload->CloseChunk();
			if (res!=IO_OK)  return res;
		}
		return IO_OK;
	}
};

typedef boost::function<float(float)> RadialFunc_f;

struct RadialFunc
{
	RadialFunc_f func;
	int id;

	// ctor
	RadialFunc(const RadialFunc &rf):func(rf.func),id(rf.id){}
	RadialFunc(RadialFunc_f func, int id):func(func), id(id){}

	// functions
	int GetID() const {return id;}

	// operators
	float operator() (float p) const {return func(p);}

	// I/O
	IOResult Save(ISave *isave) const;
	IOResult Load(ILoad *iload);
};

namespace RadialFuncs
{
	struct ListOfFuncs
	{
		std::vector<RadialFunc>	list;
		ListOfFuncs();
	};
	extern ListOfFuncs listOfFuncs;
	extern RadialFunc DefaultFunc;
};

struct ElasticTransformation
{
	RadialFunc g;			// radial function
	std::vector<Point3> ai;	// coefficients of each anchor point
	std::vector<Point3> qi; // target points
	Matrix3 A;
	Point3 alpha;

	ElasticTransformation(RadialFunc g = RadialFuncs::DefaultFunc):g(g){}
	Point3 operator()(Point3 &q){
		Point3 p = A*p + alpha;
		for (std::vector<Point3>::const_iterator it=ai.begin(), qit=qi.begin(); it!=ai.end(), qit!=qi.end(); ++it, ++qit){
			p += (*it) * g((q-(*qit)).Length());
		}
		return p;
	}
	IOResult Save(ISave *isave){
		ULONG nbWritten;
		isave->BeginChunk(MORPH_ELASTIC_A_CHUNK);
		if (A.Save(isave)!=IO_OK) goto IOError;
		isave->EndChunk();

		isave->BeginChunk(MORPH_ELASTIC_AI_CHUNK);
		if (SaveVector(isave,ai)!=IO_OK) goto IOError;
		isave->EndChunk();

		isave->BeginChunk(MORPH_ELASTIC_ALPHA_CHUNK);
		if (isave->Write(&alpha, sizeof(Point3), &nbWritten)!=IO_OK) goto IOError;
		isave->EndChunk();

		isave->BeginChunk(MORPH_ELASTIC_G_CHUNK);
		if (g.Save(isave)!=IO_OK) goto IOError;
		isave->EndChunk();

		isave->BeginChunk(MORPH_ELASTIC_QI_CHUNK);
		if (SaveVector(isave,qi)!=IO_OK) goto IOError;
		isave->EndChunk();

		return IO_OK;
	IOError:
		return IO_ERROR;
	}
	IOResult Load(ILoad *iload){
		IOResult res = IO_OK;
		ULONG nbRead;
		while (IO_OK==(res=iload->OpenChunk())) {
			switch (iload->CurChunkID()) {
				case MORPH_ELASTIC_A_CHUNK:
					res = A.Load(iload);
					break;
				case MORPH_ELASTIC_AI_CHUNK:
					res = LoadVector(iload, ai);
					break;
				case MORPH_ELASTIC_ALPHA_CHUNK:
					res = iload->Read(&alpha,sizeof(Point3),&nbRead);
					break;
				case MORPH_ELASTIC_G_CHUNK:
					res = g.Load(iload);
					break;
				case MORPH_ELASTIC_QI_CHUNK:
					res = LoadVector(iload, qi);
					break;
			}
			iload->CloseChunk();
			if (res!=IO_OK)  return res;
		}
		return IO_OK;
	}
};

struct WarpTransformation
{
	RigidTransformation rigid;
	ElasticTransformation elastic;
	Point3 operator()(Point3 &p){
		return elastic(rigid(p));
	}
};