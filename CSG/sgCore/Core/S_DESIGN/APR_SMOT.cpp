#include "../sg.h"

static OSCAN_COD apr_smooth_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);

static  OSCAN_COD  apr_line      (lpOBJ obj,lpSCAN_CONTROL lpsc);
static  OSCAN_COD  apr_circle    (lpOBJ obj,lpSCAN_CONTROL lpsc);
static  OSCAN_COD  apr_arc       (lpOBJ obj,lpSCAN_CONTROL lpsc);
static  OSCAN_COD  apr_spline    (lpOBJ obj,lpSCAN_CONTROL lpsc);

static OSCAN_COD  (**apr_typeg)(lpOBJ obj,lpSCAN_CONTROL lpsc);

BOOL apr_smooth( hOBJ hobj ){
	SCAN_CONTROL  sc;
	OSTATUS 			status;
	MNODE					mnode;
	D_POINT				first_point;

	apr_typeg = (OSCAN_COD(**)(lpOBJ obj,lpSCAN_CONTROL lpsc))GetMethodArray(OMT_APR_SMOT);

	apr_typeg[OLINE]  = apr_line;		// OLINE
	apr_typeg[OCIRCLE]= apr_circle;	// OCIRCLE
	apr_typeg[OARC]   = apr_arc;		// OARC
	apr_typeg[OSPLINE]= apr_spline;	// OSPLINE

	rib_data.first= TRUE;
	init_scan(&sc);
	sc.user_geo_scan  = apr_smooth_scan;
	sc.data           = &rib_data;

	if (o_scan(hobj,&sc) == OSFALSE) return FALSE;
//        
//   !
	if (!get_status_path(hobj, &status)) return FALSE;
	if (status & ST_CLOSE) {
		if (!read_elem(&rib_data.vdim, 0, &mnode)) return FALSE;
		first_point = mnode.p;
		if (!read_elem(&rib_data.vdim, rib_data.vdim.num_elem - 1, &mnode)) return FALSE;
		mnode.p = first_point;
		if (!write_elem(&rib_data.vdim, rib_data.vdim.num_elem - 1, &mnode)) return FALSE;
	}
	return TRUE;
}

#pragma argsused
static  OSCAN_COD apr_smooth_scan(hOBJ hobj,lpSCAN_CONTROL lpsc){
	lpOBJ obj;
	BOOL cod;

	obj = (lpOBJ)hobj;
	cod = apr_typeg[lpsc->type](obj,lpsc);
	return (OSCAN_COD)cod;
}

#pragma argsused
static  OSCAN_COD apr_line(lpOBJ obj, lpSCAN_CONTROL lpsc){
	lpGEO_LINE 	g  = (lpGEO_LINE)obj->geo_data;
	lpRIB_DATA 	dc = (lpRIB_DATA)lpsc->data;
	MNODE 			mnode;
	lpD_POINT 		p1,p2;
	OSTATUS 		constr1,constr2;
	sgFloat			dx;
	register short i;

	if ( lpsc->status&ST_DIRECT ) {
		p1 = &g->v2; p2 = &g->v1;
		constr1 = (OSTATUS)(obj->status&ST_CONSTR2);
		constr2 = (OSTATUS)(obj->status&ST_CONSTR1);
	}	else {
		p1 = &g->v1; p2 = &g->v2;
		constr1 = (OSTATUS)(obj->status&ST_CONSTR1);
		constr2 = (OSTATUS)(obj->status&ST_CONSTR2);
	}
//------------------------------------------------------>
	if ( dc->first ) {
		dc->first = FALSE;
		mnode.p   = *p1;
		mnode.num = (constr1) ? AP_CONSTR : AP_NULL;
		mnode.num |= AP_SOPR;
		if ( !add_elem(&dc->vdim,&mnode) ) return OSFALSE;
	}

	dx = 1./dc->n_type;  // degree+1
	for( i=1; i<dc->n_type; i++ ){
		mnode.num = AP_CONSTR;
		dpoint_parametr( p1, p2, dx*i, &mnode.p);
		if ( !add_elem( &dc->vdim, &mnode) ) return OSFALSE;
	}
	mnode.num = (constr2) ? AP_CONSTR : AP_NULL;
	mnode.num |= AP_SOPR;
	mnode.p = *p2;
	if ( !add_elem( &dc->vdim, &mnode) ) return OSFALSE;
	return OSTRUE;
}

