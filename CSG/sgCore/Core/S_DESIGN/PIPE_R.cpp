#include "../sg.h"

static OSCAN_COD apr_geo_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);

static  OSCAN_COD  part_line      (lpOBJ obj,lpSCAN_CONTROL lpsc);
static  OSCAN_COD  part_circle    (lpOBJ obj,lpSCAN_CONTROL lpsc);
static  OSCAN_COD  part_arc       (lpOBJ obj,lpSCAN_CONTROL lpsc);
static  OSCAN_COD  part_spline    (lpOBJ obj,lpSCAN_CONTROL lpsc);
static  OSCAN_COD  (**apr_typeg)(lpOBJ obj,lpSCAN_CONTROL lpsc);

typedef struct{
	sgFloat R;// 
	short    num; //    
	sgFloat d_alpha; //   
}FORM; //    
typedef FORM  * lpFORM;

typedef struct{
	lpFORM 	 form;
	lpMESHDD mdd;
	lpMATR	 matr;
}CREATE_PIPE;
typedef CREATE_PIPE * lpCREATE_PIPE;

static BOOL create_real_pipe( lpMESHDD g, hOBJ path, sgFloat R, short num );
static BOOL write_cross 		( lpMESHDD g, hOBJ hpath, sgFloat r, lpMATR matr,
															lpFORM form );
static BOOL apr_pipe        ( lpMESHDD mdd, hOBJ hobj, lpFORM form, lpMATR matr );
static  BOOL create_cyl_part ( lpCREATE_PIPE data, lpD_POINT vc1,
															lpD_POINT vc2, short constr );
static  BOOL create_tor_part ( lpCREATE_PIPE data, lpARC_DATA arc_data, short n_direct,
															short beg, short step, short constr );

//lpNPW	c_np_top = NULL;


BOOL real_pipe_mesh(sgFloat R1, sgFloat R2, short num, hOBJ hpath, bool clos, hOBJ *hrez ){
	short 		i;
	sgFloat	R;
	MESHDD  mdd;
	BOOL		cod;

	c_num_np	= -32767;
	*hrez 		= NULL;
	if ( !np_init_list(&c_list_str) ) return FALSE;

//   
	if ((c_np_top = creat_np_mem(TNPW,MAXNOV,MAXNOE,MAXNOC, MAXNOF,
																MAXNOE)) == NULL)  goto err1;
	np_init((lpNP)c_np_top);
	c_np_top->ident = c_num_np++;
	for( i=0; i<2; i++ ){
		if( i==1 && R2 <= 0 ) break;
		R = (i==0) ? (R1) : (-R2);
//   
		init_vdim(&mdd.vdim,sizeof(MNODE));
		if( !create_real_pipe( &mdd, hpath, R, num ) ) goto err1a;
//  
		if( !put_top_bottom( c_np_top, &mdd )) goto err1a;
		if (!(np_mesh4p(&c_list_str,&c_num_np,&mdd,0.))) goto err1a;
		free_vdim(&mdd.vdim);
	}

	free_np_mem(&c_np_top);
	cod = cinema_end(&c_list_str,hrez);
	return cod;

err1a:
	free_vdim(&mdd.vdim);
err1:
	free_np_mem(&c_np_top);
	np_end_of_put(&c_list_str,NP_CANCEL,0,NULL);
	return FALSE;
}

static BOOL create_real_pipe( lpMESHDD g, hOBJ hpath, sgFloat R, short num ){
FORM form;
MATR matr;

	form.num = num;
//     
	if( !write_cross( g, hpath, R, matr, &form ) ) return FALSE;
	if( (form.num + 1) > MAXSHORT ) {
		cinema_handler_err(CIN_OVERFLOW_MDD);
		return FALSE;
	}

	g->n = form.num+1;
	g->m = 1;

//     
	if( !apr_pipe( g, hpath, &form, matr )) return FALSE;
	return TRUE;
}

