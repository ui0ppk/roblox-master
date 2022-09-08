#include "../sg.h"

static BOOL Bild_Coons_Surf( lpVDIM u1, lpVDIM v1, lpVDIM u2, lpVDIM v2,
											lpMESHDD g, short n_u, short n_v, short step_u,	short step_v);
/**********************************************************
* Bilding Coons surface with dimension n_u x n_v and
* boundaries u1 v2 u2 v2
*/
BOOL Coons_Surface( lpVDIM u1, lpVDIM v1, lpVDIM u2, lpVDIM v2,
										short n_u, short n_v, short constr_u,	short constr_v,
										hOBJ *hrez){
	short     		c_num_np;
	MESHDD      mdd;
	BOOL				cod;

	c_num_np	= -32767;
	*hrez			= NULL;
	if ( !np_init_list(&c_list_str) ) return FALSE;

	init_vdim(&mdd.vdim,sizeof(MNODE));
	if( !Bild_Coons_Surf( u1, v1, u2, v2, &mdd,
												n_u, n_v, constr_u, constr_v  ) ) goto err2;
	if (!(np_mesh3p(&c_list_str,&c_num_np,&mdd,0.))) goto err2;


	free_vdim(&mdd.vdim);
	cod = cinema_end(&c_list_str,hrez);
	return cod;
err2:
	free_vdim(&mdd.vdim);
//nb err:
	np_end_of_put(&c_list_str,NP_CANCEL,0,NULL);
	return FALSE;
}


static BOOL Bild_Coons_Surf( lpVDIM u1, lpVDIM v1, lpVDIM u2, lpVDIM v2,
											lpMESHDD g, short n_u, short n_v, short step_u,	short step_v){
	MNODE       node;
	D_POINT     node_u1_0, node_u2_0, node_v2_0, node_v2_n;
	D_POINT   	node_u1_i, node_u2_i, node_v1_j, node_v2_j;
	sgFloat      a0u, a0v, a1u, a1v, u, v, dv, du;
	int		      i_surf, j_surf, mask;

	g->n = n_u;
	g->m = n_v;
	dv =  1./( g->m - 1);
	du =  1./( g->n - 1);

	read_elem( u1, 0,      &node_u1_0 );
	read_elem( u2, 0,      &node_u2_0 );
	read_elem( v2, 0,      &node_v2_0 );
	read_elem( v2, g->m-1, &node_v2_n );

	for(  j_surf=0; j_surf<g->m; j_surf++ )
	{
		v = dv*j_surf;
		a0v = 1- 3*v*v + 2*v*v*v;
		a1v = 1 - a0v;

		read_elem( v2, j_surf, &node_v2_j );
		read_elem( v1, j_surf, &node_v1_j );
		mask = 0;
		if( !( j_surf%step_v ) ) mask = COND_U;

		for( i_surf=0; i_surf<g->n; i_surf++ )
		{
			u = du*i_surf;
			a0u = 1- 3*u*u + 2*u*u*u;
			a1u = 1 - a0u;

			read_elem( u1, i_surf, &node_u1_i );
			read_elem( u2, i_surf, &node_u2_i );

			node.p.x = a0u*node_v1_j.x + a1u*node_v2_j.x +
								 a0v*node_u1_i.x + a1v*node_u2_i.x -
								 a0v*( a0u*node_u1_0.x + a1u*node_v2_0.x ) -
								 a1v*( a0u*node_u2_0.x + a1u*node_v2_n.x );

			node.p.y = a0u*node_v1_j.y + a1u*node_v2_j.y +
								 a0v*node_u1_i.y + a1v*node_u2_i.y -
								 a0v*( a0u*node_u1_0.y + a1u*node_v2_0.y ) -
								 a1v*( a0u*node_u2_0.y + a1u*node_v2_n.y );

			node.p.z = a0u*node_v1_j.z + a1u*node_v2_j.z +
								 a0v*node_u1_i.z + a1v*node_u2_i.z -
								 a0v*( a0u*node_u1_0.z + a1u*node_v2_0.z ) -
								 a1v*( a0u*node_u2_0.z + a1u*node_v2_n.z );
			node.num = mask;
			if( !( i_surf%step_u ) ) node.num |= COND_V;
			if( !add_elem( &g->vdim, &node ) ) return FALSE;
		}
	}
	return TRUE;
}


