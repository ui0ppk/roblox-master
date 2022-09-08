#include "../sg.h"
static BOOL NURBS_Knot_Ins( short nump, short p, sgFloat *U, lpW_NODE P, sgFloat *u, sgFloat t,
														 short k, short s, short r, sgFloat *U_new, lpW_NODE P_new, sgFloat *u_new);
/*static short NURBS_Knot_Del( sgFloat *U, lpW_NODE P, short *nump, short d, sgFloat t,
														 short k, short s, short r);
//														 sgFloat *U_new, lpW_NODE P_new, short *nump_new,
*/
static void Increase_Beze_Degree(lpW_NODE beze1, sgFloat *u1, lpW_NODE beze2, sgFloat *u2,
                                 short degree, short add );
static sgFloat c_beze(short k1, short k2);
static short factor(short k);

/*******************************************************************************
* Main function for multy inserting knot
* U    - initial control vector
* P    - initial control points
* nump -number of control points
* p    - degree of spline
* t     - parameter of the inserting knot
* U_new - new control vector
* P_new - new control points
* nump_new - number of new control points
* r     - number of insertions to create break point
* k     - number of span into control vector for new knot
* s			- number of existing inserting knot
* type - =0 - insert full break point
*        =1 - insert knot r times
*/
BOOL Insert_Knot( sgFloat *U, lpW_NODE P, sgFloat *u, short nump, short p, sgFloat t,
									sgFloat **U_new, lpW_NODE *P_new, sgFloat **u_new, short *nump_new,
									short r, short k, short s ){
short 	num;

	num = nump + p + 1;

//number of inserting points = p-s+r-1;
	*nump_new=nump+r;
	if( (*U_new = (sgFloat*)SGMalloc( ( num + r )*sizeof(sgFloat) ) ) == NULL ) return FALSE;
	if( (*P_new = (W_NODE *)SGMalloc( ( *nump_new )*sizeof(W_NODE) ) ) == NULL ) return FALSE;
	if( (*u_new = (sgFloat*)SGMalloc( ( *nump_new )*sizeof(sgFloat) ) ) == NULL ) return FALSE;

//insert nessesary number of knots
	NURBS_Knot_Ins( nump, p, U, P, u, t, k, s, r, *U_new, *P_new, *u_new );
	return TRUE;
}

/*******************************************************************************
* multiplicity insert ( r times ) knot t
* nump - length of P
* d    - degree
* U    - char. vector
* P    - points of the char. poligon
* t    - knot to be insert
* k    - number of knot to be insert into vector U
* s    - multiplicity of existing knot t into vector U ( may be =0 )
* r    - multiplicity of insertinon
* U_new, P_New - new vector and poligon
* type - =0 - insert full break point
*        =1 - insert knot r times
*/
static BOOL NURBS_Knot_Ins( short nump, short d, sgFloat *U, lpW_NODE P, sgFloat *u,
 														sgFloat t, short k, short s, short r,
														sgFloat *U_new, lpW_NODE P_new, sgFloat *u_new){
BOOL		 rt=FALSE;
short  	 i, j, mp, jjj, jj, L;//, number_t;
sgFloat   alpha, *uu;
lpW_NODE PP;

	if( (PP = (lpW_NODE)SGMalloc((d+1)*sizeof(W_NODE))) == NULL ) return FALSE;
	if( (uu = (sgFloat*)SGMalloc((d+1)*sizeof(sgFloat))) == NULL ) goto err;
	mp = nump + d + 1;

//create new knot vector
	for( i=0; 	i<=k;  i++ ) U_new[i]   = U[i];
	for( i=1; 	i<=r;  i++ ) U_new[k+i] = t;
	for( i=k+1; i<mp;  i++ ) U_new[i+r] = U[i];

//save knot points
	for( i=0; 	i<=k-d; i++ ) {P_new[i]   = P[i]; 		u_new[i]   = u[i];}
	for( i=k-s; i<nump; i++ ) {P_new[i+r] = P[i]; 		u_new[i+r] = u[i];}
	for( i=0; 	i<=d-s; i++ ) {PP[i]		  = P[k-d+i]; uu[i]		   = u[k-d+i];}

	L=k-d;
	jjj=(r+s==d+1)?(r):(r+1);
//  jjj=r;
//insert knor r times
	for( j=1; j<jjj; j++ ){
		L = k-d+j;
		for( i=0; i<=d-j-s; i++ ){
			alpha = (t-U[L+i])/(U[i+k+1]-U[L+i]);
			for( jj=0; jj<4; jj++ )
				PP[i].vertex[jj]=alpha*PP[i+1].vertex[jj] + (1.-alpha)*PP[i].vertex[jj];
      uu[i]=alpha*uu[i+1] + (1.-alpha)*uu[i];
		}
		P_new[L] = PP[0];
    if( j==jjj-1 ) u_new[L] = t;
    else				   u_new[L] = uu[0];
		P_new[k+r-j-s] = PP[d-j-s];  u_new[k+r-j-s] = uu[d-j-s];
	}

	for( i=L+1; i<k-s; i++ ) { P_new[i] = PP[i-L]; u_new[i] = uu[i-L];}

//  L=-1;
//	for( i=0; i<mp+r; i++ )
//  	if( fabs(U_new[i]-t)<eps_n && fabs(U_new[i]-U_new[i+1])<eps_n){
//    	L=i;
//      break;
//    }
//  if( L>0 ) u_new[L-1]=t;
//	for( i=0; i<nump+d-s; i++ ) u_new[i]=(U_new[i+1]+U_new[i+2])/2.;

  rt=TRUE;
  SGFree(uu);
err:
	SGFree(PP);
	return rt;
}

