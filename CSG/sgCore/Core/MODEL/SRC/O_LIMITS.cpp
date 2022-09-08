#include "../../sg.h"

BOOL get_object_limits_lsk_draw(hOBJ hobj, lpD_POINT min, lpD_POINT max, lpMATR m, BOOL draw);
static void  o_modify_limits_by_point(lpD_POINT p, lpREGION_3D reg);
static OSCAN_COD limits_geo_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);


static OSCAN_COD  (**limits_type_g)(void * geo, lpSCAN_CONTROL lpsc );

//---     
BOOL get_object_limits(hOBJ hobj, lpD_POINT min, lpD_POINT max)
{
	return get_object_limits_lsk(hobj, min, max, NULL);
}
//---     
BOOL get_object_limits_lsk(hOBJ hobj, lpD_POINT min, lpD_POINT max, lpMATR m)
{
	return get_object_limits_lsk_draw(hobj, min, max, m, TRUE);
}
//---     
BOOL get_object_limits_lsk_draw(hOBJ hobj, lpD_POINT min, lpD_POINT max, lpMATR m, BOOL draw)
{
	SCAN_CONTROL sc;
	REGION_3D lim;

  limits_type_g = (OSCAN_COD (**)(void *geo, lpSCAN_CONTROL lpsc))GetMethodArray(OMT_LIMITS);

	min->x = min->y = min->z =  GAB_NO_DEF;
	max->x = max->y = max->z = -GAB_NO_DEF;
	lim.min = *min;
	lim.max = *max;
	init_scan(&sc);
	sc.m = m;
	sc.scan_draw = draw;
	sc.user_geo_scan = limits_geo_scan;
	sc.lparam = (LPARAM)&lim;
	if ( o_scan(hobj,&sc) == OSFALSE ) return FALSE;
	*min = lim.min;
	*max = lim.max;
	return TRUE;
}

#pragma argsused
static OSCAN_COD limits_geo_scan(hOBJ hobj,lpSCAN_CONTROL lpsc)
{
	lpOBJ obj;
	BOOL  ret;

	if ( !limits_type_g[lpsc->type]) {
		handler_err(ERR_NUM_OBJECT);
		return OSFALSE;
	}

	obj = (lpOBJ)hobj;
	ret = limits_type_g[lpsc->type](obj->geo_data,lpsc);
	return (OSCAN_COD)ret;
}

OSCAN_COD get_point_limits(void * geo, lpSCAN_CONTROL lpsc)
{
	D_POINT p;

  if ( lpsc->m ) o_hcncrd(lpsc->m,(lpD_POINT)geo,&p);
  else           memcpy(&p,(lpD_POINT)geo,sizeof(p));
	o_modify_limits_by_point(&p,(lpREGION_3D)lpsc->lparam);
	return OSTRUE;
}

OSCAN_COD get_line_limits(void * geo, lpSCAN_CONTROL lpsc)
{
	D_POINT p1,p2;

	if ( lpsc->m ) {
    o_hcncrd(lpsc->m,&((lpGEO_LINE)geo)->v1,&p1);
    o_hcncrd(lpsc->m,&((lpGEO_LINE)geo)->v2,&p2);
	} else {
    memcpy(&p1,&((lpGEO_LINE)geo)->v1,sizeof(p1));
    memcpy(&p2,&((lpGEO_LINE)geo)->v2,sizeof(p2));
	}
	o_modify_limits_by_point(&p1,(lpREGION_3D)lpsc->lparam);
	o_modify_limits_by_point(&p2,(lpREGION_3D)lpsc->lparam);
	return OSTRUE;
}

#pragma argsused
OSCAN_COD get_circle_limits(void * g, lpSCAN_CONTROL lpsc)
{
	short         i, num;
	D_POINT     p;
	ARC_DATA    ad;
	GEO_CIRCLE  geo;

	memcpy(&geo,g,sizeof(geo));
	if ( lpsc->m ) o_trans_arc((lpGEO_ARC)&geo, lpsc->m, OECS_CIRCLE);
	num = o_init_ecs_arc(&ad,(lpGEO_ARC)&geo, OECS_CIRCLE,vi_h_tolerance);
	for (i = 0; i <= num; i++) {
		o_get_point_on_arc(&ad,(sgFloat)i / num, &p);
		o_modify_limits_by_point(&p, (lpREGION_3D)lpsc->lparam);
	}
	return OSTRUE;
}

