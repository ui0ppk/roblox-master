#include "CORE//sg.h"

#include "UndoRedo.h"

sgCObject::sgCObject() 
{
	m_object_handle = NULL;
	m_temp_matrix = NULL;
	memset(&m_min,0,sizeof(SG_POINT));
	memset(&m_max,0,sizeof(SG_POINT));
	m_user_dynamic_data = NULL;
}


sgCObject::sgCObject(SG_OBJ_HANDLE objH) 
{
	m_object_handle = objH;
	((lpOBJ)(m_object_handle))->extendedClass = this;
	m_temp_matrix = NULL;
	memset(&m_min,0,sizeof(SG_POINT));
	memset(&m_max,0,sizeof(SG_POINT));
	m_user_dynamic_data = NULL;
}

void  sgCObject::PostCreate()
{
    D_POINT min, max;

	if (!get_object_limits(m_object_handle, &min, &max))
	{
		assert(0);
	}
    m_min.x = min.x; 
    m_min.y = min.y; 
    m_min.z = min.z; 

	lpOBJ pobj = (lpOBJ)m_object_handle;
    lpGEO_BREP geoBREP = (lpGEO_BREP)pobj->geo_data;
    geoBREP->min = min;
    geoBREP->max = max;

	if (GetAttribute(SG_OA_DRAW_STATE)==0)
	{
		SetAttribute(SG_OA_DRAW_STATE, SG_DS_FULL);
		SetAttribute(SG_OA_DRAW_STATE,
			GetAttribute(SG_OA_DRAW_STATE)& ~SG_DS_FRAME);
	}

	/************************************************************************/
	/* SET NAME                                                             */
	/************************************************************************/
	if(!strcmp(GetName(), ""))
	{
		switch (pobj->type)
		{
		case OPOINT:
			SetName("Point");
			return;
		case OLINE:
			SetName("Line");
			return;
		case OCIRCLE:
			SetName("Circle");
			return;
		case OARC:
			SetName("Arc");
			return;
		case OSPLINE:
			SetName("Spline");
			return;
		case OTEXT:
			SetName("Text");
			return;
		case ODIM:
			SetName("Dimension");
			return;
		case OPATH:
			SetName("Contour");
			return;
		case OBREP:
			SetName("3D Object");
			return;
		case OGROUP:
			SetName("Group");
			return;
		default:
			SetName("Unknown");
			return;
		}
	}
	
	if (application_interface && 
		application_interface->GetSceneTreeControl()&&
		pobj->handle_of_tree_node)
	{
		application_interface->GetSceneTreeControl()->UpdateNode(pobj->handle_of_tree_node);
	}
}

sgCObject::~sgCObject()
{
	if (m_user_dynamic_data)
	{
		m_user_dynamic_data->Finalize();
		m_user_dynamic_data = NULL;
	}
	if (m_temp_matrix)
		delete m_temp_matrix;
	lpOBJ lpO = (lpOBJ)GetObjectHandle(this);
	if (lpO->bit_buffer)
	{
		SGFree(lpO->bit_buffer);
	}
	lpO->bit_buffer = NULL;
	lpO->bit_buffer_size = 0;
}

sgCObject*   sgCObject::Clone()
{
	hOBJ hnewObj;
	if (!o_copy_obj(m_object_handle, &hnewObj, NULL)) 
	{
		global_sg_error = SG_ER_INTERNAL;
		return NULL;
	}
	global_sg_error = SG_ER_SUCCESS;
	return ObjectFromHandle(hnewObj);
}

SG_OBJECT_TYPE	sgCObject::GetType() const
{
	lpOBJ pobj = (lpOBJ)m_object_handle;
	switch (pobj->type)
	{
	case OPOINT:
		return SG_OT_POINT;
	case OLINE:
		return SG_OT_LINE;
	case OCIRCLE:
		return SG_OT_CIRCLE;
	case OARC:
		return SG_OT_ARC;
	case OSPLINE:
		return SG_OT_SPLINE;
	case OTEXT:
		return SG_OT_TEXT;
	case OPATH:
		return SG_OT_CONTOUR;
	case ODIM:
		return SG_OT_DIM;
	case OBREP:
		return SG_OT_3D;
	case OGROUP:
		return SG_OT_GROUP;
	default:
		return SG_OT_BAD_OBJECT;
	}
	return SG_OT_BAD_OBJECT;
}

