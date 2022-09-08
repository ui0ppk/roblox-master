#include "../../sg.h"
static void change_point (lpSURF_DAT surf_dat, lpD_POINT point, short k );
static void change_weight_point(lpSURF_DAT surf_dat, sgFloat w, short k );

BOOL move_surf_knot(lpSURF_DAT surf_dat, short i, short j, lpD_POINT point ){
short 	k;

	if( surf_dat->sdtype & SURF_APPR ){
// 
	  if( surf_dat->poles & SURF_POLEU0 && j==0 ){
    	for( k=0; k<surf_dat->P.n; k++ ) change_point( surf_dat, point, k );
      return TRUE;
    }
    if( surf_dat->poles & SURF_POLEU1 && j==surf_dat->P.m-1 ){
    	for( k=0; k<surf_dat->P.n; k++ ) change_point( surf_dat, point, surf_dat->P.n*j+k );
      return TRUE;
    }

    if( surf_dat->poles & SURF_POLEV0 && i==0 ){
    	for( k=0; k<surf_dat->P.m; k++ ) change_point( surf_dat, point, surf_dat->P.n*k );
      return TRUE;
    }

    if( surf_dat->poles & SURF_POLEV1 && i==surf_dat->P.n-1 ){
    	for( k=0; k<surf_dat->P.n; k++ ) change_point( surf_dat, point, surf_dat->P.n*k+i );
      return TRUE;
    }

//
		if( surf_dat->sdtype & SURF_CLOSEU && ( i==0 || i==surf_dat->P.n-1) ||
				surf_dat->sdtype & SURF_CLOSEV && ( j==0 || j==surf_dat->P.m-1) ){
	  	if( surf_dat->sdtype & SURF_CLOSEU && ( i==0 || i==surf_dat->P.n-1) ){  //  U
				change_point( surf_dat, point, surf_dat->P.n*j );
				change_point( surf_dat, point, surf_dat->P.n*j+surf_dat->P.n-1 );
	    }
  		if( surf_dat->sdtype & SURF_CLOSEV && ( j==0 || j==surf_dat->P.m-1) ){  //  V
				change_point( surf_dat, point, i );
				change_point( surf_dat, point, surf_dat->P.n*(surf_dat->P.m-1)+i );
	    }
  	  return TRUE;
    }
//  
    change_point( surf_dat, point, surf_dat->P.n*j+i );
	}else{ 	}
	return TRUE;
}

BOOL change_surf_weight_knot(lpSURF_DAT surf_dat, short i, short j, sgFloat w){
short 	k;

	if( surf_dat->sdtype & SURF_APPR ){
// 
	  if( surf_dat->poles & SURF_POLEU0 && j==0 ){
    	for( k=0; k<surf_dat->P.n; k++ ) change_weight_point( surf_dat, w, k );
      return TRUE;
    }
    if( surf_dat->poles & SURF_POLEU1 && j==surf_dat->P.m-1 ){
    	for( k=0; k<surf_dat->P.n; k++ ) change_weight_point( surf_dat, w, surf_dat->P.n*j+k );
      return TRUE;
    }

    if( surf_dat->poles & SURF_POLEV0 && i==0 ){
    	for( k=0; k<surf_dat->P.m; k++ ) change_weight_point( surf_dat, w, surf_dat->P.n*k );
      return TRUE;
    }

    if( surf_dat->poles & SURF_POLEV1 && i==surf_dat->P.n-1 ){
    	for( k=0; k<surf_dat->P.n; k++ ) change_weight_point( surf_dat, w, surf_dat->P.n*k+i );
      return TRUE;
    }

//
  	if( surf_dat->sdtype & SURF_CLOSEU && ( i==0 || i==surf_dat->P.n-1) ){  //  U
			change_weight_point( surf_dat, w, surf_dat->P.n*j );
			change_weight_point( surf_dat, w, surf_dat->P.n*j+surf_dat->P.n-1 );
      return TRUE;
    }
  	if( surf_dat->sdtype & SURF_CLOSEV && ( j==0 || j==surf_dat->P.m-1) ){  //  V
			change_weight_point( surf_dat, w, i );
			change_weight_point( surf_dat, w, surf_dat->P.n*(surf_dat->P.m-1)+i );
      return TRUE;
    }

//  
		change_weight_point( surf_dat, w, surf_dat->P.n*j+i );
	}else{ 	}
	return TRUE;
}

