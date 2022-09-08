#include "../../sg.h"

BOOL np_ort_loop(lpNPW np, short face, short loop, sgFloat *sq)
{
	D_POINT a1,a2,a3,c={0,0,0};
	short 		i=0;
	short     rt,rf;
	short     v,v1,v2;
	BOOL    ret = TRUE;

	rf = np->c[loop].fe;
	rt = rf;
	v = BE(rt,np);    v1 = EE(rt,np);
	rt = SL(rt,np);   v2 = EE(rt,np);
	if (v2 == v) ret = FALSE;
//		put_message(INTERNAL_ERROR,"  ",0);
	dpoint_sub(&np->v[v],&np->v[v1], &a1);
	do {
		i++;
		if (i > np->noe) return FALSE;
		dpoint_sub(&np->v[v],&np->v[v2], &a2);
		dvector_product(&a1, &a2, &a3);
		c.x += a3.x;
		c.y += a3.y;
		c.z += a3.z;
		rt = SL(rt,np);  v2 = EE(rt,np);
		memcpy(&a1,&a2,sizeof(a1));
		if (v2 == v1) ret = FALSE;
		v1 = BE(rt,np);
	} while (rt != rf);
	*sq = np->p[face].v.x*c.x + np->p[face].v.y*c.y + np->p[face].v.z*c.z;
	return ret;
}

BOOL np_include_ll(lpNPW np,short face, short loop1, short loop2)
{
	short r,v;
	short k1,k2,k;

	r = np->c[loop1].fe;
	v = BE(r,np);
	if (fabs(np->p[face].v.x) > fabs(np->p[face].v.y)) {
    if (fabs(np->p[face].v.x) > fabs(np->p[face].v.z))
         { k1 = 1;      k2 = 2; }
    else { k1 = 0;      k2 = 1; }
  } else {
    if (fabs(np->p[face].v.y) > fabs(np->p[face].v.z))
				 { k1 = 0;  k2 = 2; }
    else { k1 = 0;  k2 = 1; }
  }
	k = np_include_vl(np,v,loop2,k1,k2);
  if ((k/2)*2 == k) return FALSE;
  return TRUE;
}

short np_include_vl(lpNPW np, short v, short loop, short k1, short k2)
{
	short r,rf;
	short v1,v2;
	short i;
	sgFloat xx,yy,xp;
	lpDA_POINT w;

	w = (lpDA_POINT)np->v;

	r = np->c[loop].fe;  rf = r;
	v1 = BE(r,np);    v2 = EE(r,np);
	i = 0;
	yy = w[v][k2];     xx = w[v][k1];
	do {
    if (v == v1 || v == v2) return 2; //return(0);
    if (!(w[v1][k2] <  yy && w[v2][k2] <  yy ||
          w[v1][k2] >= yy && w[v2][k2] >= yy))  {
      xp = w[v1][k1] + ((w[v2][k1] - w[v1][k1])*(w[v1][k2] - yy))/
					(w[v1][k2] - w[v2][k2]);
      if (xp > xx) i++;
    }
		r = SL(r,np); v1 = v2;  v2 = EE(r,np);
  } while(r != rf);
  if (i == 0) i=2;
  return (i);
}
