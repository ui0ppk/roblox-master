#include "../../sg.h"
static BOOL put_point_mesh( lpSURF_DAT surf_dat, lpMESHDD mdd, MATR i_surf_matr,
										 sgFloat t_v, short constr,
										 short i, short middle_u, short *il, short ie );
static BOOL put_point_array( lpSURF_DAT surf_dat, lpD_POINT array, sgFloat t_v,
														 short i, short middle_u, short *count_u, short ie );

static void	calc_w_point_surf( lpSURF_DAT srf_dat, sgFloat *point, sgFloat *N_i, sgFloat *N_j );
static sgFloat calc_w_surf(lpSURF_DAT srf_dat, sgFloat *N_i, sgFloat *N_j );

static void calc_w_deriv_surface_U( lpSURF_DAT srf_dat, lpD_POINT deriv,
																		sgFloat *N_i, sgFloat *N_i1, sgFloat *N_j );
static void calc_w_deriv_surface_V( lpSURF_DAT srf_dat, lpD_POINT deriv,
																		sgFloat *N_i, sgFloat *N_j, sgFloat *N_j1 );
static BOOL Calculate_Weight_Deriv_U(lpSURF_DAT srf_dat, sgFloat u, sgFloat v, lpD_POINT d);
static BOOL Calculate_Weight_Deriv_V(lpSURF_DAT srf_dat, sgFloat u, sgFloat v, lpD_POINT d);
//---------------------------------------------------->>>>>

BOOL Bild_Mesh_On_Surf(lpSURF_DAT surf_dat, lpMESHDD mdd, short middle_u, short middle_v){
 if( !Bild_Mesh_On_Surf_Restrict(surf_dat, mdd, middle_u, middle_v,
																 0, surf_dat->P.n-1, 0, surf_dat->P.m-1))	return FALSE;
 return TRUE;
}

BOOL Bild_Mesh_On_Surf_Restrict(lpSURF_DAT surf_dat, lpMESHDD mdd,
																short middle_u, short middle_v, short ib, short ie, short jb, short je){
short		i, j, k, number;
short		count_u=0, count_v=0, num_v, t=0;
sgFloat  t_v, dt_v, w;
W_NODE	wnode;
MNODE 	node;
MATR		i_surf_matr;

  if( !o_minv(surf_matr, i_surf_matr) ) return FALSE;

	if( surf_dat->degree_p == 1 && surf_dat->degree_q == 1 ){
		mdd->m=je-jb+1;
		mdd->n=ie-ib+1;
		for( number=0, j=jb; j<=je; j++, number += surf_dat->P.n )
			for( i=ib; i<=ie; i++ )	{
				read_elem ( &surf_dat->P.vdim, number+i, &wnode );
	      Create_3D_From_4D( &wnode, (sgFloat *)&node.p, &w);
				if( !o_hcncrd( i_surf_matr, &node.p, &node.p ) ) return FALSE;
				node.num = (short)CONSTR_U;
        node.num |= (short)CONSTR_V;
				if( !add_elem(&mdd->vdim, &node ) ) return FALSE;
			}
		return TRUE;
	}

	for( j=jb; j<=je-1; j++ ){
		t_v=surf_dat->v[j];
		for( i=ib; i<=ie-1; i++ )
			if( !put_point_mesh( surf_dat, mdd, i_surf_matr, t_v, COND_U, i, middle_u, &t, ie )) return FALSE;
		count_v++;

		dt_v=(surf_dat->v[j+1]-surf_dat->v[j]);
		num_v=middle_v+(short)(dt_v*10);
		dt_v /= (num_v+1);

		for( k=0; k<num_v; k++ ){
			t_v += dt_v;
			for( i=ib; i<=ie-1; i++ )
				if( !put_point_mesh( surf_dat, mdd, i_surf_matr, t_v, 0, i, middle_u, &t, ie )) return FALSE;
			count_v++;
		}
	}
	for( i=ib; i<=ie-1; i++ )
		if( !put_point_mesh( surf_dat, mdd, i_surf_matr, surf_dat->v[je], COND_U, i, middle_u, &count_u, ie )) return FALSE;
	count_v++;

	mdd->m=count_v;
	mdd->n=count_u;
	return TRUE;
}

