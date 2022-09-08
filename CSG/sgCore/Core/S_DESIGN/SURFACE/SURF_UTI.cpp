#include "../../sg.h"

BOOL begin_use_surf(lpGEO_SURFACE geo_surf, lpSURF_DAT surf_dat){
short 	 i, j,k, number, type;
lpW_NODE p;
lpSNODE  d;
W_NODE	 node;
int     tmp_int;

//   
memset(surf_dat, 0, sizeof(SURF_DAT));

if(geo_surf->type & (NSURF_NURBS)){
	if ( geo_surf->type & NSURF_APPR ) {// 
//
		type = SURF_APPR;

// ---------------->>>>>>>>>     <<<<<<<<<<-------------
//    
		if( !init_vdim( &surf_dat->knots.vdim, sizeof(D_POINT))) goto err; //Coordinates of knots
		if( !init_vdim( &surf_dat->P.vdim, sizeof(W_NODE))) goto err; //Coordinates of knots
		surf_dat->knots.n=surf_dat->P.n=geo_surf->numpu;
		surf_dat->knots.m=surf_dat->P.m=geo_surf->numpv;

//    
		if((surf_dat->u = (sgFloat*)SGMalloc(geo_surf->numpu*sizeof(sgFloat))) == NULL) goto err;
		surf_dat->allocSizeFor_u = geo_surf->numpu;

		if((surf_dat->v = (sgFloat*)SGMalloc(geo_surf->numpv*sizeof(sgFloat))) == NULL) goto err;
		surf_dat->allocSizeFor_v = geo_surf->numpv;

//------------------------------------------------------------------------------
//   
		p = (lpW_NODE)geo_surf->hpoint;
    d = (lpSNODE)geo_surf->hderivates;
		k=geo_surf->numpu*geo_surf->numpv;

//      
		for( i=0; i<k; i++ ) if( !add_elem(&surf_dat->knots.vdim, &p[i] )) goto err;

	 	for( number=0, j=0; j<surf_dat->P.m; j++, number +=surf_dat->P.n  ){
	 		for( i=0; i<surf_dat->P.n; i++ ){
       	Create_4D_From_3D( &node, (sgFloat *)&p[number+i], d[number+i].p[1] );
	 			add_elem(&surf_dat->P.vdim, &node);
        surf_dat->u[i]=d[number+i].p[0]; //  
	      surf_dat->v[j]=d[number+i].p[2];
     	}
   	}
//  
			for( i=0; i<surf_dat->P.n-1; i++ )
        if( dpoint_distance((lpD_POINT)&p[i], (lpD_POINT)&p[i+1] ) > eps_d ) goto met1;
      surf_dat->poles  = SURF_POLEU0;
met1:
			number=surf_dat->P.n *(surf_dat->P.m-1);
			for( i=0; i<surf_dat->P.n-1; i++ )
        if( dpoint_distance((lpD_POINT)&p[number+i], (lpD_POINT)&p[number+i+1] ) > eps_d ) goto met2;
		tmp_int = (int)surf_dat->poles;
		tmp_int |= SURF_POLEU1;
		surf_dat->poles = (SURF_POLES)tmp_int;
		//surf_dat->poles |= SURF_POLEU1;
met2:
 			for( number=0, j=0; j<surf_dat->P.m-1; j++, number +=surf_dat->P.n  )
        if( dpoint_distance((lpD_POINT)&p[number], (lpD_POINT)&p[number+surf_dat->P.n] ) > eps_d ) goto met3;
		tmp_int = (int)surf_dat->poles;
		tmp_int |= SURF_POLEV0;
		surf_dat->poles = (SURF_POLES)tmp_int;
        //surf_dat->poles |= SURF_POLEV0;
met3:
 			for( number=surf_dat->P.n, j=0; j<surf_dat->P.m-1; j++, number +=surf_dat->P.n  )
        if( dpoint_distance((lpD_POINT)&p[number], (lpD_POINT)&p[number+surf_dat->P.n] ) > eps_d ) goto met4;
		tmp_int = (int)surf_dat->poles;
		tmp_int |= SURF_POLEV1;
		surf_dat->poles = (SURF_POLES)tmp_int;
        //surf_dat->poles |= SURF_POLEV1;
met4:

// ---------------->>>>>>>>>    <<<<<<<<<<-------------
//    
		surf_dat->num_u = geo_surf->numpu+geo_surf->degreeu+1;
		if((surf_dat->U = (sgFloat*)SGMalloc(surf_dat->num_u*sizeof(sgFloat))) == NULL) goto err;
		surf_dat->allocSizeFor_U = surf_dat->num_u;

		surf_dat->num_v = geo_surf->numpv+geo_surf->degreev+1;
		if((surf_dat->V = (sgFloat*)SGMalloc(surf_dat->num_v*sizeof(sgFloat))) == NULL) goto err;
		surf_dat->allocSizeFor_V = surf_dat->num_v;

		memcpy(surf_dat->U, geo_surf->hparam, surf_dat->num_u*sizeof(sgFloat));

		memcpy(surf_dat->V, (char*)geo_surf->hparam + surf_dat->num_u*sizeof(sgFloat), surf_dat->num_v*sizeof(sgFloat));
	}else{ // 
	}

  surf_dat->orient = geo_surf->orient;
	if( geo_surf->type & NSURF_CLOSEU )	surf_dat->sdtype = SURF_CLOSEU;
	if( geo_surf->type & NSURF_CLOSEV )	
	{
		tmp_int = (int)surf_dat->sdtype;
		tmp_int |= SURF_CLOSEV;
		surf_dat->sdtype = (SURF_TYPE_IN)tmp_int;//|= SURF_CLOSEV;
	}
//   
	if( !init_surf_dat( SURF_GEO, geo_surf->degreeu, geo_surf->degreev,
											type, surf_dat)) goto err2;
}else{// 
}
	return TRUE;

err:
	nurbs_handler_err(SURF_NO_MEMORY);
err2:
	free_surf_data(surf_dat);
	return FALSE;
}

