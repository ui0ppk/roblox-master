#include "../../sg.h"

#pragma argsused
BOOL surf_parameter(lpSURF_DAT surf_dat, short conditon, BOOL p ){
short 		 	i;
sgFloat 		 *U=NULL, *V=NULL, w;
W_NODE			node;
lpDA_POINT  Q;

	if( p ){
		U = surf_dat->U;
		V = surf_dat->V;
	}
	if( surf_dat->sdtype & SURF_APPR ){//
//U-direction
		if( (Q=(lpDA_POINT)SGMalloc( surf_dat->P.n*sizeof(DA_POINT) )) == NULL) return FALSE;
		for(i=0; i<surf_dat->P.n; i++) {
			read_elem( &surf_dat->P.vdim, i, &node );
	    Create_3D_From_4D( &node, (sgFloat *)&Q[i], &w );
    }
//the first boundary may be degenerated
    if( dpoint_distance((lpD_POINT)&Q[0], (lpD_POINT)&Q[1])<eps_d ){
			for(i=0; i<surf_dat->P.n; i++){
				read_elem( &surf_dat->P.vdim, i+surf_dat->P.n, &node );
		    Create_3D_From_4D( &node, (sgFloat *)&Q[i], &w );
      }
    }
		parameter_free( surf_dat->P.n-1, surf_dat->degree_p,
										Q, (sgFloat *)surf_dat->u, U );
		SGFree( Q );

//V-direction
		if( (Q=(lpDA_POINT)SGMalloc( surf_dat->P.m*sizeof(DA_POINT) )) == NULL) return FALSE;
		for(i=0; i<surf_dat->P.m; i++){
			read_elem( &surf_dat->P.vdim, surf_dat->P.n*i, &node );
	    Create_3D_From_4D( &node, (sgFloat *)&Q[i], &w );
    }
		parameter_free( surf_dat->P.m-1, surf_dat->degree_q,
										Q, surf_dat->v, V );
		SGFree( Q );
		if( p ){
			surf_dat->num_u=surf_dat->P.n+surf_dat->degree_p+1;
			surf_dat->num_v=surf_dat->P.m+surf_dat->degree_q+1;
		}
	}else{ //
	}
	return TRUE;
}

#pragma argsused
BOOL calculate_surf_P(lpSURF_DAT surf_dat, short condition){
	if( surf_dat->sdtype & SURF_APPR ){ // approximative spline
//temporary !!!!!!!!!!!!!!!!
		 if( !knots_on_surface( surf_dat ) )return FALSE;
	}else{
	}
	return TRUE;
}



