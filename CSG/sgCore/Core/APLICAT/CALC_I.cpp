#include "../sg.h"

typedef struct
{
	BOOL    type[X_NUM];
	sgFloat  my_value[X_NUM];
	D_POINT c_point;
	sgFloat  Density;
}CALC_DATA;
typedef CALC_DATA * lpCALC_DATA;

// my internal function
static void calc_surface_square ( lpNPW np, lpCALC_DATA my );
static BOOL	calc_mech_aplication( lpNPW np, lpCALC_DATA my );
static sgFloat calc_tetra_volume ( lpD_POINT point_a, lpD_POINT point_b,
																	lpD_POINT point_c, lpD_POINT point_d );
static OSCAN_COD calc_scan      ( hOBJ hobj,lpSCAN_CONTROL lpsc);
static OSCAN_COD test_point_scan( hOBJ hobj, lpSCAN_CONTROL lpsc);
/**********************************************************
* calculate characteristics for group of objects
* group - list of objects
* flag  - characteristics will be calculated
*/
void Calculate_Group_M_In( lpLISTH group,
										 sgFloat *Density, sgFloat M_to_UNIT,
										 BOOL *flag, sgFloat *value ){
	short    i, j;
	sgFloat value_object[X_NUM];
	hOBJ   object;

	for( i=0; i<X_NUM; i++ ) value[i] = 0.;
	object = group->hhead;  // list begining
	j=0;
	while( object != NULL ){      // while objects exist into list
// calculate characteristics for one object
		Calculate_M_In( object, Density[j], M_to_UNIT, flag, value_object );
// summary characteristics for objects to obtain common charact.
		for( i=0; i<X_NUM; i++ ) value[i] += value_object[i];
// get the next element from the list
		get_next_item_z( SEL_LIST, object, &object );
		j++;
	}
	if( flag[X_MASS_CENTER_X] ) {
		value[X_MASS_CENTER_X] /= value[X_MASS];
		value[X_MASS_CENTER_Y] /= value[X_MASS];
		value[X_MASS_CENTER_Z] /= value[X_MASS];
	}
}

/**********************************************************
* calculate characteristics for object
* type - characteristics will be calculated
*	  X_SQUARE  - body surface square
*		X_VOLUME  - body volume
*		X_MASS    - body mass
*   X_WEIGHT  - body weight
*   X_STATIC_MOMENT_XY, YZ, ZX   - static moment
*   X_IM_OX,  X_IM_OY,  X_IM_OZ  - in. moment around axis
*   X_IM_XY,  X_IM_YZ,  X_IM_XZ  - in. moment around plane
*   X_CIM_XY, X_CIM_YZ, X_CIM_XZ - centr. in. moment
*   X_MASS_CENTER_X, Y, Z        - coordinate of mass center
*   Density                      - body density
*   M_to_UNIT                    - coefficient of translation from current to meter
*/
void Calculate_M_In( hOBJ object, sgFloat Density, sgFloat M_to_UNIT,
										 BOOL *flag, sgFloat *value ){
	short           i;
	SCAN_CONTROL  sc;
	CALC_DATA     calc_data;
	sgFloat        G,  //acceleration of free fall
								size1, size2, size3;
	size1 = M_to_UNIT;
	size2 = size1*size1;
	size3 = size2*size1;

	G = 9.80665;
	calc_data.Density	  = Density; //*size3;

//--------------------------------------------------------->
	for( i=0; i<X_NUM; i++ ){
		calc_data.type[i]     = flag[i];
		calc_data.my_value[i] = 0.;
	}
	calc_data.c_point.x =	calc_data.c_point.y = calc_data.c_point.z = 0.;

	if( flag[X_MASS_CENTER_X] ) calc_data.type[X_MASS] = TRUE;
//	if( flag[X_IM_ABIT_AXIS]  )
//		calc_data.type[X_IM_OX]  = calc_data.type[X_IM_OY]  =
//		calc_data.type[X_IM_OX]  = calc_data.type[X_CIM_XY] =
//		calc_data.type[X_CIM_YZ] = calc_data.type[X_CIM_ZX] = TRUE;

	init_scan(&sc);
	sc.user_geo_scan = test_point_scan;
	sc.data 				 = &calc_data;
//	if ( !o_scan(object,&sc) )  return FALSE;
	o_scan(object,&sc);

//calculate center point of geometrical mass center
	calc_data.c_point.x /= calc_data.my_value[0];
	calc_data.c_point.y /= calc_data.my_value[0];
	calc_data.c_point.z /= calc_data.my_value[0];
	calc_data.my_value[0] = 0.;

	sc.user_geo_scan = calc_scan;
//	if ( !o_scan(object,&sc) )  return FALSE;
	o_scan(object,&sc);

//calculate body weight
	if( calc_data.type[X_WEIGHT] )
		calc_data.my_value[X_WEIGHT] = calc_data.my_value[X_MASS] * G;

//calculate mass center of the body
	if( calc_data.type[X_MASS_CENTER_X] ){
		calc_data.my_value[X_MASS_CENTER_X] *= size3;
		calc_data.my_value[X_MASS_CENTER_Y] *= size3;
		calc_data.my_value[X_MASS_CENTER_Z] *= size3;
	}
//--------------------------->
// in. moment relatively arbitrary axis
//  calc_data->my_value[X_IM_ABIT_AXIS] =
//		calc_data->my_value[X_IM_OX]*cos(ALFA)*cos(ALFA) +
//	  calc_data->my_value[X_IM_OY]*cos(BETA)*cos(BETA) +
//	  calc_data->my_value[X_IM_OZ]*cos(GAMMA)*cos(GAMMA) -
//	  2*calc_data->my_value[X_CIM_YZ]*cos(BETA)*cos(GAMMA) -
//	  2*calc_data->my_value[X_CIM_ZX]*cos(ALFA)*cos(GAMMA) -
//  	2*calc_data->my_value[X_CIM_XY]*cos(BETA)*cos(ALFA) ;
//
//--------------------------->
// correct obtaining resalts to mm
			 value[X_SQUARE] = calc_data.my_value[X_SQUARE]*size2;
	for( i=X_VOLUME; i<=X_WEIGHT; i++ )
			 value[i] = calc_data.my_value[i]*size3;
	for( i=X_STATIC_MOMENT_XY; i<X_IM_OX; i++ )
			 value[i] = calc_data.my_value[i]*size3*M_to_UNIT;
	for( i=X_IM_OX; i<=X_CIM_ZX; i++ )
			 value[i] = calc_data.my_value[i]*size3*size2;
	for( i=X_MASS_CENTER_X; i<X_NUM; i++ )
			 value[i] = calc_data.my_value[i];//*M_to_UNIT;
}

