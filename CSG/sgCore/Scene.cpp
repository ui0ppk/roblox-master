#include "CORE//sg.h"
#include "UndoRedo.h"

#include <list>
#include <vector>

static    CommandsGroup*    current_undo_redo_group = NULL;

static    std::list<CommandsGroup*>    undo_list;
static    std::list<CommandsGroup*>    redo_list;

sgCScene*  scene=NULL;

class  SceneObjectsList : public IObjectsList
{
public:
	virtual  int         GetCount()	const
	{
		return (objects.num);
	};
	virtual  sgCObject*  GetHead()	const
	{
		if (objects.hhead==NULL)
			return NULL;
		return ((lpOBJ)objects.hhead)->extendedClass;
	};
	virtual  sgCObject*  GetNext(sgCObject* c_obj) const
	{
		hOBJ hobj,hnext;
		hobj = GetObjectHandle(c_obj);
		get_next_item_z(OBJ_LIST,hobj, &hnext);  //  
		if (hnext==NULL)
			return NULL;
		return ((lpOBJ)hnext)->extendedClass;
	};
	virtual  sgCObject*  GetTail()	const
	{
		if (objects.htail==NULL)
			return NULL;
		return ((lpOBJ)objects.htail)->extendedClass;
	};
	virtual  sgCObject*  GetPrev(sgCObject* c_obj) const
	{
		hOBJ hobj,hprev;
		hobj = GetObjectHandle(c_obj);
		get_prev_item_z(OBJ_LIST,hobj, &hprev);  //  
		if (hprev==NULL)
			return NULL;
		return ((lpOBJ)hprev)->extendedClass;
	}
};


class  SceneSelectedObjectsList : public IObjectsList
{
public:
	virtual  int         GetCount()	const
	{
		return (selected.num);
	};
	virtual  sgCObject*  GetHead()	const
	{
		if (selected.hhead==NULL)
			return NULL;
		return ((lpOBJ)selected.hhead)->extendedClass;
	};
	virtual  sgCObject*  GetNext(sgCObject* c_obj) const
	{
		hOBJ hobj,hnext;
		hobj = GetObjectHandle(c_obj);
		get_next_item_z(SEL_LIST, hobj, &hnext);
		if (hnext==NULL)
			return NULL;
		return ((lpOBJ)hnext)->extendedClass;
	};
	virtual  sgCObject*  GetTail()	const
	{
		if (selected.htail==NULL)
			return NULL;
		return ((lpOBJ)selected.htail)->extendedClass;
	};
	virtual  sgCObject*  GetPrev(sgCObject* c_obj) const
	{
		hOBJ hobj,hprev;
		hobj = GetObjectHandle(c_obj);
		get_prev_item_z(SEL_LIST,hobj, &hprev);  //  
		if (hprev==NULL)
			return NULL;
		return ((lpOBJ)hprev)->extendedClass;
	}
};

sgCScene* sgCScene::GetScene()
{
	if(scene==NULL)
		scene = new sgCScene;
	return scene;
}

sgCScene::sgCScene()
{
	m_objects_list = new SceneObjectsList;
	m_selected_objects_list = new SceneSelectedObjectsList;
}

sgCScene::~sgCScene()
{
	delete m_objects_list;
	delete m_selected_objects_list;
	if (temp_font)
	{
		delete temp_font;
		temp_font = NULL;
	}
}

IObjectsList*  sgCScene::GetObjectsList() const 
{return m_objects_list;};

IObjectsList*  sgCScene::GetSelectedObjectsList() const
{ return m_selected_objects_list;};

bool sgCScene::AttachObject(sgCObject* obj)
{
	if (!obj)
		return false;

	lpOBJ lll = (lpOBJ)GetObjectHandle(obj);
	if (lll->isAttachedToScene)
		return false;

	if (!fc_assign_curr_ident(GetObjectHandle(obj))) 
		return false;
	if (!attach_and_regen(GetObjectHandle(obj)))
		return false;

	if (current_undo_redo_group)
	{
		UR_ELEMENT   tmp_UR;
		tmp_UR.obj = GetObjectHandle(obj);
		tmp_UR.op_type = ADD_OBJ;
		//tmp_UR.matrix = new sgFloat[16];
		current_undo_redo_group->AddElement(tmp_UR);
	}
	return true;
}

