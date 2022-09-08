#include "..//CORE//sg.h"

sgCObject*  sgSurfaces::Face(const sgC2DObject&  outContour,
							 const sgC2DObject** holes,
							 int holes_count)
{
	sgC2DObject* f_o = const_cast<sgC2DObject*>(&outContour);
	
	hOBJ*  select_list_before = NULL;
	int    select_list_before_size = 0;
	if (selected.num>0)
	{
		select_list_before = (hOBJ*)SGMalloc((selected.num)*sizeof(hOBJ));
		select_list_before_size = selected.num;
		hOBJ  tmp_hobj = selected.hhead;
		hOBJ  tmp_hobj_next;
		int   tmpI = 0;
		while(tmp_hobj)
		{
			select_list_before[tmpI] = tmp_hobj;
			get_next_item_z(SEL_LIST, tmp_hobj, &tmp_hobj_next);
			((lpOBJ)tmp_hobj)->extendedClass->Select(false);
			tmpI++;
			tmp_hobj = tmp_hobj_next;
		}
	}


	short*  object_statuses = (short*)SGMalloc((holes_count+1)*sizeof(short));
	object_statuses[0] = ((lpOBJ)GetObjectHandle(f_o))->status;

	hOBJ* otvs_handls = NULL;
	if (holes_count>0)
	   otvs_handls = (hOBJ*)SGMalloc(sizeof(hOBJ)*holes_count);
	for (int i=0;i<holes_count;i++)
	{
		otvs_handls[i] = GetObjectHandle(holes[i]);
		object_statuses[i+1] = ((lpOBJ)otvs_handls[i])->status;
	}

	if (test_correct_paths_array_for_face_command(GetObjectHandle(f_o),
				otvs_handls,holes_count)!=TCPR_SUCCESS)
	{
		SGFree(otvs_handls);

		((lpOBJ)GetObjectHandle(f_o))->status = object_statuses[0];
		for (int i=0;i<holes_count;i++)
		{
			((lpOBJ)otvs_handls[i])->status = object_statuses[i+1];
		}
		SGFree(object_statuses);

		for (int i=0;i<select_list_before_size;i++)
			((lpOBJ)select_list_before[i])->extendedClass->Select(true);
		SGFree(select_list_before);

		global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
		return NULL;
	}
	if (otvs_handls)
		SGFree(otvs_handls);

	LISTH listh_otv;
	init_listh(&listh_otv);
	
	hOBJ tmpHndl=NULL;
	lpOBJ obj = NULL;

	tmpHndl = GetObjectHandle(f_o);
	
	attach_item_tail_z(SEL_LIST,&listh_otv, tmpHndl);

	for (int i=0;i<holes_count;i++)
	{
		tmpHndl = GetObjectHandle(holes[i]);
	
		attach_item_tail_z(SEL_LIST,&listh_otv, tmpHndl);
	}

	hOBJ  ob_new;
	if (!faced_mesh(&listh_otv, &ob_new))
	{
		((lpOBJ)GetObjectHandle(f_o))->status = object_statuses[0];
		for (int i=0;i<holes_count;i++)
		{
			((lpOBJ)GetObjectHandle(holes[i]))->status = object_statuses[i+1];
		}
		SGFree(object_statuses);

		for (int i=0;i<select_list_before_size;i++)
			((lpOBJ)select_list_before[i])->extendedClass->Select(true);
		SGFree(select_list_before);

		global_sg_error = SG_ER_INTERNAL;
	
		return NULL;
	}

	for (int i=0;i<holes_count;i++)
		detach_item_z(SEL_LIST,&listh_otv, GetObjectHandle(holes[i]));
	detach_item_z(SEL_LIST,&listh_otv,  GetObjectHandle(f_o));

	((lpOBJ)GetObjectHandle(f_o))->status = object_statuses[0];
	for (int i=0;i<holes_count;i++)
	{
		((lpOBJ)GetObjectHandle(holes[i]))->status = object_statuses[i+1];
	}
	SGFree(object_statuses);

	for (int i=0;i<select_list_before_size;i++)
		((lpOBJ)select_list_before[i])->extendedClass->Select(true);
	SGFree(select_list_before);

	global_sg_error = SG_ER_SUCCESS;
	
	return ObjectFromHandle(ob_new);

}







