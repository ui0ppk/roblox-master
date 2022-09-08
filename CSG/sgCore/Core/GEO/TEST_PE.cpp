#include "../sg.h"

BOOL test_in_pe_2d(lpD_POINT p, OBJTYPE type, BOOL flag_pr, void  *geo)
{

sgFloat al, bl, cl;
lpD_POINT    p1, p2;

	switch (type){
		case OLINE:
			p1 = &((lpGEO_LINE) geo)->v1;
			p2 = &((lpGEO_LINE) geo)->v2;
			if(!flag_pr){
				eq_line(p1, p2, &al, &bl, &cl);
				if(fabs(al*p->x + bl*p->y +cl) >= eps_d)
					return FALSE;
			}
			if(fabs(p1->x - p2->x) < fabs(p1->y - p2->y)){
				min_max_2(p1->y, p2->y, &al, &bl);
				if(p->y <= al - eps_d ||
					 p->y >= bl + eps_d)
					return FALSE;
				}
			else {
				min_max_2(p1->x, p2->x, &al, &bl);
				if(p->x <= al - eps_d ||
					 p->x >= bl + eps_d)
					return FALSE;
			}
			break;
		default:
			p1 = &((lpGEO_ARC) geo)->vc;
			al = ((lpGEO_ARC) geo)->r;
			if(!flag_pr)
				if(fabs(al - dpoint_distance_2d(p, p1)) >= eps_d)
					return FALSE;

			if(type == OCIRCLE)
				break;

			return test_in_pa_2d(p, (lpGEO_ARC)geo);
	}
	return TRUE;
}

BOOL test_in_pa_2d(lpD_POINT p, lpGEO_ARC a)
{
sgFloat an, ar, at, pi2;
lpD_POINT vb, ve;

	pi2 = 2.*M_PI;
	an = a->ab;
	ar = a->angle;
	vb = &a->vb;
	ve = &a->ve;

	if(ar < 0.){
		an += ar;
		ar = -ar;
		vb = &a->ve;
		ve = &a->vb;
	}
	while (an < 0.)
		an += pi2;
	while (an > pi2)
		an -= pi2;

	ar += an;
	at = v_angle(p->x - a->vc.x, p->y - a->vc.y);
	if(at < an){
		if(dpoint_distance_2d(p, vb) < eps_d)
			return TRUE;
		else
			at += pi2;
	}

	if(at > ar){
		if(dpoint_distance_2d(p, ve) < eps_d)
			return TRUE;
		else
			return FALSE;
	}
	return TRUE;
}

short test_in_arrey(lpD_POINT p, short *kp,
                  OBJTYPE type, BOOL flag_pr, void  *geo)

{
short i, j;

  for( i = 0, j = 0; i < *kp; i++)
		if(test_in_pe_2d(&p[i], type, flag_pr, geo))
      dpoint_copy(&p[j++], &p[i]);
  return (*kp = j);
}

BOOL test_in_pl_3d(BOOL flag_pr, lpD_POINT p, lpGEO_LINE l, BOOL flag_line)

{
D_POINT w1, w2;

	if(flag_pr && (!flag_line))
		return TRUE;
  if(coll_tst(dpoint_sub(&l->v2, &l->v1, &w1), FALSE,
              dpoint_sub(p, &l->v1, &w2), FALSE) != 1)
		return FALSE;
	if(!flag_line)
		return TRUE;
	if((dpoint_distance(p, &l->v1) - dpoint_distance(&l->v2, &l->v1))
		 >= eps_d)
		return FALSE;
	return TRUE;
}

BOOL test_in_pa_3d(BOOL flag_pr, lpD_POINT p, lpGEO_ARC a, BOOL flag_arc)

{
D_POINT w;
GEO_ARC a2d;
MATR    g_a, a_g;

	if(flag_pr && (!flag_arc))
		return TRUE;
//    
	trans_arc_to_acs(a, flag_arc, &a2d, g_a, a_g);
	o_hcncrd(g_a, p, &w);
	if(fabs(w.z) >= eps_d)
		return FALSE;
	return test_in_pe_2d(&w, (flag_arc) ? OARC : OCIRCLE, flag_pr, &a2d);
}

BOOL test_in_pe_3d(BOOL flag_pr, lpD_POINT p, OBJTYPE type,
                   void  *geo, BOOL flag_el){
//    
	switch(type){
	  case OLINE:
			return test_in_pl_3d(flag_pr, p, (lpGEO_LINE)geo, flag_el);
		case OCIRCLE:
			flag_el = FALSE;
		case OARC:
	    return test_in_pa_3d(flag_pr, p, (lpGEO_ARC)geo,  flag_el);
	  default:
		  return FALSE;
	}
}

lpD_POINT test_cont(lpD_POINT pp, short kp, lpD_POINT vp,
                    OBJTYPE type, void  *geo, BOOL flag_el,
										lpD_POINT p){
//       
//    
	if(vp == NULL)
		dpoint_copy(p, &pp[kp]);
	else
		nearest_to_vp(vp, pp, kp, p);
  if(test_in_pe_3d(TRUE, p, type, geo, flag_el))
		return p;
//   
	if(bool_var[CONTOSNAP]){
	  put_message(EL_CONT_OSNAP, NULL, 1);
		return p;
	}
	el_geo_err = EL_CONT_OSNAP;
	return NULL;
}