BOOL add_break_surf_knot( lpSURF_DAT surf_dat, short i, short j ){
	if( !add_break_surf_line( surf_dat, 0, j )) return FALSE;
//  if( !add_break_surf_line( surf_dat, 1, i )) return FALSE;
  return TRUE;
}

BOOL move_surf_line(lpSURF_DAT surf_dat, short count, short direction, sgFloat D, lpD_POINT vector){
short 	i, number;
sgFloat	w;
W_NODE  node;
D_POINT point;

	if( surf_dat->sdtype & SURF_APPR ){
		if( direction==0 ){
			for( i=0; i<surf_dat->P.n; i++){
				read_elem(&surf_dat->P.vdim, surf_dat->P.n*count+i, &node);
		    Create_3D_From_4D( &node, (sgFloat *)&point, &w );
				point.x += vector->x;//*D;
				point.y += vector->y;//*D;
				point.z += vector->z;//*D;
        Create_4D_From_3D( &node, (sgFloat *)&point, w);
				write_elem(&surf_dat->P.vdim, surf_dat->P.n*count+i, &node);
			}
		}else{
			for( number=0, i=0; i<surf_dat->P.m; i++, number+=surf_dat->P.n){
				read_elem(&surf_dat->P.vdim, surf_dat->P.n*i+count, &node);
		    Create_3D_From_4D( &node, (sgFloat *)&point, &w );
				point.x += vector->x;//*D;
				point.y += vector->y;//*D;
				point.z += vector->z;//*D;
        Create_4D_From_3D( &node, (sgFloat *)&point, w);
				write_elem(&surf_dat->P.vdim, surf_dat->P.n*i+count, &node);
			}
		}
	}else{
	}
	if( surf_dat->knots.n > 2 && surf_dat->knots.m > 2 ){
//!!!!!!!!!!--    ---!!!!
//		if( !surf_parameter(surf_dat, 0, FALSE )) return FALSE;
		calculate_surf_P(surf_dat, 0);
	}
	return TRUE;
}

BOOL change_surf_weight_on_line(lpSURF_DAT surf_dat, short count, short direction, sgFloat W){
short 	i, number;
sgFloat  w1;
D_POINT	point;
W_NODE  node;

	if( surf_dat->sdtype & SURF_APPR ){
		if( direction==0 )
			for( i=0; i<surf_dat->P.n; i++){
		   	read_elem(&surf_dat->P.vdim, surf_dat->P.n*count+i, &node);
    		Create_3D_From_4D( &node, (sgFloat *)&point, &w1);
    		Create_4D_From_3D( &node, (sgFloat *)&point, W  );
				write_elem(&surf_dat->P.vdim, surf_dat->P.n*count+i, &W);
      }
		else
			for( number=0, i=0; i<surf_dat->P.m; i++, number+=surf_dat->P.n){
		   	read_elem(&surf_dat->P.vdim, surf_dat->P.n*i+count, &node);
    		Create_3D_From_4D( &node, (sgFloat *)&point, &w1);
    		Create_4D_From_3D( &node, (sgFloat *)&point, W  );
				write_elem(&surf_dat->P.vdim, surf_dat->P.n*i+count, &W);
      }

		if( surf_dat->knots.n > 2 && surf_dat->knots.m > 2 ){
//!!!!!!!!!!--    ---!!!!
			if( !surf_parameter(surf_dat, 0, TRUE )) return FALSE;
			calculate_surf_P(surf_dat, 0);
		}
	}
	return TRUE;
}

