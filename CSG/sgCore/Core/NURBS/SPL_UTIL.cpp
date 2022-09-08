#include "../sg.h"
static  BOOL check_spl_max(lpSPLY_DAT spl, short num, short degree );

BOOL begin_use_sply(lpGEO_SPLINE geo_sply, lpSPLY_DAT sply_dat){
	short    	 i, j, type;
	sgFloat		 *u;//, w;
	lpDA_POINT p;
	lpSNODE    d;

	//   
	memset(sply_dat, 0, sizeof(SPLY_DAT));

	if (geo_sply->type & SPLY_NURBS){  //  NURBS
		if ( geo_sply->type & SPLY_APPR ) {// 
//
			type = SPL_APPR;

// ---------------->>>>>>>>>     <<<<<<<<<<-------------
//    
 			if((sply_dat->knots    = (DA_POINT*)SGMalloc(geo_sply->nump*sizeof(DA_POINT))) == NULL) goto err;
			sply_dat->allocSizeFor_knots = geo_sply->nump;
			if((sply_dat->P        = (W_NODE *)SGMalloc(geo_sply->nump*sizeof(W_NODE  ))) == NULL) goto err;
			sply_dat->allocSizeFor_P = geo_sply->nump;

//    
			if((sply_dat->u = (sgFloat *)SGMalloc((geo_sply->nump)*sizeof(sgFloat))) == NULL) goto err;
			sply_dat->allocSizeFor_u = geo_sply->nump;


//   
			p = (lpDA_POINT)geo_sply->hpoint;
      d = (lpSNODE)geo_sply->hderivates;
//      
 			memcpy(sply_dat->knots, p, geo_sply->nump*sizeof(DA_POINT));

			for( i=0; i<geo_sply->nump; i++ ){
        Create_4D_From_3D( &sply_dat->P[i], (sgFloat *)&p[i], d[i].p[1] );
        sply_dat->u[i]=d[i].p[0]; //  
			}

			sply_dat->nump = sply_dat->numk = geo_sply->nump;
			sply_dat->numd = 0;

// ---------------->>>>>>>>>    <<<<<<<<<<-------------
//    
			if( geo_sply->numu )  
				i = geo_sply->numu;
			else
				i = geo_sply->nump+geo_sply->degree+1;

			if((sply_dat->U = (sgFloat *)SGMalloc(i*sizeof(sgFloat))) == NULL) goto err;
			sply_dat->allocSizeFor_U = i;


//   
			if( geo_sply->numu )
			{
				u = (sgFloat*)geo_sply->hvector;
				memcpy(sply_dat->U, u, geo_sply->numu*sizeof(sgFloat));
     		    sply_dat->numU = i;
			}
		} else { // 
//
			type = SPL_INT;
// ---------------->>>>>>>>>   <<<<<<<<<<--------------------------
//    
			if((sply_dat->knots     = (DA_POINT*)SGMalloc(geo_sply->nump*sizeof(DA_POINT))) == NULL) goto err;
			sply_dat->allocSizeFor_knots = geo_sply->nump;


//      
			if((sply_dat->u = (sgFloat *)SGMalloc((geo_sply->nump)*sizeof(sgFloat))) == NULL) goto err;
			sply_dat->allocSizeFor_u = geo_sply->nump;


//  
			p = (lpDA_POINT)geo_sply->hpoint;
			memcpy(sply_dat->knots, p, geo_sply->nump*sizeof(DA_POINT));
			sply_dat->numk = geo_sply->nump;

//     -
      i=geo_sply->numd+geo_sply->nump;
      if(geo_sply->type & SPLY_CLOSE) i+=2;
      if((sply_dat->P = (W_NODE *)SGMalloc(i*sizeof(W_NODE))) == NULL) goto err;
	  sply_dat->allocSizeFor_P = i;


//   == 1.
      for( j=0; j<i; j++ ) sply_dat->P[j].vertex[3]=1.;
 			sply_dat->nump = i;

// ---------------->>>>>>>>>   <<<<<<<<<<--------------------
//   
			if((sply_dat->derivates = (SNODE *)SGMalloc(geo_sply->nump*sizeof(SNODE   ))) == NULL) goto err;
			sply_dat->allocSizeFor_derivates = geo_sply->nump;


// 
			if( geo_sply->numd ){
				d = (lpSNODE)geo_sply->hderivates;
				for (i = 0; i < geo_sply->numd; i++) {
					if (0 <= d[i].num && d[i].num < geo_sply->numd) {
						memcpy(sply_dat->derivates[d[i].num].p, d[i].p, sizeof(DA_POINT));
						sply_dat->derivates[d[i].num].num = 1;
					}
				}
			}
			sply_dat->numd = geo_sply->numd;

// ---------------->>>>>>>>>    <<<<<<<<<<-------------
//    
			if( geo_sply->numu )  i = geo_sply->numu;
			else								  i = 2*(geo_sply->nump+geo_sply->degree+1);
			if((sply_dat->U = (sgFloat *)SGMalloc(i*sizeof(sgFloat))) == NULL) goto err;
			sply_dat->allocSizeFor_U = i;


//   
			if( geo_sply->numu ){
				u = (sgFloat*)geo_sply->hvector;
				memcpy(sply_dat->U, u, geo_sply->numu*sizeof(sgFloat));
	 			sply_dat->numU = i;
			}
		}
	}

	if( geo_sply->type & SPLY_CLOSE )		sply_dat->sdtype = SPL_CLOSE;
//   
	if( !init_sply_dat( SPL_GEO, geo_sply->degree, type, sply_dat)) goto err1;
	return TRUE;

err:
	nurbs_handler_err(SPL_NO_MEMORY);
err1:
	free_sply_dat(sply_dat);
	return FALSE;
}

