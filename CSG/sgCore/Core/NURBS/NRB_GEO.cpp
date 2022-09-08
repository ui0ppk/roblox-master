#include "../sg.h"
static BOOL Extract_Spline_Part(lpSPLY_DAT spline, sgFloat t1, sgFloat t2,
												 sgFloat **U_new1, lpW_NODE *P_new1, sgFloat **u_new1, 
												 short *k1, short *k2);
static void Create_Trim_Spline( sgFloat *U_new, lpW_NODE P_new, sgFloat *u_new,
																 short k1, short k2, short p, lpSPLY_DAT spline );
//------------------------------------------------------------------------->>>>>
//----------->>>Create break point into NURBS Curves <<<-----------
//------------------------------------------------------------------------->>>>>
/*******************************************************************************
* Insert nessesary knot into NURBS curve to create break point.
* That points are marked to edit together
* type - =0 - create multiply point to change curve form
*				 =1 - create break point to trim curve
* type   >20 - insert knot with multiplicity (type-20)
*/
BOOL Create_Multy_Point( lpSPLY_DAT spline, sgFloat t1, short type ){
BOOL		rt=FALSE;
short		nump_new, k, r, s;
sgFloat 	*U_new=NULL, *u_new=NULL;
W_NODE  *P_new=NULL;

//k  - position the t1 into  control vector
//s  - number of t1 into control vector
//r  - number of insertion of the parameter t1 to create break point
	if( !Detect_Multy_Insertion( spline->U, spline->degree, t1, spline->nump, &s, &k )) return FALSE;
  r=spline->degree-s;
  if( type == 1 ) r++;
  if( type > 20 ) r=type-20;
	if( r>0 ){ // need to insert knots
		if( !Insert_Knot( spline->U, spline->P, spline->u, spline->nump, spline->degree,
											t1, &U_new, &P_new, &u_new, &nump_new, r, k, s ) ) goto err;

// modify spline structure
    if( !Modify_Spline_Str( spline, U_new, nump_new+spline->degree+1,
														P_new, u_new, nump_new, spline->degree ) ) goto err;
  }
	rt=TRUE;
err:
	if( P_new ) SGFree( P_new );
	if( U_new ) SGFree( U_new );
  if( u_new ) SGFree( u_new );
	return rt;
}

//---------------------------------------------------------------->>>>>
//   ------------------>>>Get NURBS Curve Part<<<------------------
//---------------------------------------------------------------->>>>>
/**********************************************************
* get part of NURBS curve from parameter t1 to t2 by inserting
* multiplying knots, reparametrizat to [0,1] and covert to Geo
*/
BOOL Get_Part_Spline_Geo(lpSPLY_DAT spline, sgFloat t1, sgFloat t2,
												 lpGEO_SPLINE geo_spline){
BOOL			rt=FALSE;
short			bound1, bound2;
sgFloat 		*U_new1=NULL, *u_new1=NULL;
W_NODE  	*P_new1=NULL;
SPLY_DAT  sply_dat;

//extract spline part by inserting knots
//bound1 - the begin point of changing into control vector
//bound2 - the end   point of changing into control vector
	if( !Extract_Spline_Part(spline, t1, t2, &U_new1, &P_new1, &u_new1, &bound1, &bound2 ) ) goto err0;
	Create_Trim_Spline( U_new1, P_new1, u_new1, bound1, bound2,	spline->degree, &sply_dat );
//reparametrization to [0,1]
  if( !Reparametrization(	sply_dat.nump, sply_dat.numU, 0, 1, spline->degree,
  												sply_dat.U, sply_dat.P, sply_dat.u )) goto err;
	if( !create_geo_sply(&sply_dat, geo_spline )) goto err;
	rt=TRUE;
err:
	free_sply_dat( &sply_dat );
err0:
	if( P_new1 ) SGFree( P_new1 );
  if( U_new1 ) SGFree( U_new1 );
  if( u_new1 ) SGFree( u_new1 );
	return rt;
}

