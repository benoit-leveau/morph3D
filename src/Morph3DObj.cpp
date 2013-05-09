/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing object plugin for 3DSMax)

	FILE: Morph3DObj.cpp

	DESCRIPTION: Morphing Object Class

	CREATED BY: Benoît Leveau

	HISTORY: 21/08/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#include "StdAfx.h"
#include "Morph3DObj.h"
#include "MorphEngine.h"
#include "iparamm2.h"
#include "ChunkID.h"
#include "MemoryManager.h"
#include <vector>

#include "Resource.h"

#pragma warning(disable: 4312) // disable "warning 'type cast' : conversion from 'LONG' to 'Morph3DObj *' of greater size"

void *Morph3DObjClassDesc::Create(BOOL loading/*=FALSE*/){
	return new Morph3DObj;
}

void Morph3DObjClassDesc::ResetClassParams(BOOL fileReset)
{
	Morph3DObj::addOppMethod = IDC_TARG_MOVE;
}

BOOL Morph3DObjClassDesc::OkToCreate(Interface *i)
{
	if (i->GetSelNodeCount()!=1) return FALSE;	
	
	ObjectState os = i->GetSelNode(0)->GetObjectRef()->Eval(i->GetTime());	
	if (os.obj->IsParticleSystem()) return FALSE;
	return os.obj->IsDeformable() || os.obj->CanConvertToType(defObjectClassID);
}

int Morph3DObjClassDesc::BeginCreate(Interface *i)
{	
	SuspendSetKeyMode();
	assert(i->GetSelNodeCount()==1);

	CreateMorph3DObjMode::Instance()->Begin(i->GetSelNode(0),(IObjParam*)i);
	i->PushCommandMode(CreateMorph3DObjMode::Instance());
	return TRUE;
}

int Morph3DObjClassDesc::EndCreate(Interface *i)
{
	ResumeSetKeyMode();
	CreateMorph3DObjMode::Instance()->End((IObjParam*)i);
	i->RemoveMode(CreateMorph3DObjMode::Instance());
	return TRUE;
}

IParamMap *Morph3DObj::pmapParam			= NULL;
IObjParam *Morph3DObj::ip                   = NULL;
HWND Morph3DObj::hParams1                   = NULL;
HWND Morph3DObj::hParams2                   = NULL;
int Morph3DObj::addOppMethod                = IDC_TARG_MOVE;
BOOL Morph3DObj::creating                   = FALSE;
Morph3DObj *Morph3DObj::editOb              = NULL;
MoveModBoxCMode*    Morph3DObj::moveMode    = NULL;
RotateModBoxCMode*  Morph3DObj::rotMode 	= NULL;
UScaleModBoxCMode*  Morph3DObj::uscaleMode  = NULL;
NUScaleModBoxCMode* Morph3DObj::nuscaleMode = NULL;
SquashModBoxCMode*  Morph3DObj::squashMode  = NULL;
SelectModBoxCMode*  Morph3DObj::selectMode  = NULL;
Matrix3 Morph3DObj::createTM;
PickOperand Morph3DObj::pickCB;
int Morph3DObj::extractCopy = FALSE;

//--- Parameter map/block descriptors -------------------------------

#define PB_COEFF		0

//
//
// Parameters

static ParamUIDesc descParam[] = {
	
	// Material ID
	ParamUIDesc(
		PB_COEFF,
		EDITTYPE_FLOAT,
		IDC_MORPH_COEFF, IDC_MORPH_COEFFSPIN,
		0.0f, 1.0f,
		0.01f)
	};
#define PARAMDESC_LENGH 1

static ParamBlockDescID descVer0[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0 }};

#define PBLOCK_LENGTH	1
#define CURRENT_VERSION	1

Morph3DObj::Morph3DObj(BOOL loading) : currentMesh(NULL)
{
	MakeRefByID(FOREVER, REF_PBLOCK, CreateParameterBlock(descVer0, PBLOCK_LENGTH, CURRENT_VERSION));	
	pblock->SetValue(PB_COEFF, 0, 0);
	version = MORPH3D_OBJ_VERSION;
	obOffset  = Matrix3(1);
	ob1 = ob2 = NULL;
	tm1 = tm2 = NULL;
	flags = 0;
	ivalid.SetEmpty();
	SetFlag(MORPH_UPDATEALWAYS | MORPH_DISPRESULT | MORPH_FIRSTUPDATE);
	selLevel = SL_OBJECT;
}

Morph3DObj::~Morph3DObj()
{
	MorphEngine::Instance()->Clear();
	DeleteAllRefsFromMe();
}

void Morph3DObj::ExtractOperand(int which)
{
	if (creating) return;

	// Compute a node TM for the new object
	assert(ip);
	ModContextList list;
	INodeTab nodes;	
	ip->GetModContexts(list,nodes);
	Matrix3 tm = nodes[0]->GetObjectTM(ip->GetTime());
	Matrix3 tmOp = GetOpTM(ip->GetTime(),which);
	Object *obj = which ? ob2 : ob1;
	if (!obj) return;
	tm = tmOp * tm;

	// Clone the object if specified
	if (extractCopy) obj = (Object*)obj->Clone();

	// Create the new node
	INode *node = ip->CreateObjectNode(obj);

	// Set the node TM.
	SuspendAnimate();
	AnimateOff();
	node->SetNodeTM(0,tm);
	ResumeAnimate();

	nodes.DisposeTemporary();
}

void Morph3DObj::SetOperandA (TimeValue t, INode *node)
{
	Object *obj = node->GetObjectRef();
	theHold.Put(new SetOperandRestore(this,0));
	opaName = TSTR(_T("M_")) + TSTR(node->GetName());

	// Plug in the object
	ReplaceReference (0, obj);
	// Make a new controller
	ReplaceReference (2, NewDefaultMatrix3Controller());	

	ivalid.SetEmpty();
	theHold.Accept(GetString(IDS_RB_PICKOPERAND));
}

void Morph3DObj::SetOperandB (TimeValue t, INode *node, INode *morphNode, int addOpMethod, int matMergeMethod, bool *canUndo)
{
	BOOL delnode = FALSE;
	Matrix3 oppTm = node->GetObjectTM(t);
	Matrix3 morphTm = morphNode->GetObjectTM(t);
	Object *obj = node->GetObjectRef();

	switch (addOppMethod) {
		case IDC_TARG_REFERENCE:
			obj = MakeObjectDerivedObject(obj);
			break;
		case IDC_TARG_COPY:
			obj = (Object*)obj->Clone();
			break;
		case IDC_TARG_MOVE:
			delnode = TRUE;
			break;
	}

	theHold.Put(new SetOperandRestore(this,1));

	opbName = TSTR(_T("M_")) + TSTR(node->GetName());

	// Plug in the object
	ReplaceReference(1,obj);

	// Grab the TM controller from the node and make a copy of it
	Control *opCont = node->GetTMController();
	opCont          = (Control*)opCont->Clone();

	// Get the object offset
	obOffset = node->GetObjTMBeforeWSM(t) * Inverse(node->GetNodeTM(t));

	// Adjust the trasform to be relative to us.
	opCont->ChangeParents(t,node->GetParentTM(t),morphTm,oppTm);
	ReplaceReference(3,opCont);
	NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);

	ivalid.SetEmpty();
	if (delnode) ip->DeleteNode(node);

	theHold.Accept(GetString(IDS_RB_PICKOPERAND));
}

Object *Morph3DObj::GetPipeObj(TimeValue t,int which)
{
	ObjectState os;
	if (which==0) {
		if (ob1) {
			os = ob1->Eval(t);
			return os.obj;
		} else {
			return NULL;
		}
	} else {
		if (ob2) {
			os = ob2->Eval(t);
			return os.obj;
		} else {
			return NULL;
		}
	}
	return os.obj;
}

Matrix3 Morph3DObj::GetOpTM(TimeValue t,int which,Interval *iv)
{
	Matrix3 tm(1);
	Interval valid, *v;
	if (iv) v = iv;
	else v = &valid;
	if (which==0) {
		if (tm1) {
			tm1->GetValue(t,&tm,*v,CTRL_RELATIVE);
		}
	} else {
		if (tm2) {			
			tm2->GetValue(t,&tm,*v,CTRL_RELATIVE);
			tm = obOffset * tm;
		}
	}
	return tm;
}

static TriObject *GetTriObject(TimeValue t,Object *obj,Interval &valid,BOOL &needsDel)
{	
	needsDel = FALSE;
	if (!obj) return NULL;
	ObjectState os = obj->Eval(t);
	valid &= os.Validity(t);
	if (os.obj->IsSubClassOf(triObjectClassID)) {
		return (TriObject*)os.obj;
	} else {
		if (os.obj->CanConvertToType(triObjectClassID)) {
			Object *oldObj = os.obj;
			TriObject *tobj = (TriObject*)os.obj->ConvertToType(t,triObjectClassID);			
			needsDel = (tobj != oldObj);			
			return tobj;
		}
	}
	return NULL;
}

