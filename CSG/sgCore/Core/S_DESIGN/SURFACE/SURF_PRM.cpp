#include "../../sg.h"

hOBJ Create_Cone_Surf( sgFloat r1_OX, sgFloat r1_OY,
                       sgFloat r2_OX, sgFloat r2_OY, sgFloat h ){
sgFloat			coeff;
D_POINT  		point;
SPLY_DAT 		sply_dat;
SURF_DAT 		surf_dat;
GEO_SURFACE geo_surf;
hOBJ 				hrez;
MESHDD      mdd;
int tmp_int;

	if( !init_sply_dat( SPL_NEW, 2, SPL_APPR, &sply_dat ) ) return NULL;

  point.x=point.y=point.z=0;
  if( r1_OX>eps_d ){
	  if( !add_sply_point( &sply_dat, &point, sply_dat.nump)) goto err;
	  point.x=r1_OX/2;
	  if( !add_sply_point( &sply_dat, &point, sply_dat.nump)) goto err;
 	  point.x=r1_OX;
 	  if( !add_sply_point( &sply_dat, &point, sply_dat.nump)) goto err;
  }

  point.x=r1_OX+(r2_OX-r1_OX)/2.;  point.z=h/2;
  if( !add_sply_point( &sply_dat, &point, sply_dat.nump)) goto err;
  point.x=r2_OX; point.z=h;
  if( !add_sply_point( &sply_dat, &point, sply_dat.nump)) goto err;

  if( r2_OX>eps_d ){
  	point.x=r2_OX/2;
	  if( !add_sply_point( &sply_dat, &point, sply_dat.nump)) goto err;
 	  point.x=0;
	  if( !add_sply_point( &sply_dat, &point, sply_dat.nump)) goto err;
  }
/// 
	if( r1_OX>eps_d && r2_OX>eps_d ){
  	sply_dat.u[0]=0.; 	sply_dat.u[1]=1./6.;	sply_dat.u[2]=1./3.;	sply_dat.u[3]=0.5;
    sply_dat.u[4]=2./3.;sply_dat.u[5]=5./6.;	sply_dat.u[6]=1.;
    sply_dat.U[0]=sply_dat.U[1]=sply_dat.U[2]=0.;
    sply_dat.U[3]=sply_dat.U[4]=1./3.;
    sply_dat.U[5]=sply_dat.U[6]=2./3.;
    sply_dat.U[7]=sply_dat.U[8]=sply_dat.U[9]=1.;
    sply_dat.numU=10;
  }else{ //
  	sply_dat.u[0]=0.; 	sply_dat.u[1]=0.25;	sply_dat.u[2]=0.5;
    sply_dat.u[3]=0.75; sply_dat.u[4]=1.;
    sply_dat.U[0]=sply_dat.U[1]=sply_dat.U[2]=0.;
    sply_dat.U[3]=sply_dat.U[4]=0.5;
    sply_dat.U[5]=sply_dat.U[6]=sply_dat.U[7]=1.;
    sply_dat.numU=8;
  }

//  
  if( !init_surf_dat( SURF_NEW, 2, 2, SURF_APPR, &surf_dat ) ) goto err;

//   
  if( r1_OY>eps_d) coeff=r1_OX/r1_OY;
  else{
	  if( r2_OY>eps_d) coeff=r2_OX/r2_OY;
    else             coeff=1.;
  }

  Create_Full_Surface_of_Revolution( &sply_dat, &surf_dat, coeff );

//    U
  tmp_int = (int)surf_dat.sdtype;
  tmp_int |= SURF_CLOSEU;
	surf_dat.sdtype = (SURF_TYPE_IN)tmp_int;//|= SURF_CLOSEU;
//  
  tmp_int = (int)SURF_POLEV0;
	tmp_int |= SURF_POLEV1;
  surf_dat.poles = (SURF_POLES)tmp_int;//SURF_POLEV0;
  //surf_dat.poles |= SURF_POLEV1;

	memset(&hrez, 0, sizeof(hOBJ));
	if ( !np_init_list(&c_list_str) ) goto err1;
//------------------------------------------------------------------------------
//   
	if( !create_geo_surf(&surf_dat, &geo_surf)) goto err2;
//------------------------------------------------------------------------------
	init_vdim(&mdd.vdim,sizeof(MNODE));
  if( !Bild_Mesh_On_Surf(&surf_dat, &mdd, 3, 3) ) goto err3;
//------------------------------------------------------------------------------
  if( (hrez=create_BREP_surface(&geo_surf, &mdd )) == NULL ) goto err4;
//------------------------------------------------------------------------------
	free_geo_surf( &geo_surf );
	free_vdim(&mdd.vdim);
  free_surf_data(&surf_dat);
  free_sply_dat (&sply_dat);
  return hrez;

err4:
	free_vdim(&mdd.vdim);
err3:
	free_geo_surf( &geo_surf );
err2:
	np_end_of_put(&c_list_str,NP_CANCEL,0,NULL);
err1:
  free_surf_data(&surf_dat);
err:
  free_sply_dat(&sply_dat);
  return NULL;
}

