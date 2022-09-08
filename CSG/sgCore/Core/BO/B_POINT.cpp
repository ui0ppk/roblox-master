#include "../sg.h"

typedef struct {
	hOBJ	 	hobj;						
	lpVDIM 	p;							
	CODE_OPERATION key;     
	lpLISTH	listh;          
} POINT_DATA;

typedef POINT_DATA 		* lpPOINT_DATA;

static OSCAN_COD b_sub_spline(lpGEO_SPLINE spline, lpSCAN_CONTROL lpsc);
static int sort_founction1(const void *a, const void *b);
static OSCAN_COD b_sub_line(lpGEO_LINE geo_line, lpSCAN_CONTROL lpsc);
static OSCAN_COD b_sub_arc(lpGEO_ARC arc, lpREGION_3D gab_arc, lpSCAN_CONTROL lpsc);
static OSCAN_COD point_scan(hOBJ hobj, lpSCAN_CONTROL lpsc);
static int sort_function( const void *a, const void *b);
static void b_get_point_on_arc(lpGEO_ARC arc, sgFloat alfa, lpD_POINT p);
static  void o_modify_limits_by_point(lpD_POINT p, lpREGION_3D reg);
static BOOL	get_ends_point(hOBJ hobj, lpD_POINT p1, lpD_POINT p2);

