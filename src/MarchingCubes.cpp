/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing object plugin for 3DSMax)

	FILE: MarchingCubes.cpp

	DESCRIPTION: Implementation of the Marching Cubes algorithm

	CREATED BY: Benoît Leveau

	HISTORY: 01/10/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#include "StdAfx.h"
#include "ADFOctree.h"
#include "MorphEngine.h"
#include "MorphEngineDefines.h"
#include "MarchingCubes.h"
#include "MarchingCubesMap.h"
#include "PlaneSets.h"
#include <algorithm>
#include <list>
#include <deque>

namespace{
	template <class T> struct MyEdge
	{
		T a;
		T b;
		inline MyEdge(){}
		inline MyEdge(const MyEdge<T> &edge):a(edge.a),b(edge.b){}
		inline MyEdge(const T &a_, const T &b_, bool bSwap=false):a(a_),b(b_){
			if (bSwap && a<b) std::swap(a,b);
		}
		inline bool operator==(const MyEdge &e){
			return (a==e.a && b==e.b);
		}
		inline bool operator<(const MyEdge &e){
			if (a<e.a) return true;
			else if (a==e.a){
				if (b<e.b) return true;
				else return false;
			}
			else
				return false;
		}
	};

	typedef MyEdge<SplPoint3> MyEdge3D;
	typedef MyEdge<SplPoint2> MyEdge2D;

	#define INTERP(C,a,b) (minBox.##C + abs(dist[a]/(dist[b]-dist[a]))*(maxBox.##C-minBox.##C))
	inline void GetMidPoint(const Point3 &minBox, const Point3 &maxBox, SplPoint3 &p, float *dist, int i)
	{
		switch (i){
			case 0:		p.x = INTERP(x,0,1);	p.y = minBox.y;			p.z = minBox.z;			break;
			case 1:		p.x = minBox.x;			p.y = INTERP(y,0,2);	p.z = minBox.z;			break;
			case 2:		p.x = INTERP(x,2,3);	p.y = maxBox.y;			p.z = minBox.z;			break;
			case 3:		p.x = maxBox.x;			p.y = INTERP(y,1,3);	p.z = minBox.z;			break;
			case 4:		p.x = INTERP(x,4,5);	p.y = minBox.y;			p.z = maxBox.z;			break;
			case 5:		p.x = minBox.x;			p.y = INTERP(y,4,6);	p.z = maxBox.z;			break;
			case 6:		p.x = INTERP(x,6,7);	p.y = maxBox.y;			p.z = maxBox.z;			break;
			case 7:		p.x = maxBox.x;			p.y = INTERP(y,5,7);	p.z = maxBox.z;			break;
			case 8:		p.x = maxBox.x;			p.y = minBox.y;			p.z = INTERP(z,1,5);	break;
			case 9:		p.x = minBox.x;			p.y = minBox.y;			p.z = INTERP(z,0,4);	break;
			case 10:	p.x = maxBox.x;			p.y = maxBox.y;			p.z	= INTERP(z,3,7);	break;
			case 11:	p.x = minBox.x;			p.y = maxBox.y;			p.z = INTERP(z,2,6);	break;
			default: ASSERT(false);
		}
	}

	/* TRUE iff A and B have same signs. */
	#define SAME_SIGNS(A, B) (((long)((unsigned long)A ^ (unsigned long)B)) >= 0)

	/* Return the max value, storing the minimum value in min */
	#define  maxmin(x1, x2, min) (x1 >= x2 ? (min = x2, x1) : (min = x1, x2))