/**********************************************************
* get part of NURBS curve from parameter t1 to t2 by inserting
* multiplying knots and create new sply_dat with param [t1 t2]
*/
BOOL Get_Part_Spline_Dat(lpSPLY_DAT spline, sgFloat t1, sgFloat t2,
												 lpSPLY_DAT sply_dat){
BOOL			rt=FALSE;
short			bound1, bound2;
sgFloat 		*U_new1=NULL, *u_new1=NULL;
W_NODE  	*P_new1=NULL;

//extract spline part by inserting knots
	if( !Extract_Spline_Part(spline, t1, t2, &U_new1, &P_new1, &u_new1, &bound1, &bound2 ) ) goto err;
//create new spline
	Create_Trim_Spline( U_new1, P_new1, u_new1, bound1, bound2, spline->degree, sply_dat );
	rt=TRUE;
err:
	if( P_new1 ) SGFree( P_new1 );
	if( U_new1 ) SGFree( U_new1 );
  if( u_new1 ) SGFree( u_new1 );
	return rt;
}

/*******************************************************************
* Extract part of spline [t1, t2]
*/
static BOOL Extract_Spline_Part(lpSPLY_DAT spline, sgFloat t1, sgFloat t2,
																sgFloat **U_new1, lpW_NODE *P_new1, sgFloat **u_new1,
																short *bound1, short *bound2 ){
BOOL		rt=FALSE;
short			i, nump_new1, nump_new2, k, r, s;
sgFloat 	*U_new2=NULL, *u_new2=NULL;
W_NODE  *P_new2=NULL;

//insert knots on the first end
	if( (*P_new1=(W_NODE *)SGMalloc(spline->nump*sizeof(W_NODE))) == NULL ) goto err;
	if( (*U_new1=(sgFloat *)SGMalloc(spline->numU*sizeof(sgFloat))) == NULL ) goto err;
	if( (*u_new1=(sgFloat *)SGMalloc(spline->nump*sizeof(sgFloat))) == NULL ) goto err;

//convert into 4D-space
	for( i=0; i<spline->nump; i++ ){
  	memcpy( &((*P_new1)[i]), &spline->P[i], sizeof(W_NODE) );
		(*u_new1)[i]=spline->u[i];
  }
	for( i=0; i<spline->numU; i++ ) (*U_new1)[i]=spline->U[i];
	nump_new1=spline->nump;

//k  - position the t1 into  control vector
//s  - number of t1 into control vector
//r  - number of insertion of the parameter t1
	if( !Detect_Multy_Insertion( *U_new1, spline->degree, t1, nump_new1, &s, &k )) goto err;
	if( (r=spline->degree-s+1)>0 ){ // need to insert knots
		if( !Insert_Knot( *U_new1, *P_new1, *u_new1, nump_new1, spline->degree,
											t1, &U_new2, &P_new2, &u_new2, &nump_new2, r, k, s ) ) goto err;
		SGFree(*U_new1); *U_new1=U_new2; U_new2=NULL;
		SGFree(*u_new1); *u_new1=u_new2; u_new2=NULL;
		SGFree(*P_new1); *P_new1=P_new2; P_new2=NULL;
		nump_new1=nump_new2;
// for left end only
		(*bound1)=k-s+1;
	} else (*bound1)=k;

//insert knots on the second end
	if( !Detect_Multy_Insertion( *U_new1, spline->degree, t2, nump_new1, &s, &k )) goto err;
	if( (r=spline->degree-s+1)>0 ){
		if( !Insert_Knot( *U_new1, *P_new1, *u_new1, nump_new1, spline->degree,
											t2, &U_new2, &P_new2, &u_new2, &nump_new2, r, k, s ) ) goto err;
		SGFree(*U_new1); *U_new1=U_new2; U_new2=NULL;
		SGFree(*u_new1); *u_new1=u_new2; u_new2=NULL;
		SGFree(*P_new1); *P_new1=P_new2; P_new2=NULL;
 //		nump_new1=nump_new2;
// for right end only
		(*bound2)=k+r;
	} else (*bound2)=k;
	if( s==(spline->degree+1) ) (*bound2)+=spline->degree;
	rt=TRUE;
err:
	if( P_new2 ) SGFree( P_new2 );
	if( U_new2 ) SGFree( U_new2 );
	if( u_new2 ) SGFree( u_new2 );  
	return rt;
}

