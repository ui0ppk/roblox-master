#include "../sg.h"

static BOOL calculate_sply(lpSPLY_DAT sply_dat);


BOOL add_sply_point(lpSPLY_DAT sply_dat, lpD_POINT point, short num){
int i;

	if( sply_dat->sdtype & SPL_INT) {
		if( sply_dat->numk >= MAX_POINT_ON_SPLINE  )
			if( !expand_spl( sply_dat, MAX_POINT_ON_SPLINE/2, sply_dat->degree, 0) ) return FALSE;
		if( num == 0 ){
			for( i=sply_dat->numk-1; i >= 0; i--) {
				memcpy(&sply_dat->knots[i+1], &sply_dat->knots[i], sizeof(DA_POINT));
				memcpy(&sply_dat->derivates[i+1], &sply_dat->derivates[i], sizeof(SNODE));
			}
    }
		memcpy(&sply_dat->knots[num], point, sizeof(D_POINT));
		sply_dat->derivates[num].num = 0; //  
		sply_dat->numk++;
		return calculate_sply(sply_dat);
	} else {
		if( sply_dat->nump >= MAX_POINT_ON_SPLINE )
			if( !expand_spl( sply_dat, MAX_POINT_ON_SPLINE/2, sply_dat->degree, 2) ) return FALSE;
    if( num == 0 )
 			for( i=sply_dat->nump-1; i >= 0; i--){
  			memcpy(&sply_dat->P[i+1], &sply_dat->P[i], sizeof(W_NODE));
        if( spl_work & EDIT_SPL )sply_dat->u[i+1] = sply_dat->u[i];
      }

 		Create_4D_From_3D( &sply_dat->P[num], (sgFloat *)point, 1. );// 
 		sply_dat->nump++;

   	if( sply_dat->nump > 2 ){
    	if( spl_work & CRE_SPL ) parameter( sply_dat, 0, TRUE );
      if( spl_work & EDIT_SPL ) spl_work = (SPL_WORK)(spl_work | ADD_SPL);
			return calculate_P(sply_dat, 0); //   
 	  }else{
			memcpy(&sply_dat->knots[num], point, sizeof(D_POINT));
 			return TRUE;
    }
	}
}

BOOL insert_sply_point(lpSPLY_DAT sply_dat, short num, lpD_POINT point){
	short    i;
  sgFloat   D1, D2, w1, w2, s, u_ins;
  D_POINT  point1, point2;

	if (sply_dat->sdtype & SPL_INT) {
		if (sply_dat->numk >= MAX_POINT_ON_SPLINE)
			if( !expand_spl( sply_dat, MAX_POINT_ON_SPLINE/2, sply_dat->degree, 0))	return FALSE;

//   
		for( i=sply_dat->numk-1; i >= num; i-- ) {
			memcpy(&sply_dat->knots[i+1], &sply_dat->knots[i], sizeof(DA_POINT));
			memcpy(&sply_dat->derivates[i+1], &sply_dat->derivates[i], sizeof(SNODE));
		}
		memcpy(&sply_dat->knots[num], point, sizeof(D_POINT));
		sply_dat->derivates[num].num = 0; //  
		sply_dat->numk++;
	} else {
		if (sply_dat->nump >= MAX_POINT_ON_SPLINE)
			if( !expand_spl( sply_dat, MAX_POINT_ON_SPLINE/2, sply_dat->degree, 2))	return FALSE;
//    
	  Create_3D_From_4D( &sply_dat->P[num-1], (sgFloat *)&point1, &w1);
		Create_3D_From_4D( &sply_dat->P[num],   (sgFloat *)&point2, &w2);
		D1=dpoint_distance( point, &point1);
    D2=dpoint_distance( point, &point2);
		s=w1*D1/(w1*D1+w2*D2);
    u_ins=sply_dat->U[num]+s*(sply_dat->U[num+sply_dat->degree]-sply_dat->U[num]);
//  
		if( !Create_Multy_Point( sply_dat, u_ins, 21 )) return FALSE;
	}
	return TRUE;
}

