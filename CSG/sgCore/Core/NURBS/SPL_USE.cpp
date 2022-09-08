#include "../sg.h"

static sgFloat calc_sply_step(lpSPLY_DAT sply_dat, sgFloat t, sgFloat sigma);
static void set_interval_data(lpSPLY_DAT sply_dat, lpD_POINT p);
static short get_close_interval(lpSPLY_DAT sply_dat);

static short    int_flag;       //  (//)(1/0/-1)
static short    first_interval; //   
static short    last_interval;  //   
static short    interval;       //   
static short    num_int_p;      //    
static short    cur_int_p;      //    
static sgFloat 	s_tolerance;    //  
static sgFloat 	step_int;       //  
//static sgFloat min_step_size;  //   
static sgFloat 	length;  				//  

/**********************************************************
*        
* deriv =0 - get point with parameter t
*       =1 - first derivates into point with parameter t
*       =2 - second derivates into point with parameter t
*/
void get_point_on_sply(lpSPLY_DAT sply_dat, sgFloat t,
											 lpD_POINT p, short deriv){
short   degree, nnn;
//sgFloat  weight;
//D_POINT point, point1;

p->x=p->y=p->z=0.;

if (t < eps_n) t = 0.;
if (t > 1 - eps_n) t = 1.;

if (-eps_n >= t || t >= 1 + eps_n) {
	memset(p, 0, sizeof(D_POINT));
	return;
}
nnn = (sply_dat->sdtype&SPL_INT) ? sply_dat->numk : sply_dat->nump;
if( nnn <= 2 ){
	p->x = sply_dat->knots[0][0]*(1-t)+sply_dat->knots[1][0]*t;
	p->y = sply_dat->knots[0][1]*(1-t)+sply_dat->knots[1][1]*t;
	p->z = sply_dat->knots[0][2]*(1-t)+sply_dat->knots[1][2]*t;
	return;
}
degree = sply_dat->degree;
while( nnn < degree + 1 ) degree--;
if( degree <= 0 ) return;

if( sply_dat->sdtype & SPL_INT  ){ // short spline
	switch( deriv ){
		case 0: //point with parameter t
			Calculate_Point( sply_dat, degree, t, p);
			break;
		case 1:     //firs,second derivates into point with parameter t
		case 2:
			Calculate_Deriv( sply_dat, degree, t, p, deriv);
			break;
	}
}else{ // APPR spline with weight
	switch( deriv ){
		case 0://point with parameter t
			Calculate_Weight_Point(sply_dat, degree, t, p);
			break;
		case 1://first derivates into point with parameter t
			Calculate_Weight_Deriv(sply_dat, degree, t, p, deriv);
			break;
		case 2://second derivates into point with parameter t
			break;
	}
}
return;
}

void get_first_sply_point(lpSPLY_DAT sply_dat, sgFloat sigma, BOOL back,
													lpD_POINT p){
short    i;
	sigma = 0.;	// : ,       !
//	if( sply_dat->degree == 1 ) sigma = 0.;

	if( sigma > 0. ){
		s_tolerance = sigma;  //  
		length = 0;
		for( i=0; i<sply_dat->numk-1; i++ )
			length += dpoint_distance( (lpD_POINT)&sply_dat->knots[i],
														(lpD_POINT)&sply_dat->knots[i+1] );
		int_flag =  ( back ) ? -1 : 1; // /
		step_int =  ( back ) ? 1. : 0.; //   
		cur_int_p = ( back ) ? sply_dat->numk-2 : 1; //   
		memcpy(p, &sply_dat->knots[ cur_int_p-int_flag], sizeof(D_POINT));
//		get_point_on_sply(sply_dat, step_int, p, 0 ); //    
//		min_step_size = 1./(sply_dat->numk*INTERNAL_SPLINE_POINT);
	}	else {
		i = sply_dat->numk - 1;
		get_first_sply_segment_point(sply_dat, sigma, (back)?i:0, (back)?0:i, p);
	}
}

void get_first_sply_segment_point(lpSPLY_DAT sply_dat, sgFloat sigma,
																	short bknot, short eknot, lpD_POINT p){
	sigma = 0.;	// : ,       !
	if(sply_dat->sdtype&SPL_CLOSE){
		if(abs(eknot - bknot) + 1 >= sply_dat->numk){
			bknot = 0;
			eknot = sply_dat->numk - 1;
		}
	}else{
		if(bknot < 0) bknot = 0;
		if(bknot > 0 && bknot > sply_dat->numk - 1) bknot = sply_dat->numk - 1;
		if(eknot < 0) eknot = 0;
		if(eknot > 0 && eknot > sply_dat->numk - 1) eknot = sply_dat->numk - 1;
	}
	first_interval = min(bknot, eknot);
	last_interval  = max(bknot, eknot) - 1;
	int_flag       = isg(eknot - bknot);
	s_tolerance    = sigma;

	if(!int_flag)	
		memcpy(p, &sply_dat->knots[bknot], sizeof(D_POINT));
	else
	{
		interval = (int_flag > 0) ? first_interval - 1 : last_interval + 1;
		set_interval_data(sply_dat, p);
	}
}