#pragma argsused
OSCAN_COD get_arc_limits(void * g, lpSCAN_CONTROL lpsc)
{
	short       i, num;
	D_POINT   p;
	ARC_DATA  ad;
	GEO_ARC   geo;

	memcpy(&geo,g,sizeof(geo));
	if ( lpsc->m ) o_trans_arc(&geo, lpsc->m, OECS_ARC);
  if ( lpsc->status & ST_ORIENT ) geo.angle = -geo.angle;
	num = o_init_ecs_arc(&ad,(lpGEO_ARC)&geo, OECS_ARC, vi_h_tolerance);
	for (i = 0; i <= num; i++) {
		o_get_point_on_arc(&ad,(sgFloat)i / num, &p);
		o_modify_limits_by_point(&p, (lpREGION_3D)lpsc->lparam);
	}
	return OSTRUE;
}

#pragma argsused
OSCAN_COD get_brep_limits(void * geo, lpSCAN_CONTROL lpsc)
{
	short       i;
	D_POINT   p;
	lpD_POINT v;

	v = npwg->v;
	for (i = 1; i <= npwg->nov; i++) {
		o_hcncrd(lpsc->m,&v[i],&p);
		o_modify_limits_by_point(&p,(lpREGION_3D)lpsc->lparam);
	}
	return OSTRUE;
}
#pragma argsused
OSCAN_COD get_spline_limits(void * geo, lpSCAN_CONTROL lpsc)
{
	D_POINT		    p;
  lpGEO_SPLINE  g = (lpGEO_SPLINE)geo;
  SPLY_DAT      sply_dat;

  if ( !begin_use_sply( g, &sply_dat) ) return OSFALSE;
	get_first_sply_point(&sply_dat, 0., FALSE, &p);
	do {
		if ( lpsc->m )
			o_hcncrd(lpsc->m,&p, &p);
		o_modify_limits_by_point(&p,(lpREGION_3D)lpsc->lparam);
	} while(get_next_sply_point(&sply_dat, &p));

  end_use_sply( g, &sply_dat );
	return OSTRUE;
}

BOOL lim_sym_line(lpD_POINT pb, lpD_POINT pe, void  *user_data);
#pragma argsused
OSCAN_COD get_text_limits(void * geo, lpSCAN_CONTROL lpsc)
{
	if(!draw_geo_text((lpGEO_TEXT)geo, lim_sym_line, lpsc)) return OSFALSE;
  return OSTRUE;
}
OSCAN_COD get_dim_limits(void * geo, lpSCAN_CONTROL lpsc)
{
	if(!draw_geo_dim((lpGEO_DIM)geo, lim_sym_line, lpsc)) return OSFALSE;
	return OSTRUE;
}
BOOL lim_sym_line(lpD_POINT pb, lpD_POINT pe, void  *user_data)
{
lpSCAN_CONTROL lpsc = (lpSCAN_CONTROL)user_data;

	if ( lpsc->m ){
		o_hcncrd(lpsc->m, pb, pb);
		o_hcncrd(lpsc->m, pe, pe);
	}
	o_modify_limits_by_point(pb, (lpREGION_3D)lpsc->lparam);
	o_modify_limits_by_point(pe, (lpREGION_3D)lpsc->lparam);
	return TRUE;
}

OSCAN_COD get_frame_limits(void * g1, lpSCAN_CONTROL lpsc)
{
	GEO_OBJ     geo;
	OBJTYPE     type;
	short       color;
	UCHAR       ltype, lthick;

	frame_read_begin((lpGEO_FRAME)g1);
	while ( frame_read_next(&type, &color, &ltype, &lthick, &geo)) {
		if ( !limits_type_g[type](&geo,lpsc) ) return OSFALSE;
  }
  return OSTRUE;
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