BOOL close_sply(lpSPLY_DAT sply_dat){
short 	i;
sgFloat  w;
D_POINT point;
//W_NODE	point;
GEO_LINE  gline1, gline2;

	if( sply_dat->sdtype & SPL_CLOSE ) {
		nurbs_handler_err(SPL_INTERNAL_ERROR);
		return FALSE;
	}
	if( sply_dat->sdtype & SPL_INT ) {
		if( sply_dat->numk<3 ) {
			nurbs_handler_err(SPL_EMPTY_DATA);
			return FALSE;
		}
		if( sply_dat->numk >= MAX_POINT_ON_SPLINE || sply_dat->nump+2 >= MAX_POINT_ON_SPLINE )
			if( !expand_spl( sply_dat, MAX_POINT_ON_SPLINE/2, sply_dat->degree, 0))	return FALSE;

		if( !dpoint_eq((lpD_POINT)&sply_dat->knots[0], (lpD_POINT)&sply_dat->knots[sply_dat->numk-1], eps_d)) sply_dat->numk++;
		memcpy(&sply_dat->knots[sply_dat->numk-1], &sply_dat->knots[0], sizeof(D_POINT));
	} else {
		if( sply_dat->nump<3 ) {
			nurbs_handler_err(SPL_EMPTY_DATA);
			return FALSE;
		}
		if( sply_dat->nump+2 >= MAX_POINT_ON_SPLINE  )
			if( !expand_spl( sply_dat, MAX_POINT_ON_SPLINE/2, sply_dat->degree, 2))	return FALSE;

/*		if( sply_dat->degree > 1 ){
//for smoth closer add new point r(n-1) = 2*r0 - r1
			for( i=0; i<4; i++)	point.vertex[i]=2*sply_dat->P[0].vertex[i] - sply_dat->P[1].vertex[i];
			if( dpoint_distance_4dl( &sply_dat->P[sply_dat->nump-1], &point) > eps_d ){
				Create_3D_From_4D( &point, (sgFloat *)&point1, &w);
				if( !add_sply_point( sply_dat, &point1, sply_dat->nump) ) return FALSE;
        sply_dat->P[sply_dat->numk-1].vertex[3]=w;
      }
		}
		if( dpoint_distance_4dl( &sply_dat->P[0], &point) > eps_d ) sply_dat->nump++;
*/

//create line on spline points
		Create_3D_From_4D( &sply_dat->P[0], (sgFloat *)&gline1.v1, &w);
		Create_3D_From_4D( &sply_dat->P[1], (sgFloat *)&gline1.v2, &w);
		Create_3D_From_4D( &sply_dat->P[sply_dat->nump-2], (sgFloat *)&gline2.v1, &w);
		Create_3D_From_4D( &sply_dat->P[sply_dat->nump-1], (sgFloat *)&gline2.v2, &w);
//---------------------------------------------------------
//detect intersection of the two lines
		if( !intersect_3d_ll(&gline1, 0, &gline2, 0, &point, &i) ) return FALSE;

//---------------------------------------------------------
		if ( i != 0 ){  // there are points of intersection
			if( !add_sply_point( sply_dat, &point, sply_dat->nump) ) return FALSE;
    	sply_dat->nump++;
			memcpy(&sply_dat->P[sply_dat->nump-1], &sply_dat->P[0], sizeof(W_NODE));
    }else return FALSE;
	}
	sply_dat->sdtype = (SPLY_TYPE_IN)(sply_dat->sdtype | SPL_CLOSE);
	return calculate_sply(sply_dat);
}

