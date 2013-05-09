/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing object plugin for 3DSMax)

	FILE: Distance.cpp

	DESCRIPTION: Implementation of the Maths Distance Functions

	CREATED BY: Benoît Leveau

	HISTORY: 16/09/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#include "StdAfx.h"
#include "Max.h"
#include "MorphEngine.h"
#include <fstream>
#include "MemoryManager.h"
#include "MorphEngineDefines.h"
#include <algorithm>
#include "MarchingCubes.h"
#include "mesh.h"
#include "SVD/svdlib.h"
#include "WarpTransform.h"

MorphEngine::MeshMorpher::MeshMorpher(Mesh *mesh_, Box3 realBox_, int max_depth_, double min_error_, int min_faces_for_subdivide_)
{
	mesh = new Mesh(*mesh_);
	max_depth = max_depth_;
	min_error = min_error_;
	realBox = realBox_;
	bbox = mesh->getBoundingBox();
	InitBox(mesh->getBoundingBox(),max_depth_);
	min_faces = min_faces_for_subdivide_;
	octree = new ADFOctree(bbox, max_depth, min_error);
	fOctree = new FaceOctree(bbox, max_depth, min_faces_for_subdivide_);
	numFaces = mesh->getNumFaces();
	numVertices = mesh->getNumVerts();
	avgNormals.verticeNormal = NULL;
	avgNormals.faceNormal = NULL;
	avgNormals.edgeNormal.clear();
	bInit = false;
}

void MorphEngine::MeshMorpher::InitBox(const Box3 &bbox_, int maxDepth)
{
	Point3 width = bbox_.Width();
	float halfBoxSize = max(width.x, max(width.y, width.z));
	Point3 center = bbox_.Center();
	int nbCells = 2<<(maxDepth-1);
	halfBoxSize*=0.5f;
	halfBoxSize*=(float)nbCells/(float)(nbCells-1);
	Point3 newHalfWidth = Point3(halfBoxSize,halfBoxSize,halfBoxSize);
	bbox = Box3(center-newHalfWidth, center+newHalfWidth);
}

void MorphEngine::MeshMorpher::Init()
{
	/*
	InitFaceNormals();
	fOctree->Fill(mesh);
	MessageBox(0,"FaceOctree filled","Info",MB_OK);
	octree->Fill(mesh, &avgNormals, fOctree);
	MessageBox(0,"ADFOctree filled","Info",MB_OK);
	bInit = true;
	*/
}

#ifdef DISPLAY_MORPH_ENGINE
void MorphEngine::MeshMorpher::Display(GraphicsWindow *gw) const{ 
	//if (fOctree) fOctree->Display(gw);
	if (octree) octree->Display(gw);
}
#endif //DISPLAY_MORPH_ENGINE

