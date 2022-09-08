#include "../sg.h"


typedef struct {
	lpVDIM		vdim;				
	D_POINT		p1;         
	D_POINT 	p2;
	REGION_3D gab;
} LINE_DATA;

typedef LINE_DATA 		* lpLINE_DATA;

//static void bo_def_axis(lpD_POINT p1, lpD_POINT p2);
static OSCAN_COD line_scan(hOBJ hobj, lpSCAN_CONTROL lpsc);
static BOOL b_line_np(lpNPW np, lpD_POINT pb, lpD_POINT pe, lpVDIM vdim);

BOOL bo_line_object(lpD_POINT pb, lpD_POINT pe, hOBJ hobj,
										lpVDIM point)
{
	short 					i;
	lpOBJ					obj;//nb  = NULL;
	lpGEO_BREP  	lpgeobrep;
	MATR					matr, matr_inv;
	REGION_3D   	gab_obj;
	SCAN_CONTROL  sc;
	OSCAN_COD			cod;
	LINE_DATA			cd;
	lpBO_INTER_POINT 	v;

	obj = (lpOBJ)hobj;
	if ( obj->type != OBREP ) {
		put_message(EMPTY_MSG,GetIDS(IDS_SG029),0);
		return FALSE;
	}
	lpgeobrep = (lpGEO_BREP)obj->geo_data;
	gab_obj.min = lpgeobrep->min;
	gab_obj.max = lpgeobrep->max;

	memcpy(matr, lpgeobrep->matr, sizeof(matr));
	if ( !o_minv(lpgeobrep->matr,matr_inv) ) { 
		return FALSE;
	}

	if ( !o_hcncrd(matr_inv,pb,&cd.p1) ) return FALSE;
	if ( !o_hcncrd(matr_inv,pe,&cd.p2) ) return FALSE;

	cd.vdim = point;
	if (cd.p1.x < cd.p2.x) cd.gab.min.x = cd.p1.x;
	else             			 cd.gab.min.x = cd.p2.x;
	if (cd.p1.y < cd.p2.y) cd.gab.min.y = cd.p1.y;
	else             			 cd.gab.min.y = cd.p2.y;
	if (cd.p1.z < cd.p2.z) cd.gab.min.z = cd.p1.z;
	else             			 cd.gab.min.z = cd.p2.z;
	if (cd.p1.x > cd.p2.x) cd.gab.max.x = cd.p1.x;
	else             			 cd.gab.max.x = cd.p2.x;
	if (cd.p1.y > cd.p2.y) cd.gab.max.y = cd.p1.y;
	else             			 cd.gab.max.y = cd.p2.y;
	if (cd.p1.z > cd.p2.z) cd.gab.max.z = cd.p1.z;
	else             			 cd.gab.max.z = cd.p2.z;

	define_eps(&gab_obj.min, &gab_obj.max); 
	if ( np_gab_inter(&gab_obj, &cd.gab) )		return TRUE;

	init_scan(&sc);
	sc.user_geo_scan = line_scan;
	sc.data          = &cd;

	cod = o_scan(hobj, &sc);

	if ( !begin_rw(point, 0) ) return FALSE;
	for (i=0; i < point->num_elem; i++) {
		v = (lpBO_INTER_POINT)get_next_elem(point);
		if ( !o_hcncrd(matr,&(v->p),&(v->p)) ) {
			end_rw(point);
			return FALSE;
		}
		if ( !o_hcncrd(matr,&(v->p1),&(v->p1)) ) {
			end_rw(point);
			return FALSE;
		}
		if ( !o_hcncrd(matr,&(v->p2),&(v->p2)) ) {
			end_rw(point);
			return FALSE;
		}
	}
	end_rw(point);

	if ( cod != OSTRUE ) return FALSE;
	return TRUE;
}
#pragma argsused
static OSCAN_COD line_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	lpLINE_DATA	data;

	if (ctrl_c_press) { 							 
		put_message(CTRL_C_PRESS, NULL, 0);
		return OSFALSE;
	}

	data = (lpLINE_DATA)(lpsc->data);

	npwg->type = TNPW;

	dpoint_minmax(&npwg->v[1], npwg->nov, &npwg->gab.min, &npwg->gab.max);
	if ( np_gab_inter(&npwg->gab, &data->gab) ) return OSTRUE;

	if ( !np_cplane(npwg) ) return OSFALSE;  

	if ( !b_line_np(npwg, &data->p1, &data->p2, data->vdim) ) return OSFALSE;
	return OSTRUE;
}

