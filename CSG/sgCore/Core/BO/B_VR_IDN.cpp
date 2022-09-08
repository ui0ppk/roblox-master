#include "../sg.h"

BOOL b_vr_idn(lpNPW np,lpD_POINT vn,lpD_POINT vk,lpNPW npr,short edge)
{
/*
#define DIST(v1,f) (fabs(np->v[v1].x*npr->p[f].v.x + \
									 np->v[v1].y*npr->p[f].v.y + np->v[v1].z*npr->p[f].v.z + \
									 npr->p[f].d))

	DA_POINT      *w;
	short           v1,v2,vv,f1,f2,i,r;
	BO_BIT_EDGE   *b, *br;
	NP_STR        str;
	lpNP_STR_LIST list_str;
	short           old_ident,index;

	if (npr == npa) {
		list_str = &lista;
	}	else {
		list_str = &listb;
	}

	old_ident = npr->ident;
	f1 = npr->efc[edge].fp;   if (f1 < 0) f1 = 0;
	f2 = npr->efc[edge].fm;   if (f2 < 0) f2 = 0;
	for (i=1 ; i <= np->noe ; i++) {
		if (np->efc[i].ep == old_ident || np->efc[i].em == old_ident) {
			v1 = np->efr[i].bv;
			if ( (DIST(v1,f1)) > eps_d ) continue;
			v2 = np->efr[i].ev;
			if ( (DIST(v2,f1)) > eps_d ) continue;
			if ( (DIST(v1,f2)) > eps_d ) continue;
			if ( (DIST(v2,f2)) > eps_d ) continue;
			w = (void*)np->v;
			if (w[v1][np_axis] > w[v2][np_axis]) {
				vv = v2;  v2 = v1;  v1 = vv;
			}
			switch (np_axis) {
				case 0:
				if (!(np->v[v1].x-eps_d < vn->x && np->v[v2].x+eps_d > vn->x &&
						 np->v[v1].x-eps_d < vk->x && np->v[v2].x+eps_d > vk->x)) continue;
				break;
				case 1:
				if (!(np->v[v1].y-eps_d < vn->y && np->v[v2].y+eps_d > vn->y &&
						 np->v[v1].y-eps_d < vk->y && np->v[v2].y+eps_d > vk->y)) continue;
				break;
				case 2:
				if (!(np->v[v1].z-eps_d < vn->z && np->v[v2].z+eps_d > vn->z &&
						 np->v[v1].z-eps_d < vk->z && np->v[v2].z+eps_d > vk->z)) continue;
				break;
			}
			if ((r = b_diver_ident(np,i,vn,vk)) == 0)  return FALSE;
*/
	short           r,index;
	BO_BIT_EDGE   *b, *br;
	NP_STR        str;
	lpNP_STR_LIST list_str;

	if (npr == npa) {
		list_str = &lista;
	}	else {
		list_str = &listb;
	}
	if ( (r=b_search_edge(np,vn,vk,npr->ident)) == 0) {
		put_message(INTERNAL_ERROR,GetIDS(IDS_SG027),0);
		return FALSE;
	}

	((BO_USER*)(np->user))[r].bnd = ((BO_USER*)(npr->user))[edge].bnd;
	((BO_USER*)(np->user))[r].ind = ((BO_USER*)(npr->user))[edge].ind;
	b =  (BO_BIT_EDGE*)&np->efr[r].el;
	br = (BO_BIT_EDGE*)&npr->efr[edge].el;
	b->flg = 1;
	if (dpoint_eq(&npr->v[npr->efr[edge].bv],
								&np->v[np->efr[r].bv],eps_d)) {
		b->em = br->em;
		b->ep = br->ep;
	} else {
		b->em = br->ep;
		b->ep = br->em;
	}
	if ( !(np_get_str(list_str,np->ident,&str,&index)) ) return FALSE;
	str.lab = 2;                
	write_elem(&list_str->vdim,index,&str);
	return TRUE;
}