void MorphEngine::MeshMorpher::InitFaceNormals()
{
	// compute standard, averaged, and angle weight averaged normals (per face, per edge and per vertex normals)
	if (avgNormals.verticeNormal){
		delete avgNormals.verticeNormal;
		avgNormals.verticeNormal = NULL;
	}
	if (avgNormals.faceNormal){
		delete avgNormals.faceNormal;
		avgNormals.faceNormal = NULL;
	}
	avgNormals.edgeNormal.clear();

	avgNormals.verticeNormal = new Point3[numVertices];
	avgNormals.faceNormal = new Point3[numFaces];
	for (int i=0;i<numVertices;++i)
		avgNormals.verticeNormal[i] = Point3(0,0,0);

	for (int i=0;i<numFaces;++i){
		int i0 = mesh->faces[i].getVert(0);
		int i1 = mesh->faces[i].getVert(1);
		int i2 = mesh->faces[i].getVert(2);
		Point3 p1 =	mesh->verts[mesh->faces[i].getVert(0)];
		Point3 p2 =	mesh->verts[mesh->faces[i].getVert(1)];
		Point3 p3 =	mesh->verts[mesh->faces[i].getVert(2)];
		Point3 v12 = (p2-p1).Normalize();
		Point3 v13 = (p3-p1).Normalize();
		Point3 v23 = (p3-p2).Normalize();
		Point3 n = (v12^v13).Normalize();
		avgNormals.faceNormal[i] = n;
		if (avgNormals.edgeNormal.find(AveragedNormal::PairOfPoints(i0, i1))!=avgNormals.edgeNormal.end())
			avgNormals.edgeNormal[AveragedNormal::PairOfPoints(i0, i1)] += n;
		else
			avgNormals.edgeNormal[AveragedNormal::PairOfPoints(i0, i1)] = n;
		if (avgNormals.edgeNormal.find(AveragedNormal::PairOfPoints(i1, i2))!=avgNormals.edgeNormal.end())
			avgNormals.edgeNormal[AveragedNormal::PairOfPoints(i1, i2)] += n;
		else
			avgNormals.edgeNormal[AveragedNormal::PairOfPoints(i1, i2)] = n;
		if (avgNormals.edgeNormal.find(AveragedNormal::PairOfPoints(i0, i2))!=avgNormals.edgeNormal.end())
			avgNormals.edgeNormal[AveragedNormal::PairOfPoints(i0, i2)] += n;
		else
			avgNormals.edgeNormal[AveragedNormal::PairOfPoints(i0, i2)] = n;
		avgNormals.verticeNormal[i0] += acos(v12%v13) * n;
		avgNormals.verticeNormal[i1] += acos(v12%v23) * n;
		avgNormals.verticeNormal[i2] += acos(v13%v23) * n;
	}
}

void MorphEngine::SetMesh1(Mesh *m, Box3 box)
{
	if (morph1) delete morph1;
	morph1 = new MeshMorpher(m, box, MAX_DEPTH, MIN_ERROR, MIN_FACES_FOR_SUBDIVIDE);
	morph1->Init();
}

//Mesh m_temp;

void MorphEngine::SetMesh2(Mesh *m, Box3 box)
{
	if (morph2) delete morph2;
	morph2 = new MeshMorpher(m, box, MAX_DEPTH, MIN_ERROR, MIN_FACES_FOR_SUBDIVIDE);
	morph2->Init();
//	m_temp.CopyBasics(*m);
}

void MorphEngine::FindMeshInCache(Mesh *&m, float coeff_morphing_) const
{
	std::vector<InterpolatedMesh *>::const_iterator it;
	for (it = meshesCache.begin(); it != meshesCache.end(); ++it){
		if ((*it)->interpCoeff == coeff_morphing_){
			m = (*it)->mesh;
			return;
		}
	}
	m = NULL;
}

void MorphEngine::ComputeInterpolatedMesh(Mesh *&m, float coeff_morphing_)
{
	std::vector<SplPoint3> listOfVertices;
	std::vector<SplFace> listOfFaces;
	
	//Marching Cubes Algorithm
	int todo;
}

