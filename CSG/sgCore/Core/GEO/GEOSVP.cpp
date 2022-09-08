#include "../sg.h"

BOOL eq_plane_pl(lpD_POINT p, lpGEO_LINE l, lpD_POINT np, sgFloat *dp){
      

  return eq_plane(p, &l->v1, &l->v2, np, dp);
}

short eq_plane_ll(lpGEO_LINE l1, lpGEO_LINE l2, lpD_POINT np, sgFloat *dp){

lpD_POINT p;
D_POINT   n, wp;
sgFloat    d;
short       i = 0;

	p = &l2->v1;
  normv(TRUE, dpoint_sub(&l1->v2, &l1->v1, &n), &n);
	d = dist_p_l(&l1->v1, &n, p, &wp);
	if(d < dist_p_l(&l1->v1, &n, &l2->v2, &wp)) i = 1;
	if(!eq_plane(p + i, &l1->v1, &l1->v2, np, dp)) return 0;
	i = 1 - i;
	return (dist_p_pl(p + i, np, *dp) < eps_d) ? 1 : -1;
}

lpD_POINT dpoint_copy(lpD_POINT pnew, lpD_POINT pold){

	memcpy(pnew, pold, sizeof(D_POINT));
	return pnew;
}

lpD_POINT dpoint_project(lpD_POINT p, lpD_POINT pp, short axis){

	switch(axis){
		case 1:  //X
			pp->x = p->y;
			pp->y = p->z;
			break;
		case 2:  //Y
			pp->x = p->x;
			pp->y = p->z;
			break;
		case 3:   //Z
			pp->x = p->x;
			pp->y = p->y;
			break;
	}
	pp->z = 0.;
	return pp;
}
lpD_POINT dpoint_reconstruct(lpD_POINT pp, lpD_POINT p, short axis, sgFloat d){

	switch(axis){
		case 1:  //X
			p->x = d;
			p->y = pp->x;
			p->z = pp->y;
			break;
		case 2:  //Y
			p->x = pp->x;
			p->y = d;
			p->z = pp->y;
			break;
		case 3:   //Z
			p->x = pp->x;
			p->y = pp->y;
			p->z = d;
			break;
	}
	return pp;
}