BOOL bend_surf_line(lpSURF_DAT surf_dat, short count, short direction, sgFloat D, lpD_POINT W){
short 	 	i, j, number;
short 	 	coeff=10;//     
sgFloat		D1, w;
W_NODE		node;
lpD_POINT line;
D_POINT 	point;

	if( surf_dat->sdtype & SURF_APPR ){
		if( direction==0 ){
			if( (line=(D_POINT*)SGMalloc(surf_dat->P.n*sizeof(D_POINT)))==NULL) return FALSE;
// 
			for( i=0; i<surf_dat->P.n; i++ ){
				read_elem(&surf_dat->P.vdim, surf_dat->P.n*count+i, &node);
        Create_3D_From_4D( &node, (sgFloat *)&(line[i]), &w);
				point.x = line[i].x+W->x*D;
				point.y = line[i].y+W->y*D;
				point.z = line[i].z+W->z*D;
        Create_4D_From_3D( &node, (sgFloat *)&point, w);
				write_elem(&surf_dat->P.vdim, surf_dat->P.n*count+i, &node);
			}

//        
			for( j=0; j<surf_dat->P.m; j++ ){
				if( j==count) continue;
				for( i=0; i<surf_dat->P.n; i++){
					read_elem(&surf_dat->P.vdim, surf_dat->P.n*j+i, &node);
	        Create_3D_From_4D( &node, (sgFloat *)&point, &w);
					D1=D/(dpoint_distance(&point, &(line[i]))*coeff);
					point.x += W->x*D1;
					point.y += W->y*D1;
					point.z += W->z*D1;
	        Create_4D_From_3D( &node, (sgFloat *)&point, w);
					write_elem(&surf_dat->P.vdim, surf_dat->P.n*j+i, &node);
				}
			}
		}else{
			if( (line=(D_POINT*)SGMalloc(surf_dat->P.m*sizeof(D_POINT)))==NULL) return FALSE;
// 
			for( number=0, i=0; i<surf_dat->P.m; i++, number+=surf_dat->P.n ){
				read_elem(&surf_dat->P.vdim, surf_dat->P.n*i+count, &node );
        Create_3D_From_4D( &node, (sgFloat *)&(line[i]), &w);
				point.x = line[i].x+W->x*D;
				point.y = line[i].y+W->y*D;
				point.z = line[i].z+W->z*D;
        Create_4D_From_3D( &node, (sgFloat *)&point, w);
				write_elem(&surf_dat->P.vdim, surf_dat->P.n*i+count, &node);
			}

//        
			for( i=0; i<surf_dat->P.m; i++ ){
				for( j=0; j<surf_dat->P.n; j++ ){
					if( j==count) continue;
					read_elem(&surf_dat->P.vdim, surf_dat->P.n*i+j, &point);
	        Create_3D_From_4D( &node, (sgFloat *)&point, &w);
					//D1=D/(dpoint_distance(&point, &(line[i]))*coeff);
					point.x += W->x*D;
					point.y += W->y*D;
					point.z += W->z*D;
	        Create_4D_From_3D( &node, (sgFloat *)&point, w);
					write_elem(&surf_dat->P.vdim, surf_dat->P.n*i+j, &node);
				}
			}
		}
		SGFree(line);
	}else{
	}
	if( surf_dat->knots.n > 2 && surf_dat->knots.m > 2 ){
//!!!!!!!!!!--    ---!!!!
		if( !surf_parameter(surf_dat, 0, FALSE )) return FALSE;
		calculate_surf_P(surf_dat, 0);
	}
	return TRUE;
}

