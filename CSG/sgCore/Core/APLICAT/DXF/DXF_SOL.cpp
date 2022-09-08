#include "../../sg.h"

BOOL dxf_solid(lpLISTH listh, lpVDIM faces)
{
	sgFloat 		height = 0;               
	D_POINT 	v = {0,0,1};              
	D_POINT		p1, p2, p3, p4;
	BOOL			flag_h = FALSE;
	short     color = 0;
	int			  num = 0;
	LAYER			elem;
	GEO_LINE	line;
	GEO_PATH	path;
	LISTH			listh2;
	hOBJ			hobj;
	FACE4 		face;

	while (1) {
		if (!get_group(dxfg)) return FALSE;
		switch (dxfg->gr) {
			case 8:
				if (find_elem(&dxfg->layers, &num, dxf_cmp_func, (void*)dxfg->str)) {
					if (!read_elem(&dxfg->layers, num, &elem)) return FALSE;
					color = elem.color;
				}
				continue;
			case 62:
				color = dxfg->p;
				continue;
			case 10:
				p1 = dxfg->xyz;
				continue;
			case 11:
				p2 = dxfg->xyz;
				continue;
			case 12:
				p4 = dxfg->xyz;
				continue;
			case 13:
				p3 = dxfg->xyz;
				continue;
			case 210:                  
				v = dxfg->xyz;
				continue;
			case 39:                 
				height = dxfg->v;
				if (height > eps_d) flag_h = TRUE;
				continue;
			case 0:
				break;
			default:
				continue;
		}
		color = dxf_color(color);
		break;
	}

	init_listh(&listh2);
	ocs_gcs(&v, dxfg->ocs_gcs, dxfg->gcs_ocs);
	o_hcncrd(dxfg->ocs_gcs, &p1, &p1);
	o_hcncrd(dxfg->ocs_gcs, &p2, &p2);
	o_hcncrd(dxfg->ocs_gcs, &p3, &p3);
	o_hcncrd(dxfg->ocs_gcs, &p4, &p4);
	if (!flag_h) {	
		face.color = color;
		face.flag_v = 0;
		face.layer = (short)num;
		face.flag = TRUE;
    face.p1 = p1;
		face.p2 = p2;
		face.p3 = p3;
		face.p4 = p4;
		return add_elem(faces, &face);
	}
	if (!dpoint_eq(&p1, &p2, eps_d)) {
		line.v1 = p1;
		line.v2 = p2;
		if ( !cr_add_obj(OLINE, color, (lpGEO_SIMPLE)&line,
										 &listh2, NULL, FALSE)) goto err;
	}
	if (!dpoint_eq(&p2, &p3, eps_d)) {
		line.v1 = p2;
		line.v2 = p3;
		if ( !cr_add_obj(OLINE, color, (lpGEO_SIMPLE)&line,
										 &listh2, NULL, FALSE)) goto err;
	}
	if (!dpoint_eq(&p3, &p4, eps_d)) {
		line.v1 = p3;
		line.v2 = p4;
		if ( !cr_add_obj(OLINE, color, (lpGEO_SIMPLE)&line,
										 &listh2, NULL, FALSE)) goto err;
	}
	if (!dpoint_eq(&p4, &p1, eps_d)) {
		line.v1 = p4;
		line.v2 = p1;
		if ( !cr_add_obj(OLINE, color, (lpGEO_SIMPLE)&line,
										 &listh2, NULL, FALSE)) goto err;
	}
	if (!listh2.num) return TRUE;
	memset(&path, 0, sizeof(path));
	o_hcunit(path.matr);
	path.listh = listh2;
	hobj = listh2.hhead;
	path.min.x = path.min.y =  path.min.z =  GAB_NO_DEF;
	path.max.x = path.max.y =  path.max.z = -GAB_NO_DEF;
	while (hobj) {
		if (!get_object_limits(hobj, &p1, &p2)) goto err;
		union_limits(&p1, &p2, &path.min, &path.max);
		get_next_item(hobj, &hobj);
	}
	return dxf_extrude(OPATH, color, (lpGEO_SIMPLE)&path, listh,
										 flag_h, &v, height, TRUE);
err:
	return FALSE;
}
