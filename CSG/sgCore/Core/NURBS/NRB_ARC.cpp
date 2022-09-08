#include "../sg.h"
//spline must be APPR!!!!

//----------------------------------------------------------------->>>>>>>>>>>>>
static void create_NURBS_line(lpSPLY_DAT sply, sgFloat *v1, sgFloat *v2);
static BOOL create_NURBS_arc( lpSPLY_DAT sply_dat, sgFloat *center, sgFloat *Ort_X,
															sgFloat *Ort_Y, sgFloat r, sgFloat a_begin, sgFloat fi);
static void create_NURBS_circle(lpSPLY_DAT sply, sgFloat *center, sgFloat *ort1,
															sgFloat *ort2,	sgFloat r1, sgFloat r2);

static OSCAN_COD nrb_cre_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);

static  OSCAN_COD  nrb_line      (lpOBJ obj,lpSCAN_CONTROL lpsc);
static  OSCAN_COD  nrb_circle    (lpOBJ obj,lpSCAN_CONTROL lpsc);
static  OSCAN_COD  nrb_arc       (lpOBJ obj,lpSCAN_CONTROL lpsc);
static  OSCAN_COD  nrb_spline    (lpOBJ obj,lpSCAN_CONTROL lpsc);

typedef struct {
	SPLY_DAT	sply_dat;
	BOOL 			first;
} NRB_DAT;  //     
typedef NRB_DAT * lpNRB_DAT;

static OSCAN_COD  (**nrb_typeg)(lpOBJ obj,lpSCAN_CONTROL lpsc);
/*******************************************************************************
*     ( OPATH, OLINE, OCIRCLE, OARC, OSPLINE)
*  NURBS
*/
BOOL transform_to_NURBS(hOBJ hobj, hOBJ *hobj_new){
	SCAN_CONTROL  sc;
	BOOL					rt=FALSE;
 	OSTATUS 			status;
//  sgFloat				coeff;
	lpOBJ	 			  obj, obj1;
	lpGEO_SPLINE  gspline;
	NRB_DAT				nrb_dat;

	nrb_typeg = (OSCAN_COD(**)(lpOBJ obj,lpSCAN_CONTROL lpsc))GetMethodArray(OMT_TRANSTONURBS);

	nrb_typeg[OLINE]  = nrb_line;		// OLINE
	nrb_typeg[OCIRCLE]= nrb_circle;	// OCIRCLE
	nrb_typeg[OARC]   = nrb_arc;		// OARC
	nrb_typeg[OSPLINE]= nrb_spline;	// OSPLINE

//----------------->>>>
//create new hobj
	if((*hobj_new = o_alloc(OSPLINE)) == NULL) return FALSE;
	obj=(lpOBJ)(*hobj_new);

	obj1=(lpOBJ)hobj;
  copy_obj_nongeo_par( obj1, obj);
//  set_obj_nongeo_par(obj);
	if (!get_status_path(hobj, &status)) return FALSE;

	gspline = (lpGEO_SPLINE)(obj->geo_data);
//----------------->>>>
//initializate spline structure
	if( !init_sply_dat( SPL_NEW, 2, SPL_APPR, &nrb_dat.sply_dat )) return FALSE;

	nrb_dat.first=TRUE;

	init_scan(&sc);
	sc.user_geo_scan   = nrb_cre_scan;
	reinterpret_cast<lpNRB_DAT &>(sc.data) = &nrb_dat;
	if(o_scan(hobj,&sc) == OSFALSE) goto err;

//reparametrization for all ribs
	if( nrb_dat.sply_dat.U[0]!=0. || nrb_dat.sply_dat.U[nrb_dat.sply_dat.numU-1]!=1.)
			if( !Reparametrization(	nrb_dat.sply_dat.nump, nrb_dat.sply_dat.numU,
															0, 1, nrb_dat.sply_dat.degree, nrb_dat.sply_dat.U,
                              nrb_dat.sply_dat.P, nrb_dat.sply_dat.u )) goto err;

	if( nrb_dat.sply_dat.nump > 2 )	calculate_P(&nrb_dat.sply_dat, 0);

	if( status & ST_CLOSE )	nrb_dat.sply_dat.sdtype = (SPLY_TYPE_IN)(nrb_dat.sply_dat.sdtype | SPL_CLOSE);
//convert spline to geo spline
	if( !create_geo_sply(&nrb_dat.sply_dat, gspline)) goto err;

	rt=TRUE;
err:
	free_sply_dat(&nrb_dat.sply_dat);
	return rt;
}