static BOOL b_line_np(lpNPW np, lpD_POINT pb, lpD_POINT pe, lpVDIM vdim)
{
#define DIST(v) ((v)->x*ka + (v)->y*kb + (v)->z*kc + kd)

	short 						v, edge, f_edge;
	short    					face, loop;
	short    					p1, p2;
	sgFloat 					ka, kb, kc, kd, cs;
	sgFloat 					r, f1, f2;
	D_POINT					v1, v2;
	BO_INTER_POINT  ip;   	
	NP_TYPE_POINT		type;  

	for (face = 1; face <= np->nof; face++) {
		ka = np->p[face].v.x; kb = np->p[face].v.y;
		kc = np->p[face].v.z;	kd = np->p[face].d;
		f1 = DIST(pb);		p1 = sg(f1,eps_d);
		f2 = DIST(pe);		p2 = sg(f2,eps_d);
		if (p1 == 0 && p2 == 0) continue;  

		if (p1 == 0) {   
			type = np_point_belog_to_face1(np, pb, face, &edge, &v);
			if ( type == NP_OUT ) continue; 
			ip.p = *pb;
			ip.t = 0.;
			if ( type == NP_IN ) 	ip.type = B_IN_ON;
			else 		              ip.type = B_ON_ON;
			loop = np->f[face].fc;
			f_edge = np->c[loop].fe;
            ip.p1 = np->v[np->efr[(int)abs(f_edge)].bv];
            ip.p2 = np->v[np->efr[(int)abs(f_edge)].ev];
			cs = dskalar_product(&ip.p1, &ip.p2);
			if ( fabs(cs) -eps_d > 1 ) {  
				dvector_product(dpoint_sub(&ip.p1,&ip.p, &v1), &np->p[face].v, &v2);
				dpoint_add(&ip.p, &v2, &ip.p2);
			}
			if ( !add_elem(vdim,&ip) ) return FALSE;
			continue;
		}

		if (p2 == 0) {  
			type = np_point_belog_to_face1(np, pe, face, &edge, &v);
			if ( type == NP_OUT ) continue; 
			ip.p = *pe;
			ip.t = dpoint_distance(pb, pe);
			if ( type == NP_IN ) 	ip.type = B_IN_ON;
			else 		              ip.type = B_ON_ON;
			loop = np->f[face].fc;
			f_edge = np->c[loop].fe;
            ip.p1 = np->v[np->efr[(int)abs(f_edge)].bv];
            ip.p2 = np->v[np->efr[(int)abs(f_edge)].ev];
			cs = dskalar_product(&ip.p1, &ip.p2);
			if ( fabs(cs) -eps_d > 1 ) {  
				dvector_product(dpoint_sub(&ip.p1,&ip.p, &v1), &np->p[face].v, &v2);
				dpoint_add(&ip.p, &v2, &ip.p2);
			}
			if ( !add_elem(vdim,&ip) ) return FALSE;
			continue;
		}

		if (p1 == p2) continue;  

		
		r = fabs(f1)/(fabs(f2) + fabs(f1));
		ip.p.x = pb->x + (pe->x - pb->x)*r;
		ip.p.y = pb->y + (pe->y - pb->y)*r;
		ip.p.z = pb->z + (pe->z - pb->z)*r;
		ip.t 	 = dpoint_distance(pb, &ip.p);

		type = np_point_belog_to_face1(np, &ip.p, face, &edge, &v);
		if ( type == NP_OUT ) continue;
		if ( type == NP_IN ) 	ip.type = B_IN_IN;
		else 		              ip.type = B_ON_IN;
		loop = np->f[face].fc;
		f_edge = np->c[loop].fe;
        ip.p1 = np->v[np->efr[(int)abs(f_edge)].bv];
        ip.p2 = np->v[np->efr[(int)abs(f_edge)].ev];
		cs = dskalar_product(&ip.p1, &ip.p2);
		if ( fabs(cs) -eps_d > 1 ) {  
			dvector_product(dpoint_sub(&ip.p1,&ip.p, &v1), &np->p[face].v, &v2);
			dpoint_add(&ip.p, &v2, &ip.p2);
		}
		if ( !add_elem(vdim,&ip) ) return FALSE;
	}
	return TRUE;
}

