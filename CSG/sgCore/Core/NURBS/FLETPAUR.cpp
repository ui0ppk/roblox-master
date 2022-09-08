#include "../sg.h"
static sgFloat min_function ( lpD_POINT p1, lpD_POINT p2 );
static sgFloat min_derivativ( lpD_POINT p1, lpD_POINT p2, lpD_POINT d );
static sgFloat sqrt_derivates( sgFloat *G, short n );
/**********************************************************
* function       - function to be minimazied
* derivativs     - derivativ of function to be minimazied
* C1, C2         - function's parameters
* X              - beginning point, return - point of local min
* n              - number of parameters
* number_of_iter - acceptable number of iterations
* for this reliz number of parameter <=5
*/
sgFloat Fletchjer_Paur( functionFP function, derivativs derivativs,
											 void  *C1, void  *C2, sgFloat *X, short n,
											 short number_of_iteration ){
short    i, j, iter;
sgFloat H[5][5]; // matrix must be n x n !!!!!!!!!!!!!!!!!!
sgFloat P[5], Q[5], U[5], V[5], Y[5], G[5], D[5], M[5];
sgFloat z, w, r, kk, wk, dk, Fp, Fq, Fr, step, Gp, Gq, Gr, min1=-1;
sgFloat G3, eps_dd, eps_nn;

	eps_dd=eps_d*eps_d;
	eps_nn=eps_n*eps_n;
//bilding of begining matrix H - it's unit matrix
	for( i=0; i<n; i++ )	for( j=0; j<n; j++ ) H[i][j] = ( i==j ) ? (1.) : (0.);

	for( iter=0;  iter <= number_of_iteration; iter++ ){
		for( i=0; i<n; i++ )  Y[i] = P[i] = X[i];

//calculating of new quantity of function into point X
		Fp = (*function)( C1, C2, X );
		if( fabs(Fp) < eps_dd ) return (0);

//calculating of new paticular derivates of function
		/*G3 =*/ (*derivativs)( C1, C2, X, G );

//calculating of beginning direction {d} = -[H]{g};
		for( i=0; i<n; i++ ){
			U[i] = G[i];
			for( D[i]=0., j=0; j<n; j++ ) D[i] -= H[i][j]*G[j];
		}
//linear founding of min on function f( x + step*d )
//------------------>  calculating of point P
		while(1){
//calculating of gradient for point P
			for( Gp = 0., i=0; i<n; i++ ) Gp += G[i]*D[i];

//founding of min along direction and choising of step from formula
// min{1, -2(Fp - Fm)/Gp}
// where Fp = F(P),
// Fm - approximation of real quantity of min, in my case Fm = 0
			if( fabs(Gp) < eps_dd ) { step = 1.; break; }
			if( (	step = fabs( 2*Fp/Gp ) ) > 1. ) step = 1.;
			if( Gp < -eps_dd ) break;

// calculating of new point P
			for( i=0; i<n; i++ ) P[i] = X[i] = P[i] - step*D[i];

//calculating of new quantity of function into point P
			Fp = (*function)( C1, C2, P );
			if( fabs(Fp) < eps_dd ) return (0);

//calculating of new paticular derivates of function
			/*G3 =*/ (*derivativs)( C1, C2, P, G );
		}

//------------------>  calculating of point Q
		while(1){
//calculating of next point Q x(i+1) = x(i) + step*d(i)
			for( i=0; i<n; i++ ) Q[i] = X[i] = P[i] + step*D[i];

//calculating of new quantity of function into point Q
			Fq = (*function)( C1, C2, Q );
			if( fabs( Fq ) < eps_dd ) return (0);

//calculating of new paticular derivates of function
			/*G3 =*/ (*derivativs)( C1, C2, Q, G );

//calculating of gradient for point Q
			for( Gq = 0., i=0; i<n; i++ ) Gq += G[i]*D[i];
//			if( Gq > 0. || Fq > Fp ) break;
			if( Gq > eps_dd || Fq > Fp ) break;
			step *= 2; //increase step to "expand" min
		}

		while(1){
//min lay on [p,q]
			z = 3*( Fp - Fq )/step + Gp + Gq;
//			if( ( w = z*z - Gp*Gq ) < 0. ) w = 0.;
			if( ( w = z*z - Gp*Gq ) < -eps_dd ) w = 0.;
			w = sqrt( w );

//approximation of min
			r = step*( 1. - ( Gq + w - z )/( Gq - Gp + 2*w ) );

//calculating of new point
			for( i=0; i<n; i++ ) X[i] = P[i] + r*D[i];

//calculating of new quantity of function into point X
			Fr = (*function)( C1, C2, X );
   		if( fabs(Fr) < eps_dd ) return (0);

//calculating of new paticular derivates of function
			G3 = (*derivativs)( C1, C2, X, G );

//calculating of gradient for point X
			for( Gr=0., i=0; i<n; i++ ) Gr += G[i]*D[i];

			if( fabs( Gr ) < eps_dd ) break;
			if( ( Fr < Fp || fabs( Fp - Fr )<= eps_dd ) &&
					( Fr < Fq || fabs( Fq - Fr )<= eps_dd ) ) break;

//			if( Fr <= Fp && Fr <= Fq ) break;

//			if( Gr > 0. ){
			if( Gr > eps_dd ){ // choise span [p,r]
				step = r;
				for( i=0; i<n; i++ ) Q[i] = X[i];
//??????
				Fq = Fr;  Gq = Gr;
				break;
			}

//taking [r,q]
			step -= r;
			for( i=0; i<n; i++ ) P[i] = X[i];
			Fp = Fr;  Gp = Gr;
		}

//changing of matrix H
// H(i+1) = H(i) + A(i) + B(i)
// A(i) = Vi*ViT / ( ViT*Ui)
// B(i) = - Hi Ui UiT Hi / (UiT Hi Ui)

		for( i=0; i<n ; i++ ){
			U[i] = G[i] - U[i];
			V[i] = X[i] - Y[i];
		}

		for( kk=0, wk=0, dk=0, i=0; i<n; i++ ){
			for( M[i]=0, j=0; j<n; j++ ) M[i] += H[i][j]*U[j];
			kk += M[i]*U[i];
			wk += V[i]*U[i];
			dk += V[i]*V[i];
		}

		if( kk != 0 && wk != 0 )	{
			kk=1./kk; wk=1./wk;
			for( i=0; i<n; i++ )
				for( j=0; j<n; j++ )
					H[i][j] = H[i][j] - M[i]*M[j]*kk + V[i]*V[j]*wk;
		}
//control
//		if( sqrt(dk) < eps_dd || G3 < eps_dd ) break;
		if( dk < eps_nn || G3 < eps_d ) break;
	}  // end of iteration loop
	min1 = ( Fr   > Fp ) ? (Fp) : (Fr);
	min1 = ( min1 > Fq ) ? (Fq) : (min1);

	return( sqrt(min1) );
}