/**********************************************************
*   
*  ,   surf_dat
*/
void end_use_surf(lpSURF_DAT surf_dat){
	free_surf_data(surf_dat);
}

/**********************************************************
* Inisialzation surface arrays
* surf_dat	- description of surface
* type 			- type of surface
* p					- degree on u-direction
* q					- degree on v-direction
*/
BOOL init_surf_dat( char sdtype, short p, short q, short type, lpSURF_DAT surf_dat )
{
short k, num_point_u, num_point_v;
int tmp_int;

if( sdtype & SURF_GEO ){ // ,       
													//      
	tmp_int = (int)surf_dat->sdtype;
	tmp_int |= sdtype;
	tmp_int |= type;
	surf_dat->sdtype = (SURF_TYPE_IN)tmp_int;
//	surf_dat->sdtype |= sdtype;  // 
//	surf_dat->sdtype |= type;    ///

	num_point_u = surf_dat->knots.m;
	num_point_v = surf_dat->knots.n;
/*	if(sply_dat->sdtype & SPL_CLOSE){
		if( sply_dat->numd == 0 ) condition=1; // closed without derivation
		else{
		 if( sply_dat->derivates[0].num == 1 ) condition=2; //special case of closed spline
		 else condition=3; // closed with derivates
		}
	}else{
		 if( sply_dat->numd == 0 ) condition=0; // without derivation
		 else condition=2; //with derivates
	}
*/
}else{ //   .      
//memory for work structure
	memset(surf_dat, 0, sizeof(SURF_DAT));
	num_point_u=num_point_v=MAX_POINT_ON_SURFACE;
	tmp_int = (int)surf_dat->sdtype;
	tmp_int |= sdtype;
	surf_dat->sdtype = (SURF_TYPE_IN)tmp_int;
//	surf_dat->sdtype |= sdtype;  // Surface type

	if( type & SURF_APPR){
		if( !init_vdim( &surf_dat->knots.vdim, sizeof(D_POINT))) goto err; //Knot's coordinates
		if( !init_vdim( &surf_dat->P.vdim, sizeof(W_NODE)) ) goto err; //  -

		k = 2*(num_point_u+p+1);
		if((surf_dat->U = (sgFloat*)SGMalloc(k*sizeof(sgFloat))) == NULL) goto err;
		surf_dat->allocSizeFor_U = k;

		k = 2*(num_point_v+q+1);
		if((surf_dat->V = (sgFloat*)SGMalloc(k*sizeof(sgFloat))) == NULL) goto err;
		surf_dat->allocSizeFor_V = k;
	}else{//
	}
}
	surf_dat->degree_p = p;  // Surface degree in U direction
	surf_dat->degree_q = q;  // Surface degree in V direction
	tmp_int = (int)surf_dat->sdtype;
	tmp_int |= type;
	surf_dat->sdtype = (SURF_TYPE_IN)tmp_int;
//	surf_dat->sdtype |= type;
  surf_dat->orient = 0;

// Surface parameter in u, v direction
	if( (surf_dat->u = (sgFloat*)SGMalloc((num_point_u)*sizeof(sgFloat))) == NULL ) goto err;
	surf_dat->allocSizeFor_u = num_point_u;
	if( (surf_dat->v = (sgFloat*)SGMalloc((num_point_v)*sizeof(sgFloat))) == NULL ) goto err;
	surf_dat->allocSizeFor_v = num_point_v;

	if( surf_dat->knots.n > 2 && surf_dat->knots.m > 2 ){
//!!!!!!!!!!--    ---!!!!
//		if( !surf_parameter(surf_dat, 0, TRUE )) goto err;
//		calculate_surf_P(surf_dat, 0);
	}
	return TRUE;
err:
	free_surf_data(surf_dat);
	nurbs_handler_err(SURF_NO_MEMORY);
	return FALSE;
}

