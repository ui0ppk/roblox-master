#include "../sg.h"

// 


typedef struct {
	lpD_POINT p;
	BOOL			first;
} DATA;
typedef DATA * lpDATA;

static OSCAN_COD pipe1_geo_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);
static OSCAN_COD pipe0_geo_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);


BOOL get_2_points_on_path(hOBJ hpath, lpD_POINT pp)
{
	SCAN_CONTROL 	sc;

	init_scan(&sc);
	sc.user_geo_scan = pipe1_geo_scan;
	sc.data          = pp;	// 1-  . -
	if (o_scan(hpath,&sc) != OSBREAK) return FALSE;
	return TRUE;
}


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

static OSCAN_COD pipe1_geo_scan(hOBJ hobj,lpSCAN_CONTROL lpsc)
{
	lpD_POINT			point, p = (lpD_POINT)lpsc->data;
	lpGEO_LINE		line;
	lpOBJ					obj;
	lpGEO_SPLINE	spline;

	switch (lpsc->type) {
		default:
			put_message(INTERNAL_ERROR,"pipe1_geo_scan",0);
			return OSFALSE;
		case OLINE:
			obj = (lpOBJ)hobj;
			line = (lpGEO_LINE)obj->geo_data;
			p[0] = line->v1;
			p[1] = line->v2;
			break;
		case OSPLINE:
			obj = (lpOBJ)hobj;
			spline = (lpGEO_SPLINE)obj->geo_data;
			point = (lpD_POINT)spline->hpoint;
			p[0] = point[0];
			p[1] = point[1];
			break;
	}
	return OSBREAK;
}