#pragma argsused
static  OSCAN_COD apr_circle(lpOBJ obj, lpSCAN_CONTROL lpsc){
	register short j, i;
	lpGEO_CIRCLE circle = (lpGEO_CIRCLE)obj->geo_data;
	lpRIB_DATA 	 dc		  = (lpRIB_DATA)lpsc->data;
	MNODE 			 mnode;
	int					 beg,num,step;
	ARC_DATA   	 ad;
	sgFloat			 dx;

	/*nb num = */o_init_ecs_arc(&ad,(lpGEO_ARC)circle, OECS_CIRCLE,c_h_tolerance);
	mnode.num = AP_CONSTR;
	num = dc->n_type+2;
	if( num < 13 ) num=13;
	if( lpsc->status&ST_DIRECT ) {
		beg = num; step = -1;
	}  else {
		beg = 0; step = 1;
	}
	dx = 1./(num-1);
	for (j = beg, i = 0; i <num; j +=step, i++ ) {
		o_get_point_on_arc(&ad, dx*j, &mnode.p);
		if (!(i % (num / 4))) mnode.num |= AP_COND;
		if ( !add_elem( &dc->vdim, &mnode) ) return OSFALSE;
		mnode.num = AP_CONSTR;
	}
	return OSTRUE;
}

#pragma argsused
static  OSCAN_COD apr_arc(lpOBJ obj, lpSCAN_CONTROL lpsc){
	register short j, i;
	lpGEO_ARC arc = (lpGEO_ARC)&obj->geo_data;
	lpRIB_DATA dc = (lpRIB_DATA)lpsc->data;
	MNODE			 mnode;
	short 			 beg,num,step;
	OSTATUS 	 constr1,constr2;
	ARC_DATA   ad;
	sgFloat 		 dx;

	/*nb num = */o_init_ecs_arc(&ad, arc, OECS_ARC,c_h_tolerance);
//	num = (short)( arc->ab/0.2617 + 1 );
	num = (short)( arc->angle/0.5235987755982 + 1 );
	if( num < dc->n_type+2 ) num = dc->n_type+2;
	if ( lpsc->status&ST_DIRECT ) {
		beg = num; step = -1;
		constr1 = (OSTATUS)(obj->status&ST_CONSTR2);
		constr2 = (OSTATUS)(obj->status&ST_CONSTR1);
	} else {
		beg = 0; step = 1;
		constr1 = (OSTATUS)(obj->status&ST_CONSTR1);
		constr2 = (OSTATUS)(obj->status&ST_CONSTR2);
	}
	if ( dc->first )  {
		dc->first = FALSE;
		mnode.num = (constr1) ? AP_CONSTR : AP_NULL;
		mnode.num |= AP_SOPR;
		i = 0;
	} else {
		beg += step;
		i = 1;
		mnode.num = AP_CONSTR;
	}
	dx=1./num;
	for (j = beg; i <= num; j +=step, i++ ) {
		if (i == num) {
			mnode.num = (constr2) ? AP_CONSTR : AP_NULL;
			mnode.num |= AP_SOPR;
		}
		o_get_point_on_arc(&ad, dx*j, &mnode.p);
		if ( !add_elem( &dc->vdim, &mnode) ) return OSFALSE;
		mnode.num = AP_CONSTR;
	}
	return OSTRUE;
}

#pragma argsused
static  OSCAN_COD apr_spline(lpOBJ obj, lpSCAN_CONTROL lpsc){
	lpRIB_DATA 		dc = (lpRIB_DATA)lpsc->data;
	SPLY_DAT    	sply_dat;
	MNODE       	mnode;
	OSCAN_COD   	cod = OSFALSE;
	OSTATUS 			constr1,constr2;
	lpGEO_SPLINE	spline= (lpGEO_SPLINE)obj->geo_data;
	int						i, j, jt, step, t_beg;

	if (spline->type & SPLY_CLOSE) {
		constr1 = constr2 = ST_CONSTR1;
	} else {
		if (lpsc->status & ST_DIRECT) {
			constr1 = (OSTATUS)(obj->status & ST_CONSTR2);
			constr2 = (OSTATUS)(obj->status & ST_CONSTR1);
		} else {
			constr1 = (OSTATUS)(obj->status & ST_CONSTR1);
			constr2 = (OSTATUS)(obj->status & ST_CONSTR2);
		}
	}
	if (!begin_use_sply( spline, &sply_dat)) return OSFALSE;
	if (lpsc->status & ST_DIRECT) {
		t_beg=sply_dat.numk-1; step=-1;
	} else {
		t_beg=0; step=1;
	}
	if (dc->first) {
		dc->first = FALSE;
		mnode.num = (constr1) ? AP_CONSTR : AP_NULL;
		mnode.num |= AP_SOPR;
		i = t_beg; jt=sply_dat.numk;
	} else {
		i = t_beg-1;   jt=sply_dat.numk-1;
		mnode.num = AP_CONSTR;
	}

	for (j=0; j<jt; j++, i += step ) {
		if (j == jt-1) {
			mnode.num = (constr2) ? AP_CONSTR : AP_NULL;
			mnode.num |= AP_SOPR;
		}
		if (!add_elem( &dc->vdim, &sply_dat.knots[i])) goto err;
		mnode.num = AP_CONSTR;
	}
	cod = OSTRUE;
err:
	end_use_sply( (lpGEO_SPLINE)obj->geo_data, &sply_dat );
	return cod;
}


