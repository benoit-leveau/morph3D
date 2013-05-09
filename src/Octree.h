/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing object plugin for 3DSMax)

	FILE: Octree.h

	DESCRIPTION: Header of the Octree class

	CREATED BY: Benoît Leveau

	HISTORY: 20/09/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#pragma once

#include <vector>
#include <map>

extern void GetChildBox(const Box3 &b, Box3 &childBox, int i);
extern void Draw(GraphicsWindow *gw, const Box3 &b);

class Coordinate{
//Data
public:
	struct dbyte{
		unsigned int flag	:	1;
		unsigned int child	:	3;
		bool operator==(dbyte &o) const{return flag==o.flag && child==o.child;}
	};
	dbyte *coord;
	int max_depth;

// Ctor
public:
	inline Coordinate(const Coordinate &c){
		max_depth = c.max_depth;
		coord = new dbyte[max_depth];
		for (int i=0;i<max_depth;++i){
			coord[i].flag = c.coord[i].flag;
			coord[i].child = c.coord[i].child;
		}
	}
	explicit inline Coordinate(int max_depth_){
		max_depth = max_depth_;
		coord = new dbyte[max_depth];
		for (int i=0;i<max_depth;++i){
			coord[i].child = 0;
			coord[i].flag = 0;
		}
	}
	inline ~Coordinate(){if (coord) delete [] coord;}
	inline void operator=(const Coordinate &c){
		for (int i=0;i<max_depth;++i){
			coord[i].flag = c.coord[i].flag;
			coord[i].child = c.coord[i].child;
		}
	}


// Member Functions
public:
	inline int GetLevel() const{
		int cur_depth = 0;
		while(cur_depth<max_depth && coord[cur_depth].flag) ++cur_depth;
		return cur_depth;
	}
	inline int GetChildToGo(int level) const{return (coord[level].child);}
	inline void Trim(int level){coord[level].flag = 0;}
	inline void GoUp(){coord[max(0,GetLevel()-1)].flag = 0;}
	inline void GoDown(int whichChild){
		int level = GetLevel();
		if (level<max_depth){
			coord[level].flag = 1;
			coord[level].child = whichChild;
		}
	}
	inline bool IsParentOf(Coordinate &c) const{
		int cur_level = 0;
		while(cur_level<max_depth && coord[cur_level]==c.coord[cur_level]) ++cur_level;
		return (cur_level!=max_depth && c.coord[cur_level].flag);
	}
};

#ifndef OPTIMIZATIONS_OCTREE
#define SUBDIVIDE(x) x->Subdivide()
#else
#define SUBDIVIDE(x) x->InitChildPointers(NextCellPtr())
#endif

template <class T> class Octree
{
	friend class FaceOctree;
	friend class ADFOctree;
	friend class MorphEngine;
	friend class MarchingCube;

// Stats Data
	USE_TIMER

// Data
private:
	class Cell{
		friend class Octree;
		friend class FaceOctree;
		private:
			Cell *parent;
			Cell *childs[8];
			T value;
		public:
			inline explicit Cell(Cell *parent = NULL) : parent(parent){
				for(int i=0;i<8;++i) childs[i] = NULL;
			}
			inline ~Cell(){
				for(int i=0;i<8;++i) if (childs[i]) {delete (childs[i]); childs[i]=NULL;}
			}
			const inline Cell *GoDown(int whichChild) const{
				if (childs[whichChild])
					return childs[whichChild];
				else
					return this;
			}
#ifndef OPTIMIZATIONS_OCTREE
			inline void Subdivide(){
				for (int i=0;i<8;++i)
					childs[i] = new Cell(this);
			}
#else
			inline void InitChildPointers(Cell *childsPtr){
				for (int i=0;i<8;++i){
					childs[i] = childsPtr++;
					childs[i]->parent = this;
				}
			}
#endif
			inline Cell *GetChildPointer(int i){
				return childs[i];
			}
			inline Cell *GetChildPointer(int i) const{
				return childs[i];
			}
			inline Cell *GetParent() const{
				return parent;
			}
			inline void operator<<(T &v){value = v;}
			inline T *GetValue() const{return const_cast<T *>(&value);}
	};
	Cell root;
	Box3 bbox;
	int max_depth;
	#ifdef OPTIMIZATIONS_OCTREE
		int cpt_cells, size;
		Cell *arrayOfCells;
	#endif //OPTIMIZATIONS_OCTREE

// Ctor
public:
	Octree(const Box3 &bbox, int max_depth) : root(), bbox(bbox), max_depth(max_depth){
#ifdef OPTIMIZATIONS_OCTREE
			cpt_cells = 0;
			size = OPT_OCTREE_ARRAY_SIZE;
			size = size + (8-size%8);
			arrayOfCells = new Cell[size]; // will be deleted by the cells themselves
#endif // OPTIMIZATIONS_OCTREE
	}

	~Octree(){
		Destroy();
	}

// Member Functions
private:
	virtual void Reset(T value) = 0;
	void Destroy(){
		for (int i=0;i<8;++i) if (root.childs[i]) {delete root.childs[i]; root.childs[i]=NULL;}
	}
#ifdef OPTIMIZATIONS_OCTREE
	inline Cell *NextCellPtr(){
		if ((8*cpt_cells)==size){
			arrayOfCells = new Cell[size]; // will be deleted by the cells themselves
			cpt_cells = 0;
		}
		return arrayOfCells+(8*cpt_cells++);
	}
#endif

public:
	void Clear(){
		Destroy();
		Reset(root.value);
	}
	inline Cell *GetCell(Coordinate &c) const{
		Cell *current = &root;
		int level = 0;
		int levelToGo = c.GetLevel();
		while(level<levelToGo){
			if (!current->childs) break;
			current = current->childs[c.GetChildToGo(level++)];
		}
		return current;
	}
	inline int GetMaxDepth() const{return max_depth;}
	#ifdef DISPLAY_MORPH_ENGINE
		void DisplayCell(GraphicsWindow *gw, const Cell *cell, const Box3 &b) const
		{
			Draw(gw, b);
			Box3 childBoxes[8];
			// if you want to display just 1 corner of the overall octree, change the start_index/end_index when cell==&root
			int start_index = 0; // (cell==&root)?5:0
			int end_index = 8; // (cell==&root)?6:8
			for (int i=start_index; i<end_index; ++i){
				Cell *c = cell->GetChildPointer(i);
				if (c){
					Box3 childBox;
					GetChildBox(b, childBox, i);
					DisplayCell(gw, cell->GetChildPointer(i), childBox);
				}
			}
		}
	#endif // DISPLAY_MORPH_ENGINE
};