static BOOL put_point_mesh( lpSURF_DAT surf_dat, lpMESHDD mdd, MATR i_surf_matr, sgFloat t_v, short constr,
										 short i, short middle_u, short *count_u, short ie ){
short		j, num_u;
sgFloat	t_u, dt_u;
MNODE		node;

	t_u=surf_dat->u[i];
	if( !get_point_on_surface(surf_dat, t_u, t_v, &node.p)) return FALSE;
	if( !o_hcncrd( i_surf_matr, &node.p, &node.p ) ) return FALSE;
	node.num  = constr;
	node.num |= COND_V;
	if( !add_elem(&mdd->vdim, &node )) return FALSE;
	(*count_u)++;

	dt_u=(surf_dat->u[i+1]-surf_dat->u[i]);
	num_u=middle_u+(short)(dt_u*10);
	dt_u /= (num_u+1);

	for( j=0; j<num_u; j++ ){
		t_u += dt_u;
		if( !get_point_on_surface(surf_dat, t_u, t_v, &node.p) ) return FALSE;
		if( !o_hcncrd( i_surf_matr, &node.p, &node.p ) ) return FALSE;
		node.num  = constr;
//    node.num |= COND_V;
		if( !add_elem(&mdd->vdim, &node )) return FALSE;
		(*count_u)++;
	}
	if( (i+1) == ie ){
		if( !get_point_on_surface(surf_dat, surf_dat->u[ie], t_v, &node.p)) return FALSE;
		if( !o_hcncrd( i_surf_matr, &node.p, &node.p ) ) return FALSE;
		node.num  = constr;
		node.num |= COND_V;
		if( !add_elem(&mdd->vdim, &node )) return FALSE;
		(*count_u)++;
	}
	return TRUE;
}

BOOL Bild_Array_On_Surf_Restrict(lpSURF_DAT surf_dat, lpD_POINT *array,
																short middle_u, short middle_v, short ib, short ie, short jb, short je){
short  	i, j, k, number;
short	 	count_u, count_v=0;
sgFloat  t_v, dt_v, w;
W_NODE	node;
D_POINT point;

	if( surf_dat->degree_p == 1 && surf_dat->degree_q == 1 ){
		for( number=0, j=jb; j<=je; j++, number += surf_dat->P.n )
			for( i=ib; i<=ie; i++ )	{
				read_elem ( &surf_dat->P.vdim, number+i, &node );
        Create_3D_From_4D( &node, (sgFloat *)&point, &w );
				memcpy(&array[j-jb][i-ib], &point, sizeof(D_POINT));
			}
		return TRUE;
	}

	for( j=jb; j<je; j++ ){
		t_v=surf_dat->v[j];
    count_u=0;
	 	for( i=ib; i<ie; i++ )
		 	if( !put_point_array( surf_dat, array[count_v], t_v, i, middle_u, &count_u, ie )) return FALSE;
		count_v++;

		dt_v=(surf_dat->v[j+1]-surf_dat->v[j])/(middle_v+1);
		for( k=0; k<middle_v; k++ ){
      t_v += dt_v;
      count_u=0;
			for( i=ib; i<ie; i++ )
				if( !put_point_array( surf_dat, array[count_v], t_v, i, middle_u, &count_u, ie )) return FALSE;
			count_v++;
		}
	}
  count_u=0;
	for( i=ib; i<ie; i++ )
		if( !put_point_array( surf_dat, array[count_v], surf_dat->v[je], i, middle_u, &count_u, ie )) return FALSE;

	return TRUE;
}