void sgCScene::DetachObject(sgCObject* obj)
{
	if (!obj)
		return;

	lpOBJ lll = (lpOBJ)GetObjectHandle(obj);
	if (!lll->isAttachedToScene)
		return;
	obj->Select(false);
	detach_item_z(OBJ_LIST, &objects,GetObjectHandle(obj)); 
	detached_objects.push_back(obj);

	if (current_undo_redo_group)
	{
		UR_ELEMENT   tmp_UR;
		tmp_UR.obj = GetObjectHandle(obj);
		tmp_UR.op_type = DELETE_OBJ;
		//tmp_UR.matrix = new sgFloat[16];
		current_undo_redo_group->AddElement(tmp_UR);
	}
}

/***************************************************************************/
/***************************************************************************/
/************************************************************************/
/* UNDO_REDO                                                            */
/************************************************************************/
bool sgCScene::StartUndoGroup()
{
	if (current_undo_redo_group!=NULL)
	{
		assert(0);
		return false;
	}
	current_undo_redo_group = new CommandsGroup;
	return true;
}

bool sgCScene::EndUndoGroup()
{
	if (current_undo_redo_group==NULL)
	{
		assert(0);
		return false;
	}

	current_undo_redo_group->ChangeOrder();
	undo_list.push_back(current_undo_redo_group);

	current_undo_redo_group = NULL;

	std::list<CommandsGroup*>::iterator Iter;

	for ( Iter = redo_list.begin( ); Iter != redo_list.end( ); Iter++ )
		delete *Iter;
	redo_list.clear();

	return true;
}

bool sgCScene::IsUndoStackEmpty() const  {	return  undo_list.empty();}
bool sgCScene::IsRedoStackEmpty() const  {	return  redo_list.empty();}


bool sgCScene::Undo()
{
	if (undo_list.empty()) 
	{	
		assert(0);	
		return false;	
	}

	std::list<CommandsGroup*>::iterator Iter;

	Iter = undo_list.end( );
	Iter--;

	CommandsGroup* tmp_group = *Iter;

	tmp_group->Execute();
	tmp_group->Inverse();

	redo_list.push_back(tmp_group);
	undo_list.pop_back();
	
	return true;
}

bool sgCScene::Redo()
{
	if (redo_list.empty()) {	assert(0);	return false;	}

	std::list<CommandsGroup*>::iterator Iter;

	Iter = redo_list.end( );
	Iter--;

	CommandsGroup* tmp_group = *Iter;

	tmp_group->Execute();
	tmp_group->Inverse();

	undo_list.push_back(tmp_group);
	redo_list.pop_back();

	return true;
}
/***************************************************************************/
/***************************************************************************/
/********************************************************************************/

void sgCScene::Clear()
{
	std::list<CommandsGroup*>::iterator Iter;

	for ( Iter = redo_list.begin( ); Iter != redo_list.end( ); Iter++ )
		delete *Iter;
	redo_list.clear();

	for ( Iter = undo_list.begin( ); Iter != undo_list.end( ); Iter++ )
		delete *Iter;
	undo_list.clear();

	if (temp_font)
	{
		delete temp_font;
		temp_font = NULL;
	}

	hOBJ  hobj, hnext;
	//      objects;
	hobj = objects.hhead;
	while (hobj) 
	{
		get_next_item(hobj, &hnext);
		o_free(hobj, &objects);
		hobj = hnext;
	}

	for (std::list<sgCObject*>::iterator i = detached_objects.begin(), 
				j = detached_objects.end(); i != j; ++i)
	{
		if ((*i)->GetParent()!=NULL)
		{
			assert(0);
		}
		o_free(GetObjectHandle(*i), NULL);
	}
	detached_objects.clear();

	if (application_interface && 
		application_interface->GetSceneTreeControl())
			application_interface->GetSceneTreeControl()->ClearTree();


	init_listh(&objects);
	init_listh(&selected);
	D_POINT min = {-100, -100, -100}, max = {100, 100, 100};
	if (!objects.hhead) 
	{
		scene_min.x = scene_min.y = scene_min.z =  GAB_NO_DEF;
		scene_max.x = scene_max.y = scene_max.z = -GAB_NO_DEF;
		modify_limits(&min, &max);
	}
	free_blocks_list();
	free_np_brep_list();

	init_vdim(&vd_blocks, sizeof(IBLOCK));
	init_vdim(&vd_brep,   sizeof(LNP));
	//init_camera_array();
}

