#include "../sg.h"

lpNPW	c_np_top = NULL, c_np_bottom = NULL;
static BOOL write_cover( lpNPW np, lpMNODE node, AP_TYPE stat );
static BOOL form_np( lpNPW np );
//**********************************************************
BOOL c_tip,  // c_tip = TRUE -  
		 ZU;

BOOL pipe_mesh(lpLISTH listh_cross, lpVDIM vdim_path, BOOL type_path, OSTATUS status1,
							 BOOL ZU,
							 AP_TYPE bstat, AP_TYPE estat, BOOL bottoms, hOBJ *hrez)
{
	BOOL        closed;
	MATR        m_all;
	hOBJ        hobject;
	MESHDD      mdd;
	BOOL				cod;

	c_num_np	= -32767;
	*hrez 		= NULL;
	if ( !np_init_list(&c_list_str) ) return FALSE;

	if( !(status1 & ST_CLOSE) && ZU )
	{
		 cinema_handler_err( CIN_NO_CORRECT_NZ );
		 goto err1;
	}

	if (0==(c_np_top = creat_np_mem(TNPW,MAXNOV,MAXNOE,MAXNOC, MAXNOF,
																MAXNOE)))  goto err1;
	np_init((lpNP)c_np_top);
	c_np_top->ident = c_num_np++;

	if( ZU ) //  
	{
		if ( (c_np_bottom = creat_np_mem(TNPW,MAXNOV,MAXNOE,MAXNOC, MAXNOF,
																		 MAXNOE)) == NULL)  goto err1;
		np_init((lpNP)c_np_bottom);
		c_np_bottom->ident = c_num_np++;
	}

	hobject = listh_cross->hhead;  //  

	while( hobject != NULL )      //     
	{
		o_hcunit( m_all );            // 
		//      
		closed = (status1 & ST_CLOSE) ? TRUE : FALSE;
		//   
		init_vdim(&mdd.vdim,sizeof(MNODE));
		if( !create_pipe( &mdd, hobject, vdim_path, m_all, &closed, ZU,
											bstat, estat ) ) goto err1a;
		//    -   (   ZU )
		if( closed && !ZU && !(status1 & ST_CLOSE) )
			if( !put_top_bottom( c_np_top, &mdd )) goto err1a;

		get_next_item_z( SEL_LIST, hobject, &hobject ); // c  

		if ( type_path ) { //   
			if (!(np_mesh4p(&c_list_str,&c_num_np,&mdd,0.))) goto err1a;
		} else {           //    
			if (!(np_mesh3p(&c_list_str,&c_num_np,&mdd,0.))) goto err1a;
		}

		free_vdim(&mdd.vdim);
	}

	if( (status1 & ST_CLOSE) && ZU )
	{
		if( !form_np( c_np_top ) ) goto err1;
		if( !form_np( c_np_bottom ) ) goto err1;
	}

	if( closed && !ZU && !(status1 & ST_CLOSE) && bottoms )
	{
		if( c_np_top->nov > MAXNOV ||	c_np_top->noe > MAXNOE )
		{
			if( !sub_division( c_np_top ) ) goto err1;
		}
		else
		{
			if( !np_cplane( c_np_top ) ) goto err1;
			if( !np_put( c_np_top, &c_list_str ) ) goto err1;
		}

//    
		c_np_top->ident = c_num_np++;
		hobject = listh_cross->hhead;  //  
		if( !put_bottom_bottom( c_np_top, hobject ) ) goto err1;
		if( c_np_top->nov > MAXNOV ||	c_np_top->noe > MAXNOE )
		{
			if( !sub_division( c_np_top ) ) goto err1;
		}
		else
		{
			if( !np_cplane( c_np_top ) ) goto err1;
			if( !np_put( c_np_top, &c_list_str ) ) goto err1;
		}
	}

	free_np_mem(&c_np_top);
	if(ZU) free_np_mem(&c_np_bottom);
//	free_vdim(vdim_path);
	cod = cinema_end(&c_list_str,hrez);
	return cod;
err1a:
	free_vdim(&mdd.vdim);
err1:
	free_np_mem(&c_np_top);
	free_np_mem(&c_np_bottom);
//	free_vdim(vdim_path);
	np_end_of_put(&c_list_str,NP_CANCEL,0,NULL);
	return FALSE;
}