//-------------------------------------------------------------------------->>>>
/*******************************************************************************
* multiplicity delete ( r times ) knot t
* nump - length of P
* d    - degree
* U    - char. vector
* P    - points of the char. poligon
* t    - knot to be delete
* k    - number of knot to be delete into vector U
* s    - multiplicity of existing knot t into vector U ( may be =0 )
* r    - multiplicity of deleting
* U_new, P_New - new vector and poligon
*/
short Del_Knot( sgFloat *U, lpW_NODE P, sgFloat *u, short *nump, short numu, short d,
                sgFloat t, short r ){
short  i, i1, j, j1, k, s, del=0;
short  iii;
sgFloat alphai, alphaj;
W_NODE node;

  for( j1=0; j1<r; j1++ ){
//detect number of knot to be deleted
	  s=0; k=-1;
		for( i=0; i<numu; i++ )  if( fabs(t-U[i])<eps_n ) {	k=i;	s++; }
		if( k<0 ) return del;

//try to delete knot with number <k> r times
//	return(NURBS_Knot_Del( U, P, nump, d, t, k, s, r));
		i=k-d; j=k-s;
	  while(j-i>0){
			alphai=(t-U[i])/(U[i+d+1]-U[i]);
			alphaj=(t-U[j])/(U[j+d+1]-U[j]);
			for( iii=0; iii<4; iii++ ){
				P[i].vertex[iii]=(P[i].vertex[iii]-(1-alphai)*P[i-1].vertex[iii])/alphai;
    	  P[j].vertex[iii]=(P[j].vertex[iii]-alphaj*P[j+1].vertex[iii])/(1-alphaj);
			}
  	  u[i]=(u[i]-(1-alphai)*u[i-1])/alphai;
    	u[j]=(u[j]-alphaj*u[j+1])/(1-alphaj);
			i++; j--;
  	}
		if( dpoint_distance_4d(&P[i-1], &P[j+1])<eps_d ){
     	for( i1=i+1; i1<*nump; i1++ ){
       	memcpy(&P[i1-1], &P[i1], sizeof(W_NODE));
        u[i1-1]=u[i1];
      }
      (*nump)--;
      for( i1=k; i1<numu-1; i1++ ) U[i1]=U[i1+1];
      numu--;
      del++;
  	}else{
	    alphai=(t-U[i])/(U[i+d+1]-U[i]);
    	for( iii=0; iii<4; iii++ )
				node.vertex[iii]=alphai*P[i+1].vertex[iii]+(1-alphai)*P[i-1].vertex[iii];
      if( dpoint_distance_4d(&P[i], &node)<eps_d ){
    		for( i1=i+1; i1<*nump; i1++ ){
      	 	memcpy(&P[i1-1], &P[i1], sizeof(W_NODE));
        	u[i1-1]=u[i1];
	       }
  	    (*nump)--;
    	  for( i1=k; i1<numu; i1++ ) U[i1-1]=U[i1];
      	numu--;
        del++;
      }
    }
//    if( del < j1+1 ) return 0;
  }
  return del;
}