/**********************************************************
* Free surface data
*/
void free_surf_data(lpSURF_DAT surf_dat){
	if( surf_dat->u )	SGFree( surf_dat->u );
	if( surf_dat->v )	SGFree( surf_dat->v );
	if( surf_dat->U )	SGFree( surf_dat->U );
	if( surf_dat->V )	SGFree( surf_dat->V );
	surf_dat->allocSizeFor_U = surf_dat->allocSizeFor_V = 
		surf_dat->allocSizeFor_u = surf_dat->allocSizeFor_v = 0;

	free_vdim(&surf_dat->knots.vdim);
	free_vdim(&surf_dat->P.vdim);
	if( surf_dat->sdtype & SURF_INT  ) free_vdim( &surf_dat->derivates->vdim );
	memset(surf_dat, 0, sizeof(SURF_DAT));
}

/**********************************************************
*    surf_dat   geo_surf
*/
BOOL create_geo_surf(lpSURF_DAT surf_dat, lpGEO_SURFACE geo_surf){
	short      i, j, k, number;
  W_NODE		 node;
	sgFloat     *u;
	lpD_POINT  p;
	lpSNODE    d;
  MATR			 i_surf_matr;
  	
  if( !o_minv( surf_matr, i_surf_matr )) return FALSE;

	if( surf_dat->sdtype & SURF_APPR ){ //
		if( surf_dat->P.n < surf_dat->degree_p+1 ||
				surf_dat->P.m < surf_dat->degree_q+1 	){
			put_message(SURF_EMPTY_DATA, NULL, 0);
			return FALSE;
		}
  } else {
		if( surf_dat->knots.m < surf_dat->degree_p+1 ||
				surf_dat->knots.n < surf_dat->degree_q+1 	){
			put_message(SURF_EMPTY_DATA, NULL, 0);
			return FALSE;
		}
  }
//  ,    
	memset( geo_surf, 0, sizeof(GEO_SURFACE));

//
	geo_surf->degreeu = surf_dat->degree_p;
	geo_surf->degreev = surf_dat->degree_q;

//   
	int tmp_int = (int)geo_surf->type;
	tmp_int |= NSURF_NURBS;
	//geo_surf->type |= NSURF_NURBS;
	if( surf_dat->sdtype & SURF_APPR  ) /*geo_surf->type*/tmp_int |= NSURF_APPR;
	if( surf_dat->sdtype & SURF_CLOSEU) /*geo_surf->type*/tmp_int |= NSURF_CLOSEU;
	if( surf_dat->sdtype & SURF_CLOSEV) /*geo_surf->type*/tmp_int |= NSURF_CLOSEV;
	geo_surf->type = (NSURF_TYPE)tmp_int;

	if( surf_dat->sdtype & SURF_APPR ){ //
		geo_surf->numpu = surf_dat->P.n;
		geo_surf->numpv = surf_dat->P.m;
		k=surf_dat->P.m*surf_dat->P.n;
    geo_surf->numd=k;
// ---------------->>>>>>>>>   <<<<<<<<<<--------------------------
		if((geo_surf->hpoint =
				SGMalloc(k*sizeof(D_POINT))) == NULL) return FALSE;
    p = (lpD_POINT)geo_surf->hpoint;
	 	if((geo_surf->hderivates =
				SGMalloc(k*sizeof(SNODE))) == NULL)	return FALSE;
    d = (lpSNODE)geo_surf->hderivates;

// 
		for( number=0, j=0; j<surf_dat->P.m; j++, number +=surf_dat->P.n  ){
			for( i=0; i<surf_dat->P.n; i++ ){
	    	read_elem( &surf_dat->P.vdim, number+i, &node );
  	    Create_3D_From_4D( &node, (sgFloat *)&p[number+i], &d[number+i].p[1]);
				o_hcncrd( i_surf_matr, &p[number+i], &p[number+i] );
    	  d[number+i].p[0]=surf_dat->u[i]; //  
      	d[number+i].p[2]=surf_dat->v[j]; //  
      }
    }

// ---------------->>>>>>>>>    <<<<<<<<<<-------------
		k=surf_dat->num_u+surf_dat->num_v;
		if((geo_surf->hparam = SGMalloc(k*sizeof(sgFloat))) == NULL) return FALSE;
		u = (sgFloat*)geo_surf->hparam;
		for( i=0; i<surf_dat->num_u; i++ ) memcpy( &u[i], &surf_dat->U[i], sizeof(sgFloat));
    for( i=0; i<surf_dat->num_v; i++ ) memcpy( &u[i+surf_dat->num_u], &surf_dat->V[i], sizeof(sgFloat));
	}else{                             //
	}
	geo_surf->orient = surf_dat->orient;
	return TRUE;
}