/**********************************************************
* create new spline respect to part of intial spline  [t1,t2]
*/
static void Create_Trim_Spline( sgFloat *U_new, lpW_NODE P_new, sgFloat *u_new,
																short k1, short k2, short p, lpSPLY_DAT sply_dat ){
short 		 i, j;

//initialization of new spline
	init_sply_dat(SPL_NEW, p, SPL_APPR, sply_dat );

//write points and weights
	sply_dat->nump = sply_dat->numk = k2 - p - k1;

	if (sply_dat->allocSizeFor_P<sply_dat->nump &&
		sply_dat->P) 
	{
		SGFree(sply_dat->P);
		sply_dat->P = (W_NODE*)SGMalloc(sply_dat->nump*sizeof(W_NODE));
		sply_dat->allocSizeFor_P = sply_dat->nump;
	}

	if (sply_dat->allocSizeFor_u<sply_dat->nump &&
		sply_dat->u) 
	{
		SGFree(sply_dat->u);
		sply_dat->u = (sgFloat*)SGMalloc(sply_dat->nump*sizeof(sgFloat));
		sply_dat->allocSizeFor_u = sply_dat->nump;
	}

	for(i=k1, j=0; i<=k2-p-1; i++, j++)
	{
		if (j<sply_dat->allocSizeFor_P)
  			memcpy( &sply_dat->P[j], &P_new[i], sizeof(W_NODE) );
		else
			assert(0);
		if (j<sply_dat->allocSizeFor_u)
			sply_dat->u[j]=u_new[i];
		else
			assert(0);
	}

// write vector
	sply_dat->numU = sply_dat->nump+p+1;

	if (sply_dat->allocSizeFor_U<sply_dat->numU &&
		sply_dat->U) 
	{
		SGFree(sply_dat->U);
		sply_dat->U = (sgFloat*)SGMalloc(sply_dat->numU*sizeof(sgFloat));
		sply_dat->allocSizeFor_U = sply_dat->numU;
	}

	for(i=k1, j=0; i<=k2; i++, j++ )	
		if (j<sply_dat->allocSizeFor_U)
			sply_dat->U[j]=U_new[i];
		else
			assert(0);
}