int Morph3DObj::RenderBegin(TimeValue t, ULONG flags)
{
	SetFlag(MORPH_INRENDER);
	if (TestFlag(MORPH_UPDATERENDER)) {
		ivalid.SetEmpty();
		NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
	}
	return 0;
}

int Morph3DObj::RenderEnd(TimeValue t)
{
	ClearFlag(MORPH_INRENDER);	
	return 0;
}

void Morph3DObj::Init(int total)
{
}

BOOL Morph3DObj::Progress(int p)
{
	SHORT res = GetAsyncKeyState(VK_ESCAPE);
	if (res&1) {
		SetFlag(MORPH_ABORTED);
		return FALSE;
	}
	else return TRUE;
}

CreateMouseCallBack* Morph3DObj::GetCreateMouseCallBack()
{
	Morph3DObjCreateCallBack::Instance()->SetObj(this);
	return Morph3DObjCreateCallBack::Instance();
}

int CreateMorph3DObjProc::proc( 
		HWND hWnd, 
		int msg, 
		int point, 
		int flags, 
		IPoint2 m ) 
	{
	switch (msg) {
		case MOUSE_POINT:
			ip->SetActiveViewport(hWnd);
			break;
		case MOUSE_FREEMOVE:
			SetCursor(LoadCursor(NULL,IDC_ARROW));
			break;
// mjm - 3.1.99
		case MOUSE_PROPCLICK:
			// right click while between creations
			ip->RemoveMode(NULL);
			break;
// mjm - end
		}	
	return TRUE;
}

RefResult CreateMorph3DObjMode::NotifyRefChanged(
		Interval changeInt, 
		RefTargetHandle hTarget, 
		PartID& partID,
		RefMessage message)
{
	switch (message) {
		case REFMSG_TARGET_SELECTIONCHANGE:		
		case REFMSG_TARGET_DELETED:			
			if (ip) ip->StopCreating();
			break;

	}
	return REF_SUCCEED;
}

void CreateMorph3DObjMode::Begin(INode *n,IObjParam *i) 
{
	MakeRefByID(FOREVER,0,n);
	svNode = node;
	assert(node);
	ip = i;
	proc.Init(ip);

	theHold.Begin();
	theHold.Put(new CreateMorph3DObjRestore);

	obj = new Morph3DObj;
	
	theHold.Put(new CreateMorph3DObjNotify(obj,1));

	obj->createTM = node->GetObjectTM(i->GetTime());
	obj->SetOperandA(i->GetTime(), node);
	node->SetObjectRef(obj);
	
	theHold.Put(new CreateMorph3DObjNotify(obj,0));

	obj->BeginEditParams(i,BEGIN_EDIT_CREATE,NULL);
}

void CreateMorph3DObjMode::End(IObjParam *i)
{
	svNode = node;
	if (obj) obj->EndEditParams(i,END_EDIT_REMOVEUI,NULL);
	DeleteAllRefsFromMe();
	ip  = NULL;
	obj = NULL;
}

void CreateMorph3DObjMode::JumpStart(IObjParam *i,Morph3DObj *o)
{
	ip  = i;
	obj = o;
	//MakeRefByID(FOREVER,0,svNode);
	obj->BeginEditParams(i,BEGIN_EDIT_CREATE,NULL);
}

int Morph3DObjCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) {
	Point3 pt;
	if (msg==MOUSE_POINT||msg==MOUSE_MOVE) {
		switch(point) {
			case 0:  // only happens with MOUSE_POINT msg
				pt = vpt->GetPointOnCP(m);
				mat.SetTrans(pt);
				break;
			case 1:								
				pt = vpt->GetPointOnCP(m);
				mat.SetTrans(pt);
				if (msg==MOUSE_POINT) 
					return CREATE_STOP;
				break;
		}
	}
	else
		if (msg == MOUSE_ABORT)
			return CREATE_ABORT;
	return TRUE;
}

int Morph3DObj::NumPipeBranches(bool selected) 
{
	int num=0;
	if ((!selected || TestFlag(MORPH_OB1SEL)) && ob1) num++;
	if ((!selected || TestFlag(MORPH_OB2SEL)) && ob2) num++;
	return num;
}

Object *Morph3DObj::GetPipeBranch(int i, bool selected) 
{
	if (i) return ob2;	
	if (ob1 && (!selected || TestFlag(MORPH_OB1SEL))) return ob1;
	return ob2;
}

INode *Morph3DObj::GetBranchINode(TimeValue t,INode *node,int i, bool selected)
{
	if(!selected)
		return CreateINodeTransformed(node,GetOpTM(t,i));

	assert(i<2);
	int index = 0;
	if (i) index = 1;
	else if (TestFlag(MORPH_OB1SEL)) index = 0;
	else index = 1;
	return CreateINodeTransformed(node,GetOpTM(t,index));	
}

int Morph3DObj::NumRefs()
{
	return 5;
}

int Morph3DObj::NumSubs()
{
	return 5;
}

Animatable* Morph3DObj::SubAnim(int i)
{
	switch (i) {
		case 0:  return ob1;
		case 1:  return tm1;
		case 2:	 return ob2;
		case 3:	 return tm2;
		case 4:	 return pblock;
		default: return NULL;
	}	
}

TSTR Morph3DObj::SubAnimName(int i)
{	
	switch (i) {
		case 0: return opaName;//GetString(IDS_RB_OPERANDA);
		case 1: return GetString(IDS_RB_OPERANDATRANSFORM);
		case 2: return opbName;//GetString(IDS_RB_OPERANDB);
		case 3: return GetString(IDS_RB_OPERANDBTRANSFORM);
		case 4: return GetString(IDS_PARAMETERS);
	}
	return _T("Error");
}

int Morph3DObj::SubNumToRefNum(int subNum)
{
	switch (subNum) {
		case 0:  return REF_OP1;
		case 1:  return REF_CONT1;
		case 2:	 return REF_OP2;
		case 3:	 return REF_CONT2;
		case 4:	 return REF_PBLOCK;
		default: return -1;
	}	
}

RefTargetHandle Morph3DObj::GetReference(int i)
{
	switch (i) {
		case REF_OP1: 		return ob1;
		case REF_OP2: 		return ob2;
		case REF_CONT1:		return tm1;
		case REF_CONT2:		return tm2;
		case REF_PBLOCK:	return pblock;
		default:			return NULL;
	}
}

void Morph3DObj::SetReference(int i, RefTargetHandle rtarg)
{
	switch (i) {
		case REF_OP1: 	 ob1 = (Object*)rtarg;	break;
		case REF_OP2: 	 ob2 = (Object*)rtarg;  break;
		case REF_CONT1:	 tm1 = (Control*)rtarg; break;
		case REF_CONT2:	 tm2 = (Control*)rtarg; break;
		case REF_PBLOCK: pblock = (IParamBlock*)rtarg; break;
	}
}

RefTargetHandle Morph3DObj::Clone(RemapDir& remap)
{
	Morph3DObj *obj = new Morph3DObj;
	if (ob1) obj->ReplaceReference(REF_OP1,remap.CloneRef(ob1));
	if (ob2) obj->ReplaceReference(REF_OP2,remap.CloneRef(ob2));
	if (tm1) obj->ReplaceReference(REF_CONT1,remap.CloneRef(tm1));
	if (tm2) obj->ReplaceReference(REF_CONT2,remap.CloneRef(tm2));
	if (pblock) obj->ReplaceReference(REF_PBLOCK, remap.CloneRef(pblock));	
	obj->flags = flags;
	obj->opaName = opaName;
	obj->opbName = opbName;
	obj->version = version;
	obj->selLevel = selLevel;
	BaseClone(this, obj, remap);
	return obj;
}

int Morph3DObj::IntersectRay(TimeValue t, Ray& r, float& at, Point3& norm)
{
	if (TestFlag(MORPH_DISPRESULT)) {
		UpdateMesh(t);
		return currentMesh->IntersectRay(r,at,norm);
	} else {
		return 0;
	}
}

Mesh* Morph3DObj::GetRenderMesh(TimeValue t, INode *inode, View& view, BOOL& needDelete)
{	
	if (!ob1 || !ob2) {
		if (ob1) {
			Object *obj =
				GetPipeObj(t,0);
			if (obj && obj->SuperClassID()==GEOMOBJECT_CLASS_ID) {
				return ((GeomObject*)obj)->GetRenderMesh(t,inode,view,needDelete);
			}
		}
		if (ob2) {
			Object *obj =
				GetPipeObj(t,1);
			if (obj && obj->SuperClassID()==GEOMOBJECT_CLASS_ID) {
				return ((GeomObject*)obj)->GetRenderMesh(t,inode,view,needDelete);
			}
		}
	}
	UpdateMesh(t);
	needDelete = FALSE;
	return currentMesh ? currentMesh : NULL;	
}