BOOL bo_points(hOBJ hobj_l, hOBJ hobj_b, lpVDIM points, CODE_OPERATION code_operation,
							 lpLISTH	listh)
{
	REGION_3D   	gabb, gabl, gab;
	hOBJ					hobj1 = NULL;
	SCAN_CONTROL  sc;
	POINT_DATA		cd;
	OSCAN_COD			cod;
	NP_TYPE_POINT answer;
	D_POINT				p1, p2;
	lpOBJ					obj;

	if ( !(code_operation == SUB || code_operation == INTER ||
				 code_operation == INTER_POINT) ) return FALSE;

	if (!get_object_limits(hobj_b, &gabb.min, &gabb.max)) return FALSE;
	if (!get_object_limits(hobj_l, &gabl.min, &gabl.max)) return FALSE;
	if ( np_gab_inter(&gabl, &gabb) ) { 
		if ( code_operation == INTER_POINT ) return TRUE;
		if ( !get_ends_point(hobj_l, &p1, &p2) ) return FALSE;
		if ( !check_point_object(&p1, hobj_b, &answer) ) return FALSE;
		if ( answer == NP_OUT && code_operation == SUB ||
				(answer == NP_IN || answer == NP_ON) && code_operation == INTER ) {
	
			if ( !o_copy_obj(hobj_l, &hobj1,"") ) return FALSE;
			obj = (lpOBJ)hobj1;
  
			obj->status &= (~(ST_SELECT|ST_BLINK|ST_DIRECT));
			attach_item_tail(listh, hobj1);
		}
		return TRUE;
	}

	np_gab_un(&gabb, &gabl, &gab);
	define_eps(&gab.min, &gab.max); 

	init_scan(&sc);
	cd.p 			= points;
	cd.listh	= listh;
	cd.hobj 	= hobj_b;
	cd.key 		= code_operation;
	sc.data   = &cd;
	sc.user_geo_scan = point_scan;

	cod = o_scan(hobj_l, &sc);

	return cod;
}
#pragma argsused
static OSCAN_COD point_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	short					count, color;
	SCAN_CONTROL 	tmp;
	lpOBJ				 	obj, obj1;
	hOBJ					hobj_new, hnext, hobj1, h_first, h_last;
	GEO_SPLINE    geo_spline;
	GEO_ARC				geo_arc;
	lpGEO_ARC			lpgeo_arc, lpgeo_arc1;
	GEO_LINE			geo_line;
	GEO_OBJ				geo;
	VLD						vld;
	REGION_3D			gab;
	OSCAN_COD			ret;
	LISTH					listh;
	lpPOINT_DATA	data;
	D_POINT				min, max, p;
	D_POINT				p11, p12, p21, p22;
	ARC_DATA    	ad;
	short						num, i;

	if ( ctrl_c_press ) return OSBREAK;

	init_listh(&listh);
	data = (lpPOINT_DATA)(lpsc->data);

	switch (lpsc->type) {
		case OPOINT:
			break;
		case OARC:
			obj = (lpOBJ)lpsc->hobj;
			geo_arc = *(lpGEO_ARC)obj->geo_data;
			if (!get_object_limits(lpsc->hobj, &gab.min, &gab.max)) return OSFALSE;
			ret = b_sub_arc(&geo_arc, &gab, lpsc);
			break;
		case OCIRCLE:        
			obj = (lpOBJ)lpsc->hobj;
			memcpy(&geo_arc, obj->geo_data, sizeof(GEO_CIRCLE));
			get_c_matr((lpGEO_CIRCLE)obj->geo_data, el_g_e, el_e_g);
			if (!get_object_limits(lpsc->hobj, &gab.min, &gab.max)) return OSFALSE;
			p.x = geo_arc.r;
			p.y = p.z = 0.;
			o_hcncrd(el_e_g, &p, &p);
			geo_arc.angle = 2.*M_PI;
			geo_arc.vb = p;
			geo_arc.ve = p;
			calc_arc_ab(&geo_arc);
			ret = b_sub_arc(&geo_arc, &gab, lpsc);
			if ( !data->listh ) break;    

			if ( data->listh->num > 1 ) { 
				h_first = data->listh->hhead;
				h_last = data->listh->htail;
				if ( !get_ends_point(h_first, &p11, &p12) ) {
					ret = OSFALSE;
					break;
				}
				if ( !get_ends_point(h_last, &p21, &p22) ) {
					ret = OSFALSE;
					break;
				}
				if ( dpoint_eq(&p11, &p22, eps_d) ) {
					detach_item(data->listh, h_first);
					obj = (lpOBJ)h_first;
					obj1 = (lpOBJ)h_last;
					lpgeo_arc = (lpGEO_ARC)(obj->geo_data);
					lpgeo_arc1 = (lpGEO_ARC)(obj1->geo_data);
					lpgeo_arc1->ve = lpgeo_arc->ve;
					lpgeo_arc1->angle += lpgeo_arc->angle;
					calc_arc_ab(lpgeo_arc1);
					o_free(h_first, NULL);
				}
			}
			break;
		case OLINE:
			obj = (lpOBJ)lpsc->hobj;
			geo_line = *(lpGEO_LINE)obj->geo_data;
			ret = b_sub_line(&geo_line, lpsc);
			break;
		case OSPLINE:
			obj = (lpOBJ)lpsc->hobj;
			geo_spline = *(lpGEO_SPLINE)obj->geo_data;
			ret=b_sub_spline(&geo_spline, lpsc);
			break;
		case OFRAME:
			init_vld(&vld);
			count = 0;
			tmp = *lpsc;
			obj = (lpOBJ)lpsc->hobj;
			frame_read_begin((lpGEO_FRAME)obj->geo_data);
			while ( frame_read_next(&(tmp.type), (short *)&(tmp.color), &(tmp.ltype),
															&(tmp.lthickness), &geo)) {
				switch (tmp.type) {
					case OCIRCLE:
						np_gab_init(&gab);     		 			 
						memcpy(&geo_arc, &geo.circle, sizeof(GEO_CIRCLE));
						if ( lpsc->m ) o_trans_arc((lpGEO_ARC)&geo_arc, lpsc->m, OECS_CIRCLE);
						get_c_matr((lpGEO_CIRCLE)&geo_arc, el_g_e, el_e_g);
						p.x = geo_arc.r;
						p.y = p.z = 0.;
						o_hcncrd(el_e_g, &p, &p);
						geo_arc.angle = 2.*M_PI;
						geo_arc.vb = p;
						geo_arc.ve = p;
						calc_arc_ab(&geo_arc);
						num = o_init_ecs_arc(&ad,&geo_arc, OECS_ARC, vi_h_tolerance);
						for (i = 0; i <= num; i++) {
							o_get_point_on_arc(&ad,(sgFloat)i / num, &p);
							o_modify_limits_by_point(&p, &gab);
						}
						ret = b_sub_arc(&geo_arc, &gab, lpsc);
						break;
					case OARC:
						np_gab_init(&gab);     		 				  
						if ( lpsc->m ) o_trans_arc(&geo.arc, lpsc->m, OECS_ARC);
						if ( lpsc->status & ST_ORIENT ) geo.arc.angle = -geo.arc.angle;
						num = o_init_ecs_arc(&ad,&geo.arc, OECS_ARC, vi_h_tolerance);
						for (i = 0; i <= num; i++) {
							o_get_point_on_arc(&ad,(sgFloat)i / num, &p);
							o_modify_limits_by_point(&p, &gab);
						}
						ret = b_sub_arc(&geo.arc, &gab, &tmp);
						break;
					case OLINE:
						if ( lpsc->m ) {
							o_hcncrd(lpsc->m,&geo_line.v1,&geo_line.v1);
							o_hcncrd(lpsc->m,&geo_line.v2,&geo_line.v2);
						}
						ret = b_sub_line(&geo.line, &tmp);
						break;
					default:
						continue;
				}
				hobj_new = data->listh->hhead;
				while (hobj_new) {
					get_next_item(hobj_new, &hnext);
					detach_item(data->listh, hobj_new);
					obj = (lpOBJ)hobj_new;
					if (count == 32000) {
						
						if ((hobj1 = o_alloc(OFRAME)) == NULL) {
							return OSFALSE;
						}
						obj1 = (lpOBJ)hobj1;
						obj1->color = lpsc->color;    					
						obj1->ltype = lpsc->ltype;    					
						obj1->lthickness = lpsc->lthickness;   
						((lpGEO_FRAME)obj1->geo_data)->num = count;
						((lpGEO_FRAME)obj1->geo_data)->vld = vld;
						o_hcunit(((lpGEO_FRAME)obj1->geo_data)->matr);
						if (!get_object_limits(hobj1, &min, &max) ) {
							o_free(hobj1,NULL);
							return OSFALSE;
						}
						obj1 = (lpOBJ)hobj1;
						((lpGEO_FRAME)obj1->geo_data)->min = min;
						((lpGEO_FRAME)obj1->geo_data)->max = max;
						copy_obj_attrib(lpsc->hobj, hobj1);
						attach_item_tail(&listh, hobj1);
						init_vld(&vld);
						count = 0;
					}
					if ( !add_vld_data(&vld, sizeof(obj->type),&obj->type)) goto err;
					color = obj->color;
					if ( !add_vld_data(&vld, sizeof(color),&color)) goto err;
					if ( !add_vld_data(&vld, sizeof(obj->ltype),&obj->ltype)) goto err;
					if ( !add_vld_data(&vld, sizeof(obj->lthickness),&obj->lthickness)) goto err;
					if ( !add_vld_data(&vld, geo_size[obj->type],obj->geo_data)) goto err;
					count++;
					o_free(hobj_new, NULL);
					hobj_new = hnext;
				}
			}

			if ((hobj1 = o_alloc(OFRAME)) == NULL) return OSFALSE;
			obj1 = (lpOBJ)hobj1;
			obj1->color = lpsc->color;    					
			obj1->ltype = lpsc->ltype;    					
			obj1->lthickness = lpsc->lthickness;  
			((lpGEO_FRAME)obj1->geo_data)->num = count;
			((lpGEO_FRAME)obj1->geo_data)->vld = vld;
			o_hcunit(((lpGEO_FRAME)obj1->geo_data)->matr);
			if (!get_object_limits(hobj1, &min, &max) ) {
				o_free(hobj1,NULL);
				return OSFALSE;
			}
			obj1 = (lpOBJ)hobj1;
			((lpGEO_FRAME)obj1->geo_data)->min = min;
			((lpGEO_FRAME)obj1->geo_data)->max = max;
			copy_obj_attrib(lpsc->hobj, hobj1);
			attach_item_tail(&listh, hobj1);

			*data->listh = listh;
			break;
		default:	// OTEXT  .
			ret = OSTRUE;
 }
	return ret;