hOBJ Create_Sphere_Surf( sgFloat r_OX, sgFloat r_OY, sgFloat r_OZ ){
D_POINT  		point;
SPLY_DAT 		sply_dat;
SURF_DAT 		surf_dat;
GEO_SURFACE geo_surf;
hOBJ 				hrez;
MESHDD      mdd;
int     tmp_int;


	if( !init_sply_dat( SPL_NEW, 2, SPL_APPR, &sply_dat ) ) return NULL;

  point.x=point.y=0;  point.z=-r_OZ;
  if( !add_sply_point( &sply_dat, &point, sply_dat.nump)) goto err;
  sply_dat.P[0].vertex[3]=1.;

  point.x=r_OX;
  if( !add_sply_point( &sply_dat, &point, sply_dat.nump)) goto err;
  Create_4D_From_3D( &sply_dat.P[1], (sgFloat *)&point, 0.5 );

  point.z=r_OZ;
  if( !add_sply_point( &sply_dat, &point, sply_dat.nump)) goto err;
  Create_4D_From_3D( &sply_dat.P[2], (sgFloat *)&point, 0.5 );

  point.x=0;
  if( !add_sply_point( &sply_dat, &point, sply_dat.nump)) goto err;
  sply_dat.P[3].vertex[3]=1.;

  sply_dat.u[0]=0.;  sply_dat.u[1]=0.25;	sply_dat.u[2]=0.75;	sply_dat.u[3]=1.;
  sply_dat.U[0]=sply_dat.U[1]=sply_dat.U[2]=0.;
  sply_dat.U[3]=0.5;
	sply_dat.U[4]=sply_dat.U[5]=sply_dat.U[6]=1.;
  sply_dat.numU=7;

//  
  if( !init_surf_dat( SURF_NEW, 2, 2, SURF_APPR, &surf_dat ) ) goto err;

//   
	Create_Full_Surface_of_Revolution( &sply_dat, &surf_dat, r_OX/r_OY );

//    U
	tmp_int = (int)surf_dat.sdtype;
  tmp_int |= SURF_CLOSEU;
	surf_dat.sdtype = (SURF_TYPE_IN)tmp_int;
//	surf_dat.sdtype |= SURF_CLOSEU;
//  
	tmp_int = (int)SURF_POLEV0;
	tmp_int |= SURF_POLEV1;
  surf_dat.poles = (SURF_POLES)tmp_int;
 // surf_dat.poles = SURF_POLEV0;
 // surf_dat.poles |= SURF_POLEV1;
//------------------------------------------------------------------------------
	memset(&hrez, 0, sizeof(hOBJ));
	if ( !np_init_list(&c_list_str) ) goto err1;
//   
	if( !create_geo_surf(&surf_dat, &geo_surf)) goto err2;
//------------------------------------------------------------------------------
	init_vdim(&mdd.vdim,sizeof(MNODE));
  if( !Bild_Mesh_On_Surf(&surf_dat, &mdd, 3, 3)) goto err3;
//------------------------------------------------------------------------------
  if( (hrez=create_BREP_surface(&geo_surf, &mdd )) == NULL ) goto err4;
//------------------------------------------------------------------------------
	free_geo_surf( &geo_surf );
	free_vdim(&mdd.vdim);
  free_surf_data(&surf_dat);
  free_sply_dat (&sply_dat);
  return hrez;

err4:
	free_geo_surf( &geo_surf );
err3:
	free_vdim(&mdd.vdim);
err2:
  np_end_of_put(&c_list_str,NP_CANCEL,0,NULL);
err1:
  free_surf_data(&surf_dat);
err:
  free_sply_dat(&sply_dat);
  return NULL;
}

