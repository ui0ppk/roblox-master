#include "../sg.h"

RIB_DATA  	rib_data;
static OSCAN_COD apr_rib_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);

static  OSCAN_COD  apr_line      (lpOBJ obj,lpSCAN_CONTROL lpsc);
static  OSCAN_COD  apr_circle    (lpOBJ obj,lpSCAN_CONTROL lpsc);
static  OSCAN_COD  apr_arc       (lpOBJ obj,lpSCAN_CONTROL lpsc);
static  OSCAN_COD  apr_spline    (lpOBJ obj,lpSCAN_CONTROL lpsc);

static OSCAN_COD  (**apr_typeg)(lpOBJ obj,lpSCAN_CONTROL lpsc);

BOOL apr_rib( hOBJ hobj, short work_type ){
	SCAN_CONTROL  sc;
	OSTATUS 			status;
	MNODE					mnode;
	D_POINT				first_point;

	apr_typeg = (OSCAN_COD(**)(lpOBJ obj,lpSCAN_CONTROL lpsc))GetMethodArray(OMT_APR_RIB);

	apr_typeg[OLINE]  = apr_line;		// OLINE
	apr_typeg[OCIRCLE]= apr_circle;	// OCIRCLE
	apr_typeg[OARC]   = apr_arc;			// OARC
	apr_typeg[OSPLINE]= apr_spline;	// OSPLINE

	rib_data.first= TRUE;
	rib_data.n_type = work_type;
	init_scan(&sc);
	sc.user_geo_scan  = apr_rib_scan;
	sc.data           = &rib_data;

	if (o_scan(hobj,&sc) == OSFALSE) return FALSE;
	if( work_type == 2 ){
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
	}
	return TRUE;
}

#pragma argsused
static  OSCAN_COD apr_rib_scan(hOBJ hobj,lpSCAN_CONTROL lpsc){
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

	switch (dc->n_type){
		case 0: // - 
			dc->n_primitiv++;
			break;
		case 1: //  -   
			dc->n_point[dc->n_num++] = 2;
			break;
		case 2: //  - 
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

			dx = 1./(dc->n_point[dc->n_num]-1);
			for( i=1; i<dc->n_point[dc->n_num]-1; i++ ){
				mnode.num = AP_CONSTR;
				dpoint_parametr( p1, p2, dx*i, &mnode.p);
				if ( !add_elem( &dc->vdim, &mnode) ) return OSFALSE;
			}
			mnode.num = (constr2) ? AP_CONSTR : AP_NULL;
			mnode.num |= AP_SOPR;
			mnode.p = *p2;
			if ( !add_elem( &dc->vdim, &mnode) ) return OSFALSE;
			dc->n_num++;
			break;
	}
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

	switch (dc->n_type){
		case 0: // - 
			dc->n_primitiv++;
			break;
		case 1: //  -   
			num = o_init_ecs_arc(&ad,(lpGEO_ARC)circle, OECS_CIRCLE,c_h_tolerance);
			i = num % 4;
			if (i) num += (4 - i);
			dc->n_point[dc->n_num++] = num+1;
			break;
		case 2:  // 
			/*nb num = */o_init_ecs_arc(&ad,(lpGEO_ARC)circle, OECS_CIRCLE,c_h_tolerance);
			mnode.num = AP_CONSTR;
			num = dc->n_point[dc->n_num]-1;
			if( lpsc->status&ST_DIRECT ) {
				beg = num; step = -1;
			}  else {
				beg = 0; step = 1;
			}
			dx = 1./num;
			for (j = beg, i = 0; i <= num; j +=step, i++ ) {
				o_get_point_on_arc(&ad, dx*j, &mnode.p);
				if (!(i % (num / 4))) mnode.num |= AP_COND;
				if ( !add_elem( &dc->vdim, &mnode) ) return OSFALSE;
				mnode.num = AP_CONSTR;
			}
			dc->n_num++;
			break;
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

	switch (dc->n_type){
		case 0: // - 
			dc->n_primitiv++;
			break;
		case 1: //  -   
			num = o_init_ecs_arc(&ad, arc, OECS_ARC, c_h_tolerance);
			dc->n_point[dc->n_num++] = num+1;
			break;
		case 2:  // 
			/*nb num = */o_init_ecs_arc(&ad, arc, OECS_ARC,c_h_tolerance);
			num = dc->n_point[dc->n_num]-1;
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
			dc->n_num++;
			break;
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
	lpGEO_SPLINE	spline;
	sgFloat				t, dt, t_beg;
	int						i, step;
//	BOOL					knot;

	switch (dc->n_type){
		case 0: // - 
			dc->n_primitiv++;
			return OSTRUE;
		case 1: //  -   
//nb			spline = (lpGEO_SPLINE)obj->geo_data;

			if (!begin_use_sply((lpGEO_SPLINE)obj->geo_data, &sply_dat)) return OSFALSE;
			get_first_sply_point(&sply_dat, c_h_tolerance,
											 ((lpsc->status & ST_DIRECT) ? TRUE : FALSE),
											 &mnode.p);
			if (dc->first) {
				dc->first = FALSE;
				dc->n_point[dc->n_num]++;
			}
			mnode.num = AP_CONSTR;
			while(get_next_sply_point(&sply_dat, &mnode.p))
//			while(get_next_sply_point_tolerance(&sply_dat, &mnode.p, &knot))
				dc->n_point[dc->n_num]++;
			dc->n_num++;
			cod = OSTRUE;
			break;
		case 2:  // 
			spline = (lpGEO_SPLINE)obj->geo_data;
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
			if (lpsc->status & ST_DIRECT) {
					t_beg=1.; step=-1;
			} else {
					t_beg=0.; step=1;
      }
			if (!begin_use_sply((lpGEO_SPLINE)obj->geo_data, &sply_dat)) return OSFALSE;
			dt = 1. / (dc->n_point[dc->n_num] - 1);
			if (dc->first) {
				dc->first = FALSE;
				mnode.num = (constr1) ? AP_CONSTR : AP_NULL;
				mnode.num |= AP_SOPR;
				t = t_beg;
				i = 0;
			} else {
				t = t_beg + step*dt;
				i = 1;
				mnode.num = AP_CONSTR;
			}
			for (; i < dc->n_point[dc->n_num]; i++ ) {
				if( step>0 && t > 1.) t = 1.;
				if( step<0 && t < 0.) t = 0.;
				get_point_on_sply(&sply_dat, t, &mnode.p, 0);
				if (i == dc->n_point[dc->n_num] - 1) {
					mnode.num = (constr2) ? AP_CONSTR : AP_NULL;
					mnode.num |= AP_SOPR;
				}
				if (!add_elem( &dc->vdim, &mnode)) goto err;
				mnode.num = AP_CONSTR;
				t = t+step*dt;
			}
			cod = OSTRUE;
			dc->n_num++;
			break;
	}
err:
	end_use_sply( (lpGEO_SPLINE)obj->geo_data, &sply_dat );
	return cod;
}

