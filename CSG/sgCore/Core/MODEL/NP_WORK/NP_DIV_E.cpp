#include "../../sg.h"

short np_divide_edge(lpNPW np, short edge, short v)
{
	short  iedge, jedge, ledge;

	iedge = abs(edge);
	ledge = np_new_edge(np,v,np->efr[iedge].ev);
	if (ledge <= 0)  return abs(ledge);
	if ( np->efc[iedge].fp >= 0 ) np->efc[ledge].fp = np->efc[iedge].fp;
	else                          np->efc[ledge].fp = -ledge;
	if ( np->efc[iedge].fm >= 0 ) np->efc[ledge].fm = np->efc[iedge].fm;
	else                          np->efc[ledge].fm = -ledge;
	np->efr[ledge].el = np->efr[iedge].el;
	np->gru[ledge]    = np->gru[iedge];
	np->efr[iedge].ev = v;
	if (np->efc[iedge].fp > 0) {
		np->efc[ledge].ep = np->efc[iedge].ep;
		np->efc[iedge].ep = ledge;
	} else
		np->efc[ledge].ep = np->efc[iedge].ep;
	if (np->efc[iedge].fm > 0) {
		np->efc[ledge].em = -iedge;
		jedge = -iedge;   edge = SL(jedge,np);
		while (edge != -iedge) {
			jedge = edge;  edge = SL(jedge,np);
		}
		if (jedge > 0)    np->efc[ jedge].ep = -ledge;
		else              np->efc[-jedge].em = -ledge;
	} else
		np->efc[ledge].em = np->efc[iedge].em;
	return (ledge);
}

void np_insert_edge(lpNPW np, short edge, short edgel)
{
	if (edgel > 0) {
		if (edge > 0) {
			np->efc[edge].em  = np->efc[edgel].ep;
			np->efc[edgel].ep = edge;
		} else {
			np->efc[-edge].ep = np->efc[edgel].ep;
			np->efc[edgel].ep = edge;
		}
	}	else {
		if (edge > 0) {
			 np->efc[edge].em = np->efc[-edgel].em;
			 np->efc[-edgel].em = edge;
		} else {
			 np->efc[-edge].ep = np->efc[-edgel].em;
			 np->efc[-edgel].em = edge;
		}
	}
}

short np_def_loop(lpNPW np, short face, short r, short *rl)
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
				if (np_def_angle(np,vk,rt,rn,face)) goto norm;
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

BOOL np_def_angle(lpNPW np, short v3, short r1, short r2, short face)
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
	if ((fabs(cr)) <= eps_d) { //  r1  r2    
		dvector_product(&a1, &a3, &c);
		cr = dskalar_product(&c, &(np->p[face].v));
		if (cr < 0) return TRUE;
		return FALSE;
	}
	dvector_product(&a1, &a3, &d1);
	dvector_product(&a3, &a2, &d2);
	cr1 = dskalar_product(&d1, &c);
	cr2 = dskalar_product(&d2, &c);
	if (cr < eps_d) {  							//  
		if(cr1 > 0  &&  cr2 > 0)  return TRUE;
		return FALSE;
	}
	if (cr1 > 0  &&  cr2 >0)
		return FALSE;
	return TRUE;
}

BOOL np_an_loops(lpNPW np, short face, short cnt1, short cnt2, short edge)
{
	short new_c;

	if (cnt1 == cnt2) {                   //    
		if ((new_c = np_new_loop(np,(short)(-edge))) == 0)  return FALSE;
		np->c[cnt1].fe = edge;
		np->c[new_c].nc = np->f[face].fc;
		np->f[face].fc = new_c;
	} else {                              //   ,
		new_c = np->f[face].fc;             //   cnt1
		if (new_c == cnt1) {
			np->f[face].fc = np->c[new_c].nc;
    	np->c[new_c].fe = 0;
    	np->c[new_c].nc = 0;
    } else {
			do {                              //    cnt1
        if (np->c[new_c].nc == cnt1) goto norm;
				new_c = np->c[new_c].nc;
			} while (new_c != 0);
      np_handler_err(NP_AN_CNT);
      return FALSE;
norm: np->c[new_c].nc = np->c[cnt1].nc;
    	np->c[cnt1].fe = 0;
    	np->c[cnt1].nc = 0;
    }
  }
	return TRUE;
}

short np_search_edge(lpNPW npw, lpD_POINT bv, lpD_POINT ev, short ident)
{
  short edge;
  short bv_npw, ev_npw;

  for (edge=1 ; edge <= npw->noe ; edge++) {
		if (npw->efc[edge].fp > 0 && npw->efc[edge].fm > 0) continue;
    if (npw->efc[edge].ep == ident || npw->efc[edge].em == ident) {
      bv_npw = npw->efr[edge].bv;
      ev_npw = npw->efr[edge].ev;
      if ( dpoint_eq(&npw->v[bv_npw],bv,eps_d) &&
           dpoint_eq(&npw->v[ev_npw],ev,eps_d) ) return  edge;
      if ( dpoint_eq(&npw->v[ev_npw],bv,eps_d) &&
					 dpoint_eq(&npw->v[bv_npw],ev,eps_d) ) return -edge;
    }
  }
  np_handler_err(NP_NO_EDGE);
	return 0;
}