void sgCScene::GetGabarits(SG_POINT& p_min, SG_POINT& p_max)
{
	//get_scene_limits();
	memcpy(&p_min,&scene_min,sizeof(SG_POINT));
	memcpy(&p_max,&scene_max,sizeof(SG_POINT));
}


/************************************************************************/
/*                                                 */
/************************************************************************/

bool sgCObject::ApplyTempMatrix()
{
	if (!m_temp_matrix)
	{
		assert(0);
		return false;
	}
	o_trans(m_object_handle,const_cast<sgFloat*>(m_temp_matrix->GetData()));
	if (!get_object_limits(m_object_handle,reinterpret_cast<D_POINT*>(&m_min),
		reinterpret_cast<D_POINT*>(&m_max)))
	{
		assert(0);
	}
	if (m_min.x<scene_min.x || m_min.y<scene_min.y || m_min.z<scene_min.z ||
		m_max.x>scene_max.x || m_max.y>scene_max.y || m_max.z>scene_max.z)
			get_scene_limits();

	if (current_undo_redo_group && !ignore_undo_redo)
	{
		UR_ELEMENT   tmp_UR;
		tmp_UR.obj = m_object_handle;
		tmp_UR.op_type = TRANS_OBJ;
		tmp_UR.matrix = new sgFloat[16];
		sgCMatrix mmm(m_temp_matrix->GetData());
		mmm.Inverse();
		memcpy(tmp_UR.matrix, mmm.GetData(), sizeof(MATR));
		current_undo_redo_group->AddElement(tmp_UR);
	}

	return true;
}

sgCGroup*  sgCGroup::CreateGroup(sgCObject** objcts, int cnt)
{
	if (objcts==NULL || cnt==0)
	{
		assert(0);
		return NULL;
	}

	hOBJ		/*hobj,*/ hgroup;
	lpOBJ		obj;
	GEO_GROUP	group;

	bool        allAttached = false;

	if (((lpOBJ)GetObjectHandle(objcts[0]))->isAttachedToScene)
		allAttached = true;

	for (int i=0;i<cnt;i++)
	{
		if ((((lpOBJ)GetObjectHandle(objcts[i]))->isAttachedToScene)!=allAttached)
			return NULL;
	}

	//  
	memset(&group,0,sizeof(GEO_GROUP));
	o_hcunit(group.matr);
	if ((hgroup = create_simple_obj_loc(OGROUP, &group)) == NULL) 
		return NULL;
	for (int i=0;i<cnt;i++)
	{
		assert(objcts[i]);
		detach_item_z(SEL_LIST,&selected,GetObjectHandle(objcts[i]));
		//detach_item_z(OBJ_LIST, &objects,objcts[i]->m_object_handle);
		if (((lpOBJ)GetObjectHandle(objcts[i]))->isAttachedToScene)
		{
			sgGetScene()->DetachObject(objcts[i]);
			detached_objects.pop_back();
		}
		obj = (lpOBJ)GetObjectHandle(objcts[i]);
		obj->status &= (~(ST_SELECT|ST_BLINK));
		obj->hhold = hgroup;
		attach_item_tail_z(OBJ_LIST,&group.listh,GetObjectHandle(objcts[i]));
	}
	obj = (lpOBJ)hgroup;
	memcpy(obj->geo_data, &group, sizeof(GEO_GROUP));

	sgCGroup* retGr = new sgCGroup();

	SetObjectHandle(retGr,hgroup);
	obj->extendedClass = retGr;

	char* typeID = "{0000000000000-0000-0000-000000000010}";
	set_hobj_attr_value(id_TypeID, hgroup, typeID);

	retGr->PostCreate();

	if (current_undo_redo_group && allAttached)
	{
		UR_ELEMENT   tmp_UR;
		tmp_UR.obj = hgroup;
		tmp_UR.op_type = CREATE_GROUP;
		tmp_UR.matrix = NULL;
		tmp_UR.m_children.reserve(cnt);
		for (int i=0;i<cnt;i++)
			tmp_UR.m_children.push_back(GetObjectHandle(objcts[i]));
		current_undo_redo_group->AddElement(tmp_UR);
	}
	else
	{
		for (int i=0;i<cnt;i++)
		{
			std::list<CommandsGroup*>::iterator Iter;
			for ( Iter = undo_list.begin( ); Iter != undo_list.end( ); Iter++ )
			{
				(*Iter)->RemoveElementWithObject(GetObjectHandle(objcts[i]));
				if ((*Iter)->IsEmpty())
				{
					undo_list.erase(Iter);
					Iter--;
				}
			}
			for ( Iter = redo_list.begin( ); Iter != redo_list.end( ); Iter++ )
			{
				(*Iter)->RemoveElementWithObject(GetObjectHandle(objcts[i]));
				if ((*Iter)->IsEmpty())
				{
					redo_list.erase(Iter);
					Iter--;
				}
			}
			std::list<sgCObject*>::iterator obj_Iter;
			for ( obj_Iter = detached_objects.begin( ); 
				obj_Iter != detached_objects.end( ); obj_Iter++ )
			{
				if ((*obj_Iter)==objcts[i])
				{
					detached_objects.erase(obj_Iter);
					obj_Iter--;
				}
			}
		}
	}

	return retGr;			
}

