#include "../../sg.h"
MATR surf_matr;
//------------------------------------------------------
//BOOL surface_end( lpVLD vld, lpLNP lnp, lpGEO_SURFACE geo_surf );
hOBJ create_BREP_surface(lpGEO_SURFACE geo_surf, lpMESHDD mdd ,bool);
//------------------------------------------------------
//GS_CODE edit_srf(void);

hOBJ surface_edit(lpGEO_SURFACE geo_surf){
SURF_TYPE_IN 	type_s;
SURF_DAT    	surf_dat;
MESHDD      	mdd;
hOBJ        	hrez;

short        srf_flags[8];
#define    fl_cont      srf_flags[3]
#define    fl_connect   srf_flags[4]
#define    fl_splcolor  srf_flags[5]
#define    fl_edit_ok   srf_flags[6]

//   gcond
	memset(&gcond, 0, sizeof(GEO_COND));
       //	fl_connect    = -1;
	gcond.flags   = srf_flags;
	gcond.geo     = (lpGEO_ARC)&surf_dat;

//     
	type_s = (geo_surf->type & NSURF_APPR) ? SURF_APPR : SURF_INT;

  if( !init_surf_dat( SURF_NEW, geo_surf->degreeu, geo_surf->degreev, type_s, &surf_dat )){
		put_message(NOT_ENOUGH_HEAP, NULL, 0);
		return NULL;
  }

//    SPLY_DAT
	if(!unpack_geo_surf(geo_surf, &surf_dat)) goto err;
	free_geo_surf( geo_surf );

//   
	assert(0);
	
  fl_cont = FALSE;


	if ( !np_init_list(&c_list_str) ) goto err;

//   
	if( !create_geo_surf(&surf_dat, geo_surf)) goto err1;
//------------------------------------------------------------------------------
	init_vdim(&mdd.vdim,sizeof(MNODE));
	if( !Bild_Mesh_On_Surf(&surf_dat, &mdd, 3, 3)) goto err2;
//------------------------------------------------------------------------------
  if( (hrez=create_BREP_surface(geo_surf, &mdd )) == NULL ) goto err3;
//------------------------------------------------------------------------------
	free_geo_surf( geo_surf );
	free_vdim(&mdd.vdim);
  free_surf_data(&surf_dat);
//------------------------------------------------------------------------------
  return hrez;
err3:
	free_vdim(&mdd.vdim);
err2:
	free_geo_surf( geo_surf );
err1:
  np_end_of_put(&c_list_str,NP_CANCEL,0,NULL);
err:
	free_surf_data(&surf_dat);
	return NULL;
}

//==============================================================================
/*******************************************************************************
*  BREP   
*/
hOBJ create_BREP_surface(lpGEO_SURFACE geo_surf, lpMESHDD mdd, bool clear_c_num_np /*= true */ ){
//BOOL				ret;
hOBJ        hrez;
lpOBJ       obj;
//LISTH       listho;
lpGEO_BREP  brp;
LNP         lnp;
VLD         vld;
//-----------------------------------------------------------------------------
//  init_listh( &listho );
	if (clear_c_num_np)
		c_num_np	= -32767;
	if( !(np_mesh3p(&c_list_str, &c_num_np, mdd, 0.)) ) return NULL;
  if( !cinema_end(&c_list_str, &hrez) ) return NULL;
//  ret=Create_MESHDD( mdd, &listho );
//  if( ret==FALSE ) return NULL;
//  else             hrez=listho.hhead;
//-----------------------------------------------------------------------------
//create new object
	obj=(lpOBJ)hrez;
  brp=(lpGEO_BREP)obj->geo_data;
  read_elem( &vd_brep, brp->num, &lnp);
  init_listh(&lnp.surf);
  init_vld(&vld);

  if( !surface_end( &vld, &lnp, geo_surf ))  	return NULL;
  write_elem( &vd_brep, brp->num, &lnp);

  return hrez;
}

//==============================================================================
/*******************************************************************************
*      BREP
*/
/*BOOL surface_end( lpVLD vld, lpLNP lnp, lpGEO_SURFACE geo_surf ){
BOOL       ret=FALSE;
short      size;
sgFloat		 *lpparam;
lpDA_POINT lppoint;
lpSNODE    lpderiv;

  if (!add_vld_data(vld, sizeof(geo_surf->type),      &geo_surf->type)) goto err;
  if (!add_vld_data(vld, sizeof(geo_surf->numpu),     &geo_surf->numpu)) goto err;
  if (!add_vld_data(vld, sizeof(geo_surf->numpv),     &geo_surf->numpv)) goto err;
  if (!add_vld_data(vld, sizeof(geo_surf->numd),      &geo_surf->numd)) goto err;
  if (!add_vld_data(vld, sizeof(geo_surf->num_curve), &geo_surf->num_curve)) goto err;
  if (!add_vld_data(vld, sizeof(geo_surf->degreeu),   &geo_surf->degreeu)) goto err;
  if (!add_vld_data(vld, sizeof(geo_surf->degreev),   &geo_surf->degreev)) goto err;
  if (!add_vld_data(vld, sizeof(geo_surf->orient),    &geo_surf->orient)) goto err;

  size = sizeof(D_POINT) * geo_surf->numpu * geo_surf->numpv;
  lppoint=(lpDA_POINT)geo_surf->hpoint;
  ret=add_vld_data(vld, size, lppoint);
  if( ret!=TRUE ) goto err;

  size = sizeof(sgFloat) * (geo_surf->numpu+geo_surf->degreeu+1+
                           geo_surf->numpv+geo_surf->degreev+1);
  lpparam=(sgFloat*)geo_surf->hparam;
  ret=add_vld_data(vld, size, lpparam);
  if( ret!=TRUE ) goto err;

  if (geo_surf->numd > 0) {
    lpderiv=(lpSNODE)geo_surf->hderivates;
	  size = sizeof(MNODE) * geo_surf->numd;
	  ret=add_vld_data(vld, size, lpderiv);
    if( ret!=TRUE ) goto err;
  }

  if (geo_surf->num_curve > 0) {	//  
  }

  lnp->surf = vld->listh;
  lnp->num_surf++;
  ret=TRUE;

err:
  return ret;
}*/