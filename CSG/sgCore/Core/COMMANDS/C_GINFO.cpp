#include "../sg.h"

static OSCAN_COD ccl_geo_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	sgFloat 				*length = (sgFloat*)lpsc->data;
	GEO_OBJ				curr_geo;
	OBJTYPE				type;
	lpOBJ					obj;
	lpGEO_SPLINE	spline;
	SPLY_DAT			sply_dat;

	type = get_simple_obj_geo(hobj, &curr_geo);
	switch (type) {
		case OLINE:
			*length += dpoint_distance(&curr_geo.line.v1, &curr_geo.line.v2);
			break;
		case OCIRCLE:
			*length += 2. * M_PI * curr_geo.circle.r;
			break;
		case OARC:
			*length += curr_geo.arc.r * fabs(curr_geo.arc.angle);
			break;
		case OSPLINE:
			obj = (lpOBJ)hobj;
			spline = (lpGEO_SPLINE)obj->geo_data;
			if (!begin_use_sply(spline, &sply_dat)) {
				return OSFALSE;
			}
			*length += spl_length(&sply_dat);
			end_use_sply(spline, &sply_dat);
			break;
	}
	return OSTRUE;
}

BOOL calc_curve_length(hOBJ hobj, sgFloat *length)

{
	SCAN_CONTROL 	sc;

	*length = 0;
	init_scan(&sc);
	sc.user_geo_scan  = ccl_geo_scan;
	sc.data 	        = (void *)length;
	if (o_scan(hobj,&sc) == OSFALSE) return FALSE;
	return TRUE;
}
