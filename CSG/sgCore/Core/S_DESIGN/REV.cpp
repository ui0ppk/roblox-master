#include "../sg.h"
static  BOOL create_revolute_body( lpMESHDD g, lpVDIM vdim, sgFloat fi);
static  BOOL rotate_point( lpMESHDD g, lpD_POINT p,
													short NUM_REV_POINTS, sgFloat, short );

BOOL rev_mesh(hOBJ hobject, sgFloat fi, BOOL zu, int	type, lpMATR matr,
							BOOL closed, hOBJ *hrez){
	BOOL					cod;
	int						np_num, i, i1;
	MESHDD      	mdd;
	VDIM					list_vdim, add_vdim;
	lpVDIM				vdim;
	lpNPW       	np = NULL;
	NP_STR				str;
	MNODE					point;
  MATR					matr1;

 	c_num_np = -32767;
	*hrez			= NULL;
	if ( !np_init_list(&c_list_str) ) return FALSE;

	// VDIM      
	if( !init_vdim(&list_vdim, sizeof(VDIM))) return FALSE;

//      
//      
//	hobject = list_vdim.hhead;   //  
//	while( hobject != NULL ){      //     
		if( !create_contour_appr(&list_vdim, hobject, &closed)) goto err;
//		get_next_item_z( SEL_LIST, hobject, &hobject ); // c  
//	}

	if (!begin_rw(&list_vdim, 0)) goto err;
	for (i = 0; i < list_vdim.num_elem; i++) {//    
		if ((vdim = (lpVDIM)get_next_elem(&list_vdim)) == NULL) goto err;
//  
		for (i1=0; i1 < vdim->num_elem; i1++){
			read_elem(vdim, i1,  &point);
			o_hcncrd( matr, &point.p, &point.p );
			write_elem(vdim, i1,  &point);
		}
	}
	end_rw(&list_vdim);

	if( zu ){
		if( type == 1 ){
//        
			cinema_handler_err( CIN_NO_CORRECT_OZ );
			goto err1;
		}

//   
		if ( (np = creat_np_mem(TNPW,MAXNOV,MAXNOE,MAXNOC, MAXNOF, MAXNOE)) == NULL)  goto err1;

		np_init((lpNP)np);
		np->ident = c_num_np++;

//       
		if( !put_bottom( np, &list_vdim ) ) goto err1;

		if( np->nov > MAXNOV ||	np->noe > MAXNOE ){ // -
//VDIM   
			if( !init_vdim(&add_vdim, sizeof( POINT3 ))) goto err1;
// 
			if( !np_cplane( np ) ) goto err1;
			if( !sub_division_face( np, &add_vdim ) ) goto err2;
//  NP  LIST
			if( !np_del_sv(&c_list_str, &c_num_np, np) ) goto err2;

//   - ,   ,
			o_hcunit(matr1);
			o_tdrot(matr1, -3, fi);
			np_num=c_list_str.number_all;
			for(i=0; i<np_num; i++){
				read_elem(&c_list_str.vdim, i, &str );
				if( !read_np(&c_list_str.bnp, str.hnp, np) ) goto err2;
				np->ident = c_num_np++;
				multiply_np( np, matr1 );
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
			o_hcunit(matr1);
			o_tdrot(matr1, -3, fi);
			np->ident = c_num_np++;
			multiply_np( np, matr1 );
			if( !np_cplane( np ) ) goto err1;
			if( !np_put( np, &c_list_str ) ) goto err1;
		}
		free_np_mem(&np);
	}
	if (!begin_rw(&list_vdim, 0)) goto err11;
	for (i = 0; i < list_vdim.num_elem; i++) {//    
		if( !init_vdim( &mdd.vdim, sizeof(MNODE) )) goto err11;
		if ((vdim = (lpVDIM)get_next_elem(&list_vdim)) == NULL) goto err11;
//   
		if( create_revolute_body( &mdd, vdim, fi ) != TRUE ) goto err12;
		if (!(np_mesh4p(&c_list_str,&c_num_np,&mdd,0.))) goto err12;
		free_vdim(&mdd.vdim);
	}
	end_rw(&list_vdim);
	free_list_vdim(&list_vdim);
//-----------------------------------------------------------
	cod = cinema_end(&c_list_str,hrez);
	return cod;

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

static  BOOL create_revolute_body( lpMESHDD g, lpVDIM vdim, sgFloat fi )
{
	short     NUM_REV_POINTS;
	int				num_elem;
	int				mask;
	lpMNODE		node;

	init_vdim(&g->vdim,sizeof(MNODE));

	NUM_REV_POINTS = ( fabs(fi) >= 2*M_PI ) ?
												( sg_m_num_meridians ) :
												((short)(fabs(fi)/(2*M_PI)*sg_m_num_meridians));

	if ( NUM_REV_POINTS == 0 ) NUM_REV_POINTS = 1;
/*	if ( vdim->num_elem > MAXINT ) {
		cinema_handler_err(CIN_OVERFLOW_MDD);
		goto err;
	}
*/
	g->n = NUM_REV_POINTS + 1;
	g->m = (short)vdim->num_elem;

	if (!begin_rw(vdim, 0)) goto err;
	for (num_elem = 0; num_elem < vdim->num_elem; num_elem++) { //    
		if ( (node = (lpMNODE)get_next_elem(vdim)) == NULL) goto err1;

		//
		mask = 0;
		if (!(node->num & AP_CONSTR)) mask = CONSTR_U;
		else if (node->num & AP_SOPR || node->num & AP_COND) mask = COND_U;

		// 
		if( !rotate_point( g, &node->p, NUM_REV_POINTS, fi, mask ) ) goto err1;
	}
	end_rw(vdim);
	return TRUE;
err1:
	end_rw(vdim);
err:
	free_vdim(&g->vdim);
	return FALSE;
}

static  BOOL rotate_point( lpMESHDD g, lpD_POINT p,
									short NUM_REV_POINTS, sgFloat fi, short mask )
{
	short     j;
	sgFloat  d_fi, f, R, z;
	MNODE   node;
	sgFloat  step_j, k_j;

	step_j = sg_m_num_meridians/4.;

	d_fi =  fi / NUM_REV_POINTS;
	z = p->z;
	R = sqrt( p->x*p->x + p->y*p->y );

	for ( j = 0, k_j=0.; j <= NUM_REV_POINTS; j++ ){
		f = ( j == NUM_REV_POINTS ) ? ( fi ) : ( d_fi*j );

		p->x = R*cos( f );
		p->y = R*sin( f );
		p->z = z;
		node.p = *p;
		node.num = mask;
		if( j == (short)(k_j+0.5) ){
			 node.num |= COND_V;
			 k_j += step_j;
		}
		if( !add_elem(&g->vdim, &node ) ) return FALSE;
	}
	return TRUE;
}