int Morph3DObj::HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, 
						IPoint2 *p, ViewExp *vpt)
{
	int res = 0;
	if (TestFlag(MORPH_DISPRESULT) && ob1 && ob2) {
		UpdateMesh(t,FALSE,inode->Selected());
		if (!currentMesh) return 0;
		HitRegion hitRegion;
		GraphicsWindow *gw = vpt->getGW();	
		Material *mtl = gw->getMaterial();		
		gw->setTransform(inode->GetObjectTM(t));
		MakeHitRegion(hitRegion, type, crossing, 4, p);
		res = currentMesh->select(gw, mtl, &hitRegion, flags & HIT_ABORTONHIT);
		if (res) return res;
	} else {
		Object *ob;
		if (ob=GetPipeObj(t,0)) {
			INodeTransformed n(inode,GetOpTM(t,0));
			res = ob->HitTest(t,&n,type,crossing,flags,p,vpt);
			if (res) return res;
		}
		if (ob=GetPipeObj(t,1)) {
			INodeTransformed n(inode,GetOpTM(t,1));
			res = ob->HitTest(t,&n,type,crossing,flags,p,vpt);
			if (res) return res;
		}
	}
	return res;
}

void Morph3DObj::Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt)
{
	if (TestFlag(MORPH_DISPRESULT) && ob1 && ob2) {
		UpdateMesh(t,FALSE,inode->Selected());
		if (!currentMesh) return;
		Matrix3 tm = inode->GetObjectTM(t);	
		GraphicsWindow *gw = vpt->getGW();		
		gw->setTransform(tm);
		currentMesh->snap( gw, snap, p, tm );
	} else {	
		Object *ob;
		if (ob=GetPipeObj(t,0)) {
			INodeTransformed n(inode,GetOpTM(t,0));
			ob->Snap(t,&n,snap,p,vpt);
		}	
		if (ob=GetPipeObj(t,1)) {
			INodeTransformed n(inode,GetOpTM(t,1));
			ob->Snap(t,&n,snap,p,vpt);
		}
	}
}

int Morph3DObj::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags)
{	
	int disp = 0;
	GraphicsWindow *gw = vpt->getGW();
	BOOL showHidden = TestFlag(MORPH_DISPHIDDEN);

	if (TestFlag(MORPH_DISPRESULT) && ob1 && ob2) {
		UpdateMesh(t,FALSE,inode->Selected());
		if (!currentMesh) return 0;
#ifdef DISPLAY_MORPH_ENGINE
		gw->setTransform(inode->GetObjectTM(t));
		MorphEngine::Instance()->Display(2, gw);
#else
		INodeTransformed n(inode,GetOpTM(t,0));
		gw->setTransform(n.GetObjectTM(t));
		currentMesh->render(gw, inode->Mtls(),(flags&USE_DAMAGE_RECT)?&vpt->GetDammageRect():NULL, COMP_ALL, inode->NumMtls());
#endif
	}
	else 
	{
		TmpTriObject objects(ip->GetTime(), this);
		if (TestFlag(MORPH_ANCHOR1SEL)){
			INodeTransformed n(inode,GetOpTM(t,0));
			objects[0].dispFlags = 0;
			objects[0].SetDispFlag(DISP_VERTTICKS|DISP_SELVERTS);
			objects[0].selLevel = MESH_VERTEX;
			objects(0).Display(t,&n,vpt,flags);
		}
		else if (TestFlag(MORPH_ANCHOR2SEL)){
			INodeTransformed n(inode,GetOpTM(t,1));
			objects[1].dispFlags = 0;
			objects[1].SetDispFlag(DISP_VERTTICKS|DISP_SELVERTS);
			objects[1].selLevel = MESH_VERTEX;
			objects(1).Display(t,&n,vpt,flags);
		}
		else{
			Object *ob;
			DWORD rlim = gw->getRndLimits();
			if (ob=GetPipeObj(t,0)) {
				INodeTransformed n(inode,GetOpTM(t,0));
				if (inode->Selected()) {
					if (TestFlag(MORPH_OB1SEL)) {
						vpt->getGW()->setColor(LINE_COLOR,1.0f,0.0f,0.0f);
					} else {
						Point3 selClr = GetUIColor(COLOR_SELECTION); 
						vpt->getGW()->setColor( LINE_COLOR, selClr.x, selClr.y, selClr.z);
					}
				}
	#ifdef DISPLAY_MORPH_ENGINE
				gw->setTransform(n.GetObjectTM(t));
				MorphEngine::Instance()->Display(0, gw);
	#endif //DISPLAY_MORPH_ENGINE
				ob->Display(t,&n,vpt,flags);
			}
			if (ob=GetPipeObj(t,1)) {
				INodeTransformed n(inode,GetOpTM(t,1));
				if (inode->Selected()) {
					if (TestFlag(MORPH_OB2SEL)) {
						vpt->getGW()->setColor(LINE_COLOR,1.0f,0.0f,0.0f);
					} else {
						vpt->getGW()->setColor( LINE_COLOR, GetSelColor());
					}
				}
	#ifdef DISPLAY_MORPH_ENGINE
				gw->setTransform(n.GetObjectTM(t));
				MorphEngine::Instance()->Display(1, gw);
	#endif //DISPLAY_MORPH_ENGINE
				ob->Display(t,&n,vpt,flags);
			}
		}
	}
	return 0;
}

void Morph3DObj::GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm, BOOL useSel)
{
	Box3 abox;
	abox.Init();
	box.Init();
	if (TestFlag(MORPH_DISPRESULT) && ob1 && ob2) {
		UpdateMesh(t);
		if (!currentMesh) return;
		box = currentMesh->getBoundingBox(tm);
	} 
	else {
		Object *ob;
		if (ob=GetPipeObj(t,0)) {				
			if (tm) {
				Matrix3 mat = GetOpTM(t,0) * *tm;
				ob->GetDeformBBox(t,abox,&mat,useSel);
			}
			else
				ob->GetDeformBBox(t,abox,NULL,useSel);
			box += abox;
		}
		if (ob=GetPipeObj(t,1)) {
			if (tm) {
				Matrix3 mat = GetOpTM(t,1) * *tm;
				ob->GetDeformBBox(t,abox,&mat,useSel);
			}
			else
				ob->GetDeformBBox(t,abox,NULL,useSel);
			box += abox;
		}
	}
}

void Morph3DObj::GetLocalBoundBox(TimeValue t, INode *inode,ViewExp* vpt, Box3& box) 
{
	Box3 abox;
	abox.Init();
	box.Init();
	if (TestFlag(MORPH_DISPRESULT) && ob1 && ob2) {
		UpdateMesh(t,FALSE,inode->Selected());
		if (!currentMesh) return;
		box = currentMesh->getBoundingBox();	
	} 
	else {
		Object *ob;
		if (ob=GetPipeObj(t,0)) {
			INodeTransformed n(inode,GetOpTM(t,0));
			ob->GetLocalBoundBox(t,&n,vpt,abox);
			if (!abox.IsEmpty()) abox = abox * GetOpTM(t,0);
			box += abox;
		}
		if (ob=GetPipeObj(t,1)) {
			INodeTransformed n(inode,GetOpTM(t,1));
			ob->GetLocalBoundBox(t,&n,vpt,abox);
			if (!abox.IsEmpty()) abox = abox * GetOpTM(t,1);
			box += abox;
		}
	}
}

void Morph3DObj::GetWorldBoundBox(TimeValue t, INode *inode, ViewExp* vpt, Box3& box)
{
	Box3 abox;
	int disp = 0;
	abox.Init();
	box.Init();
	if (TestFlag(MORPH_DISPRESULT) && ob1 && ob2) {
		UpdateMesh(t,FALSE,inode->Selected());
		if (!currentMesh) return;
		Matrix3 mat = inode->GetObjectTM(t);	
		box = currentMesh->getBoundingBox();
		if (!box.IsEmpty()) box = box * mat;
	}
	else{
		Object *ob;
		if (ob=GetPipeObj(t,0)){
			INodeTransformed n(inode,GetOpTM(t,0));
			ob->GetWorldBoundBox(t,&n,vpt,abox);
			box += abox;
		}
		if (ob=GetPipeObj(t,1)){
			INodeTransformed n(inode,GetOpTM(t,1));
			ob->GetWorldBoundBox(t,&n,vpt,abox);
			box += abox;
		}
	}
}

