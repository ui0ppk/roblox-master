#include "../sg.h"
static void	create_work_vector( sgFloat *U, short numu, sgFloat *U_max,
																short *K_max, short*m);
static void	merge_work_vector(sgFloat *U_max, short *K_max, short *m_max,
															sgFloat *U_tmp, short *K_tmp, short m_tmp);

/**********************************************************
* Merge two or more control vectors to create common
* control vector vector
*/
BOOL Merge_Control_Vectors( lpVDIM ribs ){
BOOL		 rt=FALSE;
short    nump_new1, nump_new2;
short 	 i=0, ii, r, k, s, m_max, m_tmp;
short		 *K_max/*=NULL*/, *K_tmp=NULL;
sgFloat	 *U_max/*=NULL*/, *U_tmp=NULL;
SPLY_DAT sply;
sgFloat 	 *U_new1=NULL, *u_new1/*=NULL*/, *U_new2=NULL, *u_new2=NULL;
W_NODE   *P_new1=NULL, *P_new2=NULL;

    long RA_max = 0;

	while( read_elem( ribs, i++, &sply ))
	{
		RA_max+=sply.allocSizeFor_U;
	}

	// RA - if((U_max=(sgFloat*)SGMalloc(2*MAX_POINT_ON_SPLINE*sizeof(sgFloat))) == NULL) return FALSE;
	// RA - if((K_max=(short *)SGMalloc(2*MAX_POINT_ON_SPLINE*sizeof(short))) 	== NULL) goto err;
	// RA - if((U_tmp=(sgFloat*)SGMalloc(MAX_POINT_ON_SPLINE  *sizeof(sgFloat)))	== NULL) goto err;
	// RA - if((K_tmp=(short *)SGMalloc(MAX_POINT_ON_SPLINE  *sizeof(short))) 	== NULL) goto err;

	if((U_max=(sgFloat*)SGMalloc(2*RA_max*sizeof(sgFloat))) == NULL) return FALSE;
	if((K_max=(short *)SGMalloc(2*RA_max*sizeof(short))) 	== NULL) goto err;
	if((U_tmp=(sgFloat*)SGMalloc(RA_max  *sizeof(sgFloat)))	== NULL) goto err;
	if((K_tmp=(short *)SGMalloc(RA_max  *sizeof(short))) 	== NULL) goto err;

	i=0;
//create Max control vector
	while( read_elem( ribs, i++, &sply ))
	{
		if(i==1)
		{ //if first - put into vector
			create_work_vector(sply.U, sply.numU, U_max, K_max, &m_max);
		}
		else
		{
			create_work_vector(sply.U, sply.numU, U_tmp, K_tmp, &m_tmp);
			merge_work_vector(U_max, K_max, &m_max, U_tmp, K_tmp, m_tmp);
		}
	}

//add knots into all ribs
	ii=0;
	while( read_elem( ribs, ii++, &sply )){ //read current rib
		if( (P_new1=(W_NODE*)SGMalloc(sply.nump*sizeof(W_NODE))) == NULL) goto err;
		if( (u_new1=(sgFloat*)SGMalloc(sply.nump*sizeof(sgFloat))) == NULL) goto err1;
		for( i=0; i<sply.nump; i++ ){
			memcpy( &P_new1[i], &sply.P[i], sizeof(W_NODE) );
    	u_new1[i]=sply.u[i];
  	}

 		if( (U_new1=(sgFloat*)SGMalloc(sply.numU*sizeof(sgFloat))) == NULL ) goto err1;
		for( i=0; i<sply.numU; i++ ) U_new1[i]=sply.U[i];
		nump_new1=sply.nump;

//insert new knots
		for( i=0; i<m_max; i++ ){
			if( !Detect_Multy_Insertion( U_new1, sply.degree, U_max[i],
																	 nump_new1, &s, &k)) goto err1;
      r=K_max[i]-s;
			if( r>0 ){
				nump_new2=0;
				if( !Insert_Knot(  U_new1, P_new1,  u_new1,  nump_new1, sply.degree, U_max[i],
                          &U_new2, &P_new2, &u_new2, &nump_new2, r, k, s ) ) goto err1;
				SGFree(U_new1); U_new1=U_new2; U_new2=NULL;
				SGFree(P_new1); P_new1=P_new2; P_new2=NULL;
				SGFree(u_new1); u_new1=u_new2; u_new2=NULL;
				nump_new1=nump_new2;
			}
		}

// modify spline structure
    if( !Modify_Spline_Str( &sply, U_new1, nump_new1+sply.degree+1, P_new1, u_new1,
    												nump_new1, sply.degree ) ) goto err1;
		if( P_new1 ) SGFree( P_new1 ); P_new1=NULL;
    if( U_new1 ) SGFree( U_new1 ); U_new1=NULL;
    if( u_new1 ) SGFree( u_new1 ); u_new1=NULL;
		if( !write_elem( ribs, ii-1, &sply )) goto err1;
	}
	rt=TRUE;
err1:
	if( P_new1 ) SGFree( P_new1 ); 	if( P_new2 ) SGFree( P_new2 );
	if( U_new1 ) SGFree( U_new1 );  if( U_new2 ) SGFree( U_new2 );
	if( u_new1 ) SGFree( u_new1 );  if( u_new2 ) SGFree( u_new2 );
err:
	if( K_tmp ) SGFree(K_tmp); if( U_tmp ) SGFree(U_tmp);
	if( K_max ) SGFree(K_max); if( U_max ) SGFree(U_max);
	return rt;
}

