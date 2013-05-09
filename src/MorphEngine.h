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
	
#include "ADFOctree.h"
#include "WarpTransform.h"

// Morph3DEngine Class Version
#define MORPH3D_ENG_VERSION 100

enum EMorphingType
{
	EMT_None,
	EMT_RigidOnly,
	EMT_ElasticOnly,
	EMT_RigidElastic,
	EMT_Morphing
};

struct Anchor
{
	Point3 s, t;
	Anchor(){}
	inline Anchor(Point3 s, Point3 t):s(s),t(t){}
};

struct SplPoint3{
	float x,y,z;
	inline SplPoint3(float x, float y, float z):x(x),y(y),z(z){}
	inline SplPoint3(float xyz[3]):x(xyz[0]),y(xyz[1]),z(xyz[2]){}
	inline SplPoint3():x(0.f),y(0.f),z(0.f){}
	inline SplPoint3(const SplPoint3 &p):x(p.x),y(p.y),z(p.z){}
	inline bool operator==(const SplPoint3 &p){return (x==p.x && y==p.y && z==p.z);}
	inline bool operator!=(const SplPoint3 &p){return !((*this)==p);}
	inline float &operator[](int index){return (index?(index>1?z:y):x);}
	inline bool operator<(const SplPoint3 &p) const{
		if (x<p.x) return true;
		else if (x>p.x) return false;
		if (y<p.y) return true;
		else if (y>p.y) return false;
		if (z<p.z) return true;
		else return false;
	}
};

struct SplPoint2{
	float x,y;
	inline SplPoint2(float x, float y):x(x),y(y){}
	inline SplPoint2(float xy[2]):x(xy[0]),y(xy[1]){}
	inline SplPoint2():x(0.f),y(0.f){}
	inline SplPoint2(const SplPoint2 &p):x(p.x),y(p.y){}
	inline bool operator==(const SplPoint2 &p){return (x==p.x && y==p.y);}
	inline bool operator!=(const SplPoint2 &p){return !((*this)==p);}
	inline float &operator[](int index){return (index?y:x);}
	inline bool operator<(const SplPoint2 &p) const{
		if (x<p.x) return true;
		else if (x>p.x) return false;
		if (y<p.y) return true;
		else return false;
	}
};

struct SplFace{
	int a, b, c;
	inline SplFace(int a, int b, int c):a(a),b(b),c(c){}
	inline SplFace():a(-1),b(-1),c(-1){}
	inline SplFace(const SplFace &f):a(f.a),b(f.b),c(f.c){}
	inline bool operator==(const SplFace &f){return (a==f.a && b==f.b && c==f.c);}
};

class MorphEngine{
private:
	class MeshMorpher{
	// Data
	private:
		Mesh *mesh;
		ADFOctree *octree;
		FaceOctree *fOctree;
		Box3 realBox;
		bool bInit;		
		int max_depth;
		double min_error;
		int min_faces;
		Box3 bbox;
		AveragedNormal avgNormals;
		int numFaces, numVertices;

	// ctor
	public:
		MeshMorpher(Mesh *m, Box3 realBox, int max_depth, double min_error, int min_faces);
		~MeshMorpher(){
			if (mesh) delete mesh;
			if (octree) delete octree;
			if (fOctree) delete fOctree;
			mesh = NULL;
			fOctree = NULL;
			octree = NULL;
		}

	// Member Functions
	private:
		void FillFaceOctree();
		void FillADFOctree();
		void InitFaceNormals();
		void InitBox(const Box3 &bbox_, int maxDepth);
	public:
		Mesh *GetMesh() const{return mesh;}
		Box3 GetBBox() const{return bbox;}
		Box3 GetRealBox() const{return realBox;}
		ADFOctree *GetADFOctreePtr () const{return octree;}
		void Init();
		#ifdef DISPLAY_MORPH_ENGINE
			void Display(GraphicsWindow *gw) const;
		#endif // DISPLAY_MORPH_ENGINE
	};
	MeshMorpher *morph1, *morph2;
	
	class InterpolatedMesh{
		friend class MorphEngine;
	private:
		Mesh *mesh;
		float interpCoeff;
	public:
		inline InterpolatedMesh(Mesh *m, float coeff):mesh(m),interpCoeff(coeff){}
		inline InterpolatedMesh():mesh(NULL),interpCoeff(-1.f){}
		inline InterpolatedMesh(const InterpolatedMesh &m):mesh(m.mesh),interpCoeff(m.interpCoeff){}
		inline ~InterpolatedMesh(){
			if (mesh) delete mesh;
		}
	};
	std::vector<InterpolatedMesh *> meshesCache;

	int version;
	EMorphingType morphingMode;

	RigidTransformation rigid;
	ElasticTransformation elastic;
	std::vector<Anchor> listOfAnchorPoints;

// ctor - dtor
public:
	explicit MorphEngine()
	{
		version = MORPH3D_ENG_VERSION;
		morphingMode = EMT_None;
		morph1 = NULL;
		morph2 = NULL;
		Init();
	}
	~MorphEngine(){
		Free();
	}
	void Free(){
		if (morph1) delete morph1;
		if (morph2) delete morph2;
		rigid = RigidTransformation();
		elastic = ElasticTransformation();
		morphingMode = EMT_None;
		listOfAnchorPoints.clear();
		for (std::vector<InterpolatedMesh *>::iterator it=meshesCache.begin();it!=meshesCache.end();++it)
			delete (*it);
		morph1 = NULL;
		morph2 = NULL;
	}
	void Init(){}

// Member Functions
private:
	void ComputeInterpolatedMesh(Mesh *&m, float coeff_morphing_);
	// void ComputeMCInCell(ADFOctree::Cell *cell, const Box3 &curBbox, std::map<SplPoint3, int>&mapOfVertices, std::vector<SplFace> &listOfFaces) const;
	void ComputeMesh(Mesh *&m, ADFOctree *octree);
	void FindMeshInCache(Mesh *&m, float coeff_morphing_) const;
	void ComputeRigidTransformation();
	void ComputeElasticTransformation();
	bool GetMorphingMesh(Mesh *&m, float coeff_morphing);
	bool GetTransformedMesh(Mesh *&m, float coeff_morphing, bool bApplyRigid, bool bApplyElastic);

public:
	Mesh *GetMesh1() const{return morph1 ? morph1->GetMesh() : NULL;}
	Mesh *GetMesh2() const{return morph2 ? morph2->GetMesh() : NULL;}
	void SetMesh1(Mesh *m, Box3 box);
	void SetMesh2(Mesh *m, Box3 box);
	bool GetResultMesh(Mesh *&m, float coeff_morphing_);
	void Clear(){
		Free();
		Init();
	}
	void SetMorphingMode(EMorphingType type);
	void AddAnchorPoint(const Point3 &p1, const Point3 &p2);
	void ValidateAnchorPoints();
	int AnchorListSize() const {return listOfAnchorPoints.size();}
	bool IsInitialized() const {return (morph1!=NULL && morph2!=NULL);}

	#ifdef DISPLAY_MORPH_ENGINE
	void Display(int index, GraphicsWindow *gw) const;
	#endif // DISPLAY_MORPH_ENGINE

// Input/Output
	IOResult Load(ILoad *iload);
	IOResult Save(ISave *isave);

// Static Instance
	static MorphEngine *Instance(){
		static MorphEngine instance;
		return &instance;
	}
};
