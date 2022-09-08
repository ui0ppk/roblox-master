#include "../sg.h"

static BOOL qu_3d_pa(lpGEO_ARC a, BOOL flag_arc, lpD_POINT p);

lpD_POINT quadrant_of_hobj(hOBJ hobj, OBJTYPE type,
													 lpD_POINT vp, lpD_POINT p)

{
GEO_OBJ   geo;

  if(!true_geo_info(hobj, &type, &geo))
		return NULL;

  return quadrant_of_geo_obj(type, &geo, vp, p);
}

lpD_POINT quadrant_of_geo_obj(OBJTYPE type, void * geo,
													 lpD_POINT vp, lpD_POINT p)

{
D_POINT   pp[4];
BOOL      flag_arc;

  set_flag_el(&type, &flag_arc);

  if(!qu_3d_pa((lpGEO_ARC)geo, FALSE, pp))
		return NULL;

  return test_cont(pp, 4, vp, type, geo, flag_arc, p);
}

static BOOL qu_3d_pa(lpGEO_ARC a, BOOL flag_arc, lpD_POINT p)


{

lpGEO_ARC  a2d = &el_geo1;
short        i;

	set_el_geo();

	if(!test_wrd_arc(a, &flag_arc))
		return FALSE;

	trans_arc_to_acs(a, flag_arc, a2d, el_g_e, el_e_g);

	for(i = 0; i < 4; i++)
		p[i].x = p[i].y = p[i].z = 0.;
	p[0].x = p[1].y = a->r;
	p[2].x = p[3].y = -(a->r);

	for(i = 0; i < 4; i++)
	  o_hcncrd(el_e_g, &p[i], &p[i]);
	return TRUE;
}