RefResult Morph3DObj::NotifyRefChanged(Interval changeInt, 
									   RefTargetHandle hTarget, 
									   PartID& partID, 
									   RefMessage message ) 
{
	switch (message) {
		case REFMSG_CHANGE:
			if (pmapParam && pmapParam->GetParamBlock()==pblock) {
				pmapParam->Invalidate();
			}
			ivalid.SetEmpty();
			break;

		case REFMSG_GET_PARAM_DIM: {
			GetParamDim *gpd = (GetParamDim*)partID;
			switch (gpd->index) {				
				case 0:
				default: gpd->dim = defaultDim; break;
				}			
			return REF_STOP; 
			}

		case REFMSG_GET_PARAM_NAME: {
			GetParamName *gpn = (GetParamName*)partID;
			switch (gpn->index) {				
				case PB_COEFF:		gpn->name = GetString(IDS_MPH_PBCOEFF); break;
				default:			gpn->name = TSTR(_T("")); break;
				}
			return REF_STOP; 
			}

		case REFMSG_SELECT_BRANCH:
			if (hTarget==ob1 || hTarget==ob2) {
				ClearFlag(MORPH_OB1SEL|MORPH_OB2SEL);
				if (hTarget==ob1) SetFlag(MORPH_OB1SEL);
				if (hTarget==ob2) SetFlag(MORPH_OB2SEL);
				NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
				NotifyDependents(FOREVER,(PartID) this,REFMSG_BRANCHED_HISTORY_CHANGED);
			}
			break;
	}
	return REF_SUCCEED;
}

//--Subobject Selection-------------------------------------------------------------


void Morph3DObj::Move(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin)
{
	if (TestFlag(MORPH_OB1SEL) && tm1) {
		SetXFormPacket pckt(val,partm,tmAxis);
		tm1->SetValue(t,&pckt,TRUE,CTRL_RELATIVE);
	}
	if (TestFlag(MORPH_OB2SEL) && tm2) {
		SetXFormPacket pckt(val,partm,tmAxis);
		tm2->SetValue(t,&pckt,TRUE,CTRL_RELATIVE);
	}
}

void Morph3DObj::Rotate(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin)
{
	if (TestFlag(MORPH_OB1SEL) && tm1) {
		SetXFormPacket pckt(val,localOrigin,partm,tmAxis);
		tm1->SetValue(t,&pckt,TRUE,CTRL_RELATIVE);
	}
	if (TestFlag(MORPH_OB2SEL) && tm2) {
		SetXFormPacket pckt(val,localOrigin,partm,tmAxis);
		tm2->SetValue(t,&pckt,TRUE,CTRL_RELATIVE);
	}
}

void Morph3DObj::Scale(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin)
{
	if (TestFlag(MORPH_OB1SEL) && tm1) {
		SetXFormPacket pckt(val,localOrigin,partm,tmAxis);
		tm1->SetValue(t,&pckt,TRUE,CTRL_RELATIVE);
	}
	if (TestFlag(MORPH_OB2SEL) && tm2) {
		SetXFormPacket pckt(val,localOrigin,partm,tmAxis);
		tm2->SetValue(t,&pckt,TRUE,CTRL_RELATIVE);
	}
}

int Morph3DObj::HitTest(TimeValue t, INode* inode, int type, int crossing, 
						int flags, IPoint2 *p, ViewExp *vpt, ModContext* mc)
{	
	if (selLevel == SL_OBJECT){
		int res = 0;
		Object *ob;
		if ((ob=GetPipeObj(t,0)) &&
			!(flags&HIT_SELONLY && !TestFlag(MORPH_OB1SEL)) &&
			!(flags&HIT_UNSELONLY && TestFlag(MORPH_OB1SEL)) ) {

				INodeTransformed n(inode,GetOpTM(t,0));

				if (ob->HitTest(t,&n,type,crossing,flags,p,vpt)) {
					vpt->LogHit(inode,mc,0,0,NULL);
					res = TRUE;
					if (flags & HIT_ABORTONHIT) return TRUE;
				}		
		}
		if ((ob=GetPipeObj(t,1)) &&
			!(flags&HIT_SELONLY && !TestFlag(MORPH_OB2SEL)) &&
			!(flags&HIT_UNSELONLY && TestFlag(MORPH_OB2SEL)) ) {

				INodeTransformed n(inode,GetOpTM(t,1));

				if (ob->HitTest(t,&n,type,crossing,flags,p,vpt)) {
					vpt->LogHit(inode,mc,0,1,NULL);
					res = TRUE;			
				}		
		}

		return res;
	}
	else{
		Interval valid;
		int savedLimits;
		int res = 0;
		bool bObj1 = TestFlag(MORPH_ANCHOR1SEL);
		Object *ob = GetPipeObj(t,bObj1?0:1);
		Interval ivalid = FOREVER;
		BOOL needsDel;
		TriObject *tob = GetTriObject(t,ob,ivalid,needsDel);
		if (ob){
			INodeTransformed n(inode,GetOpTM(t,bObj1?0:1));

			HitRegion hr;
			MakeHitRegion(hr,type, crossing, DEF_PICKBOX_SIZE, p);

			// Setup GW
			GraphicsWindow *gw = vpt->getGW();
			gw->setHitRegion(&hr);

			//Matrix3 mat = GetOpTM(t,bObj1?0:1);
			Matrix3 mat = bObj1 ? inode->GetObjectTM(t) : n.GetObjectTM(t);
			gw->setTransform(mat);	
			gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~GW_ILLUM);
			gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
			gw->clearHitCode();

			Mesh *mesh = &tob->GetMesh();

			SubObjHitList hitList;
			MeshSubHitRec *rec;
			DWORD thisFlags = flags | SUBHIT_VERTS;
			res = mesh->SubObjectHitTest(gw, gw->getMaterial(), &hr, thisFlags, hitList);
			rec = hitList.First();
			while (rec) {
				vpt->LogHit(inode,mc,rec->dist,rec->index,NULL);
				rec = rec->Next();
				res = TRUE;
			}
			gw->setRndLimits(savedLimits);	
		}
		if (needsDel) tob->DeleteThis();
		return res;
	}
}

int Morph3DObj::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags, ModContext* mc)
{
	return 0;
}

void Morph3DObj::SelectSubComponent(HitRecord *hitRec, BOOL selected, BOOL all, BOOL invert)
{
	if (selLevel == SL_OBJECT){
		while (hitRec) {
			if (hitRec->hitInfo) {
				if (selected) SetFlag(MORPH_OB2SEL);
				else ClearFlag(MORPH_OB2SEL);
			} else {
				if (selected) SetFlag(MORPH_OB1SEL);
				else ClearFlag(MORPH_OB1SEL);
			}
			if (all) hitRec = hitRec->Next();
			else break;
		}
		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
		NotifyDependents(FOREVER,(PartID)this,REFMSG_BRANCHED_HISTORY_CHANGED);
		if (ip) SetupUI2();
	}
	else if (selLevel == SL_VERTEX){
		if (TestFlag(MORPH_ANCHOR1SEL)){
			ClearFlag(MORPH_ANCHOR1SEL);
			SetFlag(MORPH_ANCHOR2SEL);
			anchor_1 = hitRec->hitInfo;
		}
		else if (TestFlag(MORPH_ANCHOR2SEL)){
			ClearFlag(MORPH_ANCHOR2SEL);
			int anchor_2 = hitRec->hitInfo;
			TmpTriObject objects(ip->GetTime(), this);
			Point3 p1 = GetOpTM(ip->GetTime(),0) * objects[0].getVert(anchor_1);
			Point3 p2 = GetOpTM(ip->GetTime(),1) * objects[1].getVert(anchor_2);
			MorphEngine::Instance()->AddAnchorPoint(p1, p2);
			//EnableCreateAnchorButton();
		}
	}
}

DWORD Morph3DObj::GetSelLevel () {
	switch (selLevel) {
		case SL_OBJECT: return IMESHSEL_OBJECT;
		case SL_VERTEX: return IMESHSEL_VERTEX;
	}
	return IMESHSEL_OBJECT;

}

void Morph3DObj::SetSelLevel (DWORD sl) {
	if (sl==IMESHSEL_OBJECT)
		selLevel = SL_OBJECT;
	else if (sl==IMESHSEL_VERTEX)
		selLevel = SL_VERTEX;
	if (ip) ip->SetSubObjectLevel(selLevel);
}

void Morph3DObj::ClearSelection(int selLevel)
{
	if (selLevel==SL_OBJECT)
	{
		ClearFlag(MORPH_OB1SEL|MORPH_OB2SEL|MORPH_ANCHOR1SEL|MORPH_ANCHOR2SEL);
		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
		if (ip) SetupUI2();
	}
	else if (selLevel==SL_VERTEX){
		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
		if (ip) SetupUI2();
	}
}

void Morph3DObj::LocalDataChanged ()
{
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	if (!ip) return;	
	ip->ClearCurNamedSelSet();
}

int Morph3DObj::SubObjectIndex(HitRecord *hitRec)
{
	return hitRec->hitInfo;
}

