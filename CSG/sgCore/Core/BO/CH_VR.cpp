#include "../sg.h"

static BOOL  check_vr_ee(lpD_POINT * vn,lpD_POINT * vk);
static BOOL  check_vr_ff(lpD_POINT * vn, lpD_POINT * vk);
static BOOL  check_vrr2(lpD_POINT vn, lpD_POINT vk);
static BOOL  check_vr_on(short nm,lpD_POINT * vn,lpD_POINT * vk);

static short  uk1,uk2;
static lpNPW np1, np2;

BOOL check_vr(lpNPW _npa, lpNPW _npb, lpD_POINT vn, lpD_POINT vk,
							short uka, short ukb,	BOOL lea, BOOL leb)
{
//	lpNP_VERTEX  vn1,vk1,vn2,vk2;
	short       nm = 0;

	uk1 = uka;  	uk2 = ukb;
	np1 = _npa;   np2 = _npb;
	if (lea == FALSE && leb == FALSE) {
	

		if ( check_vr_ff(&vn,&vk) ) return TRUE;
		return FALSE;
	}
	if ( lea == FALSE ) {
		nm = 1;  
//		object_number = 2;
	}
	if ( leb == FALSE ) {
		nm = 2;  
//		object_number = 1;
	}
	object_number = 1;
	if (nm != 0) {
		if ( check_vr_on(nm,&vn,&vk) ) return TRUE;
		return FALSE;
	}
	

	if ( check_vr_ee(&vn,&vk) ) return TRUE;
	return FALSE;
}