static  OSCAN_COD nrb_cre_scan(hOBJ hobj,lpSCAN_CONTROL lpsc){
	lpOBJ obj;
	BOOL cod;

	obj = (lpOBJ)hobj;
	cod = nrb_typeg[lpsc->type](obj,lpsc);
	return (OSCAN_COD)cod;
}
//----------------------------------------------------------------->>>>>>>>>>>>>
/*******************************************************************************
* Create new object, it's a line as NURBS
*/
#pragma argsused
static  OSCAN_COD nrb_line(lpOBJ obj, lpSCAN_CONTROL lpsc){
lpNRB_DAT  data = (lpNRB_DAT)(lpsc->data);
lpGEO_LINE gline;
SPLY_DAT	 sply_dat, sply_tmp;

/*typedef struct {
	D_POINT  v1;
	D_POINT  v2;
}GEO_LINE;
*/
	gline = (lpGEO_LINE)obj->geo_data;

	if(data->first){
		create_NURBS_line(&data->sply_dat, (sgFloat*)&gline->v1, (sgFloat*)&gline->v2 );
 		if(	lpsc->status & ST_DIRECT) if( !Reorient_Spline( &data->sply_dat ) ) return OSFALSE;
		data->first=FALSE;
	}else{
//initializate spline structure
		if( !init_sply_dat( SPL_NEW, 2, SPL_APPR, &sply_dat )) return (OSCAN_COD)FALSE;
		create_NURBS_line(&sply_dat, (sgFloat*)&gline->v1, (sgFloat*)&gline->v2 );
 		if(	lpsc->status & ST_DIRECT) if( !Reorient_Spline( &sply_dat ) ) goto err;
		if( !Two_Spline_Union_Dat(&sply_dat, &data->sply_dat, &sply_tmp)) goto err;
		static int  ccc = 0;
		ccc++;
		free_sply_dat(&data->sply_dat);
		data->sply_dat=sply_tmp;
		free_sply_dat(&sply_dat);
	}
	return OSTRUE;
err:
	free_sply_dat(&sply_dat);
	free_sply_dat(&sply_tmp);
	return OSFALSE;
}

/*******************************************************************************
* Create line as NURBS
*/
static void create_NURBS_line(lpSPLY_DAT sply, sgFloat* v1, sgFloat* v2){
short i;

// put knots into spline
	for( i=0; i<3; i++) {
	  sply->U[i]=0.;
  	sply->U[i+3]=1.;
// put weigth into spline
		sply->P[i].vertex[3]=1.;
  }
// put control points into spline
  for( i=0; i<3; i++ ){
  	sply->P[0].vertex[i]=v1[i];
	  sply->P[2].vertex[i]=v2[i];
    sply->P[1].vertex[i]=(sply->P[0].vertex[i]+sply->P[2].vertex[i])/2.;
  }

	sply->numU=6;
	sply->nump=sply->numk=3;
  parameter_free_APPR( sply->nump-1, 2, sply->P, sply->u, NULL );
}

