#include "../sg.h"

static OSCAN_COD skin6_geo_scan(hOBJ hobj, lpSCAN_CONTROL lpsc);
static hOBJ new_begin_path(hOBJ hobj, lpVDIM vdim, short num);

BOOL apr_rib( hOBJ hobj, short work_type );
static BOOL Skinned_Surface_BRP( lpLISTH list, lpVDIM begin_point,
																 lpMESHDD mdd );
/**********************************************************
*  Cross-section design - NURBS skining
*  rib_curves - set of cross section ( non approximed )
*  closed     - =TRUE - if spine curve is closed
*  solid      - =TRUE - closer for solid
*/
BOOL Skin_Surface_BRP( lpLISTH rib_curves, lpVDIM begin_point,
											 BOOL closed,	 BOOL solid, hOBJ *hrez){
	MESHDD      mdd;
	lpNPW       np = NULL;
	BOOL				cod;

	c_num_np	= -32767;
	*hrez			= NULL;
	if ( !np_init_list(&c_list_str) ) return FALSE;

	init_vdim(&mdd.vdim,sizeof(MNODE));
	if( Skinned_Surface_BRP( rib_curves, begin_point, &mdd ) == 0 ) goto err1;

	if (!(np_mesh3p(&c_list_str,&c_num_np,&mdd,0.))) goto err1;

//   
	if( !closed && solid ){
		if ((np = creat_np_mem(TNPW,MAXNOV,MAXNOE,MAXNOC, MAXNOF,
													MAXNOE)) == NULL)  goto err1;
		np_init((lpNP)np);
		np->ident = c_num_np++;
		if( !put_top_bottom( np, &mdd )) goto err1;
		if( np->nov > MAXNOV ||	np->noe > MAXNOE ){
			if( !sub_division( np ) ) goto err1;
		} else {
			if( !np_cplane( np ) ) goto err1;
			if( !np_put( np, &c_list_str ) ) goto err1;
		}
//    
		np->ident = c_num_np++;
		put_bottom_bottom_skin( np, &mdd );
		if( np->nov > MAXNOV ||	np->noe > MAXNOE ){
			if( !sub_division( np ) ) goto err1;
		}	else {
			if( !np_cplane( np ) ) goto err1;
			if( !np_put( np, &c_list_str ) ) goto err1;
		}
	free_np_mem(&np);
	}

	free_vdim(&mdd.vdim);
	cod = cinema_end(&c_list_str,hrez);
	return cod;
err1:
	free_vdim(&mdd.vdim);
	free_np_mem(&np);
	np_end_of_put(&c_list_str,NP_CANCEL,0,NULL);
	return FALSE;
}

/**********************************************************
* calculate points on skinning surface
* list - list of cross-section
* mdd  - bult points
*/
static BOOL Skinned_Surface_BRP( lpLISTH list, lpVDIM begin_point, lpMESHDD mdd ){
	short      i, rib_point, n_section, mask;
	short		 *quantaty;
	MNODE    node_s;
	hOBJ   	 object, object1;

	if( (quantaty = (short*)SGMalloc(rib_data.n_primitiv*sizeof(short) )) == NULL)goto err;
	object = list->hhead;  // list begining

	if( (rib_data.n_point = (short*)SGMalloc(rib_data.n_primitiv*sizeof(short))) == NULL)
			goto err;
	n_section=0;
	while( object != NULL ){      // while objects exist into list
		rib_data.n_num=0;
		if ((object1 = new_begin_path(object, begin_point, n_section)) == NULL) goto err;
// calculate nessesary number of point on each primitiv
		for( i=0; i<rib_data.n_primitiv; i++ ) rib_data.n_point[i] = 0;
		apr_rib( object1, 1 );//Approximate cross sections
		if( n_section == 0 )
			for( i=0; i<rib_data.n_primitiv; i++ )
				quantaty[i] = rib_data.n_point[i];
		else
			for( i=0; i<rib_data.n_primitiv; i++ )
				if( rib_data.n_point[i] > quantaty[i] )
								quantaty[i] = rib_data.n_point[i];
		if(object1 != object) o_free(object1, NULL);
		get_next_item_z( SEL_LIST, object, &object );
		n_section++;
	}

	for( i=0; i<rib_data.n_primitiv; i++ )	rib_data.n_point[i] =	quantaty[i];
	SGFree( quantaty);

	object = list->hhead;  // list begining
	n_section=0;
	while( object != NULL ){ // while objects exist into list
		rib_data.n_num=0;
		rib_point=0;
		if ((object1 = new_begin_path(object, begin_point, n_section)) == NULL) goto err;
		if ( !init_vdim(&rib_data.vdim, sizeof(MNODE)) ) goto err;
		apr_rib( object1, 2 );//Approximate cross sections

		while(	read_elem ( &rib_data.vdim, rib_point, &node_s ) ){// and put into MESDD
			rib_point++;
			mask = (short) CONSTR_U;
			if( !(node_s.num & AP_CONSTR)) mask |= CONSTR_V;
			else if (node_s.num & AP_SOPR || node_s.num & AP_COND) mask |= COND_V;
			node_s.num = mask;
			if( !add_elem(&mdd->vdim, &node_s ) ) goto err;
		}
		if(object1 != object) o_free(object1, NULL);
		get_next_item_z( SEL_LIST, object, &object );
		free_vdim(&rib_data.vdim);
		n_section++;
	}
	mdd->m=n_section;
	mdd->n=rib_point;

	SGFree(rib_data.n_point);
	return TRUE;
err:
	if(rib_data.n_point) SGFree(rib_data.n_point);
	if(&rib_data.vdim  ) free_vdim(&rib_data.vdim);
	return FALSE;
}
//------------------------------------------------------------------>>>>>
typedef struct {
	LISTH 	listh1, listh2;
	D_POINT p;
	BOOL		start;
} DATA2;

