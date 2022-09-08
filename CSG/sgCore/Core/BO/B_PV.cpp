#include "../sg.h"


short  b_pv(lpNPW np1, short r, lpD_PLANE pl)
{
	short    v1,v2;
	short    gr,p;
	D_POINT x2,x3;
	sgFloat  dl,f;

	v1 = BE(r,np1);  v2 = EE(r,np1);
	if (r > 0)     gr = np1->efc[ r].fp;
	else           gr = np1->efc[-r].fm;
	if (gr < 0) gr = 0;
	dpoint_sub(&(np1->v[v2]), &(np1->v[v1]), &x2);
	dnormal_vector (&x2);
	dl = 1;  //eps_d*1.e7;
	dvector_product(&(np1->p[gr].v),&x2, &x3);
	x3.x *= dl; 	x3.y *= dl; 	x3.z *= dl;
	x3.x += np1->v[v1].x;
	x3.y += np1->v[v1].y;
	x3.z += np1->v[v1].z;
	f = (pl->v.x*x3.x + pl->v.y*x3.y + pl->v.z*x3.z + pl->d);
  p = sg(f,eps_d);
  return p;
}

short b_face_angle(lpNPW np1, short r, lpNPW np2, short f1, short f2, short pe)
{
	short p1,p2,p,f;
	short mvy[3][3]={
    	{ 1, 3, 0},
    	{ 2, 2, 0},
    	{ 0, 0, 0}
    	};
	short mvo[3][3]={
    	{ 1, 1, 1},
    	{ 1, 2, 2},
    	{ 1, 3, 0}
    	};

  if (r > 0)   f = np1->efc[ r].fp;
  else         f = np1->efc[-r].fm;
  if (f < 0)   f = 0;
	p1 = b_pv(np1, r, &np2->p[f1]);
	p1 = p1 + 1;
  p2 = b_pv(np1, r, &np2->p[f2]);
	p2 = p2 + 1;
  if (pe < 0)   p = mvy[p1][p2];
  else          p = mvo[p1][p2];
  if (p == 2) {
		if (fabs(np1->p[f].v.x - np2->p[f1].v.x) < eps_n &&
				fabs(np1->p[f].v.y - np2->p[f1].v.y) < eps_n &&
				fabs(np1->p[f].v.z - np2->p[f1].v.z) < eps_n )  return (3);
		else                       			 	                  return (2);
	}
	if (p == 3) {
		if (fabs(np1->p[f].v.x - np2->p[f2].v.x) < eps_n &&
				fabs(np1->p[f].v.y - np2->p[f2].v.y) < eps_n &&
				fabs(np1->p[f].v.z - np2->p[f2].v.z) < eps_n )  return (3);
		else                              			            return (2);
	}
	return p;
}
