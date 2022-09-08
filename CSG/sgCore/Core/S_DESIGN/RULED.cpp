#include "../sg.h"
static BOOL Bild_Ruled_Surf( lpSURF_DAT surf_dat, lpVDIM ribs, BOOL closed );
/**********************************************************
*  Curve creating - rulled surface
*  list				 - list of hOBJ, containing curves for ruled surface
*	 spin				 - spin line for creating surface
*  closed    	 - =TRUE - if curves are closed
*  solid     	 - =TRUE - closer for solid
*  p         	 - degree of first curves
*  q         	 - degree of second curve
*/
BOOL Ruled_Surface( lpLISTH list, lpD_POINT spin, short q, short p, BOOL closed,
										BOOL solid,	hOBJ *hrez){
	BOOL				cod;
	short    		degree;
	MESHDD      mdd;
	VDIM				ribs, rib_curves;
	lpNPW       np = NULL;
	SURF_DAT	  surf_dat;
  GEO_SURFACE	geo_surf;
//==============================================================================

	o_hcunit(surf_matr); //   
	c_num_np	= -32767;
	*hrez			= NULL;
	if( !np_init_list(&c_list_str) ) goto err;

//get curves from the list and put them into VDIM(sply_dat)
	if( !Create_Ribs( list, &rib_curves ) ) goto err;

//elevate the degree of lower order if it's posible
	degree = ( p >= q ) ? p : q;
	cod=Elevete_Rib_Degree( &rib_curves, degree, &ribs );

	free_vdim_hobj( &rib_curves );
	if( cod!=TRUE ) return FALSE;

//reorientation if it's nesessary
	if( !Reorient_Ribs( list, &ribs, spin )) goto err0;

//initialization for surface structure
	if( !init_surf_dat( SURF_NEW, degree, 1, SURF_APPR, &surf_dat )) goto err0;
	surf_dat.sdtype = (SURF_TYPE_IN)(surf_dat.sdtype | SURF_RULED);

//merge control vectors
	if( !Merge_Control_Vectors( &ribs )) goto err1;

//create ruled surface
	if( !Bild_Ruled_Surf( &surf_dat, &ribs, closed ) ) goto err1;

//create GEO_SURFACE
	if( !create_geo_surf(&surf_dat, &geo_surf)) goto err1;
//==============================================================================
//  BREP
	init_vdim(&mdd.vdim,sizeof(MNODE));
	if( !Bild_Mesh_On_Surf(&surf_dat, &mdd, 3, 3)) goto err2;


	//==============================================================================
//  
	
	if(closed && solid )
	{
		if ( (np = creat_np_mem(TNPW,MAXNOV,MAXNOE,MAXNOC, MAXNOF,
			MAXNOE)) == NULL)  goto err2;
		np_init((lpNP)np);
		np->ident = c_num_np++;
		// 
		if( !put_top_bottom( np, &mdd )) goto err3;
		if( np->nov > MAXNOV ||	np->noe > MAXNOE ){
			if( !sub_division( np ) ) goto err3;
		} else {
			if( !np_cplane( np ) ) goto err3;
			if( !np_put( np, &c_list_str ) ) goto err3;
		}
		
		//   
		np->ident = c_num_np++;
		put_bottom_bottom_skin( np, &mdd );
		if( np->nov > MAXNOV ||	np->noe > MAXNOE ){
			if( !sub_division( np ) ) goto err3;
		}	else {
			if( !np_cplane( np ) ) goto err3;
			if( !np_put( np, &c_list_str ) ) goto err3;
		}
		free_np_mem(&np);
	}
//------------------------------------------------------------------------------
  if( ((*hrez)=create_BREP_surface(&geo_surf, &mdd ,false) ) == NULL ) goto err3;
  free_geo_surf( &geo_surf );
//------------------------------------------------------------------------------
  free_vdim(&mdd.vdim);
  free_surf_data(&surf_dat);
  free_vdim_sply(&ribs);

  return TRUE;
//==============================================================================
err3:
 free_np_mem(&np);
err2:
	free_vdim(&mdd.vdim);
  free_geo_surf( &geo_surf );
err1:
	free_surf_data(&surf_dat);
err0:
	free_vdim_sply(&ribs);
err:
    np_end_of_put(&c_list_str,NP_CANCEL,0,NULL);
	return FALSE;
}

/**********************************************************
* Create data for ruled surface
*/
static BOOL Bild_Ruled_Surf( lpSURF_DAT surf_dat, lpVDIM ribs, BOOL closed ){

//put ribs into surface description
	if( !put_surf_ribs( surf_dat, SURF_RULED, ribs, closed )) return FALSE;

	surf_dat->V[0]=surf_dat->V[1]=0.;
	surf_dat->V[2]=surf_dat->V[3]=1.;
	surf_dat->v[0]=0.;	surf_dat->v[1]=1.;
	surf_dat->num_u=surf_dat->P.n+surf_dat->degree_p+1;
	surf_dat->num_v=4;
	return TRUE;
}