#pragma argsused
static OSCAN_COD test_point_scan( hOBJ hobj, lpSCAN_CONTROL lpsc ){
	int					i;
	lpCALC_DATA dc = (lpCALC_DATA)lpsc->data;

//calculate point of geometrical mass center
	for( i=1; i<=npwg->nov; i++ ){
//proected coordinates
		if ( !o_hcncrd(lpsc->m,&npwg->v[i],&npwg->v[i])) return OSFALSE;
		dc->c_point.x += npwg->v[i].x;
		dc->c_point.y += npwg->v[i].y;
		dc->c_point.z += npwg->v[i].z;
	}
	dc->my_value[0] += (sgFloat)npwg->nov;
	return OSTRUE;
}

#pragma argsused
static OSCAN_COD calc_scan( hOBJ hobj, lpSCAN_CONTROL lpsc ){
	int					i, ipr;
	lpCALC_DATA dc = (lpCALC_DATA)lpsc->data;

	for( i=1; i<=npwg->nov; i++ ){
	//proected coordinates
		if ( !o_hcncrd(lpsc->m,&npwg->v[i],&npwg->v[i])) return OSFALSE;
	}
//calculate simple body surface square
	if( dc->type[X_SQUARE] )	calc_surface_square( npwg, dc );

//calculate static moments
	for( ipr=0, i=X_VOLUME; i<X_NUM; i++ )
		if( dc->type[i] ) { ipr = 1; break; }
	if( ipr	)	calc_mech_aplication( npwg, dc );

	return OSTRUE;
}

/**********************************************************
* calculate simple body surface square
*/
static void calc_surface_square( lpNPW np, lpCALC_DATA my ){
	short     face, countour, first_edge, edge, vertex_0;
	D_POINT point_a, point_b, point_c, point_d;

	for( face = 1; face <= np->nof; face++ ){ //face loop
		point_c.x = point_c.y = point_c.z = 0.;
		countour = np->f[face].fc; // take first countour of face
		while (countour){           // countour loop
			first_edge = edge = np->c[countour].fe;
			vertex_0 = BE(edge,np);
			dpoint_sub( &np->v[EE(edge,np)], &np->v[vertex_0], &point_a );
			edge = SL(edge,np);      //take the next edge
			do {                     //vertex loop
				dpoint_sub( &np->v[EE(edge,np)], &np->v[vertex_0], &point_b );
				dvector_product( &point_a, &point_b, &point_d );
				dpoint_add( &point_c, &point_d, &point_c );
				point_a = point_b;
				edge = SL(edge,np);    //take the next edge
			} while (edge != first_edge);
			countour = np->c[countour].nc; //take next countour
		}
		my->my_value[X_SQUARE] += sqrt( dskalar_product( &point_c, &point_c ) )/2.;
	}
}

