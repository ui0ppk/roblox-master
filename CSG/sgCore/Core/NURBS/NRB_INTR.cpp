#include "../sg.h"
/**********************************************************
* obtain NURBScurve-to-point distance
* spl   - NURBS curve
* point - point for curve-to-point distance
* t     - point or point projection lie on curve with parameter t
* dist  - distance between point and its projection
*/
BOOL Spline_By_Point( lpSPLY_DAT spl, void  *point, sgFloat *t, sgFloat *dist ){
sgFloat 		 X[1];
D_POINT  	 node;

	X[0]=*t;
	if( (*dist = Fletchjer_Paur( function_0, derivativs_0,
												(void  *)spl, (void  *)point,
												 X, 1, 50 )) == -1 ) return FALSE;
	*t=X[0];
//	if( fabs(*dist) > eps_d ){ // point out of curve
		if( X[0] < -eps_n ) *t=0;
		if( X[0] > 1.+eps_n ) *t=1.;
		get_point_on_sply( spl, *t, &node, 0 );
		*dist= dpoint_distance( (D_POINT *)point, &node );
//	}
	return TRUE;
}

/**********************************************************
* obtain NURBScurve-to-plane distance
* spl   - NURBS curve
* plane - points on plane
* t     - point lie on curve with parameter t
* dist  - distance between point and its projection
*/
BOOL Spline_By_Plane( lpSPLY_DAT spl, void  *plane,
											sgFloat *t, sgFloat *bound, sgFloat *dist ){
sgFloat 	X[3];

//found parameter for the begining point on curve
	X[0]=*t;
//found parameter for the begining point on plane
//the plane parametrization is made trought beginning point =>
	X[1]=X[2]=0.;

//	if(!Spline_By_Point( spl, &point[0], &X[0], dist )) return FALSE;

	if( (*dist = Fletchjer_Paur( function_2, derivativs_2,
												(void  *)spl, (void  *)plane,
												 X, 3, 400 )) == -1 ) return FALSE;

	if(	( bound[0] <= X[0] && X[0] < bound[1] ) ||
			( X[0] == 1. && bound[1] == 1. )) *t=X[0];
	else *t=-1.;
	return TRUE;
}

/**********************************************************
* obtain NURBS curve-to-line distance
* spl   - NURBS curve
* line  - points on line
* t     - point or point projection lie on curve with parameter t
* dist  - distance between point and its projection
* span  - number of the interval intersection found on
*/
BOOL Spline_By_Line( lpSPLY_DAT spl, void  *line,
										 sgFloat *t_sply, sgFloat *t_line,
										 sgFloat *bound,  sgFloat *dist ){
sgFloat    X[2];
D_POINT   *point;

//found parameter for the begining point on curve and line
	X[0]=*t_sply;
	X[1]=*t_line;
	point=(D_POINT*)line;

	if( (*dist = Fletchjer_Paur( function_1, derivativs_1,
												(void  *)spl, (void  *)point,
												 X, 2, 400 )) == -1 ) return FALSE;

	if(	( bound[0] <= X[0] && X[0] < bound[1] ) ||
			( X[0] == 1. && bound[1] == 1. )) *t_sply=X[0];
	else *t_sply=-1.;
	return TRUE;
}

/**********************************************************
* obtain sply-by-sply intersections
* sply_dat1      - data for first spline
* sply_dat2      - data for second spline
* intersect_list - list of intersection points
*/
BOOL Spline_By_Spline( lpSPLY_DAT spl1, void  *spl2,
											 sgFloat *t1,      sgFloat *t2,
											 sgFloat *bound1,  sgFloat *bound2,
											 sgFloat *dist	){
sgFloat     X[2];

//found parameter for the begining point on the first curve
	X[0]=*t1;
	X[1]=*t2;

	if( (*dist = Fletchjer_Paur( function_3, derivativs_3,
												(void  *)spl1, (void  *)spl2,
												 X, 2, 400 )) == -1 ) return FALSE;

	if(	( bound1[0] <= X[0] && X[0] < bound1[1] ) ||
			( X[0] == 1. && bound1[1] == 1. )) *t1=X[0];
//	else 																	 *t1=-1.;
	if(	( bound2[0] <= X[1] && X[1] < bound2[1] ) ||
			( X[1] == 1. && bound2[1] == 1. )) *t2=X[1];
//	else                                   *t2=-1.;

// *t1=X[0]; *t2=X[1];
	return TRUE;
}

/**********************************************************
* obtain NURBSsurface-to-point distance
* srf   - NURBS surface
* point - point for curve-to-point distance
* tu, tv- point or point projection lie on surface with parameter (tu,tv)
* dist  - distance between point and its projection
*/
BOOL Surface_By_Point( lpSURF_DAT srf, void *point, sgFloat *tu, sgFloat *tv, sgFloat *dist ){
sgFloat 		 X[2];
D_POINT  	 node;

	X[0]=*tu;
  X[1]=*tv;
	if( (*dist = Fletchjer_Paur( function_0_0, derivativs_0_0,
												(void  *)srf, (void  *)point,
												 X, 2, 50 )) == -1 ) return FALSE;
	*tu=X[0];
  *tv=X[1];
//	if( fabs(*dist) > eps_d ){ // point out of curve
		if( X[0] < -eps_n )   *tu=0;
		if( X[0] > 1.+eps_n ) *tu=1.;
		if( X[1] < -eps_n )   *tv=0;
		if( X[1] > 1.+eps_n ) *tv=1.;
		if( !get_point_on_surface( srf, *tu, *tv, &node ) ) return FALSE;
		*dist= dpoint_distance( (D_POINT *)point, &node );
//	}
	return TRUE;
}
