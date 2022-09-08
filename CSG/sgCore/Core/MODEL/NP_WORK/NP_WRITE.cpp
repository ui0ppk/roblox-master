#include "../../sg.h"

static BOOL np_str_form_precision(BOOL (*user_gab_inter)(lpREGION_3D g1,lpREGION_3D g2),
		 lpNP_STR_LIST list1, lpNP_STR_LIST list2, sgFloat precision);

static void str_compress(lpNP_STR_LIST list);
static void calk_volume(lpNPW np, sgFloat *volume);
static BOOL search_ort(lpNP_STR_LIST list, lpNPW np, lpNP_STR str);

void np_zero_list(lpNP_STR_LIST list)
{
	memset(list,0,sizeof(NP_STR_LIST));
//	list->bnp.bd.handle = -1;
	list->bnp.bd.handle = NULL;
}
BOOL np_init_list(lpNP_STR_LIST list)
{
  np_zero_list(list);
	if ( !begin_rw_np(&list->bnp) ) return FALSE;
  init_vdim(&list->vdim,sizeof(NP_STR) );
  init_vdim(&list->nb,sizeof(int) );
  return TRUE;
}

BOOL np_put(lpNPW np, lpNP_STR_LIST list)
{
	NP_STR  str;
	VNP     vnp;

	dpoint_minmax(&np->v[1], np->nov, &np->gab.min, &np->gab.max);

	memset(&str,0,sizeof(str));
	str.ident = np->ident;
	str.gab = np->gab;
	if ( !(vnp = write_np(&list->bnp,np,(NPTYPE)np->type)) )	return FALSE;
	str.hnp = vnp;
	if ( !add_elem(&list->vdim,&str) ) return FALSE;
	list->number_all++;
	return TRUE;
}

BOOL np_get_str(lpNP_STR_LIST list, short ident, lpNP_STR str1, short *index)
{
	register int 	i;
  lpNP_STR 			str;

  begin_rw(&list->vdim,0);
  for (i=0; i<list->number_all; i++) {
    str = (lpNP_STR)get_next_elem(&list->vdim);
    if ( str->ident == ident ) {
			*str1 = *str;
			if ( index ) *index = i;
      break;
    }
	}
  end_rw(&list->vdim);
	if ( i == list->number_all ) {
    np_handler_err(NP_NO_IDENT);
    return FALSE;
	}
  return TRUE;
}

BOOL np_str_form(BOOL (*user_gab_inter)(lpREGION_3D g1,lpREGION_3D g2),
		 lpNP_STR_LIST list1, lpNP_STR_LIST list2)
{
	BOOL       rt = TRUE;
  lpNP_STR   str2;
  NP_STR     str1;
  int        i, j;
	char       flg_one;

	if ( list1 == list2 ) flg_one = 1;
	else                  flg_one = 0;

//	num_proc = start_grad(GetIDS(IDS_SG316), list1->number_all);

	for (i=0; i<list1->number_all; i++) {  //    
		if (ctrl_c_press) {												 //   Ctrl/C
//			put_message(CTRL_C_PRESS, NULL, 0);
//			end_grad  (num_proc , i);
			return OSBREAK;
		}
//		step_grad (num_proc , i);
		read_elem(&list1->vdim,i,&str1);
		if (str1.lab != 0) continue;
		str1.b = list1->nb.num_elem;              //  
		str1.len = 0;
		begin_rw(&list2->vdim,0);
		for (j=0; j<list2->number_all; j++) { //    
			str2 = (lpNP_STR)get_next_elem(&list2->vdim);
			if (str2->lab != 0) continue;
			if ( i == j && flg_one ) continue;
			if ((user_gab_inter(&str1.gab,&str2->gab)) == FALSE)
				add_elem(&list1->nb, &j);
		}
		end_rw(&list2->vdim);

		str1.len = (short)(list1->nb.num_elem - str1.b);
		if ( str1.len ) write_elem(&list1->vdim,i,&str1);
	}
//	end_grad  (num_proc , i);
	return rt;
}