//---------------------------------------------------------->
//================================SPLINE========================================
/***********************************************************
* functions for NURBScurve-to-point distance minimization
*/
sgFloat function_0( void  *sply_dat, void  *point, sgFloat X[] ){
	D_POINT  p1, *p;

//point on NURBS curve
	get_point_on_sply( (lpSPLY_DAT)sply_dat, X[0], &p1, 0 );

//initial point
	p=(D_POINT*)point;

//function to be minimized
	return( min_function( &p1, p ) );
}

sgFloat derivativs_0( void  *sply_dat, void  *point, sgFloat X[], sgFloat G[] ){
	D_POINT  p1, d1, *p;

//point and derivative on NURBS curve
	get_point_on_sply( (lpSPLY_DAT)sply_dat, X[0], &p1, 0 );
	get_point_on_sply( (lpSPLY_DAT)sply_dat, X[0], &d1, 1 );

	p=(D_POINT*)point;

	G[0] = min_derivativ( &p1, p, &d1 );
	return( sqrt_derivates( G, 1 ) );
}

/***********************************************************
* functions for NURBScurve-to-line distance minimization
*/
sgFloat function_1( void  *sply_dat, void  *line, sgFloat X[] ){
	D_POINT  p1, p2, *p;

//point on NURBS curve
	get_point_on_sply( (lpSPLY_DAT)sply_dat, X[0], &p1, 0 );

//parametrical line is r=p1+(p2-p1)*lambda;
	p=(D_POINT*)line;

//point on line
	p2.x = p[0].x + ( p[1].x - p[0].x )*X[1];
	p2.y = p[0].y + ( p[1].y - p[0].y )*X[1];
	p2.z = p[0].z + ( p[1].z - p[0].z )*X[1];

//function to be minimized
	return( min_function( &p1, &p2 ) );
}