void free_geo_surf( lpGEO_SURFACE geo_surf ){
	SGFree( geo_surf->hpoint);
	SGFree( geo_surf->hderivates);
	SGFree( geo_surf->hparam);
  if (geo_surf->num_curve > 0){
			SGFree( geo_surf->curve);
  }
}
/*******************************************************************************
*   GEO_SURFACE   
*  SURF_DAT
*/
BOOL unpack_geo_surf(lpGEO_SURFACE geo_surf, lpSURF_DAT surf_dat){
short      i, j, number;//, condition;
sgFloat     *u;
lpDA_POINT p;
lpSNODE    d;
W_NODE		 node;
int tmp_int;

	if(surf_dat->sdtype & SURF_GEO) {
		put_message(SURF_INTERNAL_ERROR, NULL, 0);
		return FALSE;
	}
	if(geo_surf->type & (NSURF_NURBS)){
		if( (geo_surf->type & NSURF_APPR) != 0 ){// 
//
			surf_dat->sdtype = SURF_APPR;
			surf_dat->P.n = surf_dat->knots.n = geo_surf->numpu;
			surf_dat->P.m = surf_dat->knots.m = geo_surf->numpv;

// ---------------->>>>>>>>>    <<<<<<<<<<--------------
			p = (lpDA_POINT)geo_surf->hpoint;
      d = (lpSNODE)geo_surf->hderivates;

 			for( number=0, j=0; j<surf_dat->P.m; j++, number +=surf_dat->P.n  ){
				for( i=0; i<surf_dat->P.n; i++ ){
        	o_hcncrd( surf_matr, (lpD_POINT)&p[number+i], (lpD_POINT)&p[number+i] );
        	Create_4D_From_3D( &node, (sgFloat *)&p[number+i], d[number+i].p[1] );
					add_elem(&surf_dat->P.vdim, &node);
          surf_dat->u[i]=d[number+i].p[0]; //  
	        surf_dat->v[j]=d[number+i].p[2];
      	}
    	}
//  
			for( i=0; i<surf_dat->P.n-1; i++ )
        if( dpoint_distance((lpD_POINT)&p[i], (lpD_POINT)&p[i+1] ) > eps_d ) goto met1;
      surf_dat->poles  = SURF_POLEU0;
met1:
			number=surf_dat->P.n *(surf_dat->P.m-1);
			for( i=0; i<surf_dat->P.n-1; i++ )
        if( dpoint_distance((lpD_POINT)&p[number+i], (lpD_POINT)&p[number+i+1] ) > eps_d ) goto met2;
		tmp_int = (int)surf_dat->poles;
		tmp_int |= SURF_POLEU1;
		surf_dat->poles = (SURF_POLES)tmp_int;
        //surf_dat->poles |= SURF_POLEU1;
met2:
 			for( number=0, j=0; j<surf_dat->P.m-1; j++, number +=surf_dat->P.n  )
        if( dpoint_distance((lpD_POINT)&p[number], (lpD_POINT)&p[number+surf_dat->P.n] ) > eps_d ) goto met3;
		tmp_int = (int)surf_dat->poles;
		tmp_int |= SURF_POLEV0;
		surf_dat->poles = (SURF_POLES)tmp_int;
        //surf_dat->poles |= SURF_POLEV0;
met3:
 			for( number=surf_dat->P.n, j=0; j<surf_dat->P.m-1; j++, number +=surf_dat->P.n  )
        if( dpoint_distance((lpD_POINT)&p[number], (lpD_POINT)&p[number+surf_dat->P.n] ) > eps_d ) goto met4;
		tmp_int = (int)surf_dat->poles;
		tmp_int |= SURF_POLEV1;
		surf_dat->poles = (SURF_POLES)tmp_int;
        //surf_dat->poles |= SURF_POLEV1;
met4:


//   
			u = (sgFloat*)geo_surf->hparam;
			surf_dat->num_u =geo_surf->numpu+geo_surf->degreeu+1;
			if (surf_dat->num_u>surf_dat->allocSizeFor_U &&
				surf_dat->U)
			{
				SGFree(surf_dat->U);
				surf_dat->U = (sgFloat*)SGMalloc(surf_dat->num_u*sizeof(sgFloat));
				surf_dat->allocSizeFor_U = surf_dat->num_u;
			}

			memcpy( surf_dat->U, u, surf_dat->num_u*sizeof(sgFloat));

			surf_dat->num_v=geo_surf->numpv+geo_surf->degreev+1;
			if (surf_dat->num_v>surf_dat->allocSizeFor_V &&
				surf_dat->V)
			{
				SGFree(surf_dat->V);
				surf_dat->V = (sgFloat*)SGMalloc(surf_dat->num_v*sizeof(sgFloat));
				surf_dat->allocSizeFor_V = surf_dat->num_v;
			}
			for( j=0; j<surf_dat->num_v; j++ ) 
				surf_dat->V[j]= u[surf_dat->num_u+j];

		}else{ // 
		}

	  surf_dat->orient = geo_surf->orient ;
	  tmp_int = (int)surf_dat->sdtype;
	  if(geo_surf->type & NSURF_CLOSEU)	tmp_int |= SURF_CLOSEU;
   	if(geo_surf->type & NSURF_CLOSEV)	tmp_int |= SURF_CLOSEV;
	  surf_dat->sdtype = (SURF_TYPE_IN)tmp_int;

/*		if(geo_surf->type & NSURF_CLOSEU){
			surf_dat->sdtype |= SURF_CLOSEU;
		if( surf_dat->numd > 0 ){
			if( surf_dat->derivates[0].num == 1 ) condition=2;
			else condition=3;
		}else condition=1;

		}else{
			surf_dat->sdtype |= SPL_FREE;
			if( sply_dat->numd > 0 ) condition=2;
			else condition=0;
		}
*/

		if( surf_dat->knots.m > 2 && surf_dat->knots.n > 2 ){
//			if( !surf_parameter(surf_dat, 0, FALSE )) return FALSE;
//			calculate_surf_P(surf_dat, 0);
		}
	}
	return TRUE;
}

