#include "../sg.h"
static BOOL Bild_Skinning_Surf( lpSURF_DAT surf, lpVDIM ribs, BOOL closed_u, BOOL closed_v );
static void clean_spline( lpSPLY_DAT spline);
/**********************************************************
*  Cross-section design - NURBS skining
*  list  		  - list of hOBJ, containing cross sections for skinning surface
*	 spin				 - spin line for creating surface
*  closed_u   - =TRUE - if all(!!!!) cross-sections are closed
*	 closed_v		- =TRUE - if spine curve is closed
*  solid      - =TRUE - closer for solid
*  n_section - number of points on spine curve
*  p         - degree of cross-section curves
*  q         - degree of spin curve
*/
#pragma argsused
BOOL Skin_Surface( lpLISTH list, lpD_POINT spin, short n_section, short q, short p,
									 BOOL closed_u, BOOL closed_v, BOOL solid, hOBJ *hrez){
	BOOL				cod;
	MESHDD      mdd;
	VDIM				ribs, rib_curves;
	lpNPW       np = NULL;
	SURF_DAT	  surf_dat;
  GEO_SURFACE	geo_surf;
//==============================================================================

	o_hcunit(surf_matr); //   
	c_num_np	= -32767;
	*hrez			= NULL;
	if ( !np_init_list(&c_list_str) ) return FALSE;

//get curves from the list and put them into VDIM(sply_dat)
	if( !Create_Ribs( list, &rib_curves )) return FALSE;

//elevate the degree of lower order if it's posible
	cod=Elevete_Rib_Degree( &rib_curves, p, &ribs );

	free_vdim_hobj( &rib_curves );
	if( cod!=TRUE ) return FALSE;

//reorientation if it's nesessary
	if( !Reorient_Ribs( list, &ribs, spin )) goto err0;

//initializatuion for surface structure
	if( !init_surf_dat( SURF_NEW, p, q, SURF_APPR, &surf_dat )) goto err0;
	surf_dat.sdtype = (SURF_TYPE_IN)(surf_dat.sdtype | SURF_SKIN);

  //merge control vectors
	if( !Merge_Control_Vectors( &ribs )) goto err1;

//create skinning surface
	if( !Bild_Skinning_Surf( &surf_dat, &ribs, closed_u, closed_v ) ) goto err1;

//create GEO_SURFACE
 	if( !create_geo_surf( &surf_dat, &geo_surf)) goto err1;

//==============================================================================
//  BREP
	init_vdim(&mdd.vdim,sizeof(MNODE));
	if( !Bild_Mesh_On_Surf(&surf_dat, &mdd, 3, 3)) goto err2;
//==============================================================================
//create bottoms
	if( closed_u && solid && !closed_v ){
		if ( (np = creat_np_mem(TNPW,MAXNOV,MAXNOE,MAXNOC, MAXNOF,
														MAXNOE)) == NULL)  goto err2;
		np_init((lpNP)np);
		np->ident = c_num_np++;
		if( !put_top_bottom( np, &mdd )) goto err3;
		if( np->nov > MAXNOV ||	np->noe > MAXNOE ){
			if( !sub_division( np ) ) goto err3;
		} else {
			if( !np_cplane( np ) ) goto err3;
			if( !np_put( np, &c_list_str ) ) goto err3;
		}
//create bottom bottom  
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
  if( ((*hrez)=create_BREP_surface(&geo_surf, &mdd,false )) == NULL ) goto err2;
  
 	free_geo_surf( &geo_surf );
	free_vdim_sply(&ribs);
	free_surf_data(&surf_dat);
	free_vdim(&mdd.vdim);
	return cod;
err3:
	free_np_mem(&np);
err2:
	free_vdim(&mdd.vdim);
 	free_geo_surf( &geo_surf );
err1:
	free_surf_data(&surf_dat);
err0:
	free_vdim_sply(&ribs);
	return FALSE;
}

/**********************************************************
*  bild NURBS skining
*/
#pragma argsused
static BOOL Bild_Skinning_Surf( lpSURF_DAT surf, lpVDIM ribs, BOOL closed_u, BOOL closed_v ){
	short     i, j, condition;
  sgFloat 		w;
	SPLY_DAT  spline;
  W_NODE		node;

//put ribs into surface description
	if( !put_surf_ribs( surf, SURF_SKIN, ribs, closed_u )) return FALSE;

// Create empty work spline for v-direction
//	if( !init_sply_dat( SPL_NEW, surf->degree_q, SPL_APPR, &spline ) )	return FALSE;
	if( !init_sply_dat( SPL_NEW, surf->degree_q, SPL_INT, &spline ) )	return FALSE;

// Bild Nurbs curves interpolating given points P_x[i]
// This is v-direction
	if( surf->degree_q == 1 ){  // approximation is out of need - surface is bild
		surf->V[0]=surf->V[1]=0.;		surf->V[2]=surf->V[3]=1.;
		surf->v[0]=0.;	            surf->v[1]=1.;

// set closer by V
		if( closed_v ) surf->sdtype = (SURF_TYPE_IN)(surf->sdtype | SURF_CLOSEV);
		surf->num_u=surf->P.n;
		surf->num_v=2;
	}else{// Compute values for kont vector V - for spin curve !
// and calculate P-points for all surface
/*		if( closed_v ){ spline.sdtype |= SPL_CLOSE;
										condition=1; // closed without derivation
										surf->P.n = surf->knots.m+2;
		}else{					condition=0; // without derivation
										surf->P.n = surf->knots.m;}
//add "free" points
		if( closed_v ){
			node.p.x=node.p.y=node.p.z=0;
			for( i=0; i<2*surf->P.m; i++ )
			if( !add_elem(&surf->P.vdim, &node) ) goto err;
		}
*/
		condition=0; // free spline
//		surf->P.n = surf->knots.m;

//  set closer by V
		if( closed_v ) surf->sdtype = (SURF_TYPE_IN)(surf->sdtype | SURF_CLOSEV);

//Calculate spline parameters for spine curve
		for( i=0; i<surf->P.n; i++ ){
			clean_spline( &spline );
			for(j=0; j<surf->P.m; j++ ){
				read_elem( &surf->P.vdim, surf->P.n*j+i, &node );
 	      Create_3D_From_4D( &node, (sgFloat *)&spline.knots[j], &w);
				spline.numk++;
			}
			if( i==0 ){
				parameter( &spline, condition, TRUE );   //calculate parameters
//put calculated parameters into surface structure
				for( j=0; j<surf->P.m; j++ )	surf->v[j] = spline.u[j];
        surf->num_v=surf->P.m+surf->degree_q+1;
				for( j=0; j<surf->num_v; j++ ) surf->V[j] = spline.U[j];
			}
			calculate_P( &spline, condition ); //calculate short spline
//write calculated P-points for all surface
			for( j=0; j<surf->P.m; j++ )
      	write_elem( &surf->P.vdim, surf->P.n*j+i, &spline.P[j] );
    }
	}
	free_sply_dat( &spline );
	return TRUE;
/*err:
	free_sply_dat( &spline );
	return FALSE;
*/
}

static void clean_spline( lpSPLY_DAT spline){
	spline->numk=0; // Number of node points
	spline->numd=0; // Number of node points with derivates
	spline->nump=0; // Number of P-points
}