static BOOL put_point_array( lpSURF_DAT surf_dat, lpD_POINT array, sgFloat t_v,
														 short i, short middle_u, short *count_u, short ie ){
short		k;
sgFloat	t_u, dt_u;
D_POINT point;

	t_u=surf_dat->u[i];
	if( !get_point_on_surface(surf_dat, t_u, t_v, &point)) return FALSE;
	memcpy(&array[(*count_u)++], &point, sizeof(D_POINT));

	dt_u=(surf_dat->u[i+1]-surf_dat->u[i])/(middle_u+1);
	for( k=0; k<middle_u; k++ ){
 	  t_u += dt_u;
		if( !get_point_on_surface(surf_dat, t_u, t_v, &point)) return FALSE;
		memcpy(&array[(*count_u)++], &point, sizeof(D_POINT));
	}

	if( (i+1) == ie ){
		if( !get_point_on_surface(surf_dat, surf_dat->u[ie], t_v, &point)) return FALSE;
		memcpy(&array[(*count_u)++], &point, sizeof(D_POINT));
	}
	return TRUE;
}

BOOL knots_on_surface(lpSURF_DAT surf_dat){
short	 	i, j, k, number;
sgFloat	w;
W_NODE	node;
D_POINT point;

	if( surf_dat->degree_p == 1 && surf_dat->degree_q == 1 ){
		for( number=0, j=0; j<surf_dat->P.m; j++, number+=surf_dat->P.n ){
			for( k=0; k<surf_dat->P.n; k++ )	{
				read_elem ( &surf_dat->P.vdim, number+k, &node );
        Create_3D_From_4D( &node, (sgFloat *)&point, &w);
				write_elem( &surf_dat->knots.vdim, number+k, &point );
			}
		}
    surf_dat->knots.m=surf_dat->P.m;
    surf_dat->knots.n=surf_dat->P.n;
		return TRUE;
	}

	for( number=0, i=0; i<surf_dat->P.m; i++, number+=surf_dat->P.n ){
		for( j=0; j<surf_dat->P.n; j++ ){
			if( !get_point_on_surface(surf_dat, surf_dat->u[j],
																surf_dat->v[i], &point)) return FALSE;
			write_elem(&surf_dat->knots.vdim, number+j, &point );
		}
	}
  surf_dat->knots.m=surf_dat->P.m;
  surf_dat->knots.n=surf_dat->P.n;
	return TRUE;
}

BOOL get_point_on_surface(lpSURF_DAT surf_dat, sgFloat u, sgFloat v,
													lpD_POINT p){
BOOL		ret=FALSE;
short		i, j;//, k, number;
sgFloat 	/*w,*/ *N_j, *N_i;
//W_NODE  P, node;

	if( ( N_j = (sgFloat*)SGMalloc( (surf_dat->P.m+3 )*sizeof(sgFloat) ) ) == NULL ) return FALSE;
	for( j=0; j<surf_dat->P.m; j++ )
		if( (surf_dat->V[j]<=v) && (v<=surf_dat->V[j+surf_dat->degree_q+1]))
			N_j[j]=nurbs_basis_func( j, surf_dat->degree_q, surf_dat->V, v);

	if( ( N_i = (sgFloat*)SGMalloc( (surf_dat->P.n+3 )*sizeof(sgFloat) ) ) == NULL ) goto err;
	for( i=0; i<surf_dat->P.n; i++ )
		if( (surf_dat->U[i]<=u) && (u<=surf_dat->U[i+surf_dat->degree_p+1]))
			N_i[i]=nurbs_basis_func( i, surf_dat->degree_p, surf_dat->U, u);

	calc_w_point_surf( surf_dat, (sgFloat *)p, N_i, N_j );

/*	memset( &node, 0, sizeof(W_NODE));
	for( number=0, j=0; j<surf_dat->P.m; j++, number +=surf_dat->P.n )
		if( fabs( N_j[j] ) > eps_n ){
     	for (i=0; i<surf_dat->P.n; i++ )
				if( fabs( N_i[i] )>eps_n ){
					read_elem(&surf_dat->P.vdim, number+i, &P);
     			for( k=0; k<4; k++ )	node.vertex[k] += P.vertex[k]*N_i[i]*N_j[j];
        }
    }
	Create_3D_From_4D( &node, (sgFloat *)p, &w);
*/

	ret=TRUE;
err:
	if( N_i )  { SGFree(N_i); }
	if( N_j )  { SGFree(N_j); }
	return ret;
}