void Morph3DObj::GetSubObjectCenters(SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc)
{
	Matrix3 tm;
	if (TestFlag(MORPH_OB1SEL)) {
		tm = GetOpTM(t,0) * node->GetObjectTM(t);
		cb->Center(tm.GetTrans(),0);
	}
	if (TestFlag(MORPH_OB2SEL)) {
		tm = GetOpTM(t,1) * node->GetObjectTM(t);
		cb->Center(tm.GetTrans(),1);
	}
}

void Morph3DObj::GetSubObjectTMs(SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc)
{
	Matrix3 tm;
	if (TestFlag(MORPH_OB1SEL)) {
		tm = GetOpTM(t,0) * node->GetObjectTM(t);
		cb->TM(tm,0);
	}
	if (TestFlag(MORPH_OB2SEL)) {
		tm = GetOpTM(t,1) * node->GetObjectTM(t);
		cb->TM(tm,1);
	}
}

void Morph3DObj::ActivateSubobjSel(int level, XFormModes& modes)
{
	ModContextList mcList;
	INodeTab nodes;
	if (!ip) return;
	selLevel = level;
	if (level == SL_OBJECT) {
		ClearFlag(MORPH_ANCHOR1SEL|MORPH_ANCHOR2SEL);
		modes = XFormModes(moveMode,rotMode,nuscaleMode,uscaleMode,squashMode,selectMode);
		NotifyDependents(
			FOREVER, 
			PART_SUBSEL_TYPE|PART_DISPLAY, 
			REFMSG_CHANGE);		
		ip->PipeSelLevelChanged();
		SetMeshVertSel(false);
	}
	else if (level == SL_VERTEX){
		ClearFlag(MORPH_ANCHOR2SEL);
		SetFlag(MORPH_ANCHOR1SEL);	
		modes = XFormModes(moveMode,rotMode,nuscaleMode,uscaleMode,squashMode,selectMode);
		SetMeshVertSel(true);
		NotifyDependents(FOREVER,PART_SELECT|PART_SUBSEL_TYPE|PART_DISPLAY,REFMSG_CHANGE);
	}
	else{
		SetMeshVertSel(true);
		ClearFlag(MORPH_ANCHOR1SEL|MORPH_ANCHOR2SEL);
	}
	ip->RedrawViews (ip->GetTime());
}

void Morph3DObj::SetMeshVertSel(bool bSet)
{
	TmpTriObject objects(ip->GetTime(), this);
	objects[0].dispFlags = 0;
	objects[0].SetDispFlag(bSet ? (DISP_VERTTICKS|DISP_SELVERTS) : 0);
	objects[0].selLevel = bSet ? MESH_VERTEX : MESH_OBJECT;
	objects[1].dispFlags = 0;
	objects[1].SetDispFlag(bSet ? (DISP_VERTTICKS|DISP_SELVERTS) : 0);
	objects[1].selLevel = bSet ? MESH_VERTEX : MESH_OBJECT;
}

IOResult Morph3DObj::Save(ISave *isave)
{
	ULONG nbWritten;
	isave->BeginChunk (MORPH_VERSION_CHUNK);
	if (isave->Write(&version, sizeof(int), &nbWritten)!=IO_OK) goto IOError;
	isave->EndChunk();

	isave->BeginChunk(MORPH_FLAGS_CHUNK);		
	if (isave->Write(&flags,sizeof(flags),&nbWritten)!=IO_OK) goto IOError;
	isave->EndChunk();

	isave->BeginChunk(MORPH_OPANAME_CHUNK);		
	if (isave->WriteWString(opaName)!=IO_OK) goto IOError;
	isave->EndChunk();

	isave->BeginChunk(MORPH_OPBNAME_CHUNK);
	if (isave->WriteWString(opbName)!=IO_OK) goto IOError;
	isave->EndChunk();

	isave->BeginChunk(MORPH_OFFSET_CHUNK);
	if (obOffset.Save(isave)!=IO_OK) goto IOError;
	isave->EndChunk();	

	isave->BeginChunk(MORPH_ENGINE_CHUNK);
	if (MorphEngine::Instance()->Save(isave)!=IO_OK) goto IOError;
	isave->EndChunk();

	return IO_OK;

IOError:
	isave->EndChunk();
	return IO_ERROR;
}


IOResult Morph3DObj::Load(ILoad *iload)
{
	ULONG nbRead;
	IOResult res = IO_OK;

	// Default names
	opaName = GetString(IDS_RB_OPERAND);
	opbName = GetString(IDS_RB_OPERAND);
	version = 0;

	while (IO_OK==(res=iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
			case MORPH_VERSION_CHUNK:
				res = iload->Read(&version,sizeof(int),&nbRead);
				break;

			case MORPH_OFFSET_CHUNK:
				obOffset.Load(iload);
				break;

			case MORPH_FLAGS_CHUNK:
				res=iload->Read(&flags,sizeof(flags),&nbRead);
				break;

			case MORPH_OPANAME_CHUNK: {
				TCHAR *buf;
				res=iload->ReadWStringChunk(&buf);
				opaName = TSTR(buf);
				break;
			}
			case MORPH_OPBNAME_CHUNK: {
				TCHAR *buf;
				res=iload->ReadWStringChunk(&buf);
				opbName = TSTR(buf);
				break;
			}
			case MORPH_ENGINE_CHUNK: {
				res = MorphEngine::Instance()->Load(iload);
				break;
			}
		}
		iload->CloseChunk();
		if (res!=IO_OK)  return res;
	}
	Invalidate();
	return IO_OK;
}

/*
int DllExport CalcMorphMesh(Mesh &meshOut, Mesh &mesh1, Mesh &mesh2, int resolution, bool smooth, int numop, float coeff1, float coeff2, float coeff, MeshOpProgress *prog = NULL,
							Matrix3 *tm1 = NULL, Matrix3 *tm2 = NULL, int whichInv = 0)
{
	static VShape *shape1, *shape2;
	static int resolution_s = -1;
	static bool bInit = false;
	static VShape *shapeOut = new VShape();
	if (resolution != resolution_s)
		bInit = false;
	if (!bInit){	
		if (shape1) delete shape1;
		if (shape2) delete shape2;
		resolution_s = resolution;
		shape1 = new VShape();
		shape2 = new VShape();
		shape1->Init(resolution, mesh1);
		shape2->Init(resolution, mesh2);
		bInit = true;
	}
	shapeOut->Interpolate(shape1, shape2, coeff);
	shapeOut->GetMesh(meshOut, smooth, numop, coeff1, coeff2);
	return 1;
}
*/

