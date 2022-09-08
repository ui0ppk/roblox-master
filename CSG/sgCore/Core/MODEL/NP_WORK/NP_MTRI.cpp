#include "../../sg.h"

#define MAXINDV SIZE_VPAGE/sizeof(short)

static  short include_vertex(lpD_POINT p, short* indv, short *num_indv, lpVDIM coor);
static  BOOL check_zero_sq(lpNPTRP trp);

static bool         was_exception_on_import = false;

BOOL	begin_tri_biand(lpTRI_BIAND trb)
{
	init_vdim(&trb->vdtri,sizeof(NPTRI));
	init_vdim(&trb->coor,sizeof(D_POINT));
	trb->ident = -32767;
	trb->num_indv = 0;

	was_exception_on_import = false;

	if ( !np_init_list(&trb->list_str)) return FALSE;
	if ( 0 ==(trb->indv = (short*)SGMalloc(MAXINDV*sizeof(*trb->indv))) ) {
		np_end_of_put(&trb->list_str,NP_CANCEL,0,NULL); //  
		np_handler_err(NP_NO_HEAP);
		return FALSE;
	}
	return TRUE;
}



static  bool  is_triangles_near(lpD_POINT tr1_1, lpD_POINT tr1_2, lpD_POINT tr1_3,
								lpD_POINT tr2_1, lpD_POINT tr2_2, lpD_POINT tr2_3)
{
	D_POINT min_1,max_1, min_2, max_2;

	min_1.x = min(tr1_1->x, min(tr1_2->x,tr1_3->x))-0.01;
	min_1.y = min(tr1_1->y, min(tr1_2->y,tr1_3->y))-0.01;
	min_1.z = min(tr1_1->z, min(tr1_2->z,tr1_3->z))-0.01;

	max_1.x = max(tr1_1->x, max(tr1_2->x,tr1_3->x))+0.01;
	max_1.y = max(tr1_1->y, max(tr1_2->y,tr1_3->y))+0.01;
	max_1.z = max(tr1_1->z, max(tr1_2->z,tr1_3->z))+0.01;

	min_2.x = min(tr2_1->x, min(tr2_2->x,tr2_3->x))-0.01;
	min_2.y = min(tr2_1->y, min(tr2_2->y,tr2_3->y))-0.01;
	min_2.z = min(tr2_1->z, min(tr2_2->z,tr2_3->z))-0.01;

	max_2.x = max(tr2_1->x, max(tr2_2->x,tr2_3->x))+0.01;
	max_2.y = max(tr2_1->y, max(tr2_2->y,tr2_3->y))+0.01;
	max_2.z = max(tr2_1->z, max(tr2_2->z,tr2_3->z))+0.01;

	if (max_1.x<min_2.x) return false;
	if (max_1.y<min_2.y) return false;
	if (max_1.z<min_2.z) return false;

	if (max_2.x<min_1.x) return false;
	if (max_2.y<min_1.y) return false;
	if (max_2.z<min_1.z) return false;

	return true;
}

static  bool  is_triangles_on_one_plane(lpD_POINT tr1_1, lpD_POINT tr1_2, lpD_POINT tr1_3,
										lpD_POINT tr2_1, lpD_POINT tr2_2, lpD_POINT tr2_3)
{
	D_POINT norm;
	sgFloat  d_coef;
	if (!eq_plane(tr1_1,tr1_2,tr1_3,&norm,&d_coef))
		return false;
	if (dist_p_pl(tr2_1,&norm,d_coef)>0.00001)
		return false;
	if (dist_p_pl(tr2_2,&norm,d_coef)>0.00001)
		return false;
	if (dist_p_pl(tr2_3,&norm,d_coef)>0.00001)
		return false;
	return true;

}
static int   is_point_on_triangle_sides(lpD_POINT tr1, lpD_POINT tr2, lpD_POINT tr3,
										lpD_POINT pnt)
{
	D_POINT norm;
	sgFloat  d_coef;
	if (!eq_plane(tr1,tr2,tr3,&norm,&d_coef))
		return 0;
	if (dist_p_pl(pnt,&norm,d_coef)>0.00001)
		return 0;

	if (dpoint_eq(tr1,pnt,eps_d) || dpoint_eq(tr2,pnt,eps_d) || dpoint_eq(tr3,pnt,eps_d))
		return 0;

	D_POINT  vectors_to_vertexes[3];
	dpoint_sub(tr1, pnt, &(vectors_to_vertexes[0]));
	dpoint_sub(tr2, pnt, &(vectors_to_vertexes[1]));
	dpoint_sub(tr3, pnt, &(vectors_to_vertexes[2]));
	if (!dnormal_vector1(&(vectors_to_vertexes[0])))
		return 0;
	if (!dnormal_vector1(&(vectors_to_vertexes[1])))
		return 0;
	if (!dnormal_vector1(&(vectors_to_vertexes[2])))
		return 0;

	sgFloat cs = dskalar_product(&(vectors_to_vertexes[0]), &(vectors_to_vertexes[1]));
	if (cs <= 0.0000001 - 1.) 
		return 1;
	cs = dskalar_product(&(vectors_to_vertexes[1]), &(vectors_to_vertexes[2]));
	if (cs <= 0.0000001 - 1.) 
		return 2;
	cs = dskalar_product(&(vectors_to_vertexes[2]), &(vectors_to_vertexes[0]));
	if (cs <= 0.0000001 - 1.) 
		return 3;

	return 0;
}

