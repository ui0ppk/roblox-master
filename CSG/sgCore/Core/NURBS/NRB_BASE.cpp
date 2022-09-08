#include "../sg.h"
static void   calculate_free_U(short n_c, short p_c, sgFloat *u_c, sgFloat *U_c );
//static sgFloat calculate_R_Sum( short n_c, lpW_NODE P, sgFloat *N);
static void calc_w_point(lpSPLY_DAT sply_dat, lpD_POINT p, sgFloat *N);
static void calc_w_deriv(lpSPLY_DAT sply_dat, short degree, 
														lpD_POINT p, short deriv, sgFloat *N_i, sgFloat *N_j);

/*******************************************************************************
* Obtaining B-sply basis function of degree p
* U_p - parametrical vector
* u_p - parameter
*/
sgFloat nurbs_basis_func( short i, short p, sgFloat *U_p, sgFloat u_p ){
	sgFloat N, N1;
	 if( p == 0 ){ // degree 0
		 if( U_p[i] <= u_p && u_p < U_p[i+1] ) return(1);
		 if( U_p[i] < U_p[i+1] && u_p == U_p[i+1] && u_p == 1 ) return(1);
		 return(0);
	 }
// degree p
	 N=0;
	 if( ( N1 = U_p[i+p] - U_p[i] ) != 0. )
		 N = ( u_p - U_p[i] )*nurbs_basis_func( i, p-1, U_p, u_p )/N1;

	 if( ( N1 = U_p[i+p+1] - U_p[i+1] ) != 0. )
		 N = N + ( U_p[i+p+1] - u_p )*nurbs_basis_func( i+1, p-1, U_p, u_p )/N1;
	 return( N );
}


/*******************************************************************************
* Obtaining parameter values by centripetal method
* n_c   - number of points
* p_c   - degree of curve
* Q_tmp - curve to creation parametrical vector
* U_c   - parametrical vector
*/
void parameter( lpSPLY_DAT sply_dat, short condition, BOOL p ){
short			i;
sgFloat *U=NULL;
	if( p ) U = sply_dat->U;
	if( sply_dat->sdtype & SPL_APPR ){
		parameter_free_APPR( sply_dat->nump-1, sply_dat->degree, sply_dat->P,
											 	 sply_dat->u, U );
		if( p ) sply_dat->numU = sply_dat->nump+sply_dat->degree+1;
	}else{
		if( p ) sply_dat->numU = sply_dat->nump+sply_dat->degree+1;
		switch(condition){
			case 0://free NURBS
				parameter_free( sply_dat->numk-1, sply_dat->degree, sply_dat->knots,
												sply_dat->u, U );
				if( p ) sply_dat->numU = sply_dat->numk+sply_dat->degree+1;
				break;
			case 1://closed NURBS without derivates
				parameter_closed( sply_dat->numk-1, sply_dat->degree, sply_dat->knots,
													 sply_dat->u, U );
				if( p ) sply_dat->numU = sply_dat->numk+sply_dat->degree+1+2;
				break;
			case 2://free NURBS with derivates
				parameter_deriv( sply_dat->numk-1, sply_dat->degree, sply_dat->knots,
												sply_dat->derivates,	sply_dat->u, U );
				if( p )	for( i=0; i<sply_dat->numk; i++)
									if( sply_dat->derivates[i].num ) sply_dat->numU++;
				break;
			case 3://closed NURBS with derivates
				parameter_deriv_closed( sply_dat->numk-1, sply_dat->degree, sply_dat->knots,
																sply_dat->derivates,	sply_dat->u, U );
				if( p ){
					sply_dat->numU = sply_dat->numk+sply_dat->degree+1+2;
					for( i=1; i<sply_dat->numk-1; i++)
						if( sply_dat->derivates[i].num ) sply_dat->numU++;
				}
				break;
		}
	}
}

/*******************************************************************************
* Obtaining parameter values by centripetal method and
* creating U-vector
*/
void parameter_free( short n_c, short p_c, lpDA_POINT Q_tmp,
														sgFloat *u_c, sgFloat *U_c ){
	short  i;
	sgFloat S;

//decrease nurbs degree
	while( n_c + 1 < p_c + 1 ) p_c--;
	if( p_c <= 0 ) return;

//calculate parameters for points, begin and end  - interval [0,1]
	u_c[0]=0.; u_c[n_c]=1.;

// compute sum
	for( S=0., i=1; i<=n_c; i++ )
		S +=  sqrt(dpoint_distance( (lpD_POINT)Q_tmp[i], (lpD_POINT)Q_tmp[i-1] ));
	S = 1./S;

//compute parameters
	for( i=1; i<n_c; i++ )
		u_c[i] = u_c[i-1] +
			sqrt(dpoint_distance( (lpD_POINT)Q_tmp[i], (lpD_POINT)Q_tmp[i-1] ))*S;

//calculate vector U for computation
	if( U_c != NULL ) calculate_free_U( n_c, p_c, u_c, U_c );
}