bool   sgCObject::SetUserGeometry(const char* user_geometry_ID,
								 const unsigned short user_geometry_size,
								 const void* user_geometry_data)
{
	if (user_geometry_ID==NULL)
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_NULL_POINTER;
		return false;
	}

	if (user_geometry_size!=0 && user_geometry_data==NULL)
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
		return false;
	}

	if (user_geometry_size==0 && user_geometry_data!=NULL)
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
		return false;
	}

	if (user_geometry_size!=0 && user_geometry_data!=NULL && user_geometry_ID==NULL)
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
		return false;
	}

	strncpy(ids, user_geometry_ID, SG_OBJ_GEOMETRY_ID_MAX_LEN);
	if (!set_hobj_attr_value(id_TypeID, m_object_handle, ids))
	{
		global_sg_error = SG_ER_INTERNAL;
		return false;
	}

	if (user_geometry_size==0 || user_geometry_data==NULL)
		return true;

	lpOBJ pobj = (lpOBJ)m_object_handle;


	if (pobj->hcsg)
	{
		if ((short*)( ( (lpCSG)pobj->hcsg )->data ))
			SGFree((short*)( ( (lpCSG)pobj->hcsg )->data ));
		((lpCSG)pobj->hcsg )->size = 0;
		SGFree(pobj->hcsg);
	}

	pobj->hcsg = SGMalloc(user_geometry_size + sizeof(unsigned short));

	((lpCSG)pobj->hcsg )->size = user_geometry_size;

	memcpy((void*)(((lpCSG)pobj->hcsg )->data),user_geometry_data,user_geometry_size);

	return true;
}

const char*		sgCObject::GetUserGeometryID() const
{
	memset(ids,0,256);
	get_hobj_attr_value(id_TypeID, m_object_handle, ids);
	return ids;
}

const void*   sgCObject::GetUserGeometry(unsigned short& user_geometry_size) const
{
	lpOBJ pobj = (lpOBJ)m_object_handle;

	if (pobj->hcsg)
	{
		user_geometry_size = ((lpCSG)pobj->hcsg )->size;
		return ( ( (lpCSG)pobj->hcsg )->data );
	}
	else
	{
		user_geometry_size = 0;
		return NULL;
	}
	return NULL;
}

bool    sgCObject::SetUserDynamicData(const SG_USER_DYNAMIC_DATA* u_d_d)
{
	m_user_dynamic_data = const_cast<SG_USER_DYNAMIC_DATA*>(u_d_d);
	return true;
}

SG_USER_DYNAMIC_DATA* sgCObject::GetUserDymanicData() const
{
	return m_user_dynamic_data;
}


unsigned short  sgCObject::GetAttribute(SG_OBJECT_ATTR_ID attributeId) const
{
	lpOBJ pobj = (lpOBJ)m_object_handle;
	if (pobj==NULL)
	{
		assert(0);
		global_sg_error = SG_ER_INTERNAL;
		return 0;
	}
	global_sg_error = SG_ER_SUCCESS;
	switch(attributeId) 
	{
	case SG_OA_COLOR:
		return pobj->color;
	case SG_OA_LINE_TYPE:
		return pobj->ltype;
	case SG_OA_LINE_THICKNESS:
		return pobj->lthickness;
	case SG_OA_LAYER:
		return pobj->layer;
	case SG_OA_DRAW_STATE:
		return pobj->drawstatus;
	default:
		assert(0);
		break;
	}
	global_sg_error = SG_ER_INTERNAL;
	return 0;
}