static   bool  sort_curves_for_coons_command(const sgC2DObject& firstSide,
											 const sgC2DObject& secondSide,
											 const sgC2DObject& thirdSide,
											 const sgC2DObject* fourthSide,
											 hOBJ&  fir_hObj,
											 hOBJ&  sec_hObj,
											 hOBJ&  thi_hObj,
											 hOBJ&  fou_hObj,
											 bool&  need_reverse_fir,
											 bool&  need_reverse_sec,
											 bool&  need_reverse_thi,
											 bool&  need_reverse_fou)
{
	typedef struct  
	{
		D_POINT  begin_of_curve;
		D_POINT  end_of_curve;
	} BEGIN_AND_END;

	BEGIN_AND_END  begins_and_ends[4];

	hOBJ           obj_handles[4];
	obj_handles[0]  = GetObjectHandle(&firstSide);
	obj_handles[1]  = GetObjectHandle(&secondSide);
	obj_handles[2]  = GetObjectHandle(&thirdSide);
	obj_handles[3]  = (fourthSide)?GetObjectHandle(fourthSide):NULL;

	int i;
	for (i=0;i<4;i++)
	{
		if (!obj_handles[i]) continue;
		if (!init_get_point_on_path(obj_handles[i], NULL)) return false;
		if (!get_point_on_path(0., &(begins_and_ends[i].begin_of_curve))) return false;
		if (!get_point_on_path(1., &(begins_and_ends[i].end_of_curve))) return false;
		term_get_point_on_path();
	}

	fir_hObj = obj_handles[0];
	need_reverse_fir = false;

	D_POINT  curStart,curFinish;

	curStart = begins_and_ends[0].begin_of_curve;
	curFinish = begins_and_ends[0].end_of_curve;

	int sec_curve_index=-1;
	for (i=1;i<4;i++)
	{
		if (!obj_handles[i]) continue;
		if (dpoint_eq(&curFinish, &(begins_and_ends[i].begin_of_curve), eps_d))
		{
			sec_hObj = obj_handles[i];
			need_reverse_sec = false;
			curFinish = begins_and_ends[i].end_of_curve;
			sec_curve_index = i;
			goto searchThird;
		}
		if (dpoint_eq(&curFinish, &(begins_and_ends[i].end_of_curve), eps_d))
		{
			sec_hObj = obj_handles[i];
			need_reverse_sec = true;
			curFinish = begins_and_ends[i].begin_of_curve;
			sec_curve_index = i;
			goto searchThird;
		}
	}

	return false;

searchThird:

	int third_curve_index=-1;

	for (i=1;i<4;i++)
	{
		if (!obj_handles[i] || i==sec_curve_index) continue;
		if (dpoint_eq(&curFinish, &(begins_and_ends[i].begin_of_curve), eps_d))
		{
			thi_hObj = obj_handles[i];
			need_reverse_thi = false;
			curFinish = begins_and_ends[i].end_of_curve;
			third_curve_index = i;
			goto searchFourth;
		}
		if (dpoint_eq(&curFinish, &(begins_and_ends[i].end_of_curve), eps_d))
		{
			thi_hObj = obj_handles[i];
			need_reverse_thi = true;
			curFinish = begins_and_ends[i].begin_of_curve;
			third_curve_index = i;
			goto searchFourth;
		}
	}

	return false;

searchFourth:

	if (fourthSide==NULL)
	{
		if (!dpoint_eq(&curFinish, &(begins_and_ends[0].begin_of_curve), eps_d))
		{
			assert(0);
			return false;
		}
		fou_hObj = NULL;
		need_reverse_fou = false;
		return true;
	}

	for (i=1;i<4;i++)
	{
		if (!obj_handles[i] || i==sec_curve_index || i==third_curve_index) continue;
		if (dpoint_eq(&curFinish, &(begins_and_ends[i].begin_of_curve), eps_d))
		{
			fou_hObj = obj_handles[i];
			need_reverse_fou = false;
			curFinish = begins_and_ends[i].end_of_curve;
			if (!dpoint_eq(&curFinish, &(begins_and_ends[0].begin_of_curve), eps_d))
			{
				assert(0);
				return false;
			}
			return true;
		}
		if (dpoint_eq(&curFinish, &(begins_and_ends[i].end_of_curve), eps_d))
		{
			fou_hObj = obj_handles[i];
			need_reverse_fou = true;
			curFinish = begins_and_ends[i].begin_of_curve;
			if (!dpoint_eq(&curFinish, &(begins_and_ends[0].begin_of_curve), eps_d))
			{
				assert(0);
				return false;
			}
			return true;
		}
	}
	return false;
}