void    correct_adding(lpTRI_BIAND trb, lpNPTRI tri)
{
	D_POINT ATrVert[3];
	D_POINT BTrVert[3];

	NPTRI   newTria;
	memset(&newTria,0, sizeof(NPTRI));

	read_elem(&(trb->coor),tri->v[0],&BTrVert[0]);
	read_elem(&(trb->coor),tri->v[1],&BTrVert[1]);
	read_elem(&(trb->coor),tri->v[2],&BTrVert[2]);

	if (dpoint_eq(&BTrVert[0],&BTrVert[1],eps_d) || 
		dpoint_eq(&BTrVert[1],&BTrVert[2],eps_d) || 
		dpoint_eq(&BTrVert[2],&BTrVert[0],eps_d))
				{
					return;
				}

	for (short i=0;i<trb->vdtri.num_elem;i++)
	{
		NPTRI   triA;
		read_elem(&(trb->vdtri),i,&triA);
		read_elem(&(trb->coor),triA.v[0],&ATrVert[0]);
		read_elem(&(trb->coor),triA.v[1],&ATrVert[1]);
		read_elem(&(trb->coor),triA.v[2],&ATrVert[2]);

		if (!is_triangles_near(&ATrVert[0],&ATrVert[1],&ATrVert[2], 
									&BTrVert[0],&BTrVert[1],&BTrVert[2]))
				continue;

			int old_on_new[3];
			old_on_new[0] = is_point_on_triangle_sides(&BTrVert[0], &BTrVert[1], &BTrVert[2], &ATrVert[0]);
			old_on_new[1] = is_point_on_triangle_sides(&BTrVert[0], &BTrVert[1], &BTrVert[2], &ATrVert[1]);
			old_on_new[2] = is_point_on_triangle_sides(&BTrVert[0], &BTrVert[1], &BTrVert[2], &ATrVert[2]);
			int new_on_old[3];
			new_on_old[0] = is_point_on_triangle_sides(&ATrVert[0], &ATrVert[1], &ATrVert[2], &BTrVert[0]);
			new_on_old[1] = is_point_on_triangle_sides(&ATrVert[0], &ATrVert[1], &ATrVert[2], &BTrVert[1]);
			new_on_old[2] = is_point_on_triangle_sides(&ATrVert[0], &ATrVert[1], &ATrVert[2], &BTrVert[2]);
			BOOL boolChecks[9];
			boolChecks[0] = dpoint_eq(&BTrVert[0],&ATrVert[0],eps_d);
			boolChecks[1] = dpoint_eq(&BTrVert[0],&ATrVert[1],eps_d);
			boolChecks[2] = dpoint_eq(&BTrVert[0],&ATrVert[2],eps_d);
			boolChecks[3] = dpoint_eq(&BTrVert[1],&ATrVert[0],eps_d);
			boolChecks[4] = dpoint_eq(&BTrVert[1],&ATrVert[1],eps_d);
			boolChecks[5] = dpoint_eq(&BTrVert[1],&ATrVert[2],eps_d);
			boolChecks[6] = dpoint_eq(&BTrVert[2],&ATrVert[0],eps_d);
			boolChecks[7] = dpoint_eq(&BTrVert[2],&ATrVert[1],eps_d);
			boolChecks[8] = dpoint_eq(&BTrVert[2],&ATrVert[2],eps_d);
			bool on_one_plane = is_triangles_on_one_plane(&ATrVert[0],&ATrVert[1],&ATrVert[2], 
													&BTrVert[0],&BTrVert[1],&BTrVert[2]);
			short trueCnt = 0;
			for (short bCnt = 0; bCnt<9; bCnt++)
				if (boolChecks[bCnt])
					trueCnt++;
			short resCnt_old_on_new = 0;
			short resCnt_new_on_old = 0;
			for (short rCnt = 0; rCnt<3; rCnt++)
			{
				if (old_on_new[rCnt])
					resCnt_old_on_new++;
				if (new_on_old[rCnt])
					resCnt_new_on_old++;
			}

			if (trueCnt>2) return;
			if (trueCnt>1 && !on_one_plane)
			{
				add_elem(&(trb->vdtri),tri);
				return;
			}
			if (trueCnt>1 && on_one_plane)
			{
				if (resCnt_new_on_old>0) return;
				if (resCnt_old_on_new>0)
				{
					write_elem(&(trb->vdtri),i,tri);
					return;
				}
				add_elem(&(trb->vdtri),tri);
				return;
			}
			if (trueCnt==1 && resCnt_new_on_old>1)
				return;
			if (trueCnt==1 && resCnt_old_on_new>1)
			{
				write_elem(&(trb->vdtri),i,tri);
				return;
			}
			/*if (trueCnt==1)
			{
				add_elem(&(trb->vdtri),tri);
				return;
			}*/

			if (resCnt_old_on_new==0 && resCnt_new_on_old==0)
			{
				//add_elem(&(trb->vdtri),tri);
				//return;
				continue;
			}

			for (short vI = 0;vI<3;vI++)
			{
				//   
				if (resCnt_old_on_new>=1 /*&&
					(resCnt_new_on_old==0 || resCnt_new_on_old==1)*/)
				{
					if (old_on_new[vI]==1) // º 0  1
						{
							memset(&newTria,0, sizeof(NPTRI));
							newTria.v[0] = tri->v[0];
							newTria.v[1] = triA.v[vI];
							newTria.v[2] = tri->v[2];
							//add_elem(&(trb->vdtri),&newTria);
							if (newTria.v[0]==1193 &&
								newTria.v[1]==1199 &&
								newTria.v[2]==1212)
							{
								int aaa=0;
								aaa++;
							}
							correct_adding(trb,&newTria);
							newTria.v[0] = triA.v[vI];
							newTria.v[1] = tri->v[1];
							newTria.v[2] = tri->v[2];
							//add_elem(&(trb->vdtri),&newTria);
							correct_adding(trb,&newTria);
							return;
						}
						else
							if (old_on_new[vI]==2) // º 1  2
							{
								memset(&newTria,0, sizeof(NPTRI));
								newTria.v[0] = tri->v[0];
								newTria.v[1] = tri->v[1];
								newTria.v[2] = triA.v[vI];
								//add_elem(&(trb->vdtri),&newTria);
								correct_adding(trb,&newTria);
								newTria.v[0] = tri->v[0];
								newTria.v[1] = triA.v[vI];
								newTria.v[2] = tri->v[2];
								//add_elem(&(trb->vdtri),&newTria);
								correct_adding(trb,&newTria);
								return;
							}
							else
								if (old_on_new[vI]==3) // º 2  3
								{
									memset(&newTria,0, sizeof(NPTRI));
									newTria.v[0] = tri->v[1];
									newTria.v[1] = tri->v[2];
									newTria.v[2] = triA.v[vI];
									//add_elem(&(trb->vdtri),&newTria);
									correct_adding(trb,&newTria);
									newTria.v[0] = tri->v[1];
									newTria.v[1] = triA.v[vI];
									newTria.v[2] = tri->v[0];
									//add_elem(&(trb->vdtri),&newTria);
									correct_adding(trb,&newTria);
									return;
								}
				}

				
				//   
				if (resCnt_new_on_old>=1/* &&
					(resCnt_old_on_new==0 ||resCnt_old_on_new==1)*/)
				{
					if (new_on_old[vI]==1) // º 0  1
					{
						memset(&newTria,0, sizeof(NPTRI));
						newTria.v[0] = triA.v[0];
						newTria.v[1] = tri->v[vI];
						newTria.v[2] = triA.v[2];
						write_elem(&(trb->vdtri),i,&newTria);
						newTria.v[0] = tri->v[vI];
						newTria.v[1] = triA.v[1];
						newTria.v[2] = triA.v[2];
						//add_elem(&(trb->vdtri),&newTria);
						correct_adding(trb,&newTria);
						//add_elem(&(trb->vdtri),tri);
						correct_adding(trb,tri);
						return;
					}
					else
						if (new_on_old[vI]==2) // º 1  2
						{
							memset(&newTria,0, sizeof(NPTRI));
							newTria.v[0] = triA.v[1];
							newTria.v[1] = tri->v[vI];
							newTria.v[2] = triA.v[0];
							write_elem(&(trb->vdtri),i,&newTria);
							newTria.v[0] = tri->v[vI];
							newTria.v[1] = triA.v[2];
							newTria.v[2] = triA.v[0];
							//add_elem(&(trb->vdtri),&newTria);
							correct_adding(trb,&newTria);
							//add_elem(&(trb->vdtri),tri);
							correct_adding(trb,tri);
							return;
						}
						else
							if (new_on_old[vI]==3) // º 2  3
							{
								memset(&newTria,0, sizeof(NPTRI));
								newTria.v[0] = triA.v[2];
								newTria.v[1] = tri->v[vI];
								newTria.v[2] = triA.v[1];
								write_elem(&(trb->vdtri),i,&newTria);
								newTria.v[0] = tri->v[vI];
								newTria.v[1] = triA.v[0];
								newTria.v[2] = triA.v[1];
								//add_elem(&(trb->vdtri),&newTria);
								correct_adding(trb,&newTria);
								//add_elem(&(trb->vdtri),tri);
								correct_adding(trb,tri);
								return;
							}
				}

				
			}
			
	}

	add_elem(&(trb->vdtri),tri);
}