BOOL move_sply_point(lpSPLY_DAT sply_dat, short num, lpD_POINT point){
  short	 i;
	sgFloat w, l1, l2;
	sgFloat r, pp[3];

	if (sply_dat->sdtype & SPL_INT) {
		l1=l2=10.*eps_d;
		if( num>0 ) l1 = dpoint_distance((lpD_POINT)&sply_dat->knots[num-1], point);
		else if( sply_dat->sdtype&SPL_CLOSE )
                l1 = dpoint_distance((lpD_POINT)&sply_dat->knots[sply_dat->numk-2], point);
		if( num<sply_dat->numk )l2 = dpoint_distance((lpD_POINT)&sply_dat->knots[num+1], point);
		else if( sply_dat->sdtype & SPL_CLOSE )
               	l2 = dpoint_distance((lpD_POINT)&sply_dat->knots[1], point);
		if( l1<eps_d || l2<eps_d ) {
//			put_message(SPL_BAD_POINT, NULL, 0);  //   
			return FALSE;
		}
		memcpy(&sply_dat->knots[num], point, sizeof(D_POINT));
		if( sply_dat->sdtype&SPL_CLOSE && (num==0 || num==sply_dat->numk-1))
			memcpy(&sply_dat->knots[(num)?0:sply_dat->numk-1], point, sizeof(D_POINT));
		return calculate_sply(sply_dat);
	}else{
    w=sply_dat->P[num].vertex[3];
		Create_4D_From_3D( &sply_dat->P[num], (sgFloat *)point, w );//  
		memcpy(&sply_dat->knots[num], point, sizeof(D_POINT));

   	if( sply_dat->sdtype&SPL_CLOSE && (num==0 || num==sply_dat->nump-1)){
	    Create_4D_From_3D( &sply_dat->P[(num)?0:sply_dat->nump-1], (sgFloat *)point, w );
			memcpy(&sply_dat->knots[(num)?0:sply_dat->nump-1], point, sizeof(D_POINT));
    }
    if( sply_dat->nump > 2 ){
	    if( spl_work & CRE_SPL ) parameter( sply_dat, 0, TRUE );
	    if( spl_work & ADD_SPL ){
      	spl_work = (SPL_WORK)(spl_work & (~ADD_SPL));
		    if( num == 0 ){
					Create_3D_From_4D( &sply_dat->P[0], (sgFloat *)pp, &w );
  	  	  if( (r=dpoint_distance( (lpD_POINT)point, (lpD_POINT)pp ))<eps_d ) return TRUE;
					r/=spl_length(sply_dat);

			    for( i=sply_dat->numU-1; i >= 0; i--) sply_dat->U[i+1] = sply_dat->U[i]+r;
  	      sply_dat->u[0]=0;
    	    sply_dat->U[0]=sply_dat->U[1]=sply_dat->U[2]=0;
      	}else{
					Create_3D_From_4D( &sply_dat->P[num-1], (sgFloat *)pp, &w );
	        if( (r=dpoint_distance( (lpD_POINT)point, (lpD_POINT)pp ))<eps_d ) return TRUE;
					r/=spl_length_P(sply_dat);

    	    sply_dat->u[num]=sply_dat->u[num-1]+r;
	        for( i=0; i<sply_dat->degree+1; i++ )
  	        sply_dat->U[sply_dat->nump+i]=sply_dat->U[sply_dat->nump-1]+r;
    	  }
  			Create_4D_From_3D( &sply_dat->P[num], (sgFloat *)point, 1. );// 
				sply_dat->numU++;
  	    if( !Reparametrization(	sply_dat->nump, sply_dat->numU,	0, 1, sply_dat->degree,
																sply_dat->U, sply_dat->P, sply_dat->u )) return FALSE;
      }
			return calculate_P(sply_dat, 0); //   
    }else{
			memcpy(&sply_dat->knots[num], point, sizeof(D_POINT));
      return TRUE;
    }
  }
}