/*
void MorphEngine::ComputeMCInCell(ADFOctree::Cell *cell, const Box3 &curBbox, std::map<SplPoint3, int>&mapOfVertices, std::vector<SplFace> &listOfFaces) const
{
	if (cell->GetChildPointer(0)){
		// node
		Box3 childBox;
		for (int i=0;i<8;++i){
			GetChildBox(curBbox, childBox, i);
			ComputeMCInCell(cell->GetChildPointer(i), childBox, mapOfVertices, listOfFaces);
		}
	}
	else{
		// leaf node	
		// First compute, the index of the cube in the MarchingCube map
		float *dist = cell->GetValue()->distances;
		int indexInMap = 0;	
		if (dist[0]>=0) indexInMap += 2;	if (dist[1]>=0) indexInMap += 1;
		if (dist[2]>=0) indexInMap += 4;	if (dist[3]>=0) indexInMap += 8;
		if (dist[4]>=0) indexInMap += 32;	if (dist[5]>=0) indexInMap += 16;
		if (dist[6]>=0) indexInMap += 64;	if (dist[7]>=0) indexInMap += 128;
		if (indexInMap!=0 && indexInMap!=255){
			// We need to create some triangles, so let's compute the mid-edges vertices
			SplPoint3 midVertices[12];
			Point3 minBox = curBbox.Min();
			Point3 maxBox = curBbox.Max();
			for (int i=0;i<12;++i)
				GetMidPoint(minBox, maxBox, midVertices[i], dist, i);
			// Now, let's add each of the triangles
			int *mapMCPtr = mapMC+15*indexInMap;
			for (int i=0;i<5;++i){
				if (*mapMCPtr==-1) break;
				int indexVertex1 = *mapMCPtr++;
				int indexVertex2 = *mapMCPtr++;
				int indexVertex3 = *mapMCPtr++;
				SplPoint3 vertex1 = midVertices[indexVertex1];
				SplPoint3 vertex2 = midVertices[indexVertex2];
				SplPoint3 vertex3 = midVertices[indexVertex3];
				int index1, index2, index3;
				index1 = index2 = index3 = -1;
				// good idea, but way too slow!!!!
				// let's see if we can find the vertices in the list
				std::map<SplPoint3, int>::iterator it = mapOfVertices.find(vertex1);
				if (it!=mapOfVertices.end()) index1 = it->second;
				it = mapOfVertices.find(vertex2);
				if (it!=mapOfVertices.end()) index2 = it->second;
				it = mapOfVertices.find(vertex3);
				if (it!=mapOfVertices.end()) index3 = it->second;				
				if (index1==-1){ // vertex not found in list, let's add it
					index1 = (int)mapOfVertices.size();
					mapOfVertices[vertex1] = index1;
				}
				if (index2==-1){ // vertex not found in list, let's add it
					index2 = (int)mapOfVertices.size();
					mapOfVertices[vertex2] = index2;
				}
				if (index3==-1){ // vertex not found in list, let's add it
					index3 = (int)mapOfVertices.size();
					mapOfVertices[vertex3] = index3;
				}
				// now, add the face to the list
				listOfFaces.push_back(SplFace(index1,index2,index3));
			}
		}
	}
}

void MorphEngine::ComputeMesh(Mesh *&m, ADFOctree *octree)
{
	std::map<SplPoint3, int> mapOfVertices;
	std::vector<SplFace> listOfFaces;
	
	//Marching Cubes Algorithm

	ComputeMCInCell(&octree->root, octree->bbox, mapOfVertices, listOfFaces);
	m = new Mesh();

	char szTmp[20];
	sprintf(szTmp,"Faces: %d",(int)listOfFaces.size());
	MessageBox(0,szTmp,"Info",MB_OK);
	sprintf(szTmp,"Vertices: %d",(int)mapOfVertices.size());
	MessageBox(0,szTmp,"Info",MB_OK);

	SplPoint3 *listOfVertices = new SplPoint3[(int)mapOfVertices.size()];
	for (std::map<SplPoint3, int>::const_iterator it=mapOfVertices.begin();it!=mapOfVertices.end();++it){
		listOfVertices[it->second].x = it->first.x;
		listOfVertices[it->second].y = it->first.y;
		listOfVertices[it->second].z = it->first.z;
	}
	
	Poly plist;
	Poly *plist_ptr = &plist;
	for (int i=0;i<listOfFaces.size();++i){
		plist_ptr->vertices[0][0] = listOfVertices[listOfFaces[i].a].x;
		plist_ptr->vertices[0][1] = listOfVertices[listOfFaces[i].a].y;
		plist_ptr->vertices[0][2] = listOfVertices[listOfFaces[i].a].z;
		plist_ptr->vertices[1][0] = listOfVertices[listOfFaces[i].b].x;
		plist_ptr->vertices[1][1] = listOfVertices[listOfFaces[i].b].y;
		plist_ptr->vertices[1][2] = listOfVertices[listOfFaces[i].b].z;
		plist_ptr->vertices[2][0] = listOfVertices[listOfFaces[i].c].x;
		plist_ptr->vertices[2][1] = listOfVertices[listOfFaces[i].c].y;
		plist_ptr->vertices[2][2] = listOfVertices[listOfFaces[i].c].z;
		if (i==listOfFaces.size()-1)
			plist_ptr->next = NULL;
		else{
			plist_ptr->next = (Poly *)malloc(sizeof(Poly));
			plist_ptr = plist_ptr->next;
		}
	}
	
	float linearEps  = 10.0f;
	float angularEps = 0.0f;
	Node *node = coplanarSets(&plist, linearEps, angularEps);

	m->setNumFaces((int)listOfFaces.size());
	m->setNumVerts((int)mapOfVertices.size());
	
	for (std::map<SplPoint3, int>::const_iterator it=mapOfVertices.begin();it!=mapOfVertices.end();++it){
		m->setVert(it->second, Point3(it->first.x,it->first.y,it->first.z));
	}

	for (int i=0;i<listOfFaces.size();++i){
		m->faces[i].setVerts(listOfFaces[i].a,listOfFaces[i].c,listOfFaces[i].b);
		m->faces[i].setEdgeVisFlags(1,0,1);
		m->faces[i].setSmGroup(0);
	}
}
*/