bool   sgCGroup::BreakGroup(sgCObject** objcts)
{
	if (objcts==NULL)
	{
		assert(0);
		return false;
	}
	GEO_GROUP geo;
	lpOBJ obj = (lpOBJ)GetObjectHandle(this);
	memcpy(&geo, obj->geo_data, sizeof(GEO_GROUP));

	hOBJ hobj = geo.listh.hhead;
	int i=0;
	while(hobj) 
	{
		//   
		obj = (lpOBJ)hobj;
		obj->hhold = NULL;
		hOBJ nextObj = NULL;
		get_next_item_z(OBJ_LIST, hobj, &nextObj);
		detach_item_z(OBJ_LIST, &geo.listh, hobj);
		if (obj->extendedClass)
			objcts[i++] = obj->extendedClass;
		else
			objcts[i++] = ObjectFromHandle(hobj);
		hobj = nextObj;
	}
	int obj_cnt = i;
	obj = (lpOBJ)GetObjectHandle(this);
	memcpy(obj->geo_data, &geo, sizeof(GEO_GROUP));

	if (current_undo_redo_group)
	{
		UR_ELEMENT   tmp_UR;
		tmp_UR.obj = GetObjectHandle(this);
		tmp_UR.op_type = BREAK_GROUP;
		tmp_UR.matrix = NULL;
		tmp_UR.m_children.reserve(obj_cnt);
		for (int i=0;i<obj_cnt;i++)
			tmp_UR.m_children.push_back(GetObjectHandle(objcts[i]));
		current_undo_redo_group->AddElement(tmp_UR);
	}
	else
	{
			std::list<CommandsGroup*>::iterator Iter;
			for ( Iter = undo_list.begin( ); Iter != undo_list.end( ); Iter++ )
			{
				(*Iter)->RemoveElementWithObject(GetObjectHandle(this));
				if ((*Iter)->IsEmpty())
				{
					undo_list.erase(Iter);
					Iter--;
				}
			}
			for ( Iter = redo_list.begin( ); Iter != redo_list.end( ); Iter++ )
			{
				(*Iter)->RemoveElementWithObject(GetObjectHandle(this));
				if ((*Iter)->IsEmpty())
				{
					redo_list.erase(Iter);
					Iter--;
				}
			}
			std::list<sgCObject*>::iterator obj_Iter;
			for ( obj_Iter = detached_objects.begin( ); 
				obj_Iter != detached_objects.end( ); obj_Iter++ )
			{
				if ((*obj_Iter)==GetObjectHandle(this))
				{
					detached_objects.erase(obj_Iter);
					obj_Iter--;
				}
			}
	}

	return true;
}