/**********************************************************
*   
*  ,   sply_dat
*/
#pragma argsused
void end_use_sply(lpGEO_SPLINE geo_sply, lpSPLY_DAT sply_dat){
	free_sply_dat( sply_dat );
}

/*********************************************************
*   sply_dat
*/
BOOL init_sply_dat(char sdtype, short degree, short type, lpSPLY_DAT sply_dat ){
	short    num_point, i, condition;

	if( sdtype & SPL_GEO ){        // ,       
													       //      
		sply_dat->sdtype = (SPLY_TYPE_IN)(sply_dat->sdtype | sdtype);  // 
		sply_dat->sdtype = (SPLY_TYPE_IN)(sply_dat->sdtype | type);    ///

//		num_point = sply_dat->numk;
		if(sply_dat->sdtype & SPL_CLOSE){
			if( sply_dat->numd == 0 ) condition=1; // closed without derivation
			else{
			 if( sply_dat->derivates[0].num == 1 ) condition=2; //special case of closed spline
			 else condition=3; // closed with derivates
			}
		}else{
			 if( sply_dat->numd == 0 ) condition=0; // without derivation
			 else condition=2; //with derivates
		}
	}	else { //   .      
		memset(sply_dat, 0, sizeof(SPLY_DAT));
		num_point = MAX_POINT_ON_SPLINE;
		sply_dat->sdtype = (SPLY_TYPE_IN)(sply_dat->sdtype | sdtype);   //  
		sply_dat->sdtype = (SPLY_TYPE_IN)(sply_dat->sdtype | SPL_FREE); //   
		condition=0;

		if ( type & SPL_APPR ) {	// 
			if((sply_dat->knots    = (DA_POINT*)SGMalloc(num_point*sizeof(DA_POINT))) == NULL) goto err;
			sply_dat->allocSizeFor_knots = num_point;
			if((sply_dat->P        = (W_NODE *)SGMalloc(num_point*sizeof(W_NODE  ))) == NULL) goto err;
			sply_dat->allocSizeFor_P = num_point;
			if((sply_dat->U = (sgFloat *)SGMalloc((num_point+degree+1)*sizeof(sgFloat))) == NULL) goto err;
			sply_dat->allocSizeFor_U = num_point+degree+1;
			for (i=0; i<num_point; i++)	sply_dat->P[i].vertex[3] = 1.;//   

		} else {	// 
			if((sply_dat->knots     = (DA_POINT*)SGMalloc(num_point*sizeof(DA_POINT))) == NULL) goto err;
			sply_dat->allocSizeFor_knots = num_point;
			if((sply_dat->derivates = (SNODE *)SGMalloc(num_point*sizeof(SNODE))) == NULL)    goto err;
			sply_dat->allocSizeFor_derivates = num_point;
			if((sply_dat->P         = (W_NODE *)SGMalloc((2*num_point+2)*sizeof(W_NODE))) == NULL) goto err;
			sply_dat->allocSizeFor_P = 2*num_point+2;
			if((sply_dat->U = (sgFloat *)SGMalloc((2*(num_point+degree+1))*sizeof(sgFloat))) == NULL) goto err;
			sply_dat->allocSizeFor_U = 2*(num_point+degree+1);

			for (i = 0; i < num_point; i++)	sply_dat->P[i].vertex[3] = 1.;
		}
//      
		if((sply_dat->u = (sgFloat *)SGMalloc((num_point)*sizeof(sgFloat))) == NULL) goto err;
		sply_dat->allocSizeFor_u = num_point;

	}
	sply_dat->degree = degree;
	sply_dat->sdtype = (SPLY_TYPE_IN)(sply_dat->sdtype | type); ///

	if( sply_dat->numk > 2 ){
  	if( sply_dat->sdtype & SPL_INT ){
			if( sply_dat->numU ) parameter( sply_dat, condition, FALSE );
			else{
       	sply_dat->numU = 2*(sply_dat->nump+sply_dat->degree+1); ;
        parameter( sply_dat, condition, TRUE );
      }
    }
		calculate_P(sply_dat, condition);
	}
	return TRUE;
err:
	free_sply_dat( sply_dat );
	nurbs_handler_err(SPL_NO_MEMORY);
	return FALSE;
}

