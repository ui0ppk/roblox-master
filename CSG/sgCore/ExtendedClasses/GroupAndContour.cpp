#include "..//Core//sg.h"

#include <vector>

class  GroupChildsList : public IObjectsList
{
	sgCGroup* m_group;
public:
	GroupChildsList(sgCGroup* gr) {m_group = gr;};

	virtual  int         GetCount()	const
	{
		lpOBJ pobj = (lpOBJ)GetObjectHandle(m_group);

		if (pobj->type==OGROUP)
			return (((lpGEO_GROUP)(pobj->geo_data))->listh.num);
		else
			return 0;
	};
	virtual  sgCObject*  GetHead()	const
	{
		lpOBJ pobj = (lpOBJ)GetObjectHandle(m_group);

		if (pobj->type==OGROUP)
		{
			VADR   objH = (((lpGEO_GROUP)(pobj->geo_data))->listh.hhead);
			return ((lpOBJ)objH)->extendedClass;
		}
		else
			return NULL;
	};
	virtual  sgCObject*  GetNext(sgCObject* c_obj) const
	{
		hOBJ nextElem;
		get_next_item(GetObjectHandle(c_obj),&nextElem);
		if (nextElem==NULL)
			return NULL;
		return ((lpOBJ)nextElem)->extendedClass;
	};
	virtual  sgCObject*  GetTail()	const
	{
		lpOBJ pobj = (lpOBJ)GetObjectHandle(m_group);

		if (pobj->type==OGROUP)
		{
			VADR   objH = (((lpGEO_GROUP)(pobj->geo_data))->listh.htail);
			return ((lpOBJ)objH)->extendedClass;
		}
		else
			return NULL;
	};
	virtual  sgCObject*  GetPrev(sgCObject* c_obj) const
	{
		hOBJ prElem;
		get_prev_item(GetObjectHandle(c_obj),&prElem);
		if (prElem==NULL)
			return NULL;
		return ((lpOBJ)prElem)->extendedClass;
	}
};


sgCGroup::sgCGroup():sgCObject()
{
	m_children = new GroupChildsList(this);
}

sgCGroup::sgCGroup(SG_OBJ_HANDLE objH):sgCObject(objH)
{
	lpOBJ pobj = (lpOBJ)objH;
	if (pobj->type!=OGROUP) 
		return;
	
	lpGEO_GROUP geo;
	hOBJ  list_elem;

	geo = (lpGEO_GROUP)pobj->geo_data;

	list_elem = geo->listh.hhead;
	while (list_elem) 
	{
		ObjectFromHandle(list_elem);
		get_next_item(list_elem,&list_elem);
	}

	m_children = new GroupChildsList(this);

	PostCreate();
}

sgCGroup::~sgCGroup()
{
	delete m_children;
}

IObjectsList*   sgCGroup::GetChildrenList() const
{
	return m_children;
}

bool   sgCGroup::SetAttribute(SG_OBJECT_ATTR_ID prId, unsigned short prop)
{
	sgCObject*  curObj = GetChildrenList()->GetHead();
	while (curObj) 
	{
		curObj->SetAttribute(prId, prop);
		curObj = GetChildrenList()->GetNext(curObj);
	}
	return sgCObject::SetAttribute(prId, prop);
}

bool	sgCGroup::ApplyTempMatrix()
{
	//if (!sgCObject::ApplyTempMatrix())
	//	return false;

	sgCObject*  curObj = GetChildrenList()->GetHead();
	while (curObj) 
	{
		curObj->InitTempMatrix()->SetMatrix(GetTempMatrix());
		if (!curObj->ApplyTempMatrix())
		{
			curObj->DestroyTempMatrix();
			return false;
		}
		curObj->DestroyTempMatrix();
		curObj = GetChildrenList()->GetNext(curObj);
	}
	
	if (!get_object_limits(GetObjectHandle(this),reinterpret_cast<D_POINT*>(&m_min),
		reinterpret_cast<D_POINT*>(&m_max)))
	{
		assert(0);
	}
	if (fabs(m_min.x-scene_min.x)<0.0000001 ||
		fabs(m_min.y-scene_min.y)<0.0000001 ||
		fabs(m_min.z-scene_min.z)<0.0000001 ||
		fabs(m_max.x-scene_max.x)<0.0000001 ||
		fabs(m_max.y-scene_max.y)<0.0000001 ||
		fabs(m_max.z-scene_max.z)<0.0000001)
		get_scene_limits();
	return true;
}