void MorphEngine::ComputeMesh(Mesh *&m, ADFOctree *octree)
{
	//Marching Cubes Algorithm + Optimization of the faces
	MarchingCube MC;
	MC.GetMeshFromOctree(&octree->root, octree->bbox, m);
}


#ifdef DISPLAY_MORPH_ENGINE
Mesh *m_temp = NULL;
void MorphEngine::Display(int index, GraphicsWindow *gw) const
{
	if (index==0 && morph1) morph1->Display(gw);
	if (index==1 && morph2) morph2->Display(gw);
	if (index==2 && m_temp){
		for (int i=0;i<m_temp->getNumFaces();++i){
			Point3 p0 = m_temp->verts[m_temp->faces[i].getVert(0)];
			Point3 p1 = m_temp->verts[m_temp->faces[i].getVert(1)];
			Point3 p2 = m_temp->verts[m_temp->faces[i].getVert(2)];
			Point3 seg[2];
			seg[0] = p0; seg[1] = p1; gw->segment(seg, 1);
			seg[0] = p1; seg[1] = p2; gw->segment(seg, 1);
			seg[0] = p2; seg[1] = p0; gw->segment(seg, 1);
		}
	}
}
#endif //DISPLAY_MORPH_ENGINE

bool MorphEngine::GetResultMesh(Mesh *&m, float coeff_morphing)
{
	if (morphingMode == EMT_None)
		return false;
	else if (morphingMode == EMT_RigidOnly)
		return GetTransformedMesh(m, coeff_morphing, true, false);
	else if (morphingMode == EMT_ElasticOnly)
		return GetTransformedMesh(m, coeff_morphing, false, true);
	else if (morphingMode == EMT_RigidElastic)
		return GetTransformedMesh(m, coeff_morphing, true, true);
	else if (morphingMode == EMT_Morphing)
		return GetMorphingMesh(m, coeff_morphing);
	return false;
}

