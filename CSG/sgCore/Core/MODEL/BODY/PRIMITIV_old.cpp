#include "../../sg.h"


short create_primitive_np(BREPKIND kind,lpLISTH listh, sgFloat  *par)
{
	short num = 0;

	switch (kind) {
		case BOX:
			num = create_box_np(listh, par);
			break;
		case SPHERE:
			num = create_sph_np(listh, par);
			break;
		case CYL:
			num = create_cyl_np(listh, par, FALSE);
			break;
		case CONE:
			num = create_cone_np(listh, par, FALSE);
			break;
		case TOR:
			num = create_tor_np(listh, par);
			break;
		case PRISM:
			num = create_cyl_np(listh, par, TRUE);
			break;
		case PYRAMID:
			num = create_cone_np(listh, par, TRUE);
			break;
		case ELIPSOID:
			num = create_elipsoid_np(listh, par);
			break;
		case SPHERIC_BAND:
			num = create_spheric_band_np(listh, par);
			break;
		default:
			handler_err(ERR_NUM_PRIMITIVE);
			break;
	}
	return num;
}

hOBJ create_primitive_obj(BREPKIND kind, sgFloat  *par)
{
	hOBJ      		hbrep     	= NULL;
	lpOBJ	    	  brep;
	lpGEO_BREP  	lpgeobrep;
	lpCSG       	csg;
	WORD        	size;
	LNP						lnp;
	D_POINT				minP, maxP;
  sgFloat				c;

	memset(&lnp,0,sizeof(lnp));
	lnp.kind = kind;
	switch (kind) {
		case BOX:
			if ( !(lnp.num_np = create_box_np(&lnp.listh, par))) return NULL;
//			lnp.num_np = 0;
			size = sizeof(sgFloat)*3;
			minP.x = minP.y = minP.z = 0.;
			maxP.x = par[0];	maxP.y = par[1];	maxP.z = par[2];
			break;
		case SPHERE:
			if ( !(lnp.num_np = create_sph_np(&lnp.listh, par))) return NULL;
			size = sizeof(sgFloat)*3;
			minP.x = minP.y = minP.z = - par[0];
			maxP.x = maxP.y = maxP.z = 	par[0];
			break;
		case CYL:
			if ( !(lnp.num_np = create_cyl_np(&lnp.listh, par, FALSE))) return NULL;
			size = sizeof(sgFloat)*3;
			minP.x = minP.y = - par[0];	minP.z = 0.;
			maxP.x = maxP.y = 	par[0];	maxP.z = par[1];
			break;
		case CONE:
			if ( !(lnp.num_np = create_cone_np(&lnp.listh, par, FALSE))) return NULL;
			size = sizeof(sgFloat)*4;
			minP.x = minP.y = - max(par[0],par[1]);		minP.z = 0;
			maxP.x = maxP.y = max(par[0],par[1]);			maxP.z = par[2];
			break;
		case TOR:
			if ( !(lnp.num_np = create_tor_np(&lnp.listh, par))) return NULL;
			size = sizeof(sgFloat)*4;
			minP.x = minP.y = - (par[0] + par[1]);		minP.z = - par[1];
			maxP.x = maxP.y = 		par[0] + par[1];		maxP.z = par[1];
			break;
		case PRISM:
			if ( !(lnp.num_np = create_cyl_np(&lnp.listh, par, TRUE))) return NULL;
			size = sizeof(sgFloat)*3;
			minP.x = minP.y = - par[0];	minP.z = 0.;
			maxP.x = maxP.y = 	par[0];	maxP.z = par[1];
			break;
		case PYRAMID:
			if ( !(lnp.num_np = create_cone_np(&lnp.listh, par, TRUE))) return NULL;
			size = sizeof(sgFloat)*4;
			minP.x = minP.y = - max(par[0],par[1]);		minP.z = 0;
			maxP.x = maxP.y = max(par[0],par[1]);			maxP.z = par[2];
			break;
		case ELIPSOID:
			if ( !(lnp.num_np = create_elipsoid_np(&lnp.listh, par))) return NULL;
			size = sizeof(sgFloat)*5;
			minP.x = - par[0];
			maxP.x = 	par[0];
			minP.y = - par[1];
			maxP.y = 	par[1];
			minP.z = - par[2];
			maxP.z = 	par[2];
			break;
		case SPHERIC_BAND:
			if ( !(lnp.num_np = create_spheric_band_np(&lnp.listh, par))) return NULL;
			size = sizeof(sgFloat)*5;
			if (par[1] * par[2] <= eps_n) c = par[0];
			else {
				c = min(fabs(par[1]), fabs(par[2]));
				c = par[0] * sqrt(1 - c * c);
			}
			minP.x = - c;
			maxP.x = 	c;
			minP.y = - c;
			maxP.y = 	c;
			minP.z = par[0] * par[1];
			maxP.z = par[0] * par[2];
			break;

		default:
			return FALSE;
	}

//  
	if ( (lnp.hcsg = SGMalloc(size+2) ) == NULL ) goto err;
	csg = (lpCSG)lnp.hcsg;
	csg->size = size;
	memcpy(csg->data,par,size);

	if ( ( hbrep = o_alloc(OBREP)) == NULL)	goto err;
	brep = (lpOBJ)hbrep;

	lpgeobrep = (lpGEO_BREP)brep->geo_data;
	o_hcunit(lpgeobrep->matr);
	lpgeobrep->min      = minP;
	lpgeobrep->max      = maxP;
	lpgeobrep->type 		= BODY;
	lpgeobrep->ident_np = -32767 + lnp.num_np;
	if ( !put_np_brep(&lnp,&lpgeobrep->num) ) goto err;

	return hbrep;
err:
  free_item_list(&lnp.listh);
	if ( lnp.hcsg ) 	SGFree(lnp.hcsg);
	if ( hbrep ) 	o_free(hbrep,NULL);
	return NULL;
}