sgCObject*  sgSurfaces::Coons(const sgC2DObject& firstSide,
							  const sgC2DObject& secondSide,
							  const sgC2DObject& thirdSide,
							  const sgC2DObject* fourthSide,
							  short uSegments,
							  short vSegments,
							  short uVisEdges,
							  short vVisEdges)
{

	if (firstSide.IsClosed() ||
		secondSide.IsClosed() ||
		thirdSide.IsClosed() ||
		(fourthSide && fourthSide->IsClosed()))
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_UNKNOWN;
		return NULL;
	}

	if (firstSide.IsSelfIntersecting() ||
		secondSide.IsSelfIntersecting() ||
		thirdSide.IsSelfIntersecting() ||
		(fourthSide && fourthSide->IsSelfIntersecting()))
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_UNKNOWN;
		return NULL;
	}

	hOBJ			hnew;
	hOBJ			hobj[4];
	bool			reverse[4];
	VDIM			vdim[4];

	if (!sort_curves_for_coons_command(firstSide,secondSide,thirdSide,fourthSide,
							hobj[0],hobj[1],hobj[2],hobj[3],
							reverse[0],reverse[1],reverse[2],reverse[3]))
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
		return NULL;
	}

	if (fourthSide == NULL) {	//     u2
		hobj[3] = hobj[2];
		hobj[2] = NULL;
		reverse[3] = reverse[2];
	}
	
	short n_u =uSegments;
	short n_v =vSegments;
	short cond_u = (uVisEdges>uSegments)?(uSegments-1):uVisEdges;
	short cond_v = (vVisEdges>vSegments)?(vSegments-1):vVisEdges;

	int i,j;

	reverse[2] = !reverse[2];
	reverse[3] = !reverse[3];

	for (i = 0; i < 4; i++) {
		init_vdim(&vdim[i], sizeof(D_POINT));
	}

	D_POINT p_1,p_2;
	for (i = 0; i < 4; i++) 
	{
		if (hobj[i] == NULL) continue;
		int jnum = ((i % 2) == 0) ? n_u : n_v;
		sgFloat dt = 1. / (jnum - 1);
		sgFloat t;
		if (reverse[i]) {
			t = 1.;
			dt = -dt;
		} else {
			t = 0.;
		}
		if (!init_get_point_on_path(hobj[i], NULL))
		{
			assert(0);
			global_sg_error = SG_ER_INTERNAL;
			for (i = 0; i < 4; i++) 
				free_vdim(&vdim[i]);
			return NULL;
		}
		if (fourthSide == NULL && i==1) {
			//   pstart    
			if (reverse[i]) 
				t = 0.; 
			else 
				t = 1.;
			if (!get_point_on_path(t, &p_1))
			{
				assert(0);
				global_sg_error = SG_ER_INTERNAL;
				for (i = 0; i < 4; i++) 
					free_vdim(&vdim[i]);
				return NULL;
			}
			if (reverse[i])  //RA
				t = 1.; 
			else 
				t = 0.;
		}
		for (j = 0; j < jnum; j++, t += dt) {
			/* RA ---- if (j == jnum - 1) {
				if (reverse[i]) 
					t = 0.; 
				else 
					t = 1.;
			}*/
			if (!get_point_on_path(t, &p_2))
			{
				assert(0);
				global_sg_error = SG_ER_INTERNAL;
				for (i = 0; i < 4; i++) 
					free_vdim(&vdim[i]);
				return NULL;
			}
			if (!add_elem(&vdim[i], &p_2))
			{
				assert(0);
				global_sg_error = SG_ER_INTERNAL;
				for (i = 0; i < 4; i++) 
					free_vdim(&vdim[i]);
				return NULL;
			}
		}
		term_get_point_on_path();
	}
	if (fourthSide == NULL) 
	{	//     
		for (i = 0; i < n_v; i++) {
			if (!add_elem(&vdim[2], &p_1))
			{
				assert(0);
				global_sg_error = SG_ER_INTERNAL;
				for (i = 0; i < 4; i++) 
					free_vdim(&vdim[i]);
				return NULL;
			}
		}
	}
	if (!Coons_Surface(&vdim[0], &vdim[3], &vdim[2], &vdim[1],
		n_u, n_v, cond_u,	cond_v, &hnew))
	{
		assert(0);
		global_sg_error = SG_ER_INTERNAL;
		for (i = 0; i < 4; i++) 
			free_vdim(&vdim[i]);
		return NULL;
	}
		
	for (i = 0; i < 4; i++) 
		free_vdim(&vdim[i]);

	global_sg_error = SG_ER_SUCCESS;
	return ObjectFromHandle(hnew);

}

