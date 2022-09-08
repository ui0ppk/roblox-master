#include "../sg.h"




SPL_WORK spl_work;

static hOBJ get_begin_simple_obj_from_path(hOBJ hobj, OSTATUS *direct)
{
	lpOBJ 			obj;
	lpGEO_PATH 	path;
	hOBJ				hobj1;

	if (*direct) *direct = ST_DIRECT;
	obj = (lpOBJ)hobj;
	int tmp_st = *direct;
	tmp_st ^= (obj->status & ST_DIRECT);
	(*direct) = (OSTATUS)tmp_st;//^= (obj->status & ST_DIRECT);
	while (obj->type == OPATH) {
		path = (lpGEO_PATH)obj->geo_data;
		if (*direct) hobj1 = path->listh.htail;
		else         hobj1 = path->listh.hhead;
		hobj = hobj1;
		obj = (lpOBJ)hobj;
		tmp_st = *direct;
		tmp_st ^= (obj->status & ST_DIRECT);
		(*direct) = (OSTATUS)tmp_st;//^= (obj->status & ST_DIRECT);
	}
	return hobj;
}


void get_endpoint_on_path(hOBJ hobj, lpD_POINT p, OSTATUS direct)
{
	lpOBJ					obj;
	lpGEO_LINE 		line;
	lpGEO_ARC			arc;
	lpD_POINT			from;
	lpGEO_SPLINE	spline;
	int						i;

	hobj = get_begin_simple_obj_from_path(hobj, &direct);
	obj = (lpOBJ)hobj;
	switch (obj->type) {
		case OLINE:
			line = (lpGEO_LINE)obj->geo_data;
			if (direct) from = &line->v2;
			else        from = &line->v1;
			*p = *from;
			break;
		case OCIRCLE:
			arc = (lpGEO_ARC)obj->geo_data;
			get_c_matr((lpGEO_CIRCLE)arc, el_g_e, el_e_g);
			p->x = arc->r;
			p->y = p->z = 0;
			o_hcncrd(el_e_g, p, p);
			break;
		case OARC:
			arc = (lpGEO_ARC)obj->geo_data;
			if (direct) from = &arc->ve;
			else        from = &arc->vb;
			*p = *from;
			break;
		case OSPLINE:
			spline = (lpGEO_SPLINE)obj->geo_data;
			from = (lpD_POINT)spline->hpoint;
			if (direct) i = spline->nump - 1;
			else        i = 0;
			*p = from[i];
			break;
	}
}

BOOL get_status_path(hOBJ hpath, OSTATUS *status)
{
	lpOBJ 				obj;
	lpGEO_ARC			arc;
//	lpGEO_SPLINE	spline;

	*status = (OSTATUS)0;
	obj = (lpOBJ)hpath;
	switch (obj->type) {
		case OLINE:
			*status = (OSTATUS)(ST_FLAT | ST_SIMPLE);
			break;
		case OARC:
			*status = (OSTATUS)(ST_FLAT | ST_SIMPLE);
			arc = (lpGEO_ARC)obj->geo_data;
			if (dpoint_eq(&arc->vb, &arc->ve, eps_d)) 
			{
				int tmp_st = *status;
				tmp_st |= ST_CLOSE;
				(*status) = (OSTATUS)tmp_st;
				//*status |= ST_CLOSE;
			}
			break;
		case OCIRCLE:
			*status = (OSTATUS)(ST_FLAT | ST_SIMPLE | ST_CLOSE);
			break;
		case OSPLINE:
/*
			spline = (lpGEO_SPLINE)obj->geo_data;
			*status = ST_SIMPLE;
			if (spline->type & SPLY_CLOSE) *status |= ST_CLOSE;
			if (!set_flat_on_path(hpath, NULL)) return FALSE;
			obj = (lpOBJ)hpath;
			*status |= (obj->status & ST_FLAT);
			break;
*/
		case OPATH:
			*status = (OSTATUS)(obj->status & (ST_FLAT | ST_SIMPLE | ST_CLOSE));
			break;
		default:
			put_message(MSG_BAD_OBJ_FOR_PATH,NULL, 0);
			return FALSE;
	}
	return TRUE;
}

