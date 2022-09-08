#include "../sg.h"

/**********************************************************
* Spline length foundation
*/
sgFloat spl_length( lpSPLY_DAT sply_dat ){
	sgFloat  S=0, sigma = c_h_tolerance;
	D_POINT p1, p2;
	short			i;

	if( sply_dat->degree == 1 ){
		for( i=0; i<sply_dat->numk-1; i++ )
			S += dpoint_distance( (lpD_POINT)&sply_dat->knots[i],
														(lpD_POINT)&sply_dat->knots[i+1] );
	} else {
		get_first_sply_point( sply_dat, sigma, FALSE, &p1);
//		while( get_next_sply_point_tolerance( sply_dat, &p2, &knot ) ){
		while( get_next_sply_point( sply_dat, &p2 ) ){
			S += dpoint_distance( &p1, &p2 );
			p1.x=p2.x; p1.y=p2.y; p1.z=p2.z;
		}
	}
	return S;
}

/**********************************************************
* Spline length foundation
*/
sgFloat spl_length_P( lpSPLY_DAT sply_dat ){
	short		i;
	sgFloat  S=0;

	for( i=0; i<sply_dat->nump-1; i++ )	S += dpoint_distance_4dl( &sply_dat->P[i], &sply_dat->P[i+1] );
	return S;
}