sgCObject*  sgSurfaces::Mesh(short dimens_1, 
										short dimens_2, 
										const SG_POINT* pnts)
{
	if (dimens_1<2 || dimens_2<2)
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_UNKNOWN;
		return NULL;
	}

	if (pnts==NULL)
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_NULL_POINTER;
		return NULL;
	}

	LISTH    listh_res;
	MESHDD   mesh_data;
	MNODE    tmp_node;

	mesh_data.m = dimens_1;
	mesh_data.n = dimens_2;

	init_vdim(&mesh_data.vdim,sizeof(MNODE));

	for(int i = 0; i < mesh_data.m; i++)
	{
		for(int j = 0; j < mesh_data.n; j++)
		{
			memcpy(&tmp_node.p,&(pnts[i*mesh_data.n+j]),sizeof(D_POINT));
			tmp_node.num = COND_U | COND_V;
			if(!add_elem( &mesh_data.vdim, &tmp_node))
			{
				free_vdim(&mesh_data.vdim);
				global_sg_error = SG_ER_INTERNAL;
				return NULL;
			}
		}
	}

	init_listh(&listh_res);

	if(!np_create_brep(&mesh_data, &listh_res))
	{
		assert(0);
		global_sg_error = SG_ER_INTERNAL;
		free_vdim(&mesh_data.vdim);
		return NULL;
	}

	hOBJ hnew = listh_res.hhead;

	if(!hnew)
	{
		assert(0);
		global_sg_error = SG_ER_INTERNAL;
		free_vdim(&mesh_data.vdim);
		return NULL;
	}

	free_vdim(&mesh_data.vdim);

	global_sg_error = SG_ER_SUCCESS;
	return ObjectFromHandle(hnew);
}

sgCObject*  sgSurfaces::SewSurfaces(const sgC3DObject** surfaces, int surf_count)
{
	if (surfaces==NULL || surf_count<2)
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_NULL_POINTER;
		return NULL;
	}


	hOBJ*  select_list_before = NULL;
	int    select_list_before_size = 0;
	if (selected.num>0)
	{
		select_list_before = (hOBJ*)SGMalloc((selected.num)*sizeof(hOBJ));
		select_list_before_size = selected.num;
		hOBJ  tmp_hobj = selected.hhead;
		hOBJ  tmp_hobj_next;
		int   tmpI = 0;
		while(tmp_hobj)
		{
			select_list_before[tmpI] = tmp_hobj;
			get_next_item_z(SEL_LIST, tmp_hobj, &tmp_hobj_next);
			((lpOBJ)tmp_hobj)->extendedClass->Select(false);
			tmpI++;
			tmp_hobj = tmp_hobj_next;
		}
	}


	short*  object_statuses = (short*)SGMalloc((surf_count)*sizeof(short));
	for (int i=0;i<surf_count;i++)
	{
		if (surfaces[i]==NULL)
		{
			for (int j=0;j<i;j++)
			{
				((lpOBJ)GetObjectHandle(surfaces[j]))->status = object_statuses[j];
			}
			SGFree(object_statuses);

			for (int i=0;i<select_list_before_size;i++)
				((lpOBJ)select_list_before[i])->extendedClass->Select(true);
			SGFree(select_list_before);

			global_sg_error = SG_ER_INTERNAL;
			return NULL;
		}
		object_statuses[i] = ((lpOBJ)GetObjectHandle(surfaces[i]))->status;
	}


	LISTH listh_surf;
	init_listh(&listh_surf);

	for (int i=0;i<surf_count;i++)
		attach_item_tail_z(SEL_LIST,&listh_surf, GetObjectHandle(surfaces[i]));
	
	hOBJ hnew = NULL;

	if(!sew(&listh_surf,&hnew, eps_d))
	{
		for (int i=0;i<surf_count;i++)
		{
			((lpOBJ)GetObjectHandle(surfaces[i]))->status = object_statuses[i];
		}
		SGFree(object_statuses);

		for (int i=0;i<select_list_before_size;i++)
			((lpOBJ)select_list_before[i])->extendedClass->Select(true);
		SGFree(select_list_before);

		global_sg_error = SG_ER_INTERNAL;

		return NULL;
	}

	for (int i=0;i<surf_count;i++)
		detach_item_z(SEL_LIST,&listh_surf, GetObjectHandle(surfaces[i]));
	
	for (int i=0;i<surf_count;i++)
	{
		((lpOBJ)GetObjectHandle(surfaces[i]))->status = object_statuses[i];
	}
	SGFree(object_statuses);

	for (int i=0;i<select_list_before_size;i++)
		((lpOBJ)select_list_before[i])->extendedClass->Select(true);
	SGFree(select_list_before);

	global_sg_error = SG_ER_SUCCESS;
	return ObjectFromHandle(hnew);

}