BOOL get_flat_path(hOBJ hpath)
{
	lpOBJ 	obj;
	OBJTYPE	type;
	OSTATUS	status;

	obj = (lpOBJ)hpath;
	type = obj->type;
	status = (OSTATUS)obj->status;
	switch (type) {
		case OLINE:
		case OARC:
		case OCIRCLE:
			return TRUE;
		case OPATH:
			return (status & ST_FLAT) ? TRUE : FALSE;
		case OSPLINE:
			return (status & ST_FLAT) ? TRUE : FALSE;//set_flat_on_path(hpath, NULL);
	}
	return FALSE;
}

void reset_direct_on_path_1(hOBJ hpath)
{
	lpOBJ 			obj, obj1;
	hOBJ				hobj1, hobj2;
	lpGEO_PATH	path;
	D_POINT			end, beg2, end2;

	obj = (lpOBJ)hpath;
	path = (lpGEO_PATH)obj->geo_data;
	if (path->listh.num == 1) return;
	hobj1 = path->listh.hhead;
	get_next_item(hobj1,&hobj2);
	get_endpoint_on_path(hobj1, &end, ST_DIRECT);
	get_endpoint_on_path(hobj2, &beg2, (OSTATUS)0);
	if (dpoint_eq(&end, &beg2, eps_d)) {	
		get_endpoint_on_path(hobj2, &end, ST_DIRECT);
		goto step2;
	}
	get_endpoint_on_path(hobj2, &end2, ST_DIRECT);
	if (dpoint_eq(&end, &end2, eps_d)) {	
		obj1 = (lpOBJ)hobj2;
		obj1->status ^= ST_DIRECT;
		end = beg2;
		goto step2;
	}
	obj1 = (lpOBJ)hobj1;							
	obj1->status ^= ST_DIRECT;
	get_endpoint_on_path(hobj1, &end, ST_DIRECT);
	if (dpoint_eq(&end, &beg2, eps_d)) {	
		end = end2;
		goto step2;
	}
	if (dpoint_eq(&end, &end2, eps_d)) {	
		obj1 = (lpOBJ)hobj2;
		obj1->status ^= ST_DIRECT;
		end = beg2;
		goto step2;
	}
step2:
	while (TRUE) {
		get_next_item(hobj2, &hobj2);
		if (!hobj2) break;
		get_endpoint_on_path(hobj2, &beg2, (OSTATUS)0);
		if (dpoint_eq(&end, &beg2, eps_d)) {	
			get_endpoint_on_path(hobj2, &end, ST_DIRECT);
		} else {
			obj1 = (lpOBJ)hobj2;					
			obj1->status ^= ST_DIRECT;
			end = beg2;
		}
	}
}

hOBJ create_path_by_point(lpD_POINT p, short num)
{
	short				i;
	lpOBJ    		obj_path, obj_line;
	lpGEO_PATH	geo_path;
	lpGEO_LINE	geo_line;
	OSCAN_COD		cod;

	if ( (obj_path = (OBJ*)o_alloc(OPATH)) == NULL)	return FALSE;
	geo_path = (lpGEO_PATH)(obj_path->geo_data);
	o_hcunit(geo_path->matr);
	init_listh(&(geo_path->listh));
	for (i=0; i < num-1; i++) {					
	 if (!dpoint_eq(&p[i], &p[i+1], eps_d)) {
		 if((obj_line = (OBJ*)o_alloc(OLINE)) == NULL) goto err;
		 obj_line->hhold = obj_path;
		 geo_line = (lpGEO_LINE)(obj_line->geo_data);
		 geo_line->v1 = p[i];
		 geo_line->v2 = p[i+1];
		 attach_item_tail(&geo_path->listh, obj_line);
	 }
	}

	dpoint_minmax(p, num, &geo_path->min, &geo_path->max);
	if (dpoint_eq(&p[0], &p[num-1], eps_d)) obj_path->status |= ST_CLOSE;
	if (!set_contacts_on_path(obj_path)) goto err;
	if (is_path_on_one_line(obj_path))
	{
		obj_path->status |= ST_ON_LINE;
		obj_path->status &= ~ST_FLAT;
	}
	else
	{
		if (set_flat_on_path(obj_path, NULL))
			obj_path->status |= ST_FLAT;
		else      
			obj_path->status &= ~ST_FLAT;
	}
	//if (!set_flat_on_path(obj_path, NULL)) goto err;
	if ((cod = test_self_cross_path(obj_path,NULL)) == OSFALSE) goto err;
	if (cod == OSTRUE) {
		obj_path->status |= ST_SIMPLE;
	}

	return obj_path;
err:
	o_free(obj_path, NULL);
	return NULL;
}
