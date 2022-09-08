#include "../sg.h"
static BOOL Intersect_Spline_By_Spline( lpOBJ obj, hOBJ hspl, short type,
																				lpVDIM data, lpVDIM data1, short sign);

static OSCAN_COD inter_geo_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);

static  OSCAN_COD  inter_line      (hOBJ hobj,lpSCAN_CONTROL lpsc);
static  OSCAN_COD  inter_circle    (hOBJ hobj,lpSCAN_CONTROL lpsc);
static  OSCAN_COD  inter_arc       (hOBJ hobj,lpSCAN_CONTROL lpsc);
static  OSCAN_COD  inter_spline    (hOBJ hobj,lpSCAN_CONTROL lpsc);

static OSCAN_COD  (**inter_typeg)(hOBJ obj,lpSCAN_CONTROL lpsc);

typedef struct {
	hOBJ		hspl;
	short 		type;
	lpVDIM 	data;
	lpVDIM 	data1;
} INTER_DAT;  //   
typedef INTER_DAT * lpINTER_DAT;

BOOL intersection_with_spline( hOBJ hspl, hOBJ hcont, short type,
															lpVDIM data, lpVDIM data1){
	SCAN_CONTROL  sc;
	INTER_DAT			inter_dat;

	inter_typeg = (OSCAN_COD(**)(hOBJ obj,lpSCAN_CONTROL lpsc))GetMethodArray(OMT_SPLINTER);

	inter_typeg[OLINE]  = inter_line;		// OLINE
	inter_typeg[OCIRCLE]= inter_circle;	// OCIRCLE
	inter_typeg[OARC]   = inter_arc;		// OARC
	inter_typeg[OSPLINE]= inter_spline;	// OSPLINE

	inter_dat.hspl =hspl;
	inter_dat.type =type;
	inter_dat.data =data;
	inter_dat.data1=data1;

	init_scan(&sc);
	sc.user_geo_scan       = inter_geo_scan;
	reinterpret_cast<lpINTER_DAT &>(sc.data)   = &inter_dat;
	if (o_scan(hcont,&sc) == OSFALSE) return FALSE;

	return TRUE;
}

static  OSCAN_COD inter_geo_scan(hOBJ hobj,lpSCAN_CONTROL lpsc){
	return( inter_typeg[lpsc->type](hobj,lpsc));
}

