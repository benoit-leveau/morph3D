/**********************************************************************
 *<
	PROJECT: Morph3DObj (Morphing object plugin for 3DSMax)

	FILE: Morph3DObj.h

	DESCRIPTION: Main Header

	CREATED BY: Benoît Leveau

	HISTORY: 21/08/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#pragma once

TCHAR *GetString(int id);

// in Module.cpp
extern HINSTANCE hInst3DSMax;

#define REF_OP1		0
#define REF_OP2		1
#define REF_CONT1	2
#define REF_CONT2	3
#define REF_PBLOCK	4

#define EM_SL_OBJECT 1
#define EM_SL_VERTEX 2

// Available selection levels
#define SL_OBJECT EM_SL_OBJECT	//1
#define SL_VERTEX EM_SL_VERTEX	//2

// Morph3DObj Class Version
#define MORPH3D_OBJ_VERSION		100

// Flag bits
#define MORPH_OB1SEL			(1<<0)
#define MORPH_OB2SEL			(1<<1)
#define MORPH_ANYSEL			(MORPH_OB1SEL|MORPH_OB2SEL)

#define MORPH_DISPRESULT		(1<<2)
#define MORPH_DISPHIDDEN		(1<<3)

#define MORPH_UPDATEALWAYS		(1<<4)
#define MORPH_UPDATERENDER		(1<<5)
#define MORPH_UPDATEMANUAL		(1<<6)
#define MORPH_UPDATESELECT		(1<<7)

#define MORPH_FIRSTUPDATE		(1<<8)

#define MORPH_INRENDER			(1<<9)

#define MORPH_ABORTED			(1<<10)

#define MORPH_NEEDSUPDATE		(1<<11)

#define MORPH_ANCHOR1SEL		(1<<12)
#define MORPH_ANCHOR2SEL		(1<<13)

//#define MORPH_SMOOTH			(1<<12)


//--- ClassDescriptor and class vars ---------------------------------

class PickOperand;

class Morph3DObjClassDesc:public ClassDesc {
public:
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE);
	const TCHAR *	ClassName() { return GetString(IDS_MPHOBJ_CNAME); }
	SClass_ID		SuperClassID() { return GEOMOBJECT_CLASS_ID; } // Geometric Object
	Class_ID		ClassID() { return Class_ID(0x2d10472d, 0x7fc925d0); }
	const TCHAR* 	Category() { return GetString(IDS_RB_COMPOUNDOBJECTS);}
	
	BOOL			OkToCreate(Interface *i);
	int 			BeginCreate(Interface *i);
	int 			EndCreate(Interface *i);
	void			ResetClassParams(BOOL fileReset);

	static Morph3DObjClassDesc *Instance(){static Morph3DObjClassDesc instance; return &instance;}
};

class Morph3DObj: public GeomObject, public MeshOpProgress{

	RefResult NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget, 
		PartID& partID, RefMessage message);

public:
	TSTR opaName, opbName;
	Object *ob1, *ob2;
	Control *tm1, *tm2;
	Matrix3 obOffset;
	DWORD flags;
	Interval ivalid;
	
	int selLevel;
	Mesh *currentMesh;
	
	int version;
	IParamBlock *pblock;
	static IParamMap *pmapParam;

	int anchor_1;

	float coeff1, coeff2, coeff_interp;
	int numpop;
	
	static IObjParam *ip;
	static HWND hParams1;
	static HWND hParams2;
	static int addOppMethod;
	static Matrix3 createTM;
	static PickOperand pickCB;
	static BOOL creating;
	static Morph3DObj *editOb;
	static int extractCopy;
	static MoveModBoxCMode *moveMode;
	static RotateModBoxCMode *rotMode;
	static UScaleModBoxCMode *uscaleMode;
	static NUScaleModBoxCMode *nuscaleMode;
	static SquashModBoxCMode *squashMode;
	static SelectModBoxCMode *selectMode;

	Morph3DObj(BOOL loading=FALSE);
	~Morph3DObj();

	void SetupUI1();
	void SetupUI2(BOOL useName=FALSE);
	void SetExtractButtonState();

	void SetFlag(DWORD mask) { flags|=mask; }
	void ClearFlag(DWORD mask) { flags &= ~mask; }
	int TestFlag(DWORD mask) { return(flags&mask?1:0); }

	void SetOperandA (TimeValue t, INode *node);
	void SetOperandB (TimeValue t, INode *node, INode *boolNode, int addOpMethod=0, int matMergeMethod=0, bool *canUndo=NULL);
	BOOL UpdateMesh(TimeValue t,BOOL force=FALSE,BOOL sel=FALSE);
	Object *GetPipeObj(TimeValue t,int which);
	Matrix3 GetOpTM(TimeValue t,int which,Interval *iv=NULL);
	void Invalidate() {ivalid.SetEmpty();}
	void ExtractOperand(int which);

	// From BaseObject
	CreateMouseCallBack* GetCreateMouseCallBack();		
	TCHAR *GetObjectName() { return GetString(IDS_RB_MORPH3D); }

	// For sub-object selection
	void Move( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin=FALSE);
	void Rotate( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin=FALSE);
	void Scale( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin=FALSE);
	
	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt, ModContext* mc);
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags, ModContext* mc);
	
	void SelectSubComponent(HitRecord *hitRec, BOOL selected, BOOL all, BOOL invert);
	void ClearSelection(int selLevel);

	int SubObjectIndex(HitRecord *hitRec);
	void GetSubObjectCenters(SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc);
	void GetSubObjectTMs(SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc);

	void ActivateSubobjSel(int level, XFormModes& modes);

	// NS: New SubObjType API
	int NumSubObjTypes();
	ISubObjType *GetSubObjType(int i);

	// From Object		
	void InitNodeName(TSTR& s) {s = GetString(IDS_RB_MORPH3D);}
	Interval ObjectValidity(TimeValue t);
	int CanConvertToType(Class_ID obtype);
	Object* ConvertToType(TimeValue t, Class_ID obtype);
	BOOL PolygonCount(TimeValue t, int& numFaces, int& numVerts);
	ObjectState Eval(TimeValue time);
	int NumPipeBranches(bool selected);
	Object *GetPipeBranch(int i, bool selected);
	INode *GetBranchINode(TimeValue t,INode *node,int i, bool selected);
	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
	void Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt);
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
	void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm, BOOL useSel);
	void GetLocalBoundBox(TimeValue t, INode *inode,ViewExp* vpt, Box3& box);
	void GetWorldBoundBox(TimeValue t, INode *inode, ViewExp* vpt, Box3& box);

	// From GeomObject		
	ObjectHandle CreateTriObjRep(TimeValue t){return NULL;}  // for rendering, also for deformation
	int IntersectRay(TimeValue t, Ray& r, float& at, Point3& norm);
	Mesh* GetRenderMesh(TimeValue t, INode *inode, View& view, BOOL& needDelete);

	// Animatable methods
	Class_ID ClassID() {return Morph3DObjClassDesc::Instance()->ClassID();}  
	void GetClassName(TSTR& s) {s = GetString(IDS_RB_MORPH3DOBJECT_CLASS);}
	void DeleteThis() {delete this;}				
	void BeginEditParams(IObjParam *ip, ULONG flags,Animatable *prev);
	void EndEditParams(IObjParam *ip, ULONG flags,Animatable *next);
	int RenderBegin(TimeValue t, ULONG flags);
	int RenderEnd(TimeValue t);

	int NumSubs();
	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);
	int SubNumToRefNum(int subNum);

	// From ref
	RefTargetHandle Clone(RemapDir& remap);
	int NumRefs();
	RefTargetHandle GetReference(int i);
	void SetReference(int i, RefTargetHandle rtarg);
	IOResult Load(ILoad *iload);
	IOResult Save(ISave *isave);

	// From MeshOpProgress
	void Init(int total);
	BOOL Progress(int p);

	DWORD GetSelLevel();
	void SetSelLevel(DWORD level);
	void LocalDataChanged();
	void SetVertSel (BitArray &set, IMeshSelect *mod, TimeValue t);
	void SetMeshVertSel(bool bSet);
};				

class PickOperand : 
	public PickModeCallback,
	public PickNodeCallback {
public:		
	Morph3DObj *mo;				

	PickOperand() {mo=NULL;}

	BOOL HitTest(IObjParam *ip,HWND hWnd,ViewExp *vpt,IPoint2 m,int flags);
	BOOL Pick(IObjParam *ip,ViewExp *vpt);

	void EnterMode(IObjParam *ip);
	void ExitMode(IObjParam *ip);

	BOOL Filter(INode *node);

	BOOL RightClick(IObjParam *ip,ViewExp *vpt)	{return TRUE;}

	PickNodeCallback *GetFilter() {return this;}
};

class CreateMorph3DObjProc : public MouseCallBack {
public:
	IObjParam *ip;
	void Init(IObjParam *i) {ip=i;}
	int proc( 
		HWND hWnd, 
		int msg, 
		int point, 
		int flags, 
		IPoint2 m );
};

class CreateMorph3DObjMode : public CommandMode, ReferenceMaker {		
public:		
	CreateMorph3DObjProc proc;
	INode *node, *svNode;
	IObjParam *ip;
	Morph3DObj *obj;

	void Begin(INode *n,IObjParam *i);
	void End(IObjParam *i);		
	void JumpStart(IObjParam *i,Morph3DObj *o);

	int Class() { return CREATE_COMMAND; }
	int ID() { return CID_USER+0x6681; }
	MouseCallBack *MouseProc(int *numPoints) {*numPoints = 1; return &proc;}
	ChangeForegroundCallback *ChangeFGProc() {return CHANGE_FG_SELECTED;}
	BOOL ChangeFG(CommandMode *oldMode) {return TRUE;}
	void EnterMode() {}
	void ExitMode() {}

	int NumRefs() {return 1;}
	RefTargetHandle GetReference(int i) {return node;}
	void SetReference(int i, RefTargetHandle rtarg) {node = (INode*)rtarg;}
	RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
		PartID& partID,  RefMessage message);		

	static CreateMorph3DObjMode *Instance(){static CreateMorph3DObjMode instance; return &instance;}
};


class CreateMorph3DObjRestore : public RestoreObj {
public:   		
	void Restore(int isUndo) {
		if (CreateMorph3DObjMode::Instance()->ip) {
			// Jump out of boolean create mode.
			CreateMorph3DObjMode::Instance()->ip->SetStdCommandMode(CID_OBJMOVE);
		}
	}	
	void Redo() {}
	TSTR Description() {return TSTR(_T("Create Morph3D"));}
};

// Sending the REFMSG_NOTIFY_PASTE message notifies the modify
// panel that the Node's object reference has changed when
// undoing or redoing.
class CreateMorph3DObjNotify : public RestoreObj {
public:   		
	Morph3DObj *obj;
	BOOL which;
	CreateMorph3DObjNotify(Morph3DObj *o, BOOL w) {
		obj = o; which = w;
	}
	void Restore(int isUndo) {
		if (which) {
			obj->NotifyDependents(FOREVER,0,REFMSG_NOTIFY_PASTE);
		}
	}	
	void Redo() {
		if (!which) {
			obj->NotifyDependents(FOREVER,0,REFMSG_NOTIFY_PASTE);
		}
	}
	TSTR Description() {return TSTR(_T("Create Morph3DObj Notify"));}
};

class Morph3DObjCreateCallBack: public CreateMouseCallBack {	
	Morph3DObj *ob;	
public:
	int proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat);
	void SetObj(Morph3DObj *obj) {ob = obj;}
	static Morph3DObjCreateCallBack *Instance(){static Morph3DObjCreateCallBack instance; return &instance;}
};

class SetOperandRestore : public RestoreObj {	
public:
	Morph3DObj *mo;
	TSTR uname, rname;
	int which;

	SetOperandRestore(Morph3DObj *m,int w){
		mo=m;
		uname=(w ? mo->opbName : mo->opaName);
	}
	void Restore(int isUndo) {
		if (which) {
			rname = mo->opbName;
			mo->opbName = uname;
		} else {
			rname = mo->opaName;
			mo->opaName = uname;
		}
		if (mo->hParams2 && mo->editOb==mo) mo->SetupUI2(TRUE);
	}
	void Redo() {
		if (which) {				
			mo->opbName = rname;
		} else {				
			mo->opaName = rname;
		}
		if (mo->hParams2 && mo->editOb==mo) mo->SetupUI2(TRUE);
	}		
};

class TmpTriObject
{
// Data
private:
	bool bInit1, bInit2;
	BOOL bNeedsDel1, bNeedsDel2;
	Mesh *mesh1, *mesh2;
	TriObject *tob1, *tob2;
	TimeValue t;
	Morph3DObj *parent;

// ctor - dtor
public:
	TmpTriObject(TimeValue t, Morph3DObj *parent);
	~TmpTriObject();

// Member Functions
private:
	void CheckInit(int index);
public:
	TriObject &operator()(int index);
	Mesh &operator[](int index);
};