BOOL Morph3DObj::UpdateMesh(TimeValue t,BOOL force,BOOL sel)
{
	if (MorphEngine::Instance()->IsInitialized() && MorphEngine::Instance()->AnchorListSize()>4){
		MorphEngine::Instance()->ValidateAnchorPoints();
		MorphEngine::Instance()->SetMorphingMode(EMT_RigidOnly);
	}
	else
		MorphEngine::Instance()->SetMorphingMode(EMT_None);

	if (((!ivalid.InInterval(t) || TestFlag(MORPH_NEEDSUPDATE)) &&
		(TestFlag(MORPH_UPDATEALWAYS) || 
		(TestFlag(MORPH_UPDATESELECT)& sel) || 
		(TestFlag(MORPH_UPDATERENDER) && TestFlag(MORPH_INRENDER)) ||
		force)))
	{
		ClearFlag(MORPH_NEEDSUPDATE);

		// Build the result mesh
		ivalid = FOREVER;

		BOOL needsDel1, needsDel2;
		TriObject *tob1, *tob2;
		tob1 = GetTriObject(t,ob1,ivalid,needsDel1);
		tob2 = GetTriObject(t,ob2,ivalid,needsDel2);

		if (tob1 && !MorphEngine::Instance()->GetMesh1()){
			Mesh *m = &tob1->GetMesh();
			m->buildBoundingBox();
			Box3 box = m->getBoundingBox();
			box.pmin = GetOpTM(ip ? ip->GetTime() : 0, 0) * box.pmin;
			box.pmax = GetOpTM(ip ? ip->GetTime() : 0, 0) * box.pmax;
			MorphEngine::Instance()->SetMesh1(m, box);
		}

		if (tob2 && !MorphEngine::Instance()->GetMesh2()){
			Mesh *m = &tob2->GetMesh();
			m->buildBoundingBox();
			Box3 box = m->getBoundingBox();
			box.pmin = GetOpTM(ip ? ip->GetTime() : 0, 1) * box.pmin;
			box.pmax = GetOpTM(ip ? ip->GetTime() : 0, 1) * box.pmax;
			MorphEngine::Instance()->SetMesh2(m, box);
		}

		if (tob1 && tob2) {
			HCURSOR hCur = SetCursor(LoadCursor(NULL,IDC_WAIT));
			Matrix3 mat1 = GetOpTM(t,0,&ivalid);
			Matrix3 mat2 = GetOpTM(t,1,&ivalid);
			float coeff;
			GetAsyncKeyState(VK_ESCAPE);
			ClearFlag(MORPH_ABORTED);
			pblock->GetValue(PB_COEFF, t, coeff, ivalid);
			bool res = MorphEngine::Instance()->GetResultMesh(currentMesh, coeff);
			SetCursor(hCur);
			if (!res) {
				// Morphing Interpolation Failed!!!
				if (ip) {
					if (TestFlag(MORPH_ABORTED)) {
						ip->DisplayTempPrompt(GetString(IDS_RB_MORPHABORTED),500);
					} else {
						ip->DisplayTempPrompt(GetString(IDS_RB_INVALIDMORPH),500);
					}
				}
				if (TestFlag(MORPH_ABORTED)) {
					// Put in manual update mode.
					ClearFlag(MORPH_UPDATEALWAYS|MORPH_UPDATERENDER|MORPH_UPDATEMANUAL|MORPH_UPDATESELECT);
					SetFlag(MORPH_UPDATEMANUAL);					
					if (ip && editOb==this) SetupUI2();
				}
				if (TestFlag(MORPH_FIRSTUPDATE)) {
					// Turn on show operands.
					ClearFlag(MORPH_DISPRESULT);
				}
				//RB: 2/29/96:
				// If the boolean op failed don't set the validity to be empty.
				// this will just cause it to re-evaluate over and over again.
				// When the user changes it in some way which may allow the boolen
				// to succeed, that change should invalidate the interval.
				//ivalid.SetEmpty();
			} else {
				if (ip) ip->DisplayTempPrompt(GetString(IDS_RB_MORPHCOMPLETED),500);
				ClearFlag(MORPH_FIRSTUPDATE);
				currentMesh->InvalidateEdgeList();
				currentMesh->InvalidateGeomCache();
				currentMesh->InvalidateTopologyCache();
			}
		}
		//if (needsDel1 && tob1->IsObjectLocked()==0) tob1->DeleteThis();
		//if (needsDel2 && tob2->IsObjectLocked()==0) tob2->DeleteThis();
		if (needsDel1) tob1->DeleteThis();
		if (needsDel2) tob2->DeleteThis();
	} else {
		// RB 4-2-96: We must be set to manual update.
		// The object should be considered valid
		if (!ivalid.InInterval(t)) {			
			ivalid.SetInstant(t);
			SetFlag(MORPH_NEEDSUPDATE);
		}		
	}

	return ivalid.InInterval(t);
}

Interval Morph3DObj::ObjectValidity(TimeValue t)
{
	UpdateMesh(t);
	if (ivalid.Empty()) return Interval(t,t);
	else return ivalid;
}

int Morph3DObj::CanConvertToType(Class_ID obtype)
{
	if (obtype==defObjectClassID||obtype==triObjectClassID||obtype==mapObjectClassID) {
		if (ob1 && ob2) return 1;
		else if (ob1) return ob1->CanConvertToType(obtype);
		else if (ob2) return ob2->CanConvertToType(obtype);
		else return 0;
	}
	return Object::CanConvertToType(obtype);
}

Object* Morph3DObj::ConvertToType(TimeValue t, Class_ID obtype)
{
	if (obtype==defObjectClassID||obtype==triObjectClassID||obtype==mapObjectClassID) {
		if (ob1 && ob2) {
			TriObject *triob;
			UpdateMesh(t);
			if (currentMesh){
				triob = CreateNewTriObject();
				triob->GetMesh() = *currentMesh;
				triob->SetChannelValidity(TOPO_CHAN_NUM,ObjectValidity(t));
				triob->SetChannelValidity(GEOM_CHAN_NUM,ObjectValidity(t));
				return triob;
			}
			else
				return NULL;
		} else {
			// RB 4-11-96:
			// There was a bug where if one of the operands was a tri-object
			// it would convert itself to a tri-object by just returning itself (as it should)
			// The problem is that there are places in the system where the
			// the system would think it needed to delete the tri-object
			// becuase it was not equal to the boolean object. In other words,
			// it thinks that the boolean convert itself to a tri-object and
			// therefore the tri-object was a temporary object.
			// So what this code does is clone the tri-object in this case
			// so that the boolean object will always return a temporary
			// object.
			Object *obj = NULL;
			if (ob1) {
				obj = ob1->ConvertToType(t,obtype);
				if (obj && (obj==ob1 || obj->IsObjectLocked())) {
					return (Object*)obj->Clone();
				} else {
					return obj;
				}
			}
			if (ob2) {
				obj = ob2->ConvertToType(t,obtype);
				if (obj && (obj==ob2 || obj->IsObjectLocked())) {
					return (Object*)obj->Clone();
				} else {
					return obj;
				}
			}			
			return NULL;
		}
	} else {
		return Object::ConvertToType(t,obtype);
	}
}

BOOL Morph3DObj::PolygonCount(TimeValue t, int& numFaces, int& numVerts) 
{
	UpdateMesh(t);
	if (currentMesh){
		numFaces = currentMesh->getNumFaces();
		numVerts = currentMesh->getNumVerts();
		return TRUE;
	}
	else{
		numFaces = 0;
		numVerts = 0;
		return FALSE;
	}
}

ObjectState Morph3DObj::Eval(TimeValue time)
{
	return ObjectState(this);
}

void Morph3DObj::SetupUI1()
{
	CheckRadioButton(hParams1,IDC_TARG_REFERENCE,IDC_TARG_INSTANCE,addOppMethod);
}

void Morph3DObj::SetupUI2(BOOL useName)
{	
	HWND hList = GetDlgItem(hParams2,IDC_MORPH_OPERANDS);
	SendMessage(hList,LB_RESETCONTENT,0,0);
	TSTR name = TSTR(_T("A: ")) + ((ob1||useName) ? opaName : TSTR(_T("")));
	SendMessage(hList,LB_ADDSTRING,0,(LPARAM)(const TCHAR*)name);
	name = TSTR(_T("B: ")) + ((ob2||useName) ? opbName : TSTR(_T("")));
	SendMessage(hList,LB_ADDSTRING,0,(LPARAM)(const TCHAR*)name);
	if (flags&MORPH_OB1SEL) {
		SendMessage(hList,LB_SETSEL,TRUE,0);
	}
	if (flags&MORPH_OB2SEL) {
		SendMessage(hList,LB_SETSEL,TRUE,1);
	}

	ICustEdit *edit = GetICustEdit(GetDlgItem(hParams2,IDC_MORPH_ANAME));
	edit->SetText(opaName);
	ReleaseICustEdit(edit);

	edit = GetICustEdit(GetDlgItem(hParams2,IDC_MORPH_BNAME));
	edit->SetText(opbName);
	ReleaseICustEdit(edit);

	CheckDlgButton(hParams2,IDC_MORPH_DISPRESULT,TestFlag(MORPH_DISPRESULT));
	CheckDlgButton(hParams2,IDC_MORPH_DISPOPS,!TestFlag(MORPH_DISPRESULT));	

	if (TestFlag(MORPH_UPDATERENDER)) CheckDlgButton(hParams2,IDC_MORPH_UPDATERENDER,TRUE);
	else CheckDlgButton(hParams2,IDC_MORPH_UPDATERENDER,FALSE);
	if (TestFlag(MORPH_UPDATEMANUAL)) CheckDlgButton(hParams2,IDC_MORPH_UPDATEMANUAL,TRUE);
	else CheckDlgButton(hParams2,IDC_MORPH_UPDATEMANUAL,FALSE);
	if (TestFlag(MORPH_UPDATEALWAYS)) CheckDlgButton(hParams2,IDC_MORPH_UPDATEALWAYS,TRUE);
	else CheckDlgButton(hParams2,IDC_MORPH_UPDATEALWAYS,FALSE);
	if (TestFlag(MORPH_UPDATESELECT)) CheckDlgButton(hParams2,IDC_MORPH_UPDATESELECT,TRUE);
	else CheckDlgButton(hParams2,IDC_MORPH_UPDATESELECT,FALSE);	

	//CheckDlgButton(hParams2,IDC_MORPH_SMOOTH,TestFlag(MORPH_SMOOTH));

	ICustButton *iBut = GetICustButton(GetDlgItem(hParams2,IDC_MORPH_RECALC));
	if (!TestFlag(MORPH_UPDATEALWAYS)) {
		iBut->Enable();
	} else {
		iBut->Disable();
	}
	ReleaseICustButton(iBut);		

	SetExtractButtonState();
}