//------------------->>>>>   <<<<<-------------------

BOOL Create_Ribs( lpLISTH list, lpVDIM ribs ){
hOBJ		hobj, hspl;

	init_vdim( ribs, sizeof(hOBJ) );
	hobj = list->hhead;  // list begining
	while( hobj != NULL ){ // while objects exist into list
		if( !transform_to_NURBS( hobj, &hspl)) goto err;
		if( !add_elem( ribs, &hspl )) goto err;
		get_next_item_z( SEL_LIST, hobj, &hobj );
	}
	return TRUE;
err:
	free_vdim(ribs);
	return FALSE;
}

/*******************************************************************************
* Elevate ribs degree for creating surface
*/
BOOL Elevete_Rib_Degree( lpVDIM ribs, short degree, lpVDIM rib_curv ){
short					i=0;
hOBJ		 			hobj;
lpOBJ 				obj;
SPLY_DAT 			sply_dat;
lpGEO_SPLINE 	gspline;

	init_vdim( rib_curv, sizeof(SPLY_DAT) );
	while( read_elem( ribs, i++, &hobj )){ // while objects exist into list
//create work spline structure for i-th curve
		if( !init_sply_dat(SPL_NEW, degree, SPL_APPR, &sply_dat )) goto err;
		obj = (lpOBJ)hobj;
		gspline = (lpGEO_SPLINE)obj->geo_data;
		if( !unpack_geo_sply( gspline, &sply_dat)) goto err1;

//put real degree
		sply_dat.degree=gspline->degree;

		if( sply_dat.degree != degree ){
			if( !Increase_Spline_Degree(&sply_dat, degree)) goto err1;
		}

		if( !add_elem(rib_curv, &sply_dat) ) goto err1;
		get_next_item_z( SEL_LIST, hobj, &hobj );
	}
	return TRUE;
err1:
	free_sply_dat(&sply_dat);
err:
	free_vdim(rib_curv);
	return FALSE;
}

/*******************************************************************************
* Change the beginning point(if nessesary)
* listh - list of objects
* ribs  - VDIM of sections, coresponding to objects
* spin  - the beginning points of sections
*/
BOOL Reorient_Ribs( lpLISTH list, lpVDIM ribs, lpD_POINT spin ){
short 	 		i=0, ii, iii, step=60;
sgFloat			t_sply, dt, dist_new, dist;
hOBJ		 		hobj;
SPLY_DAT 		sply_dat, sply_dat1, sply_dat2;
D_POINT			point;
OSTATUS 		status;

	hobj = list->hhead;  // list begining
	while( hobj != NULL ){ // while objects exist into list
		read_elem( ribs, i, &sply_dat );
		if (!get_status_path(hobj, &status)) return FALSE;
		if (status & ST_CLOSE) {
//if spline is closed, change the beginning point(if nessesary)
			if( dpoint_eq(&spin[i],(lpD_POINT)&sply_dat.knots[0], eps_d) ) break;	//countor's beginning is not changing

//detect for the beginning point parameter
			dist=1.e35;
			for( ii=0; ii<sply_dat.nump-1; ii++ ){
				dt=(sply_dat.u[ii+1]-sply_dat.u[ii])/step;
        for( iii=0; iii<step; iii++ ){
					get_point_on_sply( &sply_dat, sply_dat.u[ii]+dt*iii, &point, 0);
					dist_new = dpoint_distance(&spin[i], &point);
					if (dist_new < dist) {
						dist = dist_new;
						t_sply=sply_dat.u[ii]+dt*iii;
					}
				}
			}
			get_point_on_sply( &sply_dat, t_sply, &spin[i], 0);
			if( !Spline_By_Point( &sply_dat, (void  *)&spin[i], &t_sply, &dist )) return FALSE;

//get spline part
			if( !Get_Part_Spline_Dat(&sply_dat, 0., t_sply, &sply_dat1)) return FALSE;
			if( !Get_Part_Spline_Dat(&sply_dat, t_sply, 1., &sply_dat2)) goto err1;

//free initial spline-rib
			free_sply_dat( &sply_dat );

//union two splines unde new order
			if( !Two_Spline_Union_Dat(&sply_dat1, &sply_dat2, &sply_dat)) goto err2;
     	free_sply_dat( &sply_dat1 );
     	free_sply_dat( &sply_dat2 );

			if( !Reparametrization(	sply_dat.nump, sply_dat.numU,
															0, 1, sply_dat.degree, sply_dat.U, sply_dat.P, sply_dat.u )) return FALSE;
		}
		if( !write_elem(ribs, i++, &sply_dat)) return FALSE;
		get_next_item_z( SEL_LIST, hobj, &hobj );
	}
	return TRUE;
err2:
 	free_sply_dat( &sply_dat2 );
err1:
 	free_sply_dat( &sply_dat1 );
	return FALSE;
}