bool   sgCObject::SetAttribute(SG_OBJECT_ATTR_ID attributeId, unsigned short attributeValue)
{
	lpOBJ pobj = (lpOBJ)m_object_handle;
	if (pobj==NULL)
	{
		assert(0);
		global_sg_error = SG_ER_INTERNAL;
		return 0;
	}
	global_sg_error = SG_ER_SUCCESS;
	switch(attributeId) 
	{
	case SG_OA_COLOR:
		pobj->color = (UCHAR)attributeValue;
		return true;
	case SG_OA_LINE_TYPE:
		pobj->ltype = (UCHAR)attributeValue;
		return true;
	case SG_OA_LINE_THICKNESS:
		pobj->lthickness = (UCHAR)attributeValue;
		return true;
	case SG_OA_LAYER:
		pobj->layer = (UCHAR)attributeValue;
		return true;
	case SG_OA_DRAW_STATE:
		pobj->drawstatus = attributeValue;
		return true;
	default:
		assert(0);
		break;
	}
	global_sg_error = SG_ER_INTERNAL;
	return 0;
}

const char*     sgCObject::GetName() const
{
	memset(ids,0,256);
	get_hobj_attr_value(id_Name, m_object_handle, ids);
	global_sg_error = SG_ER_SUCCESS;
	return ids;
}

bool  sgCObject::SetName(const char* object_name)
{
	if (object_name==NULL)
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_NULL_POINTER;
		return false;
	}
	global_sg_error = SG_ER_SUCCESS;
	strncpy(ids, object_name, SG_OBJ_NAME_MAX_LEN);
	if (!set_hobj_attr_value(id_Name, m_object_handle, ids))
	{
		global_sg_error = SG_ER_INTERNAL;
		return false;
	}
	return true;
}

const sgCObject*  sgCObject::GetParent() const
{
	lpOBJ pobj = (lpOBJ)m_object_handle;
	if (!pobj->hhold)
		return NULL;
	return ((lpOBJ)(pobj->hhold))->extendedClass;
}

void sgCObject::GetGabarits(SG_POINT& p_min, SG_POINT& p_max)
{
	memcpy(&p_min,&m_min,sizeof(SG_POINT));
	memcpy(&p_max,&m_max,sizeof(SG_POINT));
}

bool   sgCObject::IsSelect() const
{
	lpOBJ pobj = (lpOBJ)m_object_handle;
	if (pobj->status & ST_SELECT)
		return true;
	return false;
}

bool  sgCObject::IsAttachedToScene() const
{
	lpOBJ pobj = (lpOBJ)m_object_handle;
	return pobj->isAttachedToScene;
}

sgCMatrix* sgCObject::InitTempMatrix()
{
	if (m_temp_matrix)
		m_temp_matrix->Identity();
	else
		m_temp_matrix = new sgCMatrix;
	return m_temp_matrix;
}

bool       sgCObject::DestroyTempMatrix()
{
	if (m_temp_matrix)
	{
		delete m_temp_matrix;
		m_temp_matrix = NULL;
		return true;
	}
	return false;
}

sgCMatrix* sgCObject::GetTempMatrix()
{
	return m_temp_matrix;
}

void   sgCObject::DeleteObject(sgCObject* obj) 
{
	if (!obj)
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_NULL_POINTER;
		return;
	}

	lpOBJ lll = (lpOBJ)obj->m_object_handle;
	if (lll->isAttachedToScene)
		return;

	global_sg_error = SG_ER_SUCCESS;

	obj->Select(false);
	o_free(GetObjectHandle(obj), NULL);

	detached_objects.remove(obj); 
}


void sgCObject::Select(bool sel)
{
	lpOBJ pobj = (lpOBJ)m_object_handle;

	if (sel)
	{
		if(pobj->status & ST_SELECT)
			return;
		attach_item_tail_z(SEL_LIST, &selected, m_object_handle);
		pobj->status |= ST_SELECT;
	}
	else
	{
		if(!(pobj->status & ST_SELECT))
			return;
		detach_item_z(SEL_LIST, &selected, m_object_handle);
		pobj->status &= (~ST_SELECT);
	}
}
