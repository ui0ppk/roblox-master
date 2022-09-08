#include "../sg.h"

static OSCAN_COD int2hobj_geo_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);
static OSCAN_COD int2hobj_geo_scan_second(hOBJ hobj, lpSCAN_CONTROL lpsc);
typedef struct {
	hOBJ 	hobj1;
	hOBJ 	hobj2;
	VDIM	points;
} INT2HOBJ, * lpINT2HOBJ;

lpD_POINT intersect_two_hobj(hOBJ hobj1, hOBJ hobj2, lpD_POINT vp, lpD_POINT p)

{
	sgFloat    		r, rmin;
	D_POINT   		wp;
	short      		i, imin;
	SCAN_CONTROL 	sc;
	INT2HOBJ 			int2hobj;

	init_vdim(&int2hobj.points, sizeof(D_POINT));
	int2hobj.hobj2 = hobj2;
	init_scan(&sc);
	sc.user_geo_scan  = int2hobj_geo_scan;
	sc.data 	        = &int2hobj;
	if (o_scan(hobj1, &sc) == OSFALSE) goto err;
	if (int2hobj.points.num_elem == 0) goto err;
	//     
	if(!vp){  //    -    
		imin = 0;
	} else {
		imin = -1;
		for (i = 0; i < int2hobj.points.num_elem; i++) {
			read_elem(&int2hobj.points, i, &wp);
			gcs_to_vcs(&wp, &wp);
			r = dpoint_distance_2d(&wp, vp);
			if(imin < 0)      {rmin = r; imin = i; }
			else if(r < rmin) {rmin = r; imin = i; }
		}
	}
	read_elem(&int2hobj.points, imin, p);
	free_vdim(&int2hobj.points);
	return p;
err:
	free_vdim(&int2hobj.points);
	el_geo_err = EL_NO_INTER_POINT;
	return NULL;
}

static OSCAN_COD int2hobj_geo_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	lpINT2HOBJ data = (lpINT2HOBJ)lpsc->data;
	SCAN_CONTROL 	sc;

	data->hobj1 = hobj;
	init_scan(&sc);
	sc.user_geo_scan  = int2hobj_geo_scan_second;
	sc.data 	        = data;
	return o_scan(data->hobj2, &sc);
}

static OSCAN_COD int2hobj_geo_scan_second(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	lpINT2HOBJ 	data = (lpINT2HOBJ)lpsc->data;
	D_POINT    	pp[2];
	short      	kp, i;
	lpOBJ      	obj1, obj2;
	hOBJ			 	hspline, hother;
//	OBJTYPE     TypeOther;

	//        hobj  data->hobj1
	//     VDIM data->points
	obj1 = (lpOBJ)hobj;
	obj2 = (lpOBJ)data->hobj1;
	//    
	if((obj1->type == OLINE || obj1->type == OCIRCLE || obj1->type == OARC) &&
		 (obj2->type == OLINE || obj2->type == OCIRCLE || obj2->type == OARC)){
		//      
		//   pp -   , kp -  
		if(!intersect_two_obj(obj1->type, obj1->geo_data, TRUE,
													obj2->type, obj2->geo_data, TRUE, pp, &kp)) {
			return OSFALSE;
		}
		for (i = 0; i < kp; i++) {
			add_elem(&data->points, &pp[i]);
		}
		return OSTRUE;
	}
	if (obj1->type == OSPLINE) {
		hspline = hobj;
		hother  = data->hobj1;
//		TypeOther = obj2->type;
	} else {
		hspline = data->hobj1;
		hother  = hobj;
//		TypeOther = obj1->type;
	}

//!!!!!! type=1, ,   VDIM(D_POINT)  
//!!!!!! VDIM2==NULL, .. type==1 ( type=0 VDIM2   
//     
	if( !intersection_with_spline( hspline, hother, 1,
															 &data->points, NULL))return OSFALSE;

/*
	switch (TypeOther) {
		case OLINE:	//    
			if (!Intersect_Spline_By_Line(hspline, hother, &data->points, 0)) return OSFALSE;
			break;
		case OARC:	//    
			if (!Intersect_Spline_By_Arc(hspline, hother, &data->points, 0)) return OSFALSE;
			break;
		case OCIRCLE:	//    
			if (!Intersect_Spline_By_Circle(hspline, hother, &data->points, 0)) return OSFALSE;
			break;
		case OSPLINE:	//    
			if (!Intersect_Spline_By_Spline(hspline, hother, &data->points, 0)) return OSFALSE;
			break;
	}
*/
	return OSTRUE;
}