BOOL get_deriv_on_surface(lpSURF_DAT surf_dat, sgFloat u, sgFloat v,
													lpD_POINT d, short deriv ){
	switch (deriv){
  	case 0:
			if( !Calculate_Weight_Deriv_U( surf_dat, u, v, d ) ) return FALSE;
      return TRUE;
    case 1:
			if( !Calculate_Weight_Deriv_V( surf_dat, u, v, d ) ) return FALSE;
      return TRUE;
    case 2:
    case 3:
    case 4:
    default:
    	return FALSE;
  }
}

static BOOL Calculate_Weight_Deriv_U(lpSURF_DAT srf_dat, sgFloat u, sgFloat v, lpD_POINT d){
BOOL   ret=FALSE;
short	 i;
sgFloat *N_i, *N_i1, *N_j;

	if((N_i=(sgFloat*)SGMalloc(srf_dat->P.n*sizeof(sgFloat)))==NULL) return FALSE;

	if((N_i1=(sgFloat*)SGMalloc((srf_dat->P.n-1)*sizeof(sgFloat)))==NULL) goto err;

	if((N_j=(sgFloat*)SGMalloc(srf_dat->P.m*sizeof(sgFloat)))==NULL) goto err;

	for( i=0; i<srf_dat->P.n; i++ )
		if( srf_dat->U[i] <= u && u <= srf_dat->U[i+srf_dat->degree_p+1] )
			N_i[i] = nurbs_basis_func( i, srf_dat->degree_p, srf_dat->U, u );

	for( i=0; i<srf_dat->P.n-1; i++ )
		if( srf_dat->U[i] <= u && u <= srf_dat->U[i + srf_dat->degree_p ] )
			 N_i1[i] = nurbs_basis_func( i+1, srf_dat->degree_p-1, srf_dat->U, u );

	for( i=0; i<srf_dat->P.m; i++ )
		if( srf_dat->V[i] <= v && v <= srf_dat->V[i+srf_dat->degree_q+1] )
			N_j[i] = nurbs_basis_func( i, srf_dat->degree_q, srf_dat->V, v );

//	for( i=1; i<sply_dat->nump; i++ )
//		if( sply_dat->U[i] <= t && t <= sply_dat->U[i + degree] )
//			 N_i[i] = nurbs_basis_func( i, degree-1, sply_dat->U, t );

	calc_w_deriv_surface_U( srf_dat, d, N_i, N_i1, N_j);
	ret=TRUE;
err:
if(N_j)  SGFree(N_j);
if(N_i)  SGFree(N_i);
if(N_i1) SGFree(N_i1);
return ret;
}

