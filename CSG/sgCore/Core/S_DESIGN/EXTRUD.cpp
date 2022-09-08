#include "../sg.h"

static BOOL create_extruded_body( lpMESHDD g, lpVDIM vdim, lpMATR matr );
static BOOL create_extruded_sides( lpMESHDD g, lpD_POINT v,
																	 lpMATR matr, short constr );

#pragma argsused
BOOL extrud_mesh(lpLISTH listh_path, short type,
								 lpMATR matr, BOOL bottoms, hOBJ *hrez ){
	BOOL				cod;
	BOOL        closed;
	short				np_num, i;
	hOBJ        hobject;
	VDIM				list_vdim, add_vdim;
	lpVDIM			vdim;
	MESHDD      mdd;
	lpNPW       np = NULL;
	NP_STR			str;

	c_num_np	= -32767;
	*hrez			= NULL;

	if( !np_init_list(&c_list_str) ) return FALSE;

// VDIM      
	if( !init_vdim(&list_vdim, sizeof(VDIM))) return FALSE;

//      
	hobject = listh_path->hhead;   //  
	while( hobject != NULL ){      //     
		if( !create_contour_appr(&list_vdim, hobject, &closed)) goto err;
		get_next_item_z( SEL_LIST, hobject, &hobject ); // c  
	}


	if( bottoms&&closed ){

//   
		if ((np = creat_np_mem(TNPW,MAXNOV,MAXNOE,MAXNOC, MAXNOF,
														MAXNOE)) == NULL)  goto err;
		np_init((lpNP)np);
		np->ident = c_num_np++;

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

			np_num=c_list_str.number_all;
			for(i=0; i<np_num; i++){
				read_elem(&c_list_str.vdim, i, &str );
				if( !read_np(&c_list_str.bnp, str.hnp, np) ) goto err2;
				np->ident = c_num_np++;
				multiply_np( np, matr );
				if( !np_cplane( np ) ) goto err2;
				if( !np_del_sv(&c_list_str, &c_num_np, np) ) goto err2;
			}

//     
			if( !add_new_points(&list_vdim, &add_vdim )) goto err2;
			free_vdim( &add_vdim );
		}else{ // 
//first bottom
			if( !np_cplane( np ) ) goto err1;
			if( !np_put( np, &c_list_str ) ) goto err1;
//second bottom
			np->ident = c_num_np++;
			multiply_np( np, matr );
			if( !np_cplane( np ) ) goto err1;
			if( !np_put( np, &c_list_str ) ) goto err1;
		}
		free_np_mem(&np);
	}
//--------------------------------------------------------------
	if (!begin_rw(&list_vdim, 0)) goto err11;
	for (i = 0; i < list_vdim.num_elem; i++) {//    
		if( !init_vdim( &mdd.vdim, sizeof(MNODE) )) goto err11;
		if ((vdim = (lpVDIM)get_next_elem(&list_vdim)) == NULL) goto err11;
//   
		if( create_extruded_body( &mdd, vdim, matr ) != TRUE ) goto err12;
		if (!(np_mesh4p(&c_list_str,&c_num_np,&mdd,0.))) goto err12;
		free_vdim(&mdd.vdim);
	}
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
//-----------------------------------------------------------
err12:
	free_vdim(&mdd.vdim);
err11:
	free_list_vdim(&list_vdim);
	np_end_of_put(&c_list_str,NP_CANCEL,0,NULL);
	return FALSE;
//-----------------------------------------------------------
}

static BOOL create_extruded_body( lpMESHDD g, lpVDIM vdim, lpMATR matr ){
	BOOL				rt=FALSE;
	short       i, mask;
	lpMNODE			node;

	g->n = 2;
	g->m = (short)vdim->num_elem;

	if (!begin_rw(vdim, 0)) return FALSE;
	for (i = 0; i < vdim->num_elem; i++) {//    
		if ((node = (lpMNODE)get_next_elem(vdim)) == NULL) goto err;

//
		mask = 0;
		if (!(node->num & AP_CONSTR)) mask = (short)CONSTR_U;
		else if (node->num & AP_SOPR || node->num & AP_COND) mask = COND_U;

		if (!create_extruded_sides(g, &node->p, matr, mask)) goto err;
	}
	rt=TRUE;
err:
	end_rw(vdim);
	return rt;
}

static BOOL create_extruded_sides(  lpMESHDD g, lpD_POINT v,
																		lpMATR matr, short mask ){
	MNODE     node;
	D_POINT   vp;

//    
	node.p.x = v->x;
	node.p.y = v->y;
	node.p.z = v->z;
	node.num = mask | CONSTR_V;
	if( !add_elem( &g->vdim, &node ) ) return FALSE;

//    
	o_hcncrd( matr, v, &vp);
	node.p.x = vp.x;
	node.p.y = vp.y;
	node.p.z = vp.z;
	if( !add_elem( &g->vdim, &node ) ) return FALSE;
	return TRUE;
}