BOOL	put_tri(lpTRI_BIAND trb, lpNPTRP trp, bool intellect_adding/*=false*/)
{
	int		j;
	NPTRI	tri;

	tri.constr = trp->constr;
	if ( trb->num_indv >= (MAXINDV-3)) {
		if ( !np_mesh3v(&trb->list_str, trp, &trb->ident, &trb->coor, c_smooth_angle,
										&trb->vdtri))		return FALSE;
		trb->num_indv = 0;
		delete_elem_list(&trb->coor,0);
		delete_elem_list(&trb->vdtri,0);
//		free_vdim(&trb->coor);
//		free_vdim(&trb->vdtri);
//		init_vdim(&trb->vdtri,sizeof(NPTRI));
//		init_vdim(&trb->coor,sizeof(D_POINT));
	}
	for ( j=0; j<3; j++ ) {
		tri.v[j] = include_vertex(&trp->v[j],trb->indv, &trb->num_indv, &trb->coor);
	}
	if ( tri.v[0] ==  tri.v[1] ||  tri.v[0] ==  tri.v[2] ||
			 tri.v[1] ==  tri.v[2] ) return TRUE;

//   0- 
	if ( check_zero_sq(trp) ) return TRUE;

	if (was_exception_on_import || !intellect_adding)
		add_elem(&trb->vdtri,&tri);
	else
	{
		try
		{
			correct_adding(trb, &tri);
		}
		catch(...)
		{
			was_exception_on_import = true;
		}
	}

	//if ( !add_elem(&trb->vdtri,&tri)) return FALSE;
	return TRUE;
}
BOOL	end_tri_biand(lpTRI_BIAND trb, lpLISTH listho, lpNPTRP trp)
 {
	BOOL	cod;
	hOBJ	hobj;

	if ( trb->num_indv <= 0 ) {
		free_vdim(&trb->vdtri);
		free_vdim(&trb->coor);
		if ( trb->indv ) SGFree(trb->indv);
		np_end_of_put(&trb->list_str,NP_CANCEL,0,NULL); //  
		trb->indv = NULL;
		return TRUE;
	}
	cod = np_mesh3v(&trb->list_str, trp, &trb->ident, &trb->coor, c_smooth_angle,
									&trb->vdtri);
	free_vdim(&trb->vdtri);
	free_vdim(&trb->coor);
	if ( trb->indv ) SGFree(trb->indv);
	if ( !cod ) goto err;
	trb->list_str.flag = 1;
	cod = cinema_end(&trb->list_str,&hobj);
	if ( hobj ) 	attach_item_tail(listho,hobj);
	return cod;
err:
	np_end_of_put(&trb->list_str,NP_CANCEL,0,NULL); //  
	return FALSE;
}