bool MorphEngine::GetMorphingMesh(Mesh *&m, float coeff_morphing)
{
	// <--temp for marching cube debug
#ifdef DISPLAY_MORPH_ENGINE
	if (m_temp==NULL)
		ComputeMesh(m_temp, morph1->GetADFOctreePtr());
	static Mesh *m_to_return = new Mesh();
	m = m_to_return;
	return true;
#endif //DISPLAY_MORPH_ENGINE
	// temp for marching cube debug-->

	int todo_remove_temp_code;
	FindMeshInCache(m, coeff_morphing); // temp
	if (m) return true; // temp
	if (coeff_morphing == 0.f){
		if (morph1){
			// m = morph1->GetMesh(); // temp
			ComputeMesh(m, morph1->GetADFOctreePtr()); // temp
			meshesCache.push_back(new InterpolatedMesh(m, coeff_morphing)); // temp
		}
	}
	else if (coeff_morphing == 1.f){
		if (morph2){
			// m = morph2->GetMesh(); // temp
			ComputeMesh(m, morph2->GetADFOctreePtr()); // temp
			meshesCache.push_back(new InterpolatedMesh(m, coeff_morphing)); // temp
		}
	}
	else if (morph1 && morph2){
		FindMeshInCache(m, coeff_morphing);
		if (!m){
			ComputeInterpolatedMesh(m, coeff_morphing);
			if (m) 
				meshesCache.push_back(new InterpolatedMesh(m, coeff_morphing));
		}
	}
	return (m!=NULL);
}

void MorphEngine::ComputeRigidTransformation()
{
//	Mesh *m1 = morph1->GetMesh();
//	Mesh *m2 = morph2->GetMesh();
//	m1->buildBoundingBox();
//	m2->buildBoundingBox();
//	Box3 b1 = m1->getBoundingBox();
//	Box3 b2 = m2->getBoundingBox();
//	rigid.OT = b2.Center() - b1.Center();
	rigid.OT = morph2->GetRealBox().Center() - morph1->GetRealBox().Center();
	rigid.origin = morph1->GetRealBox().Center();

	std::vector<Anchor> listOfModifiedAnchorPoints(listOfAnchorPoints);
	for (std::vector<Anchor>::iterator it=listOfModifiedAnchorPoints.begin(); it!=listOfModifiedAnchorPoints.end(); ++it){
		(*it).s -= morph1->GetRealBox().Center(); //
		(*it).t -= morph2->GetRealBox().Center(); //rigid.OT;
	}

	// compute the center of each ensemble of points
	Point3 center_s(0,0,0), center_t(0,0,0);
	for (std::vector<Anchor>::const_iterator it=listOfModifiedAnchorPoints.begin(); it!=listOfModifiedAnchorPoints.end(); ++it){
		center_s += (*it).s;
		center_t += (*it).t;
	}
	center_s /= listOfModifiedAnchorPoints.size();
	center_t /= listOfModifiedAnchorPoints.size();

	// now construct a matrix that is the sum of all (PointSource-CenterSource)*(PointTarget-CenterTarget)
	DMat matrix = svdNewDMat(3,3);
	for (std::vector<Anchor>::const_iterator it=listOfModifiedAnchorPoints.begin(); it!=listOfModifiedAnchorPoints.end(); ++it){
		Point3 t = ((*it).t-center_t);
		Point3 s = ((*it).s-center_s);
		matrix->value[0][0] += t.x * s.x;	matrix->value[0][1] += t.x * s.y;	matrix->value[0][2] += t.x * s.z;	
		matrix->value[1][0] += t.y * s.x;	matrix->value[1][1] += t.y * s.y;	matrix->value[1][2] += t.y * s.z;	
		matrix->value[2][0] += t.z * s.x;	matrix->value[2][1] += t.z * s.y;	matrix->value[2][2] += t.z * s.z;	
	}
	SMat spareMatrix = svdConvertDtoS(matrix);

	// Compute the Singular Value Decomposition of the matrix
	SVDRec svdResult = svdLAS2A(spareMatrix, 0);

	DMat V = svdTransposeD(svdResult->Vt);
	DMat Ut = svdResult->Ut;
	
	Matrix3 mV(	Point3(V->value[0][0], V->value[0][1], V->value[0][2]),
				Point3(V->value[1][0], V->value[1][1], V->value[1][2]),
				Point3(V->value[2][0], V->value[2][1], V->value[2][2]),
				Point3(0,0,0));

	Matrix3 mUt(Point3(Ut->value[0][0], Ut->value[0][1], Ut->value[0][2]),
				Point3(Ut->value[1][0], Ut->value[1][1], Ut->value[1][2]),
				Point3(Ut->value[2][0], Ut->value[2][1], Ut->value[2][2]),
				Point3(0,0,0));

	// Compute the Rotation matrix of the rigid transformation
	rigid.R = mV*mUt;
	Quat q(rigid.R);
	q.w *= -1.0f;
	q.MakeMatrix(rigid.R);

	// Compute the Translation vector of the rigid transformation
	rigid.T = center_t - rigid.R * center_s;
}

