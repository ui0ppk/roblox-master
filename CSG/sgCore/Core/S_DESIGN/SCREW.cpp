#include "../sg.h"

static BOOL create_screw( lpMESHDD g, hOBJ hpath,
									 lpD_POINT f_axis, lpD_POINT s_axis,
									 sgFloat H_v, sgFloat H, short n_s, BOOL *closed );
static BOOL create_screw_sides( lpMESHDD g, lpD_POINT v, MATR m_i,
												 sgFloat d_h, sgFloat d_f, short k_direct,
												 short mask, sgFloat n_cond );

BOOL screw_mesh(lpLISTH listh_path, lpD_POINT f, lpD_POINT s,
								 sgFloat step, sgFloat height, short num_v, BOOL bottoms, hOBJ *hrez)
{
	BOOL        closed;
	MATR        m_all;
	hOBJ        hobject;
	MESHDD      mdd;
	lpNPW       np = NULL;
	BOOL		cod;//nb  = FALSE;

	c_num_np = -32767;
	*hrez = NULL;

	if ( !np_init_list(&c_list_str) ) {
		free_vdim(&mdd.vdim);
		return FALSE;
	}

//   
	if ( (np = creat_np_mem(TNPW,MAXNOV,MAXNOE,MAXNOC, MAXNOF,
													MAXNOE)) == NULL)  goto err1;
	np_init((lpNP)np);
	np->ident = c_num_np++;

	hobject = listh_path->hhead;  //  
	while( hobject != NULL )      //     
	{
		o_hcunit( m_all );            // 
//      
		if( !create_screw(&mdd, hobject, f, s, step, height, num_v, &closed) ) goto err1;
//    -   
		if( closed ) if( !put_top1_bottom( np, &mdd ) ) goto err1;

		get_next_item_z( SEL_LIST, hobject, &hobject ); // c  

		if (!(np_mesh3p(&c_list_str,&c_num_np,&mdd,0.))) goto err1;
		free_vdim(&mdd.vdim);
	}

	if( closed && bottoms )
	{
		if( np->nov > MAXNOV ||	np->noe > MAXNOE )
		{
			if( !sub_division( np ) ) goto err1;
		}
		else
		{
			if( !np_cplane( np ) ) goto err1;
			if( !np_put( np, &c_list_str ) ) goto err1;
		}
//    
		np->ident = c_num_np++;
		hobject = listh_path->hhead;   //  
		if( !put_bottom_bottom( np, hobject ) ) goto err1;
		if( np->nov > MAXNOV ||	np->noe > MAXNOE )
		{
			if( !sub_division( np ) ) goto err1;
		}
		else
		{
			if( !np_cplane( np ) ) goto err1;
			if( !np_put( np, &c_list_str ) ) goto err1;
		}
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

static BOOL create_screw( lpMESHDD g, hOBJ hpath,
									 lpD_POINT f_axis, lpD_POINT s_axis,
									 sgFloat H_v, sgFloat H, short n_s, BOOL *closed  )
{
	short         i, n_all, mask;
	sgFloat      d_f, d_h, n_cond;
	MATR        m_all, m_local_v, m_local_i;
	D_POINT     p;
	VDIM				vdim;
	MNODE				node;
	OSTATUS     status;
	lpMNODE			lpnode;

	d_f   = 2*M_PI/n_s;       //   
	d_h   = H_v/n_s;          //   

	n_all = (short)( H/H_v * n_s + 0.5) + 1;   // -   
	if (n_all <= 1) n_all = 2;
	n_cond = n_s / 4.;		//    

	if (!apr_path(&vdim,hpath)) return FALSE;
	if (!get_status_path(hpath, &status)) return FALSE;
	*closed = (status & ST_CLOSE) ? TRUE : FALSE;
	o_hcunit(m_all);

	init_vdim(&g->vdim,sizeof(MNODE));

	o_hcunit( m_local_v );                     // 
	o_tdtran( m_local_v, dpoint_inv(f_axis, &p) ); //    ..
//      
	o_hcrot1( m_local_v, dpoint_sub( s_axis, f_axis, &p ) );
	o_minv  ( m_local_v, m_local_i );          //  

	o_hcmult(m_all, m_local_v);

/*	if ( vdim.num_elem > MAXINT ) {
		cinema_handler_err(CIN_OVERFLOW_MDD);
		goto err;
	}
*/	g->n = n_all;
	g->m = (short)vdim.num_elem;

	if (!begin_rw(&vdim, 0)) goto err;
	for (i = 0; i < vdim.num_elem; i++) {//    
		if ( (lpnode = (lpMNODE)get_next_elem(&vdim)) == NULL) goto err1;
		node = *lpnode;
//     
		if (! (o_hcncrd( m_all, &node.p, &node.p )) ) goto err1;
//      (   )
//		if (! (o_hcncrd( m_local_v, &node.p, &node.p )) ) goto err1;
//    -   c  
		mask = 0;
		if (!(node.num & AP_CONSTR)) mask = (short) CONSTR_U;
		else if (node.num & AP_SOPR || node.num & AP_COND) mask = COND_U;
		if(  create_screw_sides( g, &node.p, m_local_i, d_h, d_f,
														 n_all, mask, n_cond ) != TRUE ) goto err1;
	}
	end_rw(&vdim);
	free_vdim(&vdim);
	return TRUE;
err1:
	end_rw(&vdim);
err:
	free_vdim(&g->vdim);
	free_vdim(&vdim);
	return FALSE;
}

static BOOL create_screw_sides( lpMESHDD g, lpD_POINT v, MATR m_i,
												 sgFloat d_h, sgFloat d_f, short k_direct,
												 short mask, sgFloat n_cond )
{
	short       it;
	sgFloat    dd_f, dd_h, R, k_cond;
	MNODE     node;

	R = sqrt( v->x*v->x + v->y*v->y );

	if( v->y >= 0 ) dd_f = acos(v->x/R);
	else            dd_f = 2*M_PI - acos(v->x/R);

/*--------------------------------------------------------*/
//   
	for( dd_h = v->z, k_cond=0., it=0; it<k_direct; it++, dd_f += d_f, dd_h += d_h )
	{
// pp   i- 
		v->x = R*cos(dd_f);
		v->y = R*sin(dd_f);
		v->z = dd_h;
//      (   )
		if (! (o_hcncrd( m_i, v, &node.p )) ) return FALSE;
		node.num = mask;
		if( !(it % (k_direct-1)) ) node.num |= CONSTR_V;
		if( it == (short)(k_cond+0.5) ){
			node.num |= COND_V;
			k_cond += n_cond;
		}
		if( !add_elem( &g->vdim, &node ) )       return FALSE;
	}
	return TRUE;
}