static void	create_work_vector( sgFloat *U, short numu, sgFloat *U_max,short *K_max, short*m)
{
	short j=0, k, s;
	*m=0;

	while(1)
	{
		for( s=0, k=j; k<numu; k++ ) 
			if( fabs(U[j]-U[k])<eps_n ) s++;
		U_max[(*m)]=U[j];
		K_max[(*m)++]=s;
		if( (j+=s)>=numu-1) break;
	}
}

static void	merge_work_vector(sgFloat *U_max, short *K_max, short *m_max,
															sgFloat *U_tmp, short *K_tmp, short m_tmp){
short i, j, l;

	for( i=0; i<m_tmp; i++ )
		for( j=0; j<*m_max; j++ )
			if( fabs(U_tmp[i]-U_max[j])<eps_n ){ //equal
				if( K_max[j]<K_tmp[i] ) K_max[j]=K_tmp[i];
				break;
			}else{
				if( (U_max[j]<U_tmp[i]) && (U_tmp[i]<U_max[j+1])){
					for(l=*m_max-1; l>=j+1; l-- ){
						U_max[l+1]=U_max[l];
						K_max[l+1]=K_max[l];
					}
					U_max[j+1]=U_tmp[i];
					K_max[j+1]=K_tmp[i];
					(*m_max)++;
					break;
				}
			}
}

//---------------------------------------------------------------->>>>>
/**********************************************************
* Create 4d point from 3d weight point
* for use nonrational B-spline
*/
void Create_4D_From_3D( lpW_NODE node, sgFloat *point, sgFloat w){
short i;

	for( i=0; i<3; i++ ) node->vertex[i] = w*point[i];
	node->vertex[3]=w;
}

/***********************************************************
* Create 3d weight point from 4d point
* for use rational B-spline
*/
void Create_3D_From_4D( lpW_NODE node, sgFloat *point, sgFloat *w){
short  i;
sgFloat t;

	*w=node->vertex[3];
	t = ((*w)==0) ? (1) : (*w);
 	for( i=0; i<3; i++)	point[i]=node->vertex[i]/t;
}

/**********************************************************
* Calculate 2point_distance into 4D-space
*/
sgFloat dpoint_distance_4d(lpW_NODE p1, lpW_NODE p2){
	return(sqrt( (p1->vertex[0]-p2->vertex[0])*(p1->vertex[0]-p2->vertex[0])+
							 (p1->vertex[1]-p2->vertex[1])*(p1->vertex[1]-p2->vertex[1])+
							 (p1->vertex[2]-p2->vertex[2])*(p1->vertex[2]-p2->vertex[2])+
							 (p1->vertex[3]-p2->vertex[3])*(p1->vertex[3]-p2->vertex[3]) ));
}

/**********************************************************
* Calculate 2point_distance into 3D-space
* (convert 4D-point into 3D-point)
*/
sgFloat dpoint_distance_4dl(lpW_NODE p1, lpW_NODE p2){
sgFloat	t;
D_POINT pp1, pp2;
	t = ((p1->vertex[3])==0) ? (1) : (p1->vertex[3]);
	pp1.x=p1->vertex[0]/t;
	pp1.y=p1->vertex[1]/t;
	pp1.z=p1->vertex[2]/t;

	t = ((p2->vertex[3])==0) ? (1) : (p2->vertex[3]);
	pp2.x=p2->vertex[0]/t;
	pp2.y=p2->vertex[1]/t;
	pp2.z=p2->vertex[2]/t;

	t=dpoint_distance( &pp1, &pp2 );
	return ( t );
}

//---------------------------------------------------------------->>>>>
/**********************************************************
* Detect multiplisity of insertion for parameter T and
* interval of the insertion
* U    - control vector
* p    - degree
* nump - number of points
* s    - number of existing parameter
* kk   - number of interval
*/
BOOL Detect_Multy_Insertion(sgFloat *U, short p, sgFloat t, short nump, short *s,
																	 short *k ){
short i, num;

	num = nump + p + 1;

//detect multiplisity
	for( *s=0, i=0; i<num; i++ ) if( fabs(U[i]-t) <= eps_n ) (*s)++;

	if( (*s) == p+1 ){
		for( (*k)=0; (*k)<num; (*k)++ )	if( fabs( U[*k]-t ) <= eps_n ) break;
//number of insertions to create break point
		return TRUE;
	}

//detect span for insertion
	for( (*k)=0; (*k)<num-1; (*k)++ ){
		if( U[*k] <= t &&  t < U[*k+1] ) break;
		if( U[*k] < U[*k+1] && t == U[*k+1] && t == 1. ) break;
	}
	if( *k>num-1 ) return FALSE;
	return TRUE;
}