/************************************************
*        --- OSCAN_COD inter_line ---
* 			     
************************************************/
/**********************************************************
* detect intersection spline by line
* spl   - spline
* hline - line
* type  - type of the returning data
*         =0 - returns VIM for parameters of intersection points
*         =1 - returns VIM for intersection points
* data  - returning VDIM ( type is detected by user )
*/
#pragma argsused
static  OSCAN_COD inter_line(hOBJ hobj, lpSCAN_CONTROL lpsc){
BOOL           rt=OSFALSE;
short            i, j, j1, n_p, k, l, span_base=60, span_base1=10, span;
sgFloat         *t_intersec;
sgFloat				 t, t1, t_beg, dist, t_sply, t_line, bound[2], tmp_bound;
//------------------------>>
lpOBJ					 obj, obj1;
lpGEO_SPLINE   gspline;
SPLY_DAT       sply_dat;
//------------------------>>
lpGEO_LINE		 gline;
GEO_LINE       gline1;
D_POINT        p, v1, v2, vv2;
lpINTER_DAT    data = (lpINTER_DAT)(lpsc->data);

	obj1 		= (lpOBJ)data->hspl;
	gspline = (lpGEO_SPLINE)(obj1->geo_data);

	obj		= (lpOBJ)hobj;
	gline = (lpGEO_LINE)obj->geo_data;

	if((t_intersec=(sgFloat*)SGMalloc(100*sizeof(sgFloat))) == NULL ) return OSFALSE;
	l=0;

//found beginning point for intersection
	if( !begin_use_sply( gspline, &sply_dat) ) goto err1;

	for(i=0; i<sply_dat.numk-1; i++ ){
		t = sply_dat.u[i+1] - sply_dat.u[i];
		span=span_base + (short)(t*100);
		t = t/span;
		memcpy(&v1, &sply_dat.knots[i], sizeof(DA_POINT));
		bound[0]=sply_dat.u[i];
//-------->>>>>>>>>>>>>>>>>>>>
		for( j=1; j<=span; j++ ){
			t_beg = sply_dat.u[i]+t*j;
			bound[1] = tmp_bound = t_beg;
			if( j==span ) memcpy(&v2, &sply_dat.knots[i+1], sizeof(DA_POINT));
			else       		get_point_on_sply(&sply_dat, t_beg, &v2, 0);

//create line on spline points
			gline1.v1=v1;
			gline1.v2=v2;

//detect intersection of the two lines
			if( !intersect_3d_ll(gline, 1, &gline1, 1, &p, &n_p) ) goto err2;

//---------------------------------------------------------------------------
			if ( n_p != 0 ){  // there are points of intersection
				t_beg=bound[0];
				t1 = t/span_base1;
				for( j1=1; j1<=span_base1; j1++ ){
					t_beg += t1;
					bound[1]=t_beg;
					if( j==span_base1 ) memcpy(&vv2, &v2, sizeof(DA_POINT));
					else       		      get_point_on_sply(&sply_dat, t_beg, &vv2, 0);

//create line on spline points
					gline1.v1=v1;
					gline1.v2=vv2;

//detect intersection of the two lines
					if( !intersect_3d_ll(gline, 1, &gline1, 1, &p, &n_p) ) goto err2;

					if ( n_p != 0 ){  // there are points of intersection
						t_sply=bound[0];
						if(!Spline_By_Point( &sply_dat, (void  *)&p, &t_sply, &dist )) goto err2;
						t_line = dpoint_distance(&p, &gline->v1)/
										 dpoint_distance(&gline->v2, &gline->v1);

//call function for calculate real point of intersection
						if( !Spline_By_Line( &sply_dat, gline,
																 &t_sply, &t_line, bound, &dist )	) goto err2;
//					if( dist >= eps_d || t_beg < 0. ) continue; // tested point is out of the spline
						if( t_sply < 0. ) continue; //tested point out of span

						if( l!=0 ){
							for( k=0; k<l; k++ )
								if( fabs( t_intersec[k] - t_sply ) < eps_n )// this point already exists
									 break;
							if( k>=l ) t_intersec[l++] = t_sply;
						}else t_intersec[l++] = t_sply;
					}
					v1=vv2;
					bound[0]=bound[1];
				}
			}
//---------------------------------------------------------------------------
			v1=v2;
			bound[0]=tmp_bound;
		}
//-------->>>>>>>>>>>>>>>>>>>>
	}
	if( data->type==0 ){ //returns only parameter of intersection points
		for( i=0; i<l; i++)	if( !add_elem( data->data, &t_intersec[i] ) )	goto err2;
	}else{
		for( i=0; i<l; i++){
			get_point_on_sply(&sply_dat, t_intersec[i], &p, 0);
			if( !add_elem( data->data, &p ) )	goto err2;
		}
	}
	rt=OSTRUE;

err2:
	end_use_sply( gspline, &sply_dat );
err1:
	SGFree(t_intersec);
	return (OSCAN_COD)rt;
}

/************************************************
*        --- OSCAN_COD inter_spline ---
* 			     
************************************************/
#pragma argsused
static  OSCAN_COD inter_spline(hOBJ hobj, lpSCAN_CONTROL lpsc){
lpINTER_DAT    data = (lpINTER_DAT)(lpsc->data);
lpOBJ					 obj;

	obj = (lpOBJ)hobj;
	return (OSCAN_COD)(Intersect_Spline_By_Spline( obj, data->hspl, data->type,
																	 data->data, data->data1, 0  ));
}