BOOL delete_sply_point(lpSPLY_DAT sply_dat, short num){
	short i, l, beg;

	if (sply_dat->sdtype & SPL_INT) {
//     
		if( (sply_dat->sdtype & SPL_CLOSE) && sply_dat->numk<sply_dat->degree+2 ) {
			sply_dat->sdtype = (SPLY_TYPE_IN)(sply_dat->sdtype & ~SPL_CLOSE);
			sply_dat->numk--;
			if( num==sply_dat->numk ) num = 0;
		}

		if( num != sply_dat->numk - 1 && num != 0) { //     
			if( dpoint_distance((lpD_POINT)&sply_dat->knots[num - 1],
												  (lpD_POINT)&sply_dat->knots[num + 1])<eps_d) goto err;
		}else{                                       //  
			if( sply_dat->sdtype & SPL_CLOSE ) { //  
				num=0;
				if(dpoint_distance((lpD_POINT)&sply_dat->knots[1],
													 (lpD_POINT)&sply_dat->knots[sply_dat->numk-2])<eps_d) goto err;
// -     1
				memcpy(&sply_dat->knots[sply_dat->numk-1], &sply_dat->knots[1], sizeof(D_POINT));
				memcpy(&sply_dat->derivates[sply_dat->numk-1], &sply_dat->derivates[1], sizeof(SNODE));
			}
		}
//  
		if( (l = sply_dat->numk-num-1)>0 ) {
				if (sply_dat->derivates[num].num) sply_dat->numd--;
				memcpy(&sply_dat->knots[num], &sply_dat->knots[num+1], l*sizeof(D_POINT));
				memcpy(&sply_dat->derivates[num], &sply_dat->derivates[num+1], l*sizeof(SNODE));
		}
		sply_dat->numk--;
   	return calculate_sply(sply_dat);
	} else {//--------------------->>>>>>>> APPR spline
//     
		if( (sply_dat->sdtype&SPL_CLOSE ) && sply_dat->nump<sply_dat->degree+2 ) {
			sply_dat->sdtype = (SPLY_TYPE_IN)(sply_dat->sdtype & ~SPL_CLOSE);
			sply_dat->nump--;
			if( num == sply_dat->nump ) num=0;
		}

//      
		if( sply_dat->sdtype & SPL_CLOSE && ( num == sply_dat->nump-1 || num == 0 ))
				memcpy( &sply_dat->P[sply_dat->nump-1], &sply_dat->P[1], sizeof(W_NODE));//   1

//   
		if( (l = sply_dat->nump-num-1 ) > 0){
			memcpy(&sply_dat->P[num], &sply_dat->P[num+1], l*sizeof(W_NODE));
			memcpy(&sply_dat->u[num], &sply_dat->u[num+1], l*sizeof(sgFloat));
    }
		sply_dat->nump--;

		if( spl_work&CRE_SPL ) parameter( sply_dat, 0, TRUE );
    else{
    	if( num==0 ) beg=sply_dat->degree+2;
      else{
      	if( num==sply_dat->nump ) beg=sply_dat->nump+2;
        else{
        	if( num==sply_dat->nump-1 ) beg=sply_dat->degree+1+num-1;
          else beg = sply_dat->degree+1+num;
        }
      }
     	for( i=beg; i<sply_dat->numU; i++ )	sply_dat->U[i-1]=sply_dat->U[i];
      sply_dat->numU--;
      if( num==0 || num==sply_dat->nump)
				 Reparametrization(	sply_dat->nump, sply_dat->numU,	0, 1, sply_dat->degree,
														sply_dat->U, sply_dat->P, sply_dat->u );

    }
		return calculate_P(sply_dat, 0); //   
  }
err:
	nurbs_handler_err(SPL_BAD_POINT);  //    
	return FALSE;
}

BOOL break_close_sply( lpSPLY_DAT sply_dat, short num ){
	short 	 	i, n;
	DA_POINT	da1;
  W_NODE    wn;
	SNODE     sn1;

	if( sply_dat->sdtype&SPL_INT ) {
		if( num<sply_dat->numk-1 ) { //     
//    num+1  
			n=sply_dat->numk-2;
			for( i=num+1; i>0; i-- ) {
				memcpy(da1, sply_dat->knots[0], sizeof(DA_POINT));
				sn1 = sply_dat->derivates[0];
				memcpy(sply_dat->knots[0], sply_dat->knots[1], n * sizeof(DA_POINT));
				memcpy(&sply_dat->derivates[0], &sply_dat->derivates[1], n * sizeof(SNODE));
				memcpy(sply_dat->knots[n], da1, sizeof(DA_POINT));
				sply_dat->derivates[n] = sn1;
			}
		}
		sply_dat->numk--;
	}else{
		if( num<sply_dat->nump-1 ) { //     
//    num+1  
			n=sply_dat->nump-2;
			for( i=num+1; i>0; i-- ) {
				memcpy(&wn, &sply_dat->P[0], sizeof(W_NODE));
				memcpy(&sply_dat->P[0], &sply_dat->P[1], n*sizeof(W_NODE));
				memcpy(&sply_dat->P[n], &wn, sizeof(W_NODE));
			}
		}
		sply_dat->nump--;
	}
	sply_dat->sdtype = (SPLY_TYPE_IN)(sply_dat->sdtype & ~SPL_CLOSE);
	return calculate_sply(sply_dat);
}