BOOL np_str_form_precision(BOOL (*user_gab_inter)(lpREGION_3D g1,lpREGION_3D g2),
		 lpNP_STR_LIST list1, lpNP_STR_LIST list2, sgFloat precision)
{
	BOOL       rt = TRUE;
	lpNP_STR   str2;
	NP_STR     str1;
	int        i, j;
	char       flg_one;
	REGION_3D  g1, g2;

	if ( list1 == list2 ) flg_one = 1;
	else                  flg_one = 0;

//	num_proc = start_grad(GetIDS(IDS_SG316), list1->number_all);

	for (i=0; i<list1->number_all; i++) {  //    
		if (ctrl_c_press) {												 //   Ctrl/C
//			put_message(CTRL_C_PRESS, NULL, 0);
//			end_grad  (num_proc , i);
			return OSBREAK;
		}
//		step_grad (num_proc , i);
		read_elem(&list1->vdim,i,&str1);
		if (str1.lab != 0) continue;
		str1.b = list1->nb.num_elem;              //  
		str1.len = 0;
		begin_rw(&list2->vdim,0);
		for (j=0; j<list2->number_all; j++) { //    
			str2 = (lpNP_STR)get_next_elem(&list2->vdim);
			if (str2->lab != 0) continue;
			if ( i == j && flg_one ) continue;
			g1 = str1.gab;
			g1.min.x -= precision;
			g1.min.y -= precision;
			g1.min.z -= precision;
			g1.max.x += precision;
			g1.max.y += precision;
			g1.max.z += precision;
			g2 = str2->gab;
			g2.min.x -= precision;
			g2.min.y -= precision;
			g2.min.z -= precision;
			g2.max.x += precision;
			g2.max.y += precision;
			g2.max.z += precision;
			if ((user_gab_inter(&g1,&g2)) == FALSE)
				add_elem(&list1->nb, &j);
		}
		end_rw(&list2->vdim);

		str1.len = (short)(list1->nb.num_elem - str1.b);
		if ( str1.len ) write_elem(&list1->vdim,i,&str1);
	}
//end:
//	end_grad  (num_proc , i);
	return rt;
}
BOOL create_hobj_list(lpNP_STR_LIST list, lpLISTH list_brep)
{
	if ( np_end_of_put(list, NP_ALL, (float)c_smooth_angle, list_brep) ) return TRUE;
	return FALSE;
}
short create_np_list(lpNP_STR_LIST list, float angle, lpLISTH list_np)
{
	return np_end_of_put(list, NP_FIRST, angle, list_np);
}