sgFloat derivativs_1( void  *sply_dat, void  *line, sgFloat X[], sgFloat G[] ){
	D_POINT  p1, p2, d1, d2, *p;

//point and derivativ on NURBS curve
	get_point_on_sply( (lpSPLY_DAT)sply_dat, X[0], &p1, 0 );
	get_point_on_sply( (lpSPLY_DAT)sply_dat, X[0], &d1, 1 );

//parametrical line is r=p+(p1-p)*lambda;
	p=(D_POINT*)line;

//point and dirivativ on line
	d2.x = p[1].x - p[0].x;
	d2.y = p[1].y - p[0].y;
	d2.z = p[1].z - p[0].z;

	p2.x = p[0].x + d2.x*X[1];
	p2.y = p[0].y + d2.y*X[1];
	p2.z = p[0].z + d2.z*X[1];

	G[0] = 		 min_derivativ( &p1, &p2, &d1 );
	G[1] = -1.*min_derivativ( &p1, &p2, &d2 );
	return( sqrt_derivates( G, 2 ) );
}

/***********************************************************
* functions for NURBScurve-to-plane distance minimization
*/
sgFloat function_2( void  *sply_dat, void  *plane, sgFloat X[] ){
	D_POINT  p1, p2, *p;

//point on NURBS curve
	get_point_on_sply( (lpSPLY_DAT)sply_dat, X[0], &p1, 0 );

//parametrical plane is r=p+(p1-p)*lambda+(p2-p)*nu;
	p=(D_POINT*)plane;

//point on plane
	p2.x = p[0].x + ( p[1].x - p[0].x )*X[1] + ( p[2].x - p[0].x )*X[2];
	p2.y = p[0].y + ( p[1].x - p[0].x )*X[1] + ( p[2].x - p[0].x )*X[2];
	p2.z = p[0].z + ( p[1].x - p[0].x )*X[1] + ( p[2].x - p[0].x )*X[2];

//function to be minimized
	return( min_function( &p1, &p2 ) );
}

sgFloat derivativs_2( void  *sply_dat, void  *plane, sgFloat X[], sgFloat G[] ){
	D_POINT  p1, p2, d1, d2, d3, *p;

//point and derivativ on NURBS curve
	get_point_on_sply( (lpSPLY_DAT)sply_dat, X[0], &p1, 0 );
	get_point_on_sply( (lpSPLY_DAT)sply_dat, X[0], &d1, 1 );

//parametrical plane is r=p+(p1-p)*lambda+(p2-p)*nu;
	p=(D_POINT*)plane;

//derivativ by first plane parameter
	d2.x = p[1].x - p[0].x;
	d2.y = p[1].y - p[0].y;
	d2.z = p[1].z - p[0].z;

//derivativ by second plane parameter
	d3.x = p[2].x - p[0].x;
	d3.y = p[2].y - p[0].y;
	d3.z = p[2].z - p[0].z;

//point on plane
	p2.x = p[0].x + d2.x*X[1] + d3.x*X[2];
	p2.y = p[0].y + d2.y*X[1] + d3.y*X[2];
	p2.z = p[0].z + d2.z*X[1] + d3.z*X[2];

	G[0] = 		 min_derivativ( &p1, &p2, &d1 );
	G[1] = -1.*min_derivativ( &p1, &p2, &d2 );
	G[2] = -1.*min_derivativ( &p1, &p2, &d3 );
	return( sqrt_derivates( G, 3 ) );
}