BOOL set_derivate_to_point(lpSPLY_DAT sply_dat, short num, lpD_POINT derivates){
	if( sply_dat->sdtype & SPL_APPR) return TRUE;
	if( (num == 0) || (num == sply_dat->numk - 1) ){// first/end point && closed
		if( dpoint_distance((lpD_POINT)&sply_dat->knots[0],
												(lpD_POINT)&sply_dat->knots[sply_dat->numk - 1]) < eps_d ){
			if( dskalar_product(derivates,derivates) < eps_d )
				 get_point_on_sply(sply_dat, sply_dat->u[num], derivates, 1);

			memcpy(sply_dat->derivates[0].p, derivates, sizeof(D_POINT));
			memcpy(sply_dat->derivates[sply_dat->numk - 1].p, derivates, sizeof(D_POINT));
			if( sply_dat->derivates[0].num == 0 ){
				sply_dat->derivates[0].num=1;
				sply_dat->numd++;
			}
			if( sply_dat->derivates[sply_dat->numk - 1].num == 0 ){
				sply_dat->derivates[sply_dat->numk - 1].num=1;
				sply_dat->numd++;
			}
			if( sply_dat->nump+1 >= MAX_POINT_ON_SPLINE  )
				if( !expand_spl( sply_dat, MAX_POINT_ON_SPLINE/2, sply_dat->degree, 1 ))
					return FALSE;
			return calculate_sply(sply_dat);
		}
	}
	memcpy(sply_dat->derivates[num].p, derivates, sizeof(D_POINT));
	if (!sply_dat->derivates[num].num) {
		sply_dat->derivates[num].num = 1;
		sply_dat->numd++;
	}
	if( sply_dat->nump >= MAX_POINT_ON_SPLINE ){
		if( !expand_spl( sply_dat, MAX_POINT_ON_SPLINE/2, sply_dat->degree, 1 ))
			return FALSE;
	}
	return calculate_sply(sply_dat);
}

BOOL fix_free_derivate_to_point(lpSPLY_DAT sply_dat, short num, BOOL fix){
	D_POINT deriv;

	if (sply_dat->sdtype & SPL_APPR) return TRUE;
	if (fix && !sply_dat->derivates[num].num) {
		get_point_on_sply(sply_dat, sply_dat->u[num], &deriv, 1);
		memcpy(sply_dat->derivates[num].p, &deriv, sizeof(D_POINT));
		sply_dat->derivates[num].num=1;
		sply_dat->numd++;
	}
	if (!fix && sply_dat->derivates[num].num) {
		sply_dat->derivates[num].num = 0;
		sply_dat->numd--;
	}
	if( sply_dat->nump >= MAX_POINT_ON_SPLINE )
		if( !expand_spl( sply_dat, MAX_POINT_ON_SPLINE/2, sply_dat->degree, 1 ))
			return FALSE;

	return calculate_sply(sply_dat);
}

/*******************************************************************************
* Change spline type from INT to APPR
*/
BOOL change_INT_APPR(lpSPLY_DAT sply_dat){
	short i;
	if( sply_dat->sdtype & SPL_APPR) return FALSE;

// free arrays for short spline
	if (sply_dat->derivates) {
		SGFree(sply_dat->derivates);
		sply_dat->derivates = NULL;
	}

	sply_dat->numd = 0;
// new arrays for APPR spline

//change spline type
	sply_dat->sdtype = (SPLY_TYPE_IN)(sply_dat->sdtype & ~SPL_INT);
	sply_dat->sdtype = (SPLY_TYPE_IN)(sply_dat->sdtype | SPL_APPR);

//all weight put into 1 at the begining
	for( i=0; i<sply_dat->nump; i++ ) sply_dat->P[i].vertex[3] = 1.;
//	calculate_sply(sply_dat);
	return TRUE;
}