short np_end_of_put(lpNP_STR_LIST list, REGIMTYPE regim, float angle,
									lpLISTH list_brep)
{
	short       v1, v2, v, w1, w2;
	short       i, ii, jj, bg, fi;
	short       face, fs, edge;
	int         number = 0, number_old, number_obj = 0, istr;
	short       ident_np;
	sgFloat      volume;
	float       gu;
	lpNPW       np1,np2;
	lpNP_STR    str_t1;
	NP_STR      str_t,str;
	BREPTYPE    type;
	int *    b = NULL;
	hOBJ        hbrep = NULL;
	lpOBJ       brep = NULL;
	lpGEO_BREP  lpgeobrep = NULL;
	VLD         vld;
	VI_LOCATION viloc,viloctmp;
	REGION_3D   g = {{1.e35,1.e35,1.e35},{-1.e35,-1.e35,-1.e35}};
	BOOL        cod;
  LNP					lnp;
	int	        num_proc = -1, ni = 0;

	if (regim == NP_CANCEL) goto cancel;        //  
	init_vld(&vld);

	//  
	if (!(np_str_form(np_gab_inter,list,list))) goto err;

	np1 = npwg;
	if ( (np2 = creat_np_mem(TNPW,MAXNOV,MAXNOE,MAXNOF,MAXNOC,MAXNOE)) == NULL )
		goto err;

//	num_proc = start_grad(GetIDS(IDS_SG317), list->number_all);

	init_listh(list_brep);        //    
	do {
		list->number_np = list->number_all;
		begin_rw(&list->vdim,0);
		fi = 0;
		while (1) {
			str_t1 = (lpNP_STR)get_next_elem(&list->vdim);
			if ( !str_t1->ort ) break;
			fi++;
		}
		str_t1->ort = 1;                  //    
		end_rw(&list->vdim);

		number_old = number = 0;
		ident_np = -32767;
		number = 0;
		volume = 0.;
		type = BODY;
		init_vld(&vld);

		do {                                    	//   
			number_old = number;
			number = 0;

			for (istr=fi; istr<list->number_all; istr++) {
				read_elem(&list->vdim,istr,&str);
				if (str.lab == 2)  continue;          // 
				if (str.ort == 0 ) {               		//    
					if ( !read_np(&list->bnp,str.hnp,np1) ) goto err;
					if ( !search_ort(list, np1, &str) ) goto err;
					if (str.ort == 0)		continue;       //   
					number++;                           //  
				} else {                              //   
					number++;
        	if (str.lab == 1)  continue;        //   
        	str.lab = 1;
					if ( !(read_np(&list->bnp,str.hnp,np1)) ) goto err;
				}
				if (str.len == 0) {                    //    
					for (i=1 ; i <= np1->noe ; i++) {
						if (np1->efc[i].fp == 0 || np1->efc[i].fm == 0) {
							type = SURF;                  		//   
							np1->efr[i].el = ST_TD_CONSTR;
						}
					}
					if (regim == NP_ALL) {
						if (type == BODY) calk_volume(np1,&volume);
						np_gab_un(&g, &np1->gab, &g);
					}
					if (ident_np <= np1->ident)   ident_np = np1->ident;

					if ( list->flag )	np_approch_to_constr(np1,angle);
//    			step_grad (num_proc , ni++);
					add_np_mem(&vld,np1,TNPB);
        	str.lab = 1;
          write_elem(&list->vdim,istr,&str);
					continue;
				}
				bg = 1;
				begin_rw(&list->nb, str.b);
				for (i=0 ; i < str.len ; i++) {
					b = (int*)get_next_elem(&list->nb);
					while ( bg <= np1->noe &&
									np1->efc[bg].fm != 0 && np1->efc[bg].fp != 0 )
						bg++;                     //   
					if (bg > np1->noe) break;            //   
					read_elem(&list->vdim,*b,&str_t);
					if (str_t.lab == 1 || str_t.lab == 2) {
						continue;
					}
					if (!(read_np(&list->bnp,str_t.hnp,np2))) goto err;

					for (ii=bg ; ii <= np1->noe ; ii++) {
						if (np1->efc[ii].fm != 0 && np1->efc[ii].fp != 0) continue;
						v1 = np1->efr[ii].bv;
						v2 = np1->efr[ii].ev;
						for (jj=1 ; jj <= np2->noe ; jj++) {
							if (np2->efc[jj].fm != 0 && np2->efc[jj].fp != 0) continue;
							w1 = np2->efr[jj].bv;
							w2 = np2->efr[jj].ev;
							if (!(dpoint_eq(&np1->v[v1],&np2->v[w1],eps_d) &&
										dpoint_eq(&np1->v[v2],&np2->v[w2],eps_d) ||
										dpoint_eq(&np1->v[v2],&np2->v[w1],eps_d) &&
										dpoint_eq(&np1->v[v1],&np2->v[w2],eps_d))) continue;
							if (np2->efc[jj].fp == 0) {
								face = np2->efc[jj].fm;
//nb								w  = w1;
								w1 = w2;
//nb								w2 = w;
							} else {
								face = np2->efc[jj].fp;
							}
							if (np1->efc[ii].fp == 0) {
/*								v  = v1;	v1 = v2;	v2 = v;    */
                v = v2;
//nb  	          v2 = v1;
                v1 = v;
								edge = -ii;
								fs = np1->efc[ii].fm;
							} else {
								edge = ii;
								fs = np1->efc[ii].fp;
							}
							if (str_t.ort == 0) {           //   
								if (dpoint_eq(&np1->v[v1],&np2->v[w1],eps_d)) {
									np_inv(np2,NP_ALL);
									str_t.ort = -1;
								} else str_t.ort = 1;
							}
							if ((gu=np_gru1(np1,edge,&np1->p[fs].v,&np2->p[face].v)) == 0) {
								np1->efr[ii].el = ST_TD_CONSTR;
								np2->efr[jj].el = ST_TD_CONSTR;
								if (np1->efc[ii].fp == 0) np1->efc[ii].ep = 0;
								else    									np1->efc[ii].em = 0;
								if (np2->efc[jj].fp == 0) np2->efc[jj].ep = 0;
								else    									np2->efc[jj].em = 0;
								np1->gru[ii] = np2->gru[jj] = 0.;
							} else {
								if (np1->efc[ii].fp == 0) {
									np1->efc[ii].fp = -ii;
									np1->efc[ii].ep = np2->ident;
								} else {
									np1->efc[ii].fm = -ii;
									np1->efc[ii].em = np2->ident;
								}
								np1->gru[ii] = gu;
								if (np2->efc[jj].fp == 0) {
									np2->efc[jj].fp = -jj;
									np2->efc[jj].ep = np1->ident;
								} else {
									np2->efc[jj].fm = -jj;
									np2->efc[jj].em = np1->ident;
								}
								np2->gru[jj] = gu;
								if (np1->efr[ii].el == 0 || np2->efr[jj].el == 0) {
									//   
									if (M_PI - angle <= gu && gu <= M_PI + angle) {
										np1->efr[ii].el &= (~ST_TD_CONSTR);
										np1->efr[ii].el |= ST_TD_APPROCH;
										np2->efr[jj].el &= (~ST_TD_CONSTR);
										np2->efr[jj].el |= ST_TD_APPROCH;
									} else {
										np1->efr[ii].el = ST_TD_CONSTR;
										np2->efr[jj].el = ST_TD_CONSTR;
									}
								}
							}
							break;
						}
					}
					for (ii=1 ; ii <= np2->noe ; ii++) {
						if (np2->efc[ii].fp == 0 || np2->efc[ii].fm == 0) {
							//   
							break;
						}
					}
          if (ii <= np2->noe || str_t.ort == 0) {
						if ( !(overwrite_np(&list->bnp,str_t.hnp,np2,TNPW)) ) goto err;
						write_elem(&list->vdim,*b,&str_t);
						continue;
					}
					if (regim == NP_ALL) {            //   
						if (type == BODY)  calk_volume(np2,&volume);
						np_gab_un(&g, &np2->gab, &g);
					}
					if (ident_np <= np2->ident)   ident_np = np2->ident;

					if ( list->flag )	np_approch_to_constr(np2,angle);
//    			step_grad (num_proc , ni++);
					add_np_mem(&vld,np2,TNPB);
          str_t.lab = 1;                   //   
//					number++;
					write_elem(&list->vdim,*b,&str_t);
				}
				end_rw(&list->nb);   b = NULL;
				for (i=1 ; i <= np1->noe ; i++) {
					if (np1->efc[i].fp == 0 || np1->efc[i].fm == 0) {
						type = SURF;
						np1->efr[i].el = ST_TD_CONSTR;  //    
					}
				}
				if (regim == NP_ALL) {
					if (type == BODY) calk_volume(np1,&volume);
					np_gab_un(&g, &np1->gab, &g);
				}
				if (ident_np <= np1->ident)            ident_np = np1->ident;

				if ( list->flag )	np_approch_to_constr(np1,angle);
//    		step_grad (num_proc , ni++);
				add_np_mem(&vld,np1,TNPB);
				str.lab = 1;                      //   
				write_elem(&list->vdim,istr,&str);
			}
		} while (number != list->number_np && number != number_old);

		if (regim == NP_FIRST) {
			np_free_stack();
			np_str_free(list);
			free_np_mem(&np2);
			*list_brep = vld.listh;
//  		end_grad  (num_proc , ni);
			return number;
		}
		if (type == BODY && volume < 0.) {
      get_first_np_loc( &vld.listh, &viloc);
			for ( i=0; i<number; i++ ) {
				begin_read_vld(&viloc);
				cod = read_np_mem(np1);
				get_read_loc(&viloctmp);
				end_read_vld();
        if ( !cod ) goto err;
				np_inv((lpNPW)np1,NP_ALL);           //  
				rezet_np_mem(&viloc,np1,TNPB);
				viloc = viloctmp;
			}
		}
		if ( ( hbrep = o_alloc(OBREP)) == NULL)	goto err;
		brep = (lpOBJ)hbrep;
		lpgeobrep = (lpGEO_BREP)brep->geo_data;
		o_hcunit(lpgeobrep->matr);
		lpgeobrep->min = g.min;
		lpgeobrep->max = g.max;
		lpgeobrep->ident_np = ident_np + 1;
		lpgeobrep->type = type;
		memset(&lnp,0,sizeof(lnp));
		lnp.kind = COMMON;
		lnp.listh = vld.listh;
		lnp.num_np = number;
		if ( !put_np_brep(&lnp,&lpgeobrep->num)) goto err;
		attach_item_tail(list_brep, hbrep);
		number_obj++;
		hbrep = NULL;

	  str_compress(list);
	} while (list->number_np != 0);    //  
	number = number_obj;
	free_np_mem(&np2);
	goto cancel;

err:
	number = 0;
	free_np_mem(&np2);
	free_vld_data(&vld);
	if (b != NULL)    end_rw(&list->nb);
	if (hbrep != NULL)      o_free(hbrep,NULL);
cancel:
//  end_grad  (num_proc , ni);
	np_free_stack();
	np_str_free(list);
	return number;
}