/**********************************************************
* create new NURBS curve from two splines( sply_dat2+sply_dat1)
* by merge knot's vectors and P-vectors and deleting of the
* multiplying knots degree of the splines must be !!! equal
*/
BOOL Two_Spline_Union_Dat(lpSPLY_DAT sply_dat1, lpSPLY_DAT sply_dat2,
													lpSPLY_DAT sply_new){


	/*	BOOL        ret;
		short       i, d, new_nump; //,r;
		sgFloat      delta;
		sgFloat      *U_new1, *u_new1;//, *U_new2=NULL;
		W_NODE      *P_new1;//, *P_new2=NULL;

		//write points and weights from first spline
		if( (P_new1=(W_NODE *)SGMalloc((sply_dat1->nump+sply_dat2->nump)*sizeof(W_NODE))) == NULL ) return FALSE;
		memcpy( P_new1, sply_dat2->P, sply_dat2->nump*sizeof(W_NODE) );
		//add second spline into new one
		for( i=0; i<sply_dat1->nump; i++ ) P_new1[sply_dat2->nump+i-1] = sply_dat1->P[i];

		// write parameters
		if( (u_new1=(sgFloat *)SGMalloc((sply_dat1->nump+sply_dat2->nump)*sizeof(sgFloat))) == NULL ) goto err;
		memcpy( u_new1, sply_dat2->u, sply_dat2->nump*sizeof(sgFloat) );
		delta=sply_dat2->u[sply_dat2->nump-1];
		for( i=0; i<sply_dat1->nump; i++ ) u_new1[sply_dat2->nump+i-1] = sply_dat1->u[i]+delta;

		// write vector
		d=sply_dat1->degree+1;
		if( (U_new1=(sgFloat *)SGMalloc((sply_dat1->numU+sply_dat2->numU)*sizeof(sgFloat))) == NULL ) goto err;
		memcpy( U_new1, sply_dat2->U, sply_dat2->numU*sizeof(sgFloat) );
		delta=sply_dat2->U[sply_dat2->numU-1];
		for(i=d; i<sply_dat1->numU; i++ ) U_new1[sply_dat2->numU+i-d-1]=sply_dat1->U[i]+delta;
		
		new_nump=sply_dat1->nump+sply_dat2->nump-1;
		
		//initialization of new spline
		init_sply_dat(SPL_NEW, sply_dat1->degree, SPL_APPR, sply_new );

		sply_new->nump = sply_new->numk = new_nump;
		memcpy( sply_new->P, P_new1, new_nump*sizeof(W_NODE) );
		memcpy( sply_new->u, u_new1,  new_nump*sizeof(sgFloat) );

		sply_new->numU = new_nump+sply_dat1->degree+1;
		memcpy( sply_new->U, U_new1, sply_new->numU*sizeof(sgFloat) );
		//  SGFree( U_new2 ); SGFree( P_new2 );
		ret=TRUE;
err:
		if(U_new1) SGFree( U_new1 );
		if(u_new1) SGFree( u_new1 );
		if(P_new1) SGFree( P_new1 );
		return ret;

*/


BOOL		 		ret=FALSE;
short			 	i, d, new_nump; //,r;
sgFloat			delta;
sgFloat 			*U_new1, *u_new1;
W_NODE  		*P_new1;

	P_new1 = NULL;
	u_new1 = NULL;
	U_new1 = NULL;

//write points and weights from first spline
  if( (P_new1=(W_NODE *)SGMalloc((sply_dat1->nump+sply_dat2->nump)*sizeof(W_NODE))) == NULL ) return FALSE;
  memcpy( P_new1, sply_dat2->P, sply_dat2->nump*sizeof(W_NODE) );
//add second spline into new one
  for( i=0; i<sply_dat1->nump; i++ ) 
	  P_new1[sply_dat2->nump+i-1] = sply_dat1->P[i];

// write parameters
	if( (u_new1=(sgFloat *)SGMalloc((sply_dat1->nump+sply_dat2->nump)*sizeof(sgFloat))) == NULL ) goto err;
  memcpy( u_new1, sply_dat2->u, sply_dat2->nump*sizeof(sgFloat) );
  delta=sply_dat2->u[sply_dat2->nump-1];
  for( i=0; i<sply_dat1->nump; i++ ) u_new1[sply_dat2->nump+i-1] = sply_dat1->u[i]+delta;

// write vector
	d=sply_dat1->degree+1;
	if( (U_new1=(sgFloat *)SGMalloc((sply_dat1->numU+sply_dat2->numU)*sizeof(sgFloat))) == NULL ) goto err;
  memcpy( U_new1, sply_dat2->U, sply_dat2->numU*sizeof(sgFloat) );
	delta=sply_dat2->U[sply_dat2->numU-1];
	for(i=d; i<sply_dat1->numU; i++ ) U_new1[sply_dat2->numU+i-d-1]=sply_dat1->U[i]+delta;
//try to delete multiply points
  new_nump=sply_dat1->nump+sply_dat2->nump-1;
//	r=Del_Knot( U_new1, P_new1, &new_nump,
//						 	sply_dat1->numu+sply_dat2->numu-d-1, sply_dat1->degree,
//							1., d);

//initialization of new spline
	init_sply_dat(SPL_NEW, sply_dat1->degree, SPL_APPR, sply_new );

	sply_new->nump = sply_new->numk = new_nump;
	// RA -      
	// ( init_sply_dat    250 )
	//   .    ?
    // RA - memcpy( sply_new->P, P_new1, new_nump*sizeof(W_NODE) );
	// RA - memcpy( sply_new->u, u_new1,  new_nump*sizeof(sgFloat) );

	//RA Begin
	if (sply_new->allocSizeFor_knots<sply_new->numk &&
		sply_new->knots)
	{
		SGFree(sply_new->knots);
		sply_new->knots = (DA_POINT*)SGMalloc(sply_new->numk*sizeof(DA_POINT));
		sply_new->allocSizeFor_knots = sply_new->numk;
	}
	memset(sply_new->knots,0,sply_new->allocSizeFor_knots*sizeof(DA_POINT));

	if (sply_new->allocSizeFor_P<new_nump &&
		sply_new->P) 
	{
		SGFree(sply_new->P);
		sply_new->P = (W_NODE*)SGMalloc(new_nump*sizeof(W_NODE));
		sply_new->allocSizeFor_P = new_nump;
	}
	memcpy( sply_new->P, P_new1, new_nump*sizeof(W_NODE) );

	if (sply_new->allocSizeFor_u<new_nump &&
		sply_new->u) 
	{
		SGFree(sply_new->u);
		sply_new->u = (sgFloat*)SGMalloc(new_nump*sizeof(sgFloat));
		sply_new->allocSizeFor_u = new_nump;
	}
	memcpy( sply_new->u, u_new1,  new_nump*sizeof(sgFloat) );
	//RA End

	sply_new->numU = new_nump+sply_dat1->degree+1;
	// RA - memcpy( sply_new->U, U_new1, sply_new->numu*sizeof(sgFloat) );
	
	//RA Begin
	if (sply_new->allocSizeFor_U<sply_new->numU &&
		sply_new->U) 
	{
		SGFree(sply_new->U);
		sply_new->U = (sgFloat*)SGMalloc(sply_new->numU*sizeof(sgFloat));
		sply_new->allocSizeFor_U = sply_new->numU;
	}
	memcpy( sply_new->U, U_new1, sply_new->numU*sizeof(sgFloat) );
	//RA End

	ret=TRUE;

err:
	if(U_new1) SGFree( U_new1 );
    if(u_new1) SGFree( u_new1 );
	if(P_new1) SGFree( P_new1 );
	return ret;
}