/**********************************************************
* detect intersection spline by spline
* hspl1 - first spline
* hspl2 - second spline
* type  - type of the returning data
*         =0 - returns VIM for parameters of intersection points
*         =1 - returns VIM for intersection points
* data  - VDIM for points of intersection
* data1 - VDIM for parameters of intersection points(in term 2-d spl)
*         in case of type=0
* sign  - =0-detect sply_by_sply intersection
*         =1-detect autointersection
*/
static BOOL Intersect_Spline_By_Spline( lpOBJ obj, hOBJ hspl, short type,
																				lpVDIM data, lpVDIM data1, short sign){
BOOL           rt=OSFALSE;
short          span1, span2, span_base=5;
short          j, jj, i, ii, n_p, l, k;
sgFloat         *intersect, *intersect1=NULL;
sgFloat         t1, t2, t_beg1, t_beg2, dist;
sgFloat				 t_sply1, t_sply2, bound1[2], bound2[2], bound1_tmp, bound2_tmp;
//------------------------>>
lpOBJ					 obj1;
lpGEO_SPLINE   gspline1, gspline2;
SPLY_DAT       sply_dat1, sply_dat2;
//------------------------>>
GEO_LINE       gline1, gline2;
D_POINT        p, p1, p2, v1, v2; //, pp2, vv2;

// spline
	obj1 = (lpOBJ)hspl;
	gspline1 = (lpGEO_SPLINE)(obj1->geo_data);
	gspline2 = (lpGEO_SPLINE)(obj->geo_data);

	if((intersect =(sgFloat*)SGMalloc(100*sizeof(sgFloat))) == NULL ) return OSFALSE;
	if( type != 0 ) if((intersect1=(sgFloat*)SGMalloc(100*sizeof(sgFloat))) == NULL ) goto err0;
	l=0;

//begin
	if( !begin_use_sply( gspline1, &sply_dat1) ) goto err1;
//	if( gspline1->type & SPLY_APPR) change_APPR_INT(&sply_dat1);

	if( !begin_use_sply( gspline2, &sply_dat2) ) goto err2;
//	if( gspline2->type & SPLY_APPR) change_APPR_INT(&sply_dat2);

//---------------------------------------------------------
	for(i=0; i<sply_dat1.numk-1; i++ ){   //first spline
		t1 = sply_dat1.u[i+1] - sply_dat1.u[i];   //take i-th interval
		span1 = span_base + (short)(t1*100);
		t1 = t1/span1;
		memcpy(&v1, &sply_dat1.knots[i], sizeof(DA_POINT));
		bound1[0]=sply_dat1.u[i];      //--------------->>>>>
//---------------------------------------------------------
		for( j=1; j<=span1; j++ ){  //devide i-th interval into the smaller ones
			t_beg1 = sply_dat1.u[i]+t1*j;
			bound1[1]=bound1_tmp=t_beg1;//--------------->>>>>
			if( j==span1 ) memcpy(&v2, &sply_dat1.knots[i+1], sizeof(DA_POINT));
			else       	   get_point_on_sply(&sply_dat1, t_beg1, &v2, 0);

//create line on spline points
			gline1.v1=v1;
			gline1.v2=v2;
//---------------------------------------------------------
			for(ii=0; ii<sply_dat2.numk-1; ii++ ){   //second spline
				if( sign&&ii==i ) continue; // imposible to intersect the same segment(for autointer.)
				t2 = sply_dat2.u[ii+1] - sply_dat2.u[ii];   //take ii-th interval
				span2 = span_base + (short)(t2*100);
				t2 = t2/span2;
				memcpy(&p1, &sply_dat2.knots[ii], sizeof(DA_POINT));
				bound2[0]=sply_dat2.u[ii];//--------------->>>>>
//---------------------------------------------------------
				for( jj=1; jj<=span2; jj++ ){  //devide ii-th interval into the smaller ones
					t_beg2 = sply_dat2.u[ii]+t2*jj;
					bound2[1]=bound2_tmp=t_beg2;//--------------->>>>>
					if( jj==span2 ) memcpy(&p2, &sply_dat2.knots[ii+1], sizeof(DA_POINT));
					else       	  	get_point_on_sply(&sply_dat2, t_beg2, &p2, 0);

//create line on spline points
					gline2.v1=p1;
					gline2.v2=p2;
//---------------------------------------------------------
//detect intersection of the two lines
					if( !intersect_3d_ll(&gline1, 1, &gline2, 1, &p, &n_p) ) goto err3;

//---------------------------------------------------------
					if ( n_p != 0 ){  // there are points of intersection
//--------------------------------->>>>>>
//call function for calculate real point of intersection
									t_sply1=bound1[0];
//									if(!Spline_By_Point( &sply_dat1, (void  *)&p, &t_sply1, &dist )) goto err3;
									t_sply2=bound2[0];
//									if(!Spline_By_Point( &sply_dat2, (void  *)&p, &t_sply2, &dist )) goto err3;

									if( !Spline_By_Spline( &sply_dat1, (void  *)&sply_dat2, &t_sply1,
																					&t_sply2, bound1, bound2, &dist )	) goto err3;
//									if( dist >= eps_d || t_sply1 < 0.||t_sply2 < 0. ) continue; // tested point is out of the spline

									if( l!=0 ){
										for( k=0; k<l; k++ )
											if( fabs( intersect[k] - t_sply1 ) < eps_n )// this point already exists
												 break;
										if( k>=l ){
											intersect[l++] = t_sply1;
											if( type==0 ) intersect1[l-1]=t_sply2;
										}
									}else{
										intersect[l++] = t_sply1;
										if( type==0 ) intersect1[l-1]=t_sply2;
									}
								}
//---------------------------------------------------------
					p1=p2;
					bound2[0]=bound2_tmp;
				}
			}
			v1=v2;
			bound1[0]=bound1_tmp;
		}
	}
	if(type==0){
		for( i=0; i<l; i++){
			if( !add_elem( data, &intersect[i] ) )	goto err3;
			if( sign !=1 ) 	if( !add_elem( data1, &intersect1[i] ) )	goto err3;
		}
	}else{
		for( i=0; i<l; i++){
			get_point_on_sply(&sply_dat1, intersect[i], &p, 0);
			if( !add_elem( data, &p ) )	goto err3;
		}
	}
	rt=OSTRUE;

err3:
	end_use_sply(gspline1, &sply_dat2);
err2:
	end_use_sply(gspline2, &sply_dat1);
err1:
	if( intersect1 != NULL ) SGFree(intersect1);
err0:
	SGFree(intersect);
	return rt;
}