static BOOL  check_vr_ff(lpD_POINT * vn, lpD_POINT * vk)
{
	BO_BIT_EDGE *b;
	lpBO_USER   usa,usb;
	D_POINT     f;
	float gu;
	short edgea, edgeb;

	if (np1->ident != np2->ident) {        
		object_number = 1;
		if ( !b_ort(vn,vk,&(np1->p[facea].v),&(np2->p[faceb].v)) ) return FALSE;
		if ( !(edgea = b_vrr(np1,*vn,*vk,&vera,uk1,facea)) ) return FALSE;
		b = (BO_BIT_EDGE*)&np1->efr[edgea].el;
		b->ep  = 1;
		b->em  = 0;
		b->flg = 1;

//		object_number = 2;
		if ( !b_ort(vn,vk,&(np2->p[faceb].v),&(np1->p[facea].v)) ) return FALSE;
		if ( !(edgeb = b_vrr(np2,*vn,*vk,&verb,uk2,faceb)) ) return FALSE;
		b = (BO_BIT_EDGE*)&np2->efr[edgeb].el;
		b->ep  = 1;
		b->em  = 0;
		b->flg = 1;
		

		f.x = np2->p[faceb].v.x;
		f.y = np2->p[faceb].v.y;
		f.z = np2->p[faceb].v.z;
		/*if (key == UNION) */
		edgea = -edgea;
		if ( (gu = np_gru(np1,edgea,&np1->p[facea].v,&f)) == 0) return FALSE;
		usa = (lpBO_USER)np1->user;
		usb = (lpBO_USER)np2->user;
        int indxxx = abs(edgea);
        usa[indxxx].bnd = gu;
		usb[edgeb].bnd = gu;
        indxxx = abs(edgea);
        usa[indxxx].ind = np2->ident;
		usb[edgeb].ind = np1->ident;
	} else { 													            
		if ( !check_vrr2( *vn,  *vk ) ) return FALSE;
	}

	return TRUE;
}
static BOOL  check_vr_on(short nm,lpD_POINT * vn,lpD_POINT * vk)
{
	short           r, edge, ir;	//, v1, v2, v;
	short           fs, fs1, fs2, face;	//, cnt, loop;
	short           p1, p2, pe1, met_p, met_o;
	short           i, ident, num_index;
	short           ukr, ukf;
	float         gu;
	lpNPW         npr, npf;
	D_POINT       ff, fr;
//	DA_POINT   		wn, wk;
//	lpDA_POINT   	w;
	BO_BIT_EDGE   *b;
	lpBO_USER     usr, usf, usa;
	lpNP_VERTEX_LIST spr, spf;
	lpNP_STR_LIST listd;
	hINDEX        hindex;
	VNP           vnp;

	if (nm == 1) {  
		npr = np2;    usr = (lpBO_USER)np2->user;
		npf = np1;    usf = (lpBO_USER)np1->user;
		spr = &verb;  ukr = uk2;
		spf = &vera;  ukf = uk1;
		face = facea;
		listd = &listb;  hindex = hindex_b; num_index = num_index_b;
	} else {        
		npr = np1;    usr = (lpBO_USER)np1->user;
		npf = np2;    usf = (lpBO_USER)np2->user;
		spr = &vera;  ukr = uk1;
		spf = &verb;  ukf = uk2;
		face = faceb;
		listd = &lista;  hindex = hindex_a; num_index = num_index_a;
	}
	r = b_vran(npr,spr,ukr);  
	if (r > 0) {
		fs1 = npr->efc[r].fp;   
		fs2 = npr->efc[r].fm;
		ident = npr->efc[r].em;     
	} else {
		fs1 = npr->efc[-r].fm;
		fs2 = npr->efc[-r].fp;
		ident = npr->efc[-r].ep;
	}
	ir = abs(r);
	if (fs2 < 0)   fs2 = np_calc_gru(npr, ir);
	if ( !(r = b_diver(npr,spr,ukr,r,*vn,*vk)) ) return FALSE; 
/*	if ( object_number == 1) object_number = 2;
	else                     object_number = 1;
*/
	p1 = b_pv(npr,         r,  &npf->p[face]);
	p2 = b_pv(npr,(short)(-r), &npf->p[face]);
    if (p1 == p2)  {
        int  indx1 = abs(r);
        b = (BO_BIT_EDGE*)&npr->efr[indx1].el;
		b->flg = 1;
		pe1 = b_pv(npr,r,&npr->p[fs2]);
		if (p1 == 1) {                             
			b->ep = 0;
			b->em = 0;
			if (pe1 == 1) return TRUE;              
		} else {                                  
			b->ep = 1;
			b->em = 1;
			if (pe1 == -1) return TRUE;             
		}
		put_message(EMPTY_MSG, GetIDS(IDS_SG313), 0);
		return FALSE;
	}
    if (p1 == -p2) {
        int indx2 = abs(r);
        b = (BO_BIT_EDGE*)&npr->efr[indx2].el;
		if ( b->flg == 1 ) return TRUE;							
		b->flg = 1;
		if (r > 0 && p1 == -1 || r < 0 && p1 == 1) {
			b->ep = 1;
			b->em = 0;
		} else {
			b->ep = 0;
			b->em = 1;
		}
		if ( !b_ort(vn,vk,&(npf->p[face].v),&(npr->p[fs1].v)) ) return FALSE;
		edge = abs(b_vrr(npf,*vn,*vk,spf,ukf,face));
		if (edge == 0) return FALSE;
		b = (BO_BIT_EDGE*)&npf->efr[edge].el;  
		b->ep = 1;
		b->em = 0;
		b->flg = 1;
	} else {                            
		if ((fabs(npf->p[face].v.x - npr->p[fs2].v.x) < eps_n) &&
				(fabs(npf->p[face].v.y - npr->p[fs2].v.y) < eps_n) &&
				(fabs(npf->p[face].v.z - npr->p[fs2].v.z) < eps_n) ) {
			if ( !b_ort_on(npr, (short)(-r), vn, vk) ) return FALSE; 
//			if ( npr->ident != npf->ident ) {   
				edge = abs(b_vrr(npf,*vn,*vk,spf,ukf,face));
				if ( edge == 0 ) return FALSE;
/*			} else {          
//				if ( !b_ort(vn,vk,&(np1->p[fs1].v),&(np1->p[fs2].v)) ) return FALSE;
				wn[0] = np1->v[spr->v[uk1-1].v].x;
				wn[1] = np1->v[spr->v[uk1-1].v].y;
				wn[2] = np1->v[spr->v[uk1-1].v].z;
				wk[0] = np1->v[spr->v[uk1].v].x;
				wk[1] = np1->v[spr->v[uk1].v].y;
				wk[2] = np1->v[spr->v[uk1].v].z;

				if ( (v1 = np_new_vertex(np1, *vn)) == 0)    return FALSE;
				if ( (v2 = np_new_vertex(np1, *vk)) == 0)    return FALSE;

				if (np1->noe+2 > np1->maxnoe)
					if ( !(o_expand_np(np1, 0, MAXNOE/4, 0, 0)) ) return FALSE;

				edge = ++np1->noe;
				np1->efr[edge].bv = v1;
				np1->efr[edge].ev = v2;
				np1->efc[edge].ep = 0;
				np1->efc[edge].em = 0;
				np1->gru[edge] = 0;

				np1->efc[edge].fm = np1->efc[edge].fp = face;
				np1->efr[edge].el = ST_TD_CONSTR;

				w = (lpDA_POINT)np1->v;
				if (w[v1][np_axis] > w[v2][np_axis]) {
					v = v1;   v1 = v2;   v2 = v;
				}
				if ((fabs(w[v1][np_axis] - wn[np_axis])) < eps_d &&
							spr->v[uk1-1].m == 1) {   
					if (b_dive(np1,spr->v[uk1-1].r,v1) == 0) return FALSE;
				}
				w = (lpDA_POINT)np1->v;
				if ((fabs(w[v2][np_axis] - wk[np_axis])) < eps_d &&
							spr->v[uk1].m == 1) {     
					if (b_dive(np1,spr->v[uk1].r,v2) == 0) return FALSE;
				}

				cnt = np1->f[face].fc;      
				while (cnt > 0) {
					loop = cnt;
					cnt = np1->c[cnt].nc;
				}
				if (cnt != 0) {
					cnt = abs(cnt);
					np1->efc[edge].ep = np1->c[cnt].fe;
					np1->c[cnt].fe = edge;
				} else {
					cnt = np_new_loop(np1,edge);
					np1->c[loop].nc = -cnt;
				}
			}
*/
            int  indx3 = abs(r);
            b = (BO_BIT_EDGE*)&npr->efr[indx3].el;
			b->flg = 1;
			if (r > 0) {
				b->em = 3;                      
				if (p1 > 0) b->ep = 0;          
				else        b->ep = 1;         
			} else {
				b->ep = 3;                      
				if (p1 > 0) b->em = 0;          
				else        b->em = 1;          
			}
			b = (BO_BIT_EDGE*)&npf->efr[edge].el;  
			b->flg = 1;
			b->ep  = 3;                      
			if (p1 > 0)   b->em = 1;          
			else          b->em = 0;          
		} else {                            
			if ( !b_ort_on(npr, r, vn, vk) ) return FALSE;
//			if ( npr->ident != npf->ident ) {
				edge = abs(b_vrr(npf,*vn,*vk,spf,ukf,face));
				if ( edge == 0 ) return FALSE;
/*			} else {         
//				if ( !b_ort(vn,vk,&(np1->p[fs1].v),&(np1->p[fs2].v)) ) return FALSE;
				wn[0] = np1->v[spr->v[uk1-1].v].x;
				wn[1] = np1->v[spr->v[uk1-1].v].y;
				wn[2] = np1->v[spr->v[uk1-1].v].z;
				wk[0] = np1->v[spr->v[uk1].v].x;
				wk[1] = np1->v[spr->v[uk1].v].y;
				wk[2] = np1->v[spr->v[uk1].v].z;

				if ( (v1 = np_new_vertex(np1, *vn)) == 0)    return FALSE;
				if ( (v2 = np_new_vertex(np1, *vk)) == 0)    return FALSE;

				if (np1->noe+2 > np1->maxnoe)
					if ( !(o_expand_np(np1, 0, MAXNOE/4, 0, 0)) ) return FALSE;

				edge = ++np1->noe;
				np1->efr[edge].bv = v1;
				np1->efr[edge].ev = v2;
				np1->efc[edge].ep = 0;
				np1->efc[edge].em = 0;
				np1->gru[edge] = 0;

				np1->efc[edge].fm = np1->efc[edge].fp = face;
				np1->efr[edge].el = ST_TD_CONSTR;

				w = (lpDA_POINT)np1->v;
				if (w[v1][np_axis] > w[v2][np_axis]) {
					v = v1;   v1 = v2;   v2 = v;
				}
				if ((fabs(w[v1][np_axis] - wn[np_axis])) < eps_d &&
							spr->v[uk1-1].m == 1) {   
					if (b_dive(np1,spr->v[uk1-1].r,v1) == 0) return FALSE;
				}
				w = (lpDA_POINT)np1->v;
				if ((fabs(w[v2][np_axis] - wk[np_axis])) < eps_d &&
							spr->v[uk1].m == 1) {     
					if (b_dive(np1,spr->v[uk1].r,v2) == 0) return FALSE;
				}

				cnt = np1->f[face].fc;      
				while (cnt > 0) {
					loop = cnt;
					cnt = np1->c[cnt].nc;
				}
				if (cnt != 0) {
					cnt = abs(cnt);
					np1->efc[edge].ep = np1->c[cnt].fe;
					np1->c[cnt].fe = edge;
				} else {
					cnt = np_new_loop(np1,edge);
					np1->c[loop].nc = -cnt;
				}
			}
*/
            int indx4 = abs(r);
            b = (BO_BIT_EDGE*)&npr->efr[indx4].el;
			b->flg = 1;                      
			if (r > 0) {
				b->em = 2;                      
				if (p1 > 0)  b->ep = 0;         
				else         b->ep = 1;         
			} else {
				b->ep = 2;                      
				if (p1 > 0)  b->em = 0;         
				else         b->em = 1;         
			}
			b = (BO_BIT_EDGE*)&npf->efr[edge].el;
			b->flg = 1;                      
			b->ep  = 2;                      
			if (p1 > 0)    b->em = 0;        
			else           b->em = 1;        
		}
	}


	ir = abs(r);
	if ( npr->ident == npf->ident ) 	{    
		usa = (lpBO_USER)npf->user;
		usa[edge].bnd = ir;
		usa[ir].bnd 	= edge;
		usa[edge].ind = npf->ident;
		usa[ir].ind 	= npf->ident;
		return TRUE;
	}

	b = (BO_BIT_EDGE*)&npr->efr[ir].el; 
	if (r > 0) { met_p = b->ep; met_o = b->em;}      
	else       { met_p = b->em; met_o = b->ep;}
	if (nm == 1) object_number = 2;
	else         object_number = 1;
	i = 0;
	if ((b_face_label(met_p)) == 1) { fs = fs1; i++;}
	if ((b_face_label(met_o)) == 1) { fs = fs2; r = -r; i++;}
	if (i == 0 || i == 2) return TRUE;
	memcpy(&fr,&npr->p[fs].v,sizeof(fr));
	memcpy(&ff,&npf->p[face].v,sizeof(ff));
	if ( (gu = np_gru(npr,r,&fr,&ff)) == 0 ) return FALSE;
    int indxxx = abs(edge);
    usf[indxxx].bnd = gu;
    if (fs == 0) {
        int indxx2 = abs(edge);
        usf[indxx2].ind = ident;
		usr[ir].bnd = gu;
		usr[ir].ind = npf->ident;
		if (p1 == 0 || p2 == 0) {
			if ( !np_write_np_tmp(npf) )              return FALSE;
			if ( !(vnp=b_get_hnp(hindex,num_index,ident)) ) {
				put_message(INTERNAL_ERROR,"no_index",0);
				goto err;
			}
			if ( !read_np(&listd->bnp,vnp,npf) )  goto err;
			if ( !b_vr_idn(npf,*vn,*vk,npr,ir) )  goto err;
			if ( !overwrite_np(&listd->bnp,vnp,npf,TNPW) ) goto err;
			if ( !np_read_np_tmp(npf) )               return FALSE;
		}
	}	else {
		usf[edge].ind = npr->ident;
		usf[edge].bnd = gu;
		usr[ir].ind 	= npf->ident;
		usr[ir].bnd 	= gu;
	}
	return TRUE;
err:
  np_free_np_tmp();
  return FALSE;
}

