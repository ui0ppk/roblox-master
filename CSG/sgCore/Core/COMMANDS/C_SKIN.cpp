#include "../sg.h"

/***********************************************************
*/
BOOL init_get_point_on_path(hOBJ hobj, sgFloat *length);
void term_get_point_on_path(void);
BOOL get_point_on_path(sgFloat t, lpD_POINT p);
BOOL calc_curve_length(hOBJ hobj, sgFloat *length);

typedef struct {
	sgFloat 		length, param;
	lpD_POINT	p;
} DATA;

typedef struct {
	SPLY_DAT	sply_dat;
	sgFloat		length;
	hOBJ			hobj;
} SKIN_DATA;

static SKIN_DATA *skin_data = NULL;
static OSCAN_COD skin4_geo_scan(hOBJ hobj, lpSCAN_CONTROL lpsc);

//------------------------------------------------------------------------------
/**********************************************************
*/
enum {POWER_U, POWER_V, E_SOLID, BUTTON_CREATE, BUTTON_CANCEL,	ENUM_NUM};

BOOL init_get_point_on_path(hOBJ hobj, sgFloat *length){
	lpOBJ					obj;
	BOOL					ret = TRUE;

	if ((skin_data = (SKIN_DATA*)SGMalloc(sizeof(SKIN_DATA))) == NULL) {
		put_message(NOT_ENOUGH_HEAP, NULL, 0);
		return FALSE;
	}
	obj = (lpOBJ)hobj;
	switch (obj->type) {
		case OSPLINE:
			if (!begin_use_sply((lpGEO_SPLINE)obj->geo_data,
													&skin_data->sply_dat)) {
				ret = FALSE;
			}
			break;
		case OLINE:
		case OARC:
		case OCIRCLE:
		case OPATH:
			if (!calc_curve_length(hobj, &skin_data->length)) ret = FALSE;
			break;
		default:
			put_message(EMPTY_MSG,GetIDS(IDS_SG242),0);
			ret = FALSE;
			break;
	}
	skin_data->hobj = (ret) ? hobj : 0;
	if (length) *length = skin_data->length;
	return ret;
}
void term_get_point_on_path(void)
{
	lpOBJ					obj;

	if (!skin_data) return;
	obj = (lpOBJ)skin_data->hobj;
	switch (obj->type) {
		case OSPLINE:
			end_use_sply((lpGEO_SPLINE)obj->geo_data, &skin_data->sply_dat);
			break;
		case OLINE:
		case OARC:
		case OCIRCLE:
		case OPATH:
			break;
		default:
			put_message(EMPTY_MSG,GetIDS(IDS_SG242),0);
			break;
	}
	SGFree(skin_data); skin_data = NULL;
	return;
}

BOOL get_point_on_path(sgFloat t, lpD_POINT p)
{
	lpOBJ					obj;
	BOOL					ret = TRUE;
	DATA					data;
	SCAN_CONTROL	sc;

	if (!skin_data) {
		
		put_message(INTERNAL_ERROR,NULL,0);
		return FALSE;
	}
	if (t < 0.) t = 0.;
	if (t > 1.) t = 1.;
	obj = (lpOBJ)skin_data->hobj;
	if (obj->type == OSPLINE) {
		if (obj->status & ST_DIRECT) t = 1. - t;	
		get_point_on_sply(&skin_data->sply_dat, t, p, FALSE);
	} else {
		data.param = t * skin_data->length;
		data.length = 0;
		data.p = p;
		init_scan(&sc);
		sc.user_geo_scan = skin4_geo_scan;
		sc.data = &data;
		if (o_scan(skin_data->hobj,&sc) == OSFALSE) ret = FALSE;
	}
	if (ret == FALSE) {SGFree(skin_data); skin_data = NULL;}
	return ret;
}

static OSCAN_COD skin4_geo_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	lpOBJ 				obj;
	lpGEO_CIRCLE	circle;
	lpGEO_LINE		line;
	lpGEO_ARC			arc;
	DATA					*data;
	sgFloat				len;
	lpD_POINT			vb, ve;
	OSCAN_COD			ret = OSTRUE;

	data = (DATA*)lpsc->data;
	switch (lpsc->type) {
		case OLINE:
			obj = (lpOBJ)hobj;
			line = (lpGEO_LINE)obj->geo_data;
			len  = dpoint_distance(&line->v1, &line->v2);
			if (data->length + len + eps_d < data->param) data->length += len;
			else {
				if (lpsc->status & ST_DIRECT) { vb = &line->v2; ve = &line->v1; }
				else                          { vb = &line->v1; ve = &line->v2; }
				dpoint_parametr(vb, ve, (data->param - data->length) / len, data->p);
				ret = OSBREAK;
			}
			break;
		case OARC:
			obj = (lpOBJ)hobj;
			arc = (lpGEO_ARC)obj->geo_data;
			len = arc->r * fabs(arc->angle);
			if (data->length + len + eps_d < data->param) data->length += len;
			else {
				len = (data->param - data->length) / len;
				if (lpsc->status & ST_DIRECT) len = 1. - len;
				init_ecs_arc(arc, ECS_ARC);
				get_point_on_arc(len, data->p);
				ret = OSBREAK;
			}
			break;
		case OCIRCLE:
			obj = (lpOBJ)hobj;
			circle = (lpGEO_CIRCLE)obj->geo_data;
			len = circle->r * 2 * M_PI;
			if (data->length + len + eps_d < data->param) data->length += len;
			else {
				len = (data->param - data->length) / len;
				if (lpsc->status & ST_DIRECT) len = 1. - len;
				init_ecs_arc((lpGEO_ARC)circle, ECS_CIRCLE);
				get_point_on_arc(len, data->p);
				ret = OSBREAK;
			}
			break;
		default:
			put_message(EMPTY_MSG,GetIDS(IDS_SG242),0);
			return OSFALSE;
	}
	return ret;
}
