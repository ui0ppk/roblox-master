#include "../sg.h"

static BOOL create_casted_body( lpMESHDD g, hOBJ hpath, sgFloat H,
																sgFloat alfa );
static BOOL create_casted_sides( lpMESHDD g, lpD_POINT previous,
																 lpD_POINT current, lpD_POINT next,
																 sgFloat H, sgFloat alfa, short mask );
static BOOL calculate_casted_bottom_point( lpD_POINT previous,
												lpD_POINT current, lpD_POINT next,
												sgFloat H, sgFloat alfa, lpD_POINT v4 );
static sgFloat sq( lpD_POINT v1, lpD_POINT v2, lpD_POINT v3 );
static BOOL check_top_bottom_cast( hOBJ hobject, sgFloat H, sgFloat alfa );
extern short orient;

BOOL casted_mesh( lpLISTH list_path, sgFloat H, sgFloat alfa, hOBJ *hrez){
	MESHDD      mdd;
	lpNPW       np = NULL;
	BOOL				cod;
	hOBJ        hobject;

	c_num_np	= -32767;
	*hrez			= NULL;
	if ( !np_init_list(&c_list_str) ) return FALSE;
//--------------------------------------------------------------
	hobject = list_path->hhead;   //  
//    
	if( check_top_bottom_cast( hobject, H, alfa ) != TRUE ){
//       
		return FALSE;
	}
//--------------------------------------------------------------
//   
	if ((np = creat_np_mem(TNPW,MAXNOV,MAXNOE,MAXNOC, MAXNOF,
													MAXNOE)) == NULL)  goto err1;
	np_init((lpNP)np);

	hobject = list_path->hhead;   //  
	while( hobject != NULL ){      //     
//      
		if( create_casted_body( &mdd, hobject, H, alfa ) != TRUE ) goto err1;
//  
		if( !put_top1_bottom(np, &mdd ) ) goto err1;

		get_next_item_z( SEL_LIST, hobject, &hobject ); // c  

		if (!(np_mesh3p(&c_list_str,&c_num_np,&mdd,0.))) goto err1;
		free_vdim(&mdd.vdim);
	}

//-----------------------------------------------------------
	np->ident = c_num_np++;
	if( np->nov > MAXNOV ||	np->noe > MAXNOE ){
		if( !sub_division( np ) ) goto err1;
	}else{
			if( !np_cplane( np ) ) goto err1;
			if( !np_put( np, &c_list_str ) ) goto err1;
	}
//    
	np_init((lpNP)np);
	np->ident = c_num_np++;
	hobject = list_path->hhead;   //  
	if( !put_bottom_bottom( np, hobject ) ) goto err1;
	if( np->nov > MAXNOV ||	np->noe > MAXNOE ){
		if( !sub_division( np ) ) goto err1;
	}else{
		if( !np_cplane( np ) ) goto err1;
		if( !np_put( np, &c_list_str )) goto err1;
	}
	free_np_mem(&np);

	cod = cinema_end(&c_list_str,hrez);
	return cod;
err1:
	free_np_mem(&np);
	free_vdim(&mdd.vdim);
	np_end_of_put(&c_list_str,NP_CANCEL,0,NULL);
	return FALSE;
}