/*******************************************************************************
* Obtaining parameter values by centripetal method and
* creating U-vector
*/
void parameter_free_APPR( short n_c, short p_c, lpW_NODE P, sgFloat *u_c, sgFloat *U_c ){
	short  i;
	sgFloat S;

//decrease nurbs degree
	while( n_c + 1 < p_c + 1 ) p_c--;
	if( p_c <= 0 ) return;

//calculate parameters for points, begin and end  - interval [0,1]
	u_c[0]=0.; u_c[n_c]=1.;

// compute sum
	for( S=0, i=1; i<=n_c; i++ ) S+=dpoint_distance_4dl( &P[i], &P[i-1] );
	S=1./S;

//compute parameters
	for( i=1; i<n_c; i++ )
		u_c[i] = u_c[i-1] + dpoint_distance_4dl( &P[i], &P[i-1] )*S;

//calculate vector U for computation
	if( U_c != NULL ) calculate_free_U( n_c, p_c, u_c, U_c );
}

/*******************************************************************************
* calculate vector U
*/
static void calculate_free_U(short n_c, short p_c, sgFloat *u_c, sgFloat *U_c ){
short  i, j, k;
sgFloat S, l;

//calculate vector U for computation
 	l = 1./(sgFloat)p_c;
 	for( i=0;     i<=p_c; i++ ) U_c[i] = 0;
 	for( i=p_c+1, j=1;   j<=n_c-p_c; i++, j++ ) {
 		for( S=0, k=j; k<=j+p_c-1; k++ ) S = S + u_c[k];
 		U_c[i] = l*S;
 	}
 	for( i=n_c+1; i<=n_c+p_c+1; i++ )  U_c[i] = 1;
}

/*******************************************************************************
* Obtaining parameter values by centripetal method for
* closed curve
*/
void parameter_closed( short n_c, short p_c, lpDA_POINT Q_tmp,
															sgFloat *u_c, sgFloat *U_c ){
	short i;
	parameter_free( n_c, p_c, Q_tmp, u_c, U_c );
	if( U_c != NULL ){
		for( i=n_c+p_c+1; i>=p_c+1; i-- ) U_c[i+1] = U_c[i];
		U_c[p_c+1] /= 2;
		for( i=n_c+p_c+2; i>=n_c+2; i-- ) U_c[i+1] = U_c[i];
		U_c[n_c+2] = (U_c[n_c+1]+1)/2;
	}
}

/*******************************************************************************
* Obtaining parameter values by centripetal method for
* free curve with derivates
*/
void parameter_deriv( short n_c, short p_c, lpDA_POINT Q_tmp, lpSNODE deriv,
											sgFloat *u_c, sgFloat *U_c ){
	short i, j, k, l, t;
	parameter_free( n_c, p_c, Q_tmp, u_c, U_c );
	if( U_c != NULL ){
		l=n_c+p_c+1;
		if(dpoint_distance((lpD_POINT)Q_tmp[0],
											 (lpD_POINT)Q_tmp[n_c - 1])< eps_d ) t=n_c-1;
		else t=n_c;
		for( i=0; i<=t; i++)
			if( deriv[i].num ){
				for( j=0; j<l; j++ )
					if( ( U_c[j] <= u_c[i] && u_c[i] < U_c[j+1] ) ||
							( U_c[j] < U_c[j+1] && u_c[i] == U_c[j+1] && u_c[i] == 1 ) ){
						for( k=l; k>=j+1; k-- ) U_c[k+1] = U_c[k];
						U_c[j+1] = ( U_c[j+1] + U_c[j])/2;
						l++;
						break;
					}
			}
	}
}

/*******************************************************************************
* Obtaining parameter values by centripetal method for
* closed curve with derivates
*/
void parameter_deriv_closed( short n_c, short p_c, lpDA_POINT Q_tmp, lpSNODE deriv,
											sgFloat *u_c, sgFloat *U_c ){
	short i, j, k, l;
	parameter_free( n_c, p_c, Q_tmp, u_c, U_c );

	if( U_c != NULL ){
		for( i=n_c+p_c+1; i>=p_c+1; i-- ) U_c[i+1] = U_c[i];
		U_c[p_c+1] /= 2;
		for( i=n_c+p_c+2; i>=n_c+2; i-- ) U_c[i+1] = U_c[i];
		U_c[n_c+2] = (U_c[n_c+1]+1)/2;

		l=n_c+p_c+3;//длна векоа U
		for( i=1; i<=n_c-1; i++)
			if( deriv[i].num ){
				for( j=0; j<l; j++ )
					if( ( U_c[j] <= u_c[i] && u_c[i] < U_c[j+1] ) ||
							( U_c[j] < U_c[j+1] && u_c[i] == U_c[j+1] && u_c[i] == 1 ) ){
						for( k=l; k>=j+1; k-- ) U_c[k+1] = U_c[k];
						U_c[j+1] = ( U_c[j+1] + U_c[j])/2;
						l++;
						break;
					}
			}
	}
}