void MorphEngine::ComputeElasticTransformation()
{
	int todo;
}

bool MorphEngine::GetTransformedMesh(Mesh *&mesh, float coeff_morphing, bool bApplyRigid, bool bApplyElastic)
{
	if (!morph1)
		return false;

	mesh = new Mesh(*morph1->GetMesh());
	if (coeff_morphing == 0.f)
		return true;
	
	RigidTransformation rigid_interp(rigid);
	rigid_interp.Interpolate(coeff_morphing);

	for (int i=0; i<mesh->numVerts; ++i){
		if (bApplyRigid)
			mesh->verts[i] = rigid_interp(mesh->verts[i]);
	}
	return true;
}

void MorphEngine::AddAnchorPoint(const Point3 &p1, const Point3 &p2)
{
	Anchor anchor(p1, p2);
	listOfAnchorPoints.push_back(anchor);
}

void MorphEngine::SetMorphingMode(EMorphingType type)
{
	morphingMode = type;
}

void MorphEngine::ValidateAnchorPoints()
{
	ComputeRigidTransformation();
	ComputeElasticTransformation();
}

IOResult MorphEngine::Save(ISave *isave)
{
	ULONG nbWritten;
	isave->BeginChunk (MORPHENGINE_VERSION_CHUNK);
	if (isave->Write(&version, sizeof(int), &nbWritten)!=IO_OK) goto IOError;
	isave->EndChunk();

	isave->BeginChunk (MORPHENGINE_MPHTYPE_CHUNK);
	if (isave->Write(&morphingMode, sizeof(EMorphingType), &nbWritten)!=IO_OK) goto IOError;
	isave->EndChunk();

	isave->BeginChunk (MORPHENGINE_RIGID_CHUNK);
	if (rigid.Save(isave)!=IO_OK) goto IOError;
	isave->EndChunk();

	isave->BeginChunk (MORPHENGINE_ELASTIC_CHUNK);
	if (elastic.Save(isave)!=IO_OK) goto IOError;
	isave->EndChunk();

	isave->BeginChunk (MORPHENGINE_ANCHORLIST_CHUNK);
	if (SaveVector(isave, listOfAnchorPoints)!=IO_OK) goto IOError;
	isave->EndChunk();

	return IO_OK;

IOError:
	isave->EndChunk();
	return IO_ERROR;
}

IOResult MorphEngine::Load(ILoad *iload)
{
	ULONG nbRead;
	IOResult res = IO_OK;
	while (IO_OK==(res=iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
			case MORPHENGINE_VERSION_CHUNK:
				res = iload->Read(&version,sizeof(int),&nbRead);
				break;
			case MORPHENGINE_MPHTYPE_CHUNK:
				res = iload->Read(&morphingMode,sizeof(EMorphingType),&nbRead);
				break;
			case MORPHENGINE_RIGID_CHUNK:
				res = rigid.Load(iload);
				break;
			case MORPHENGINE_ELASTIC_CHUNK:
				res = elastic.Load(iload);
				break;
			case MORPHENGINE_ANCHORLIST_CHUNK:
				res = LoadVector(iload, listOfAnchorPoints);
				break;
		}
		iload->CloseChunk();
		if (res!=IO_OK)  return res;
	}
	return IO_OK;
}
