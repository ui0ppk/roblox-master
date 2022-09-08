#include "../sg.h"

static OSCAN_COD arc_scan(hOBJ hobj, lpSCAN_CONTROL lpsc);
static BOOL b_arc_np(lpNPW np, lpGEO_ARC arc, BOOL flag_arc, lpVDIM vdim);
static sgFloat def_angle_point_on_arc(lpD_POINT p, lpGEO_ARC a);

BOOL bo_arc_object(lpGEO_ARC arc, lpREGION_3D gab_arc, hOBJ hbody, lpVDIM vdim)
{
	lpOBJ					obj;//nb  = NULL;
	SCAN_CONTROL  sc;
	OSCAN_COD			cod;
	BO_ARC_DATA			cd;

	obj = (lpOBJ)hbody;
	if ( obj->type != OBREP ) {
		put_message(EMPTY_MSG,GetIDS(IDS_SG029),0); 
		return FALSE;
	}

	cd.arc 			= *arc;
	cd.flag_arc = TRUE;
	cd.gab 			= *gab_arc;

	init_scan(&sc);
	cd.vdim = vdim;
	sc.user_geo_scan = arc_scan;
	sc.data          = &cd;

	cod = o_scan(hbody, &sc);

	if ( cod != OSTRUE ) return FALSE;
	return TRUE;
}
#pragma argsused
static OSCAN_COD arc_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	int						i;
	lpBO_ARC_DATA	data;
	lpD_POINT			v;

	if (ctrl_c_press) {  	 
		put_message(CTRL_C_PRESS, NULL, 0);
		return OSFALSE;
	}

	data = (lpBO_ARC_DATA)(lpsc->data);
	npwg->type = TNPW;

	v = npwg->v;
	for (i = 1; i <= npwg->nov ; i++) {    
		if ( !o_hcncrd(lpsc->m,&v[i],&v[i])) return OSFALSE;
	}
	dpoint_minmax(&npwg->v[1], npwg->nov, &npwg->gab.min, &npwg->gab.max);
	if ( np_gab_inter(&npwg->gab, &data->gab) ) return OSTRUE;

	if ( !np_cplane(npwg) ) return OSFALSE;        

	if ( !b_arc_np(npwg, &data->arc, data->flag_arc, data->vdim) ) return OSFALSE;
	return OSTRUE;
}

static BOOL b_arc_np(lpNPW np, lpGEO_ARC arc, BOOL flag_arc, lpVDIM vdim)
{
#define DIST(v) ((v)->x*ka + (v)->y*kb + (v)->z*kc + kd)

	short 						i, v, edge, kp;
	short    					face;
	D_PLANE					pl;    
	GEO_LINE				line;
	BO_INTER_POINT  ip;   	
	NP_TYPE_POINT		type;   
	NP_VERTEX_LIST 	ver = {0,0,NULL,NULL};
	BOOL            rt;//nb  = TRUE;
	D_POINT					p[2];

	pl.v = arc->n;
	pl.d = dcoef_pl(&arc->vc, &pl.v);

	if ( !(rt = np_ver_alloc(&ver)) ) return FALSE;

	for (face = 1; face <= np->nof; face++) {
		if (fabs(np->p[face].v.x - pl.v.x) <= eps_n &&
				fabs(np->p[face].v.y - pl.v.y) <= eps_n &&
				fabs(np->p[face].v.z - pl.v.z) <= eps_n ||
				fabs(np->p[face].v.x + pl.v.x) <= eps_n &&
				fabs(np->p[face].v.y + pl.v.y) <= eps_n &&
				fabs(np->p[face].v.z + pl.v.z) <= eps_n ) continue;
		ver.uk = 0;
		np->nov_tmp = np->maxnov;
		if ( !(rt=np_pl(np, face, &ver, &pl)) ) { rt = FALSE; goto end; }
		if (ver.uk == 0) continue;    
		line.v1 = np->v[ver.v[0].v];  
		line.v2 = np->v[ver.v[1].v];
		if ( !intersect_3d_la(&line, FALSE,	arc, flag_arc, p, &kp) ) {
			rt = FALSE;
			goto end;
		}
		if ( !kp ) continue;  
		for (i = 0; i < kp; i++) {
			type = np_point_belog_to_face1(np, &p[i], face, &edge, &v);
			if ( type == NP_OUT ) continue;
			ip.p = p[i];
			if ( flag_arc && (fabs(dpoint_distance(&p[i], &arc->vb)) < eps_d ||
												fabs(dpoint_distance(&p[i], &arc->ve)) < eps_d) ) { 
				if ( type == NP_IN ) 	ip.type = B_IN_ON;
				else 		              ip.type = B_ON_ON;
			} else {
				if ( type == NP_IN ) 	ip.type = B_IN_IN;
				else 		              ip.type = B_ON_IN;
			}
			ip.t = fabs(def_angle_point_on_arc(&ip.p, arc));
			if ( !add_elem(vdim,&ip) ) { rt = FALSE; goto end; }
			continue;
		}
	}
end:
	np_free_ver(&ver);
	return rt;
}

static sgFloat def_angle_point_on_arc(lpD_POINT p, lpGEO_ARC a)
{
	D_POINT w;
	sgFloat an;

	if ( dpoint_distance(p, &a->vb) < eps_d ) return 0;
	if ( dpoint_distance(p, &a->ve) < eps_d ) return a->angle;

//	memcpy(a1, a, sizeof(GEO_ARC));
//	memcpy(a2, a, sizeof(GEO_ARC));
//	dpoint_copy(&a1->ve, p);
//	dpoint_copy(&a2->vb, p);
	get_c_matr((lpGEO_CIRCLE)a, el_g_e, el_e_g);
	o_hcncrd(el_g_e, p, &w);
	an = v_angle(w.x, w.y);
	if(a->angle > 0.)
		while(an < a->ab)
			an += 2.*M_PI;
	else
		while(an > a->ab)
			an -= 2.*M_PI;
	an = an - a->ab;
	return an;
}