err:
	return OSFALSE;

}
static int sort_function( const void *a, const void *b)
{
	 if ( ((lpBO_INTER_POINT)a)->t > ((lpBO_INTER_POINT)b)->t) return 1;
	 return -1;
}
static void b_get_point_on_arc(lpGEO_ARC arc, sgFloat alfa, lpD_POINT p)
{
	ARC_DATA     ad;

	o_init_ecs_arc(&ad, arc, OECS_ARC, vi_h_tolerance);
	o_get_point_on_arc(&ad, alfa, p);
}
OSCAN_COD b_sub_line(lpGEO_LINE line, lpSCAN_CONTROL lpsc)
{
	short 							j, jj;
	VDIM							points;
	D_POINT						v1, v2, p1, p2, p3;
	lpPOINT_DATA			data;
	lpBO_INTER_POINT	v, vv;
	lpGEO_LINE				geo_line;
	BO_INTER_POINT		ip;
	OSCAN_COD					rt = OSTRUE;
	NP_TYPE_POINT 		answer;
	lpOBJ							obj;
	hOBJ							hline;

	data = (lpPOINT_DATA)(lpsc->data);
	if (!init_vdim(&points,sizeof(BO_INTER_POINT))) return OSFALSE;

	v1 = line->v1;
	v2 = line->v2;

	if ( !bo_line_object(&v1, &v2, data->hobj, &points) )	{
		rt = OSFALSE;
		goto end;
	}
	if ( !data->listh ) {  
		if ( points.num_elem == 0 ) goto end;  

		if ( !begin_rw(&points, 0) ) { rt = OSFALSE; goto end; }
		for (j=0; j < points.num_elem; j++) {
			v = (lpBO_INTER_POINT)get_next_elem(&points);

			if ( !begin_rw(data->p,0) ) {
				rt = OSFALSE;
				end_rw(&points);
				goto end;
			}
			for (jj=0; jj < (data->p)->num_elem; jj++) {
				vv = (lpBO_INTER_POINT)get_next_elem(data->p);
				if ( dpoint_eq(&v->p, &vv->p, eps_d) ) break; 
			}
			end_rw(data->p);
			if (jj < (data->p)->num_elem) continue;
			if ( !add_elem(data->p,v) ) {
				rt = OSFALSE;
				end_rw(&points);
				goto end;
			}
		}
		end_rw(&points);
		goto end;
	}

	if ( !sort_vdim(&points, sort_function) ) { rt = OSFALSE; goto end; }
	ip.p = v2;
	ip.type = B_IN_ON;        
	if ( !add_elem(&points, &ip) ) { rt = OSFALSE; goto end; }
	if ( !begin_rw(&points, 0) ) { rt = OSFALSE; goto end; }
	p1 = v1;
	for (j=0; j < points.num_elem; j++) {
		v = (lpBO_INTER_POINT)get_next_elem(&points);
		p2 = v->p;
		if ( fabs(dpoint_distance(&p1, &p2)) < eps_d) {
			p1 = p2;
			continue;
		}
		dpoint_parametr(&p1, &p2, .5, &p3); 
		if ( !check_point_object(&p3, data->hobj, &answer) ) {
			end_rw(&points);
			rt = OSFALSE;
			goto end;
		}
		if ( answer == NP_OUT && data->key == SUB ||
				(answer == NP_IN || answer == NP_ON) && data->key == INTER ) { 
			if((hline = o_alloc(OLINE)) == NULL) {
				end_rw(&points);
				rt = OSFALSE;
				goto end;
			}
			obj = (lpOBJ)hline;
			obj->color = lpsc->color;    					
			obj->ltype = lpsc->ltype;    					
			obj->lthickness = lpsc->lthickness;   
			geo_line = (lpGEO_LINE)(obj->geo_data);
			geo_line->v1 = p1;
			geo_line->v2 = p2;
			copy_obj_attrib(lpsc->hobj, hline);
			attach_item_tail(data->listh, hline);
		}
		p1 = p2;
	}
	end_rw(&points);
end:
	free_vdim(&points);
	return rt;
}
OSCAN_COD b_sub_arc(lpGEO_ARC arc, lpREGION_3D gab_arc, lpSCAN_CONTROL lpsc)
{
	short 							j, jj;
	sgFloat						t1, t2;
	VDIM							points;
	D_POINT						p1, p2, p3;
	lpPOINT_DATA			data;
	lpBO_INTER_POINT	v, vv;
	lpGEO_ARC					geo_arc;
	BO_INTER_POINT		ip;
	OSCAN_COD					rt = OSTRUE;
	NP_TYPE_POINT 		answer;
	lpOBJ							obj;
	hOBJ							harc;//nb  = NULL;

	data = (lpPOINT_DATA)(lpsc->data);
	if (!init_vdim(&points,sizeof(BO_INTER_POINT))) {	rt=OSFALSE; goto end;}

	if ( !bo_arc_object(arc, gab_arc, data->hobj, &points) ) return OSFALSE;

	if ( !data->listh ) {    
		if ( points.num_elem == 0 ) goto end;  

		if ( !begin_rw(&points, 0) ) { rt = OSFALSE; goto end; }
		for (j=0; j < points.num_elem; j++) {
			v = (lpBO_INTER_POINT)get_next_elem(&points);

			if ( !begin_rw(data->p,0) ) {
				rt = OSFALSE;
				end_rw(&points);
				goto end;
			}
			for (jj=0; jj < (data->p)->num_elem; jj++) {
				vv = (lpBO_INTER_POINT)get_next_elem(data->p);
				if ( dpoint_eq(&v->p, &vv->p, eps_d) ) break; 
			}
			end_rw(data->p);
			if (jj < (data->p)->num_elem) continue;
			if ( !add_elem(data->p,v) ) {
				rt = OSFALSE;
				end_rw(&points);
				goto end;
			}
		}
		end_rw(&points);
		goto end;
	}

	if ( !sort_vdim(&points, sort_function) ) { rt = OSFALSE; goto end; }
	ip.p = arc->ve;
	ip.type = B_IN_ON;        
	ip.t = fabs(arc->angle);
	if ( !add_elem(&points, &ip) ) { rt = OSFALSE; goto end; }
	if ( !begin_rw(&points, 0) ) { rt = OSFALSE; goto end; }
	p1 = arc->vb;    t1 = 0;
	for (j=0; j < points.num_elem; j++) {
		v = (lpBO_INTER_POINT)get_next_elem(&points);
		p2 = v->p;    t2 = v->t;
		b_get_point_on_arc(arc, (t2-(t2-t1)/2)/fabs(arc->angle), &p3); 
		if ( !check_point_object(&p3, data->hobj, &answer) ) {
			end_rw(&points);
			rt = OSFALSE;
			goto end;
		}
		if ( answer == NP_OUT && data->key == SUB ||
				(answer == NP_IN || answer == NP_ON) && data->key == INTER ) { 
			if((harc = o_alloc(OARC)) == NULL) {
				end_rw(&points);
				rt = OSFALSE;
				goto end;
			}
			obj = (lpOBJ)harc;
			obj->color = lpsc->color;    					
			obj->ltype = lpsc->ltype;    					
			obj->lthickness = lpsc->lthickness;   
			geo_arc = (lpGEO_ARC)(obj->geo_data);
			*geo_arc = *arc;
			geo_arc->vb = p1;
			geo_arc->ve = p2;
			geo_arc->angle = (t2 - t1)*sg(arc->angle,eps_d);
			calc_arc_ab(geo_arc);
			copy_obj_attrib(lpsc->hobj, harc);
			attach_item_tail(data->listh, harc);
		}
		p1 = p2;   t1 = t2;
	}
	end_rw(&points);
end:
	free_vdim(&points);
	return rt;
}
static  void o_modify_limits_by_point(lpD_POINT p, lpREGION_3D reg)
{
	if (p->x < reg->min.x) reg->min.x = p->x;
	if (p->y < reg->min.y) reg->min.y = p->y;
	if (p->z < reg->min.z) reg->min.z = p->z;
	if (p->x > reg->max.x) reg->max.x = p->x;
	if (p->y > reg->max.y) reg->max.y = p->y;
	if (p->z > reg->max.z) reg->max.z = p->z;
}

