#include "../../sg.h"

static OSCAN_COD trans_pre_scan(hOBJ hobj, lpSCAN_CONTROL lpsc);


BOOL o_trans(hOBJ hobj, lpMATR matr)
{
	SCAN_CONTROL sc;
	init_scan(&sc);
	sc.user_pre_scan = trans_pre_scan;
	sc.lparam = (LPARAM)matr;
	if ( o_scan(hobj,&sc) == OSFALSE ) return FALSE;
	SetModifyFlag(TRUE);  //   
	return TRUE;
}
static OSCAN_COD trans_pre_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	lpOBJ obj;

	obj = (lpOBJ)hobj;
	trans_geo(obj->type, obj->geo_data, (lpMATR)lpsc->lparam);
	if ( lpsc->type == OINSERT ) return OSSKIP;
	return OSTRUE;
}
void trans_geo(OBJTYPE type, void * geo, lpMATR matr)
{
void  (**trans_type)(void * geo, lpMATR matr);

  trans_type = (void(**)(void *geo, lpMATR matr))GetMethodArray(OMT_TRANS);
	if ( trans_type[type] )
		trans_type[type](geo,matr);
}

void   trans_point     (void * geo, lpMATR matr)
{
  lpGEO_POINT p = (GEO_POINT*)geo;
	o_hcncrd(matr,p,p);
}
void   trans_line      (void * geo, lpMATR matr)
{
	lpGEO_LINE p = (GEO_LINE*)geo;
	o_hcncrd(matr,&p->v1,&p->v1);
	o_hcncrd(matr,&p->v2,&p->v2);
}
void   trans_circle    (void * geo, lpMATR matr)
{
	o_trans_arc((lpGEO_ARC)geo, matr, OECS_CIRCLE);
}
void   trans_arc       (void * geo, lpMATR matr)
{
	o_trans_arc((lpGEO_ARC)geo, matr, OECS_ARC);
}
void   trans_complex   (void * geo, lpMATR matr)
{
	o_hcmult(((lpGEO_BREP)geo)->matr,matr);
}

void   trans_dim       (void * geo, lpMATR matr)
{
	odim_trans((lpGEO_DIM)geo, matr);
}

void   trans_spline   (void * geo, lpMATR matr)
{
	register short i;
	lpGEO_SPLINE g = (GEO_SPLINE*)geo;
	lpD_POINT    p;
	lpSNODE			 deriv;
	D_POINT			 pp;

	p = (lpD_POINT)g->hpoint;
	if (!(g->type & SPLY_APPR) && g->numd > 0) {
		deriv = (lpSNODE)g->hderivates;
		for ( i = 0; i < g->numd; i++ ) {
			pp = p[deriv[i].num];
			o_trans_vector(matr, &pp, (lpD_POINT)&deriv[i].p);
		}
	}
	for ( i = 0; i < g->nump; i++ ) {
		o_hcncrd(matr, &p[i], &p[i]);
	}
}

void get_scanned_geo(hOBJ hobj, lpSCAN_CONTROL lpsc, void * geo)
{
  lpOBJ obj;

  obj = (lpOBJ)hobj;
  memcpy(geo,obj->geo_data,geo_size[obj->type]);
  if ( lpsc->m )
    trans_geo(lpsc->type, geo, lpsc->m);
	if ( lpsc->type == OARC && (lpsc->status&ST_ORIENT) )
    ((lpGEO_ARC)geo)->angle = -((lpGEO_ARC)geo)->angle;
}