BOOL create_pipe( lpMESHDD g, hOBJ cross, lpVDIM vdim_path, MATR m_all,
									BOOL *d_tip, BOOL zu, AP_TYPE bstat, AP_TYPE estat )
{
	short         i, mask;
	sgFloat      D, R;
	D_POINT     v1, v2;
	lpMNODE     previos, current, next;
	VDIM        vdim_cross;

	ZU = zu;

//  
	if( !create_cross( cross, &vdim_cross ) ) goto err;
	if( c_tip && ZU )
	{
		cinema_handler_err(CIN_NO_CORRECT_OZ);
		goto err;
	}
	//       .   
	if (ZU) {
		if ( (next = (lpMNODE)get_elem(&vdim_cross, 0)) == NULL) goto err;
		next->num = bstat;
		if ( (next =
				(lpMNODE)get_elem(&vdim_cross, vdim_cross.num_elem - 1)) == NULL) goto err;
		next->num = estat;
	}
/*	if ( vdim_cross.num_elem > MAXINT ) {
		cinema_handler_err(CIN_OVERFLOW_MDD);
		goto err;
	}
*/	g->n = (short)vdim_cross.num_elem;
	g->m = (short)vdim_path->num_elem;

//             
	if (!begin_rw(vdim_path, 0)) goto err;
	if ( (previos = (lpMNODE)get_next_elem(vdim_path)) == NULL) goto err1;
	o_hcncrd( m_all, &previos->p, &previos->p);

//             
	if ( (current = (lpMNODE)get_next_elem(vdim_path)) == NULL) goto err1;
	o_hcncrd( m_all, &current->p, &current->p);

//    (   )
	if( !(*d_tip) )
		if( !put_unclosed_path( 0, g, &previos->p, &current->p, &vdim_cross ))
			goto err1;

//     
	for (i = 2; i < vdim_path->num_elem; i++)
	{
		if ( (next = (lpMNODE)get_next_elem(vdim_path)) == NULL) goto err1;
//       
		o_hcncrd( m_all, &next->p, &next->p);

//    -   
// pp i- 
		if( !create_cross_section( previos, current, next,
															 &v1, &v2, &D, &R ) ) goto err1;
		mask = 0;
		if (!(current->num & AP_CONSTR)) mask = (short)CONSTR_U;
		else if (current->num & AP_SOPR || current->num & AP_COND)
			mask = COND_U;
		if( !write_cross_section( g, mask, &vdim_cross,
															&v1, &v2, D, R, bstat, estat ) )  goto err1;
		previos = current;
		current = next;
	}
	end_rw(vdim_path);

//    (   )
	if( !(*d_tip) )
	{
		if( !put_unclosed_path( 1, g, &previos->p, &current->p,
																				&vdim_cross )) goto err;	}
	else
	{
		if( !put_closed_path( g, previos, current,
													vdim_path, &vdim_cross, m_all,
													bstat, estat)) goto err; }
	free_vdim(&vdim_cross);
	*d_tip = c_tip;
	return TRUE;
err1:
	end_rw(vdim_path);
err:
	free_vdim(&g->vdim);
	free_vdim(&vdim_cross);
	return FALSE;
}

BOOL create_cross( hOBJ hpath, lpVDIM vdim_cross )
{
//	short         i=-1;
//  MATR        m_all;
//	lpMNODE     node;
  OSTATUS			status;

//     
	if (!apr_path( vdim_cross, hpath )) return FALSE;
  if (!get_status_path( hpath, &status)) return FALSE;
  c_tip = (status & ST_CLOSE) ? TRUE : FALSE;
	return TRUE;
}

BOOL put_unclosed_path( short j, lpMESHDD g,
												 lpD_POINT previos, lpD_POINT current,
												 lpVDIM vdim_cross )
{
	short     i, mask;
	sgFloat  D, R;
	MNODE   node;
	D_POINT v1;
	lpMNODE	lpnode;

	if( !j )
	{
		if (!begin_rw(vdim_cross, 0)) return FALSE;
		for (i = 0; i < vdim_cross->num_elem; i++) {
			if ( (lpnode = (lpMNODE)get_next_elem(vdim_cross)) == NULL) goto err;
			node = *lpnode;
			mask = (short)CONSTR_U;
			if (!(node.num & AP_CONSTR)) mask |= CONSTR_V;
			else if (node.num & AP_SOPR || node.num & AP_COND) mask |= COND_V;
			node.num = mask;
			if( !add_elem( &g->vdim, &node ) ) goto err;
		}
		end_rw(vdim_cross);
	}
	else
	{
		dpoint_sub( current, previos, &v1 );
		if( !dnormal_vector( &v1 ) )
		{
			cinema_handler_err( CIN_BAD_PATH );
			return FALSE;
		}
		D = dskalar_product( &v1, current );
		R = dskalar_product( &v1, &v1 );

		if (!write_cross_section(g, (short)CONSTR_U, vdim_cross,
														 &v1, &v1, D, R, AP_NULL, AP_NULL)) return FALSE;
	}
	return TRUE;
err:
	end_rw(vdim_cross);
	return FALSE;
}