hOBJ Create_Spheric_Band_Surf( sgFloat r, sgFloat min_c, sgFloat max_c ){
sgFloat				dp, bp1, bp2, ep1, ep2;
D_POINT				bp, ep, cp;
SPLY_DAT 			sply_dat;
SURF_DAT 			surf_dat;
GEO_SURFACE 	geo_surf;
lpGEO_SPLINE 	geo_spline;
lpGEO_PATH  	geo_path;
lpGEO_LINE		geo_line;
lpGEO_ARC			geo_arc;
hOBJ 					hrez, hobj_new, hobj_path, hobj_line, hobj_arc;
lpOBJ       	obj, obj_path, obj_line, obj_arc;
MESHDD      	mdd;
int tmp_int;

	if ( (hobj_path = o_alloc(OPATH)) == NULL)	return NULL;
	obj_path = (lpOBJ)hobj_path;
	geo_path = (lpGEO_PATH)(obj_path->geo_data);
	o_hcunit(geo_path->matr);
	init_listh(&geo_path->listh);

//  
	bp1=r*sqrt(1-min_c*min_c);
  bp2=r*min_c;

//  
	ep1=r*sqrt(1-max_c*max_c);
  ep2=r*max_c;

//----------------------------- ----------------------------------
	if( min_c != -1. ){
		if((hobj_line = o_alloc(OLINE)) == NULL) goto err_path1;
		obj_line = (lpOBJ)hobj_line;
		obj_line->hhold = hobj_path;
		geo_line = (lpGEO_LINE)(obj_line->geo_data);

		geo_line->v1.x = 0.;
  	geo_line->v1.y = 0.;
		geo_line->v1.z = bp2;

		geo_line->v2.x = bp1;
  	geo_line->v2.y = 0.;
		geo_line->v2.z = bp2;

		attach_item_tail(&geo_path->listh, hobj_line);
	}
//----------------------------- -------------------------------------
	if((hobj_arc = o_alloc(OARC)) == NULL) goto err_path1;
	obj_arc = (lpOBJ)hobj_arc;
	obj_arc->hhold = hobj_path;
	geo_arc = (lpGEO_ARC)(obj_arc->geo_data);

  cp.x=cp.y=cp.z=0.;// 

  bp.x=bp1;
  bp.z=0.;
  bp.y=bp2;

  ep.x=ep1;
  ep.z=0.;
  ep.y=ep2;

	if( !arc_b_e_c(&bp, &ep, &cp, FALSE, FALSE, FALSE, geo_arc)) goto err_path1;
  geo_arc->vb.x=bp1;
  geo_arc->vb.y=0.;
  geo_arc->vb.z=bp2;

  geo_arc->ve.x=ep1;
  geo_arc->ve.y=0.;
  geo_arc->ve.z=ep2;

  if( min_c == -1 && max_c == 1 ) {  	geo_arc->n.x=0.; geo_arc->n.y=-1.; geo_arc->n.z=0.; }
  else	if( !eq_plane( &geo_arc->vb, &geo_arc->ve, &cp,	&geo_arc->n, &dp) ) goto err_path1;

	attach_item_tail(&geo_path->listh, hobj_arc);

//----------------------------- ----------------------------------
	if( max_c != 1 ){
		if((hobj_line = o_alloc(OLINE)) == NULL) goto err_path1;
		obj_line = (lpOBJ)hobj_line;
		obj_line->hhold = hobj_path;
		geo_line = (lpGEO_LINE)(obj_line->geo_data);

		geo_line->v1.x = ep1;
  	geo_line->v1.y = 0.;
	 	geo_line->v1.z = ep2;

		geo_line->v2.x = 0.;
	  geo_line->v2.y = 0.;
		geo_line->v2.z = ep2;

		attach_item_tail(&geo_path->listh, hobj_line);
  }

	obj_path->status |= ST_SIMPLE;

//----------------------------- -----------------------------------
//   
 	if( !transform_to_NURBS( hobj_path, &hobj_new)) goto err_path1;

//    
  obj = (lpOBJ)hobj_new;
	geo_spline = (lpGEO_SPLINE)obj->geo_data;
	if( !begin_use_sply(geo_spline, &sply_dat)) goto err_path2;

//  
  if( !init_surf_dat( SURF_NEW, 2, 2, SURF_APPR, &surf_dat ) ) goto err;

//   
	Create_Full_Surface_of_Revolution( &sply_dat, &surf_dat, 1. );

//    U
	tmp_int = (int)surf_dat.sdtype;
  tmp_int |= SURF_CLOSEU;
	surf_dat.sdtype = (SURF_TYPE_IN)tmp_int;//|= SURF_CLOSEU;
//	surf_dat.sdtype |= SURF_CLOSEU;

//  
	tmp_int = (int)SURF_POLEV0;
	tmp_int |= SURF_POLEV1;
  surf_dat.poles = (SURF_POLES)tmp_int;
 // surf_dat.poles = SURF_POLEV0;
 // surf_dat.poles |= SURF_POLEV1;
//------------------------------------------------------------------------------
	memset(&hrez, 0, sizeof(hOBJ));
	if ( !np_init_list(&c_list_str) ) goto err1;
//   
	if( !create_geo_surf(&surf_dat, &geo_surf)) goto err2;
//------------------------------------------------------------------------------
	init_vdim(&mdd.vdim,sizeof(MNODE));
  if( !Bild_Mesh_On_Surf(&surf_dat, &mdd, 3, 3)) goto err3;
//------------------------------------------------------------------------------
  if( (hrez=create_BREP_surface(&geo_surf, &mdd )) == NULL ) goto err4;
//------------------------------------------------------------------------------
	free_geo_surf( &geo_surf );
	free_vdim(&mdd.vdim);
  free_surf_data(&surf_dat);
  free_sply_dat (&sply_dat);
	o_free(hobj_new, NULL);
	o_free(hobj_path, NULL);
  return hrez;

err4:
	free_vdim(&mdd.vdim);
err3:
	free_geo_surf( &geo_surf );
err2:
  np_end_of_put(&c_list_str,NP_CANCEL,0,NULL);
err1:
  free_surf_data(&surf_dat);
err:
  free_sply_dat(&sply_dat);
err_path2:
	o_free(hobj_new, NULL);
err_path1:
	o_free(hobj_path, NULL);
  return NULL;
}