//---------------------------------------------------------------->>>>>
//   ----------------->>>Increase NURBS Degree<<<------------------
//---------------------------------------------------------------->>>>>
/**********************************************************
* Increase spline degree
*/
BOOL Increase_Spline_Degree(lpSPLY_DAT sply_dat, short new_degree){
BOOL		rt=FALSE;
short 		i, j, k, l, m, r, s, k1, add, m_del;
short			nump_new1, nump_new2, degree_plus, *r_insert=NULL;
sgFloat 	*U_new1, *u_new1, *U_new2=NULL, *u_new2=NULL, *par_insert=NULL;
W_NODE  *P_new1, *P_new2=NULL;

	add=new_degree-sply_dat->degree;
	degree_plus=sply_dat->degree+1;

	if( (P_new1=(W_NODE *)SGMalloc(sply_dat->nump*sizeof(W_NODE))) == NULL) return FALSE;
	if( (u_new1=(sgFloat *)SGMalloc(sply_dat->nump*sizeof(sgFloat))) == NULL) goto err;
	for( i=0; i<sply_dat->nump; i++ ){
		memcpy( &P_new1[i], &sply_dat->P[i], sizeof(W_NODE) );
    u_new1[i]=sply_dat->u[i];
  }

	if( sply_dat->nump==degree_plus ){ //It's Beze Curve already!
//increase degree for composite curve
		nump_new2=0;
		Increase_Composite_Curve_Degree( P_new1, &P_new2, u_new1, &u_new2, degree_plus,
    																 &nump_new2, sply_dat->degree, add);
//create new control knots vector
		if( (U_new1=(sgFloat *)SGMalloc((nump_new2+new_degree+1)*sizeof(sgFloat)))==NULL) goto err;
		for(i=0; i<new_degree+1; i++) { U_new1[i]=0.; U_new1[i+new_degree+1]=1.;}
		k=2*(new_degree+1);
	}else{ // It's not Beze Curve - create composite curve from initial curve
		U_new1=(sgFloat *)SGMalloc(sply_dat->numU*sizeof(sgFloat));
		for( i=0; i<sply_dat->numU; i++ ) U_new1[i]=sply_dat->U[i];
		nump_new1=sply_dat->nump;

		m=0;
		if((r_insert  =(short *)SGMalloc((sply_dat->nump-degree_plus)*sizeof(short)))==NULL) goto err;
		if((par_insert=(sgFloat *)SGMalloc((sply_dat->nump-degree_plus)*sizeof(sgFloat)))==NULL) goto err1;

//insert knots on the each parameter
		for(i=degree_plus, j=1; j<=sply_dat->nump-degree_plus; i++, j++	) {
//detect r=number of the insertions for j-th knot
			if( !Detect_Multy_Insertion( U_new1, sply_dat->degree, sply_dat->U[i],
																	 nump_new1, &s, &k1)) goto err1;
			if( (r=sply_dat->degree-s+1)>0 ){ //if this node must be insert to create Beze Curve
				r_insert[m]=r;
				par_insert[m++]=sply_dat->U[i];

				if( !Insert_Knot( U_new1, P_new1, u_new1, nump_new1, sply_dat->degree,
													sply_dat->U[i], &U_new2, &P_new2, &u_new2, &nump_new2, r, k1, s ) ) goto err;

				SGFree(U_new1); U_new1=U_new2; U_new2=NULL;
        SGFree(u_new1); u_new1=u_new2; u_new2=NULL;
				SGFree(P_new1); P_new1=P_new2; P_new2=NULL;
				nump_new1=nump_new2;	nump_new2=0;
			}
		}
//increase degree for composite curve
		Increase_Composite_Curve_Degree(P_new1, &P_new2, u_new1, &u_new2, nump_new1,
    																&nump_new2, sply_dat->degree, add);
		k=0;
		SGFree(U_new1);
//create new control knots vector
		if( (U_new1=(sgFloat *)SGMalloc((nump_new2+new_degree+1)*sizeof(sgFloat)))==NULL) goto err1;

		for(i=0; i<new_degree+1; i++) U_new1[k++]=0.;
		for( j=1; j<=sply_dat->nump-degree_plus; j+=s	) {
//detect multiplisity
			for( s=0, l=1; l<=sply_dat->nump-degree_plus; l++ )
				if( fabs(sply_dat->U[j+sply_dat->degree]-sply_dat->U[l+sply_dat->degree]) < eps_n ) s++;
			for( l=0; l<new_degree+1; l++ )	U_new1[k++]=sply_dat->U[j+sply_dat->degree];
		}
		for(i=0; i<new_degree+1; i++)	 U_new1[k++]=1.;

//deleting if the inserted knots
		m_del=0;
		for( i=0; i<m; i++ )
			m_del+=Del_Knot( U_new1, P_new2, u_new2, &nump_new2, nump_new2+new_degree+1,
										 new_degree, par_insert[i], r_insert[i]);

		k-=m_del;
		SGFree( par_insert ); par_insert=NULL;
		SGFree( r_insert );     r_insert=NULL;
	}

//modify spline structure for increase degree
  if( !Modify_Spline_Str( sply_dat, U_new1, k, P_new2, u_new2, nump_new2, new_degree ) ) goto err;
	rt=TRUE;

err1:
	if( par_insert ) SGFree( par_insert ); 	if( r_insert ) SGFree( r_insert );
err:
	if( P_new1 ) SGFree( P_new1 ); 	if( P_new2 ) SGFree( P_new2 );
	if( U_new1 ) SGFree( U_new1 );  if( U_new2 ) SGFree( U_new2 );
  if( u_new1 ) SGFree( u_new1 );  if( u_new2 ) SGFree( u_new2 );
	return rt;
}

