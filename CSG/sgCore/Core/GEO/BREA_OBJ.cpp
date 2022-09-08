#include "../sg.h"

BOOL break_line_p(lpD_POINT p, lpGEO_LINE l, lpGEO_LINE l1, lpGEO_LINE l2);
void break_circle_p(lpD_POINT p, lpGEO_CIRCLE c, lpGEO_ARC a);
BOOL break_arc_p(lpD_POINT p, lpGEO_ARC a, lpGEO_ARC a1, lpGEO_ARC a2);

BOOL break_line_p(lpD_POINT p, lpGEO_LINE l, lpGEO_LINE l1, lpGEO_LINE l2)
//   
{
	if(dpoint_distance(p, &l->v1) < eps_d ||
	   dpoint_distance(p, &l->v2) < eps_d )
		 return FALSE;
	dpoint_copy(&l1->v1, &l->v1);
	dpoint_copy(&l1->v2, p);
	dpoint_copy(&l2->v1, p);
	dpoint_copy(&l2->v2, &l->v2);
	return TRUE;
}

void break_circle_p(lpD_POINT p, lpGEO_CIRCLE c, lpGEO_ARC a)
  
{
D_POINT w;

	memcpy(a, c, sizeof(GEO_CIRCLE));
	get_c_matr(c, el_g_e, el_e_g);
	o_hcncrd(el_g_e, p, &w);
	a->ab = v_angle(w.x, w.y);
	a->angle = 2.*M_PI;
	dpoint_copy(&a->vb, p);
	dpoint_copy(&a->ve, p);
}

BOOL break_arc_p(lpD_POINT p, lpGEO_ARC a, lpGEO_ARC a1, lpGEO_ARC a2)
 
{
D_POINT w;
sgFloat an;

	if(dpoint_distance(p, &a->vb) < eps_d ||
		 dpoint_distance(p, &a->ve) < eps_d )
		 return FALSE;

	memcpy(a1, a, sizeof(GEO_ARC));
	memcpy(a2, a, sizeof(GEO_ARC));
	dpoint_copy(&a1->ve, p);
	dpoint_copy(&a2->vb, p);
	get_c_matr((lpGEO_CIRCLE)a, el_g_e, el_e_g);
	o_hcncrd(el_g_e, p, &w);
	an = a2->ab = v_angle(w.x, w.y);
	if(a->angle > 0.)
		while(an < a->ab)
			an += 2.*M_PI;
	else
		while(an > a->ab)
			an -= 2.*M_PI;
	a1->angle = an - a->ab;
	a2->angle -= a1->angle;
	return TRUE;
}

short break_obj_p(lpD_POINT p, OBJTYPE type, void  *geo,
								 void  *geo1, OBJTYPE *type1,
								 void  *geo2, OBJTYPE *type2)
 
{
short ke = 0;
	*type1 = *type2 = type;
	switch(type){
		case OLINE:
			if(break_line_p(p, (lpGEO_LINE)geo, (lpGEO_LINE)geo1, (lpGEO_LINE)geo2))
				ke = 2;
			break;
		case OARC:
			if(break_arc_p(p, (lpGEO_ARC)geo, (lpGEO_ARC)geo1, (lpGEO_ARC)geo2))
				ke = 2;
			break;
		case OCIRCLE:
			break_circle_p(p, (lpGEO_CIRCLE)geo, (lpGEO_ARC)geo1);
	    *type1 = *type2 = OARC;
			ke = 1;
			break;
	}
	return ke;
}