hOBJ Create_Torus_Surf( sgFloat rn_OX, sgFloat rn_OY, sgFloat ro_OX, sgFloat ro_OY ){
short					i;
sgFloat				w;
D_POINT				p;
SPLY_DAT 			sply_dat;
SURF_DAT 			surf_dat;
GEO_SURFACE 	geo_surf;
lpGEO_SPLINE 	geo_spline;
lpGEO_PATH  	geo_path;
lpGEO_CIRCLE	geo_circle;
hOBJ 					hrez, hobj_new, hobj_path, hobj_circle;
lpOBJ       	obj, obj_path, obj_circle;
MESHDD      	mdd;
int tmp_int;

	if ( (hobj_path = o_alloc(OPATH)) == NULL)	return NULL;
	obj_path = (lpOBJ)hobj_path;
	geo_path = (lpGEO_PATH)(obj_path->geo_data);
	o_hcunit(geo_path->matr);
	init_listh(&geo_path->listh);

//----------------------------- -------------------------------
	if((hobj_circle = o_alloc(OCIRCLE)) == NULL) goto err_path1;
	obj_circle = (lpOBJ)hobj_circle;
	obj_circle->hhold = hobj_path;
	geo_circle = (lpGEO_CIRCLE)(obj_circle->geo_data);

	geo_circle->r=ro_OX;

  geo_circle->n.x=0.;
  geo_circle->n.y=1.;
  geo_circle->n.z=0.;

  geo_circle->vc.x=geo_circle->vc.y=geo_circle->vc.z=0.;

	attach_item_tail(&geo_path->listh, hobj_circle );

	obj_path->status |= ST_SIMPLE;

//----------------------------- -----------------------------------
//   
 	if( !transform_to_NURBS( hobj_path, &hobj_new)) goto err_path1;

//    
  obj = (lpOBJ)hobj_new;
	geo_spline = (lpGEO_SPLINE)obj->geo_data;
	if( !begin_use_sply(geo_spline, &sply_dat)) goto err_path2;
  for( i=0; i<sply_dat.nump; i++ ){
    Create_3D_From_4D( &sply_dat.P[i], (sgFloat *)&p, &w );
    p.x = p.x + rn_OX;
    Create_4D_From_3D( &sply_dat.P[i], (sgFloat *)&p, w );
  }

//  
  if( !init_surf_dat( SURF_NEW, 2, 2, SURF_APPR, &surf_dat ) ) goto err;

//   
	Create_Full_Surface_of_Revolution( &sply_dat, &surf_dat, rn_OX/rn_OY );

//    U
	tmp_int = (int)surf_dat.sdtype;
  tmp_int |= SURF_CLOSEU;
  tmp_int |= SURF_CLOSEV;
	surf_dat.sdtype = (SURF_TYPE_IN)tmp_int;//|= SURF_CLOSEU;
//	surf_dat.sdtype |= SURF_CLOSEU;
//	surf_dat.sdtype |= SURF_CLOSEV;
//------------------------------------------------------------------------------
	memset(&hrez, 0, sizeof(hOBJ));
	if ( !np_init_list(&c_list_str) ) goto err1;
//   
	if( !create_geo_surf(&surf_dat, &geo_surf)) goto err2;
//------------------------------------------------------------------------------
	init_vdim(&mdd.vdim,sizeof(MNODE));
  if( !Bild_Mesh_On_Surf(&surf_dat, &mdd, 3, 3)) goto err3;
//------------------------------------------------------------------------------
  if( (hrez=create_BREP_surface(&geo_surf, &mdd )) == NULL ) goto err4;
//------------------------------------------------------------------------------
	free_geo_surf( &geo_surf );
	free_vdim(&mdd.vdim);
  free_surf_data(&surf_dat);
  free_sply_dat (&sply_dat);
	o_free(hobj_new, NULL);
	o_free(hobj_path, NULL);
  return hrez;

err4:
	free_vdim(&mdd.vdim);
err3:
	free_geo_surf( &geo_surf );
err2:
  np_end_of_put(&c_list_str,NP_CANCEL,0,NULL);
err1:
  free_surf_data(&surf_dat);
err:
  free_sply_dat(&sply_dat);
err_path2:
	o_free(hobj_new, NULL);
err_path1:
	o_free(hobj_path, NULL);
  return NULL;
}


