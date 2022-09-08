#include "..//CORE//sg.h"

static      bool     reorient_matr_from_path(hOBJ obj_header, MATR mtrx, bool close_obj)
{
	VDIM			vdim;
	lpMNODE		    lpnode;
	BOOL			Bool;
	short			sig, num_elem, ctype;
	D_POINT		    p;

	if (!apr_path(&vdim,obj_header))
		return false;

	sig = 1;
	ctype = 0;
	if (!begin_rw(&vdim, 0)) goto badRet;
	Bool = FALSE;	// TRUE -        
	//      sig
	for (num_elem = 0; num_elem < vdim.num_elem; num_elem++) 
	{
		if ((lpnode = (MNODE*)get_next_elem(&vdim)) == NULL) {
			end_rw(&vdim);
			goto badRet;
		}
		o_hcncrd(mtrx, &lpnode->p, &p);
		if (fabs(p.x) < eps_d) 
		{
			if (close_obj || (num_elem != 0 && num_elem < vdim.num_elem - 1)) 
			{
				end_rw(&vdim);
				goto badRet;
			}
			ctype++;
		} 
		else 
		{
			if (p.x * sig < 0.) 
			{
				if (!Bool && sig == 1) sig = -1;
				else {
					end_rw(&vdim);
					goto badRet;
				}
				Bool = TRUE;
			}
		}
	}
	end_rw(&vdim);
	if (sig == -1) o_tdrot(mtrx, -3, M_PI);

	free_vdim(&vdim);
	return true;
badRet:
	free_vdim(&vdim);
	return false;
}

sgCObject*  sgKinematic::Rotation(const sgC2DObject&  rotObj,
											const SG_POINT& axePnt1,
											const SG_POINT& axePnt2,
											sgFloat angl_degree,
											bool isClose,
											short  meridians_count/*=24*/)
{
	if (fabs(angl_degree)<0.00001)
		return NULL;

	lpOBJ lpRotObjHandle = (lpOBJ)GetObjectHandle(&rotObj);

	SG_VECTOR plN;
	sgFloat    plD;
	bool isPl = rotObj.IsPlane(&plN,&plD);
	if (rotObj.IsSelfIntersecting() ||
		(!rotObj.IsPlane(&plN,NULL) && !rotObj.IsLinear()))
			return NULL;
	
	// ,       
	if (isPl)
	{
		if (sgSpaceMath::GetPlanePointDistance(axePnt1,plN,plD)>0.00001)
		{
			global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
			return NULL;
		}
		if (sgSpaceMath::GetPlanePointDistance(axePnt2,plN,plD)>0.00001)
		{
			global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
			return NULL;
		}
	}
	else
	{
		assert(rotObj.IsLinear());
		SG_POINT begP,endP;
		begP = rotObj.GetPointFromCoefficient(0.0);
		endP = rotObj.GetPointFromCoefficient(1.0);
		if (sgSpaceMath::IsPointsOnOneLine(begP,endP,axePnt1) &&
			sgSpaceMath::IsPointsOnOneLine(begP,endP,axePnt2))
		{
			global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
			return NULL;
		}
		SG_VECTOR pllNN;
		sgFloat pllDD;
		if (!sgSpaceMath::PlaneFromPoints(axePnt1,axePnt2,begP,pllNN,pllDD))
		{
			global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
			return NULL;
		}
		if (sgSpaceMath::GetPlanePointDistance(endP,pllNN,pllDD)>0.00001)
		{
			global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
			return NULL;
		}
		
		//      
		plN = pllNN;
	}

	if (angl_degree>360.0)
		angl_degree = 360.0;
	if (angl_degree<-360.0)
		angl_degree = -360.0;

	MATR	 matr;

	o_hcunit(matr);
	D_POINT f_axis,s_axis;
	memcpy(&f_axis,&axePnt1,sizeof(D_POINT));
	memcpy(&s_axis,&axePnt2,sizeof(D_POINT));
	D_POINT plN_DP;
	memcpy(&plN_DP,&plN,sizeof(D_POINT));
	D_POINT p,p1,p2;
	o_tdtran(matr, dpoint_inv(&f_axis, &p));
	o_hcrot1(matr, dpoint_sub(&s_axis, &f_axis, &p));
	p.x = p.y = p.z = 0;
	o_hcncrd(matr, &p, &p);
	o_hcncrd(matr, &plN_DP, &p1);
	dpoint_sub(&p1, &p, &p2);
	sgFloat alpha = atan2(p2.y, p2.x);
	o_tdrot(matr, -3, M_PI / 2. - alpha);
	
	if (!reorient_matr_from_path(GetObjectHandle(&rotObj), matr, rotObj.IsClosed()))
	{
		assert(0);
		global_sg_error = SG_ER_INTERNAL;
		return NULL;
	}

	short oldMeridiansCount = sg_m_num_meridians;
	sg_m_num_meridians = meridians_count;

	if (sg_m_num_meridians<4)
		sg_m_num_meridians=4;
	if (sg_m_num_meridians>36)
		sg_m_num_meridians=36;


	hOBJ hobj=NULL;
	if (!rev_mesh(lpRotObjHandle, angl_degree/180.0*M_PI, isClose, 0, matr, FALSE, &hobj))
	{
		global_sg_error = SG_ER_INTERNAL;
		sg_m_num_meridians = oldMeridiansCount;
		return NULL;
	}

	o_minv(matr, matr);
	o_trans(hobj, matr);

	sg_m_num_meridians = oldMeridiansCount;
	
	return ObjectFromHandle(hobj);

}