class  ContChildsList : public IObjectsList
{
	sgCContour* m_cont;
public:
	ContChildsList(sgCContour* cn) {m_cont = cn;};

	virtual  int         GetCount()	const
	{
		lpOBJ pobj = (lpOBJ)GetObjectHandle(m_cont);

		if (pobj->type==OPATH)
			return (((lpGEO_PATH)(pobj->geo_data))->listh.num);
		else
			return 0;
	};
	virtual  sgCObject*  GetHead()	const
	{
		lpOBJ pobj = (lpOBJ)GetObjectHandle(m_cont);

		if (pobj->type==OPATH)
		{
			VADR   objH = (((lpGEO_PATH)(pobj->geo_data))->listh.hhead);
			return ((lpOBJ)objH)->extendedClass;
		}
		else
			return NULL;
	};
	virtual  sgCObject*  GetNext(sgCObject* c_obj) const
	{
		hOBJ nextElem;
		get_next_item(GetObjectHandle(c_obj),&nextElem);
		if (nextElem==NULL)
			return NULL;
		return ((lpOBJ)nextElem)->extendedClass;
	};
	virtual  sgCObject*  GetTail()	const
	{
		lpOBJ pobj = (lpOBJ)GetObjectHandle(m_cont);

		if (pobj->type==OPATH)
		{
			VADR   objH = (((lpGEO_PATH)(pobj->geo_data))->listh.htail);
			return ((lpOBJ)objH)->extendedClass;
		}
		else
			return NULL;
	};
	virtual  sgCObject*  GetPrev(sgCObject* c_obj) const
	{
		hOBJ prElem;
		get_prev_item(GetObjectHandle(c_obj),&prElem);
		if (prElem==NULL)
			return NULL;
		return ((lpOBJ)prElem)->extendedClass;
	}
};


sgCContour::sgCContour():sgC2DObject()
{
	m_children = new ContChildsList(this);
}

sgCContour::sgCContour(SG_OBJ_HANDLE objH):sgC2DObject(objH)
{
	lpOBJ pobj = (lpOBJ)objH;
	if (pobj->type!=OPATH) 
		return;

	lpGEO_PATH geo = (lpGEO_PATH)pobj->geo_data;
	hOBJ  list_elem = geo->listh.hhead;;
	
	while (list_elem) 
	{
		ObjectFromHandle(list_elem);
		get_next_item(list_elem,&list_elem);
	}

	
	if (is_path_on_one_line(objH))
	{
		pobj->status |= ST_ON_LINE;
		pobj->status &= ~ST_FLAT;
	}
	else
	{
		pobj->status &= ~ST_ON_LINE;
		D_PLANE plane;
		if (set_flat_on_path(objH, &plane))
		{
			pobj->status |= ST_FLAT;
			memcpy(&m_plane_normal,&plane.v,sizeof(D_POINT));
			m_plane_D =plane.d;
		}
		else  
		{
			pobj->status &= ~ST_FLAT;
		}
	}

	m_children = new ContChildsList(this);

	PostCreate();
}

sgCContour::~sgCContour()
{
	delete m_children;
}


IObjectsList*   sgCContour::GetChildrenList() const
{
	return m_children;
}



bool   sgCContour::SetAttribute(SG_OBJECT_ATTR_ID prId, unsigned short prop)
{
	sgCObject*  curObj = GetChildrenList()->GetHead();
	while (curObj) 
	{
		curObj->SetAttribute(prId, prop);
		curObj = GetChildrenList()->GetNext(curObj);
	}
	return sgCObject::SetAttribute(prId, prop);
}