void Morph3DObj::SetExtractButtonState()
{
	ICustButton *iBut = GetICustButton(GetDlgItem(hParams2,IDC_MORPH_EXTRACTOP));
	if (!creating && (flags & (MORPH_OB1SEL|MORPH_OB2SEL))) {
		iBut->Enable();
		EnableWindow(GetDlgItem(hParams2,IDC_MORPH_EXTRACT_INTANCE),TRUE);
		EnableWindow(GetDlgItem(hParams2,IDC_MORPH_EXTRACT_COPY),TRUE);
	} else {
		iBut->Disable();
		EnableWindow(GetDlgItem(hParams2,IDC_MORPH_EXTRACT_INTANCE),FALSE);
		EnableWindow(GetDlgItem(hParams2,IDC_MORPH_EXTRACT_COPY),FALSE);
	}
	ReleaseICustButton(iBut);	

	CheckDlgButton(hParams2,IDC_MORPH_EXTRACT_INTANCE,!extractCopy);
	CheckDlgButton(hParams2,IDC_MORPH_EXTRACT_COPY,extractCopy);
}

BOOL PickOperand::Filter(INode *node)
{
	if (node) {
		ObjectState os = node->GetObjectRef()->Eval(mo->ip->GetTime());
		if (os.obj->IsParticleSystem() || 
			os.obj->SuperClassID()!=GEOMOBJECT_CLASS_ID) {
				node = NULL;
				return FALSE;
		}

		node->BeginDependencyTest();
		mo->NotifyDependents(FOREVER,0,REFMSG_TEST_DEPENDENCY);
		if(node->EndDependencyTest()) {
			node = NULL;
			return FALSE;
		}		
	}

	return node ? TRUE : FALSE;
}

BOOL PickOperand::HitTest(IObjParam *ip,HWND hWnd,ViewExp *vpt,IPoint2 m,int flags)
{	
	INode *node = ip->PickNode(hWnd,m,this);

	if (node) {
		ObjectState os = node->GetObjectRef()->Eval(ip->GetTime());
		if (os.obj->SuperClassID()!=GEOMOBJECT_CLASS_ID) {
			node = NULL;
			return FALSE;
		}

		node->BeginDependencyTest();
		mo->NotifyDependents(FOREVER,0,REFMSG_TEST_DEPENDENCY);
		if(node->EndDependencyTest()) {
			node = NULL;
			return FALSE;
		}		
	}

	return node ? TRUE : FALSE;
}

BOOL PickOperand::Pick(IObjParam *ip,ViewExp *vpt)
{
	INode *node = vpt->GetClosestHit();
	assert(node);

	ModContextList mcList;
	INodeTab nodes;
	INode *ourNode = NULL;
	if (!mo->creating) {
		// Grab the node for the first intance of us.
		ip->GetModContexts(mcList,nodes);
		if (nodes.Count()) ourNode = nodes[0];
	}
	else
		ourNode = CreateMorph3DObjMode::Instance()->node;

	theHold.Begin();
	mo->SetOperandB (ip->GetTime(), node, ourNode, 1);		
	theHold.Accept(IDS_DS_CREATE);

	nodes.DisposeTemporary ();

	// Automatically check show result and do one update
	// mo->SetFlag(MORPH_DISPRESULT);
	mo->ClearFlag(MORPH_DISPRESULT);

	mo->SetFlag(MORPH_FIRSTUPDATE);
	CheckRadioButton(mo->hParams2,IDC_MORPH_DISPRESULT,IDC_MORPH_DISPOPS,IDC_MORPH_DISPRESULT);
	mo->UpdateMesh(ip->GetTime(),TRUE);
	mo->SetupUI2();	

	if (mo->creating) {
		CreateMorph3DObjMode::Instance()->JumpStart(ip,mo);
		ip->SetCommandMode(CreateMorph3DObjMode::Instance());
		ip->RedrawViews(ip->GetTime());
		return FALSE;
	} else {
		return TRUE;
	}
}

void PickOperand::EnterMode(IObjParam *ip)
{
	ICustButton *iBut = GetICustButton(GetDlgItem(mo->hParams1,IDC_PICK_MORPHOPERAND));
	if (iBut) iBut->SetCheck(TRUE);
	ReleaseICustButton(iBut);
}

void PickOperand::ExitMode(IObjParam *ip)
{
	ICustButton *iBut = GetICustButton(GetDlgItem(mo->hParams1,IDC_PICK_MORPHOPERAND));
	if (iBut) iBut->SetCheck(FALSE);
	ReleaseICustButton(iBut);
}

static INT_PTR CALLBACK Morph3DObjParamDlgProc1(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Morph3DObj *mo = (Morph3DObj*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
	if (!mo && message!=WM_INITDIALOG) return FALSE;

	switch (message) {
		case WM_INITDIALOG: {
			mo = (Morph3DObj*)lParam;
			SetWindowLongPtr(hWnd,GWLP_USERDATA,lParam);
			mo->hParams1 = hWnd;			

			ICustButton *iBut = GetICustButton(GetDlgItem(hWnd,IDC_PICK_MORPHOPERAND));
			iBut->SetType(CBT_CHECK);
			iBut->SetHighlightColor(GREEN_WASH);
			ReleaseICustButton(iBut);

			mo->SetupUI1();
			return FALSE;	// stop default keyboard focus - DB 2/27
							}

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
		case IDC_PICK_MORPHOPERAND:
			if (mo->ip->GetCommandMode()->ID() == CID_STDPICK) {
				if (mo->creating) {
					CreateMorph3DObjMode::Instance()->JumpStart(mo->ip,mo);
					mo->ip->SetCommandMode(CreateMorph3DObjMode::Instance());
				} else {
					mo->ip->SetStdCommandMode(CID_OBJMOVE);
				}
			} else {
				mo->pickCB.mo = mo;						
				mo->ip->SetPickMode(&mo->pickCB);
			}
			break;

		case IDC_TARG_REFERENCE:
		case IDC_TARG_COPY:
		case IDC_TARG_MOVE:
		case IDC_TARG_INSTANCE:
			mo->addOppMethod = LOWORD(wParam);
			break;				
			}
			break;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			mo->ip->RollupMouseMessage(hWnd,message,wParam,lParam);
			return FALSE;

		default:
			return FALSE;
	}
	return TRUE;
}