//---------------------->>>>>>CINEMATIC FUNCTION<<<<<<--------------------------

void Create_Full_Surface_of_Revolution( lpSPLY_DAT spline, lpSURF_DAT surface, sgFloat coeff){
  short   i, j;
  sgFloat  w1, w2, R;
  D_POINT point_0, point_1;
  W_NODE  node;

// V - 
  for( i=0; i<spline->nump; i++ ) surface->v[i]=spline->u[i];
  for( i=0; i<spline->numU; i++ ) surface->V[i]=spline->U[i];
  surface->num_v=spline->numU;

// U - 
  surface->u[0]=0.;
	surface->u[1]=0.125;
	surface->u[2]=0.375;
	surface->u[3]=0.5;
	surface->u[4]=0.625;
	surface->u[5]=0.875;
	surface->u[6]=1.;

  surface->U[0]=surface->U[1]=surface->U[2]=0.;
  surface->U[3]=0.25;
  surface->U[4]=surface->U[5]=0.5;
  surface->U[6]=0.75;
  surface->U[7]=surface->U[8]=surface->U[9]=1.;
  surface->num_u=10;

	for( j=0; j<spline->nump; j++ ){
    Create_3D_From_4D( &spline->P[j], (sgFloat *)&point_1, &w1 );
    point_0.x=point_0.y=0.;
	  point_0.z=point_1.z;
    R=dpoint_distance( &point_1, &point_0 );
    w1=spline->P[j].vertex[3];
    w2=0.5*w1;

    point_0.x=R;
		o_hcncrd( surf_matr, &point_0, &point_1 );
    Create_4D_From_3D( &node, (sgFloat *)&point_1, w1 );
		add_elem( &surface->P.vdim, &node ); //---0

    point_0.y=R/coeff;
		o_hcncrd( surf_matr, &point_0, &point_1 );
    Create_4D_From_3D( &node, (sgFloat *)&point_1, w2 );
    add_elem( &surface->P.vdim, &node ); //---1

    point_0.x=-R;
		o_hcncrd( surf_matr, &point_0, &point_1 );
    Create_4D_From_3D( &node, (sgFloat *)&point_1, w2 );
    add_elem( &surface->P.vdim, &node ); //---2

    point_0.y=0;
		o_hcncrd( surf_matr, &point_0, &point_1 );
    Create_4D_From_3D( &node, (sgFloat *)&point_1, w1 );
		add_elem( &surface->P.vdim, &node ); //---3

    point_0.y=-R/coeff;
		o_hcncrd( surf_matr, &point_0, &point_1 );
    Create_4D_From_3D( &node, (sgFloat *)&point_1, w2 );
    add_elem( &surface->P.vdim, &node ); //---4

    point_0.x=R;
		o_hcncrd( surf_matr, &point_0, &point_1 );
    Create_4D_From_3D( &node, (sgFloat *)&point_1, w2 );
    add_elem( &surface->P.vdim, &node ); //---5

    point_0.y=0;
		o_hcncrd( surf_matr, &point_0, &point_1 );
    Create_4D_From_3D( &node, (sgFloat *)&point_1, w1 );
     add_elem( &surface->P.vdim, &node ); //---6
  }

  surface->P.n=7;
  surface->P.m=spline->nump;

// 
	calculate_surf_P(surface, 0);
}

