#include "../sg.h"
static void write_P_from_knots(lpSPLY_DAT sply_dat, short condition );

BOOL calculate_P(lpSPLY_DAT sply_dat, short condition){
	short    p_c, k, i, nnn;
	sgFloat **A;

	nnn = (sply_dat->sdtype & SPL_INT) ? sply_dat->numk : sply_dat->nump;
	k=nnn+sply_dat->numd;
	p_c = sply_dat->degree;
//decrease nurbs degree
	while( nnn < p_c+1 ) p_c--;
	if( p_c <= 0 ){
		nurbs_handler_err(SPL_INTERNAL_ERROR);
		return FALSE;
	}
	if( sply_dat->sdtype & SPL_APPR ){ // approximative spline
		if( ( sply_dat->sdtype&SPL_GEO ) ){
//			if( sply_dat->nump > sply_dat->numk){
//				SGFree( sply_dat->knots );
//				if( (sply_dat->knots = SGMalloc(sply_dat->nump*sizeof(DA_POINT)))== NULL) return FALSE;
//      }
		}
		for( i=0; i<sply_dat->nump; i++ )
			 get_point_on_sply( sply_dat, sply_dat->u[i],
													(lpD_POINT)&sply_dat->knots[i], 0);
		sply_dat->numk = sply_dat->nump;
	}else{                             // interpolative spline
		if( (A = mem_matrix( k+1, k+1 )) == NULL)	return FALSE;
		write_P_from_knots( sply_dat, condition ); //rigth part of equation
		nurbs_matrix_create( sply_dat, p_c, A, condition );

		if( condition == 0 || condition == 2 ){
//NURBS free or NURBS free with derivates
			if( !gauss( A, sply_dat->P, k-1) ) goto err;
			sply_dat->nump = k; //number of point in convex
		}else if( (condition == 1 || condition == 3) && sply_dat->degree > 1 ){
// NURBS is closed with/without derivates
// for closer consider c(1) = c(n), c`(1)=c`(n), c``(1)=c``(n),
// and in matrix - first  string  c(1)=P(1);
//                 second string  c`(1)-c`(n)=0;
//                 n+1-th string  c``(1)-c``(n)=0;
			if( !gauss(  A,	sply_dat->P, k ) ) goto err;
			memcpy(&sply_dat->P[k+1], &sply_dat->P[0], sizeof(DA_POINT));
			sply_dat->nump = k + 2; //number of point in convex
		}
		free_matrix( A, k+1 );
	}
	return TRUE;
err:
	free_matrix( A, k+1 );
	nurbs_handler_err(SPL_INTERNAL_ERROR);
	return FALSE;
}

/**********************************************************
* Put right part of equation
*/
static void write_P_from_knots(lpSPLY_DAT sply_dat, short condition ){
	short i, j, k;

	switch(condition){
		case 0://free
			for( i=0; i<sply_dat->numk; i++ ){
				for( j=0; j<3 ;j++ ) sply_dat->P[i].vertex[j]=sply_dat->knots[i][j];
        sply_dat->P[i].vertex[3]=1.;
      }
			break;
		case 1://closed
			for( j=0; j<3 ;j++ ) sply_dat->P[0].vertex[j]=sply_dat->knots[0][j];
      sply_dat->P[0].vertex[3]=1.;
			for( j=0; j<3 ;j++ ) sply_dat->P[1].vertex[j]=0.;
      sply_dat->P[1].vertex[3]=1.;

			for( i=2; i<=sply_dat->numk-1; i++ ){
				for( j=0; j<3 ;j++ ) sply_dat->P[i].vertex[j]=sply_dat->knots[i-1][j];
	      sply_dat->P[i].vertex[3]=1.;
      }
			for( j=0; j<3 ;j++ ) sply_dat->P[sply_dat->numk].vertex[j]=0.;
   	  sply_dat->P[sply_dat->numk].vertex[3]=1.;
			break;
		case 2: //free with derivates
			for( i=0, j=0; i<sply_dat->numk-1; i++, j++ ){
        for( k=0; k<3; k++ ) sply_dat->P[j].vertex[k]=sply_dat->knots[i][k];
        sply_dat->P[j].vertex[3]=1.;
				if( sply_dat->derivates[i].num ){
          j++;
  	      for( k=0; k<3; k++ ) sply_dat->P[j].vertex[k]=sply_dat->derivates[i].p[k];
     		  sply_dat->P[j].vertex[3]=1.;
        }
			}
			if( sply_dat->derivates[sply_dat->numk-1].num ){
  	    for( k=0; k<3; k++ ) sply_dat->P[j].vertex[k]=sply_dat->derivates[sply_dat->numk-1].p[k];
     		sply_dat->P[j].vertex[3]=1.;
        j++;
      }
      for( k=0; k<3; k++ ) sply_dat->P[j].vertex[k]=sply_dat->knots[sply_dat->numk-1][k];
 		  sply_dat->P[j].vertex[3]=1.;
			break;
		case 3://closed with derivates
			for( j=0; j<3 ;j++ ) sply_dat->P[0].vertex[j]=sply_dat->knots[0][j];
      sply_dat->P[0].vertex[3]=1.;
			for( j=0; j<3 ;j++ ) sply_dat->P[1].vertex[j]=0.;
      sply_dat->P[1].vertex[3]=1.;
			j=2;
			if( sply_dat->derivates[0].num ){
  	    for( k=0; k<3; k++ ) sply_dat->P[j].vertex[k]=sply_dat->derivates[0].p[k];
     		sply_dat->P[j++].vertex[3]=1.;
      }

			for( i=1; i<sply_dat->numk-1; i++, j++ ){
        for( k=0; k<3; k++ ) sply_dat->P[j].vertex[k]=sply_dat->knots[i][k];
        sply_dat->P[j].vertex[3]=1.;

				if( sply_dat->derivates[i].num ){
          j++;
 		 	    for( k=0; k<3; k++ ) sply_dat->P[j].vertex[k]=sply_dat->derivates[i].p[k];
    	 		sply_dat->P[j].vertex[3]=1.;
        }
			}
			if( sply_dat->derivates[sply_dat->numk-1].num ){
  	    for( k=0; k<3; k++ ) sply_dat->P[j].vertex[k]=sply_dat->derivates[sply_dat->numk-1].p[k];
     		sply_dat->P[j++].vertex[3]=1.;
      }
      for( k=0; k<3; k++ ) sply_dat->P[j].vertex[k]=sply_dat->knots[sply_dat->numk-1][k];
 		  sply_dat->P[j].vertex[3]=1.;
			break;
	}
}