static BOOL  check_vr_ee(lpD_POINT * vn,lpD_POINT * vk)
{
	short     edgea,edgeb,ir,jr,r;
	short     pe1,met,i1,i2,pra,prb;
	short     fma,fpa,fmb,fpb;
	short     identa,identb;
	short     fs1,fs2;
	float   gu;
	D_POINT f1,f2;
	BO_BIT_EDGE *ba,*bb;
	lpBO_USER usa,usb;
	VNP           vnp;

	usa = (lpBO_USER)np1->user;
	usb = (lpBO_USER)np2->user;

	object_number = 1;
	edgea = b_vran(np1,&vera,uk1);
	if ( !(edgea = b_diver(np1,&vera,uk1,edgea,*vn,*vk)) ) return FALSE; 
	ir = abs(edgea);
//	object_number = 2;
	edgeb = b_vran(np2,&verb,uk2);
	if ( !(edgeb = b_diver(np2,&verb,uk2,edgeb,*vn,*vk)) ) return FALSE; 
	jr = abs(edgeb);
	ba = (BO_BIT_EDGE*)&np1->efr[ir].el;
	bb = (BO_BIT_EDGE*)&np2->efr[jr].el;
	if (ba->flg == 1 && bb->flg == 1) return TRUE;   

	fpa = np1->efc[ir].fp;       fpb = np2->efc[jr].fp;
	fma = np1->efc[ir].fm;       fmb = np2->efc[jr].fm;
	if (fpa < 0 || fma < 0) {        
		if (fpa < 0)  { fpa = np_calc_gru(np1, ir); identa = np1->efc[ir].ep; }
		else          { fma = np_calc_gru(np1, ir); identa = np1->efc[ir].em; }
	}
	if ( fpb == 0 || fmb == 0 ) {
		put_message(NO_LIMIT_COND,NULL,0);
		return FALSE;
	}
	if (fpb < 0 || fmb < 0) {        
		if (fpb < 0)  { fpb = np_calc_gru(np2, jr); identb = np2->efc[jr].ep; }
		else          { fmb = np_calc_gru(np2, jr); identb = np2->efc[jr].em; }
  }
	pra = 0;
	if (ba->flg == 0) {              
		pe1 = b_pv(np2,jr,&np2->p[fmb]);
		ba->ep = b_face_angle(np1, ir,np2,fpb,fmb,pe1);
		ba->em = b_face_angle(np1,(short)(-ir),np2,fpb,fmb,pe1);
		ba->flg = 1;
	} else  pra = 1;
  prb = 0;
	if (bb->flg == 0) {             
		pe1 = b_pv(np1,ir,&np1->p[fma]);
		bb->ep = b_face_angle(np2,         jr,  np1, fpa, fma, pe1);
		bb->em = b_face_angle(np2,(short)(-jr), np1, fpa, fma, pe1);
		bb->flg = 1;
  } else prb = 1;

	if ( np1->ident == np2->ident ) 	{    
		usa = (lpBO_USER)np1->user;
		usa[ir].bnd = jr;
		usa[jr].bnd	= ir;
		usa[ir].ind = np1->ident;
		usa[jr].ind = np1->ident;
		return TRUE;
	}

	
	i1 = 0;    i2 = 0;
	met = ba->ep;
  object_number = 1;
	if ((b_face_label(met)) == 1) { fs1 = fpa; r =  ir; i1++; }
  met = ba->em;
  if ((b_face_label(met)) == 1) { fs1 = fma; r = -ir; i1++; }
	met = bb->ep;
  object_number = 2;
  if ((b_face_label(met)) == 1) { fs2 = fpb; i2++; }
	met = bb->em;
	if ((b_face_label(met)) == 1) { fs2 = fmb; i2++; }
	if (i1 == 2 && i2 == 0 || i1 == 0 && i2 == 2 ||
			i1 == 0 && i2 == 0) return TRUE;
	if (i1 == 1 && i2 == 1) {
		memcpy(&f1,&np1->p[fs1].v,sizeof(f1));
		memcpy(&f2,&np2->p[fs2].v,sizeof(f2));
		if ( (gu = np_gru(np1,r,&f1,&f2)) == 0 ) return FALSE;
		if (pra == 0) {
			usa[ir].bnd = gu;
			if (fs2 == 0) usa[ir].ind = identb;
			else          usa[ir].ind = np2->ident;
			if (fs1 == 0) {           
				if (ba->ep == 2 || ba->ep == 3 ||
						ba->em == 2 || ba->em == 3) {               
					if ( !np_write_np_tmp(np2) ) return FALSE;
					if ( !(vnp = b_get_hnp(hindex_a, (short)num_index_a, identa)) ) {
						put_message(INTERNAL_ERROR,"no_index",0);
						goto err;
					}
					if ( !read_np(&lista.bnp,vnp,np2) )  goto err;
					if ( !b_vr_idn(np2,*vn,*vk,np1,ir) )  goto err;
					if ( !overwrite_np(&lista.bnp,vnp,np2,TNPW) ) goto err;
					if ( !np_read_np_tmp(np2) ) return FALSE;
				}
			}
		}
		if (prb == 0) {
			usb[jr].bnd = gu;
			if (fs1 == 0) usb[jr].ind = identa;
			else          usb[jr].ind = np1->ident;
			if (fs2 == 0) {           
				if (bb->ep == 2 || bb->ep == 3 ||
						bb->em == 2 || bb->em == 3 ) {              
					if ( !np_write_np_tmp(np1) ) return FALSE;
					if ( !(vnp = b_get_hnp(hindex_b, (short)num_index_b, identb)) ) {
						put_message(INTERNAL_ERROR,"no_index",0);
						goto err;
					}
					if ( !read_np(&listb.bnp,vnp,np1) )  goto err;
					if ( !b_vr_idn(np1,*vn,*vk,np2,jr) )  goto err;
					if ( !overwrite_np(&listb.bnp,vnp,np1,TNPW) ) goto err;
					if ( !np_read_np_tmp(np1) ) return FALSE;
				}
			}
    }
    return TRUE;
  }
	if (i1 == 2 && i2 == 2)
		put_message(TOO_MANY_FACES,NULL,0);
	else
		put_message(INTERNAL_ERROR,"err_b_vr_ee",0);
	return FALSE;
err:
	np_free_np_tmp();
	return FALSE;
}

