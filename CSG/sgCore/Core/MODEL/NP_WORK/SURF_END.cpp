#include "../../sg.h"

BOOL surface_end( lpVLD vld, lpLNP lnp, lpGEO_SURFACE geo_surf ){
BOOL       ret=FALSE;
int        size;
sgFloat		 *lpparam/* = NULL*/;
lpDA_POINT lppoint/*=NULL*/;
lpSNODE    lpderiv;

  if (!add_vld_data(vld, sizeof(geo_surf->type),      &geo_surf->type)) goto err;
  if (!add_vld_data(vld, sizeof(geo_surf->numpu),     &geo_surf->numpu)) goto err;
  if (!add_vld_data(vld, sizeof(geo_surf->numpv),     &geo_surf->numpv)) goto err;
  if (!add_vld_data(vld, sizeof(geo_surf->numd),      &geo_surf->numd)) goto err;
  if (!add_vld_data(vld, sizeof(geo_surf->num_curve), &geo_surf->num_curve)) goto err;
  if (!add_vld_data(vld, sizeof(geo_surf->degreeu),   &geo_surf->degreeu)) goto err;
  if (!add_vld_data(vld, sizeof(geo_surf->degreev),   &geo_surf->degreev)) goto err;
  if (!add_vld_data(vld, sizeof(geo_surf->orient),   &geo_surf->orient)) goto err;

  size = sizeof(D_POINT) * geo_surf->numpu * geo_surf->numpv;
  lppoint=(lpDA_POINT)geo_surf->hpoint;
  ret=add_vld_data(vld, size, lppoint);
  if( ret!=TRUE ) goto err;

  size = sizeof(sgFloat) * (geo_surf->numpu + geo_surf->degreeu+1+
													 geo_surf->numpv+ geo_surf->degreeu+1);
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
}