#include "../sg.h"

//static BOOL  b_def_angle(lpNPW np, short v3, short r1, short r2, short face);
//short  b_def_loop(lpNPW np, short face, short r, short *rl);

BOOL b_as(lpNPW np)
{
	short face;
	short r,rt;
	short cl,cnt,cnt1,cnt2;
	short l,new_c;
	short rl1=0,rl2=0;

	for (face=1 ; face <= np->nof ; face++) {
		cnt = np->f[face].fc;
		do {
			cl = cnt;
			cnt = np->c[cnt].nc;
		} while (cnt > 0);
		if (cnt == 0)  continue;      
		cnt = -cnt;
		np->c[cl].nc = 0;
		r = np->c[cnt].fe;
		for(;;) {
			rt = r;
			if (rt == 0) break;
			r = np->efc[r].ep;
			np->c[cnt].fe = r;
			np->efc[rt].ep  = -rt;     
			np->efc[rt].em  =  rt;
			l = 0;
			cnt1 = np_def_loop(np, face,         rt,  &rl1);   
			cnt2 = np_def_loop(np, face,(short)(-rt), &rl2);   
			if (cnt1 != 0) {
				l = l + 1;
				np_insert_edge(np,rt,rl1);
			}
			if (cnt2 != 0) {
				l = l + 1;
				np_insert_edge(np,(short)(-rt),rl2);
			}
			if (l == 1) continue;
			if (l == 0) {                       
				if ((new_c = np_new_loop(np,rt)) == 0) return FALSE;
				np->c[new_c].nc = np->f[face].fc;
				np->f[face].fc = new_c;
				continue;
			}
			np_an_loops(np,face,cnt1,cnt2,rt);
		}
	}
	return TRUE;
}

/*
short b_def_loop(lpNPW np, short face, short r, short *rl)
{
	short vn,vk,vt;
	short rt,rn,rf;
	short cnt;

	vn = BE(r,np);
  vk = EE(r,np);
  cnt = np->f[face].fc;
  do {
    rt = rf = np->c[cnt].fe;
		vt = EE(rt,np);
    do {
      if (vt == vn) {
        rn = SL(rt,np);
				if (rn == -rt) goto norm;
				if (b_def_angle(np,vk,rt,rn,face)) goto norm;
			}
			rt = SL(rt,np);
			vt = EE(rt,np);
		} while (rt != rf);
		cnt = np->c[cnt].nc;
  } while (cnt != 0);
	return 0;
norm:  *rl = rt;
	return cnt;
}
*/

/*
static BOOL  b_def_angle(lpNPW np, short v3, short r1, short r2, short face)
{
	short    v,v1,v2;
	D_POINT a1, a2, a3, c,d1,d2;
	sgFloat cr, cr1, cr2;

	v1 = BE(r1,np);  v2 = EE(r2,np);  v = EE(r1,np);
	dpoint_sub(&(np->v[v]),&(np->v[v1]), &a1);
	dpoint_sub(&(np->v[v]),&(np->v[v2]), &a2);
	dpoint_sub(&(np->v[v]),&(np->v[v3]), &a3);
	dvector_product(&a1, &a2, &c);
	cr = dskalar_product(&c, &(np->p[face].v));
	if ((fabs(cr)) <= eps_d) { 
		dvector_product(&a1, &a3, &c);
		cr = dskalar_product(&c, &(np->p[face].v));
		if (cr < 0) return TRUE;
		return FALSE;
	}
	dvector_product(&a1, &a3, &d1);
	dvector_product(&a3, &a2, &d2);
	cr1 = dskalar_product(&d1, &c);
	cr2 = dskalar_product(&d2, &c);
	if (cr < eps_d) {  
		if(cr1 > 0  &&  cr2 > 0)  return TRUE;
		return FALSE;
	}
	if (cr1 > 0  &&  cr2 >0)    return FALSE;
	return TRUE;
}
*/