	bool SegIntersect(const SplPoint2 &p1, const SplPoint2 &p2, const SplPoint2 &q1, const SplPoint2 &q2)
	{
		long a, b, c, d;				/* parameter calculation variables */
		short max1, max2, min1, min2;	/* bounding box check variables */

		/*  First make the bounding box test. */
		max1 = maxmin(p1.x, p2.x, min1);
		max2 = maxmin(q1.x, q2.x, min2);
		if((max1 < min2) || (min1 > max2)) return false; /* no intersection */
		max1 = maxmin(p1.y, p2.y, min1);
		max2 = maxmin(q1.y, q2.y, min2);
		if((max1 < min2) || (min1 > max2)) return false; /* no intersection */

		/* See if the endpoints of the second segment lie on the opposite
		sides of the first.  If not, return 0. */
		a = (long)(q1.x - p1.x) * (long)(p2.y - p1.y) -
			(long)(q1.y - p1.y) * (long)(p2.x - p1.x);
		b = (long)(q2.x - p1.x) * (long)(p2.y - p1.y) -
			(long)(q2.y - p1.y) * (long)(p2.x - p1.x);
		if(a!=0 && b!=0 && SAME_SIGNS(a, b)) return false;

		/* See if the endpoints of the first segment lie on the opposite
		sides of the second.  If not, return 0.  */
		c = (long)(p1.x - q1.x) * (long)(q2.y - q1.y) -
			(long)(p1.y - q1.y) * (long)(q2.x - q1.x);
		d = (long)(p2.x - q1.x) * (long)(q2.y - q1.y) -
			(long)(p2.y - q1.y) * (long)(q2.x - q1.x);
		if(c!=0 && d!=0 && SAME_SIGNS(c, d) ) return false;

		// At this point each segment meets the line of the other.
		// det = a - b;
		// (det == 0) => The segments are colinear.
		return true;
	}
}

//extern Mesh m_temp;

/*
void RecAddNodes(std::vector<Node *> &listOfNodes, Node *node)
{
	listOfNodes.push_back(node);
	if (node->right)
		RecAddNodes(listOfNodes, node->right);
	if (node->left)
		RecAddNodes(listOfNodes, node->left);
}

void RecCountNodes(int &cpt, Node *node)
{
	++cpt;
	if (node->right)
		RecCountNodes(cpt, node->right);
	if (node->left)
		RecCountNodes(cpt, node->left);
}
*/