int np_end_of_put_1(lpNP_STR_LIST list, REGIMTYPE regim, float angle,
									lpLISTH list_brep, sgFloat precision)
{
	short         v1, v2, v, w1, w2;
	int         i, ii, jj, bg, fi, istr;
	short         face, fs, edge, edge_i, edge_j;
	int         number = 0, number_old, ident_np, number_obj = 0;
	sgFloat      volume, d1, d2, d;
	float       gu;
	lpNPW       np1,np2;
	lpNP_STR    str_t1;
	NP_STR      str_t,str;
	BREPTYPE    type;
	long *		  b      = NULL;
	hOBJ        hbrep  = NULL;
	lpOBJ       brep   = NULL;
	lpGEO_BREP  lpgeobrep = NULL;
	VLD         vld;
	VI_LOCATION viloc,viloctmp;
	REGION_3D   g; // = {{1.e35,1.e35,1.e35},{-1.e35,-1.e35,-1.e35}};
	BOOL        cod, flg_j;
	LNP					lnp;
	int	        num_proc = -1, ni = 0;
	short				edge_lable = 0;
	D_POINT			ln, ln2, p1, p2, pp, wv1, wv2, pw1, pw2;

	g.min.x = g.min.y = g.min.z =  1.e35;
	g.max.x = g.max.y = g.max.z = -1.e35;

	edge_lable |= ST_TD_CONSTR;
	edge_lable |= ST_TD_APPROCH;
	edge_lable |= ST_TD_DUMMY;
	edge_lable |= ST_TD_COND;

	if (regim == NP_CANCEL) goto cancel;        //  
	init_vld(&vld);

	//  
	if (!(np_str_form_precision(np_gab_inter, list, list, precision))) goto err;

	np1 = npwg;
	if ( (np2 = creat_np_mem(TNPW,MAXNOV,MAXNOE,MAXNOF,MAXNOC,MAXNOE)) == NULL )
		goto err;

//	num_proc = start_grad(GetIDS(IDS_SG317), list->number_all);

	init_listh(list_brep);        //    
	do {
		list->number_np = list->number_all;
		begin_rw(&list->vdim,0);
		fi = 0;
		while (1) {         //      
			str_t1 = (lpNP_STR)get_next_elem(&list->vdim);
			if ( !str_t1->ort && str_t1->lab != 2 ) break;
			fi++;
		}
		str_t1->ort = 1;                  //    
		end_rw(&list->vdim);

		number_old = number = 0;
		ident_np = -32767;
		number = 0;
		volume = 0.;
		type = BODY;
		init_vld(&vld);

		do {                                    //   
			number_old = number;
			number = 0;
			if (ctrl_c_press) {												 //   Ctrl/C
	//			put_message(CTRL_C_PRESS, NULL, 0);
				goto err;
			}
			for (istr=fi; istr<list->number_all; istr++) {
				read_elem(&list->vdim,istr,&str);
				if (str.lab == 2)  continue;                     // 
				if (str.ort == 0 ) {               //    
					if ( !read_np(&list->bnp,str.hnp,np1) ) goto err;
					if ( !search_ort(list, np1, &str) ) goto err;
					if (str.ort == 0)		continue;        //   
					number++;                               //  
				} else {                              //   
					number++;
					if (str.lab == 1)  continue;           //   
					str.lab = 1;
					if ( !(read_np(&list->bnp,str.hnp,np1)) ) goto err;
				}
				if (str.len == 0) {                    //    
					for (i=1 ; i <= np1->noe ; i++) {
						if (np1->efc[i].fp == 0 || np1->efc[i].fm == 0) {
							type = SURF;                  //   
							np1->efr[i].el = ST_TD_CONSTR;
						}
					}
					if (regim == NP_ALL) {
						if (type == BODY) calk_volume(np1,&volume);
						np_gab_un(&g, &np1->gab, &g);
					}
					if (ident_np <= np1->ident)   ident_np = np1->ident;
					if ( list->flag )	np_approch_to_constr(np1,angle);
//					step_grad (num_proc , ni++);
					add_np_mem(&vld,np1,TNPB);
					str.lab = 1;
					write_elem(&list->vdim,istr,&str);
					continue;
				}
				bg = 1;
				begin_rw(&list->nb, str.b);
				for (i=0 ; i < str.len ; i++) {
					b = (long*)get_next_elem(&list->nb);
					while ( bg <= np1->noe &&
									np1->efc[bg].fm != 0 && np1->efc[bg].fp != 0 )
						bg++;                     //   
					if (bg > np1->noe) break;            //   
					read_elem(&list->vdim,*b,&str_t);
					if (str_t.lab == 1 || str_t.lab == 2) {
						continue;
					}
					if (!(read_np(&list->bnp,str_t.hnp,np2))) goto err;

					for (ii=bg ; ii <= np1->noe ; ii++) {
						if (np1->efc[ii].fm != 0 && np1->efc[ii].fp != 0) continue;
						v1 = np1->efr[ii].bv;
						v2 = np1->efr[ii].ev;
						p1 = np1->v[v1];
						p2 = np1->v[v2];
						dpoint_sub(&p2, &p1, &ln);
						dnormal_vector(&ln);
						for (jj=1 ; jj <= np2->noe ; jj++) {
							if (np2->efc[jj].fm != 0 && np2->efc[jj].fp != 0) continue;
							w1 = np2->efr[jj].bv;
							w2 = np2->efr[jj].ev;
							if ( (d1 = dist_p_l(&p1, &ln, &np2->v[w1], &pw1)) > precision) continue;
							if ( (d2 = dist_p_l(&p1, &ln, &np2->v[w2], &pw2)) > precision) continue;
							d1 = get_point_par_3d(&p1, &p2, &pw1);
							d2 = get_point_par_3d(&p1, &p2, &pw2);
//							d1 = get_point_par_3d(&p1, &p2, &np2->v[w1]);
//							d2 = get_point_par_3d(&p1, &p2, &np2->v[w2]);

							edge_i = ii;
							edge_j = jj;
							if ( d1 < d2 )	{  	//   
								wv1 = np2->v[w1];
								wv2 = np2->v[w2];
								flg_j = TRUE;
							} else {       			//   
								wv1 = np2->v[w2];
								wv2 = np2->v[w1];
								flg_j = FALSE;
								d = d1; d1 = d2; d2 = d;
								pp = pw1; pw1 = pw2; pw2 = pp;
							}
							if ( d2 <= 0. || d1 >= 1. ) continue;	//   

							dpoint_sub(&wv2, &wv1, &ln2);
							dnormal_vector(&ln2);
							if ( !(dpoint_eq(&pw1, &p1, eps_d)) ) {	//     
								if ( d1 < 0. ) { //     -    p1
									dist_p_l(&wv1, &ln2, &p1, &pp);	//   p1   edge_j
//       ,  ,  
									np1->v[v1] = pp;	p1 = pp;			//     
									if ( (v = np_new_vertex(np2, &pp)) == 0 ) goto err;
									if ( (edge = np_divide_edge(np2, edge_j, v)) == 0) goto err;
									if ( edge < np2->noe ) continue;
									if ( !flg_j ) edge_j = jj;
									else          edge_j = edge;
								} else {
									if ( d1 > 0. ) { //     -    wv1
//										dist_p_l(&np1->v[v1], &ln, &wv1, &pp);	//   wv1   edge_i
										wv1 = pw1;							//     
//       ,  ,  
										if ( flg_j ) np2->v[w1] = pw1;
										else         np2->v[w2] = pw1;
										if ( (v = np_new_vertex(np1, &pw1)) == 0 ) goto err;
										if ( (edge_i = np_divide_edge(np1, edge_i, v)) == 0) goto err;
										if ( edge_i < np1->noe ) continue;
									} else {	// d1 = 0 -   
										if ( flg_j )  np2->v[w1] = np1->v[v1];
										else          np2->v[w2] = np1->v[v1];
									}
								}
							} else {
								if ( flg_j )  np2->v[w1] = np1->v[v1];
								else          np2->v[w2] = np1->v[v1];
							}
							if ( !(dpoint_eq(&pw2, &p2, eps_d)) ) {	//     
								if ( d2 > 1. ) { //     -    p2
									dist_p_l(&wv1, &ln2, &p2, &pp);	//   p2   edge_j
//       ,  ,  
									np1->v[v2] = pp;	p2 = pp;			//     
									if ( (v = np_new_vertex(np2, &pp)) == 0 ) goto err;
									if ( (edge = np_divide_edge(np2, edge_j, v)) == 0) goto err;
									if ( edge < np2->noe ) continue;
									if ( !flg_j ) edge_j = edge;
								} else {
									if ( d2 < 1. ) { //     -    wv2
//										dist_p_l(&np1->v[v1], &ln, &wv2, &pp);	//   wv2   edge_i
										wv2 = pw2;																//     
										if ( flg_j ) np2->v[w2] = pw2;
										else         np2->v[w1] = pw2;
										if ( (v = np_new_vertex(np1, &pw2)) == 0 ) goto err;
										if ( (edge = np_divide_edge(np1, edge_i, v)) == 0) goto err;
										if ( edge < np1->noe ) continue;
									} else {	// d2 = 1 -   
										if ( flg_j )  np2->v[w2] = np1->v[v2];
										else          np2->v[w1] = np1->v[v2];
									}
								}
							} else {
								if ( flg_j )  np2->v[w2] = np1->v[v2];
								else          np2->v[w1] = np1->v[v2];
							}
							// edge_i   edge_j -    ii  jj

							if (np2->efc[edge_j].fp == 0) {
								w1 = np2->efr[edge_j].ev;
								face = np2->efc[edge_j].fm;
							} else {
								w1 = np2->efr[edge_j].bv;
								face = np2->efc[edge_j].fp;
							}
							if (np1->efc[edge_i].fp == 0) {
								v1 = np1->efr[edge_i].ev;
								edge = -edge_i;
								fs = np1->efc[edge_i].fm;
							} else {
								v1 = np1->efr[edge_i].bv;
								edge = edge_i;
								fs = np1->efc[edge_i].fp;
							}
							if (str_t.ort == 0) {           //   
								if (dpoint_eq(&np1->v[v1],&np2->v[w1],eps_d)) {
									np_inv(np2,NP_ALL);
									str_t.ort = -1;
								} else str_t.ort = 1;
							}
							gu = np_gru1(np1,edge,&np1->p[fs].v,&np2->p[face].v);
							if (np1->efc[edge_i].fp == 0) {
								np1->efc[edge_i].fp = -edge_i;
								np1->efc[edge_i].ep = np2->ident;
							} else {
								np1->efc[edge_i].fm = -edge_i;
								np1->efc[edge_i].em = np2->ident;
							}
							np1->gru[edge_i] = gu;
							if (np2->efc[edge_j].fp == 0) {
								np2->efc[edge_j].fp = -edge_j;
								np2->efc[edge_j].ep = np1->ident;
							} else {
								np2->efc[edge_j].fm = -edge_j;
								np2->efc[edge_j].em = np1->ident;
							}
							np2->gru[edge_j] = gu;
							if ( !(np1->efr[edge_i].el & edge_lable &&
									np2->efr[edge_j].el & edge_lable) ) {
								//   
								if (M_PI - angle <= gu && gu <= M_PI + angle) {
									np1->efr[edge_i].el &= (~ST_TD_CONSTR);
									np1->efr[edge_i].el |= ST_TD_APPROCH;
									np2->efr[edge_j].el &= (~ST_TD_CONSTR);
									np2->efr[edge_j].el |= ST_TD_APPROCH;
								} else {
									np1->efr[edge_i].el = ST_TD_CONSTR;
									np2->efr[edge_j].el = ST_TD_CONSTR;
								}
							}
							if ( ii == abs(edge_i) ) break;
							else {
								v1 = np1->efr[ii].bv;
								v2 = np1->efr[ii].ev;
								p1 = np1->v[v1];
								p2 = np1->v[v2];
							}
						}
					}
					for (ii=1 ; ii <= np2->noe ; ii++) {
						if (np2->efc[ii].fp == 0 || np2->efc[ii].fm == 0) {
							//   
							break;
						}
					}
					if (ii <= np2->noe || str_t.ort == 0) {
						if ( !(overwrite_np(&list->bnp,str_t.hnp,np2,TNPW)) ) goto err;
						write_elem(&list->vdim,*b,&str_t);
						continue;
					}
					if (regim == NP_ALL) {            //   
						if (type == BODY)  calk_volume(np2,&volume);
						np_gab_un(&g, &np2->gab, &g);
					}
					if (ident_np <= np2->ident)   ident_np = np2->ident;
					if ( list->flag )	np_approch_to_constr(np2,angle);
//					step_grad (num_proc , ni++);
					add_np_mem(&vld,np2,TNPB);
					str_t.lab = 1;                   //   
//					number++;
					write_elem(&list->vdim,*b,&str_t);
				}
				end_rw(&list->nb);   b = NULL;
				for (i=1 ; i <= np1->noe ; i++) {
					if (np1->efc[i].fp == 0 || np1->efc[i].fm == 0) {
						type = SURF;
						np1->efr[i].el = ST_TD_CONSTR;  //    
					}
				}
				if (regim == NP_ALL) {
					if (type == BODY) calk_volume(np1,&volume);
					np_gab_un(&g, &np1->gab, &g);
				}
				if (ident_np <= np1->ident)   ident_np = np1->ident;
				if ( list->flag )	np_approch_to_constr(np1,angle);
//				step_grad (num_proc , ni++);
				add_np_mem(&vld,np1,TNPB);
				str.lab = 1;                      //   
				write_elem(&list->vdim,istr,&str);
			}
		} while (number != list->number_np && number != number_old);

		if (regim == NP_FIRST) {
			np_free_stack();
			np_str_free(list);
			free_np_mem(&np2);
			*list_brep = vld.listh;
//			end_grad  (num_proc , ni);
			return number;
		}
		if (type == BODY && volume < 0.) {
			get_first_np_loc( &vld.listh, &viloc);
			for ( i=0; i<number; i++ ) {
				begin_read_vld(&viloc);
				cod = read_np_mem(np1);
				get_read_loc(&viloctmp);
				end_read_vld();
				if ( !cod ) goto err;
				np_inv((lpNPW)np1,NP_ALL);           //  
				rezet_np_mem(&viloc,np1,TNPB);
				viloc = viloctmp;
			}
		}
		if ( ( hbrep = o_alloc(OBREP)) == NULL)	goto err;
		brep = (lpOBJ)hbrep;
		lpgeobrep = (lpGEO_BREP)brep->geo_data;
		o_hcunit(lpgeobrep->matr);
		lpgeobrep->min = g.min;
		lpgeobrep->max = g.max;
		lpgeobrep->ident_np = ident_np + 1;
		lpgeobrep->type = type;
		memset(&lnp,0,sizeof(lnp));
		lnp.kind = COMMON;
		lnp.listh = vld.listh;
		lnp.num_np = number;
		if ( !put_np_brep(&lnp,&lpgeobrep->num)) goto err;
		attach_item_tail(list_brep, hbrep);
		number_obj++;
		hbrep = NULL;

		str_compress(list);
	} while (list->number_np != 0);    //  
	number = number_obj;
	free_np_mem(&np2);
	goto cancel;

err:
	number = 0;
	free_np_mem(&np2);
	free_vld_data(&vld);
	if (b != NULL)    end_rw(&list->nb);
	if (hbrep != NULL)      o_free(hbrep,NULL);
cancel:
//	end_grad  (num_proc , ni);
	np_free_stack();
	np_str_free(list);
	return number;
}