void free_sply_dat( lpSPLY_DAT sply_dat){
	if (sply_dat->knots) {
		SGFree(sply_dat->knots);
		sply_dat->knots = NULL;
		sply_dat->allocSizeFor_knots = 0;
	}
	if (sply_dat->derivates) {
		SGFree(sply_dat->derivates);
		sply_dat->derivates = NULL;
		sply_dat->allocSizeFor_derivates = 0;
	}
	if (sply_dat->P) {
		SGFree(sply_dat->P);
		sply_dat->P = NULL;
		sply_dat->allocSizeFor_P = 0;
	}
	if (sply_dat->U) {
		SGFree(sply_dat->U);
		sply_dat->U = NULL;
		sply_dat->allocSizeFor_U = 0;
	}
	if (sply_dat->u) {
		SGFree(sply_dat->u);
		sply_dat->u = NULL;
		sply_dat->allocSizeFor_u = 0;
	}
}

BOOL create_geo_sply(lpSPLY_DAT sply_dat, lpGEO_SPLINE geo_sply){
	short      i, j;
	sgFloat		 *u;
	lpDA_POINT p;
	lpSNODE    d;

	if( sply_dat->numk < sply_dat->degree+1 ){
		put_message(SPL_EMPTY_DATA, NULL, 0);
		return FALSE;
	}

//  ,    
	memset( geo_sply, 0, sizeof(GEO_SPLINE));

//
	geo_sply->degree = sply_dat->degree;

//   
	geo_sply->type = SPLY_NURBS;
	if( sply_dat->sdtype & SPL_APPR ) geo_sply->type |= SPLY_APPR;
	if( sply_dat->sdtype & SPL_CLOSE)	geo_sply->type |= SPLY_CLOSE;

	if( sply_dat->sdtype & SPL_APPR ){ //

// ---------------->>>>>>>>>   <<<<<<<<<<--------------------------
		if((geo_sply->hpoint =
				SGMalloc(sply_dat->nump*sizeof(DA_POINT))) == NULL) return FALSE;
		p = (lpDA_POINT)geo_sply->hpoint;
		if((geo_sply->hderivates =
				SGMalloc(sply_dat->nump*sizeof(SNODE))) == NULL)	return FALSE;
		d = (lpSNODE)geo_sply->hderivates;

    for( i=0; i<sply_dat->nump; i++ )
	{
	  if (i<sply_dat->allocSizeFor_P)
		Create_3D_From_4D( &sply_dat->P[i], (sgFloat *)&p[i], &d[i].p[1]);
	  else
	  {
		  assert(0);
	  }

	  if (i<sply_dat->allocSizeFor_u)
		d[i].p[0]=sply_dat->u[i]; //  
	  else
	  {
		  assert(0);
	  }
    }

		geo_sply->nump = geo_sply->numd = sply_dat->nump;

// ---------------->>>>>>>>>    <<<<<<<<<<-------------
		if((geo_sply->hvector =
				SGMalloc(sply_dat->numU*sizeof(sgFloat))) == NULL) return FALSE;
		u = (sgFloat*)geo_sply->hvector;
		memcpy( u, sply_dat->U, sply_dat->numU*sizeof(sgFloat));
 		geo_sply->numu = sply_dat->numU;

	}else{                             //
// ---------------->>>>>>>>>   <<<<<<<<<<--------------------------
		if((geo_sply->hpoint =
				SGMalloc(sply_dat->numk*sizeof(DA_POINT))) == NULL) return FALSE;
		p = (lpDA_POINT)geo_sply->hpoint;
		memcpy( p, sply_dat->knots, sply_dat->numk*sizeof(DA_POINT));
		geo_sply->nump = sply_dat->numk;

// ---------------->>>>>>>>>   <<<<<<<<<<--------------------
		if( sply_dat->numd ){
			if((geo_sply->hderivates =
					SGMalloc(sply_dat->numd*sizeof(SNODE))) == NULL)	return FALSE;
			d = (lpSNODE)geo_sply->hderivates;
			for( j=0, i=0; i<sply_dat->numk; i++ )
				if( sply_dat->derivates[i].num ){
					memcpy( d[j].p, sply_dat->derivates[i].p,	sizeof(DA_POINT));
					d[j++].num = i;
				}
		}
		geo_sply->numd = sply_dat->numd;

// ---------------->>>>>>>>>    <<<<<<<<<<-------------
		if((geo_sply->hvector =
				SGMalloc(sply_dat->numU*sizeof(sgFloat))) == NULL) return FALSE;
		u = (sgFloat*)geo_sply->hvector;
		memcpy( u, sply_dat->U, sply_dat->numU*sizeof(sgFloat));
 		geo_sply->numu = sply_dat->numU;
	}
	return TRUE;
}