static BOOL	get_ends_point(hOBJ hobj, lpD_POINT p1, lpD_POINT p2)
{
	lpOBJ		obj;
	D_POINT	tmp;
	sgFloat	ln;

	obj = (lpOBJ)hobj;
	switch (obj->type) {
		case OLINE:
			*p1 = ((lpGEO_LINE)(obj->geo_data))->v1;
			*p2 = ((lpGEO_LINE)(obj->geo_data))->v2;
			break;
		case OARC:
			*p1 = ((lpGEO_ARC)(obj->geo_data))->vb;
			*p2 = ((lpGEO_ARC)(obj->geo_data))->ve;
			break;
		case OPATH:
			if ( !init_get_point_on_path(hobj, &ln) ) return FALSE;
			if ( !get_point_on_path(0, p1) ) {
				term_get_point_on_path();
				return FALSE;
			}
			if ( !get_point_on_path(ln, p2) ) {
				term_get_point_on_path();
				return FALSE;
			}
			term_get_point_on_path();
			return TRUE;
		default:
			return FALSE;
	}
	if (obj->status & ST_DIRECT) {
		tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
	}
	return TRUE;
}

static OSCAN_COD b_sub_spline(lpGEO_SPLINE spline, lpSCAN_CONTROL lpsc){
	short 							i, j, j1, k, l, num_elem, ipp, span, span_base=60;
	short								tes1, tes2, span_base1=10;
	sgFloat						t, t1, t_beg, dist, bound[2], tmp_bound, t_sply;
	sgFloat						*t_intersec/*=NULL*/;
	SPLY_DAT					sply_dat;
	lpPOINT_DATA			data;
	VDIM							points, points1;
	D_POINT						p, v1, v2, vv2, plane[3];
	BO_INTER_POINT		vv1;
	OSCAN_COD					rt = OSFALSE;
	lpGEO_SPLINE			geo_spline;
	NP_TYPE_POINT 		answer;
	lpOBJ							obj;
	hOBJ							hspline = NULL;

	data = (lpPOINT_DATA)(lpsc->data);

	if((t_intersec=(sgFloat*)SGMalloc(100*sizeof(sgFloat))) == NULL ) return rt;
	l=0;

//found beginning point for intersection
	tes1=0;
	if( !begin_use_sply( spline, &sply_dat) ) goto end1;
	for(i=0; i<sply_dat.numk-1; i++ ){
		t = sply_dat.u[i+1] - sply_dat.u[i];
		span = span_base + (short)(t*100);
		t = t/span;
		memcpy(&v1, &sply_dat.knots[i], sizeof(DA_POINT));
		bound[0]=sply_dat.u[i];
		for( j=1; j<=span; j++ ){
			if( !tes1 ) {
				if( !init_vdim(&points,sizeof(BO_INTER_POINT))) goto end2;
				tes1=1;
			}
			t_beg = sply_dat.u[i]+t*j;
			bound[1] = tmp_bound = t_beg;
			if( j==span ) memcpy(&v2, &sply_dat.knots[i+1], sizeof(DA_POINT));
			else       		get_point_on_sply(&sply_dat, t_beg, &v2, 0);
			if( (rt = (OSCAN_COD)bo_line_object(&v1, &v2, data->hobj, &points) ) != OSTRUE )
					 goto end3;

			tes2=0;
			if ( points.num_elem != 0 ){  // there are points of intersection
				tes1=0;
				t_beg = bound[0];
				t1 = t/span_base1;
				for( j1=1; j1<=span_base1; j1++ ){
					if( !tes2 ){
						if (!init_vdim(&points1,sizeof(BO_INTER_POINT))) goto end3;
						tes2=1;
					}
					t_beg += t1;
					bound[1]=t_beg;
					if( j1==span_base1 ) memcpy(&vv2, &v2, sizeof(DA_POINT));
					else       		       get_point_on_sply(&sply_dat, t_beg, &vv2, 0);

					if( (rt = (OSCAN_COD)bo_line_object(&v1, &v2, data->hobj, &points1) ) != OSTRUE )
							 goto end4;

					if ( points1.num_elem != 0 ){  // there are points of intersection
						tes2=0;
						num_elem=0;
						while( read_elem( &points, num_elem++, &vv1 )){
							t_sply=bound[0];
              p=vv1.p;
							if(!Spline_By_Point( &sply_dat, (void  *)&p, &t_sply, &dist )) return OSFALSE;

//call function for calculate real point of intersection
//create plane to intersect
							plane[0]=vv1.p;
							plane[1]=vv1.p1;
							plane[2]=vv1.p2;
							if( !Spline_By_Plane( &sply_dat, (void  *)plane, &t_sply, bound, &dist )	) goto end4;
//					if( dist >= eps_d || t_beg < 0. ) continue; // tested point is out of the spline
							if( t_sply < 0. ) continue; //tested point out of span

							for( ipp=0, k=0; k<l; k++ )
								if( fabs( t_intersec[k] - t_beg ) < eps_n ){// this point already exists
									ipp=1; break;}

							if( !ipp ){
								t_intersec[l++] = t_beg;
								if( !data->listh ){     // found only points of intersection
									get_point_on_sply(&sply_dat, t_beg, &vv1.p, 0);
									if( !add_elem( data->p, &vv1 ) )	goto end4;
								}
							}
						}
					}
					v1=vv2;
					bound[0]=bound[1];
					if( !tes2 ) free_vdim(&points1);
				}
			}
			v1=v2;
			bound[0]=tmp_bound;
			if( !tes1 ) free_vdim(&points);
		}
	}

	if( !data->listh ){     // found only points of intersection
		rt = OSTRUE;
		goto end2;
	}
// sorting points of intersection by parameter
	qsort( (void *)t_intersec, l, sizeof(sgFloat), sort_founction1 );
	if( fabs( t_intersec[0] ) > eps_n ){
		for( i=l-1; i>=0; i-- ) t_intersec[i+1]= t_intersec[i];
		t_intersec[0]=0.;
		l++;
	}
	if( fabs( t_intersec[l-1]-1. ) > eps_n )	t_intersec[l++]=1.;
//detect spline part lieing in/out of the body
	for( i=0; i<l-1; i++ ){
		get_point_on_sply(&sply_dat,
			t_intersec[i]+(t_intersec[i+1]-t_intersec[i])/2., &v1, 0);
		if ( !check_point_object(&v1, data->hobj, &answer) ) goto end2;
		if ( answer == NP_OUT	 && data->key == SUB ||
				(answer == NP_IN || answer == NP_ON) && data->key == INTER
				){ // create new sply
			if((hspline = o_alloc(OSPLINE)) == NULL) goto end2;
			obj = (lpOBJ)hspline;
			obj->color = lpsc->color;    					// Color
			obj->ltype = lpsc->ltype;    					// Line Type
			obj->lthickness = lpsc->lthickness;   // Thickness

			geo_spline = (lpGEO_SPLINE)(obj->geo_data);

			if( !(Get_Part_Spline_Geo( &sply_dat,
				 t_intersec[i], t_intersec[i+1], geo_spline ))) goto end2;
			copy_obj_attrib(lpsc->hobj, hspline);
			attach_item_tail(data->listh, hspline);
		}
	}
	rt=OSTRUE;
end4:
	free_vdim(&points1);
end3:
	free_vdim(&points);
end2:
	end_use_sply( spline, &sply_dat);
end1:
	if( t_intersec ) SGFree( t_intersec );
	return rt;
}

static int sort_founction1(const void *a, const void *b){
	 if( ( *(sgFloat*)a - *(sgFloat*)b ) >  eps_n ) return  1;
	 if( ( *(sgFloat*)a - *(sgFloat*)b ) < -eps_n ) return -1;
	 return 0;
}