BOOL add_break_surf_line( lpSURF_DAT surf_dat, short direction, short num ){
	short			i, j, ii;
  sgFloat		break_param;
	SPLY_DAT  spline;
  VDIM 			P_new;
  W_NODE		node;

	if( surf_dat->sdtype & SURF_APPR ){
		if( !direction ){ //u-direction

// Create empty work spline for u-direction
			if( !init_sply_dat( SPL_NEW, surf_dat->degree_q, SPL_APPR, &spline ) )	return FALSE;

			if( !init_vdim( &P_new, sizeof(W_NODE)) ) goto err; //  -

      break_param = surf_dat->v[num];
      for( ii=0; ii<surf_dat->P.n; ii++ ){
      	spline.nump=0;
//  
	  		for( i=0; i<surf_dat->P.m; i++ ){
					read_elem( &surf_dat->P.vdim, surf_dat->P.n*i+ii, &spline.P[i] );
					spline.nump++;
  	  	}

// Put surface parameters into spline structure
			for( j=0; j<surf_dat->P.m; j++ )   spline.u[j]=surf_dat->v[j];
			for( j=0; j<surf_dat->num_v; j++ ) spline.U[j]=surf_dat->V[j];
      spline.numU=surf_dat->num_v;

//       num
			  if( !Create_Multy_Point( &spline, break_param, 0 ) ) goto err1;

//write calculated P-points for all surface into temporary VDIM
				for( j=0; j<spline.nump; j++ ) add_elem( &P_new, &spline.P[j] );
      }

//write calculated P-points for all surface
      free_vdim( &surf_dat->P.vdim );
			if( !init_vdim( &surf_dat->P.vdim, sizeof(W_NODE)) ) goto err1; //  -

      for( j=0; j<spline.nump; j++ ){
	      for( i=0; i<surf_dat->P.n; i++ ){
        	read_elem( &P_new, spline.nump*i+j, &node );
				  add_elem( &surf_dat->P.vdim, &node );
        }
      }
			surf_dat->P.m=spline.nump;
			free_vdim( &P_new );

     	if( spline.numU > 2*(MAX_POINT_ON_SURFACE+surf_dat->degree_q+1) ){
       	SGFree( surf_dat->v );
        if( ( surf_dat->v=(sgFloat*)SGMalloc( spline.nump*sizeof(sgFloat) ) ) == NULL ) goto err1;
        SGFree( surf_dat->V );
        if( ( surf_dat->V=(sgFloat*)SGMalloc( spline.numU*sizeof(sgFloat) ) ) == NULL ) goto err1;
      }
// Put new surface parameters from spline structure to surface structure
			for( j=0; j<spline.nump; j++ ) surf_dat->v[j]=spline.u[j];
			for( j=0; j<spline.numU; j++ ) surf_dat->V[j]=spline.U[j];
 			surf_dat->num_v=spline.numU;
     	free_sply_dat( &spline );

	  }else{ // v-direction
// Create empty work spline for v-direction
			if( !init_sply_dat( SPL_NEW, surf_dat->degree_p, SPL_APPR, &spline ) )	return FALSE;

      if( !init_vdim( &P_new, sizeof(W_NODE)) ) goto err; //  -

      break_param=surf_dat->u[num];
      for( ii=0; ii<surf_dat->P.m; ii++ ){
      	spline.nump=0;
//  
	  		for( i=0; i<surf_dat->P.n; i++ ){
					read_elem( &surf_dat->P.vdim, surf_dat->P.n*i+ii, &spline.P[i] );
					spline.nump++;
  	  	}
// Put surface parameters into spline structure
				for( j=0; j<surf_dat->P.n; j++ )   spline.u[j]=surf_dat->u[j];
				for( j=0; j<surf_dat->num_u; j++ ) spline.U[j]=surf_dat->U[j];
    	  spline.numU=surf_dat->num_u;

//       num
			  if( !Create_Multy_Point( &spline, break_param, 0 ) ) goto err1;

//write calculated P-points for all surface into temporary VDIM
				for( j=0; j<spline.nump; j++ ) add_elem( &P_new, &spline.P[j] );
      }

//write calculated P-points for all surface
      free_vdim( &surf_dat->P.vdim );
			if( !init_vdim( &surf_dat->P.vdim, sizeof(W_NODE)) ) goto err1; //  -

      for( i=0; i<surf_dat->P.m; i++ ){
	      for( j=0; j<spline.nump; j++ ){
        	read_elem( &P_new, spline.nump*i+j, &node );
				  add_elem( &surf_dat->P.vdim, &node );
        }
      }
      surf_dat->P.n=spline.nump;
      free_vdim( &P_new );

     	if( spline.numU > 2*(MAX_POINT_ON_SURFACE+surf_dat->degree_p+1) ){
       	SGFree( surf_dat->u );
        if( ( surf_dat->u=(sgFloat*)SGMalloc( spline.nump*sizeof(sgFloat) ) ) == NULL ) goto err1;
        SGFree( surf_dat->U );
        if( ( surf_dat->U=(sgFloat*)SGMalloc( spline.numU*sizeof(sgFloat) ) ) == NULL ) goto err1;
      }
// Put new surface parameters from spline structure to surface structure
			for( j=0; j<spline.nump; j++ ) surf_dat->u[j]=spline.u[j];
			for( j=0; j<spline.numU; j++ ) surf_dat->U[j]=spline.U[j];
 			surf_dat->num_u=spline.numU;
     	free_sply_dat( &spline );
  	}
    return TRUE;
  }
err1:
  free_vdim( &P_new );
err:
	free_sply_dat( &spline );
  return FALSE;
}

static void change_point(lpSURF_DAT surf_dat, lpD_POINT point, short k ){
sgFloat  w;
D_POINT p;
W_NODE	node;

 	read_elem(&surf_dat->P.vdim, k, &node);
  Create_3D_From_4D( &node, (sgFloat *)&p, &w );
  Create_4D_From_3D( &node, (sgFloat *)point, w );
 	write_elem(&surf_dat->P.vdim, k, &node);
}

static void change_weight_point(lpSURF_DAT surf_dat, sgFloat w, short k ){
sgFloat  w1;
D_POINT p;
W_NODE	node;

  read_elem( &surf_dat->P.vdim, k, &node);
  Create_3D_From_4D( &node, (sgFloat *)&p, &w1 );
  Create_4D_From_3D( &node, (sgFloat *)&p, w);
	write_elem(&surf_dat->P.vdim, k, &node);
}