//----------------------------------------------------------------->>>>>>>>>>>>>
/*******************************************************************************
* Create new object, it's an arc as NURBS
*/
#pragma argsused
static  OSCAN_COD nrb_arc(lpOBJ obj, lpSCAN_CONTROL lpsc){
lpNRB_DAT  data = (lpNRB_DAT)(lpsc->data);
lpGEO_ARC  garc;
MATR    	 matr;
D_POINT  	 Ort_X, Ort_Y;
SPLY_DAT	 sply_dat, sply_tmp;
/*
typedef struct {
	sgFloat   r;
	D_POINT  n;      //   
	D_POINT  vc;
	D_POINT  vb;
	D_POINT  ve;
	sgFloat   ab;
	sgFloat   angle;
}GEO_ARC;
*/
	garc  = (lpGEO_ARC)obj->geo_data;

//detect points on orts
// first ort
	o_hcunit(matr);
	if( !o_rotate_xyz( matr, &garc->vc, &garc->n, -garc->ab ))return OSFALSE;
	o_hcncrd( matr, &garc->vb, &Ort_X);

	dpoint_sub( &Ort_X, &garc->vc, &Ort_X );
	if( !dnormal_vector ( &Ort_X )) return OSFALSE;

// second ort
	o_hcunit(matr);
	if( !o_rotate_xyz( matr, &garc->vc, &garc->n, -garc->ab+M_PI/2. ))return OSFALSE;
	o_hcncrd( matr, &garc->vb, &Ort_Y);

	dpoint_sub( &Ort_Y, &garc->vc, &Ort_Y );
	if( !dnormal_vector ( &Ort_Y )) return OSFALSE;

	if(data->first){
		if( !create_NURBS_arc(&data->sply_dat, (sgFloat*)&garc->vc, (sgFloat*)&Ort_X,
                          (sgFloat*)&Ort_Y, garc->r, garc->ab, garc->angle)) return OSFALSE;
		if(	lpsc->status & ST_DIRECT) if( !Reorient_Spline( &data->sply_dat ) ) return OSFALSE;
		data->first=FALSE;
	}else{
//initializate spline structure
		if( !init_sply_dat( SPL_NEW, 2, SPL_APPR, &sply_dat )) return (OSCAN_COD)FALSE;
		if( !create_NURBS_arc(&sply_dat, (sgFloat*)&garc->vc, (sgFloat*)&Ort_X,
    											(sgFloat*)&Ort_Y, garc->r, garc->ab, garc->angle)) goto err0;
		if(	lpsc->status & ST_DIRECT) if( !Reorient_Spline( &sply_dat ) ) goto err0;

		if( !Two_Spline_Union_Dat(&sply_dat, &data->sply_dat, &sply_tmp)) goto err;
		free_sply_dat(&data->sply_dat);
		data->sply_dat=sply_tmp;
		free_sply_dat(&sply_dat);
	}
	return OSTRUE;
err:
	free_sply_dat(&sply_tmp);
err0:
	free_sply_dat(&sply_dat);
	return OSFALSE;
}