//------------------------------------------------------------------------------
/**********************************************************
* put ribs into surface description
*/
BOOL put_surf_ribs( lpSURF_DAT surf_dat, short type, lpVDIM ribs, BOOL closed ){
short 		 i=0,j;
SPLY_DAT sply_dat;
int tmp_int;

	tmp_int = (int)surf_dat->sdtype;
	tmp_int |= type;
	surf_dat->sdtype = (SURF_TYPE_IN)tmp_int;//|= type;
//    U
	if( closed ) 
	{
		tmp_int|=SURF_CLOSEU;
		surf_dat->sdtype=(SURF_TYPE_IN)tmp_int;//|= SURF_CLOSEU;
	}
	while( read_elem( ribs, i++, &sply_dat))
	{
		if( i==1 )
		{	
			// put U-vector and u
			if (surf_dat->allocSizeFor_U<sply_dat.numU &&
				surf_dat->U)
			{
				SGFree(surf_dat->U);
				surf_dat->U = (sgFloat*)SGMalloc(sply_dat.numU*sizeof(sgFloat));
				surf_dat->allocSizeFor_U = sply_dat.numU;
			}
			memcpy(surf_dat->U, sply_dat.U, sply_dat.numU*sizeof(sgFloat));
			
			surf_dat->num_u=sply_dat.numU;

			if (surf_dat->allocSizeFor_u<sply_dat.nump &&
				surf_dat->u)
			{
				SGFree(surf_dat->u);
				surf_dat->u = (sgFloat*)SGMalloc(sply_dat.nump*sizeof(sgFloat));
				surf_dat->allocSizeFor_u = sply_dat.nump;
			}
			memcpy(surf_dat->u, sply_dat.u, sply_dat.nump*sizeof(sgFloat));
		}
//put control points
		for( j=0; j<sply_dat.nump; j++)
			if( !add_elem(&surf_dat->P.vdim, &sply_dat.P[j] )) return FALSE;
	}
	surf_dat->P.n=sply_dat.nump;
	surf_dat->P.m=i-1;
	return TRUE;
}