bool	sgCContour::ApplyTempMatrix()
{
	//if (!sgCObject::ApplyTempMatrix())
	//	return false;

	sgCObject*  curObj = GetChildrenList()->GetHead();
	while (curObj) 
	{
		curObj->InitTempMatrix()->SetMatrix(GetTempMatrix());
		if (!curObj->ApplyTempMatrix())
		{
			curObj->DestroyTempMatrix();
			return false;
		}
		curObj->DestroyTempMatrix();
		curObj = GetChildrenList()->GetNext(curObj);
	}

	lpOBJ obj = (lpOBJ)GetObjectHandle(this);
	if (obj->status & ST_FLAT)
	{
		D_PLANE plane;

		// ROBLOX change
		//#if (SG_CURRENT_PLATFORM==SG_PLATFORM_IOS)
            sgFloat savingEpsD = eps_d;
            eps_d *=100.0;    //  fix for sgFloat = float  (not double )
        //#endif
		if (set_flat_on_path(GetObjectHandle(this), &plane))
		{
			obj->status |= ST_FLAT;
			memcpy(&m_plane_normal,&plane.v,sizeof(D_POINT));
			m_plane_D =plane.d;
		}
		else  
		{
			//assert(0);
			obj->status &= ~ST_FLAT;
		}
		// ROBLOX change
		//#if (SG_CURRENT_PLATFORM==SG_PLATFORM_IOS)
             eps_d = savingEpsD;
	    //#endif
	}

	if (!get_object_limits(GetObjectHandle(this),reinterpret_cast<D_POINT*>(&m_min),
		reinterpret_cast<D_POINT*>(&m_max)))
	{
		assert(0);
	}
	if (fabs(m_min.x-scene_min.x)<0.0000001 ||
		fabs(m_min.y-scene_min.y)<0.0000001 ||
		fabs(m_min.z-scene_min.z)<0.0000001 ||
		fabs(m_max.x-scene_max.x)<0.0000001 ||
		fabs(m_max.y-scene_max.y)<0.0000001 ||
		fabs(m_max.z-scene_max.z)<0.0000001)
		get_scene_limits();
	return true;
}

bool   sgCContour::IsClosed() const
{
	if (((lpOBJ)GetObjectHandle(this))->status & ST_CLOSE)
		return true;
	else
		return false;
}

bool   sgCContour::IsPlane(SG_VECTOR* plN,sgFloat* plD) const
{
	if (((lpOBJ)GetObjectHandle(this))->status & ST_FLAT)
	{
		if (plN)
			*plN = m_plane_normal;
		if (plD)
			*plD = m_plane_D;
		return true;
	}
	else
		return false;
}


bool  sgCContour::IsLinear() const
{
	if (((lpOBJ)GetObjectHandle(this))->status & ST_ON_LINE)
		return true;
	else
		return false;
}


bool   sgCContour::IsSelfIntersecting()  const
{
	if (((lpOBJ)GetObjectHandle(this))->status & ST_SIMPLE)
		return false;
	else
		return true;
}

typedef struct  
{
	SG_POINT   beginP;
	SG_POINT   endP;
}POINT_PAIR;

typedef struct  
{
	SG_POINT   Pnt;
	int        indexOfPointObject;
	bool       isExistPair;
	int        indexOfPairObject;
}GRAPH_VERTEX;

/*
static void switch_st_direct(hOBJ hobj)
{
	lpOBJ obj;

	obj = (lpOBJ)hobj;
	obj->status ^= ST_DIRECT;
}
*/

static bool isExistObjectInArray(sgCObject** obArr, int cnt, sgCObject* ob)
{
	for (int i=0;i<cnt;i++)
		if (obArr[i]==ob)
			return true;
	return false;
}