sgCObject*  sgSurfaces::LinearSurfaceFromSections(const sgC2DObject& firstSide,
													const sgC2DObject& secondSide,
													sgFloat firstParam,
													bool   isClose)
{
	short degree=0;
	short q=0;
	
	{
		lpOBJ obj = (lpOBJ)GetObjectHandle(&firstSide);
		if( obj->type==OLINE || obj->type==OARC || obj->type==OCIRCLE || obj->type==OPATH )	
			q=2;
		else 	
			q = ((lpGEO_SPLINE)obj->geo_data)->degree; 	//case OSPLINE:
		if( q > degree ) 
			degree=q;

	    obj = (lpOBJ)GetObjectHandle(&secondSide);
		if( obj->type==OLINE || obj->type==OARC || obj->type==OCIRCLE || obj->type==OPATH )	
			q=2;
		else 	
			q = ((lpGEO_SPLINE)obj->geo_data)->degree; 	//case OSPLINE:
		if( q > degree ) 
			degree=q;
	}

	short p=degree;

	LISTH					listh_ribs;
	init_listh(&listh_ribs);

	hOBJ*  select_list_before = NULL;
	int    select_list_before_size = 0;
	if (selected.num>0)
	{
		select_list_before = (hOBJ*)SGMalloc((selected.num)*sizeof(hOBJ));
		select_list_before_size = selected.num;
		hOBJ  tmp_hobj = selected.hhead;
		hOBJ  tmp_hobj_next;
		int   tmpI = 0;
		while(tmp_hobj)
		{
			select_list_before[tmpI] = tmp_hobj;
			get_next_item_z(SEL_LIST, tmp_hobj, &tmp_hobj_next);
			((lpOBJ)tmp_hobj)->extendedClass->Select(false);
			tmpI++;
			tmp_hobj = tmp_hobj_next;
		}
	}


	short  object_statuses[2];
	object_statuses[0] = ((lpOBJ)GetObjectHandle(&firstSide))->status;
	object_statuses[1] = ((lpOBJ)GetObjectHandle(&secondSide))->status;
	

	attach_item_tail_z(SEL_LIST, &listh_ribs, GetObjectHandle(&firstSide));
	attach_item_tail_z(SEL_LIST, &listh_ribs, GetObjectHandle(&secondSide));

	D_POINT  spin_pnts[2];
    SG_POINT  tmpPnt[2];
    tmpPnt[0] = firstSide.GetPointFromCoefficient(firstParam);
    tmpPnt[1] = secondSide.GetPointFromCoefficient(0.0);
    memcpy(&spin_pnts[0],&(tmpPnt[0]),sizeof(D_POINT));
    memcpy(&spin_pnts[1],&(tmpPnt[1]),sizeof(D_POINT));
	
	hOBJ hnew;
	bool isSidesClosed = (firstSide.IsClosed() && secondSide.IsClosed());
	bool canSolid = (isClose && isSidesClosed && firstSide.IsPlane(NULL,NULL) && secondSide.IsPlane(NULL,NULL));
	//----------------Ruled_surface----------------->>>>>>>>>>>>>
	if (!Ruled_Surface(&listh_ribs, spin_pnts, degree, p,
		isSidesClosed, canSolid, &hnew)) 
	{
		((lpOBJ)GetObjectHandle(&firstSide))->status = object_statuses[0];
		((lpOBJ)GetObjectHandle(&secondSide))->status = object_statuses[1];
		
		for (int i=0;i<select_list_before_size;i++)
			((lpOBJ)select_list_before[i])->extendedClass->Select(true);
		SGFree(select_list_before);

		global_sg_error = SG_ER_INTERNAL;

		return NULL;	
	}

	detach_item_z(SEL_LIST,&listh_ribs, GetObjectHandle(&firstSide));
	detach_item_z(SEL_LIST,&listh_ribs, GetObjectHandle(&secondSide));

	((lpOBJ)GetObjectHandle(&firstSide))->status = object_statuses[0];
	((lpOBJ)GetObjectHandle(&secondSide))->status = object_statuses[1];

	for (int i=0;i<select_list_before_size;i++)
		((lpOBJ)select_list_before[i])->extendedClass->Select(true);
	SGFree(select_list_before);

	global_sg_error = SG_ER_SUCCESS;
	return ObjectFromHandle(hnew);
}