/**********************************************************
* multiplicity delete ( r times ) knot t
* nump - length of P
* d    - degree
* U    - char. vector
* P    - points of the char. poligon
* t    - knot to be delete
* k    - number of knot to be delete into vector U
* s    - multiplicity of existing knot t into vector U ( may be =0 )
* r    - multiplicity of deleting
* U_new, P_New - new vector and poligon
* nump_new     - length of P_new
*/
/*static short NURBS_Knot_Del( sgFloat *U, lpW_NODE P, short *nump, short d, sgFloat t,
														short k, short s, short r){
short 			i, j, l, m, n=0, iii;
short				first, last;
sgFloat	  alphai, alphaj;
W_NODE	 	*point, p;

	if( (point = SGMalloc((*nump)*sizeof(W_NODE))) == NULL ) return 0;
	for( i=0; i<*nump; i++ )	point[i]=P[i];

//	memset(point, 0, nump*sizeof(W_NODE));

	first=k-d+1;
	last =k-s-1;
//	if (s==d+1) last++;

	for( l=1; l<=r; l++ ){
		j=++last;
		i=--first;
		while(j-i>l-1){
			alphai=(t-U[i])/(U[i+d+l]-U[i]);
			alphaj=(t-U[j-l+1])/(U[j+d+1]-U[j-l+1]);
			for( iii=0; iii<4; iii++ ){
				point[i].vertex[iii]=(point[i].vertex[iii]-(1-alphai)*point[i-1].vertex[iii])/alphai;
				point[j].vertex[iii]=(point[j].vertex[iii]-alphaj*point[j+1].vertex[iii])/(1-alphaj);
			}
			i++; j--;
		}
		if(dpoint_distance_4dl(&point[i-1], &point[j+1])<eps_d){
//delete point and knot
			for (m=i-1; m<*nump-1;     m++) point[m]=point[m+1];
			for (m=k;   m<*nump+d+1-1; m++) U[m]=U[m+1];
			//k--; s--; r--;
			(*nump)--; n++;
		}else{
			alphai=(t-U[i])/(U[i+d+1+l]-U[i]);
			for( iii=0; iii<4; iii++ ) p.vertex[iii]=alphai*point[i+l+1].vertex[iii]+
																							 (1-alphai)*point[i-1].vertex[iii];
			if(fabs(dpoint_distance_4dl(&P[i], &p))<=eps_d){
//delete point and knot
				for (m=i-1; m<*nump-1;     m++) point[m]=point[m+1];
				for (m=k;   m<*nump+d+1-1; m++) U[m]=U[m+1];
				//k--; s--; r--;
				(*nump)--; n++;
			}
		}
	}

//	if(n==0) goto err; // imposible to delete this knot

// modify control vector
	for( i=0; i<*nump; i++) P[i]=point[i];
//err:
	SGFree(point);
	return (n);
}
*/

