#include "../sg.h"

typedef struct {
	VDIM		vdim;											// objct point in OXY
	lpMATR	matr;											// object matrix
	int	k;		       								 	// intersection points
	NP_TYPE_POINT lab;
} POINT_DATA;

typedef POINT_DATA 		* lpPOINT_DATA;

static OSCAN_COD point_scan(hOBJ hobj, lpSCAN_CONTROL lpsc);
static OSCAN_COD point_scan1(hOBJ hobj, lpSCAN_CONTROL lpsc);
static int sort_function( const void *a, const void *b);

D_POINT point;

/************************************************
 *       --- BOOL check_point_object ---
 *  Check point and body (point in, out, on body)
************************************************/

BOOL check_point_object(lpD_POINT p, hOBJ hobj, lpNP_TYPE_POINT answer)
{
	lpOBJ					obj;//nb = NULL;
	lpGEO_BREP  	lpgeobrep;
	REGION_3D   	gab_obj, gab_point;
	D_POINT				v1, v2;
	lpD_POINT			p1, p2;
	sgFloat 				alfa, min_cos, cs;
	int						i, min_ind;
	MATR					matr_inv;
	SCAN_CONTROL  sc;
	POINT_DATA		cd;
	OSCAN_COD			cod;

	obj = (lpOBJ)hobj;
	lpgeobrep = (lpGEO_BREP)obj->geo_data;
	if ( lpgeobrep->type != BODY ) {
		put_message(EMPTY_MSG, GetIDS(IDS_SG005), 0);
		return FALSE;
	}
	gab_obj.min = lpgeobrep->min;
	gab_obj.max = lpgeobrep->max;

	if ( !o_minv(lpgeobrep->matr,matr_inv) ) { 
		return FALSE;
	}

	if ( !o_hcncrd(matr_inv,p,&point) ) return FALSE;
	gab_point.min = point;
	gab_point.max = point;

	define_eps(&gab_obj.min, &gab_obj.max); 
	if ( np_gab_inter(&gab_obj, &gab_point) ) {
		*answer = NP_OUT;
		return TRUE;
	}
// find ray
	cd.k = 0;
	cd.matr = NULL;
	init_scan(&sc);
	sc.data          = &cd;
	sc.user_geo_scan = point_scan1;

	cod = o_scan(hobj, &sc);
	if (cod == OSBREAK && cd.lab == NP_ON) { //point on object
		*answer = NP_ON;
		return TRUE;
	}
	if ( cod != OSTRUE ) return FALSE;
	if ( cd.lab == NP_ON ) {		// points on ray

		// sort points
		if ( !sort_vdim(&cd.vdim, sort_function) ) return FALSE;

		begin_rw(&cd.vdim, 0);
		p1 = (D_POINT*)get_next_elem(&cd.vdim);
		dpoint_sub(p1, &point, &v1);
		dnormal_vector (&v1);
		min_cos = 1.;
		min_ind = 1;
		for( i=1; i < cd.vdim.num_elem-1; i++ ){
			p2 = (D_POINT*)get_next_elem(&cd.vdim);
			dpoint_sub(p2, &point, &v2);
			dnormal_vector (&v2);
			cs = fabs(dskalar_product( &v1, &v2));
			if (cs < min_cos) { min_cos = cs; min_ind = i; }
			v1 = v2;
		}
		end_rw(&cd.vdim);
		read_elem( &cd.vdim, min_ind, &v1 );
		read_elem( &cd.vdim, min_ind-1, &v2 );
		dnormal_vector (&v1);
		dnormal_vector (&v2);
		cs = dskalar_product( &v1, &v2);
		alfa = acos(v1.x);
		if (v1.y < 0) alfa = -alfa;
		alfa += acos(cs)/2;

		o_hcunit(matr_inv);
		o_tdtran(matr_inv, dpoint_inv(&point, &v1));
		o_tdrot(matr_inv, -3, alfa);
		o_tdtran(matr_inv, &point);
		cd.matr = matr_inv;
	}

	free_vdim(&cd.vdim);

	init_scan(&sc);
	sc.data          = &cd;
	sc.user_geo_scan = point_scan;

	cod = o_scan(hobj, &sc);
	switch ( cod ) {
		case OSTRUE:
			if ((cd.k/2)*2 == cd.k) *answer = NP_OUT;
			else                    *answer = NP_IN;
			return TRUE;
		case OSBREAK:
			*answer = NP_ON;
			return TRUE;
		case OSFALSE:
		case OSSKIP:
		default:
			return FALSE;
	}

}
#pragma argsused
static OSCAN_COD point_scan1(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	register  	 	short i;
	lpPOINT_DATA	data;

	if (ctrl_c_press) { 							 
		put_message(CTRL_C_PRESS, NULL, 0);
		return OSFALSE;
	}

	data = (lpPOINT_DATA)(lpsc->data);
	if ( scan_flg_np_begin ) {         // first NP
		if ( !init_vdim(&data->vdim, sizeof(D_POINT)) ) return OSFALSE;
		data->lab = NP_OUT;
	}

	for (i = 1; i <= npwg->nov ; i++) {
		if (dpoint_eq(&point, &npwg->v[i], eps_d)) {
			data->lab = NP_ON; 						 // point on NP
			return OSBREAK;
		}
		if ( npwg->v[i].z < point.z - eps_d  ||
				 npwg->v[i].z > point.z + eps_d	) continue;
		if ( npwg->v[i].x < point.x + eps_d ) continue;
		if ( npwg->v[i].y > point.y - 2*eps_d &&
				 npwg->v[i].y < point.y + 2*eps_d	) data->lab = NP_ON;
		if ( !add_elem(&data->vdim,&npwg->v[i]) ) return OSFALSE;
	}
	return OSTRUE;
}
#pragma argsused
static OSCAN_COD point_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	register  	 	short i;
	lpD_POINT 	 	v;
	lpPOINT_DATA	data;
	D_POINT				min, max;
	NP_TYPE_POINT lab;

	if (ctrl_c_press) { 												
		put_message(CTRL_C_PRESS, NULL, 0);
		return OSFALSE;
	}

	data = (lpPOINT_DATA)(lpsc->data);
	if ( scan_flg_np_begin ) {         // first NP
		data->k = 0;
	}

	npwg->type = TNPW;

	if ( data->matr ){
		v = npwg->v;
		for (i = 1; i <= npwg->nov ; i++) {     // coordinate projection
			if ( !o_hcncrd(data->matr,&v[i],&v[i])) goto err;
		}
	}
	dpoint_minmax(&npwg->v[1], npwg->nov, &min, &max);
	if ( max.x + eps_d < point.x ) return OSTRUE;
	if ( min.y - eps_d > point.y || max.y + eps_d < point.y ) return OSTRUE;
	if ( min.z - eps_d > point.z || max.z + eps_d < point.z ) return OSTRUE;

	if ( !np_cplane(npwg) ) goto err;        // find planes coefficients

	lab = np_point_np(&point, npwg);
	if (lab == NP_ON) {
		data->lab = NP_ON;
		return OSBREAK;
	}
	if (lab == NP_IN) data->k++;
	return OSTRUE;
err:
	return OSFALSE;
}
static int sort_function( const void *a, const void *b)
{
	sgFloat a1, a2;
	D_POINT	p1, p2;

	dpoint_sub((lpD_POINT)a, &point, &p1);
	dpoint_sub((lpD_POINT)b, &point, &p2);
	if (fabs(p1.y) < eps_d )	a1 = 0.;
	else	a1 = atan(p1.y / p1.x);
	if (fabs(p2.y) < eps_d )	a2 = 0.;
	else	a2 = atan(p2.y / p2.x);
	if ( a1 < a2 ) return -1;
	else 					 return  1;
}