sgCObject*  sgSurfaces::SplineSurfaceFromSections(const sgC2DObject** sections,
													 const sgFloat* params,
													 int sections_count,
													 bool isClose)
{

	if (sections==NULL || sections_count<3 || params==NULL)
	{
		return NULL;
	}
	short degree=0;
	short q=0;

	for (int i=0;i<sections_count;i++)
	{
		lpOBJ obj = (lpOBJ)GetObjectHandle(sections[i]);
		if( obj->type==OLINE || obj->type==OARC || obj->type==OCIRCLE || obj->type==OPATH )	
			q=2;
		else 	
			q = ((lpGEO_SPLINE)obj->geo_data)->degree; 	//case OSPLINE:
		if( q > degree ) 
			degree=q;
	}

	short p=degree;

	LISTH	listh_ribs;
	init_listh(&listh_ribs);

	hOBJ*  select_list_before = NULL;
	int    select_list_before_size = 0;
	if (selected.num>0)
	{
		select_list_before = (hOBJ*)SGMalloc((selected.num)*sizeof(hOBJ));
		select_list_before_size = selected.num;
		hOBJ  tmp_hobj = selected.hhead;
		hOBJ  tmp_hobj_next;
		int   tmpI = 0;
		while(tmp_hobj)
		{
			select_list_before[tmpI] = tmp_hobj;
			get_next_item_z(SEL_LIST, tmp_hobj, &tmp_hobj_next);
			((lpOBJ)tmp_hobj)->extendedClass->Select(false);
			tmpI++;
			tmp_hobj = tmp_hobj_next;
		}
	}


	short*  object_statuses = (short*)SGMalloc((sections_count)*sizeof(short));
	for (int i=0;i<sections_count;i++)
	{
		if (sections[i]==NULL)    
		{
			for (int j=0;j<i;j++)
			{
				((lpOBJ)GetObjectHandle(sections[j]))->status = object_statuses[j];
			}
			SGFree(object_statuses);

			for (int i=0;i<select_list_before_size;i++)
				((lpOBJ)select_list_before[i])->extendedClass->Select(true);
			SGFree(select_list_before);

			global_sg_error = SG_ER_INTERNAL;
			return NULL;
		}
		object_statuses[i] = ((lpOBJ)GetObjectHandle(sections[i]))->status;
	}


	for (int i=0;i<sections_count;i++)
	{
		attach_item_tail_z(SEL_LIST, &listh_ribs, GetObjectHandle(sections[i]));
	}
	
	D_POINT*  spin_pnts = (D_POINT*)SGMalloc(sections_count*sizeof(D_POINT));

	for (int i=0;i<sections_count;i++)
	{
        SG_POINT  aaaPnt = sections[i]->GetPointFromCoefficient(params[i]);
        memcpy(&spin_pnts[i],&(aaaPnt),
					sizeof(D_POINT));
	}

	
	hOBJ hnew;
	//----------------Ruled_surface----------------->>>>>>>>>>>>>
	if (!Skin_Surface(&listh_ribs, spin_pnts,(short)listh_ribs.num, p, q,
				isClose,!isClose,isClose,&hnew)) 
	{
		SGFree(spin_pnts);

		for (int i=0;i<sections_count;i++)
		{
			((lpOBJ)GetObjectHandle(sections[i]))->status = object_statuses[i];
		}
		SGFree(object_statuses);

		for (int i=0;i<select_list_before_size;i++)
			((lpOBJ)select_list_before[i])->extendedClass->Select(true);
		SGFree(select_list_before);

		global_sg_error = SG_ER_INTERNAL;

		return NULL;	
	}

	SGFree(spin_pnts);

	for (int i=0;i<sections_count;i++)
		detach_item_z(SEL_LIST,&listh_ribs, GetObjectHandle(sections[i]));

	for (int i=0;i<sections_count;i++)
	{
		((lpOBJ)GetObjectHandle(sections[i]))->status = object_statuses[i];
	}
	SGFree(object_statuses);

	for (int i=0;i<select_list_before_size;i++)
		((lpOBJ)select_list_before[i])->extendedClass->Select(true);
	SGFree(select_list_before);

	global_sg_error = SG_ER_SUCCESS;
	return ObjectFromHandle(hnew);
}