static void calc_w_deriv_surface_U( lpSURF_DAT srf_dat, lpD_POINT deriv,
												sgFloat *N_i, sgFloat *N_i1, sgFloat *N_j ){
//N_i -  U
//N_j -  V

short		 i, j, ii, jj, k;
sgFloat   feet, w_w, w_s, w_der, wij, wi1j;
sgFloat   *ppoint, point[3], A[3], A_tmp_i[3], A_tmp_ii[3], A_tmp_jj[3], P_s[3];
DA_POINT Pij, Pi1j;
W_NODE	 node, node1;

	ppoint = (sgFloat*)deriv;
//------------>>>>>>>>>>weight
	w_w=calc_w_surf( srf_dat, N_i, N_j );
//------------>>>>>>>>>>derivates of weight
	w_der=0;
	for( j=0; j<srf_dat->P.m; j++ ){
  	if( N_j[j] != 0 ){
      w_s=0;
	    for( i=0; i<srf_dat->P.n-1; i++ ){
				if( N_i1[i] != 0 ){
					if( (feet = srf_dat->U[i+srf_dat->degree_p+1] - srf_dat->U[i+1]) > eps_n ){
    		   	read_elem( &srf_dat->P.vdim, j*srf_dat->P.n+i,   &node );
        		read_elem( &srf_dat->P.vdim, j*srf_dat->P.n+i+1, &node1 );
	        	w_s += (node1.vertex[3]-node.vertex[3])*N_i1[i]/feet;
          }
        }
      }
      w_der += w_s*N_j[j];
    }
  }
  w_der *= srf_dat->degree_p;

//------------>>>>>>>>>>point
	calc_w_point_surf( srf_dat, point, N_i, N_j );

//------------>>>>>>>>>> A`[u,v]
  A[0]=A[1]=A[2]=0.;
	for(j=0; j<srf_dat->P.m; j++ ){
   	if( N_j[j] !=0 ) {
		  A_tmp_i[0]=A_tmp_i[1]=A_tmp_i[2]=0.;
			for(i=0; i<srf_dat->P.n-1; i++ ){
  			if( N_i1[i] != 0 ){
		    	if( ( feet=srf_dat->U[i+srf_dat->degree_p+1] - srf_dat->U[i+1]) > eps_n ){
				   	read_elem( &srf_dat->P.vdim, j*srf_dat->P.n+i, &node );
			 			Create_3D_From_4D( &node, (sgFloat *)Pij, &wij);
		 				read_elem( &srf_dat->P.vdim, j*srf_dat->P.n+i+1,&node );
					 	Create_3D_From_4D( &node, (sgFloat *)Pi1j, &wi1j);
            w_s = wi1j - wij;
            for( k=0; k<3; k++ ) P_s[k] = Pi1j[k] - Pij[k];

					  A_tmp_jj[0]=A_tmp_jj[1]=A_tmp_jj[2]=0.;
						for(jj=0; jj<srf_dat->P.m; jj++) {
						  if( N_j[jj] != 0 ){
		  					A_tmp_ii[0]=A_tmp_ii[1]=A_tmp_ii[2]=0.;
								for(ii=0; ii<srf_dat->P.n; ii++ ){
	  							if( N_i[ii] != 0 ){
								   	read_elem( &srf_dat->P.vdim, jj*srf_dat->P.n+ii, &node );
                    Create_3D_From_4D( &node, (sgFloat *)Pij, &wij);
						  	    for( k=0; k<3; k++ ) A_tmp_ii[k] += N_i[ii]*(Pij[k]*w_s+wij*P_s[k]);
                  }
					 	 		}
						    for( k=0; k<3; k++ ) A_tmp_jj[k] += N_j[jj]*A_tmp_ii[k];
			  		  }
						}
  					for( k=0; k<3; k++ ) A_tmp_i[k] += N_i1[i]*A_tmp_jj[k]/feet;
					}
    		}
		  }
			for( k=0; k<3; k++ ) A[k] += N_j[j]*A_tmp_i[k];
		}
  }

//------------>>>>>>>>>> derivates
	for(i=0; i<3; i++) ppoint[i]=(srf_dat->degree_p*A[i]-w_der*point[i])/w_w;
}