static BOOL write_cross( lpMESHDD g, hOBJ hpath, sgFloat r,
												 lpMATR matr, lpFORM form ){
	register  short i;
	sgFloat 		alpha;
	short 			mask, step_i;
	OSTATUS   status;
	VDIM 			vdim_path;
	MNODE 		node, node1;
	D_POINT   norm, norm_p={0, 0, 1};
	MATR			matri;

	init_vdim(&vdim_path, sizeof(MNODE));
	if (!apr_path(&vdim_path, hpath))      goto err1;
	if (!read_elem(&vdim_path, 0, &node))  goto err1;
	if (!read_elem(&vdim_path, 1, &node1)) goto err1;
	dpoint_sub(&node1.p, &node.p, &norm);
	free_vdim(&vdim_path);

//  
	o_hcunit(matr);
	o_hcrot1(matr,&norm);
	o_minv(matr,matri);
	o_hcunit(matr);
	o_hcrot1(matr,&norm_p);
	o_hcmult(matr,matri);
	o_tdtran(matr,&node.p);

	if (!get_status_path( hpath, &status)) return FALSE;
	if(status & ST_CLOSE ) mask= COND_U;
	else mask = (short) CONSTR_U;

	form->R = fabs(r);
	form->d_alpha = 2*M_PI/form->num;
	if( r<0 ) form->d_alpha = -form->d_alpha;
	step_i = (short)(0.25*(sgFloat)form->num);
	for( i = 0; i <= form->num; i++ ) {
		alpha = ( i==form->num ) ? (2*M_PI) : (form->d_alpha*i);
		node.p.x = form->R*cos(alpha);
		node.p.y = form->R*sin(alpha);
		node.p.z = 0;
		node.num = mask;
		if (!(i % step_i)) node.num |= COND_V;
//		else node.num |= CONSTR_V;
		if ( !o_hcncrd( matr, &node.p, &node.p ) ) return FALSE;
		if ( !add_elem(&g->vdim, &node) ) return FALSE;
	}
	return TRUE;

err1:
	free_vdim(&vdim_path);
	return FALSE;
}

static BOOL apr_pipe(lpMESHDD mdd, hOBJ hobj, lpFORM form, lpMATR matr){
	SCAN_CONTROL  sc;
	CREATE_PIPE   create_pipe;

	create_pipe.form = form;
	create_pipe.mdd  = mdd;
	create_pipe.matr = matr;

	apr_typeg = (OSCAN_COD (**)(lpOBJ obj,lpSCAN_CONTROL lpsc))GetMethodArray(OMT_PIPE_R);

	apr_typeg[OLINE]  = part_line;		// OLINE
	apr_typeg[OCIRCLE]= part_circle;	// OCIRCLE
	apr_typeg[OARC]   = part_arc;		// OARC
	apr_typeg[OSPLINE]= part_spline;	// OSPLINE

	init_scan(&sc);
	sc.user_geo_scan  = apr_geo_scan;
	reinterpret_cast<CREATE_PIPE* &>(sc.data) = &create_pipe;
	if (o_scan(hobj,&sc) == OSFALSE) return FALSE;
	return TRUE;
}

static  OSCAN_COD apr_geo_scan(hOBJ hobj,lpSCAN_CONTROL lpsc){
	lpOBJ obj;
	BOOL cod;

	obj = (lpOBJ)hobj;
	cod = apr_typeg[lpsc->type](obj,lpsc);
	return (OSCAN_COD)cod;
}

#pragma argsused
static  OSCAN_COD part_line( lpOBJ obj, lpSCAN_CONTROL lpsc ){
	lpGEO_LINE g = (lpGEO_LINE)obj->geo_data;
	short mask;
	lpD_POINT p1, p2;
	OSTATUS constr2;

	if ( lpsc->status&ST_DIRECT ) {
		p1 = &g->v2; p2 = &g->v1;
		constr2 = (OSTATUS)(obj->status&ST_CONSTR1);
	} else {
		p1 = &g->v1; p2 = &g->v2;
		constr2 = (OSTATUS)(obj->status&ST_CONSTR2);
	}
	mask = (constr2) ? AP_CONSTR : AP_NULL;
	mask |= AP_SOPR;
	if( !create_cyl_part( (lpCREATE_PIPE)lpsc->data, p1, p2, mask )) return OSFALSE;

	return OSTRUE;
}