/************************************************
*        --- OSCAN_COD inter_arc ---
* 			     
************************************************/
/**********************************************************
* detect intersection spline by arc
* hspl  - spline
* harc  - circular arc
* type  - type of the returning data
*         =0 - returns VIM for parameters of intersection points
*         =1 - returns VIM for intersection points
* data  - returning VDIM ( type is detected by user )
*/
static  OSCAN_COD inter_arc(hOBJ hobj, lpSCAN_CONTROL lpsc){
BOOL           rt;
//------------------------>>
hOBJ           hspl_arc=NULL;
lpOBJ					 obj;
//------------------------>>
lpINTER_DAT    data = (lpINTER_DAT)(lpsc->data);

//create new hobj
	if(!transform_to_NURBS( hobj, &hspl_arc )) return OSFALSE;

	obj = (lpOBJ)hspl_arc;
//detect intersection of two splines
	rt=Intersect_Spline_By_Spline( obj, data->hspl, data->type,
																	 data->data, data->data1, 0  );
	o_free(hspl_arc, NULL);
	return (OSCAN_COD)rt;
}

/**********************************************************
* detect intersection spline by circle
* hspl  - spline
* harc  - circle
* type  - type of the returning data
*         =0 - returns VIM for parameters of intersection points
*         =1 - returns VIM for intersection points
* data  - VDIM for points of intersection
*/
static  OSCAN_COD inter_circle(hOBJ hobj, lpSCAN_CONTROL lpsc){
BOOL           rt;
//------------------------>>
hOBJ           hspl_circ=NULL;
lpOBJ					 obj;
//------------------------>>
lpINTER_DAT    data = (lpINTER_DAT)(lpsc->data);

//create new hobj
	if(!transform_to_NURBS( hobj, &hspl_circ )) return OSFALSE;

	obj = (lpOBJ)hspl_circ;
//detect intersection of two splines
	rt=Intersect_Spline_By_Spline( obj, data->hspl, data->type,
																	 data->data, data->data1, 0  );
	o_free(hspl_circ, NULL);
	return (OSCAN_COD)rt;
}