hOBJ Create_Surface_of_Revolution( lpSPLY_DAT spline, sgFloat Fi, short type, BOOL closed ){
short					i, j;
sgFloat 				w1, wij, R;//, coeff;
D_POINT				point_0, point_1, cp;
W_NODE  			node;
SPLY_DAT 			sply_dat;
SURF_DAT 			surf_dat;
GEO_SURFACE 	geo_surf;
lpGEO_SPLINE 	geo_spline;
lpGEO_PATH  	geo_path;
lpGEO_ARC			geo_arc;
hOBJ 					hrez, hobj_new, hobj_path, hobj_arc;
lpOBJ       	obj, obj_path, obj_arc;
MESHDD      	mdd;
int tmp_int;
//	coeff=R_OX/R_OY;
//------------------------------------------------------------------------------
	if( (hobj_path = o_alloc(OPATH)) == NULL )	return NULL;
	obj_path = (lpOBJ)hobj_path;
	geo_path = (lpGEO_PATH)(obj_path->geo_data);
	o_hcunit(geo_path->matr);
	init_listh(&geo_path->listh);
//----------------------------- -------------------------------------
	if((hobj_arc = o_alloc(OARC)) == NULL) goto err_path1;
	obj_arc = (lpOBJ)hobj_arc;
	obj_arc->hhold = hobj_path;
	geo_arc = (lpGEO_ARC)(obj_arc->geo_data);

  cp.x=cp.y=cp.z=0.;// 

  point_0.x=1.;
  point_0.y=0.;
  point_0.z=0.;

  point_1.x=cos(Fi);
  point_1.y=sin(Fi);
  point_1.z=0.;

	if( !arc_b_e_c(&point_0, &point_1, &cp, FALSE, FALSE, FALSE, geo_arc)) goto err_path1;

 	geo_arc->n.x=0.; geo_arc->n.y=0.; geo_arc->n.z=1.;
	attach_item_tail(&geo_path->listh, hobj_arc);

//----------------------------- -----------------------------------
//   
 	if( !transform_to_NURBS( hobj_path, &hobj_new)) goto err_path1;

//    
  obj = (lpOBJ)hobj_new;
	geo_spline = (lpGEO_SPLINE)obj->geo_data;
	if( !begin_use_sply(geo_spline, &sply_dat)) goto err_path2;

//  
  if( !init_surf_dat( SURF_NEW, 2, 2, SURF_APPR, &surf_dat ) ) goto err;

// V - 
  for( i=0; i<spline->nump; i++ ) surf_dat.v[i]=spline->u[i];
  for( i=0; i<spline->numU; i++ ) surf_dat.V[i]=spline->U[i];
  surf_dat.num_v=spline->numU;

// U - 
  for( i=0; i<sply_dat.nump; i++ ) surf_dat.u[i]=sply_dat.u[i];
	for( i=0; i<sply_dat.numU; i++ ) surf_dat.U[i]=sply_dat.U[i];
  surf_dat.num_u=sply_dat.numU;

	for( j=0; j<spline->nump; j++ ){ //  
	  Create_3D_From_4D( &spline->P[j], (sgFloat *)&point_1, &w1 );
		o_hcncrd( surf_matr, &point_1, &point_1 );

	  point_0.x=point_0.y=0.;
	  point_0.z=point_1.z;
    R=dpoint_distance( &point_1, &point_0 );

  	for( i=0; i<sply_dat.nump; i++ ){ //  
  	  wij=w1*sply_dat.P[i].vertex[3];
		  Create_3D_From_4D( &sply_dat.P[i], (sgFloat *)&point_0, &w1 );
      point_1.x=point_0.x*R;
      point_1.y=point_0.y*R;
//      point_1.z=point_0.z;

    	Create_4D_From_3D( &node, (sgFloat *)&point_1, wij );
			add_elem( &surf_dat.P.vdim, &node );
    }
  }

  surf_dat.P.n=sply_dat.nump;
  surf_dat.P.m=spline->nump;

// 
	calculate_surf_P( &surf_dat, 0);

//    U
	if( closed ) 
	{
		tmp_int = (int)surf_dat.sdtype;
		tmp_int |= SURF_CLOSEV;
		surf_dat.sdtype = (SURF_TYPE_IN)tmp_int;
	//	surf_dat.sdtype |= SURF_CLOSEV;
	}
  if( fabs(Fi) >= 2*M_PI ) 
  {
	  int tmp_int = (int)surf_dat.sdtype;
	  tmp_int |= SURF_CLOSEU;
	  surf_dat.sdtype = (SURF_TYPE_IN)tmp_int;
	  //surf_dat.sdtype |= SURF_CLOSEU;
  }

//  
	if( type !=0 ){
		tmp_int = (int)SURF_POLEV0;
		tmp_int |= SURF_POLEV1;
		surf_dat.poles = (SURF_POLES)tmp_int;
	  //surf_dat.poles = SURF_POLEV0;
  	//surf_dat.poles |= SURF_POLEV1;
  }

//------------------------------------------------------------------------------
	memset(&hrez, 0, sizeof(hOBJ));
	if ( !np_init_list(&c_list_str) ) goto err1;
//   
  o_hcunit(surf_matr); //  
	if( !create_geo_surf(&surf_dat, &geo_surf)) goto err2;
//------------------------------------------------------------------------------
	init_vdim(&mdd.vdim,sizeof(MNODE));
  if( !Bild_Mesh_On_Surf(&surf_dat, &mdd, 3, 3)) goto err3;
//------------------------------------------------------------------------------
  if( (hrez=create_BREP_surface(&geo_surf, &mdd )) == NULL ) goto err4;
//------------------------------------------------------------------------------
	free_geo_surf( &geo_surf );
	free_vdim(&mdd.vdim);
  free_surf_data(&surf_dat);
  free_sply_dat (&sply_dat);
	o_free(hobj_new, NULL);
	o_free(hobj_path, NULL);
  return hrez;

err4:
	free_vdim(&mdd.vdim);
err3:
	free_geo_surf( &geo_surf );
err2:
  np_end_of_put(&c_list_str,NP_CANCEL,0,NULL);
err1:
  free_surf_data(&surf_dat);
err:
  free_sply_dat(&sply_dat);
err_path2:
	o_free(hobj_new, NULL);
err_path1:
	o_free(hobj_path, NULL);
  return NULL;
}

