#include "../sg.h"

lpD_POINT near_3d_pl(lpGEO_LINE l, lpD_POINT vp, lpD_POINT p)


{
sgFloat   a, b, c;
BOOL     be_flag;
lpGEO_LINE lp = (lpGEO_LINE)&el_geo1;
D_POINT  pp;

	if(!test_wrd_line(l))
		return NULL;
	gcs_to_vcs (&l->v1, &lp->v1);
	gcs_to_vcs (&l->v2, &lp->v2);
	if(!eq_line(&lp->v1, &lp->v2, &a, &b, &c)){

		min_max_2(lp->v1.z, lp->v2.z, &a, &b);
		if(vp->z > a && vp->z < b){
			lp->v1.z = vp->z;
			return vcs_to_gcs (&lp->v1, p);
		}
		be_flag = (fabs(lp->v1.z - vp->z) < fabs(lp->v2.z - vp->z));
	}
	else{

		p_int_lines(a, b, c, -b, a, b*vp->x - a*vp->y, &pp);
		if(test_in_pe_2d(&pp, OLINE, TRUE, lp)){

			return dpoint_parametr(&l->v1, &l->v2, dpoint_distance_2d(&lp->v1, &pp)/
														 dpoint_distance_2d(&lp->v1, &lp->v2), p);
		}
		be_flag = (dpoint_distance_2d(&pp, &lp->v1) < 
				       dpoint_distance_2d(&pp, &lp->v2));
	}
	if(be_flag)
		return dpoint_copy(p, &l->v1);
	else
		return dpoint_copy(p, &l->v2);
}

lpD_POINT near_3d_pa(lpGEO_ARC a, BOOL flag_arc, lpD_POINT vp, lpD_POINT p)


{
D_POINT va;   
D_POINT vg;
lpGEO_ARC a2d = &el_geo1;


	if(!test_wrd_arc(a, &flag_arc))
		return NULL;

	tr_vp_to_acs(a, flag_arc, vp, &vg, &va, a2d, el_g_e, el_e_g);

	switch(perp_2d_pa(flag_arc, a2d, &va, &vg)){
		case -1:  
      o_hcncrd(el_e_g, &a2d->vb, p);
			break;
		case 1:
      o_hcncrd(el_e_g, &vg, p);
			break;
		case 0:
      nearest_to_vp(vp, &a->vb, 2, p);
			break;
	}
	return p;
}
