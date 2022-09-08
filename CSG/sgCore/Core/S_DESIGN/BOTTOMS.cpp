#include "../sg.h"

static void form_countor( lpNPW np, short contour, short nov_old );

BOOL put_top1_bottom(lpNPW np, lpMESHDD g )
{
	short       v, nov_old;
	MNODE			node;

//      ->  np
	if( np->nov + g->m > np->maxnov ) if( !my_expand_face( np, g->m )) return FALSE;

	nov_old = v = np->nov;
	int num_elem = 2*g->n-1;
	while( read_elem( &g->vdim, num_elem, &node ) )
	{
		np->v[ ++v ] = node.p;
		num_elem += g->n;
	}
	np->nov = v;

	form_countor( np, g->m, nov_old );
	np->f[1].fl |= ST_FACE_PLANE;

	return TRUE;
}

BOOL put_top_bottom(lpNPW np, lpMESHDD g )
{
	int       v, nov_old, num_elem;
	lpMNODE		node;

//      ->  np
	if( np->nov + g->n > np->maxnov )  if( !my_expand_face( np, g->n )) return FALSE;

//  
	num_elem = g->m*g->n - g->n;
	nov_old = v = np->nov;

	if (!begin_rw(&g->vdim, ++num_elem)) return FALSE;
	for (; num_elem < g->vdim.num_elem; num_elem++) {
		if ( (node = (lpMNODE)get_next_elem(&g->vdim)) == NULL) {
			end_rw(&g->vdim);
			return FALSE;
		}
		np->v[ ++v ] = node->p;
	}
	end_rw(&g->vdim);
	np->nov = v;

	form_countor( np, g->n, nov_old );
	np->f[1].fl |= ST_FACE_PLANE;

	return TRUE;
}

BOOL put_bottom_bottom(lpNPW np, hOBJ cross )
{
	short         i;
	short         nov_old;
	VDIM        vdim_cross;
	lpMNODE     node;

	np->nov = np->noe = np->noc = np->nof = 0;
	while( cross != NULL )      //     
	{
//  
		if( !create_cross( cross, &vdim_cross ) ) return FALSE;
		nov_old = np->nov;
		if (!begin_rw(&vdim_cross, 1)) goto err;
//   -  
		if( np->nov + vdim_cross.num_elem > np->maxnov )
			if( !my_expand_face( np, (short)vdim_cross.num_elem )) return FALSE;
		for (i = 1; i < vdim_cross.num_elem; i++) {
			if ( (node = (lpMNODE)get_next_elem(&vdim_cross)) == NULL) {
				end_rw(&vdim_cross);
				goto err;
			}
			np->v[++np->nov] = node->p;
		}
		end_rw(&vdim_cross);
/*		if ( vdim_cross.num_elem > MAXINT ) {
			cinema_handler_err(CIN_OVERFLOW_MDD);
			goto err;
		}
*/		form_countor( np, (short)vdim_cross.num_elem, nov_old );

		get_next_item_z( SEL_LIST, cross, &cross ); // c  
		free_vdim(&vdim_cross);
	}
	np->f[1].fl |= ST_FACE_PLANE;

	return TRUE;
err:
	free_vdim(&vdim_cross);
	return FALSE;
}

BOOL put_bottom_bottom_skin(lpNPW np, lpMESHDD g )
{
	short     v, num_elem;
	MNODE		node;

//  
	v = 0;
	np->noe = np->noc = np->nof = 0;

	np->nov = g->n;
	for (num_elem=0; num_elem < np->nov; num_elem++) {
		read_elem( &g->vdim, num_elem, &node );
		np->v[ ++v ] = node.p;
	}
//  np->nov = v;
	form_countor( np, g->n, 0 );
	np->f[1].fl |= ST_FACE_PLANE;

	return TRUE;
}

BOOL put_bottom_rev( lpNPW np, lpMESHDD g, BOOL clouser, BOOL i )
{
	short       num_elem;
	MNODE     node;

	np->nov = np->noe = np->noc = np->nof = 0;

//  
	if( g->m  > np->maxnov )  if( !my_expand_face( np, g->m )) return FALSE;

	if (!i)	{ num_elem = ( clouser ) ? (g->n)     : (0); }
	else    { num_elem = ( clouser ) ? (2*g->n-1) : (g->n-1); }
	while( read_elem( &g->vdim, num_elem, &node ) )
	{
		np->v[ ++np->nov ] = node.p;
		num_elem += g->n;
	}

	num_elem = ( clouser ) ? ( g->m) : (g->m+1);
	form_countor( np, num_elem, 0 );
	np->f[1].fl |= ST_FACE_PLANE;

	return TRUE;
}

  
static void form_countor( lpNPW np, short contour, short nov_old )
{
	short i, edge, edge_old, v;

	edge_old = edge = np->noe;
	v = nov_old;
	for (i = 0 ; i < contour - 2; i++)
	{
		np->efr[++edge].bv = ++v;
		np->efr[  edge].ev = v+1;
		np->efr[  edge].el = ST_TD_CONSTR;
		np->efc[  edge].ep = edge + 1;
		np->efc[  edge].fp = 1;
		np->efc[  edge].em = 0;
		np->efc[  edge].fm = 0;
	}
	np->efr[++edge].bv = ++v;
	np->efr[  edge].ev = nov_old+1;
	np->efr[  edge].el = ST_TD_CONSTR;
	np->efc[  edge].ep = edge_old+1;
	np->efc[  edge].fp = 1;
	np->efc[  edge].em = 0;
	np->efc[  edge].fm = 0;
	np->noe = edge;

	if( np->nof == 0) np->f[1].fc = 1;
	else              np->c[np->noc].nc = np->noc+1;
	np->noc++;
	np->c[np->noc].fe = edge_old+1;
	np->c[np->noc].nc = 0;
	np->nof = 1;
}
 
BOOL my_expand_face( lpNPW np, short m )
{
	short j_nov, j_noe;

	j_nov = np->nov + m - np->maxnov;
	j_noe = 0;
	if( np->noe + m > np->maxnoe ) j_noe = np->noe + m - np->maxnoe;
	expand_user_data = NULL;
	np->nov_tmp = np->maxnov;
	if( !o_expand_np(np, j_nov, j_noe, 0, 0 ) )	return FALSE;
	return TRUE;
}