hOBJ Create_Extruded_Surface( lpSPLY_DAT spline, lpMATR matr, BOOL closed ){
  short   		i, j;
  sgFloat  		w;
  D_POINT 		point;
  W_NODE  		node;
  SURF_DAT 		surf_dat;
	GEO_SURFACE geo_surf;
  hOBJ 				hrez;
  MESHDD      mdd;

//  
  if( !init_surf_dat( SURF_NEW, 1, 2, SURF_APPR, &surf_dat ) ) return NULL;

// V - 
  for( i=0; i<spline->nump; i++ ) surf_dat.v[i]=spline->u[i];
  for( i=0; i<spline->numU; i++ ) surf_dat.V[i]=spline->U[i];
  surf_dat.num_v=spline->numU;

// U - 
  surf_dat.u[0]=0.;
	surf_dat.u[1]=1.;

  surf_dat.U[0]=surf_dat.U[1]=0.;
  surf_dat.U[2]=surf_dat.U[3]=1.;
  surf_dat.num_u=4;

	for( j=0; j<spline->nump; j++ ){
//  
		add_elem( &surf_dat.P.vdim, &spline->P[j] ); //---0

//  
    Create_3D_From_4D( &spline->P[j], (sgFloat *)&point, &w );
   	o_hcncrd( matr, &point, &point);
//    point.x += d*W->x;
//    point.y += d*W->y;
//    point.z += d*W->z;
    Create_4D_From_3D( &node, (sgFloat *)&point, w );
		add_elem( &surf_dat.P.vdim, &node ); //---1
  }

  surf_dat.P.n=2;
  surf_dat.P.m=spline->nump;

// 
	calculate_surf_P( &surf_dat, 0);

//    U
  if( closed ) 
  {
	int tmp_int = (int)surf_dat.sdtype;
	tmp_int |= SURF_CLOSEV;
	surf_dat.sdtype = (SURF_TYPE_IN)tmp_int;
//	  surf_dat.sdtype |= SURF_CLOSEV;
  }

//------------------------------------------------------------------------------
	memset(&hrez, 0, sizeof(hOBJ));
	if ( !np_init_list(&c_list_str) ) goto err1;
//   
	if( !create_geo_surf(&surf_dat, &geo_surf)) goto err2;
//------------------------------------------------------------------------------
	init_vdim(&mdd.vdim,sizeof(MNODE));
  if( !Bild_Mesh_On_Surf(&surf_dat, &mdd, 3, 3)) goto err3;
//------------------------------------------------------------------------------
  if( (hrez=create_BREP_surface(&geo_surf, &mdd )) == NULL ) goto err4;
//------------------------------------------------------------------------------
	free_geo_surf( &geo_surf );
	free_vdim(&mdd.vdim);
  free_surf_data(&surf_dat);
  return hrez;

err4:
	free_vdim(&mdd.vdim);
err3:
	free_geo_surf( &geo_surf );
err2:
  np_end_of_put(&c_list_str,NP_CANCEL,0,NULL);
err1:
  free_surf_data(&surf_dat);
  return NULL;
}


