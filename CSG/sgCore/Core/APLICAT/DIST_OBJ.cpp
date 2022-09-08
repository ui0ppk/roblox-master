#include "../sg.h"

typedef struct {
	sgFloat 	f_min;							 	// min dist
	D_POINT p;										
	D_POINT p_min;							
} DIST_DATA;

typedef DIST_DATA 		* lpDIST_DATA;

static OSCAN_COD dist_scan(hOBJ hobj, lpSCAN_CONTROL lpsc);
static OSCAN_COD dist_put_np(lpSCAN_CONTROL lpsc, short *par);
static OSCAN_COD dist_scan_line( lpSCAN_CONTROL lpsc);
static OSCAN_COD dist_scan_brep( lpSCAN_CONTROL lpsc);
static sgFloat dist_gab(lpD_POINT p, lpREGION_3D gab);
static sgFloat dist_point_to_lines(lpD_POINT p, lpNPW np, lpD_POINT pp_min);
static  void o_modify_limits_by_point(lpD_POINT p, lpREGION_3D reg);

/************************************************
 *       --- BOOL dist_point_to_hobj ---
 *  Distance from oject to point
 ************************************************/
sgFloat dist_point_to_hobj(lpD_POINT p, hOBJ hobj, lpD_POINT p_min)
{
	REGION_3D   	gab_obj;
	SCAN_CONTROL  sc;
	DIST_DATA			cd;
	OSCAN_COD			cod;

	if (!get_object_limits(hobj, &gab_obj.min, &gab_obj.max)) return -1;
	o_modify_limits_by_point(p, &gab_obj);
	define_eps(&gab_obj.min, &gab_obj.max); 
//	define_vi_regen_func();

	init_scan(&sc);
	cd.p = *p;
	cd.f_min = 1.e35;
	memset(&cd.p_min, 0, sizeof(D_POINT));
	sc.data          = &cd;
	sc.user_geo_scan = dist_scan;

	cod = o_scan(hobj, &sc);

	if (cod == OSFALSE) return -1;

	*p_min = cd.p_min;
//	*dist  = cd.f_min;

	return (cd.f_min);
}

#pragma argsused
static OSCAN_COD dist_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	OSCAN_COD  cod=OSFALSE;

	if ( ctrl_c_press ) return OSTRUE;

	if ( lpsc->type == OPOINT) {
		return OSTRUE;
	}

	return cod;

}
#pragma argsused
static OSCAN_COD dist_put_np(lpSCAN_CONTROL lpsc, short *par)
{
	OSCAN_COD ret = OSTRUE;

	switch (lpsc->type) {
		case OPOINT:
			break;
		case OBREP:
			ret = dist_scan_brep( lpsc);
			break;
		case OLINE:
		case OARC:
		case OCIRCLE:
		case OSPLINE:
		case OFRAME:
			ret = dist_scan_line( lpsc);
			break;
		default:	// OTEXT
			ret = OSTRUE;
 }
	return ret;
}
//for linear objects
#pragma argsused
static OSCAN_COD dist_scan_line( lpSCAN_CONTROL lpsc)
{
	MATR				matr_inv;
	sgFloat 			f;
	D_POINT			p, pp_min;
	lpDIST_DATA	data;

	data = (lpDIST_DATA)(lpsc->data);

	if  ( lpsc->m ) {          									
		if ( !o_minv(lpsc->m,matr_inv) ) return OSFALSE; 
		if ( !o_hcncrd(matr_inv, &data->p, &p) ) return OSFALSE;
	} else p = data->p;

	if ( (f = dist_point_to_lines(&p, npwg, &pp_min)) < 0 ) return OSFALSE;
	if ( f < data->f_min ) {
		data->f_min = f;
		if ( fabs(f) < eps_d ) { 
			data->p_min = data->p;
			return OSBREAK;
		}
		if  ( lpsc->m ) {           
			if ( !o_hcncrd(lpsc->m, &pp_min, &data->p_min) ) return OSFALSE;
		} else
			data->p_min = pp_min;
	}

	return OSTRUE;
}
// for brep
#pragma argsused
static OSCAN_COD dist_scan_brep( lpSCAN_CONTROL lpsc)
{
	MATR				matr_inv;
	sgFloat 			f;
	D_POINT			p, pp_min;
	lpDIST_DATA	data;

	if ( !np_write_np_tmp(npwg) ) return OSFALSE; 

	data = (lpDIST_DATA)(lpsc->data);

	if  ( lpsc->m ) {
		if ( !o_minv(lpsc->m,matr_inv) ) return OSFALSE; 
		if ( !o_hcncrd(matr_inv, &data->p, &p) ) return OSFALSE;
	}

	dpoint_minmax(&npwg->v[1], npwg->nov, &npwg->gab.min, &npwg->gab.max);
	if ( dist_gab(&p, &npwg->gab) >= data->f_min ) return OSTRUE;

	if ( !np_read_np_tmp(npwg) ) return OSFALSE; 
	if ( !np_cplane(npwg) ) return OSFALSE;  

	f = np_point_dist_to_np(&p, npwg, data->f_min, &pp_min);
	if ( f < data->f_min ) {
		data->f_min = f;
		if ( fabs(f) < eps_d ) { 
			data->p_min = data->p;
			return OSBREAK;
		}
		if  ( lpsc->m )
			if ( !o_hcncrd(lpsc->m, &pp_min, &data->p_min) ) return OSFALSE;
	}
	return OSTRUE;
}

static sgFloat dist_gab(lpD_POINT p, lpREGION_3D gab)
{
	int			i;
	sgFloat 	par[3];
	sgFloat	f;
	D_POINT	v = {0.,0.,0.};

	par[0] = gab->max.x - gab->min.x;
	par[1] = gab->max.y - gab->min.y;
	par[2] = gab->max.z - gab->min.z;
	o_create_box(par);

	dpoint_sub(&gab->min, &v, &v);
	for (i=1; i <= npwg->nov; i++) {
		dpoint_add(&npwg->v[i], &v, &npwg->v[i]);
	}
	np_cplane(npwg);  
	f = np_point_dist_to_np(p, npwg, 1.e35, &v);
	return( f );
}

static sgFloat dist_point_to_lines(lpD_POINT p, lpNPW np, lpD_POINT p_min)
{
	short 			i, v1, v2;
	sgFloat 		f, f_min = 1e35;
	lpD_POINT p1, p2;
	D_POINT		pp, pp_min;

	for (i = 1; i <= np->noe; i++) {
		v1 = np->efr[i].bv;		p1 = &np->v[v1];
		v2 = np->efr[i].ev;    p2 = &np->v[v2];
		f = dist_point_to_segment( p, p1, p2,	&pp);
		if ( f < f_min ) {
			f_min = f;
			if ( fabs(f) < eps_d ) { 
				*p_min = *p;
				return 0.;
			}
			pp_min = pp;
		}
	}

	*p_min = pp_min;

	return f_min;
}
static  void o_modify_limits_by_point(lpD_POINT p, lpREGION_3D reg)
{
	if (p->x < reg->min.x) reg->min.x = p->x;
	if (p->y < reg->min.y) reg->min.y = p->y;
	if (p->z < reg->min.z) reg->min.z = p->z;
	if (p->x > reg->max.x) reg->max.x = p->x;
	if (p->y > reg->max.y) reg->max.y = p->y;
	if (p->z > reg->max.z) reg->max.z = p->z;
}

