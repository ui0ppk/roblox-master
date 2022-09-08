#include "../sg.h"

BOOL test_wrd_line(lpGEO_LINE l)
{
	if(dpoint_eq(&l->v1, &l->v2, eps_d)){
		el_geo_err = EL_LINE_NULL_LEN;
		return FALSE;
	}
	return TRUE;
}

BOOL test_wrd_arc(lpGEO_ARC a, BOOL *flag_arc)
{

	if(fabs(a->r) < eps_d){
    el_geo_err = EL_NULL_RAD;
		return FALSE;
	}
	if(!(*flag_arc))
		return TRUE;

	if(fabs(a->angle) >= 2*M_PI){
		*flag_arc = FALSE;
		return TRUE;
	}
	if(dpoint_eq(&a->vb, &a->ve, eps_d)){
		if(fabs(a->angle) < M_PI){
			 el_geo_err = EL_ARC_NULL_LEN;
			 return FALSE;
		}
		else
			*flag_arc = FALSE;
	}
	return TRUE;
}