static BOOL create_NURBS_arc( lpSPLY_DAT sply_dat, sgFloat* center, sgFloat* Ort_X,
                              sgFloat* Ort_Y, sgFloat r, sgFloat a_begin, sgFloat fi){
short    i, ii, j, index, narc, n_p;
sgFloat	 d_fi, w1, c, s, c1, s1, angle, f;
DA_POINT P0, P1, P2, T0, T2;
GEO_LINE gline1, gline2;

//detect number of part the arc consist of
	f=fabs(fi);
	if( ( f < M_PI/2. ) || ( fabs(f-M_PI/2.) < eps_n ) ) narc=1;
	else {
		if( ( f < M_PI    ) || ( fabs(f-M_PI   ) < eps_n ) ) narc=2;
		else {
			if( ( f < 3*M_PI/2. ) || ( fabs(f-3*M_PI/2.) < eps_n ) ) narc=3;
			else 	if( ( f < 2*M_PI    ) || ( fabs(f-2*M_PI   ) < eps_n ) ) narc=4;
		}
	}
// create control points and put them into spline
	d_fi=fi/narc;

	j=2*narc+1;
	sply_dat->nump=sply_dat->numk=j;
	w1=cos(d_fi/2.);

 	c=cos(a_begin); c1=r*c;
	s=sin(a_begin); s1=r*s;
  for( i=0; i<3; i++) P0[i]=center[i] + c1*Ort_X[i] + s1*Ort_Y[i];
  Create_4D_From_3D( &sply_dat->P[0], (sgFloat *)&P0, 1.);
  for( i=0; i<3; i++)	T0[i] = P0[i] - s*Ort_X[i] + c*Ort_Y[i];

//create line on points
	memcpy( &gline1.v1, P0, sizeof(D_POINT));
 	memcpy( &gline1.v2, T0, sizeof(D_POINT));

	angle=a_begin;
	for(ii=1, index=0; ii<=narc; ii++, index+=2 ){
		angle += d_fi;
    if( angle > 2*M_PI || fabs( angle - 2*M_PI )< eps_n ) angle -=2*M_PI;

	 	c=cos(angle); c1=r*c;
		s=sin(angle); s1=r*s;
    for( i=0; i<3; i++)	P2[i] = center[i] + c1*Ort_X[i] + s1*Ort_Y[i];
	  Create_4D_From_3D( &sply_dat->P[index+2], (sgFloat *)&P2, 1.);
    for( i=0; i<3; i++)	T2[i] = P2[i] - s*Ort_X[i] + c*Ort_Y[i];

//create line on points
		memcpy( &gline2.v1, P2, sizeof(D_POINT));
  	memcpy( &gline2.v2, T2, sizeof(D_POINT));

//detect intersection of the two lines
		if( !intersect_3d_ll(&gline1, 0, &gline2, 0, (lpD_POINT)&P1, &n_p) ) return FALSE;
		if( n_p==0 ) return FALSE;
	  Create_4D_From_3D( &sply_dat->P[index+1], (sgFloat *)&P1, w1);

//		P0=P2;
//		T0=T2;
		memcpy( P0, P2, sizeof(DA_POINT));
  	memcpy( T0, T2, sizeof(DA_POINT));
		memcpy( &gline1.v1, P0, sizeof(D_POINT));
  	memcpy( &gline1.v2, T0, sizeof(D_POINT));
	}

//create knots and put them into spline
//nuber of knots = number_of_control_points+degree+1
	sply_dat->numU=j+2+1;
	for(i=0;i<3; i++){ sply_dat->U[i]=0.; sply_dat->U[i+j]=1.;}
	switch(narc){
		case 1:
			break;
		case 2:
			sply_dat->U[3] = sply_dat->U[4] = 0.5;
			break;
		case 3:
			sply_dat->U[3] = sply_dat->U[4] = 1./3.;
			sply_dat->U[5] = sply_dat->U[6] = 2./3.;
			break;
		case 4:
			sply_dat->U[3] = sply_dat->U[4] = 0.25;
			sply_dat->U[5] = sply_dat->U[6] = 0.5;
			sply_dat->U[7] = sply_dat->U[8] = 0.75;
			break;
	}
  parameter_free_APPR( sply_dat->nump-1, 2, sply_dat->P, sply_dat->u, NULL );
	return TRUE;
}

//----------------------------------------------------------------->>>>>>>>>>>>>
/*******************************************************************************
* Create new object, it's a circle as NURBS
*/
#pragma argsused
static  OSCAN_COD nrb_circle(lpOBJ obj, lpSCAN_CONTROL lpsc){
lpNRB_DAT  data = (lpNRB_DAT)(lpsc->data);
lpGEO_CIRCLE gcirc;
D_POINT  Ort_X, Ort_Y;
sgFloat	 r1, r2;

	gcirc = (lpGEO_CIRCLE)obj->geo_data;

//detect two points on circle
	init_ecs_arc((lpGEO_ARC)gcirc, ECS_CIRCLE);
// first ort
	get_point_on_arc(0., &Ort_X);
	dpoint_sub( &Ort_X, &gcirc->vc, &Ort_X );
	if( !dnormal_vector ( &Ort_X )) return OSFALSE;
//first radii
	 r1=gcirc->r;

// second ort
	get_point_on_arc(1./4., &Ort_Y);
	dpoint_sub( &Ort_Y, &gcirc->vc, &Ort_Y );
	if( !dnormal_vector ( &Ort_Y )) return OSFALSE;
// second radii
	r2=r1;

	if(data->first){
		create_NURBS_circle( &data->sply_dat, (sgFloat*)&gcirc->vc, (sgFloat*)&Ort_X,
                         (sgFloat*)&Ort_Y, r1, r2);
 		if(	lpsc->status & ST_DIRECT) if( !Reorient_Spline( &data->sply_dat ) ) return OSFALSE;
		data->first=FALSE;
	}
	return OSTRUE;
}