static  BOOL create_cyl_part( lpCREATE_PIPE data, lpD_POINT vc1,
														 lpD_POINT vc2, short constr ){
	int			 i, step_i;
	sgFloat   alpha;
	D_POINT	 v;
	MNODE    node;

//  
	dpoint_sub(vc2, vc1, &v);
	o_tdtran( data->matr, &v );
	step_i = (int)(0.25*(sgFloat)data->form->num);
//   
	for ( i = 0; i <= data->form->num; i++ ) {
		alpha = ( i==data->form->num ) ? (2*M_PI) : (data->form->d_alpha*i);
		node.p.x = data->form->R*cos(alpha);
		node.p.y = data->form->R*sin(alpha);
		node.p.z = 0;
		node.num = constr;
		if (!(i % step_i )) node.num |= COND_V;
//		else node.num |= CONSTR_V;
		if ( !o_hcncrd( data->matr, &node.p, &node.p ) ) return FALSE;
		if ( !add_elem(&data->mdd->vdim, &node) ) return FALSE;
	}
	data->mdd->m++;
	return TRUE;
}

#pragma argsused
static  OSCAN_COD part_circle(lpOBJ obj, lpSCAN_CONTROL lpsc){
	register short i;
	lpGEO_CIRCLE circle = (lpGEO_CIRCLE)obj->geo_data;
	short 				beg, num, step, mask;
	ARC_DATA    ad;

	num = o_init_ecs_arc(&ad,(lpGEO_ARC)circle, OECS_CIRCLE,c_h_tolerance);
	i = num % 4;
	if (i) num += (4 - i);

	if ( lpsc->status&ST_DIRECT ) {
		beg = num; step = -1;
	} else {
		beg = 0; step = 1;
	}

	mask = AP_CONSTR ;

	if( !create_tor_part( (lpCREATE_PIPE)lpsc->data, &ad, num, beg, step, mask )) return OSFALSE;
	return OSTRUE;
}

static  BOOL create_tor_part( lpCREATE_PIPE data, lpARC_DATA arc_data,
														 short n_direct, short beg, short step, short constr ){
	int			 i, j, jj, step_i;	//, step_j, ii, mask;
	sgFloat	 r_direct, alpha_i, alpha_j;
	MNODE		 node;

	r_direct = arc_data->geo_arc.r;

	if( r_direct <= data->form->R ){
		//  
		cinema_handler_err(CIN_OUT_RADIUS_BAD);
		return FALSE;
	}
	alpha_j = arc_data->geo_arc.angle/ n_direct;
//	step_j = n_direct/4.;
	step_i = (int)(0.25*(sgFloat)data->form->num);

//   
	for (jj = beg+step, j = 1; j <= n_direct; jj+=step, j++ ) {
//  
		o_rotate_xyz( data->matr, &arc_data->geo_arc.vc,
															&arc_data->geo_arc.n, step*alpha_j);

/*		if( !(j % step_j )) mask = COND_U;
		else{
		 if( j == n_direct ) mask = constr;
		 else mask = 0;
		}*/

		for ( i = 0; i <= data->form->num; i++ ) {
			alpha_i = ( i==data->form->num ) ? (2*M_PI) : (data->form->d_alpha*i);
			node.p.x = data->form->R*cos(alpha_i);
			node.p.y = data->form->R*sin(alpha_i);
			node.p.z = 0;
			node.num = constr;
			if (!(i % step_i)) node.num |= COND_V;
//			else node.num |= CONSTR_V;
			if ( !o_hcncrd( data->matr, &node.p, &node.p ) ) return FALSE;
			if ( !add_elem(&data->mdd->vdim, &node) ) return FALSE;
		}
	}
	data->mdd->m += n_direct;
	return TRUE;
}

#pragma argsused
static  OSCAN_COD part_arc(lpOBJ obj, lpSCAN_CONTROL lpsc){
	lpGEO_ARC arc = (lpGEO_ARC)&obj->geo_data;
	short 			beg, num, step, mask;
	OSTATUS 	constr2;
	ARC_DATA  ad;

	num = o_init_ecs_arc(&ad, arc, OECS_ARC, c_h_tolerance);
	if ( lpsc->status&ST_DIRECT ) {
		beg = num; step = -1;
		constr2 = (OSTATUS)(obj->status&ST_CONSTR1);
	} else {
		beg = 0; step = 1;
		constr2 = (OSTATUS)(obj->status&ST_CONSTR2);
	}
	mask = (constr2) ? AP_CONSTR : AP_NULL;
	mask |= AP_SOPR;

	if( !create_tor_part( (lpCREATE_PIPE)lpsc->data, &ad, num, beg, step, mask )) return OSFALSE;
	return OSTRUE;
}

#pragma argsused
static  OSCAN_COD part_spline(lpOBJ obj, lpSCAN_CONTROL lpsc){
//       !!!!!!!!!!!!!!!!!!!!!
	return OSFALSE;
}