/*******************************************************************************
* Change spline type from APPR to INT
*/
BOOL change_APPR_INT(lpSPLY_DAT sply_dat){
short j = MAX_POINT_ON_SPLINE, i;

	if( sply_dat->sdtype & SPL_INT) return FALSE;
// free array for APPR spline
	if( sply_dat->P ){
		SGFree( sply_dat->P );
		sply_dat->P = NULL;
	}

// new arrays for short spline
	if((sply_dat->P = (W_NODE*)SGMalloc((2*j+2)*sizeof(W_NODE))) == NULL)return FALSE;
//all weight put into 1
	for( i=0; i<2*j+2; i++ ) sply_dat->P[i].vertex[3] = 1.;

	if((sply_dat->derivates = (SNODE*)SGMalloc(j*sizeof(SNODE))) == NULL) return FALSE;
	memset(sply_dat->derivates, 0, j*sizeof(SNODE));
	sply_dat->numd = 0;

	sply_dat->sdtype = (SPLY_TYPE_IN)(sply_dat->sdtype & ~SPL_APPR);
	sply_dat->sdtype = (SPLY_TYPE_IN)(sply_dat->sdtype | SPL_INT);
	calculate_sply(sply_dat);
	return TRUE;
}

/*******************************************************************************
* Change spline degree( for APPR spline ONLY!!!)
*/
BOOL change_degree(lpSPLY_DAT sply_dat, short new_p ){
	if( sply_dat->sdtype & SPL_INT ) return TRUE;
	if( new_p <= 0 ) return FALSE;
  if( new_p == sply_dat->degree ) return TRUE;
  if( new_p > sply_dat->degree )	if( !Increase_Spline_Degree( sply_dat, new_p)) return FALSE;
//  else                           	if( !Decrease_Spline_Degree( sply_dat, new_p)) return FALSE;
	return calculate_P(sply_dat, 0);
}

/*******************************************************************************
*  Change weight
*  num - number of point
*  new_w - new weight
*/
BOOL change_weight(lpSPLY_DAT sply_dat, short num, sgFloat new_w ){
sgFloat  w;
D_POINT point;

	if( sply_dat->sdtype & SPL_INT ) return TRUE;
	if( new_w<0 && new_w>1 ) return FALSE;
	Create_3D_From_4D( &sply_dat->P[num], (sgFloat *)&point, &w );
 	Create_4D_From_3D( &sply_dat->P[num], (sgFloat *)&point, new_w );
  return calculate_P(sply_dat, 0);
}

/*******************************************************************************
* Put break into spline knots
* if APR-spline - put sgFloat spline knot
*/
BOOL add_break_knots(lpSPLY_DAT sply_dat, short num ){
	if( sply_dat->sdtype & SPL_INT ) return TRUE;
  if( !Create_Multy_Point( sply_dat, sply_dat->u[num], 0 ) ) return FALSE;
	return TRUE;
}

/*******************************************************************************
* Extract break from spline point
* i.e. extract sgFloat spline knot
*/
BOOL del_break_knots(lpSPLY_DAT sply_dat, short num ){
	if( sply_dat->sdtype & SPL_INT ) return TRUE;
  if( !Del_Knot( sply_dat->U, sply_dat->P, sply_dat->u, &sply_dat->nump,
  							 sply_dat->numU, sply_dat->degree, sply_dat->u[num], sply_dat->degree) )
                 return FALSE;
	return calculate_P(sply_dat, 0);
}

/*******************************************************************************
* Calculate spline
*/
static BOOL calculate_sply(lpSPLY_DAT sply_dat){
	short  nnn, condition, i;
  sgFloat weight;

	nnn = (sply_dat->sdtype & SPL_INT) ? sply_dat->numk : sply_dat->nump;
	if (nnn > 2) {
//detect the spline type
		if( sply_dat->sdtype & SPL_CLOSE ){
		 if( sply_dat->numd == 0 ) condition=1; // closed without derivation
		 else{
			 if( sply_dat->derivates[0].num == 1 ) condition=2; //special case of closed spline
			 else condition=3; // closed with derivates
		 }
		}else{
		 if( sply_dat->numd == 0 ) condition=0; // without derivation
		 else condition=2; //with derivates
		}
//-------->>>>>>>>>>>>>>>>>>>>>>>>>>
//!!!!!!!!! , ..  .    
		parameter( sply_dat, condition, TRUE );
		return calculate_P(sply_dat, condition); //   
	} else if (sply_dat->sdtype & SPL_APPR) {
		sply_dat->numk = sply_dat->nump;
    for( i=0; i<sply_dat->nump; i++ )
		  Create_3D_From_4D( &sply_dat->P[i], (sgFloat *)&sply_dat->knots[i], &weight);
	}
	return TRUE;
}