sgCObject*   sgKinematic::Extrude(const sgC2DObject&  outContour,
								  const sgC2DObject** holes,
								   int holes_count,
								   const SG_VECTOR& extrDir,
								   bool isClose)
{
	SG_VECTOR aaaVec = extrDir;
	if (!sgSpaceMath::NormalVector(aaaVec))
		return NULL;

	sgC2DObject* f_o = const_cast<sgC2DObject*>(&outContour);

	if (f_o->GetType()!=SG_OT_LINE)
		if (!f_o->IsPlane(NULL,NULL) || f_o->IsSelfIntersecting())
		{
			global_sg_error = SG_ER_BAD_ARGUMENT_UNKNOWN;
			return NULL;
		}

    int real_otvs_count = (f_o->IsClosed())?holes_count:0;
	bool real_is_close = (f_o->IsClosed())?isClose:false;


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
	if (real_otvs_count>0)
		otvs_handls = (hOBJ*)SGMalloc(sizeof(hOBJ)*real_otvs_count);
	for (int i=0;i<real_otvs_count;i++)
	{
		otvs_handls[i] = GetObjectHandle(holes[i]);
		object_statuses[i+1] = ((lpOBJ)otvs_handls[i])->status;
	}
	if (f_o->IsClosed() && (test_correct_paths_array_for_face_command(GetObjectHandle(f_o),
		otvs_handls,real_otvs_count)!=TCPR_SUCCESS))
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

	for (int i=0;i<real_otvs_count;i++)
	{
		tmpHndl = GetObjectHandle(holes[i]);
		attach_item_tail_z(SEL_LIST,&listh_otv, tmpHndl);
	}

	hOBJ  ob_new;
	MATR matr;
	o_hcunit(matr);
	D_POINT tmpVec;
	memcpy(&tmpVec,&extrDir,sizeof(D_POINT));
	o_tdtran(matr, &tmpVec);
	if (!extrud_mesh(&listh_otv, 0/*1*/, matr, real_is_close, &ob_new))
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

	for (int i=0;i<real_otvs_count;i++)
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

sgCObject*   sgKinematic::Spiral(const sgC2DObject&  outContour,
								 const sgC2DObject** holes,
								 int holes_count,
								  const SG_POINT& axePnt1, 
								  const SG_POINT& axePnt2,
								  sgFloat screw_step,
								  sgFloat screw_height,
								  short  meridians_count,
								  bool isClose)
{
	sgC2DObject* f_o = const_cast<sgC2DObject*>(&outContour);

	if (fabs(screw_step)<0.0001 || fabs(screw_height)<0.0001)
		return NULL;

	if ((screw_step*screw_height)<0.0)
		screw_height = screw_height*-1.0;

	if (f_o->GetType()!=SG_OT_LINE)
		if (!f_o->IsPlane(NULL,NULL) || f_o->IsSelfIntersecting())
		{
			global_sg_error = SG_ER_BAD_ARGUMENT_UNKNOWN;
			return NULL;
		}

	int real_otvs_count = (f_o->IsClosed())?holes_count:0;
	bool real_is_close = (f_o->IsClosed())?isClose:false;

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
	if (real_otvs_count>0)
		otvs_handls = (hOBJ*)SGMalloc(sizeof(hOBJ)*real_otvs_count);
	for (int i=0;i<real_otvs_count;i++)
	{
		otvs_handls[i] = GetObjectHandle(holes[i]);
		object_statuses[i+1] = ((lpOBJ)otvs_handls[i])->status;
	}
	if (f_o->IsClosed() && (test_correct_paths_array_for_face_command(GetObjectHandle(f_o),
		otvs_handls,real_otvs_count)!=TCPR_SUCCESS))
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

	for (int i=0;i<real_otvs_count;i++)
	{
		tmpHndl = GetObjectHandle(holes[i]);
		attach_item_tail_z(SEL_LIST,&listh_otv, tmpHndl);
	}

	hOBJ  ob_new;
	
	D_POINT ppp1;
	D_POINT ppp2;
	memcpy(&ppp1, &axePnt1, sizeof(D_POINT));
	memcpy(&ppp2, &axePnt2, sizeof(D_POINT));
	
	if (!screw_mesh(&listh_otv, &ppp1, &ppp2, screw_step, screw_height,
		meridians_count, real_is_close, &ob_new))
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

	for (int i=0;i<real_otvs_count;i++)
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





static OSCAN_COD pipe0_geo_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);
static short     test_on_valid_solid(hOBJ htrace, hOBJ hcontour,
										lpD_POINT ntrace, lpD_POINT btrace,
										lpAP_TYPE bstatus, lpAP_TYPE estatus);
static void      trans_listh(MATR matr, lpLISTH listh, NUM_LIST num_list);
static BOOL      test_to_cross(lpD_POINT test, lpD_POINT vb, lpD_POINT ve);


sgCObject*   sgKinematic::Pipe(const sgC2DObject&  outContour,
									const sgC2DObject** holes,
									int holes_count,
									const sgC2DObject& guideContour,
									const SG_POINT& point_in_outContour_plane,
									sgFloat angle_around_point_in_outContour_plane,
									bool& isClose)
{
	sgC2DObject* f_o = const_cast<sgC2DObject*>(&outContour);
	sgC2DObject* napravl = const_cast<sgC2DObject*>(&guideContour);

	if (f_o->GetType()!=SG_OT_LINE)
		if (!f_o->IsPlane(NULL,NULL) || f_o->IsSelfIntersecting())
		{
			global_sg_error = SG_ER_BAD_ARGUMENT_UNKNOWN;
			return NULL;
		}

	int real_otvs_count = (f_o->IsClosed())?holes_count:0;
	bool real_is_close = (f_o->IsClosed())?isClose:false;

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
	if (real_otvs_count>0)
		otvs_handls = (hOBJ*)SGMalloc(sizeof(hOBJ)*real_otvs_count);
	for (int i=0;i<real_otvs_count;i++)
	{
		otvs_handls[i] = GetObjectHandle(holes[i]);
		object_statuses[i+1] = ((lpOBJ)otvs_handls[i])->status;
	}
	if (f_o->IsClosed() && (test_correct_paths_array_for_face_command(GetObjectHandle(f_o),
		otvs_handls,real_otvs_count)!=TCPR_SUCCESS))
	{
		if (otvs_handls)
			SGFree(otvs_handls);

		((lpOBJ)GetObjectHandle(f_o))->status = object_statuses[0];
		for (int i=0;i<holes_count;i++)
		{
			((lpOBJ)otvs_handls[i])->status = object_statuses[i+1];
		}
		if (object_statuses)
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

	for (int i=0;i<real_otvs_count;i++)
	{
		tmpHndl = GetObjectHandle(holes[i]);
		
		attach_item_tail_z(SEL_LIST,&listh_otv, tmpHndl);
	}

	VDIM 	vdim_path;
	MNODE mnode0, mnode1;
	D_POINT norm;
	OSTATUS napravl_status;
	OSTATUS first_obraz_status;

	if (!apr_path(&vdim_path, GetObjectHandle(napravl)))
	{
		assert(0);
		((lpOBJ)GetObjectHandle(f_o))->status = object_statuses[0];
		for (int i=0;i<holes_count;i++)
		{
			((lpOBJ)otvs_handls[i])->status = object_statuses[i+1];
		}
		SGFree(object_statuses);

		for (int i=0;i<select_list_before_size;i++)
			((lpOBJ)select_list_before[i])->extendedClass->Select(true);
		SGFree(select_list_before);

		global_sg_error = SG_ER_INTERNAL;
		return NULL;
	}
	if (!read_elem(&vdim_path, 0, &mnode0))
	{
		assert(0);
		free_vdim(&vdim_path);
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
	if (!read_elem(&vdim_path, 1, &mnode1))
	{
		assert(0);
		free_vdim(&vdim_path);
		((lpOBJ)GetObjectHandle(f_o))->status = object_statuses[0];
		for (int i=0;i<holes_count;i++)
		{
			((lpOBJ)otvs_handls[i])->status = object_statuses[i+1];
		}
		SGFree(object_statuses);

		for (int i=0;i<select_list_before_size;i++)
			((lpOBJ)select_list_before[i])->extendedClass->Select(true);
		SGFree(select_list_before);

		global_sg_error = SG_ER_INTERNAL;
		return NULL;
	}
	dpoint_sub(&mnode1.p, &mnode0.p, &norm);

	if (!get_status_path( GetObjectHandle(napravl), &napravl_status))   // status3
	{
		assert(0);
		free_vdim(&vdim_path);
		
		((lpOBJ)GetObjectHandle(f_o))->status = object_statuses[0];
		for (int i=0;i<holes_count;i++)
		{
			((lpOBJ)otvs_handls[i])->status = object_statuses[i+1];
		}
		SGFree(object_statuses);

		for (int i=0;i<select_list_before_size;i++)
			((lpOBJ)select_list_before[i])->extendedClass->Select(true);
		SGFree(select_list_before);

		global_sg_error = SG_ER_INTERNAL;
		return NULL;
	}

	if (!get_status_path(listh_otv.hhead, &first_obraz_status))
	{
		assert(0);
		free_vdim(&vdim_path);
		
		((lpOBJ)GetObjectHandle(f_o))->status = object_statuses[0];
		for (int i=0;i<holes_count;i++)
		{
			((lpOBJ)otvs_handls[i])->status = object_statuses[i+1];
		}
		SGFree(object_statuses);

		for (int i=0;i<select_list_before_size;i++)
			((lpOBJ)select_list_before[i])->extendedClass->Select(true);
		SGFree(select_list_before);

		global_sg_error = SG_ER_INTERNAL;
		return NULL;
	}

	MATR  return_matrix;
	o_hcunit(return_matrix);

	MATR matr,matri;
	D_POINT p2,p;
	D_POINT  p1;
	get_endpoint_on_path(GetObjectHandle(napravl), &p1, (OSTATUS)0);
	memcpy(&p2,&point_in_outContour_plane,sizeof(D_POINT));
	o_hcunit(matr);
	o_hcrot1(matr,&norm);
	o_minv(matr,matri);
	o_hcunit(matr);
	o_tdtran(matr,dpoint_inv(&p2,&p));
	SG_VECTOR plNorm;
	f_o->IsPlane(&plNorm,NULL);
	memcpy(&p,&plNorm,sizeof(D_POINT));
	o_hcrot1(matr,&p);
	o_hcmult(matr,matri);
	o_tdtran(matr,&p1);

	o_hcmult(return_matrix,matr);
	trans_listh(matr, &listh_otv, SEL_LIST); //       

	MATR  once_more_was_res_geo;

	o_hcunit(once_more_was_res_geo);
	o_hcrot1(once_more_was_res_geo, &norm);
	o_minv(once_more_was_res_geo,once_more_was_res_geo);
	o_tdtran(once_more_was_res_geo,&p1);
	o_minv(once_more_was_res_geo,matri); // matri -      -   

	o_tdrot(matri,-3,angle_around_point_in_outContour_plane/180.0*M_PI);
	o_hcmult(matri,once_more_was_res_geo);

	o_hcmult(return_matrix,matri);
	trans_listh(matri, &listh_otv, SEL_LIST);

	o_minv(return_matrix,return_matrix);

	/*MATR			mtmp, msave;
	
	//        
	place_ucs_to_lcs(GetObjectHandle(napravl), mtmp);
	o_minv(mtmp, msave);
	o_trans(GetObjectHandle(napravl), msave);
	append_no_undo = 1;
	trans_listh(msave, &listh_otv, SEL_LIST);*/

	AP_TYPE		bstat, estat;
	bstat = estat = AP_NULL;
	BOOL		solid1, closed1;

	if ((napravl_status & ST_CLOSE) && (napravl_status & ST_FLAT) &&
						!(first_obraz_status & ST_CLOSE)) 
	{
			short code = test_on_valid_solid(GetObjectHandle(napravl),
				listh_otv.hhead,&norm,&p1,&bstat,&estat);
			if (code == -1)
			{
					assert(0);
					free_vdim(&vdim_path);
					
					((lpOBJ)GetObjectHandle(f_o))->status = object_statuses[0];
					for (int i=0;i<holes_count;i++)
					{
						((lpOBJ)otvs_handls[i])->status = object_statuses[i+1];
					}
					SGFree(object_statuses);

					for (int i=0;i<select_list_before_size;i++)
						((lpOBJ)select_list_before[i])->extendedClass->Select(true);
					SGFree(select_list_before);

					global_sg_error = SG_ER_INTERNAL;
					return NULL;
			}
			if (code == 1) 
			{
				closed1 = isClose;
			}
			if (code == 0) 
			{
				isClose = false;
			}
	} 
	else 
		closed1 = FALSE;

	if ((first_obraz_status & ST_CLOSE) && !(napravl_status & ST_CLOSE)) 
		solid1 = isClose;
	else 
		solid1 = FALSE;
	

	
	hOBJ  ob_new;

	if (!pipe_mesh(&listh_otv, &vdim_path, FALSE, napravl_status, closed1,
			bstat, estat, solid1, &ob_new))
	{
		assert(0);

		trans_listh(return_matrix, &listh_otv, SEL_LIST);

		free_vdim(&vdim_path);
		
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

	trans_listh(return_matrix, &listh_otv, SEL_LIST);

	free_vdim(&vdim_path);

	for (int i=0;i<real_otvs_count;i++)
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





//---     
static void trans_listh(MATR matr, lpLISTH listh, NUM_LIST num_list)
{
	hOBJ		hobj;
//	D_POINT	mn, mx;

	sgCMatrix mmm(matr);

	hobj = listh->hhead;
	while (hobj) {
		if (((lpOBJ)hobj)->extendedClass)
		{
			((lpOBJ)hobj)->extendedClass->InitTempMatrix()->SetMatrix(&mmm);
			((lpOBJ)hobj)->extendedClass->ApplyTempMatrix();
			((lpOBJ)hobj)->extendedClass->DestroyTempMatrix();
		}
		/*o_trans(hobj, matr);
		get_object_limits(hobj, &mn, &mx);
		modify_limits(&mn, &mx);*/
		get_next_item_z(num_list, hobj, &hobj);
	}
}

typedef struct {
	lpD_POINT p;
	BOOL			first;
} DATA;
typedef DATA * lpDATA;

//     / 
static OSCAN_COD pipe0_geo_scan(hOBJ hobj,lpSCAN_CONTROL lpsc)
{
	lpDATA		data = (lpDATA)lpsc->data;
	GEO_OBJ		curr_geo;
	OBJ				curr_obj;
	D_POINT		curr_v[2], p;
	SPLY_DAT	sply_dat;
	void      *adr;

	get_simple_obj(hobj, &curr_obj, &curr_geo);
	if (lpsc->type != OSPLINE) {
		if (lpsc->status & ST_DIRECT) pereor_geo(curr_obj.type, &curr_geo);
		adr = &curr_geo;
	} else {
		if (!begin_use_sply(&curr_geo.spline, &sply_dat)) return OSFALSE;
		adr = &sply_dat;
	}
	calc_geo_direction(curr_obj.type, adr, curr_v);
	if (lpsc->type == OSPLINE) {
		end_use_sply(&curr_geo.spline, &sply_dat);
		if (lpsc->status & ST_DIRECT) {
			p = curr_v[0];
			dpoint_inv(&curr_v[1], &curr_v[0]);
			dpoint_inv(&p,         &curr_v[1]);
		}
	}
	if (data->first) {
		data->first = FALSE;
		data->p[0] = curr_v[0];
	}
	data->p[1] = curr_v[1];
	return OSTRUE;
}
/*
      
     

(    ,   
    /  
   /  )

 :   htrace - .  ( )
				hcontour - .  
				ntrace -     
							 
			btrace -   
 : 
		bstatus -     
		estatus -     
:	     1 -     
					0 -     
					-1 - 
*/
static short  test_on_valid_solid(hOBJ htrace, hOBJ hcontour,
								  lpD_POINT ntrace, lpD_POINT btrace,
								  lpAP_TYPE bstatus, lpAP_TYPE estatus)
{
	SCAN_CONTROL 	sc;
	D_PLANE 			plane;
	int						code;
	D_POINT				inside, test, px, py, p, tang[2];
	VDIM					vdim;
	MNODE					node1;
	lpMNODE				node;
	MATR					matr;
	int						i, ret;
	DATA					data;

	// inside - ,   
	if (!set_flat_on_path(htrace, &plane)) return -1;
	code = test_orient_path(htrace, &plane);
	switch (code) {
		case 1:
			dvector_product(&plane.v, ntrace, &inside);
			break;
		case -1:
			dvector_product(ntrace, &plane.v, &inside);
			break;
		default:
			assert(0);
		case -2:
			return -1;
	}

	//     /    /
	//   
	if (!apr_path(&vdim, hcontour)) return -1;
	//      
	// XOY,    inside    OX!
	lcs_oxy(btrace, dpoint_add(btrace, &inside, &px),
		dpoint_add(btrace, &plane.v, &py), matr);
	o_minv(matr, matr);
	if (!begin_rw(&vdim, 0)) goto er;
	for (i = 0; i < vdim.num_elem; i++) {
		if ((node = (lpMNODE)get_next_elem(&vdim)) == NULL) goto er1;
		o_hcncrd(matr, &node->p, &node->p);
	}
	end_rw(&vdim);
	//     
	if (!begin_rw(&vdim, 0)) goto er;
	if ((node = (lpMNODE)get_next_elem(&vdim)) == NULL) goto er1;
	test = node->p;
	for (i = 1; i < vdim.num_elem; i++) {
		if ((node = (lpMNODE)get_next_elem(&vdim)) == NULL) goto er1;
		if (i > 1) {
			if (test_to_cross(&test, &node->p, &p)) goto ex0;
		}
		p = node->p;
	}
	end_rw(&vdim);
	//     
	if (!read_elem(&vdim, vdim.num_elem - 1, &node1)) goto er;
	test = node1.p;
	if (!begin_rw(&vdim, 0)) goto er;
	for (i = 0; i < vdim.num_elem - 1; i++) {
		if ((node = (lpMNODE)get_next_elem(&vdim)) == NULL) goto er1;
		if (i) {
			if (test_to_cross(&test, &node->p, &p)) goto ex0;
		}
		p = node->p;
	}
	end_rw(&vdim);

	//      . 
	init_scan(&sc);
	data.p 		 = tang;
	data.first = TRUE;
	sc.user_geo_scan = pipe0_geo_scan;
	sc.data          = &data;	//   /  
	if (o_scan(hcontour,&sc) != OSTRUE) goto er;
	dpoint_inv(&tang[0], &tang[0]);
	int  tmpInt;
	if (coll_tst(&inside, FALSE, &tang[0], FALSE) == 1)
		tmpInt = AP_CONSTR | AP_COND;
	else 
		tmpInt = AP_SOPR;
	*bstatus = (AP_TYPE)tmpInt;

	if (coll_tst(&inside, FALSE, &tang[1], FALSE) == 1)
		tmpInt = AP_CONSTR | AP_COND;
	else 
		tmpInt = AP_SOPR;
	*estatus = (AP_TYPE)tmpInt;

	//  
	ret = 1;

ex:
	free_vdim(&vdim);
	return ret;
ex0:
	end_rw(&vdim);
	ret = 0;
	goto ex;
er1:
	end_rw(&vdim);
er:
	ret = -1;
	goto ex;
}

/*
 /   vb  ve
    test   OX
(   - XOY!)
*/
static BOOL test_to_cross(lpD_POINT test, lpD_POINT vb, lpD_POINT ve)
{
	sgFloat ymin, xmax, ymax, x;

	xmax = vb->x;
	ymin = ymax = vb->y;
	if (ve->y < vb->y) ymin = ve->y;
	if (ve->x > vb->x) xmax = ve->x;
	if (ve->y > vb->y) ymax = ve->y;
	if (xmax + eps_d < test->x ||
		ymin - eps_d > test->y ||
		ymax + eps_d < test->y) return FALSE;
	if (ymax - ymin < eps_d) return TRUE;	//    (  )
	x = vb->x + (test->y - vb->y) / (ve->y - vb->y) * (ve->x - vb->x);
	if (x + eps_d < test->x) return FALSE;
	return TRUE; //    (x,test->y)  
}