bool   sgCContour::TopologySort(sgCObject** objcts, int cnt)
{
	POINT_PAIR*  b_e_P = (POINT_PAIR*)malloc(sizeof(POINT_PAIR)*cnt);

	int i,j;

	//   
	for (i=0;i<cnt;i++)
	{
		if (objcts[i]->GetType()==SG_OT_LINE)
		{
			sgCLine* ln = reinterpret_cast<sgCLine*>(objcts[i]);
			b_e_P[i].beginP = ln->GetGeometry()->p1;
			b_e_P[i].endP = ln->GetGeometry()->p2;
		}
		else
			if (objcts[i]->GetType()==SG_OT_ARC)
			{
				sgCArc* ar = reinterpret_cast<sgCArc*>(objcts[i]);
				b_e_P[i].beginP = ar->GetGeometry()->begin;
				b_e_P[i].endP = ar->GetGeometry()->end;
			}
			else
				if (objcts[i]->GetType()==SG_OT_CONTOUR)
				{
					sgCContour* cntr = reinterpret_cast<sgCContour*>(objcts[i]);
					if (cntr->IsClosed())
					{
						free(b_e_P);
						return false;
					}
					b_e_P[i].beginP = cntr->GetPointFromCoefficient(0.0);
					b_e_P[i].endP = cntr->GetPointFromCoefficient(1.0);
				}
				else
				{
					free(b_e_P);
					return false;
				}
	}

	GRAPH_VERTEX*  gr_Pnts = (GRAPH_VERTEX*)malloc(sizeof(GRAPH_VERTEX)*2*cnt);
	memset(gr_Pnts,0,sizeof(GRAPH_VERTEX)*2*cnt);

	for (i=0;i<cnt;i++)
	{
		gr_Pnts[2*i].Pnt = b_e_P[i].beginP;
		gr_Pnts[2*i].indexOfPointObject = i;
		gr_Pnts[2*i].isExistPair = false;
		gr_Pnts[2*i].indexOfPairObject = -1;
		for (j=0;j<cnt;j++)
		{
			if (i!=j)
			{
				if (dpoint_eq((lpD_POINT)&(b_e_P[i].beginP),(lpD_POINT)&(b_e_P[j].beginP),eps_d))
				{
					gr_Pnts[2*i].isExistPair = true;
					gr_Pnts[2*i].indexOfPairObject = j;
					break;
				}
				if (dpoint_eq((lpD_POINT)&(b_e_P[i].beginP),(lpD_POINT)&(b_e_P[j].endP),eps_d))
				{
					gr_Pnts[2*i].isExistPair = true;
					gr_Pnts[2*i].indexOfPairObject = j;
					break;
				}
			}
		}

		gr_Pnts[2*i+1].Pnt = b_e_P[i].endP;
		gr_Pnts[2*i+1].indexOfPointObject = i;
		gr_Pnts[2*i+1].isExistPair = false;
		gr_Pnts[2*i+1].indexOfPairObject = -1;
		for (j=0;j<cnt;j++)
		{
			if (i!=j)
			{
				if (dpoint_eq((lpD_POINT)&(b_e_P[i].endP),(lpD_POINT)&(b_e_P[j].beginP),eps_d))
				{
					gr_Pnts[2*i+1].isExistPair = true;
					gr_Pnts[2*i+1].indexOfPairObject = j;
					break;
				}
				if (dpoint_eq((lpD_POINT)&(b_e_P[i].endP),(lpD_POINT)&(b_e_P[j].endP),eps_d))
				{
					gr_Pnts[2*i+1].isExistPair = true;
					gr_Pnts[2*i+1].indexOfPairObject = j;
					break;
				}
			}
		}
	}

	free(b_e_P);

	int  begInd= -1;
	int  endInd = -1;

	for (i=0;i<2*cnt;i++)
	{
		if(!gr_Pnts[i].isExistPair && begInd==-1 && endInd==-1)
			begInd = i;
		else
			if(!gr_Pnts[i].isExistPair && begInd!=-1 && endInd==-1)
				endInd = i;
			else
				if(!gr_Pnts[i].isExistPair && begInd!=-1 && endInd!=-1)
					{
						assert(0);
						free(gr_Pnts);
						return false;

					}
	}

	/*sgCObject** tmpArray = (sgCObject**)malloc(sizeof(sgCObject*)*cnt);
	memset(tmpArray,0,sizeof(sgCObject*)*cnt);


	if (begInd==-1 && endInd==-1) // 
	{
		tmpArray[0] = objcts[gr_Pnts[0].indexOfPointObject];
		assert(gr_Pnts[0].indexOfPairObject!=-1);
		tmpArray[1] = objcts[gr_Pnts[0].indexOfPairObject];
		int lastInd = gr_Pnts[0].indexOfPairObject;
		gr_Pnts[0].indexOfPointObject = gr_Pnts[0].indexOfPairObject = -1;
		for (i=2;i<cnt;i++)
		{
			for (j=0;j<2*cnt;j++)
			{
				if (gr_Pnts[j].indexOfPointObject!=lastInd &&
					gr_Pnts[j].indexOfPairObject==lastInd)
				{
					   tmpArray[i] = objcts[gr_Pnts[j].indexOfPointObject];
					   for (int k=0;k<2*cnt;k++)
					   		if(gr_Pnts[k].indexOfPointObject==lastInd)
								gr_Pnts[k].indexOfPointObject=
									gr_Pnts[k].indexOfPairObject = -1;
					   lastInd = gr_Pnts[j].indexOfPointObject;
					   gr_Pnts[j].indexOfPointObject = gr_Pnts[j].indexOfPairObject = -1;
					   break;
				}
			}
			if (tmpArray[i]==NULL)
			{
				assert(0);
				free(gr_Pnts);
				free(tmpArray);
				return false;
			}
		}
	}
	else  //  
	{
		tmpArray[0] = objcts[gr_Pnts[begInd].indexOfPointObject];
		int lastInd = gr_Pnts[begInd].indexOfPointObject;
		gr_Pnts[begInd].indexOfPointObject = -1;
		for (i=1;i<cnt-1;i++)
		{
			for (j=0;j<2*cnt;j++)
			{
				if (gr_Pnts[j].indexOfPointObject!=lastInd &&
					gr_Pnts[j].indexOfPairObject==lastInd)
				{
					tmpArray[i] = objcts[gr_Pnts[j].indexOfPointObject];
					for (int k=0;k<2*cnt;k++)
						if(gr_Pnts[k].indexOfPointObject==lastInd)
							gr_Pnts[k].indexOfPointObject=
							gr_Pnts[k].indexOfPairObject = -1;
					lastInd = gr_Pnts[j].indexOfPointObject;
					gr_Pnts[j].indexOfPointObject = gr_Pnts[j].indexOfPairObject = -1;
					break;
				}
			}
			if (tmpArray[i]==NULL)
			{
				assert(0);
				free(gr_Pnts);
				free(tmpArray);
				return false;
			}
		}
		tmpArray[cnt-1] = objcts[gr_Pnts[endInd].indexOfPointObject];
	}



	memcpy(objcts,tmpArray,sizeof(sgCObject*)*cnt);

	free(gr_Pnts);
	free(tmpArray);
	return true;*/


	sgCObject** tmpArray = (sgCObject**)malloc(sizeof(sgCObject*)*cnt);
	memset(tmpArray,0,sizeof(sgCObject*)*cnt);

	if (begInd==-1 && endInd==-1) // 
		tmpArray[0] = objcts[0];
	else
	{
		D_POINT tmpP;
		get_endpoint_on_path(GetObjectHandle(objcts[gr_Pnts[begInd].indexOfPointObject]),
						&tmpP,(OSTATUS)0);
		bool thisOrNot = (dpoint_eq((lpD_POINT)&(gr_Pnts[begInd].Pnt),(lpD_POINT)&(tmpP),eps_d)!=FALSE);
		if (!thisOrNot)
		{
			lpOBJ obj = (lpOBJ)(GetObjectHandle(objcts[gr_Pnts[begInd].indexOfPointObject]));
			obj->status ^= ST_DIRECT;
		}
		tmpArray[0] = objcts[gr_Pnts[begInd].indexOfPointObject];
	}
	free(gr_Pnts);

	D_POINT begLast,endLast;
	get_endpoint_on_path(GetObjectHandle(tmpArray[0]),&begLast,(OSTATUS)0);
	D_POINT begFirst = begLast;
	get_endpoint_on_path(GetObjectHandle(tmpArray[0]),&endLast,(OSTATUS)ST_DIRECT);
	for (int i=1;i<cnt;i++)
	{
		for (int j=0;j<cnt;j++)
		{
			if (!isExistObjectInArray(tmpArray,i,objcts[j]))
			{
				D_POINT begCur,endCur;
				get_endpoint_on_path(GetObjectHandle(objcts[j]),&begCur,(OSTATUS)0);
				get_endpoint_on_path(GetObjectHandle(objcts[j]),&endCur,(OSTATUS)ST_DIRECT);
				if (dpoint_eq((lpD_POINT)&(endLast),(lpD_POINT)&(begCur),eps_d))
				{
					tmpArray[i] = objcts[j];
					begLast = begCur;
					endLast = endCur;
					break;
				}
				if (dpoint_eq((lpD_POINT)&(endLast),(lpD_POINT)&(endCur),eps_d))
				{
					lpOBJ obj = (lpOBJ)GetObjectHandle(objcts[j]);
					obj->status ^= ST_DIRECT;
					tmpArray[i] = objcts[j];
					begLast = endCur;
					endLast = begCur;
					break;
				}
				/*if (dpoint_eq((lpD_POINT)&(begFirst),(lpD_POINT)&(endCur),eps_d))
				{
					//  
					tmpArray[cnt-1] = objcts[j];
					assert(i==cnt-1);
					goto endLabel;
				}*/
				/*if (dpoint_eq((lpD_POINT)&(begFirst),(lpD_POINT)&(begCur),eps_d))
				{
					//    
					assert(0);
					free(tmpArray);
					return false;
				}*/
			}
		}
	}

	memcpy(objcts,tmpArray,sizeof(sgCObject*)*cnt);

	free(tmpArray);
	return true;
}