//------------------------------------------------------------------------------
/*******************************************************************************
* Obtaining parameter values by centripetal method for
* closed curve with derivates
*/
/*static sgFloat calculate_R_Sum( short n_c, lpW_NODE P, sgFloat *N){
	short j;
	sgFloat S=0;
	for( j=0; j<n_c; j++ ) if( N[j]>0 ) S += P[j].vertex[3]*N[j];
	return( S );
} */

/**********************************************************
* Obtaining the point for NURBS with weight
* sply_dat - spline
* t        - point's parameter to calculate derivates
* p        - calculated point
*/
BOOL Calculate_Weight_Point(lpSPLY_DAT sply_dat, short degree, sgFloat t, lpD_POINT p){
short 	  i;
sgFloat *N;
	if((N=(sgFloat*)SGMalloc(sply_dat->nump*sizeof(sgFloat)))==NULL) return FALSE;

	for( i=0; i<sply_dat->nump; i++ )
		if( sply_dat->U[i] <= t && t <= sply_dat->U[i + degree + 1] )
			N[i] = nurbs_basis_func( i, degree, sply_dat->U, t );

	calc_w_point(sply_dat, p, N );
	SGFree(N);
	return TRUE;
}

static void calc_w_point(lpSPLY_DAT sply_dat, lpD_POINT p, sgFloat *N){
short  i, k;
sgFloat w;
W_NODE node;

	memset( &node, 0, sizeof(W_NODE));
	for( i=0; i<sply_dat->nump; i++ )
		if( N[i] != 0 )
      for( k=0; k<4; k++ )	node.vertex[k] += sply_dat->P[i].vertex[k]*N[i];

  Create_3D_From_4D( &node, (sgFloat *)p, &w);
}


/**********************************************************
* Obtaining the first degree derivates for NURBS with weight
* sply_dat - spline
* t        - point's parameter to calculate derivates
* p        - calculated derivates
* deriv    - degree of derivate
*/
BOOL Calculate_Weight_Deriv(lpSPLY_DAT sply_dat, short degree, sgFloat t,
														lpD_POINT p, short deriv){
BOOL   ret=FALSE;
short		 i;
sgFloat *N_j, *N_i;

	if((N_j=(sgFloat*)SGMalloc(sply_dat->nump*sizeof(sgFloat)))==NULL) return FALSE;

	if((N_i=(sgFloat*)SGMalloc(sply_dat->nump*sizeof(sgFloat)))==NULL) goto err;

	for( i=0; i<sply_dat->nump; i++ )
		if( sply_dat->U[i] <= t && t <= sply_dat->U[i+degree+1] )
			N_j[i] = nurbs_basis_func( i, degree, sply_dat->U, t );

	for( i=1; i<sply_dat->nump; i++ )
		if( sply_dat->U[i] <= t && t <= sply_dat->U[i + degree] )
			 N_i[i] = nurbs_basis_func( i, degree-1, sply_dat->U, t );

	calc_w_deriv( sply_dat, degree, p, deriv, N_i, N_j);
	ret=TRUE;
err:
if(N_j) SGFree(N_j);
if(N_i) SGFree(N_i);
return ret;
}