static  short include_vertex(lpD_POINT p, short* indv, short *num_indv,
																lpVDIM coor)
{
	register short 	k, ib, ie, j, jj;
	D_POINT				pc;

	jj	= 1;
	j		= -1;
	ib	= 0;													//    
	ie	= *num_indv-1;								//    
	while (1) {
		if (ib > ie) break;							//  
		j = (ib + ie) >> 1;							//   
		k = indv[MAXINDV - j - 1];			//   
		read_elem(coor,k,&pc);
		if (fabs(pc.x - p->x) < eps_d) {
			p->x = pc.x;
			if (fabs(pc.y - p->y) < eps_d) {
				p->y = pc.y;
				if (fabs(pc.z - p->z) < eps_d) {
					p->z = pc.z;
					return k;
				}	else {
					if (p->z < pc.z)	jj = -1;
					else							jj =  1;
				}
			} else {
				if (p->y < pc.y)	jj = -1;
				else							jj =  1;
			}
		} else {
			if (p->x < pc.x)	jj = -1;
			else							jj =  1;
		}
		if (jj < 0) ie = j - 1;
		else        ib = j + 1;
	}
	if (jj > 0) j++;
	memcpy(&indv[MAXINDV - *num_indv - 1],&indv[MAXINDV - *num_indv],
			sizeof(short) * (*num_indv - j + 1));	//   j...now 
	indv[MAXINDV - j - 1] = *num_indv;	// j-      num_indv
	if ( !add_elem(coor,p) ) return -1;
	(*num_indv)++;
	return (*num_indv-1);
}
/*-----------------09.01.97 13:06-------------------
    0- 
--------------------------------------------------*/
static  BOOL check_zero_sq(lpNPTRP trp)
{
	D_POINT pl,cl/*,cl2*/;
	sgFloat	d = 1./pow(10.,export_floating_precision); //  EVA 7.12.01

	/*dpoint_sub(&trp->v[1],&trp->v[0],&cl);
	dpoint_sub(&trp->v[2],&trp->v[0],&cl2);
	if (!dnormal_vector (&cl))
	{
		return TRUE;
	}
	if (!dnormal_vector (&cl2))
	{
		return TRUE;
	}

	sgFloat cs = dskalar_product(&cl, &cl2);
	if (fabs(cs) >= 0.99999) 
		return TRUE;

	dpoint_sub(&trp->v[0],&trp->v[1],&cl);
	dpoint_sub(&trp->v[2],&trp->v[1],&cl2);
	if (!dnormal_vector (&cl))
	{
		return TRUE;
	}
	if (!dnormal_vector (&cl2))
	{
		return TRUE;
	}

	cs = dskalar_product(&cl, &cl2);
	if (fabs(cs) >= 0.99999) 
		return TRUE;

	return FALSE;
	*/


	dpoint_sub(&trp->v[1],&trp->v[0],&cl);
	if (!dnormal_vector (&cl))
	{
		return TRUE;
	}
	if ( dist_p_l(&trp->v[0], &cl, &trp->v[2], &pl) < d)	goto md;

	dpoint_sub(&trp->v[2],&trp->v[1],&cl);
	if (!dnormal_vector (&cl))
	{
		return TRUE;
	}
	if ( dist_p_l(&trp->v[1], &cl, &trp->v[0], &pl) < d)	goto md;

	dpoint_sub(&trp->v[0],&trp->v[2],&cl);
	if (!dnormal_vector (&cl))
	{
		return TRUE;
	}
	if ( dist_p_l(&trp->v[2], &cl, &trp->v[1], &pl) < d)	goto md;
	return FALSE;
md:
	return TRUE;
}