//------------------------------------------------------------------------>>>>>>
/*******************************************************************************
*  create the Beze from the composite curve
*	 P_new1 - initial control points
*  P_new2 - rezulting control points
*  span   - number of span
*  degree - initial degree
*  add    - increase degree on add
*/
BOOL Increase_Composite_Curve_Degree(lpW_NODE P_new1, lpW_NODE *P_new2,
																		 sgFloat *u_new1, sgFloat **u_new2,
																			short nump_new1, short *nump_new2,
                                      short degree, short add){
short			j, k, j1, j2, nump;
sgFloat		*u1=NULL, *u2=NULL;
lpW_NODE  beze1/*=NULL*/, beze2=NULL;

	if( (*P_new2 = (W_NODE *)SGMalloc( ( 2*nump_new1)*sizeof(W_NODE) ) ) == NULL ) return FALSE;
	if( (*u_new2 = (sgFloat*)SGMalloc( ( 2*nump_new1)*sizeof(sgFloat) ) ) == NULL ) return FALSE;

	j1=j2=0;
	while( j2<nump_new1-1 ){
		for( j=j1+1; j<nump_new1-1; j++)
			if( (dpoint_distance_4d( &P_new1[j], &P_new1[j+1] )) < eps_d ) break;
		j2=j;
		nump=j2-j1+1;

		if( (beze1 = (W_NODE *)SGMalloc( nump*sizeof(W_NODE) ) ) == NULL ) goto err;
		if( (u1    = (sgFloat*)SGMalloc( nump*sizeof(sgFloat) ) ) == NULL ) goto err;
		if( (beze2 = (W_NODE *)SGMalloc( ( nump+add)*sizeof(W_NODE) ) ) == NULL ) goto err;
    if( (u2    = (sgFloat*)SGMalloc( (nump+add)*sizeof(sgFloat) ) ) == NULL ) goto err;

		for( j=j1, k=0; j<=j2; j++, k++ ) {	beze1[k]=P_new1[j]; u1[k]=u_new1[j]; }

//increase degree of the Beze segment
		Increase_Beze_Degree( beze1, u1, beze2, u2, degree, add );

//		if( j1==0 ) (*P_new2)[(*nump_new2)++]=beze1[0];
//		for( j=1; j<nump+add-1; j++ ) (*P_new2)[(*nump_new2)++]=beze2[j];
//		if( j2==nump_new1-1) (*P_new2)[(*nump_new2)++]=beze1[nump-1];

		for( j=0; j<=nump+add-1; j++ ){
    	(*P_new2)[(*nump_new2)  ]=beze2[j];
      (*u_new2)[(*nump_new2)++]=u2[j];
    }
//		if( j2==nump_new1-1) (*P_new2)[(*nump_new2)++]=beze1[nump-1];

		SGFree( beze1 ); /*beze1=NULL;*/
    SGFree( u1 );    u1   =NULL;
		SGFree( beze2 ); beze2=NULL;
    SGFree( u2 );    u2   =NULL;
		j1=j2+1;
	}

	return TRUE;
err:
	if(beze1) SGFree(beze1);
  if(u1)    SGFree(u1);
	if(beze2) SGFree(beze2);
	if(u2)    SGFree(u2);
	return FALSE;
}

/*******************************************************************************
*  increase degree of the Beze segment
*  beze1  - initial segment
*  beze2  - resalting segment
*  degree - initial degree
*  add    - changing degreee
*/
static void Increase_Beze_Degree(lpW_NODE beze1, sgFloat *u1,
                                 lpW_NODE beze2, sgFloat *u2, short degree, short add ){
short 		i, j, k, i1, i2;
sgFloat	L, L_i;
	for( i=0; i<=degree+add; i++ ){//increase degree for span
		i1=max(0, i-add);
		i2=min(degree,i);
		L_i=1./c_beze(degree+add, i);
		for( j=i1; j<=i2; j++ ){
			L=c_beze(degree, j)*c_beze(add, i-j)*L_i;
			for( k=0; k<4; k++ )	beze2[i].vertex[k] += L*beze1[j].vertex[k];
      u2[i] += L*u1[j];
		}
	}
}

static sgFloat c_beze(short k1, short k2){
	return (factor(k1)/(factor(k2)*factor(k1-k2)));
}

static short factor(short k){
	short i, l=1;
	if( k==0 ) return l;
	for( i=1; i<=k; i++ ) l*=i;
	return l;
}

//-------------------------------------------------------------------------->>>>