static BOOL  check_vrr2(lpD_POINT vn, lpD_POINT vk)
{
//	short       	v1, v2, v;
	short      	  edge1, edge2;	//, cnt, loop;
//	DA_POINT   	wn,wk;
	lpBO_USER   usa;
	BO_BIT_EDGE *b;
//	lpDA_POINT 	w;

	object_number = 1;
	if ( !b_ort(&vn,&vk,&(npa->p[facea].v),&(npa->p[faceb].v)) ) return FALSE;
	if ( !(edge1 = b_vrr(npa,vn,vk,&vera,uk1,facea)) ) return FALSE;
	b = (BO_BIT_EDGE*)&npa->efr[edge1].el;
	b->ep  = 1;
	b->em  = 0;
	b->flg = 1;

//		object_number = 2;
//	if ( !b_ort(&vn,&vk,&(npa->p[faceb].v),&(npa->p[facea].v)) ) return FALSE;
	if ( !(edge2 = b_vrr(npa,vn,vk,&verb,uk2,faceb)) ) return FALSE;
	b = (BO_BIT_EDGE*)&np2->efr[edge2].el;
	b->ep  = 0;
	b->em  = 1;
	b->flg = 1;

/*
	if ( !b_ort(&vn,&vk,&(npa->p[facea].v),&(npa->p[faceb].v)) ) return FALSE;
	wn[0] = npa->v[vera.v[uk1-1].v].x;
	wn[1] = npa->v[vera.v[uk1-1].v].y;
	wn[2] = npa->v[vera.v[uk1-1].v].z;
	wk[0] = npa->v[vera.v[uk1].v].x;
	wk[1] = npa->v[vera.v[uk1].v].y;
	wk[2] = npa->v[vera.v[uk1].v].z;

	if ( (v1 = np_new_vertex(npa, vn)) == 0)    return FALSE;
	if ( (v2 = np_new_vertex(npa, vk)) == 0)    return FALSE;

	if (npa->noe+2 > npa->maxnoe)
		if ( !(o_expand_np(npa, 0, MAXNOE/4, 0, 0)) ) return FALSE;

	edge1 = ++npa->noe;
	npa->efr[edge1].bv = v1;
	npa->efr[edge1].ev = v2;
	npa->efc[edge1].ep = 0;
	npa->efc[edge1].em = 0;
	npa->gru[edge1] = 0;

	npa->efc[edge1].fm = npa->efc[edge1].fp = facea;
	npa->efr[edge1].el = ST_TD_CONSTR;

	edge2 = ++npa->noe;
	npa->efr[edge2].bv = v1;
	npa->efr[edge2].ev = v2;
	npa->efc[edge2].ep = 0;
	npa->efc[edge2].em = 0;
	npa->gru[edge2] = 0;

	npa->efc[edge2].fm = npa->efc[edge2].fp = faceb;
	npa->efr[edge2].el = ST_TD_CONSTR;

	w = (lpDA_POINT)npa->v;
	if (w[v1][np_axis] > w[v2][np_axis]) {
		v = v1;   v1 = v2;   v2 = v;
	}
	if ((fabs(w[v1][np_axis] - wn[np_axis])) < eps_d &&
				vera.v[uk1-1].m == 1) {   
		if (b_dive(npa,vera.v[uk1-1].r,v1) == 0) return FALSE;
	}
	w = (lpDA_POINT)npa->v;
	if ((fabs(w[v2][np_axis] - wk[np_axis])) < eps_d &&
				vera.v[uk1].m == 1) {    
		if (b_dive(npa,vera.v[uk1].r,v2) == 0) return FALSE;
	}
	b = (BO_BIT_EDGE*)&npa->efr[edge1].el;
	b->ep  = 1;
	b->em  = 0;
	b->flg = 1;

	wn[0] = npa->v[verb.v[uk2-1].v].x;
	wn[1] = npa->v[verb.v[uk2-1].v].y;
	wn[2] = npa->v[verb.v[uk2-1].v].z;
	wk[0] = npa->v[verb.v[uk2].v].x;
	wk[1] = npa->v[verb.v[uk2].v].y;
	wk[2] = npa->v[verb.v[uk2].v].z;

	w = (lpDA_POINT)npa->v;
	if ((fabs(w[v1][np_axis] - wn[np_axis])) < eps_d &&
				verb.v[uk2-1].m == 1) {   
		if (b_dive(npa,verb.v[uk2-1].r,v1) == 0) return FALSE;
	}
	w = (lpDA_POINT)npa->v;
	if ((fabs(w[v2][np_axis] - wk[np_axis])) < eps_d &&
				verb.v[uk2].m == 1) {     
		if (b_dive(npa,verb.v[uk2].r,v2) == 0) return FALSE;
	}
	b = (BO_BIT_EDGE*)&npa->efr[edge2].el;
	b->ep  = 0;
	b->em  = 1;
	b->flg = 1;

	cnt = npa->f[facea].fc;      
	while (cnt > 0) {
		loop = cnt;
		cnt = npa->c[cnt].nc;
	}
	if (cnt != 0) {
		cnt = abs(cnt);
		npa->efc[edge1].ep = npa->c[cnt].fe;
		npa->c[cnt].fe = edge1;
	} else {
		cnt = np_new_loop(npa,edge1);
		npa->c[loop].nc = -cnt;
	}

	cnt = npa->f[faceb].fc;     
	while (cnt > 0) {
		loop = cnt;
		cnt = npa->c[cnt].nc;
	}
	if (cnt != 0) {
		cnt = abs(cnt);
		npa->efc[edge2].ep = npa->c[cnt].fe;
		npa->c[cnt].fe = edge2;
	} else {
		cnt = np_new_loop(npa,edge2);
		npa->c[loop].nc = -cnt;
	}
*/
	
	usa = (lpBO_USER)npa->user;
	usa[edge1].bnd = edge2;
	usa[edge2].bnd = edge1;
	usa[edge1].ind = npa->ident;
	usa[edge2].ind = npa->ident;

	return TRUE;
}