/**********************************************************
* devide spline by point into two splines
* spl - spline
* type -=0 point.x is point parameter
*       =1 - point - point of the deviding
* spl_l, spl_r - obtaining splines after the deviding
*/
BOOL Devide_Spline_By_Point(hOBJ hspl, short type, lpD_POINT point,
														hOBJ hspl_l, hOBJ hspl_r){
BOOL         rt=OSFALSE;
sgFloat       t, dist;
lpOBJ        obj, obj1;
lpGEO_SPLINE gspline;
SPLY_DAT     sply_dat;
lpGEO_SPLINE spline;

	obj = (lpOBJ)hspl;
	gspline = (lpGEO_SPLINE)(obj->geo_data);

//begin use spline
	if( !begin_use_sply( gspline, &sply_dat) ) return OSFALSE;

//detect parameter for the dividing point
	if( type==1 )	if( !Spline_By_Point( &sply_dat, point, &t, &dist )) goto err1;
	else          t=point->x;

//split spline into two parts
//first part
	if((hspl_l = o_alloc(OSPLINE)) == NULL) goto err1;

	obj1 = (lpOBJ)hspl_l;
	obj1->color = obj->color;    					// 
	obj1->ltype = obj->ltype;    					//  
	obj1->lthickness = obj->lthickness;   // 
	spline = (lpGEO_SPLINE)(obj1->geo_data);
	if( !Get_Part_Spline_Geo(&sply_dat, 0, t, spline )){
		 o_free(hspl_l,NULL);
		 goto err1;
	}
	copy_obj_attrib(hspl, hspl_l);

//second part
	if((hspl_r = o_alloc(OSPLINE)) == NULL)	goto err1;

	obj1 = (lpOBJ)hspl_r;
	obj1->color = obj->color;    					// 
	obj1->ltype = obj->ltype;    					//  
	obj1->lthickness = obj->lthickness;   // 
	spline = (lpGEO_SPLINE)(obj1->geo_data);
	if( !Get_Part_Spline_Geo(&sply_dat, t, 0, spline )){
		 o_free(hspl_r,NULL);
		 goto err1;
	}
	copy_obj_attrib(hspl, hspl_r);
	rt=OSTRUE;

err1:
	end_use_sply(gspline, &sply_dat);
	return rt;
}

/**********************************************************
* detect autointersection spline
* hspl  - spline
* type  - type of the returning data
*         =0 - returns VIM for parameters of intersection points
*         =1 - returns VIM for intersection points
* data  - VDIM for points of intersection
*/
BOOL AutoIntersect_Spline(hOBJ hspl, short type, lpVDIM data){
BOOL  rt;
hOBJ  hobj_n;

	if ( !o_copy_obj(hspl, &hobj_n,"") ) return FALSE;
	rt=Intersect_Spline_By_Spline((OBJ*)hspl, hobj_n, type, data, NULL, 1);
	o_free(hobj_n, NULL);
	return rt;
}
//----------------------------------------------------->>>>