/**********************************************************
* calculate mecanical applications
*/
static BOOL	calc_mech_aplication( lpNPW np, lpCALC_DATA my){
	short     face, countour, first_edge, edge;
	sgFloat  Volume, Teta;
	D_POINT point_a, point_b, point_c, point_d;

	for (face = 1; face <= np->nof; face++){
		countour = np->f[face].fc;
		while (countour){
			first_edge = edge = np->c[countour].fe;
			point_a = np->v[BE(edge,np)];
//			if ( !o_hcncrd( m, &point_a, &point_a )) return FALSE;
			edge    = SL(edge,np);
			point_b = np->v[BE(edge,np)];
//			if ( !o_hcncrd( m, &point_b, &point_b )) return FALSE;
			edge = SL(edge,np);
			do {
				point_c = np->v[BE(edge,np)];
//				if ( !o_hcncrd( m, &point_c, &point_c )) return FALSE;

//calculate geometrical mass center of tetra
				point_d.x = ( point_a.x + point_b.x + point_c.x + my->c_point.x )/4.;
				point_d.y = ( point_a.y + point_b.y + point_c.y + my->c_point.y )/4.;
				point_d.z = ( point_a.z + point_b.z + point_c.z + my->c_point.z )/4.;

//calculate volume of tetra
				Volume = calc_tetra_volume( &point_a, &point_b,
																		&point_c, &my->c_point );

//calculate volume
				if( my->type[X_VOLUME] ) my->my_value[ X_VOLUME ] += Volume;

				Teta = Volume*my->Density;

//calculate static moments
				if( my->type[X_STATIC_MOMENT_XY] )
					my->my_value[X_STATIC_MOMENT_XY] += point_d.z*Teta;
				if( my->type[X_STATIC_MOMENT_YZ] )
					my->my_value[X_STATIC_MOMENT_YZ] += point_d.x*Teta;
				if( my->type[X_STATIC_MOMENT_ZX] )
					my->my_value[X_STATIC_MOMENT_ZX] += point_d.y*Teta;

//calculate mass center
				if( my->type[X_MASS] || my->type[X_WEIGHT] )
						my->my_value[X_MASS] += Teta;

				if( my->type[X_MASS_CENTER_X] ){
					my->my_value[X_MASS_CENTER_X] += point_d.x*Teta;
					my->my_value[X_MASS_CENTER_Y] += point_d.y*Teta;
					my->my_value[X_MASS_CENTER_Z] += point_d.z*Teta;
				}
//calculate axis inert. moment
				if( my->type[X_IM_OX] ) my->my_value[X_IM_OX] +=
					( point_d.y*point_d.y + point_d.z*point_d.z )*Teta;
				if( my->type[X_IM_OY] ) my->my_value[X_IM_OY] +=
					( point_d.x*point_d.x + point_d.z*point_d.z )*Teta;
				if( my->type[X_IM_OZ] ) my->my_value[X_IM_OZ] +=
					( point_d.x*point_d.x + point_d.y*point_d.y )*Teta;

//calculate plane inert. moment
				if( my->type[X_IM_XY] ) my->my_value[X_IM_XY] += point_d.z*point_d.z*Teta;
				if( my->type[X_IM_YZ] ) my->my_value[X_IM_YZ] += point_d.x*point_d.x*Teta;
				if( my->type[X_IM_ZX] ) my->my_value[X_IM_ZX] += point_d.y*point_d.y*Teta;

//calculate center. inert. moment
				if( my->type[X_CIM_XY] ) my->my_value[X_CIM_XY] += point_d.x*point_d.y*Teta;
				if( my->type[X_CIM_YZ] ) my->my_value[X_CIM_YZ] += point_d.y*point_d.z*Teta;
				if( my->type[X_CIM_ZX] ) my->my_value[X_CIM_ZX] += point_d.z*point_d.x*Teta;
//--------------------------->
// in. moment relatively arbitrary axis
//  my->my_value[X_IM_ABIT_AXIS] =
//
//--------------------------->

				edge = SL(edge,np);
				point_b = point_c;
			} while (edge != first_edge);
			countour = np->c[countour].nc;
		}
	}
	return TRUE;
}

/**********************************************************
* calculate volume of tetra
*/
static sgFloat calc_tetra_volume( lpD_POINT point_a, lpD_POINT point_b,
																 lpD_POINT point_c, lpD_POINT point_d ){
	D_POINT point_ad, point_bd, point_cd;

	dpoint_sub( point_a, point_d, &point_ad );
	dpoint_sub( point_b, point_d, &point_bd );
	dpoint_sub( point_c, point_d, &point_cd );
	return(	(point_ad.x*point_bd.y*point_cd.z +
					 point_ad.z*point_bd.x*point_cd.y +
					 point_ad.y*point_bd.z*point_cd.x -
					 point_ad.z*point_bd.y*point_cd.x -
					 point_ad.y*point_bd.x*point_cd.z -
					 point_ad.x*point_bd.z*point_cd.y)*0.16666667 );
}
