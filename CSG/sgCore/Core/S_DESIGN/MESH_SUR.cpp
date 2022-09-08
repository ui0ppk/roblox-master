#include "../sg.h"
/*******************************************************************************
*  Create surface by mesh, degree_u x degree_v
*  mdd   - mesh of D_POINT
*/
#pragma argsused
BOOL Create_Surf_By_Mesh( lpMESHDD mdd, short degree_u, short degree_v, hOBJ *hrez){
	short 			i, j;
  D_POINT			point;
  W_NODE			node;
	SURF_DAT	  surf_dat;
  GEO_SURFACE	geo_surf;
//==============================================================================

	o_hcunit(surf_matr); //   
	c_num_np	= -32767;
	*hrez			= NULL;
	if ( !np_init_list(&c_list_str) ) return FALSE;

//initializatuion for surface structure
	if( !init_surf_dat( SURF_NEW, degree_u, degree_v, SURF_APPR, &surf_dat )) goto err0;
//	surf_dat.sdtype |= SURF_MESH;

//get curves from the mesh and put them into surface structure
	for( i=0; i<mdd->m; i++ ){
  	for( j=0; j<mdd->n; j++ ){
			if( !read_elem( &mdd->vdim, mdd->n*i+j, &point )) goto err1;
      node.vertex[0]=point.x;
      node.vertex[1]=point.y;
      node.vertex[2]=point.z;
      node.vertex[3]=1.;
      if( !add_elem( &surf_dat.P.vdim, &node)) goto err1;
    }
  }
  surf_dat.P.n=mdd->n;
  surf_dat.P.m=mdd->m;

	if( !surf_parameter( &surf_dat, 0, TRUE )) goto err1;
	calculate_surf_P( &surf_dat, 0 );

//create GEO_SURFACE
 	if( !create_geo_surf( &surf_dat, &geo_surf)) goto err1;

//==============================================================================
//  BREP
	init_vdim(&mdd->vdim,sizeof(MNODE));
	if( !Bild_Mesh_On_Surf(&surf_dat, mdd, 3, 3)) goto err2;
//==============================================================================
  if( ((*hrez)=create_BREP_surface(&geo_surf, mdd )) == NULL ) goto err3;

 	free_geo_surf( &geo_surf );
	free_surf_data(&surf_dat);
	free_vdim(&mdd->vdim);
	return TRUE;

err3:
	free_vdim(&mdd->vdim);
err2:
 	free_geo_surf( &geo_surf );
err1:
	free_surf_data(&surf_dat);
err0:
  np_end_of_put(&c_list_str,NP_CANCEL,0,NULL);
	return FALSE;
}