//---------------------------------------------------------------->>>>>
// ------------->>>NURBS Curve reparametrization<<<------------------
//---------------------------------------------------------------->>>>>
/**********************************************************
* [ab] - initial interval i.e [ab]->[cd]
* [cd] - rezalting interval
* p    - degree
* U		 - control parametrical vector
*/
BOOL Reparametrization(	short nump, short numu,	sgFloat c, sgFloat d, short p,
												sgFloat *U, lpW_NODE P, sgFloat *u ){
short i, j;
sgFloat w0, wn, Gamma, Alfa, S, t1, t2, a, b;
	a=U[0];
  b=U[numu-1];
	w0=pow((double)P[0].vertex[3], (double)1./p);
	wn=pow((double)P[nump-1].vertex[3], (double)1./p);
	for( i=0; i<numu; i++ ){
		t1=w0*(b-U[i]); t2=wn*(U[i]-a);
		if( (Alfa =( t1 + t2 ) ) < eps_n ) return FALSE;
		U[i] = ( t1*c + t2*d )/Alfa;
	}

  a=u[0];
  b=u[nump-1];
	for( i=0; i<nump; i++ ){
		t1=w0*(b-u[i]); t2=wn*(u[i]-a);
		if( (Alfa =( t1 + t2 ) ) < eps_n ) return FALSE;
		u[i] = ( t1*c + t2*d )/Alfa;
	}
//	Gamma = wn-w0;
//	Alfa  = wn*d-w0*c;
	Gamma = 0;
	Alfa  = d-c;
	for( i=0; i<nump; i++ ){
		for( S=1., j=1; j<=p; j++ ) S = S*( Gamma*U[i+j] - Alfa );
		P[i].vertex[3] *= S;
	}
	return TRUE;
}