static hOBJ new_begin_path(hOBJ hobj, lpVDIM vdim, short num){
	hOBJ 					hobj1;
	OSTATUS 			status;
	D_POINT				p, p1;
	lpGEO_SPLINE	spline;
	VADR					hpoint1, hderivates1;
	lpD_POINT			point, point1;
	lpMNODE				mnode, mnode1;
	int						i, ibeg, j;
	lpOBJ					obj;
	DATA2         data;
	SCAN_CONTROL	sc;
	GEO_PATH			path;
	OBJTYPE				type;
	GEO_CIRCLE		circle;
	GEO_ARC				arc;

	if (!get_status_path(hobj, &status)) return NULL;
	if (status & ST_CLOSE) {
		if (!read_elem(vdim, num, &p)) return NULL;
		obj = (lpOBJ)hobj;
		type = obj->type;
		memcpy(&circle, obj->geo_data, sizeof(GEO_CIRCLE));
		if (type == OCIRCLE) {
			arc.r  = circle.r;
			arc.n  = circle.n;
			arc.vc = circle.vc;
			arc.vb = p;
			arc.ve = p;
			arc.angle = 2 * M_PI;
			calc_arc_ab(&arc);
			obj = (lpOBJ)hobj;
			hobj1 = create_simple_obj_to_prot(obj, OARC, &arc);
			return hobj1;
		}
		get_endpoint_on_path(hobj, &p1, (OSTATUS)0);
		if (dpoint_eq(&p, &p1, eps_d)) return hobj;	//    
		obj = (lpOBJ)hobj;
		if (obj->type == OSPLINE) {	//    
			spline = (lpGEO_SPLINE)obj->geo_data;
			if ((hpoint1 = SGMalloc(sizeof(D_POINT)*spline->nump)) == NULL) goto err0;
			if (spline->numd > 0) {
				if ((hderivates1 = SGMalloc(sizeof(MNODE)*spline->numd)) == NULL) goto err1;
			}
			point = (lpD_POINT)spline->hpoint;
			for (i = 0; i < spline->nump; i++) {
				if (dpoint_eq(&point[i], &p, eps_d)) break;
			}
			if (i == spline->nump) {
				put_message(INTERNAL_ERROR,"c_skin1",0);
				goto err2;
			}
			ibeg = i;
			point1 = (lpD_POINT)hpoint1;
			if (spline->numd > 0) {
				mnode  = (lpMNODE)spline->hderivates;
				mnode1 = (lpMNODE)hderivates1;
			}
			for (j = 0; i < spline->nump - 1; i++, j++) {
				point1[j] = point[i];
				if (spline->numd > 0) mnode1[j] = mnode[i];
			}
			for (i = 0; i < ibeg; i++, j++) {
				point1[j] = point[i];
				if (spline->numd > 0) mnode1[j] = mnode[i];
			}
			point1[j] = point[ibeg];
			if (spline->numd > 0) mnode1[j] = mnode[ibeg];
			SGFree(spline->hpoint);
			spline->hpoint = hpoint1;
			if (spline->numd > 0) {
				SGFree(spline->hderivates);
				spline->hderivates = hderivates1;
			}
			hobj1 = hobj;
		} else {	// OPATH  ( OARC   OCIRCLE!)
			init_listh(&data.listh1);
			init_listh(&data.listh2);
			data.start = FALSE;
			data.p = p;
			init_scan(&sc);
			sc.user_geo_scan = skin6_geo_scan;
			sc.data          = &data;
			if (o_scan(hobj, &sc) == OSFALSE) goto err0;
			while ((hobj1 = data.listh1.hhead) != NULL) {
				detach_item(&data.listh1, hobj1);
				attach_item_tail(&data.listh2, hobj1);
			}
			obj = (lpOBJ)hobj;
			memcpy(&path, obj->geo_data, sizeof(GEO_PATH));
			path.listh = data.listh2;
			hobj1 = create_simple_obj_to_prot(obj, OPATH, &path);
			obj = (lpOBJ)hobj1;
			obj->status &= ~ST_DIRECT;
		}
		return hobj1;
	}
	return hobj;
err2:
	if (spline->numd > 0) SGFree(hderivates1);
err1:
	SGFree(hpoint1);
err0:
	return NULL;
}


static OSCAN_COD skin6_geo_scan(hOBJ hobj, lpSCAN_CONTROL lpsc){
	hOBJ		hobj1;
	lpOBJ		obj;
	DATA2		*data;
	D_POINT	p;

	data = (DATA2*)lpsc->data;
	if (!o_copy_obj(hobj, &hobj1, "")) return OSFALSE;
	obj = (lpOBJ)hobj1;
	obj->status &= ~ST_DIRECT;
	obj->status |= lpsc->status & ST_DIRECT;
	if (!data->start) {
		get_endpoint_on_path(hobj1, &p, (OSTATUS)0);
		if (dpoint_eq(&p, &data->p, eps_d)) data->start = TRUE;
		else attach_item_tail(&data->listh1, hobj1);
	}
	if (data->start) {
		attach_item_tail(&data->listh2, hobj1);
	}
	return OSTRUE;
}
