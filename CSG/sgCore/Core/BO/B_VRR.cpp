#include "../sg.h"



BOOL b_ort(lpD_POINT * vn,lpD_POINT * vk,
					 lpD_POINT n1, lpD_POINT n2)
{
	lpD_POINT v;
	sgFloat sp;
	D_POINT a3, a4;

	dvector_product(n1, n2, &a3); 
	dpoint_sub(*vk, *vn, &a4);      
	if ( !dnormal_vector (&a3) ) goto err;
	if ( !dnormal_vector (&a4) ) goto err;
	sp = dskalar_product(&a3, &a4);
	if ( sg(sp,eps_d) < 0 ) {
		v = *vn;
		*vn = *vk;
		*vk = v;
	}
	return TRUE;
err:
	return FALSE;
}

BOOL b_ort_on(lpNPW np, short r, lpD_POINT * vn, lpD_POINT * vk)
{
	short       v1,v2;
	sgFloat    sp;
	D_POINT   a3, a4;
	lpD_POINT v;

	v1 = BE(r,np);
	v2 = EE(r,np);
	dpoint_sub(*vk, *vn, &a3);
	dpoint_sub(&(np->v[v2]), &(np->v[v1]), &a4);
	if ( !dnormal_vector (&a3) ) goto err;
	if ( !dnormal_vector (&a4) ) goto err;
	sp = dskalar_product(&a3, &a4);
	if ((sg(sp,eps_d)) < 0) {
		v = *vn;
		*vn = *vk;
		*vk = v;
	}
	return TRUE;
err:
	return FALSE;
}

short b_vrr(lpNPW np,lpD_POINT vn,lpD_POINT vk,lpNP_VERTEX_LIST ver,
					short ukt, short face)
{
	short        v1, v2, v;
	short        edge, cnt, loop;
	DA_POINT   wn,wk;
	lpDA_POINT w;

	wn[0] = np->v[ver->v[ukt-1].v].x;
	wn[1] = np->v[ver->v[ukt-1].v].y;
	wn[2] = np->v[ver->v[ukt-1].v].z;
	wk[0] = np->v[ver->v[ukt].v].x;
	wk[1] = np->v[ver->v[ukt].v].y;
	wk[2] = np->v[ver->v[ukt].v].z;

//	memcpy(wn,&(np->v[ver->v[ukt-1].v]),sizeof(D_POINT));
//	memcpy(wk,&(np->v[ver->v[ukt].v]),sizeof(D_POINT));
	if ( (v1 = np_new_vertex(np, vn)) == 0)    return 0;
	if ( (v2 = np_new_vertex(np, vk)) == 0)    return 0;
	if ( (edge = np_new_edge(np,v1,v2)) <= 0) return edge;
	np->efc[edge].fm = np->efc[edge].fp = face;
	np->efr[edge].el = ST_TD_CONSTR;
	w = (lpDA_POINT)np->v;
	if (w[v1][np_axis] > w[v2][np_axis]) {
		v = v1;   v1 = v2;   v2 = v;
	}
	if ((fabs(w[v1][np_axis] - wn[np_axis])) < eps_d &&
			 ver->v[ukt-1].m == 1) {   
		if (b_dive(np,ver->v[ukt-1].r,v1) == 0) return (0);
	}
	w = (lpDA_POINT)np->v;
	if ((fabs(w[v2][np_axis] - wk[np_axis])) < eps_d &&
			 ver->v[ukt].m == 1) {    
		if (b_dive(np,ver->v[ukt].r,v2) == 0) return (0);
	}
	cnt = np->f[face].fc;          
	while (cnt > 0) {
		loop = cnt;
		cnt = np->c[cnt].nc;
	}
	if (cnt != 0) {
		cnt = abs(cnt);
		np->efc[edge].ep = np->c[cnt].fe;
		np->c[cnt].fe = edge;
	} else {
		cnt = np_new_loop(np,edge);
		np->c[loop].nc = -cnt;
	}
	return (edge);
}

short b_vran(lpNPW np, lpNP_VERTEX_LIST sp, short ukt)
{
  short r1,r2;
  short v1,v2,vn,vk;

  r1 = sp->v[ukt].r;
  r2 = sp->v[ukt-1].r;
  v1 = sp->v[ukt].v;
  v2 = sp->v[ukt-1].v;
  vn = np->efr[(int)abs(r1)].bv;
  vk = np->efr[(int)abs(r1)].ev;
	if ((v1 == vn) & (v2 == vk) || (v1 == vk) & (v2 == vn)) return (r1);
	return (r2);
}