sgCContour*  sgCContour::CreateContour(sgCObject** objcts, int cnt)
{
	if (objcts==NULL || cnt<2)
	{
		assert(0);
		return NULL;
	}

	bool        allAttached = false;

	if (((lpOBJ)GetObjectHandle(objcts[0]))->isAttachedToScene)
		allAttached = true;

	for (int i=0;i<cnt;i++)
	{
		if ((((lpOBJ)GetObjectHandle(objcts[i]))->isAttachedToScene)!=allAttached)
			return NULL;
	}

	if (!sgCContour::TopologySort(objcts,cnt))
		return NULL;

	GEO_PATH		geo_path;
	LISTH			listh;

	init_listh(&listh);

	for (int i=0;i<cnt;i++)
	{
		assert(objcts[i]);
		detach_item_z(SEL_LIST,&selected,GetObjectHandle(objcts[i]));
		//detach_item_z(OBJ_LIST, &objects, objcts[i]->m_object_handle);
		if (((lpOBJ)GetObjectHandle(objcts[i]))->isAttachedToScene)
		{
			sgGetScene()->DetachObject(objcts[i]);
			detached_objects.pop_back();
		}
		attach_item_tail(&listh, GetObjectHandle(objcts[i]));
	}

	memset(&geo_path, 0, sizeof(geo_path));
	geo_path.kind = PATH_COMMON;
	o_hcunit(geo_path.matr);
	geo_path.listh = listh;
	hOBJ hpath = o_alloc(OPATH);
	lpOBJ obj = (lpOBJ)hpath;
	set_obj_nongeo_par(obj);
	fc_assign_curr_ident(hpath);
	memcpy(obj->geo_data, &geo_path, geo_size[OPATH]);
	hOBJ hobj = listh.hhead;
	while (hobj) {
		obj = (lpOBJ)hobj;
		obj->hhold = hpath;
		get_next_item(hobj,&hobj);
	}

	obj = (lpOBJ)hpath;
	obj->status &= ~(ST_CLOSE | ST_FLAT | ST_SIMPLE | ST_ON_LINE);
	if (!set_contacts_on_path(hpath)) assert(0);

	D_PLANE plane;
	if (is_path_on_one_line(hpath))
	{
		obj->status |= ST_ON_LINE;
		obj->status &= ~ST_FLAT;
	}
	else
	{		
		if (set_flat_on_path(hpath, &plane))
			obj->status |= ST_FLAT;
		else      
			obj->status &= ~ST_FLAT;
	}
	//if (!set_flat_on_path(hpath, NULL)) assert(0);
	OSCAN_COD		cod;
	if ((cod = test_self_cross_path(hpath,NULL)) == OSFALSE)
	{
		/*assert(0);
		return NULL;*/
		obj->status &= ~ST_SIMPLE;
	}
	if (cod == OSTRUE) {
		obj = (lpOBJ)hpath;
		obj->status |= ST_SIMPLE;
	}
	//put_message(PRO_CREATE_PATH,text,1);

	sgCContour* retC = new sgCContour();

	obj = (lpOBJ)hpath;

	SetObjectHandle(retC, hpath);
	obj->extendedClass = retC;

	if (((lpOBJ)(hpath))->status & ST_FLAT)
	{
		memcpy(&retC->m_plane_normal,&plane.v,sizeof(D_POINT));
		retC->m_plane_D =plane.d;
	}

	char* typeID = "{0000000000000-0000-0000-000000000007}";
	set_hobj_attr_value(id_TypeID, hpath, typeID);


	retC->PostCreate();

	if (current_undo_redo_group && allAttached)
	{
		UR_ELEMENT   tmp_UR;
		tmp_UR.obj = hpath;
		tmp_UR.op_type = CREATE_GROUP;
		tmp_UR.matrix = NULL;
		tmp_UR.m_children.reserve(cnt);
		for (int i=0;i<cnt;i++)
			tmp_UR.m_children.push_back(GetObjectHandle(objcts[i]));
		current_undo_redo_group->AddElement(tmp_UR);
	}
	else
	{
		for (int i=0;i<cnt;i++)
		{
			std::list<CommandsGroup*>::iterator Iter;
			for ( Iter = undo_list.begin( ); Iter != undo_list.end( ); Iter++ )
			{
				(*Iter)->RemoveElementWithObject(GetObjectHandle(objcts[i]));
				if ((*Iter)->IsEmpty())
				{
					undo_list.erase(Iter);
					Iter--;
				}
			}
			for ( Iter = redo_list.begin( ); Iter != redo_list.end( ); Iter++ )
			{
				(*Iter)->RemoveElementWithObject(GetObjectHandle(objcts[i]));
				if ((*Iter)->IsEmpty())
				{
					redo_list.erase(Iter);
					Iter--;
				}
			}
			std::list<sgCObject*>::iterator obj_Iter;
			for ( obj_Iter = detached_objects.begin( ); 
				obj_Iter != detached_objects.end( ); obj_Iter++ )
			{
				if ((*obj_Iter)==objcts[i])
				{
					detached_objects.erase(obj_Iter);
					obj_Iter--;
				}
			}
		}
	}

	return retC;		
}