BOOL unpack_geo_sply(lpGEO_SPLINE geo_sply, lpSPLY_DAT sply_dat){
short      i, condition;
sgFloat		 *u;
lpDA_POINT p;
lpSNODE    d;

	if(sply_dat->sdtype & SPL_GEO) {
		put_message(SPL_INTERNAL_ERROR, NULL, 0);
		return FALSE;
	}

	if(geo_sply->type & SPLY_NURBS)
	{
		if( (geo_sply->type & SPLY_APPR) != 0 )
		{// 
			sply_dat->sdtype = SPL_APPR;
// ---------------->>>>>>>>>    <<<<<<<<<<--------------
			p = (lpDA_POINT)geo_sply->hpoint;
			d = (lpSNODE)geo_sply->hderivates;
				// RA-BEGIN 
					if (geo_sply->nump>sply_dat->allocSizeFor_knots &&
						sply_dat->knots)
					{
						SGFree(sply_dat->knots);
						sply_dat->knots = (lpDA_POINT)SGMalloc(geo_sply->nump*sizeof(DA_POINT));
						sply_dat->allocSizeFor_knots = geo_sply->nump;
					}
					if (geo_sply->nump>sply_dat->allocSizeFor_P &&
						sply_dat->P)
					{
						SGFree(sply_dat->P);
						sply_dat->P = (W_NODE*)SGMalloc(geo_sply->nump*sizeof(W_NODE));
						sply_dat->allocSizeFor_P = geo_sply->nump;
					}
					if (geo_sply->nump>sply_dat->allocSizeFor_u &&
						sply_dat->u)
					{
						SGFree(sply_dat->u);
						sply_dat->u = (sgFloat*)SGMalloc(geo_sply->nump*sizeof(sgFloat));
						sply_dat->allocSizeFor_u = geo_sply->nump;
					}
				// RA - END
			memcpy(sply_dat->knots, p, geo_sply->nump*sizeof(DA_POINT));
			
			for( i=0; i<geo_sply->nump; i++ )
			{
				Create_4D_From_3D( &sply_dat->P[i], (sgFloat *)&p[i], d[i].p[1] );
				sply_dat->u[i]=d[i].p[0]; //  
			}

			sply_dat->nump = sply_dat->numk = geo_sply->nump;
			sply_dat->numd = 0;
// ---------------->>>>>>>>>    <<<<<<<<<<-------------
			if( geo_sply->numu )
			{
					// RA-BEGIN 
							if (geo_sply->numu>sply_dat->allocSizeFor_U &&
								sply_dat->U)
						{
							SGFree(sply_dat->U);
							sply_dat->U =(sgFloat*)SGMalloc(geo_sply->numu*sizeof(sgFloat));
							sply_dat->allocSizeFor_U = geo_sply->numu;
						}
						
					// RA - END
				u = (sgFloat*)geo_sply->hvector;
				memcpy(sply_dat->U, u, geo_sply->numu*sizeof(sgFloat));
				sply_dat->numU = geo_sply->numu;
			}
		}
		else
		{ // 
				sply_dat->sdtype = SPL_INT;

// ---------------->>>>>>>>>   <<<<<<<<<<--------------------------
					// RA-BEGIN 
					if (geo_sply->nump>sply_dat->allocSizeFor_knots &&
						sply_dat->knots)
					{
						SGFree(sply_dat->knots);
						sply_dat->knots = (lpDA_POINT)SGMalloc(geo_sply->nump*sizeof(DA_POINT));
						sply_dat->allocSizeFor_knots = geo_sply->nump;
					}
					// RA - END
				p = (lpDA_POINT)geo_sply->hpoint;
				memcpy(sply_dat->knots, p, geo_sply->nump*sizeof(DA_POINT));
				sply_dat->numk = geo_sply->nump;
// ---------------->>>>>>>>>   <<<<<<<<<<--------------------
				if( geo_sply->numd )
				{
					d = (lpSNODE)geo_sply->hderivates;
					for(i=0; i<sply_dat->numd; i++ )
					{
						if (d[i].num<sply_dat->allocSizeFor_derivates)
						{
							memcpy(sply_dat->derivates[d[i].num].p, d[i].p, sizeof(DA_POINT));
							sply_dat->derivates[d[i].num].num = 1;
						}
						else
						{
							assert(0);
						}
					}
				}
				sply_dat->numd = geo_sply->numd;

// ---------------->>>>>>>>>    <<<<<<<<<<-------------
				if( geo_sply->numu )
				{
					u = (sgFloat*)geo_sply->hvector;
					if (geo_sply->numu<sply_dat->allocSizeFor_U)
					{
						memcpy(sply_dat->U, u, geo_sply->numu*sizeof(sgFloat));
	 					sply_dat->numU = geo_sply->numu;
					}
					else
					{
						assert(0);
					}
				}
		}
	}

	if(geo_sply->type & SPLY_CLOSE)
	{
		sply_dat->sdtype = (SPLY_TYPE_IN)(sply_dat->sdtype | SPL_CLOSE);
		if( sply_dat->numd > 0 )
		{
			if( sply_dat->derivates[0].num == 1 ) 
				condition=2;
			else 
				condition=3;
		}else 
			condition=1;
	}
	else
	{
		sply_dat->sdtype = (SPLY_TYPE_IN)(sply_dat->sdtype | SPL_FREE);
		if( sply_dat->numd > 0 ) 
			condition=2;
		else 
			condition=0;
	}

	sply_dat->degree=geo_sply->degree;

	if( sply_dat->numk > 2 )
	{
		if( sply_dat->sdtype & SPL_INT )
		{
				if( sply_dat->numU ) 
					parameter( sply_dat, condition, FALSE );
				else 
				{
      				sply_dat->numU = geo_sply->nump+geo_sply->degree+1;
		  			parameter( sply_dat, condition, TRUE );
				}
		}
		calculate_P(sply_dat, condition); //   
	}
	return TRUE;
}