#pragma argsused
BOOL put_closed_path(  lpMESHDD g,
											 lpMNODE previous, lpMNODE current,
											 lpVDIM vdim_path, lpVDIM vdim_cross,
											 lpMATR m_all, AP_TYPE bstat, AP_TYPE estat )
{
	short      i, mask;
	sgFloat   D, R;
	D_POINT  v1, v2;
	MNODE    next;
	lpMNODE  node;

	read_elem( vdim_path, 1, &next);
	o_hcncrd( m_all, &next.p, &next.p);
// pp 1-  (  )
	if( !create_cross_section( previous, current, &next,
														 &v1, &v2, &D, &R ) ) return FALSE;
	mask = 0;
	if (!(current->num & AP_CONSTR)) mask = (short)CONSTR_U;
	else if (current->num & AP_SOPR || current->num & AP_COND) mask = (short)COND_U;
	if( !write_cross_section( g, mask, vdim_cross,
														&v1, &v2, D, R, bstat, estat))  return FALSE;

	R = g->vdim.num_elem;
	if (!begin_rw(&g->vdim, 0)) return FALSE;
	for (i = 0; i <vdim_cross->num_elem; i++) {//   .
		if ( (node = (lpMNODE)get_next_elem(&g->vdim)) == NULL) return FALSE;
		if ( !add_elem(&g->vdim, node)) return FALSE;
		if( i==0 && ZU ) write_cover( c_np_top,  node, bstat );
		if( i==vdim_cross->num_elem-1 && ZU ) write_cover( c_np_bottom, node, estat );
	}
	end_rw(&g->vdim);
	return TRUE;
}

BOOL create_cross_section( lpMNODE previos, lpMNODE current, lpMNODE next,
													 lpD_POINT v1, lpD_POINT v2, sgFloat *D, sgFloat *R )
{
	dpoint_sub( &current->p, &previos->p, v1 );
	if( !dnormal_vector( v1 ) )
	{
		cinema_handler_err( CIN_BAD_PATH );
		return FALSE;
	}

	dpoint_sub( &next->p, &current->p, v2 );
	if( !dnormal_vector( v2 ) )
	{
		cinema_handler_err( CIN_BAD_PATH );
		return FALSE;
	}

	v2->x = ( v1->x + v2->x )/2;
	v2->y = ( v1->y + v2->y )/2;
	v2->z = ( v1->z + v2->z )/2;
	if( !dnormal_vector( v2 ) )
	{
		cinema_handler_err( CIN_BAD_PATH );
		return FALSE;
	}

	*D = dskalar_product( v2, &current->p );
	*R = dskalar_product( v2, v1 );
	return TRUE;
}


BOOL write_cross_section( lpMESHDD g, short mask, lpVDIM vdim_cross,
													lpD_POINT v1, lpD_POINT v2, sgFloat D, sgFloat R,
													AP_TYPE bstat, AP_TYPE estat)
{
	short      i;
	sgFloat   t;
	MNODE    node;
	lpMNODE  node_c;

	if (!begin_rw(vdim_cross, 0)) return FALSE;
	for (i = 0; i < vdim_cross->num_elem; i++) {//   .
		if ( (node_c = (lpMNODE)get_next_elem(vdim_cross)) == NULL) goto err;
		t = ( D - dskalar_product( v2, &node_c->p ))/R;
		node.p.x = node_c->p.x = node_c->p.x + v1->x*t;
		node.p.y = node_c->p.y = node_c->p.y + v1->y*t;
		node.p.z = node_c->p.z = node_c->p.z + v1->z*t;
		node.num = mask;
		if (!(node_c->num & AP_CONSTR))	node.num |= CONSTR_V;
		else
			if (node_c->num & AP_SOPR ||
					node_c->num & AP_COND)		node.num |= COND_V;
		if ( !add_elem(&g->vdim, &node)) goto err;
	}
	if( ZU )
	{
		read_elem  (vdim_cross, 0, &node );
		write_cover( c_np_top,    &node, bstat );
		read_elem  (vdim_cross, vdim_cross->num_elem-1, &node );
		write_cover( c_np_bottom, &node, estat );
	}
	end_rw(vdim_cross);
	return TRUE;
err:
	end_rw(vdim_cross);
	return FALSE;
}

static BOOL write_cover( lpNPW np, lpMNODE node, AP_TYPE stat )
{
//      ->  np
	if( np->nov + 1 > np->maxnov ) if( !my_expand_face( np, 1 )) return FALSE;
	if( np->nov != 0 )
	{
// 
		np->efr[np->nov].bv = np->nov;
		np->efr[np->nov].ev = np->nov+1;
		np->efr[np->nov].el = (stat & AP_CONSTR) ?
														ST_TD_COND | ST_TD_APPROCH : ST_TD_CONSTR;
// 
		np->efc[np->nov].ep = np->nov+1;
		np->efc[np->nov].fp = 1;
		np->efc[np->nov].fm = np->efc[np->nov].em = 0;
		np->noe++;
	}
	np->v[++(np->nov)] = node->p;
	return TRUE;
}

static BOOL form_np( lpNPW np )
{
	np->nov--;
// 
	np->efr[np->nov].ev = 1;

// 
	np->f[1].fc = 1;
	np->f[1].fl |= ST_FACE_PLANE;
	np->nof = 1;
	np->noc = 1;
	np->c[1].nc = 0;
	np->c[1].fe = 1;

	np->efc[np->nov].ep = 1;
// 
	if( np->nov > MAXNOV ||	np->noe > MAXNOE )
	{
		if( !sub_division( np ) ) return FALSE;
	}
	else
	{
		if( !np_cplane( np ) ) return FALSE;
		if( !np_put( np, &c_list_str ) ) return FALSE;
	}
	c_num_np++;
	return TRUE;
}