bool   sgCContour::BreakContour(sgCObject** objcts)
{
	if (objcts==NULL)
	{
		assert(0);
		global_sg_error = SG_ER_BAD_ARGUMENT_NULL_POINTER;
		return false;
	}
	GEO_PATH geo;
	lpOBJ obj = (lpOBJ)GetObjectHandle(this);
	memcpy(&geo, obj->geo_data, geo_size[OPATH]);

	hOBJ hobj = geo.listh.hhead;
	int i=0;
	while(hobj) 
	{
		//   
		obj = (lpOBJ)hobj;
		obj->hhold = NULL;
		hOBJ nextObj = NULL;
		get_next_item_z(OBJ_LIST, hobj, &nextObj);
		detach_item_z(OBJ_LIST, &geo.listh, hobj);
		if (obj->extendedClass)
			objcts[i++] = obj->extendedClass;
		else
			objcts[i++] = ObjectFromHandle(hobj);
		hobj = nextObj;
	}
	int obj_cnt = i;
	obj = (lpOBJ)GetObjectHandle(this);
	memcpy(obj->geo_data, &geo, geo_size[OPATH]);

	if (current_undo_redo_group)
	{
		UR_ELEMENT   tmp_UR;
		tmp_UR.obj = GetObjectHandle(this);
		tmp_UR.op_type = BREAK_GROUP;
		tmp_UR.matrix = NULL;
		tmp_UR.m_children.reserve(obj_cnt);
		for (int i=0;i<obj_cnt;i++)
			tmp_UR.m_children.push_back(GetObjectHandle(objcts[i]));
		current_undo_redo_group->AddElement(tmp_UR);
	}
	else
	{
		std::list<CommandsGroup*>::iterator Iter;
		for ( Iter = undo_list.begin( ); Iter != undo_list.end( ); Iter++ )
		{
			(*Iter)->RemoveElementWithObject(GetObjectHandle(this));
			if ((*Iter)->IsEmpty())
			{
				undo_list.erase(Iter);
				Iter--;
			}
		}
		for ( Iter = redo_list.begin( ); Iter != redo_list.end( ); Iter++ )
		{
			(*Iter)->RemoveElementWithObject(GetObjectHandle(this));
			if ((*Iter)->IsEmpty())
			{
				redo_list.erase(Iter);
				Iter--;
			}
		}
		std::list<sgCObject*>::iterator obj_Iter;
		for ( obj_Iter = detached_objects.begin( ); 
			obj_Iter != detached_objects.end( ); obj_Iter++ )
		{
			if ((*obj_Iter)==GetObjectHandle(this))
			{
				detached_objects.erase(obj_Iter);
				obj_Iter--;
			}
		}
	}

	global_sg_error = SG_ER_SUCCESS;
	return true;
}