void MarchingCube::GetMeshFromOctree(ADFOctree::Cell *root, const Box3 &curBbox, Mesh *&mesh)
{
	Poly plist;
	Poly *plist_ptr = &plist;

	// <----- uncomment after test
	ComputeMCInCell(root, curBbox, plist_ptr);
	// ------------>
	
	// <------- remove when done
	// test mesh
	//for (int i=0;i<m_temp.getNumFaces();++i){
	//	Face &f = m_temp.faces[i];
	//	Point3 &p0 = m_temp.verts[f.getVert(0)];
	//	Point3 &p2 = m_temp.verts[f.getVert(1)];
	//	Point3 &p1 = m_temp.verts[f.getVert(2)];
	//	plist_ptr->vertices[0][0] = p0.x;	plist_ptr->vertices[0][1] = p0.y;	plist_ptr->vertices[0][2] = p0.z;
	//	plist_ptr->vertices[1][0] = p1.x;	plist_ptr->vertices[1][1] = p1.y;	plist_ptr->vertices[1][2] = p1.z;
	//	plist_ptr->vertices[2][0] = p2.x;	plist_ptr->vertices[2][1] = p2.y;	plist_ptr->vertices[2][2] = p2.z;
	//	plist_ptr->next = (Poly *)malloc(sizeof(Poly));
	//	plist_ptr = plist_ptr->next;
	//}
	// ------------->

	Poly *p = &plist;
	while (p->next!=plist_ptr) p=p->next;
	p->next = NULL;
	delete plist_ptr;

	std::vector<Node *> listOfNodes;

	float linearEps  = 1e8f;
	float angularEps = 0.0f;
	Node *tree = coplanarSets(listOfNodes, &plist, linearEps, angularEps);

	Poly plist2;
	Poly *plist2_ptr = &plist2;

	//	// first version, recursively browsing the tree and adding the polygons
	//	ComputeTree(tree, plist2_ptr);
	
	//	// second version, recursively browsing the tree to add the nodes to a vector
	//	// then, we process each node by iterating through the vector
	//	std::vector<Node *> listOfNodes;
	//	RecAddNodes(listOfNodes, tree);
	//	for (std::vector<Node *>::iterator it=listOfNodes.begin();it!=listOfNodes.end();++it)
	//		ComputeTree(*it, plist2_ptr, false);
	
	// third version passes the listOfNodes vector directly to the coplanarSets() function so
	// the vector is filled during generation of the tree
	for (std::vector<Node *>::iterator it=listOfNodes.begin();it!=listOfNodes.end();++it)
		ComputeTree(*it, plist2_ptr, false);

	p = &plist2;
	while (p->next!=plist2_ptr) p=p->next;
	p->next = NULL;
	delete plist2_ptr;

	std::map<SplPoint3, int> mapOfVertices;
	Poly *ptr = &plist2;
	int nbOfPoints = 0;
	int nbOfFaces = 0;
	while(ptr!=NULL){
		for (int i=0;i<3;++i){
			int curIndex;
			SplPoint3 pt(ptr->vertices[i][0],ptr->vertices[i][1],ptr->vertices[i][2]);
			std::map<SplPoint3, int>::const_iterator it = mapOfVertices.find(pt);
			if (it==mapOfVertices.end()){
				curIndex = nbOfPoints;
				mapOfVertices[pt] = nbOfPoints++;
			}
			else
				curIndex = it->second;
			ptr->vertices[i][0] = curIndex;
		}
		++nbOfFaces;
		ptr = ptr->next;
	}

	mesh = new Mesh();
	mesh->setNumVerts(nbOfPoints);
	mesh->setNumFaces(nbOfFaces);

	for (std::map<SplPoint3, int>::const_iterator it=mapOfVertices.begin();it!=mapOfVertices.end();++it){
		mesh->setVert(it->second, it->first.x, it->first.y, it->first.z);
	}

	ptr = &plist2;
	nbOfFaces = 0;
	while(ptr!=NULL){
		mesh->faces[nbOfFaces].setVerts((int)(ptr->vertices[0][0]),(int)(ptr->vertices[2][0]),(int)(ptr->vertices[1][0]));
		mesh->faces[nbOfFaces].setEdgeVisFlags(1,1,1);
		mesh->faces[nbOfFaces].setSmGroup(1);
		++nbOfFaces;
		ptr = ptr->next;
	}
}

#define FILL_EDGE(arg1, arg2) \
	elist.push_back(MyEdge3D(SplPoint3(pcurList->vertices[arg1]),SplPoint3(pcurList->vertices[arg2]),true));

#define EQUAL(a, b, c)	\
	(a == b && b == c && a == c)

#define INF 1e15

#define ALIGNED(a, b, c) \
	((a==b || a==c || b==c) || EQUAL((((b.x-a.x)!=0) ? (c.x-a.x)/(b.x-a.x) : INF), (((b.y-a.y)!=0) ? (c.y-a.y)/(b.y-a.y) : INF), (((b.z-a.z)!=0) ? (c.z-a.z)/(b.z-a.z) : INF)))

inline bool Aligned(const SplPoint3 &a, const SplPoint3 &b, const SplPoint3 &c, double eps=0.0f)
{
	double Dx1 = b.x - a.x;
	double Dy1 = b.y - a.y;
	double Dz1 = b.z - a.z;

	double Dx2 = c.x - a.x;
	double Dy2 = c.y - a.y;
	double Dz2 = c.z - a.z;

	double Cx = Dy1 * Dz2 - Dy2 * Dz1;
	double Cy = Dx2 * Dz1 - Dx1 * Dz2;
	double Cz = Dx1 * Dy2 - Dx2 * Dy1;

	return (abs(Cx*Cx + Cy*Cy + Cz*Cz) <= eps);
}