/***********************************************************
* functions for NURBScurve-to-NURBScurve distance minimization
*/
sgFloat function_3( void  *sply_dat1, void  *sply_dat2, sgFloat X[] ){
	D_POINT p1, p2;

	get_point_on_sply( (lpSPLY_DAT)sply_dat1, X[0], &p1, 0 );
	get_point_on_sply( (lpSPLY_DAT)sply_dat2, X[1], &p2, 0 );
	return( min_function( &p1, &p2 ) );
}

sgFloat derivativs_3( void  *sply_dat1, void  *sply_dat2, sgFloat X[], sgFloat G[] ){
	D_POINT p1, p2, d1, d2;

	get_point_on_sply( (lpSPLY_DAT)sply_dat1, X[0], &p1, 0 );
	get_point_on_sply( (lpSPLY_DAT)sply_dat1, X[0], &d1, 1 );
	get_point_on_sply( (lpSPLY_DAT)sply_dat2, X[1], &p2, 0 );
	get_point_on_sply( (lpSPLY_DAT)sply_dat2, X[1], &d2, 1 );

	G[0] = 		 min_derivativ( &p1, &p2, &d1 );
	G[1] = -1.*min_derivativ( &p1, &p2, &d2 );
	return( sqrt_derivates( G, 2 ) );
}

/**********************************************************
* function to be minimized
*/
static sgFloat min_function( lpD_POINT p1, lpD_POINT p2 ){
	return(( p1->x - p2->x )*( p1->x - p2->x )+
				 ( p1->y - p2->y )*( p1->y - p2->y )+
				 ( p1->z - p2->z )*( p1->z - p2->z ) );
}

/**********************************************************
* derivatives
*/
static sgFloat min_derivativ( lpD_POINT p1, lpD_POINT p2, lpD_POINT d ){
	return( 2*( p1->x - p2->x )*d->x +
					2*( p1->y - p2->y )*d->y +
					2*( p1->z - p2->z )*d->z );
}

static sgFloat sqrt_derivates( sgFloat *G, short n ){
short i;
sgFloat t=0;
	for( i=0; i<n; i++ ) t += G[i]*G[i];
	return ( sqrt(t) );
}

//================================SURFACE=======================================
/***********************************************************
* functions for NURBSsurface-to-point distance minimization
*/
sgFloat function_0_0( void *srf_dat, void *point, sgFloat X[] ){
	D_POINT  p1, *p;

//point on NURBS surface
	get_point_on_surface( (lpSURF_DAT)srf_dat, X[0], X[1], &p1 );

//initial point
	p=(D_POINT*)point;

//function to be minimized
	return( min_function( &p1, p ) );
}

sgFloat derivativs_0_0( void  *srf_dat, void *point, sgFloat X[], sgFloat G[] ){
	D_POINT  p1, d1, d2, *p;

//point and derivative on NURBS surface
	get_point_on_surface( (lpSURF_DAT)srf_dat, X[0], X[1], &p1 );

  get_deriv_on_surface( (lpSURF_DAT)srf_dat, X[0], X[1], &d1, 0 );
  get_deriv_on_surface( (lpSURF_DAT)srf_dat, X[0], X[1], &d2, 0 );

	p=(D_POINT*)point;

	G[0] = min_derivativ( &p1, p, &d1 );
	G[1] = min_derivativ( &p1, p, &d2 );

	return( sqrt_derivates( G, 2 ) );
}
