#include "../../sg.h"


short create_elipsoid_np(lpLISTH listh, sgFloat  *par)
{
	sgFloat	ra = par[0], rb = par[1], rc=par[2];
	int			n  = (int)par[3], 	m = (int)par[4];
	int			i, j;
	sgFloat	d_alpha, d_beta, beta, alpha;
	MNODE       node;
	MESHDD      mdd;
	NP_STR_LIST list_str;
	short         num_np = -32767;
	short         mask;
	sgFloat      step_j, step_i, k_i, k_j;

	init_vdim(&mdd.vdim,sizeof(node));
	m+=2;
	step_j = (m-1)/2.;
	step_i = n/4.;

	d_alpha = 2 * M_PI / n;
	d_beta = M_PI / (m-1);
	for ( j = 0, k_j=0.; j < m; j++ ){
		if ( j == (short)(k_j+0.5) ) {
			mask = COND_U;
			k_j += step_j;
		}
		else						mask = 0;

		beta = ( j == m-1 ) ? ( M_PI ) : ( d_beta*j );
		for (i = 0, k_i=0.; i <= n ; i++){
			alpha = ( i == 0 ) ? ( 2*M_PI ) : ( d_alpha*(n-i) );
			node.p.x = ra*sin(beta)*cos(alpha);
			node.p.y = rb*sin(beta)*sin(alpha);
			node.p.z = rc*cos(beta);
			node.num = mask;
			if ( i == (short)(k_i+0.5) ) {
				node.num |= COND_V;
				k_i += step_i;
			}
			if ( !add_elem(&mdd.vdim,&node) ) {
				free_vdim(&mdd.vdim);
				return FALSE;
			}
		}
	}
	mdd.m = m;
	mdd.n = n + 1;

	if ( !np_init_list(&list_str) ) goto err;
	if(!(np_mesh4p(&list_str,&num_np,&mdd,0.))) goto err;
	free_vdim(&mdd.vdim);

	 //   
	if ( !(num_np = np_end_of_put(&list_str,NP_FIRST,100,listh)) ) goto err;
	goto end;

err:
	free_vdim(&mdd.vdim);
	np_end_of_put(&list_str,NP_CANCEL,0,listh);
	num_np = 0;
end:
	return num_np;
}