/**********************************************************
* expand memory for spline
* num  		 - number of add element
* degree	 - degree of curve
* type = 0 - add point
*				 1 - add derivation
*				 2 - add knot
*/
BOOL expand_spl( lpSPLY_DAT spline, short num, short degree, short type ){
	UINT				size;
	VLD					vld;
	VI_LOCATION	loc;

	if ( !check_spl_max( spline, num, degree) ) return FALSE;
	init_vld(&vld);

// Save information
	if( spline->P && spline->nump )
		if( !add_vld_data( &vld, sizeof(W_NODE)*(spline->nump), &spline->P[0])) goto err;

	if( spline->U && spline->numU )
		if( !add_vld_data( &vld, sizeof(sgFloat)*(spline->numU), &spline->U[0])) goto err;

	if( type == 0 || type == 2 ){
		if( spline->knots && spline->numk )
			if( !add_vld_data( &vld, sizeof(DA_POINT)*(spline->numk), &spline->knots[0])) goto err;

		if( spline->u && spline->numk )
			if( !add_vld_data( &vld, sizeof(sgFloat)*(spline->numk), &spline->u[0])) goto err;

		if( spline->derivates && spline->numd ){
				if( !add_vld_data( &vld, sizeof(SNODE)*(spline->numk), &spline->derivates[0])) goto err;}
	}

//  
	if( spline->P ){   SGFree(spline->P); 	spline->P = NULL; spline->allocSizeFor_P=0;}
	if( spline->U ){   SGFree(spline->U);	spline->U = NULL; spline->allocSizeFor_U=0;}
	if( type == 0 || type == 2 ){
		if( spline->knots){	SGFree(spline->knots); spline->knots = NULL; spline->allocSizeFor_knots=0;}
		if( spline->u)		{	SGFree(spline->u);     spline->u = NULL; spline->allocSizeFor_u=0;}
		if( spline->derivates ){	SGFree(spline->derivates); spline->derivates = NULL; spline->allocSizeFor_derivates=0;}
	}

// Alloc memory
	size = sizeof(W_NODE)*(MAX_POINT_ON_SPLINE+num);
	if( ( spline->P = (W_NODE *)SGMalloc(size)) == NULL ) goto err1;
	spline->allocSizeFor_P=MAX_POINT_ON_SPLINE+num;

	size = sizeof(sgFloat)*(MAX_POINT_ON_SPLINE+num+degree+1);
	if( ( spline->U = (sgFloat *)SGMalloc(size)) == NULL ) goto err1;
	spline->allocSizeFor_U=MAX_POINT_ON_SPLINE+num+degree+1;

	if( type == 0 || type == 2 ){
		size = sizeof(DA_POINT)*(MAX_POINT_ON_SPLINE+num);
		if( ( spline->knots = (DA_POINT*)SGMalloc(size)) == NULL ) goto err1;
		spline->allocSizeFor_knots=MAX_POINT_ON_SPLINE+num;

		size = sizeof(sgFloat)*(MAX_POINT_ON_SPLINE+num);
		if( ( spline->u = (sgFloat *)SGMalloc(size)) == NULL ) goto err1;
		spline->allocSizeFor_u=MAX_POINT_ON_SPLINE+num;

		if( type == 0 ){
			size = sizeof(SNODE)*(MAX_POINT_ON_SPLINE+num);
			if( ( spline->derivates = (SNODE *)SGMalloc(size)) == NULL ) goto err1;
			spline->allocSizeFor_derivates=MAX_POINT_ON_SPLINE+num;
		}
	}

// rearrange the spline
	loc.hpage		= vld.listh.hhead;
	loc.offset	= sizeof(RAW);
	begin_read_vld(&loc);

	size = sizeof(W_NODE)*(spline->nump);
	read_vld_data(size,&spline->P[0]);

	size = sizeof(sgFloat)*(spline->numU);
	read_vld_data(size,&spline->U[0]);

	if( type == 0 || type == 1 ){
		size = sizeof(DA_POINT)*(spline->numk);
		read_vld_data(size,&spline->knots[0]);

		size = sizeof(sgFloat)*(spline->numk);
		read_vld_data(size,&spline->u[0]);

		if( type == 0 ){
			size = sizeof(SNODE)*(spline->numk);
			read_vld_data(size,&spline->derivates[0]);
		}
	}
	end_read_vld();
	free_vld_data(&vld);

	return TRUE;
err:
	nurbs_handler_err(SPL_NO_MEMORY);
	free_vld_data(&vld);
	return FALSE;
err1:
	nurbs_handler_err(SPL_NO_HEAP);
	free_vld_data(&vld);
	return FALSE;
}

static  BOOL check_spl_max(lpSPLY_DAT spl, short num, short degree ){
	if ( sizeof(*spl->P)*(long)(spl->nump+num) >= 65535L ) 	goto err;
	if ( sizeof(*spl->U)*(long)(spl->nump+degree+num+1) >= 65535L ) goto err;
	if ( sizeof(*spl->knots)*(long)(spl->numk+num) >= 65535L ) goto err;
	if ( sizeof(*spl->u)*(long)(spl->numk+num) >= 65535L ) 	goto err;
	if ( sizeof(*spl->derivates)*(long)(spl->numk+num) >= 65535L ) 	goto err;
	return TRUE;
err:
	nurbs_handler_err(SPL_MAX);
	return FALSE;
}