/**********************************************************
* free vdim with description of ribs curves
*/
void free_vdim_hobj( lpVDIM rib_curve ){
short  i=0;
hOBJ hobj;

	while( read_elem( rib_curve, i++, &hobj) ) o_free(hobj, NULL);
	free_vdim( rib_curve );
}

/**********************************************************
* free vdim with description of splines
*/
void free_vdim_sply(lpVDIM ribs ){
short  		 i=0;
SPLY_DAT sply;

	while( read_elem( ribs, i++, &sply) ) free_sply_dat( &sply );
	free_vdim( ribs );
}

//------------------------------------------------------------------------------
/********************************************************************
* Create hip of triangles from MESHDD
*/
BOOL Create_MESHDD( lpMESHDD mdd, lpLISTH listho ){
	short			i, j;//, num;
	NPTRP			trp;
	TRI_BIAND	trb;
  MNODE     p1, p2, p3, p4;

	if ( !begin_tri_biand(&trb) ) return FALSE;
  for( j=0; j<mdd->m-1; j++){
	  for( i=0; i<mdd->n-1; i++){
      read_elem( &mdd->vdim, j*mdd->n+i,   &p1);
      read_elem( &mdd->vdim, j*mdd->n+i+1, &p2);
      read_elem( &mdd->vdim, (j+1)*mdd->n+i,   &p3);
      read_elem( &mdd->vdim, (j+1)*mdd->n+i+1, &p4);
//---------------------------------------------------------
//first triangle
			memcpy( &trp.v[0], &p1.p, sizeof(D_POINT));
			memcpy( &trp.v[1], &p2.p, sizeof(D_POINT));
			memcpy( &trp.v[2], &p3.p, sizeof(D_POINT));
//create construct.
     	trp.constr = 0;
      if( p1.num & COND_U ) trp.constr |= TRP_CONSTR;
      else 									trp.constr |= TRP_APPROCH;
      trp.constr |= TRP_DUMMY << 2;
      if( p1.num & COND_V ) trp.constr |= TRP_CONSTR << 4;
      else                  trp.constr |= TRP_APPROCH << 4;
//#define TRP_CONSTR  0x01
//#define TRP_APPROCH 0x02
//#define TRP_DUMMY   0x03

			if ( !put_tri(&trb, &trp)) goto err1;
//---------------------------------------------------------
//second triangle
			memcpy( &trp.v[0], &p2.p, sizeof(D_POINT));
			memcpy( &trp.v[1], &p4.p, sizeof(D_POINT));
			memcpy( &trp.v[2], &p3.p, sizeof(D_POINT));
//create construct.
    	trp.constr = 0;
      if( p4.num & COND_V ) trp.constr |= TRP_CONSTR;
      else                  trp.constr |= TRP_APPROCH;
      if( p4.num & COND_U ) trp.constr |= TRP_CONSTR << 2;
      else                  trp.constr |= TRP_APPROCH << 2;
      trp.constr |= TRP_DUMMY << 4;
			if ( !put_tri(&trb, &trp)) goto err1;
//---------------------------------------------------------
    }
  }
	if( !end_tri_biand(&trb, listho) )	goto err1;
  if( listho->num != 1 ) goto err1;
	return TRUE;
err1:
	a_handler_err(AE_ERR_DATA);
	end_tri_biand(&trb, listho);
	return FALSE;
}