/**********************************************************
* create nurbs curve for circle/ellipse
*/
static void create_NURBS_circle( lpSPLY_DAT sply, sgFloat *center, sgFloat *ort1,
																 sgFloat * ort2, sgFloat r1, sgFloat r2){
	 short i;

// put knots into spline
   for( i=0; i<3; i++ ) { sply->U[i]=0.; sply->U[i+7]=1.; }
	 sply->U[3]=0.25;
	 sply->U[4]=sply->U[5]=0.5;
	 sply->U[6]=0.75;
	 sply->numU=10;

// put weigth into spline
	 for( i=0; i<3; i++ ) sply->P[3*i].vertex[3]=1.;
   sply->P[1].vertex[3]=sply->P[2].vertex[3]=sply->P[4].vertex[3]=sply->P[5].vertex[3]=0.5;

// put control points into spline
	 for( i=0; i<3; i++ ) sply->P[0].vertex[i] = center[i]+r1*ort1[i];
   memcpy( &sply->P[6], &sply->P[0], sizeof(W_NODE) );

	 for( i=0; i<3; i++ ){
   	 sply->P[1].vertex[i] = (sply->P[0].vertex[i]+r2*ort2[i])/2.;
     sply->P[2].vertex[i] = (center[i]+r2*ort2[i]-r1*ort1[i])/2.;
	   sply->P[3].vertex[i] = center[i]-r1*ort1[i];
   }
	 for( i=0; i<3; i++ ){
	   sply->P[4].vertex[i]= (sply->P[3].vertex[i]-r2*ort2[i])/2.;
	   sply->P[5].vertex[i]= (center[i]-r2*ort2[i]+r1*ort1[i])/2.;
   }

   sply->nump=sply->numk=7;
   parameter_free_APPR( sply->nump-1, 2, sply->P, sply->u, NULL );
}
//-------------------------------------------->>>>>>>>>>>>>

/**********************************************************
* Create new object, it's a spline as NURBS
*/
#pragma argsused
static  OSCAN_COD nrb_spline(lpOBJ obj, lpSCAN_CONTROL lpsc){
lpNRB_DAT  	 data = (lpNRB_DAT)(lpsc->data);
BOOL			 	 ret=OSFALSE;
SPLY_DAT	 	 sply_dat, sply_tmp;
lpGEO_SPLINE gspline;

	gspline = (lpGEO_SPLINE)obj->geo_data;

	if(data->first){
//		if( !begin_use_sply(gspline, &data->sply_dat)) return OSFALSE;
		if( !unpack_geo_sply( gspline, &data->sply_dat)) return OSFALSE;
		if( data->sply_dat.sdtype & SPL_INT ) if( !change_INT_APPR(&data->sply_dat) ) return OSFALSE;
		if(	lpsc->status & ST_DIRECT) if( !Reorient_Spline(&data->sply_dat) ) return OSFALSE;
//		parameter_free_APPR( data->sply_dat.nump-1, 2, data->sply_dat.P, data->sply_dat.u, NULL );
		data->first=FALSE;
		return OSTRUE;
	}else{
//initializate spline structure
		if( !begin_use_sply(gspline, &sply_dat)) return OSFALSE;
		if( sply_dat.sdtype & SPL_INT )	if( !change_INT_APPR(&sply_dat) ) goto err;
		if(	lpsc->status & ST_DIRECT)    if( !Reorient_Spline(&sply_dat) ) goto err;
		parameter_free_APPR( data->sply_dat.nump-1, 2, data->sply_dat.P, data->sply_dat.u, NULL );

		if( !Two_Spline_Union_Dat(&sply_dat, &data->sply_dat, &sply_tmp)) goto err1;
		free_sply_dat(&data->sply_dat);
		data->sply_dat=sply_tmp;
		free_sply_dat(&sply_dat);
	}
	return OSTRUE;
err1:
	free_sply_dat(&sply_tmp);
err:
	free_sply_dat(&sply_dat);
	return (OSCAN_COD)ret;
}