#pragma argsused
static void calc_w_deriv(lpSPLY_DAT sply_dat, short degree,
												lpD_POINT p, short deriv, sgFloat *N_i, sgFloat *N_j){
short		 i, j, k;
sgFloat   feet, w_w, w_der, wi, wj, wi0;
sgFloat   *ppoint;
DA_POINT point={0,0,0}, A={0,0,0}, pi, pj, pi0 ;

	ppoint = (sgFloat*)p;
//------------>>>>>>>>>>weight
	w_w=0;
	for( i=0; i<sply_dat->nump; i++ )
  	if( N_j[i] != 0 ) w_w += sply_dat->P[i].vertex[3]*N_j[i];

//------------>>>>>>>>>>derivates of weight
	w_der=0;
	for( i=1; i<sply_dat->nump; i++ ){
		if( N_i[i] != 0 )
			if( (feet =  sply_dat->U[i+degree] - sply_dat->U[i]) > eps_n )
					w_der += (sply_dat->P[i].vertex[3] - sply_dat->P[i-1].vertex[3])*N_i[i]/feet;
	}
	w_der *= degree;

//------------>>>>>>>>>>point
	calc_w_point(sply_dat, (lpD_POINT)&point, N_j);

//------------>>>>>>>>>> A`[u]
	for(j=0; j<sply_dat->nump; j++)
		if( N_j[j] !=0 ){
			Create_3D_From_4D( &sply_dat->P[j], (sgFloat *)pj, &wj);
			for(i=1; i<sply_dat->nump; i++)
				if( N_i[i] !=0 ){
        	Create_3D_From_4D( &sply_dat->P[i-1], (sgFloat *)pi0, &wi0);
					Create_3D_From_4D( &sply_dat->P[i], (sgFloat *)pi, &wi);
					if( (feet = (sply_dat->U[i+degree] - sply_dat->U[i])) > eps_n ){
						 feet=N_j[j]*N_i[i]/feet;
						 for( k=0; k<3; k++)
//							 A[k] += feet*(sply_dat->P[j].vertex[3]*(sply_dat->P[i].vertex[k]-
//               																				 sply_dat->P[i-1].vertex[k])+
//														 sply_dat->P[j].vertex[k]*(sply_dat->P[i].vertex[3]-
//                             													 sply_dat->P[i-1].vertex[3]));
							 A[k] += feet*(wj*(pi[k]-pi0[k])+pj[k]*(wi-wi0));

					}
				}
		}
	for(i=0; i<3; i++) ppoint[i]=(degree*A[i]-w_der*point[i])/w_w;
}

/**********************************************************
* Obtaining the point for NURBS without weight
* sply_dat - spline
* t        - point's parameter to calculate derivates
* p        - calculated point
*/
void Calculate_Point(lpSPLY_DAT sply_dat, short degree, sgFloat t, lpD_POINT p){
short    i, j;
sgFloat N_j, *point;

	point = (sgFloat*)p;
	for( i=0; i<sply_dat->nump; i++ )
		if( sply_dat->U[i] <= t && t <= sply_dat->U[i+degree+1] )
			if( ( N_j = nurbs_basis_func( i, degree, sply_dat->U, t )) != 0 ){
				for(j=0; j<3; j++) point[j] += sply_dat->P[i].vertex[j]*N_j;
			}
}

/**********************************************************
* Obtaining the first degree derivates for NURBS with weight
* sply_dat - spline
* t        - point's parameter to calculate derivates
* p        - calculated derivates
* deriv    - degree of derivate
*/
void Calculate_Deriv(lpSPLY_DAT sply_dat, short degree, sgFloat t,
											 lpD_POINT p, short deriv){
short    i, j, k;
sgFloat N_j, feet, feet1;
sgFloat *point;

point = (sgFloat*)p;
if( deriv==1){
	for( i=1; i<sply_dat->nump; i++ )
		if( sply_dat->U[i] <= t && t <= sply_dat->U[i + degree] )
			if( ( N_j = nurbs_basis_func( i, degree-1, sply_dat->U, t )) != 0 ){
				feet =  sply_dat->U[i+degree] - sply_dat->U[i];
				if ( feet >= eps_n ){
					N_j /= feet;
					for( j=0; j<3; j++)	point[j] += (sply_dat->P[i].vertex[j] -
                                           sply_dat->P[i-1].vertex[j])*N_j;
				}
			}
	for( j=0; j<3; j++)	point[j] *= degree;
}else{
	for( j=2; j<sply_dat->nump; j++ ){
//				if( sply_dat->U[j] <= t && t <= sply_dat->U[j+degree-1] ){
		i=j-1;
		if( ( N_j = nurbs_basis_func( j, degree-2, sply_dat->U, t )) != 0 ){
			if( (feet = sply_dat->U[i+degree] - sply_dat->U[i+1]) >= eps_n ){
				N_j /= feet;
				if( (feet=sply_dat->U[i+degree+1] - sply_dat->U[i+1]) >= eps_n )
					feet = 1./feet;
            else feet = 0.;
				if( (feet1=sply_dat->U[i+degree] - sply_dat->U[i]) >= eps_n )
					feet1 = 1./feet1;
            else feet1 = 0.;
				for( k=0; k<3; k++ )
					point[k] += ( sply_dat->P[j-2].vertex[k]*feet1 -
												sply_dat->P[j-1].vertex[k]*(feet + feet1) +
												sply_dat->P[j  ].vertex[k]*feet )*N_j;
						}
					}
	//			}
			}
		for(j=0; j<3; j++) point[j] *= degree*(degree-1);
	}
}