static INT_PTR CALLBACK Morph3DObjParamDlgProc2(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Morph3DObj *mo = (Morph3DObj*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
	if (!mo && message!=WM_INITDIALOG) return FALSE;

	switch (message) {
		case WM_INITDIALOG: {
			mo = (Morph3DObj*)lParam;
			SetWindowLongPtr(hWnd,GWLP_USERDATA,lParam);
			mo->hParams2 = hWnd;			
			mo->SetupUI2();
			return FALSE;	// DB 2/27
		}
		case WM_CUSTEDIT_ENTER: {
			ICustEdit *edit;
			TCHAR buf[256];
			switch (LOWORD(wParam)) {
				case IDC_MORPH_ANAME:
				edit = GetICustEdit(GetDlgItem(hWnd,IDC_MORPH_ANAME));					
				edit->GetText(buf,256);
				mo->opaName = TSTR(buf);					
				if (mo->ob1) mo->ob1->NotifyDependents(FOREVER,PART_ALL,REFMSG_NODE_NAMECHANGE,TREE_VIEW_CLASS_ID);
				break;
			case IDC_MORPH_BNAME:
				edit = GetICustEdit(GetDlgItem(hWnd,IDC_MORPH_BNAME));					
				edit->GetText(buf,256);
				mo->opbName = TSTR(buf);
				if (mo->ob2) mo->ob2->NotifyDependents(FOREVER,PART_ALL,REFMSG_NODE_NAMECHANGE,TREE_VIEW_CLASS_ID);
				break;					
				}
			mo->SetupUI2();			
			break;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam)) {				
			case IDC_MORPH_RECALC:
				mo->ivalid.SetEmpty();
				mo->UpdateMesh(mo->ip->GetTime(),TRUE);
				mo->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
				mo->ip->RedrawViews(mo->ip->GetTime());
				break;

			case IDC_MORPH_DISPOPS:
			case IDC_MORPH_DISPRESULT:
				if (IsDlgButtonChecked(hWnd,IDC_MORPH_DISPRESULT)) {						
					mo->SetFlag(MORPH_DISPRESULT);
				} else {
					mo->ClearFlag(MORPH_DISPRESULT);
				}
				mo->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
				mo->ip->RedrawViews(mo->ip->GetTime());
				break;

			case IDC_MORPH_UPDATEALWAYS:
				mo->ClearFlag(MORPH_UPDATEALWAYS|MORPH_UPDATERENDER|MORPH_UPDATEMANUAL|MORPH_UPDATESELECT);
				mo->SetFlag(MORPH_UPDATEALWAYS);					
				mo->ip->RedrawViews(mo->ip->GetTime());
				mo->SetupUI2();
				break;

			case IDC_MORPH_UPDATESELECT:
				mo->ClearFlag(MORPH_UPDATEALWAYS|MORPH_UPDATERENDER|MORPH_UPDATEMANUAL|MORPH_UPDATESELECT);
				mo->SetFlag(MORPH_UPDATESELECT);					
				mo->ip->RedrawViews(mo->ip->GetTime());
				mo->SetupUI2();
				break;				

			case IDC_MORPH_UPDATERENDER:
				mo->ClearFlag(MORPH_UPDATEALWAYS|MORPH_UPDATERENDER|MORPH_UPDATEMANUAL|MORPH_UPDATESELECT);
				mo->SetFlag(MORPH_UPDATERENDER);					
				mo->SetupUI2();
				break;

			case IDC_MORPH_UPDATEMANUAL:
				mo->ClearFlag(MORPH_UPDATEALWAYS|MORPH_UPDATERENDER|MORPH_UPDATEMANUAL|MORPH_UPDATESELECT);
				mo->SetFlag(MORPH_UPDATEMANUAL);					
				mo->SetupUI2();
				break;
//			case IDC_MORPH_SMOOTH:
//				if (IsDlgButtonChecked(hWnd,IDC_MORPH_SMOOTH)) {
//					mo->SetFlag(MORPH_SMOOTH);
//				} else {
//					mo->ClearFlag(MORPH_SMOOTH);
//				}
//				mo->Invalidate();
//				mo->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
//				mo->ip->RedrawViews(mo->ip->GetTime());
//				break;
			case IDC_MORPH_OPERANDS:
				if (HIWORD(wParam)==LBN_SELCHANGE) {
					mo->flags &= ~MORPH_ANYSEL;
					if (SendMessage((HWND)lParam,LB_GETSEL,0,0)) {
						mo->flags |= MORPH_OB1SEL;
					}
					if (SendMessage((HWND)lParam,LB_GETSEL,1,0)) {
						mo->flags |= MORPH_OB2SEL;
					}
					mo->NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
					mo->NotifyDependents(FOREVER,(PartID) mo,REFMSG_BRANCHED_HISTORY_CHANGED);
					mo->ip->RedrawViews(mo->ip->GetTime());
					mo->SetExtractButtonState();
				}
				break;

			case IDC_MORPH_EXTRACT_INTANCE:
				mo->extractCopy = FALSE;
				break;
			case IDC_MORPH_EXTRACT_COPY:
				mo->extractCopy = TRUE;
				break;
			case IDC_MORPH_EXTRACTOP:
				theHold.Begin();
				if (mo->flags&MORPH_OB1SEL) mo->ExtractOperand(0);
				if (mo->flags&MORPH_OB2SEL) mo->ExtractOperand(1);
				theHold.Accept(GetString(IDS_RB_EXTRACTOP));
				mo->ip->RedrawViews(mo->ip->GetTime());
				break;
			}
			break;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			mo->ip->RollupMouseMessage(hWnd,message,wParam,lParam);
			return FALSE;
		default:
			return FALSE;
	}
	return TRUE;
}

void Morph3DObj::BeginEditParams(IObjParam *ip, ULONG flags,Animatable *prev)
{	
	this->ip = ip;	
	editOb   = this;

	if (flags&BEGIN_EDIT_CREATE) {
		creating = TRUE;
	} 
	else {
		creating = FALSE;
		// Create sub object editing modes.
		moveMode       = new MoveModBoxCMode(this,ip);
		rotMode        = new RotateModBoxCMode(this,ip);
		uscaleMode     = new UScaleModBoxCMode(this,ip);
		nuscaleMode    = new NUScaleModBoxCMode(this,ip);
		squashMode     = new SquashModBoxCMode(this,ip);
		selectMode     = new SelectModBoxCMode(this,ip);

		// Add our sub object type
		// TSTR type(GetString(IDS_RB_OPERANDS));
		// const TCHAR *ptype[] = {type};
		// This call is obsolete. Please see BaseObject::NumSubObjTypes() and BaseObject::GetSubObjType()
		// 	ip->RegisterSubObjectTypes(ptype, 1);
	}

	if (!hParams1) {
		hParams1 = ip->AddRollupPage( 
			hInst3DSMax, 
			MAKEINTRESOURCE(IDD_MORPH3DOBJ_PARAM1),
			Morph3DObjParamDlgProc1, 
			GetString(IDS_RB_PICKMORPH), 
			(LPARAM)this);		
		ip->RegisterDlgWnd(hParams1);
		hParams2 = ip->AddRollupPage( 
			hInst3DSMax, 
			MAKEINTRESOURCE(IDD_MORPH3DOBJ_PARAM2),
			Morph3DObjParamDlgProc2, 
			GetString(IDS_PARAMETERS), 
			(LPARAM)this);		
		ip->RegisterDlgWnd(hParams1);	
	} else {
		SetWindowLongPtr(hParams1,GWLP_USERDATA,(LONG_PTR)this);
		SetWindowLongPtr(hParams2,GWLP_USERDATA,(LONG_PTR)this);
		SetupUI1();
		SetupUI2();
	}
	pmapParam = CreateCPParamMap(
		descParam, PARAMDESC_LENGH,
		pblock,
		ip,
		hInst3DSMax,
		MAKEINTRESOURCE(IDD_MORPH3DOBJ_PARAM3),
		GetString(IDS_PARAMETERS),
		0);	
}

void Morph3DObj::EndEditParams(IObjParam *ip, ULONG flags,Animatable *next)
{	
	editOb = NULL;

	if (flags&END_EDIT_REMOVEUI) {
		ip->UnRegisterDlgWnd(hParams1);
		ip->DeleteRollupPage(hParams1);
		hParams1 = NULL;
		ip->UnRegisterDlgWnd(hParams2);
		ip->DeleteRollupPage(hParams2);
		hParams2 = NULL;
	}
	else {
		SetWindowLongPtr(hParams1,GWLP_USERDATA,(LONG)NULL);
		SetWindowLongPtr(hParams2,GWLP_USERDATA,(LONG)NULL);
	}	
	if (!creating) {
		ip->DeleteMode(moveMode);
		ip->DeleteMode(rotMode);
		ip->DeleteMode(uscaleMode);
		ip->DeleteMode(nuscaleMode);
		ip->DeleteMode(squashMode);
		ip->DeleteMode(selectMode);
		if ( moveMode ) delete moveMode;
		moveMode = NULL;
		if ( rotMode ) delete rotMode;
		rotMode = NULL;
		if ( uscaleMode ) delete uscaleMode;
		uscaleMode = NULL;
		if ( nuscaleMode ) delete nuscaleMode;
		nuscaleMode = NULL;
		if ( squashMode ) delete squashMode;
		squashMode = NULL;
		if ( selectMode ) delete selectMode;
		selectMode = NULL;	
	}
	ip->ClearPickMode();
	ip = NULL;
	creating = FALSE;
	DestroyCPParamMap(pmapParam);
	pmapParam = NULL;
}

int Morph3DObj::NumSubObjTypes() 
{ 
	return 2;
}

static GenSubObjType SOT_Operands(1);
static GenSubObjType SOT_Anchor(2);

ISubObjType *Morph3DObj::GetSubObjType(int i) 
{
	static bool initialized = false;
	if(!initialized)
	{
		initialized = true;
		SOT_Operands.SetName(GetString(IDS_RB_OPERANDS));
		SOT_Anchor.SetName(GetString(IDS_RB_ANCHOR));
	}

	switch(i)
	{
	case 0:
		return &SOT_Operands;
	case 1:
		return &SOT_Anchor;
	}
	return NULL;
}

void TmpTriObject::CheckInit(int index)
{
	if (index==0 && !bInit1){
		Object *ob = parent->GetPipeObj(t,0);
		Interval ivalid = FOREVER;
		tob1 = GetTriObject(t,ob,ivalid,bNeedsDel1);
		mesh1 = &tob1->GetMesh();
		bInit1 = true;
	}
	else if (index!=0 && !bInit2){
		Object *ob = parent->GetPipeObj(t,1);
		Interval ivalid = FOREVER;
		tob2 = GetTriObject(t,ob,ivalid,bNeedsDel2);
		mesh2 = &tob2->GetMesh();
		bInit2 = true;
	}
}

TmpTriObject::TmpTriObject(TimeValue t, Morph3DObj *parent):
					t(t),
					parent(parent),
					bInit1(false),
					bInit2(false),
					bNeedsDel1(false),
					bNeedsDel2(false)
{
}

TmpTriObject::~TmpTriObject()
{
	if (bInit1 && bNeedsDel1)
		tob1->DeleteThis();
	if (bInit2 && bNeedsDel2)
		tob2->DeleteThis();
}

TriObject &TmpTriObject::operator()(int index)
{
	CheckInit(index);
	return index ? (*tob2) : (*tob1);
}

Mesh &TmpTriObject::operator[](int index)
{
	CheckInit(index);
	return index ? (*mesh2) : (*mesh1);
}