BOOL search_ort(lpNP_STR_LIST list, lpNPW np, lpNP_STR str)
{
	short  edge, index, ident;
	NP_STR strn;

	for (edge = 1 ; edge <= np->noe ; edge++) {
		if (np->efc[edge].fp < 0 || np->efc[edge].fm < 0) {
			if (np->efc[edge].fp < 0) ident = np->efc[edge].ep;
			else                      ident = np->efc[edge].em;
			if ( !np_get_str(list,ident,&strn,&index) ) return FALSE;
			if (strn.ort == 0) continue;
			if (strn.ort == -1) { str->ort = -1; np_inv(np,NP_ALL); }
			else                  str->ort =  1;
			return TRUE;
		}
	}
	return TRUE;
}

//   "" 
static void calk_volume(lpNPW np, sgFloat *volume)
{
	D_POINT a,b,c,d;
	short   face, contur, first_edge, edge, vertex;

	for (face = 1; face <= np->nof; face++) {
		contur = np->f[face].fc;
		while (contur) {
			first_edge = edge = np->c[contur].fe;
			vertex = BE(edge,np);
			a = np->v[vertex];
			edge = SL(edge,np);
			vertex = BE(edge,np);
			c = np->v[vertex];
			edge = SL(edge,np);
			do {
				b = c;
				vertex = BE(edge,np);
				c = np->v[vertex];
				dvector_product(&a,&b,&d);
       	*volume += dskalar_product(&c,&d);
				edge = SL(edge,np);
			} while (edge != first_edge);
			contur = np->c[contur].nc;
		}
	}
}

void str_compress(lpNP_STR_LIST list)
{
  lpNP_STR 			str;
  register int 	i;

	list->number_np = 0;

  begin_rw(&list->vdim,0);
	for (i=0; i< list->number_all; i++) {
    str = (lpNP_STR)get_next_elem(&list->vdim);
		if ( str->lab == 0 ) {
			list->number_np++;
			continue;
		}
    str->lab = 2;
	}
  end_rw(&list->vdim);
}

void np_str_free(lpNP_STR_LIST list)
{
//	if ( list->bnp.bd.handle != -1 )	end_rw_np(&list->bnp);
	if ( list->bnp.bd.handle != NULL )	end_rw_np(&list->bnp);
	if ( list->vdim.page[0] )  free_vdim(&list->vdim);
	if ( list->nb.page[0] )    free_vdim(&list->nb);
	np_zero_list(list);
}

