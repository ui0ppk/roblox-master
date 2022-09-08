#include "../sg.h"


BOOL faced_mesh(lpLISTH listh_path, hOBJ *hrez ){
	BOOL				cod;
	BOOL        closed;
	hOBJ        hobject;
	lpNPW       np = NULL;
	VDIM				list_vdim, add_vdim;

	c_num_np	= -32767;
	*hrez			= NULL;

	if ( !np_init_list(&c_list_str) )	return FALSE;

// VDIM      
	if( !init_vdim(&list_vdim, sizeof(VDIM))) return FALSE;

//      
	hobject = listh_path->hhead;   //  
	while( hobject != NULL ){      //     
		if( !create_contour_appr(&list_vdim, hobject, &closed)) goto err;
		get_next_item_z( SEL_LIST, hobject, &hobject ); // c  
	}

//   
	if ( (np = creat_np_mem(TNPW,MAXNOV,MAXNOE,MAXNOC, MAXNOF,
													MAXNOE)) == NULL)  goto err1;
	np_init((lpNP)np);
	np->ident= c_num_np++;

//       
	if( !put_bottom( np, &list_vdim ) ) goto err1;

	if( np->nov > MAXNOV ||	np->noe > MAXNOE ){// -
//VDIM   
		if( !init_vdim(&add_vdim, sizeof( POINT3 ))) goto err1;
// 
		if( !np_cplane( np ) ) goto err1;
		if( !sub_division_face( np, &add_vdim ) ) goto err2;

//  NP  LIST
		if( !np_del_sv(&c_list_str, &c_num_np, np) ) goto err2;

		if ( add_vdim.num_elem ) {	
		//     
			if( !add_new_points(&list_vdim, &add_vdim )) goto err2;
			free_vdim( &add_vdim );
    }
	}else{ // 
//first bottom
		if( !np_cplane( np ) ) goto err1;
		if( !np_put( np, &c_list_str ) ) goto err1;
	}

	free_np_mem(&np);
	end_rw(&list_vdim);
	free_list_vdim(&list_vdim);
	cod = cinema_end(&c_list_str,hrez);
	return cod;
//-----------------------------------------------------------
err2:
	free_vdim(&add_vdim);
err1:
	free_np_mem(&np);
err:
	free_list_vdim(&list_vdim);
	np_end_of_put(&c_list_str,NP_CANCEL,0,NULL);
	return FALSE;
}