#define DEF_AND_FILL_BOX(listOfPoints, pMin, pMax)		\
	SplPoint3 pMin(1e8,1e8,1e8), pMax(-1e8,-1e8,-1e8);	\
	for (std::vector<SplPoint3>::const_iterator itl=listOfPoints.begin();itl!=listOfPoints.end();++itl){	\
		if (pMin.x > (*itl).x) pMin.x = (*itl).x;		\
		if (pMin.y > (*itl).y) pMin.y = (*itl).y;		\
		if (pMin.z > (*itl).z) pMin.z = (*itl).z;		\
		if (pMax.x < (*itl).x) pMax.x = (*itl).x;		\
		if (pMax.y < (*itl).y) pMax.y = (*itl).y;		\
		if (pMax.z < (*itl).z) pMax.z = (*itl).z;		\
	}

bool bTest = false;
bool bTest2 = true;

inline bool AlmostEqual(SplPoint3 a, SplPoint3 b){
	return (abs(a.x-b.x)<1e-5 && abs(a.y-b.y)<1e-5 && abs(a.z-b.z)<1e-5);
}

void MarchingCube::ComputeTree(Node *node, Poly *&plist_ptr, bool bRecursive/*=true*/) const
{
	// node is the current node considered
	// plist_ptr is the global list of polygons, add each polygon created to this list

	Poly *pcurList = node->plist; // the current of points list 
	if (bTest || pcurList->next==NULL || pcurList->next->next==NULL){
		// only one or two faces, no need to optimize this set, add it directly to the list...
		while (pcurList && bTest2){
			Point3 normal(node->plane[0],node->plane[1],node->plane[2]);
			Point3 p0(pcurList->vertices[0][0], pcurList->vertices[0][1], pcurList->vertices[0][2]);
			Point3 p1(pcurList->vertices[1][0], pcurList->vertices[1][1], pcurList->vertices[1][2]);
			Point3 p2(pcurList->vertices[2][0], pcurList->vertices[2][1], pcurList->vertices[2][2]);
			plist_ptr->vertices[0][0] =	p0.x;	plist_ptr->vertices[0][1] = p0.y;	plist_ptr->vertices[0][2] =	p0.z;
			if (normal%((p1-p0)^(p2-p0))<0){
				plist_ptr->vertices[1][0] =	p2.x;	plist_ptr->vertices[1][1] = p2.y;	plist_ptr->vertices[1][2] =	p2.z;
				plist_ptr->vertices[2][0] =	p1.x;	plist_ptr->vertices[2][1] = p1.y;	plist_ptr->vertices[2][2] =	p1.z;
			}
			else{
				plist_ptr->vertices[1][0] =	p1.x;	plist_ptr->vertices[1][1] = p1.y;	plist_ptr->vertices[1][2] =	p1.z;
				plist_ptr->vertices[2][0] =	p2.x;	plist_ptr->vertices[2][1] = p2.y;	plist_ptr->vertices[2][2] =	p2.z;
			}
			pcurList = pcurList->next;
			plist_ptr->next = new Poly();
			plist_ptr = plist_ptr->next;	
		}
	}
	else{
		// now the tricky part, we need to process the polygons to optimize the faces...

		int nbOfEdges = 0;
		Poly *pCtrList = pcurList;
		while (pCtrList){
			++nbOfEdges;
			pCtrList = pCtrList->next;
		}

		std::vector<MyEdge3D> elist;
		elist.reserve(nbOfEdges);
		
		while (pcurList){	
			FILL_EDGE(0,1);
			FILL_EDGE(0,2);
			FILL_EDGE(1,2);
			pcurList = pcurList->next;
		}
		// now we have elist = list of all edges 

		// sort the list of edges
		std::sort(elist.begin(), elist.end());
		
		// if two consecutive edges in the list are identical, it means this edge can be removed
		// as it means that this edge is shared by two polygons (keep only the edges which appear once)
		std::deque<MyEdge3D> elist2;
		std::vector<MyEdge3D>::iterator it = elist.begin();
		for (it=elist.begin();it!=elist.end();++it){
			if ((it+1)!=elist.end()){
				if ((*it)==(*(it+1))){
					++it;
					continue;
				}
			}
			elist2.push_back(*it);
		}
		elist.clear();// we can clear this list as it's useless now
		
		// now we need to remove asymetric identical edges (AB and AC with ABC colinear ==> keep only BC)

		bool bFound = true;
		int nOldIndex = 0;
		while (bFound)
		{
			std::deque<MyEdge3D>::iterator it;
			std::deque<MyEdge3D>::iterator it2;
			bFound = false;
			int nIndex = 0;
			for (it=elist2.begin();it!=elist2.end();++it){
				if (nIndex<nOldIndex) continue;
				++nIndex;
				for (it2=it+1;it2!=elist2.end();++it2){
					if (AlmostEqual((*it).a, (*it2).a) && AlmostEqual((*it).b, (*it2).b)){
						bFound = true;
						std::swap(*it2, elist2.back());
						elist2.pop_back();
						std::swap(*it, elist2.back());
						elist2.pop_back();
					}
					else if (AlmostEqual((*it).a, (*it2).b) && AlmostEqual((*it).b, (*it2).a)){
						ASSERT(false);
						bFound = true;
						std::swap(*it2, elist2.back());
						elist2.pop_back();
						std::swap(*it, elist2.back());
						elist2.pop_back();
					}
					else if ((*it).a==(*it2).a){
						if (Aligned((*it).a,(*it).b,(*it2).b,1e-5)){
							bFound = true;
							MyEdge3D edge((*it).b,(*it2).b, true);
							std::swap(*it2, elist2.back());
							elist2.pop_back();
							std::swap(*it, elist2.back());
							elist2.pop_back();
							elist2.push_back(edge);
						}
					}
					else if ((*it).b==(*it2).b){
						if (Aligned((*it).a,(*it2).a,(*it).b,1e-5)){
							bFound = true;
							MyEdge3D edge((*it).a,(*it2).a, true);
							std::swap(*it2, elist2.back());
							elist2.pop_back();
							std::swap(*it, elist2.back());
							elist2.pop_back();
							elist2.push_back(edge);
						}
					}
					else if ((*it).a==(*it2).b){
						if (Aligned((*it).a,(*it2).a,(*it).b,1e-5)){
							bFound = true;
							MyEdge3D edge ((*it).b,(*it2).a, true);
							std::swap(*it2, elist2.back());
							elist2.pop_back();
							std::swap(*it, elist2.back());
							elist2.pop_back();
							elist2.push_back(edge);
						}
					}
					else if ((*it).b==(*it2).a){
						if (Aligned((*it).a,(*it).b,(*it2).b,1e-5)){
							bFound = true;
							MyEdge3D edge((*it).a,(*it2).b, true);
							std::swap(*it2, elist2.back());
							elist2.pop_back();
							std::swap(*it, elist2.back());
							elist2.pop_back();
							elist2.push_back(edge);
						}
					}
					if (bFound) break;
				}
				if (bFound){
					nOldIndex = nIndex;
					break;
				}
			}
		}
	
		// now create separate polygons, and then delete collinear vertices in each polygon
		std::deque<std::vector<SplPoint3>> listOfPolygons;
		bool *bEdgeAdded = new bool[elist2.size()];
		for (int i=0;i<elist2.size();++i) bEdgeAdded[i] = false;
		for (int i=0;i<elist2.size()-2;++i){
			if (bEdgeAdded[i]) continue;
			std::vector<SplPoint3> polyList;
			polyList.push_back(elist2[i].a);
			polyList.push_back(elist2[i].b);
			SplPoint3 toSearch(elist2[i].b);
			SplPoint3 toSearchEnd(elist2[i].a);
			bool bFound = true;
			int j=i+1;
			while (bFound){
				bFound = false;
				for (j=i+1;j<elist2.size();++j){
					if (bEdgeAdded[j]) continue;
					if (AlmostEqual(elist2[j].a, toSearch)){
						toSearch = elist2[j].b;
						bFound = true;
					}
					else if (AlmostEqual(elist2[j].b, toSearch)){
						toSearch = elist2[j].a;
						bFound = true;
					}
					if (bFound){
						if (AlmostEqual(toSearch, toSearchEnd))
							bFound = false;
						else
							polyList.push_back(toSearch);
						bEdgeAdded[j] = true;
						j = i;
						break;
					}
				}
			}
			
			if (polyList.size()<3)
				continue;

			// now we have a polygon in poly list, so let's delete collinear vertices

			std::vector<SplPoint3> polyList_col;
			std::vector<SplPoint3>::iterator it1 = polyList.begin();
			
			SplPoint3 p0(*it1++);
			polyList_col.push_back(p0);
			
			SplPoint3 p1(*it1++);
			polyList_col.push_back(p1);

			//it1 now contains 3rd element of polyList

			// browse the list to delete collinear vertices
			for (;it1!=polyList.end();++it1){
				if (Aligned(p0, p1, (*it1),1e-5))
					polyList_col.pop_back();
				else
					p0 = p1;
				p1 = *it1;	
				polyList_col.push_back(*it1);
			}

			if (polyList_col.size()<3)
				continue;

			// check to see if last element is between the previous one and the first in the list
			p1 = polyList_col.back();
			SplPoint3 p2 = polyList_col.front();
			if (Aligned(p0, p1, p2,1e-5))
				polyList_col.pop_back();

			if (polyList_col.size()<3)
				continue;

			// check to see if first element is between the last one and the second in the list
			p0 = polyList_col.back();
			p1 = polyList_col.front();
			p2 = *(polyList_col.begin()+1);
			if (Aligned(p0, p1, p2,1e-5)){
				std::swap(*(polyList_col.begin()), polyList_col.back());
				polyList_col.pop_back();
			}

			// add this polygon to a list of polygon, so later we can detect holes!
			if (polyList_col.size()<3)
				continue;
			else
				listOfPolygons.push_back(polyList_col);
		}
	
		std::vector<std::vector<std::vector<SplPoint3>>> listOfPolygonsWithHoles;
		{
			// detect holes by browsing through the list of polygons
			std::deque<std::vector<SplPoint3>>::iterator it = listOfPolygons.begin();
			std::deque<std::vector<SplPoint3>>::iterator it2;
			while (it!=listOfPolygons.end()){
				std::vector<std::vector<SplPoint3>> aPolygonWithHoles;
				aPolygonWithHoles.push_back(*it);
				DEF_AND_FILL_BOX((*it), pMin, pMax); // define pMin and pMax as the bounding box of *it
				it2 = it+1;
				while (it2!=listOfPolygons.end()){
					DEF_AND_FILL_BOX((*it2), pMin2, pMax2);	// define pMin2 and pMax2 as the bounding box of *it2
					if (pMin2.x>pMin.x && pMin2.x<pMax.x && 
						pMin2.y>pMin.y && pMin2.y<pMax.y && 
						pMin2.z>pMin.z && pMin2.z<pMax.z && 
						pMax2.x>pMin.x && pMax2.x<pMax.x && 
						pMax2.y>pMin.y && pMax2.y<pMax.y && 
						pMax2.z>pMin.z && pMax2.z<pMax.z)
					{
						aPolygonWithHoles.push_back(*it2);
						std::swap(*it2,listOfPolygons.back());
						listOfPolygons.pop_back();
					}
					else
						++it2;
				}
				++it;
				listOfPolygonsWithHoles.push_back(aPolygonWithHoles);
			}
		}
		
		// then triangulate each polygon
		for (std::vector<std::vector<std::vector<SplPoint3>>>::iterator itc = listOfPolygonsWithHoles.begin(); itc!=listOfPolygonsWithHoles.end(); ++itc){
			if ((*itc).size()==1 || DONT_DETECT_HOLES){
				// simple polygon without holes

			//std::vector<SplPoint3> &curPoly = itc->front();
			for (std::vector<std::vector<SplPoint3>>::iterator itc2 = (*itc).begin(); itc2!=(*itc).end(); ++itc2){
				std::vector<SplPoint3> &curPoly = *itc2;

				std::vector<SplPoint2> curPoly2D;
				for (std::vector<SplPoint3>::iterator itl = curPoly.begin(); itl != curPoly.end(); ++itl){
					float coeff = (*itl).z / node->plane[2];
					curPoly2D.push_back(SplPoint2((*itl).x - coeff * node->plane[0], (*itl).y - coeff * node->plane[1]));
				}

				// fill a list containing all the 2D edges in the polygon (projected on the plane determined by the polygon itself)
				std::vector<MyEdge2D> listOf2DEdges;
				for (std::vector<SplPoint2>::iterator itl = curPoly2D.begin(); itl != curPoly2D.end(); ++itl){
					std::vector<SplPoint2>::iterator itl2 = itl+1;
					if (itl2==curPoly2D.end()){
						listOf2DEdges.push_back(MyEdge2D(*itl, *curPoly2D.begin()));
						break;
					}
					else
						listOf2DEdges.push_back(MyEdge2D(*itl, *itl2));
				}

				int index = 0;
				size_t size;
				int nLoop = 0;
				while((size=curPoly2D.size())>2){
					// prevent infinite loops in collinear edges
					if (nLoop>2*size) break;

					// 'index' is the current vertex, let's consider the triangle formed by 'index'-'index+1'-'index+2'
					// analyze the edge 'index'-'index+2'
					SplPoint2 &pt0	= curPoly2D[index%size];
					SplPoint2 &pt1	= curPoly2D[(index+1)%size];
					SplPoint2 &pt2	= curPoly2D[(index+2)%size];
					// MyEdge2D edge(pt0,pt2);
					
					bool bValidTriangle = true;
					if (curPoly2D.size()>3){
						// is this edge intersecting a boundary edge of the polygon?
						for (std::vector<MyEdge2D>::iterator ite = listOf2DEdges.begin(); ite != listOf2DEdges.end(); ++ite){
							if ((*ite).a == pt0) continue;	if ((*ite).a == pt1) continue;	if ((*ite).a == pt2) continue;
							if ((*ite).b == pt0) continue;	if ((*ite).b == pt1) continue;	if ((*ite).b == pt2) continue;
							if (SegIntersect((*ite).a, (*ite).b, pt0, pt2)) {bValidTriangle = false; break;}
						}
						if (!bValidTriangle){++nLoop; ++index; continue;}

						// is this edge is contained by the polygon?
						// we'll just test the mid-point of the edge, if this point is in the polygon, the whole edge
						// must be in the polygon (or else another segment is intersecting the edge, which we've just checked
						// in the previous test)
						float x = 0.5f * (pt0.x + pt2.x);
						float y = 0.5f * (pt0.y + pt2.y);
						bool odd=false;
						for (std::vector<MyEdge2D>::iterator ite = listOf2DEdges.begin(); ite != listOf2DEdges.end(); ++ite){
							if (((*ite).a == pt0 && (*ite).b == pt2)||((*ite).a == pt2 && (*ite).b == pt0)) continue;
							if ((*ite).a.y<y && (*ite).b.y>=y || (*ite).b.y<y && (*ite).a.y>=y) {
								if ((*ite).a.x+(y-(*ite).a.y)/((*ite).b.y-(*ite).a.y)*((*ite).b.x-(*ite).a.x)<x){
									odd=!odd; 
								}
							}
						}
						bValidTriangle = odd || (listOf2DEdges.size()==0);
					}

					if (bValidTriangle){
						// create a polygon
						Point3 normal(node->plane[0],node->plane[1],node->plane[2]);
						Point3 p0(curPoly[index%size].x, curPoly[index%size].y, curPoly[index%size].z);
						Point3 p1(curPoly[(index+1)%size].x, curPoly[(index+1)%size].y, curPoly[(index+1)%size].z);
						Point3 p2(curPoly[(index+2)%size].x, curPoly[(index+2)%size].y, curPoly[(index+2)%size].z);
						plist_ptr->vertices[0][0] =	p0.x;	plist_ptr->vertices[0][1] = p0.y;	plist_ptr->vertices[0][2] =	p0.z;
						if (normal%((p1-p0)^(p2-p0))<0){
							plist_ptr->vertices[1][0] =	p2.x;	plist_ptr->vertices[1][1] = p2.y;	plist_ptr->vertices[1][2] =	p2.z;
							plist_ptr->vertices[2][0] =	p1.x;	plist_ptr->vertices[2][1] = p1.y;	plist_ptr->vertices[2][2] =	p1.z;
						}
						else{
							plist_ptr->vertices[1][0] =	p1.x;	plist_ptr->vertices[1][1] = p1.y;	plist_ptr->vertices[1][2] =	p1.z;
							plist_ptr->vertices[2][0] =	p2.x;	plist_ptr->vertices[2][1] = p2.y;	plist_ptr->vertices[2][2] =	p2.z;
						}
						plist_ptr->next = new Poly();
						plist_ptr = plist_ptr->next;

						std::vector<SplPoint2>::iterator curPoly2D_it = curPoly2D.begin();
						int it_index=0; while(it_index<(index+1)%size) {++curPoly2D_it; ++it_index;}
						curPoly2D.erase(curPoly2D_it);

						std::vector<SplPoint3>::iterator curPoly3D_it = curPoly.begin();
						it_index=0; while(it_index<(index+1)%size) {++curPoly3D_it; ++it_index;}
						curPoly.erase(curPoly3D_it);

						nLoop = 0;
						// next vertex considered will be index+2 because we've just erased index+1 point
						continue;
					}
					else
						++nLoop;
					// in other cases, do nothing, the considered vertex will be index+1
					++index;
				}
			}
			}
			else{
				// polygon contains holes
				ASSERT(false);
			}
		}
	}
	if (bRecursive){
		if (node->left)
			ComputeTree(node->left, plist_ptr);
		if (node->right)
			ComputeTree(node->right, plist_ptr);
	}
}

void MarchingCube::ComputeMCInCell(ADFOctree::Cell *cell, const Box3 &curBbox, Poly *&plist_ptr) const
{
	if (cell->GetChildPointer(0)){
		// node
		Box3 childBox;
		for (int i=0;i<8;++i){
			GetChildBox(curBbox, childBox, i);
			ComputeMCInCell(cell->GetChildPointer(i), childBox, plist_ptr);
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
				plist_ptr->vertices[0][0] = vertex1.x;	plist_ptr->vertices[0][1] = vertex1.y;	plist_ptr->vertices[0][2] = vertex1.z;
				plist_ptr->vertices[1][0] = vertex2.x;	plist_ptr->vertices[1][1] = vertex2.y;	plist_ptr->vertices[1][2] = vertex2.z;
				plist_ptr->vertices[2][0] = vertex3.x;	plist_ptr->vertices[2][1] = vertex3.y;	plist_ptr->vertices[2][2] = vertex3.z;
				plist_ptr->next = new Poly ();
				plist_ptr = plist_ptr->next;
			}
		}
	}
}
