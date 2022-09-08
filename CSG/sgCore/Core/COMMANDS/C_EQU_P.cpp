#include "../sg.h"

static	sgFloat	height = 0., height2 = 0.;

BOOL create_equ_path2(hOBJ hobj, lpD_PLANE lpPlane, sgFloat height1, sgFloat height2,
										 BOOL arc_equd, hOBJ * hobj_equ)
{
	VDIM				vdim;
	MATR				matr, matr2;
	D_POINT			p, vn, vk, min, max;
	D_PLANE			plane;
	MNODE				node;
	lpMNODE			lpNode;
	ZCUT 				cut1, cut2;
	OSTATUS			status;
	short				i, max_i, count;
	UCHAR				stat;
	IDENT_V			idf, idp, idv, idvb, Bool;
	hOBJ				hobj_line;
	lpOBJ				obj_path, obj_line;
	lpGEO_PATH	geo_path;
	lpGEO_LINE	geo_line;
	OSCAN_COD		cod;
	sgFloat 			k;

	*hobj_equ = NULL;
	if (lpPlane) plane = *lpPlane;
	else {
		if (!set_flat_on_path(hobj, &plane)) goto falseLab;
	}
	if (!get_status_path(hobj, &status)) goto falseLab;
	if (!(status & ST_FLAT)) goto falseLab;
	if (!apr_path(&vdim, hobj)) goto falseLab;
	if (!read_elem(&vdim, 0L, &node)) goto cancel;
	o_hcunit(matr);
	o_tdtran(matr, dpoint_inv(&node.p, &p));
	o_hcrot1(matr, &plane.v);
	if (!begin_rw(&vdim, 0L)) goto cancel;
	zc_init(&cut1, 0.);
	stat = ZC_NULL;
	idf = zcf_create_new_face(&cut1, stat);
	idp = zcp_create_new_path(&cut1, idf, stat);
	max_i = (short)((status & ST_CLOSE) ? vdim.num_elem - 1 : vdim.num_elem);
	for (i = 0; i < max_i; i++) {
		lpNode = (MNODE*)get_next_elem(&vdim);
		o_hcncrd(matr, &lpNode->p, &p);
		zcv_add_to_tail(&cut1, idp, &p, 0);
	}
	end_rw(&vdim);
	free_vdim(&vdim);
	if ((status & ST_CLOSE)) zcp_close(&cut1, idp);
	zc_init(&cut2, 0);

	Bool = zcf_equid(&cut1, &cut2, idf, height1, height2, arc_equd);
	zc_free(&cut1);
	if (!Bool) {
		zc_free(&cut2);
		goto cancel;
	}

	idf = zcf_get_first_face(&cut2);
	idp = zcp_get_first_path_in_face(&cut2, idf);
	if ( !idp ) {    
//		put_message(MSG_SHU_022, NULL, 1);
		zc_free(&cut2);
//		ret = G_OK;
		return TRUE;
	}
	if ((*hobj_equ = o_alloc(OPATH)) == NULL)	goto err;
	obj_path = (lpOBJ)(*hobj_equ);
	geo_path = (lpGEO_PATH)obj_path->geo_data;
	o_hcunit(geo_path->matr);
	init_listh(&geo_path->listh);
	idv = idvb = zcv_get_first_point(&cut2, idp, &p, &k);
	p.z = 0;
	o_minv(matr, matr2);
	o_hcncrd(matr2, &p, &vn);
	min = vn;
	max = vn;
	do {
		idv = zcv_get_next_point(&cut2, idv, &p, &k);
		if (idv == 0) break;	
		p.z = 0;
		o_hcncrd(matr2, &p, &vk);
		modify_limits_by_point(&vk, &min, &max);
		if((hobj_line = o_alloc(OLINE)) == NULL) goto err2;
		obj_line = (lpOBJ)hobj_line;
		obj_line->hhold = *hobj_equ;
		geo_line = (lpGEO_LINE)(obj_line->geo_data);
		geo_line->v1 = vn;
		geo_line->v2 = vk;
		
		attach_item_tail(&geo_path->listh, hobj_line);
		vn = vk;
	} while (idv != idvb);
	geo_path->min = min;
	geo_path->max = max;
	count = (short)geo_path->listh.num;

	if (count == 0) goto err2;
	if (!set_contacts_on_path(*hobj_equ)) goto err2;
	obj_path = (lpOBJ)(*hobj_equ);
	if (is_path_on_one_line(*hobj_equ))
	{
		obj_path->status |= ST_ON_LINE;
		obj_path->status &= ~ST_FLAT;
	}
	else
	{
		if (set_flat_on_path(*hobj_equ, NULL))
			obj_path->status |= ST_FLAT;
		else      
			obj_path->status &= ~ST_FLAT;
	}
	//if (!set_flat_on_path(*hobj_equ, NULL)) goto err2;
	if ((cod = test_self_cross_path(*hobj_equ,NULL)) == OSFALSE) goto err2;
	zc_free(&cut2);
	if (cod == OSTRUE) {
		obj_path = (lpOBJ)(*hobj_equ);
		obj_path->status |= ST_SIMPLE;
	}
	return TRUE;
err2:
	o_free(*hobj_equ, NULL);
	*hobj_equ = NULL;
err:
	zc_free(&cut2);
cancel:
	free_vdim(&vdim);
falseLab:
	put_message(MSG_SHU_037, NULL, 0);
	return FALSE;
}
