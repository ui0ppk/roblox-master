#include "../sg.h"

BOOL test_geo_to_plane(OBJTYPE type, void * geo, lpD_PLANE pl)
{
	lpGEO_ARC arc;

	switch (type) {
		case OLINE:
			if (!test_points_to_plane(2, (lpD_POINT)geo, pl))	goto met_error;
			break;
		case OARC:
		case OCIRCLE:
			arc = (lpGEO_ARC)(geo);
			if (!test_points_to_plane(1, &arc->vc, pl))	goto met_error;
			if (!coll_tst(&arc->n, TRUE, &pl->v, TRUE)) goto met_error;
			break;
		case OSPLINE:
/*			if (test_spline_on_plane((lpGEO_SPLINE)&geo, pl) != 1)
				goto met_error;*/
			break;
		default:
			put_message(INTERNAL_ERROR,GetIDS(IDS_SG039),0);
//			put_message(INTERNAL_ERROR,"test_geo_to_plane",0);
	}
	return TRUE;
met_error:
	el_geo_err = OBJ_OR_POINT_OUT_PLANE;
	return FALSE;
}