//---------------------------------------------------------------->>>>>
//   ------------->>>NURBS Curve reorientation<<<------------------
//---------------------------------------------------------------->>>>>
/**********************************************************
* reorientation of spline
*/
BOOL Reorient_Spline( lpSPLY_DAT sply_dat ){
short 		ii;
sgFloat		*U;
lpW_NODE  P_tmp;

	if( (P_tmp=(W_NODE *)SGMalloc( sply_dat->nump*sizeof(W_NODE)))==NULL) return FALSE;
	memcpy(P_tmp, sply_dat->P, sply_dat->nump*sizeof(W_NODE));
	for( ii=0; ii<sply_dat->nump; ii++)
		memcpy( &sply_dat->P[ii], &P_tmp[sply_dat->nump-1-ii], sizeof(W_NODE));
	SGFree(P_tmp);

	if( (U=(sgFloat *)SGMalloc( sply_dat->numU*sizeof(sgFloat)))==NULL)	goto err;
	memcpy(U, sply_dat->U, sply_dat->numU*sizeof(sgFloat));
	for( ii=0; ii<sply_dat->numU; ii++){
		U[ii]-=1.;
		sply_dat->U[sply_dat->numU-1-ii]=fabs(U[ii]);
	}
	memcpy(U, sply_dat->u, sply_dat->nump*sizeof(sgFloat));
	for( ii=0; ii<sply_dat->nump; ii++){
		U[ii]-=1.;
		sply_dat->u[sply_dat->nump-1-ii]=fabs(U[ii]);
	}
	SGFree(U);
  return TRUE;
err:
	SGFree(P_tmp);
	return FALSE;
}

//---------------------------------------------------------------->>>>>
// ------------->>>NURBS Modify work structure  <<<------------------
//---------------------------------------------------------------->>>>>
/**********************************************************
* Modify NURBS by new control points and vector
*/
BOOL Modify_Spline_Str( lpSPLY_DAT sply_dat, sgFloat *U_new, short numu,
	  										lpW_NODE P_new, sgFloat *u_new, short nump, short degree ){
//write control vector
	if( numu>MAX_POINT_ON_SPLINE ){
  	SGFree( sply_dat->U );
    if( (sply_dat->U=(sgFloat *)SGMalloc(numu*sizeof(sgFloat)))==NULL ) return FALSE;
  }
	memcpy( sply_dat->U, U_new, numu*sizeof(sgFloat) );
	sply_dat->numU=numu;

//write control points
	if( nump>MAX_POINT_ON_SPLINE ){
  	SGFree( sply_dat->P );
    SGFree( sply_dat->knots );
    SGFree( sply_dat->u );
    if( (sply_dat->P=(W_NODE *)SGMalloc(nump*sizeof(W_NODE)))==NULL) return FALSE;
    if( (sply_dat->knots=(DA_POINT*)SGMalloc(nump*sizeof(DA_POINT)))==NULL) return FALSE;
	  if( (sply_dat->u=(sgFloat *)SGMalloc(nump*sizeof(sgFloat)))==NULL ) return FALSE;
  }
	memcpy( sply_dat->P, P_new, nump*sizeof(W_NODE) );
	memcpy( sply_dat->u, u_new, nump*sizeof(sgFloat) );
	sply_dat->nump=nump;

	sply_dat->degree = degree;
	calculate_P(sply_dat, 0);
  return TRUE;
}