/*******************************************************************************
*       (u,v)   V
*/
static BOOL Calculate_Weight_Deriv_V(lpSURF_DAT srf_dat, sgFloat u, sgFloat v, lpD_POINT d){
BOOL   ret=FALSE;
short	 i;
sgFloat *N_i, *N_j, *N_j1;

	if((N_i=(sgFloat*)SGMalloc(srf_dat->P.n*sizeof(sgFloat)))==NULL) return FALSE;

	if((N_j=(sgFloat*)SGMalloc(srf_dat->P.m*sizeof(sgFloat)))==NULL) return FALSE;

	if((N_j1=(sgFloat*)SGMalloc((srf_dat->P.m-1)*sizeof(sgFloat)))==NULL) goto err;

	for( i=0; i<srf_dat->P.n; i++ )
		if( srf_dat->U[i] <= u && u <= srf_dat->U[i+srf_dat->degree_p+1] )
			N_i[i] = nurbs_basis_func( i, srf_dat->degree_p, srf_dat->U, u );

	for( i=0; i<srf_dat->P.m; i++ )
		if( srf_dat->V[i] <= v && v <= srf_dat->V[i+srf_dat->degree_q+1] )
			N_j[i] = nurbs_basis_func( i, srf_dat->degree_q, srf_dat->V, v );

	for( i=0; i<srf_dat->P.m-1; i++ )
		if( srf_dat->V[i] <= v && v <= srf_dat->V[i + srf_dat->degree_q ] )
			 N_j1[i] = nurbs_basis_func( i+1, srf_dat->degree_q-1, srf_dat->V, v );

//	for( i=1; i<sply_dat->nump; i++ )
//		if( sply_dat->U[i] <= t && t <= sply_dat->U[i + degree] )
//			 N_i[i] = nurbs_basis_func( i, degree-1, sply_dat->U, t );

	calc_w_deriv_surface_V( srf_dat, d, N_i, N_j, N_j1);
	ret=TRUE;
err:
if(N_j)  SGFree(N_j);
if(N_i)  SGFree(N_i);
if(N_j1) SGFree(N_j1);
return ret;
}