BOOL get_next_sply_point(lpSPLY_DAT sply_dat, lpD_POINT p){
BOOL   knot;
	if( s_tolerance > 0. )
		return get_next_sply_point_tolerance( sply_dat, p, &knot );
	else return get_next_sply_point_and_knot(sply_dat, p, &knot);
}

BOOL get_next_sply_point_tolerance(lpSPLY_DAT sply_dat, lpD_POINT p, BOOL *knot ){
	sgFloat t;
//------------------------------------------------------------------------------
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!	
	return get_next_sply_point_and_knot(sply_dat, p, knot);
//------------------------------------------------------------------------------
	if( int_flag > 0 && step_int >= 1. ) return FALSE; //  
	if( int_flag < 0 && step_int <= 0. ) return FALSE; //  
	*knot = FALSE;
	if( sply_dat->degree == 1 ){
		step_int = sply_dat->u[cur_int_p];
		memcpy(p, &sply_dat->knots[cur_int_p], sizeof(D_POINT));
		*knot = TRUE;
		cur_int_p += int_flag;
		return TRUE;
	}
	t = calc_sply_step( sply_dat, step_int, s_tolerance);
	t /= length;
//	if( t > min_step_size ) t=min_step_size;
	t = step_int + int_flag*t;
	if( t < eps_n )    t = 0.;
	if( t > 1.-eps_n ) t = 1.;
	if( fabs( t-sply_dat->u[cur_int_p] )< eps_n ||
						(t>sply_dat->u[cur_int_p]&&int_flag==1) ||
						(t<sply_dat->u[cur_int_p]&&int_flag==-1) ){ // 
		step_int = sply_dat->u[cur_int_p];
		memcpy(p, &sply_dat->knots[cur_int_p], sizeof(D_POINT));
		*knot = TRUE;
		cur_int_p += int_flag;
	}else{
		step_int = t;
		get_point_on_sply(sply_dat, step_int, p, 0 ); //   
	}
	return TRUE;
}

BOOL get_next_sply_point_and_knot(lpSPLY_DAT sply_dat, lpD_POINT p, BOOL *knot){
short    i, ic;

	if(!int_flag) return FALSE;  //    
	if(++cur_int_p > num_int_p){
		set_interval_data(sply_dat, p);
		*knot = TRUE;
	}else{
		i = (int_flag > 0) ? cur_int_p : num_int_p - cur_int_p + 1;
		ic = get_close_interval(sply_dat);
		get_point_on_sply(sply_dat, sply_dat->u[ic] + i*step_int, p, FALSE);
		*knot = FALSE;
	}
	return TRUE;
}

static void set_interval_data(lpSPLY_DAT sply_dat, lpD_POINT p){
short ic;
sgFloat t;

	interval += int_flag;
	ic = get_close_interval(sply_dat);
	memcpy(p, &sply_dat->knots[(int_flag > 0) ? ic : ic + 1], sizeof(D_POINT));

	if(interval > last_interval || interval < first_interval)
		int_flag = 0;
	else{
//		if(s_tolerance > 0.) num_int_p = calc_sply_step(sply_dat, ic, s_tolerance);
//		else
		if( sply_dat->numk > 2 ){
/*			while(1){
				if( (t=sply_dat->u[ic+1] - sply_dat->u[ic]) > eps_n) break;
				interval += int_flag;
				ic = get_close_interval(sply_dat);
				memcpy(p, &sply_dat->knots[(int_flag > 0) ? ic : ic + 1], sizeof(D_POINT));

				if(interval > last_interval || interval < first_interval){
					int_flag = 0;
					return;
				}
			}
*/
			t=sply_dat->u[ic+1] - sply_dat->u[ic];
			num_int_p = INTERNAL_SPLINE_POINT+(short)(t*100);
			step_int = t/(num_int_p +1);
		}	else {
			num_int_p = INTERNAL_SPLINE_POINT;
			step_int = 1./(num_int_p +1);
		}
		cur_int_p = 0;
	}
}

static sgFloat calc_sply_step(lpSPLY_DAT sply_dat, sgFloat t, sgFloat sigma){
sgFloat  ro, r1, r2, r3;
D_POINT p_1, p_2;

	get_point_on_sply( sply_dat, t, &p_1, 1 );
	get_point_on_sply( sply_dat, t, &p_2, 2 );
//ro( )= 1/
	r1 = dskalar_product( &p_1, &p_1 );
	r2 = dskalar_product( &p_2, &p_2 );
	r3 = dskalar_product( &p_1, &p_2 );
	ro = r1*sqrt(r1);
	r1 = r1*r2 - r3*r3;
	if( r1 <= eps_n ) return ( 2*sigma );
	else{
		ro= ro/sqrt( r1 );
		if( (r1 = 2*ro-sigma ) <= eps_n ) return ( sqrt(8*sigma*ro) );
		else return ( sqrt(4*sigma*(2*ro-sigma))	);
	}
}

/**********************************************************
*      
*/
static short get_close_interval(lpSPLY_DAT sply_dat){
short num;
	if(sply_dat->sdtype & SPL_CLOSE){
		num = sply_dat->numk - 1;
		if(interval < 0)    return interval + num;
		if(interval >= num) return interval - num;
	}
	return interval;
}
