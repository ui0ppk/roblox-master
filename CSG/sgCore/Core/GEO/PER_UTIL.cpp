#include "../sg.h"

short perp_2d_pa(BOOL flag_arc, lpGEO_ARC a, lpD_POINT p, lpD_POINT pp)


{

	if(!normv(FALSE, dpoint_sub(p, &a->vc, pp), pp))
		 return -1;  // неопеделенно - пеп. з ена
	dpoint_add(&a->vc, dpoint_scal(pp, a->r, pp), pp);
	if(flag_arc)
		return (test_in_pa_2d(pp, a)) ? 1 : 0;
	return 1;
}

BOOL perp_3d_pl(lpGEO_LINE l, lpD_POINT b, lpD_POINT p)


{

sgFloat  d;
D_POINT n;

	set_el_geo();
	if(!normv(TRUE, dpoint_sub(&l->v2, &l->v1, &n), &n)){
		el_geo_err = EL_LINE_NULL_LEN;  
		return FALSE;
	}


	d = - n.x*b->x - n.y*b->y - n.z*b->z;


  intersect_3d_pl(&n, d, &l->v1, &n, p);
	return TRUE;
}

BOOL perp_3d_pa(lpGEO_ARC a, BOOL flag_arc, lpD_POINT b, lpD_POINT vp,
								lpD_POINT p, short *kp, short *np)


{

short       i;
D_POINT   w;
D_POINT   c = {0., 0., 0.};
lpGEO_ARC a2d = &el_geo1;
sgFloat    al, bl, cl;

	set_el_geo();
	*np = *kp = 0;

	if(!test_wrd_arc(a, &flag_arc))
		return FALSE;

  trans_arc_to_acs(a, flag_arc, a2d, el_g_e, el_e_g);
  o_hcncrd(el_g_e, b, &w);

	if(hypot(w.x, w.y) < eps_d){


		if(vp == NULL){
	    el_geo_err = NEED_PP_MSG;
			return FALSE;
		}
    if(!near_3d_pa(a, flag_arc, vp, p))
			return FALSE;
		 *kp = 1;
		 return TRUE;
	}
	eq_line(&w, &c, &al, &bl, &cl);
  *kp = intersect_2d_lc(al, bl, cl, 0., 0., a->r, p);

	if(flag_arc)
		if(!test_in_arrey(p, kp, OARC, TRUE, a2d)){
			*kp = 0;
	    return TRUE;
		}

	for(i = 0; i < *kp; i++)
    o_hcncrd(el_e_g, &p[i], &p[i]);

	if(*kp == 2)
		if(dpoint_distance(b, &p[0]) >
		   dpoint_distance(b, &p[1]))
			*np = 1;
	return TRUE;
}

BOOL perp_to_obj(OBJTYPE type, void  *geo, BOOL flag_el,
                lpD_POINT b, lpD_POINT vp,
								lpD_POINT p, short *kp, short *np)


{

  switch (type) {
		case OLINE:
      if(!perp_3d_pl((lpGEO_LINE)geo, b, p))
		     return FALSE;
			*kp = 1;
			*np = 0;
			return TRUE;
		case OARC:
			return perp_3d_pa((lpGEO_ARC)geo, flag_el, b, vp, p, kp, np);
		default:
			return FALSE;
  }
}