static void calc_w_deriv_surface_V( lpSURF_DAT srf_dat, lpD_POINT deriv,
												sgFloat *N_i, sgFloat *N_j, sgFloat *N_j1 ){
//N_i -  U
//N_j -  V

short		 i, j, ii, jj, k;
sgFloat   feet, w_w, w_s, w_der, wij, wij1;
sgFloat   *ppoint, point[3], A[3], A_tmp_i[3], A_tmp_ii[3], A_tmp_jj[3], P_s[3];
DA_POINT Pij, Pij1;
W_NODE	 node, node1;

	ppoint = (sgFloat*)deriv;
//------------>>>>>>>>>>weight
	w_w=calc_w_surf( srf_dat, N_i, N_j );
//------------>>>>>>>>>>derivates of weight
	w_der=0;
	for( j=0; j<srf_dat->P.m-1; j++ ){
  	if( N_j1[j] != 0 ){
			if( (feet = srf_dat->V[j+srf_dat->degree_q+1] - srf_dat->V[j+1]) > eps_n ){
	      w_s=0;
		    for( i=0; i<srf_dat->P.n; i++ ){
					if( N_i[i] != 0 ){
   			   	read_elem( &srf_dat->P.vdim, j*srf_dat->P.n+i,   &node );
       			read_elem( &srf_dat->P.vdim, (j+1)*srf_dat->P.n+i, &node1 );
        		w_s += (node1.vertex[3]-node.vertex[3])*N_i[i];
  	      }
    	  }
      	w_der += w_s*N_j[j]/feet;
      }
    }
  }
  w_der *= srf_dat->degree_q;
//------------>>>>>>>>>>point
	calc_w_point_surf( srf_dat, point, N_i, N_j );
//------------>>>>>>>>>> A`[u,v]
  A[0]=A[1]=A[2]=0.;
	for(j=0; j<srf_dat->P.m-1; j++ ){
   	if( N_j1[j] !=0 ) {
		 	if( ( feet=srf_dat->V[j+srf_dat->degree_q+1] - srf_dat->V[j+1]) > eps_n ){
			  A_tmp_i[0]=A_tmp_i[1]=A_tmp_i[2]=0.;
				for(i=0; i<srf_dat->P.n; i++ ){
  				if( N_i[i] != 0 ){
				   	read_elem( &srf_dat->P.vdim, j*srf_dat->P.n+i, &node );
			 			Create_3D_From_4D( &node, (sgFloat *)Pij, &wij);
		 				read_elem( &srf_dat->P.vdim, (j+1)*srf_dat->P.n+i,&node );
					 	Create_3D_From_4D( &node, (sgFloat *)Pij1, &wij1);
            w_s = wij1 - wij;
            for( k=0; k<3; k++ ) P_s[k]  = Pij1[k] - Pij[k];
					  A_tmp_jj[0]=A_tmp_jj[1]=A_tmp_jj[2]=0.;
						for(jj=0; jj<srf_dat->P.m; jj++) {
						  if( N_j[jj] != 0 ){
		  					A_tmp_ii[0]=A_tmp_ii[1]=A_tmp_ii[2]=0.;
								for(ii=0; ii<srf_dat->P.n; ii++ ){
	  							if( N_i[ii] != 0 ){
								   	read_elem( &srf_dat->P.vdim, jj*srf_dat->P.n+ii, &node );
			 							Create_3D_From_4D( &node, (sgFloat *)Pij, &wij);
						  	    for( k=0; k<3; k++ ) A_tmp_ii[k] += N_i[ii]*(Pij[k]*w_s+wij*P_s[k]);
                  }
					 	 		}
						    for( k=0; k<3; k++ ) A_tmp_jj[k] += N_j[jj]*A_tmp_ii[k];
			  		  }
						}
  					for( k=0; k<3; k++ ) A_tmp_i[k] += N_i[i]*A_tmp_jj[k];
					}
			  }
				for( k=0; k<3; k++ ) A[k] += N_j1[j]*A_tmp_i[k]/feet;
			}

    }
  }
//------------>>>>>>>>>> derivates
	for(i=0; i<3; i++) ppoint[i]=(srf_dat->degree_q*A[i]-w_der*point[i])/w_w;
}

/*******************************************************************************
*         
*/
static sgFloat calc_w_surf(lpSURF_DAT srf_dat, sgFloat *N_i, sgFloat *N_j ){
short  i, j;
sgFloat w_w, w_s;
W_NODE node;	

	w_w=0;
	for( j=0; j<srf_dat->P.m; j++ ){
  	if( N_j[j] != 0 ){
      w_s=0;
	    for( i=0; i<srf_dat->P.n; i++ ){
				if( N_i[i] != 0 ){
        	read_elem( &srf_dat->P.vdim, j*srf_dat->P.n+i, &node );
        	w_s += node.vertex[3]*N_i[i];
        }
      }
      w_w += w_s*N_j[j];
    }
	}
  return w_w;
}

/*******************************************************************************
*        
*/
static void	calc_w_point_surf( lpSURF_DAT srf_dat, sgFloat *point, sgFloat *N_i, sgFloat *N_j ){
short 	number, i, j, k;
sgFloat 	w;
W_NODE	node, P;

	memset( &node, 0, sizeof(W_NODE));
	for( number=0, j=0; j<srf_dat->P.m; j++, number +=srf_dat->P.n )
		if( fabs( N_j[j] ) > eps_n ){
     	for( i=0; i<srf_dat->P.n; i++ )
				if( fabs( N_i[i] )>eps_n ){
					read_elem(&srf_dat->P.vdim, number+i, &P);
     			for( k=0; k<4; k++ )	node.vertex[k] += P.vertex[k]*N_i[i]*N_j[j];
        }
    }
	Create_3D_From_4D( &node, point, &w);
}