//***********************************************************/
static BOOL create_casted_body( lpMESHDD g, hOBJ hpath, sgFloat H,
																sgFloat alfa ){
	int			i, mask;
	VDIM		vdim;
	lpMNODE	lpnode;
	MNODE		current, previous, next, first;

	if (!apr_path(&vdim,hpath)) return FALSE;

	init_vdim(&g->vdim,sizeof(MNODE));

	if ( vdim.num_elem > MAXINT ) {
		cinema_handler_err(CIN_OVERFLOW_MDD);
		goto err;
	}

	g->n = 2;
	g->m = (short)vdim.num_elem;
	read_elem( &vdim, g->m-2, &previous );
	if (!begin_rw(&vdim, 0)) goto err;
	if ((lpnode = (lpMNODE)get_next_elem(&vdim)) == NULL) goto err1;
	current = *lpnode;
	first = current;
	for (i = 1; i < vdim.num_elem; i++) {//    
		if( i == vdim.num_elem-1 ) next = first;
		else {
			if ((lpnode = (lpMNODE)get_next_elem(&vdim)) == NULL) goto err1;
			next = *lpnode;
		}

//  
		if( i==1 )
		 orient = ( sq( &previous.p, &current.p, &next.p) < 0 ) ? (-1):(1);

//
		mask = 0;
		if (!(current.num & AP_CONSTR)) mask = CONSTR_U;
		else if (current.num & AP_SOPR || current.num & AP_COND) mask = COND_U;

    
		if (!create_casted_sides(g, &previous.p, &current.p, &next.p,
														 H, alfa, mask)) goto err1;
		previous = current;
		current = next;
	}
	end_rw(&vdim);
	read_elem( &g->vdim, 0, &previous );
	read_elem( &g->vdim, 1, &next);
	if( !add_elem( &g->vdim, &previous ) )     return FALSE;
	if( !add_elem( &g->vdim, &next ) )     return FALSE;
	free_vdim(&vdim);
	return TRUE;
err1:
	end_rw(&vdim);
err:
	free_vdim(&g->vdim);
	free_vdim(&vdim);
	return FALSE;
}

static BOOL create_casted_sides( lpMESHDD g, lpD_POINT previous,
																 lpD_POINT current, lpD_POINT next,
																 sgFloat H, sgFloat alfa, short mask ){
	MNODE     node;
	D_POINT   vp;
//    
	node.p.x = current->x;
	node.p.y = current->y;
	node.p.z = current->z;
	node.num = mask | CONSTR_V;
	if( !add_elem( &g->vdim, &node ) ) return FALSE;
//    
	if( !calculate_casted_bottom_point( previous, current, next,
																			H, alfa, &vp ) ) return FALSE;
	node.p.x = vp.x;
	node.p.y = vp.y;
	node.p.z = vp.z;
	if( !add_elem( &g->vdim, &node ) ) return FALSE;

	return TRUE;
}

static BOOL calculate_casted_bottom_point( lpD_POINT previous,
												lpD_POINT current, lpD_POINT next, sgFloat H,
												sgFloat alfa, lpD_POINT v4 ){
	sgFloat distance, fi, a;
	D_POINT v1, v2, direct;

//      
	dpoint_sub(previous, current, &v1);
	if( !dnormal_vector ( &v1 )) return FALSE;

	dpoint_sub(next, current, &v2);
	if( !dnormal_vector ( &v2 )) return FALSE;

	dpoint_add( &v1, &v2, &direct);
	if( orient*sq(&v1, &v2, &direct) < 0)	dpoint_inv( &direct, &direct);

	a = sqrt( direct.x*direct.x + direct.y*direct.y + direct.z*direct.z );
	fi = acos( a/2 );

//  ,      current  
	distance = ((H*tan(alfa))/sin(fi))/a;
	v4->x = current->x + direct.x*distance;
	v4->y = current->y + direct.y*distance;
	v4->z = current->z + direct.z*distance + H;
	return TRUE;
}

/**********************************************************
*/
static sgFloat sq( lpD_POINT v1, lpD_POINT v2, lpD_POINT v3 ){
	return( ( v2->y - v1->y )*( v2->z - v3->z ) +
					( v2->x - v1->x )*( v2->y - v3->y ) +
					( v2->z - v1->z )*( v2->x - v3->x ) -
					( v2->y - v1->y )*( v2->x - v3->x ) -
					( v2->z - v1->z )*( v2->y - v3->y ) -
					( v2->x - v1->x )*( v2->z - v3->z ) );
}

static BOOL  check_top_bottom_cast( hOBJ hobject, sgFloat H, sgFloat alfa ){
//	lpVDIM		vdim=NULL;

 /*	while( hobject != NULL ){      //     
		init_vdim(vdim,sizeof(MNODE));
		if (!apr_path( vdim, hobject )) goto err1;
//  
//		if( create_top_bottom_cast( hobject, H, alfa ) != TRUE ) return FALSE;
		get_next_item_z( SEL_LIST, hobject, &hobject ); // c  
		free_vdim(vdim);
	}*/
//	return TRUE;
//err:
//	end_rw(vdim);
//err1:
//	free_vdim(vdim);
	return FALSE;
}
