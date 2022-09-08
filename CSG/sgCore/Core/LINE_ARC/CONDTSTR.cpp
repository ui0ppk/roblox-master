#include "../sg.h"

void trans_cond_to_plane(short cnum, lpD_POINT pp, lpD_POINT pg){

lpMATR     gcs_pl = gcond.m[0];
lpD_POINT  vp     = gcond.p;
lpGEO_ARC  arc;
lpGEO_LINE line;
D_POINT    wp1, wp2;

	if(gcond.ctype[cnum]&CND_SCAL)
		return;
	o_hcncrd(gcs_pl, &vp[cnum], &pp[cnum]);
	switch(gcond.type[cnum]){
		case OPOINT:
			dpoint_copy(&pg[cnum], &pp[cnum]);
			pg[cnum].z = 0.;
			break;
		case OARC:
		case OCIRCLE:
	    arc = &gcond.geo[cnum];
			o_hcncrd(gcs_pl, &arc->vc, &pg[cnum]);
			pg[cnum].z = arc->r;
			break;
		case OLINE:
	    line = (lpGEO_LINE)(&gcond.geo[cnum]);
			o_hcncrd(gcs_pl, &line->v1, &wp1);
			o_hcncrd(gcs_pl, &line->v2, &wp2);
		  eq_line(&wp1, &wp2, &pg[cnum].x, &pg[cnum].y, &pg[cnum].z);
			break;
	}
}

BOOL test_cond_to_plane(short cnum){

lpD_PLANE pl  = gcond.pl;
lpD_POINT vp  = gcond.p;
lpGEO_ARC geo;

	if(gcond.ctype[cnum]&CND_SCAL)
		return TRUE;
	geo = &gcond.geo[cnum];
	switch(gcond.type[cnum]){
		case OPOINT:
			return test_points_to_plane(1, &vp[cnum], pl);
		case OLINE:
			if(!test_points_to_plane(2, (lpD_POINT)geo, pl))
				goto met_error;
			break;
		case OARC:
		case OCIRCLE:
			if(!test_points_to_plane(1, &geo->vc, pl))
				goto met_error;
			if(!coll_tst(&geo->n, TRUE, &pl->v, TRUE))
				goto met_error;
	    if(cnum < gcond.maxcond - 1) 
			  gcond.flags[1] = PL_FIXED;
			break;
	}
	return TRUE;
met_error:
	el_geo_err = OBJ_OR_POINT_OUT_PLANE;
	return FALSE;
}

BOOL test_points_to_plane(short nump, lpD_POINT p, lpD_PLANE pl){

short i;
BOOL flag = TRUE;

	for(i = 0; i < nump; i++){
	 if(fabs(dist_p_pl(&p[i], &pl->v, pl->d)) < eps_d)
		 continue;
	 flag = FALSE;
	}
	if(!flag)
		el_geo_err = OBJ_OR_POINT_OUT_PLANE;
	return flag;
}
