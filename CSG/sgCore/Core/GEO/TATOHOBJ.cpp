#include "../sg.h"

static BOOL tang_3d_pa(lpGEO_ARC a, BOOL flag_arc,
                       lpD_POINT b, lpD_POINT p, short *kp);

lpD_POINT tangent_to_hobj(hOBJ hobj, OBJTYPE type,
                          lpD_POINT vp, lpD_POINT p)

{
GEO_OBJ   geo;

  if(!true_geo_info(hobj, &type, &geo))
		return NULL;
  return tangent_to_geo_obj(type, &geo, vp, p);
}

lpD_POINT tangent_to_geo_obj(OBJTYPE type, void * geo,
                          lpD_POINT vp, lpD_POINT p)

{
D_POINT   pp[2];
short       kp;
BOOL      flag_arc;

  set_flag_el(&type, &flag_arc);

  if(!tang_3d_pa((lpGEO_ARC)geo, FALSE, &curr_point, pp, &kp))
		return NULL;

  return test_cont(pp, kp, vp, type, geo, flag_arc, p);
}

static BOOL tang_3d_pa(lpGEO_ARC a, BOOL flag_arc,
                       lpD_POINT b, lpD_POINT p, short *kp)


{

lpGEO_ARC  a2d = &el_geo1;
sgFloat     xc, yc;
short        i;

	set_el_geo();
	*kp = 0;

	if(!test_wrd_arc(a, &flag_arc))
		return FALSE;

	trans_arc_to_acs(a, flag_arc, a2d, el_g_e, el_e_g);

	o_hcncrd(el_g_e, b, p);

	if(fabs(p->z) >= eps_d){
		el_geo_err = EL_NO_ARC_PLANE;
		return FALSE;
	}
	xc = p->x/2.;
	yc = p->y/2.;
	if(!(*kp = intersect_2d_cc(0., 0., a->r, xc, yc, hypot(xc, yc), p))){
		el_geo_err = EL_NO_TANG;
		return FALSE;
	}

	if(flag_arc)
		if(!test_in_arrey(p, kp, OARC, TRUE, a2d)){
			el_geo_err = EL_NO_TANG;
			return FALSE;
		}

	for(i = 0; i < *kp; i++)
    o_hcncrd(el_e_g, &p[i], &p[i]);
  return TRUE;
}
