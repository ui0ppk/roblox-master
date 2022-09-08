#include "../../sg.h"

short	flg_exist = 1;    //   

BOOL np_face(lpNPW np, sgFloat r, short n, sgFloat h,short *num_np)
{
	short    i;
	sgFloat alfa;

	alfa = M_PI*2/n;
	if( n > np->maxnov )
		if ( !(o_expand_np( np, (short)(n-np->maxnov), 0, 0, 0)) ) return FALSE;
	if( n > np->maxnoe )
		if ( !(o_expand_np( np, 0, (short)(n-np->maxnoe), 0, 0)) ) return FALSE;

	np_init((lpNP)np);
	np->ident = (*num_np)++;

	for (i = 1 ; i <= n ; i++) {
		np->v[i].x = r*cos(alfa*i);
		np->v[i].y = r*sin(alfa*i);
		np->v[i].z = h;
	}
	np->nov = n;

	for (i = 1 ; i <= n ; i++) {
		np->efr[i].bv = i;
		np->efr[i].ev = i+1;
		np->efc[i].ep = i+1;
		np->efc[i].fp = 1;
		np->efc[i].em = 0;
		np->efc[i].fm = 0;
		np->efr[i].el = ST_TD_CONSTR;
	}
	np->efr[n].ev = 1;
	np->efc[n].ep = 1;
	np->noe = n;

	np->f[1].fc = 1;
	np->c[1].fe = 1;
	np->c[1].nc = 0;
	np->nof = np->noc = 1;

	np_cplane(np);
	return TRUE;
}

short np_edge_exist(lpNPW np, short v1, short v2)
{

	short i;
	lpEDGE_FRAME e;

	e = np->efr;
	for (i=1; i <= np->noe; i++) {
		if (e[i].bv == v1 && e[i].ev == v2) return i;
		if (e[i].ev == v1 && e[i].bv == v2) return -i;
	}
	return 0;
}

short np_new_edge(lpNPW np, short v1, short v2)
{

	short edge;

	if ( flg_exist ) {
		edge = np_edge_exist(np,v1,v2);
		if (edge != 0) return -(abs(edge));               //   
	}

	if (np->noe == np->maxnoe)
		if ( !(o_expand_np(np, 0, MAXNOE/4, 0, 0)) ) return 0;

	edge = ++np->noe;
	np->efr[edge].bv = v1;
	np->efr[edge].ev = v2;
	np->efr[edge].el = 0;
	if (np->type == TNPF) return edge;                        // 
  np->efc[edge].ep = 0;
  np->efc[edge].em = 0;
  np->efc[edge].fm = 0;
  np->efc[edge].fp = 0;
	if (np->gru != NULL)                           //  
		np->gru[edge] = 0;
  return edge;
}

short np_new_face(lpNPW np, short cnt)
{
	short i,nf;
	lpD_PLANE p;

	for (i=1; i <= np->nof ; i++) {      //    
		if (np->f[i].fc != 0) continue;
		nf = i;
		goto met;
	}
	if (np->nof == np->maxnof)
    if ( !(o_expand_np(np, 0, 0, MAXNOF/4, 0)) ) return 0;

	nf = ++np->nof;
met:
	np->f[nf].fc = cnt;
	if ( &(np->p[nf]) != NULL) {
		p = &(np->p[nf]);
		p->v.x = 0.;
		p->v.y = 0.;
		p->v.z = 0.;
		p->d   = 0.;
	}
  np->f[nf].fl = 0;
  return nf;
}

short np_new_loop(lpNPW np, short first_edge)
{
	short i;
  lpCYCLE c;

	c = np->c;
	for (i=1; i <= np->noc ; i++) {     //    
		if (c[i].fe != 0) continue;
    c[i].fe = first_edge;
		c[i].nc = 0;
		return (i);
	}
	if (np->noc == np->maxnoc)
    if ( !(o_expand_np(np, 0, 0, 0, MAXNOC/4)) ) return 0;

	np->noc++;
	np->c[np->noc].fe = first_edge;
	np->c[np->noc].nc = 0;
	return (np->noc);
}

short np_new_vertex(lpNPW np, lpD_POINT new_v)
{
	short i;
	lpD_POINT v;
  D_POINT   new1;

	new1 = *new_v;
	v = np->v;
	for (i=1; i <= np->nov; i++) {
		if (!(dpoint_eq(&new1, &v[i],eps_d))) continue;
		return (i);
	}
	if (np->nov == np->nov_tmp)
    if ( !(o_expand_np(np, MAXNOV/4, 0, 0, 0)) ) return 0;

	i = ++np->nov;
	np->v[i] = new1;

  	if (np->nov > 0) {
		np->vertInfo[i] = np->vertInfo[1];
    }

	return (i);
}
