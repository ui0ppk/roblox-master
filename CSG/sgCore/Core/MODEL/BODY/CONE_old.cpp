#include "../../sg.h"

short create_cone_np(lpLISTH listh, sgFloat  *par, BOOL constr)
{
	sgFloat 		  r1 = par[0];       //   
	sgFloat 		  r2 = par[1];       //   
	sgFloat 		  h  = par[2];       // 
	int			    n  = (int)par[3];       // -   
	short         i, j;
	sgFloat      r,z,alfa,d_alfa;
	MESHDD      mdd;
	MNODE       node;
	lpNPW       np = NULL;
	sgFloat      k_i, step_i;

	c_num_np = -32767;
	init_vdim(&mdd.vdim,sizeof(node));
	if ( !np_init_list(&c_list_str) ) goto err;

	d_alfa = M_PI*2/n;
	step_i = n/4.;
	r = r1; z = 0;
	for (j=0 ; j < 2 ; j++ ) {
		for (i = 0, k_i=0.; i <= n ; i++) {
			alfa = ( i != n ) ? (d_alfa*i) : 2*M_PI;
			node.p.x = r*cos(alfa);
			node.p.y = r*sin(alfa);
			node.p.z = z;
			node.num = (short)CONSTR_U;
			if ( constr )	node.num |= (short)CONSTR_V;
			else {
				if ( i == (short)(k_i+0.5) ) {
					node.num |= (short)COND_V;
					k_i += step_i;
				}
			}
			if ( !add_elem(&mdd.vdim,&node) ) {
				free_vdim(&mdd.vdim);
				return FALSE;
			}
		}
//    if (j == 1) break;
		 r = r2; z = h;
	}
	mdd.n = n+1;
	mdd.m = 2;

	if(!(np_mesh4p(&c_list_str,&c_num_np,&mdd,0.))) goto err;
	free_vdim(&mdd.vdim);

//   
	if ((np = creat_np_mem(TNPW,MAXNOV,MAXNOE,MAXNOC, MAXNOF,
													MAXNOE)) == NULL)  goto err1;
	np_init((lpNP)np);
	if ( r1 > eps_d ) {
		np_face(np, r1, n, 0.,&c_num_np);							  //  
		np->f[1].fl = ST_FACE_PLANE;
		if( np->nov > MAXNOV ||	np->noe > MAXNOE ) {
			if( !sub_division( np ) ) goto err1;
		}	else	if ( !np_put(np,&c_list_str) ) goto err1;
	}
	if ( r2 > eps_d ) {
		np_face(np, r2, n, h, &c_num_np);               //  
		np->f[1].fl = ST_FACE_PLANE;
		if( np->nov > MAXNOV ||	np->noe > MAXNOE ) {
			if( !sub_division( np ) ) goto err1;
		} else if ( !np_put(np,&c_list_str) ) goto err1;
	}
	 //   
	if ((c_num_np = np_end_of_put(&c_list_str,NP_FIRST,30,listh)) == 0) goto err1;

	free_np_mem(&np);
	return c_num_np;

err1:
	free_np_mem(&np);
err:
	free_vdim(&mdd.vdim);
	np_end_of_put(&c_list_str,NP_CANCEL,0,NULL);
	return 0;
}
