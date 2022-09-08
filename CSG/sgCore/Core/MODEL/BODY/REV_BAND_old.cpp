#include "../../sg.h"

static  BOOL create_spheric_band( lpMESHDD g, sgFloat r,
																			sgFloat c1, sgFloat c2, sgFloat, short );
static  hOBJ create_my_arc(sgFloat coef1, sgFloat coef2, sgFloat r );
static  BOOL rotate_point( lpMESHDD g, lpD_POINT p,
													short NUM_REV_POINTS, sgFloat, short );

short create_spheric_band_np( lpLISTH listh, sgFloat  *par ){
	sgFloat		r 		 = par[0]; //  
	sgFloat		coeff1 = par[1]; //  
	sgFloat		coeff2 = par[2];
	sgFloat    toll	 = par[3]; // 
	int			  n_merd = (int)par[4]; //- 
	BOOL			zu		 = TRUE;//par[5]; //   
	MESHDD      mdd;
	lpNPW       np = NULL;

	c_num_np = -32767;
	if ( !np_init_list(&c_list_str) ) return FALSE;

	if ( !create_spheric_band(&mdd, r, coeff1, coeff2, toll, n_merd ) ) goto err0;
	if (!(np_mesh4p(&c_list_str,&c_num_np,&mdd,0.))) goto err1;

	if( zu ){
		if ((np = creat_np_mem(TNPW,MAXNOV,MAXNOE,MAXNOC, MAXNOF, MAXNOE)) == NULL)  goto err1;
		np_init((lpNP)np);
//    
		if( fabs(coeff2) != 1 ){
			np->ident = c_num_np++;
			put_top_bottom( np, &mdd );

			if( np->nov > MAXNOV ||	np->noe > MAXNOE ){
				if( !sub_division( np ) ) goto err2;
			}
			else{
				if( !np_cplane( np ) ) goto err2;
				if( !np_put( np, &c_list_str ) ) goto err2;
			}
		}
		if( fabs(coeff1) != 1 ){
//    
			np->ident = c_num_np++;
			put_bottom_bottom_skin( np, &mdd );

			if( np->nov > MAXNOV ||	np->noe > MAXNOE ){
				if( !sub_division( np ) ) goto err2;
			}
			else{
				if ( !np_cplane( np ) ) goto err2;
				if ( !np_put( np, &c_list_str ) ) goto err2;
			}
		}
		free_np_mem(&np);
	}
	free_vdim(&mdd.vdim);

//   
	if ((c_num_np = np_end_of_put(&c_list_str,NP_FIRST,30,listh)) == 0) goto err1;
	return c_num_np;

err2:
	free_np_mem(&np);
err1:
	free_vdim(&mdd.vdim);
err0:
	np_end_of_put(&c_list_str,NP_CANCEL,0,NULL);
	return FALSE;
}

static  BOOL create_spheric_band( lpMESHDD g, sgFloat r,	sgFloat c1,
																			sgFloat c2, sgFloat toll, short n_merd ){
	short         NUM_REV_POINTS;
	int				num_elem;
	int					mask;
	sgFloat 			fi=2*M_PI, t;
	lpMNODE			node;
	VDIM				vdim_band;
	hOBJ			  hobj;

// 
	if( (hobj = create_my_arc(  c1, c2, r )) == NULL ) return FALSE;

	init_vdim(&g->vdim,sizeof(MNODE));

	NUM_REV_POINTS = ( fabs(fi) >= 2*M_PI ) ?
												( n_merd ) : ((short)(fabs(fi)/(2*M_PI)*n_merd));

	if ( NUM_REV_POINTS == 0 ) NUM_REV_POINTS = 1;

// 

	init_vdim(&vdim_band, sizeof(MNODE));
	t = c_h_tolerance;
	c_h_tolerance = toll;
	if (!apr_path( &vdim_band, hobj )) goto err1;
	c_h_tolerance = t;

	g->n = NUM_REV_POINTS + 1;
	g->m = (short)vdim_band.num_elem;

	if (!begin_rw(&vdim_band, 0)) goto err1;
	for (num_elem = 0; num_elem < vdim_band.num_elem; num_elem++) {
		if ((node = (lpMNODE)get_next_elem(&vdim_band)) == NULL) goto err2;
//
		mask = 0;
		if (!(node->num & AP_CONSTR)) mask = CONSTR_U;
		else if (node->num & AP_SOPR || node->num & AP_COND) mask = COND_U;

// 
		if( !rotate_point( g, &node->p, NUM_REV_POINTS, fi, mask ) ) goto err2;
	}
	o_free( hobj, NULL );
	end_rw(&vdim_band);
	free_vdim(&vdim_band);
	return TRUE;
err2:
	end_rw(&vdim_band);
err1:
	o_free( hobj, NULL );
	free_vdim(&vdim_band);
	free_vdim(&g->vdim);
	return FALSE;
}

static  hOBJ create_my_arc(sgFloat coef1, sgFloat coef2, sgFloat r )
{
	GEO_ARC     arc;
	sgFloat 			fi_b, fi_e;
	hOBJ				hobj;
	lpOBJ				obj;

	arc.vc.x = 0.;//
	arc.vc.y = 0.;
	arc.vc.z = 0.;
	arc.r    = r; //
	arc.n.x  = 0; //
	arc.n.y  =-1;
	arc.n.z  = 0;
	fi_b=acos(fabs(coef1));
	if( coef1 >= 0 ) fi_b += M_PI/2;
	else             fi_b = M_PI*3/2 - fi_b;
	arc.vb.x=r*cos(fi_b);// 
	arc.vb.y=0;
	arc.vb.z=r*sin(fi_b);
	fi_e=acos(fabs(coef2));
	if( coef2 >= 0 ) fi_e += M_PI/2;
	else             fi_e = M_PI*3/2 - fi_e;
	arc.ve.x=r*cos(fi_e);// 
	arc.ve.y=0;
	arc.ve.z=r*sin(fi_e);
	arc.angle=-(fi_b-fi_e);
	calc_arc_ab(&arc);
	if((hobj = o_alloc(OARC)) == NULL)
		return NULL;
	obj = (lpOBJ)hobj;
	memcpy(obj->geo_data, &arc, sizeof(arc));
	return hobj;// '
}

static  BOOL rotate_point( lpMESHDD g, lpD_POINT p,
									short NUM_REV_POINTS, sgFloat fi, short mask ){
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

