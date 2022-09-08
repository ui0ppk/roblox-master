#include "../sg.h"

static BOOL  b_vr_ee(lpD_POINT * vn,lpD_POINT * vk);
static BOOL  b_vr_ff(lpD_POINT * vn, lpD_POINT * vk);
static BOOL  b_vr_on(short nm,lpD_POINT * vn,lpD_POINT * vk);

short  uk1,uk2;

BOOL b_vr(short uka, short ukb, BOOL lea, BOOL leb)
{
	lpNP_VERTEX  vn1,vk1,vn2,vk2;
	short       nm = 0;
	D_POINT   vvn,vvk;
	lpD_POINT vn,vk;
	lpDA_POINT  w1, w2;

	uk1 = uka;  uk2 = ukb;
	w1 = (lpDA_POINT)npa->v;
	w2 = (lpDA_POINT)npb->v;
	vn1 = &(vera.v[uk1-1]);  vk1 = &(vera.v[uk1]);
	vn2 = &(verb.v[uk2-1]);  vk2 = &(verb.v[uk2]);

	
	if (w1[vn1->v][np_axis] < w2[vn2->v][np_axis])
				memcpy(&vvn,&(npb->v[vn2->v]),sizeof(vvn));
	else 	memcpy(&vvn,&(npa->v[vn1->v]),sizeof(vvn));
	vn = &vvn;
	
	if (w1[vk1->v][np_axis] < w2[vk2->v][np_axis])
			 memcpy(&vvk,&(npa->v[vk1->v]),sizeof(vvk));
	else memcpy(&vvk,&(npb->v[vk2->v]),sizeof(vvk));
	vk = &vvk;
//  if ( !tb_put_otr(vn,vk) ) return FALSE;
//  f3d_line(&vport[curr_vp].wind3d,vn,vk);  
/*
	if (key == PENETRATE)  {
		t_vr_pnt(&vn);
		return;
	}
*/
	if (key == LINE_INTER) {
		if ( !(b_vr_line(vn,vk)) ) return FALSE;
		return TRUE;
	}
	if (lea == FALSE && leb == FALSE) {
	
		if ( b_vr_ff(&vn,&vk) ) return TRUE;
		return FALSE;
	}
	if ( lea == FALSE ) {
		nm = 1; 
		object_number = 2;
	}
	if ( leb == FALSE ) {
		nm = 2;  
		object_number = 1;
	}
	if (nm != 0) {
		if ( b_vr_on(nm,&vn,&vk) ) return TRUE;
		return FALSE;
	}
	//  - 

	if ( b_vr_ee(&vn,&vk) ) return TRUE;
	return FALSE;
}

static BOOL  b_vr_ff(lpD_POINT * vn, lpD_POINT * vk)
{
	BO_BIT_EDGE *b;
	lpBO_USER   usa,usb;
	D_POINT     f;
  float gu;
	short edgea, edgeb;

	object_number = 1;
  if ( !b_ort(vn,vk,&(npa->p[facea].v),&(npb->p[faceb].v)) ) return FALSE;
	if ( !(edgea = b_vrr(npa,*vn,*vk,&vera,uk1,facea)) ) return FALSE;
    b = (BO_BIT_EDGE*)&npa->efr[(int)abs(edgea)].el;
	b->ep  = 1;
	b->em  = 0;
	b->flg = 1;

	object_number = 2;
	if ( !b_ort(vn,vk,&(npb->p[faceb].v),&(npa->p[facea].v)) ) return FALSE;
	if ( !(edgeb = b_vrr(npb,*vn,*vk,&verb,uk2,faceb)) ) return FALSE;
    b = (BO_BIT_EDGE*)&npb->efr[(int)abs(edgeb)].el;
	b->ep  = 1;
	b->em  = 0;
	b->flg = 1;
	

	if ( !brept1 ) 	{               
		usa = (lpBO_USER)npa->user;
        usa[(int)abs(edgea)].bnd = 0.;
        usa[(int)abs(edgea)].ind = 0;
		return TRUE;
	}

	f.x = npb->p[faceb].v.x;
	f.y = npb->p[faceb].v.y;
	f.z = npb->p[faceb].v.z;
  if (key == SUB) { edgea = -edgea;  f.x = - f.x; f.y = - f.y; f.z = - f.z; }
  if (key == UNION) edgea = -edgea;
	if ( (gu = np_gru(npa,edgea,&npa->p[facea].v,&f)) == 0 ) return FALSE;
	usa = (lpBO_USER)npa->user;
	usb = (lpBO_USER)npb->user;
    usa[(int)abs(edgea)].bnd = gu;
	usb[edgeb].bnd = gu;
    usa[(int)abs(edgea)].ind = npb->ident;
	usb[edgeb].ind = npa->ident;

  return TRUE;
}
static BOOL  b_vr_on(short nm,lpD_POINT * vn,lpD_POINT * vk)
{
	short           r,edge,ir,edgea;
	short           fs,fs1,fs2,face;
	short           p1,p2,pe1,met_p,met_o;
	short           i,ident,num_index;
	short           ukr,ukf;
	float         gu;
	lpNPW         npr,npf;
	D_POINT       ff,fr;
	BO_BIT_EDGE   *b;
	lpBO_USER     usr,usf,usa;
	lpNP_VERTEX_LIST spr,spf;
	lpNP_STR_LIST listd;
	hINDEX        hindex;
	VNP           vnp;

	if (nm == 1) {
		npr = npb;    usr = (lpBO_USER)npb->user;
		npf = npa;    usf = (lpBO_USER)npa->user;
		spr = &verb;  ukr = uk2;
		spf = &vera;  ukf = uk1;
		face = facea;
		listd = &listb;  hindex = hindex_b; num_index = num_index_b;
	} else {
		npr = npa;    usr = (lpBO_USER)npa->user;
		npf = npb;    usf = (lpBO_USER)npb->user;
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
	if (fs2 == 0) {       
		if ( !brept1 && object_number == 1)  { 
			npr->p[0] = npr->p[fs1];
		}	else {
			put_message(NO_LIMIT_COND,NULL,0);
			return FALSE;
		}
	} else { if (fs2 < 0)   fs2 = np_calc_gru(npr, ir); }
	if ( !(r = b_diver(npr,spr,ukr,r,*vn,*vk)) ) return FALSE; 
	if ( object_number == 1) object_number = 2;
	else                     object_number = 1;
	p1 = b_pv(npr,         r,  &npf->p[face]);
	p2 = b_pv(npr,(short)(-r), &npf->p[face]);
	if (p1 == p2)  {                            
        b = (BO_BIT_EDGE*)&npr->efr[(int)abs(r)].el;
		b->flg = 1;
		pe1 = b_pv(npr,r,&npr->p[fs2]);
		if (p1 == 1) {                             
			b->ep = 0;
			b->em = 0;
			if (pe1 == -1) {                         
				if (key != UNION) return TRUE;
			} else                                  
				if (!(key == SUB && object_number == 2)) return TRUE;
		} else {                                  
			b->ep = 1;
			b->em = 1;
			if (pe1 == -1) {                         
				if (!(key == SUB && nm == 1)) return TRUE;
			} else                                   
				if (key != INTER) return TRUE;
		}
		put_message(TOO_MANY_FACES,NULL,0);
		return FALSE;
	}
	if (p1 == -p2) {                              
        b = (BO_BIT_EDGE*)&npr->efr[(int)abs(r)].el;
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
        b = (BO_BIT_EDGE*)&npf->efr[(int)abs(edge)].el;
		b->ep = 1;
		b->em = 0;
		b->flg = 1;
	} else {                            
		if ((fabs(npf->p[face].v.x - npr->p[fs2].v.x) < eps_n) &&
				(fabs(npf->p[face].v.y - npr->p[fs2].v.y) < eps_n) &&
				(fabs(npf->p[face].v.z - npr->p[fs2].v.z) < eps_n) ) {
			if ( !b_ort_on(npr, (short)(-r), vn, vk) ) return FALSE; 
			edge = abs(b_vrr(npf,*vn,*vk,spf,ukf,face));
			if ( edge == 0 ) return FALSE;
            b = (BO_BIT_EDGE*)&npr->efr[(int)abs(r)].el;
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
			edge = abs(b_vrr(npf,*vn,*vk,spf,ukf,face));
			if ( edge == 0 ) return FALSE;
            b = (BO_BIT_EDGE*)&npr->efr[(int)abs(r)].el;
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

	if ( !brept1 ) 	{              
		if (nm == 1) 	edgea = r;
		else          edgea = edge;
		usa = (lpBO_USER)npa->user;
        usa[(int)abs(edgea)].bnd = 0.;
        usa[(int)abs(edgea)].ind = 0;
		return TRUE;
	}

	ir = abs(r);
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
	if (key == SUB) {  
		if (nm == 2)  { ff.x = - ff.x; ff.y = - ff.y; ff.z = - ff.z; }
		else          { fr.x = - fr.x; fr.y = - fr.y; fr.z = - fr.z; r = -r;}
	}
	if ( (gu = np_gru(npr,r,&fr,&ff)) == 0 ) return FALSE;
    usf[(int)abs(edge)].bnd = gu;
	if (fs == 0) {               
        usf[(int)abs(edge)].ind = ident;
		usr[ir].bnd = gu;
		usr[ir].ind = npf->ident;
		if (p1 == 0 || p2 == 0) {
			if ( !np_write_np_tmp(npf) )              return FALSE;
			if ( !(vnp=b_get_hnp(hindex,num_index,ident)) ) {
				put_message(INTERNAL_ERROR,GetIDS(IDS_SG023),0);
				goto err;
			}
			if ( !read_np(&listd->bnp,vnp,npf) )  goto err;
			if ( !b_vr_idn(npf,*vn,*vk,npr,ir) )  goto err;
			if ( !overwrite_np(&listd->bnp,vnp,npf,TNPW) ) goto err;
			if ( !np_read_np_tmp(npf) )               return FALSE;
		}
	}	else {
        usf[(int)abs(edge)].ind = npr->ident;
		usr[ir].bnd = gu;
		usr[ir].ind = npf->ident;
	}
	return TRUE;
err:
  np_free_np_tmp();
  return FALSE;
}

static BOOL  b_vr_ee(lpD_POINT * vn,lpD_POINT * vk)
{
	short				edgea, edgeb, ir, jr, r;
	short				pe1, met, i1, i2, pra, prb;
	short				fma, fpa, fmb, fpb;
	short				identa, identb;
	short				fs1, fs2;
	float				gu;
	D_POINT			f1, f2;
	BO_BIT_EDGE *ba, *bb;
	lpBO_USER usa, usb;
  VNP           vnp;

	usa = (lpBO_USER)npa->user;
	usb = (lpBO_USER)npb->user;

	object_number = 1;
	edgea = b_vran(npa,&vera,uk1);
	if ( !(edgea = b_diver(npa,&vera,uk1,edgea,*vn,*vk)) ) return FALSE; 
	ir = abs(edgea);
	object_number = 2;
	edgeb = b_vran(npb,&verb,uk2);
	if ( !(edgeb = b_diver(npb,&verb,uk2,edgeb,*vn,*vk)) ) return FALSE; 
	jr = abs(edgeb);
	ba = (BO_BIT_EDGE*)&npa->efr[ir].el;
	bb = (BO_BIT_EDGE*)&npb->efr[jr].el;
	if (ba->flg == 1 && bb->flg == 1) return TRUE;   

	fpa = npa->efc[ir].fp;       fpb = npb->efc[jr].fp;
	fma = npa->efc[ir].fm;       fmb = npb->efc[jr].fm;
	if (fpa == 0 || fma == 0 ) { 
		if ( !brept1 )  { 					
			if (fpa == 0)	npa->p[0] = npa->p[fma];
			else          npa->p[0] = npa->p[fpa];
		}	else {
			put_message(NO_LIMIT_COND,NULL,0);
			return FALSE;
		}
	} else {
		if (fpa < 0 || fma < 0) {        
			if (fpa < 0)  { fpa = np_calc_gru(npa, ir); identa = npa->efc[ir].ep; }
			else          { fma = np_calc_gru(npa, ir); identa = npa->efc[ir].em; }
		}
	}
	if ( fpb == 0 || fmb == 0 ) {
		put_message(NO_LIMIT_COND,NULL,0);
		return FALSE;
	}
	if (fpb < 0 || fmb < 0) {       
    if (fpb < 0)  { fpb = np_calc_gru(npb, jr); identb = npb->efc[jr].ep; }
		else          { fmb = np_calc_gru(npb, jr); identb = npb->efc[jr].em; }
  }
	pra = 0;
	if (ba->flg == 0) {              
		pe1 = b_pv(npb,jr,&npb->p[fmb]);
		ba->ep = b_face_angle(npa,         ir, npb, fpb, fmb, pe1);
		ba->em = b_face_angle(npa,(short)(-ir),npb, fpb, fmb, pe1);
		ba->flg = 1;
	} else  pra = 1;
  prb = 0;
	if (bb->flg == 0) {              
		pe1 = b_pv(npa,ir,&npa->p[fma]);
		bb->ep = b_face_angle(npb,         jr, npa, fpa, fma, pe1);
		bb->em = b_face_angle(npb,(short)(-jr),npa, fpa, fma, pe1);
		bb->flg = 1;
  } else prb = 1;

	

	if ( !brept1 ) 	{              
		usa = (lpBO_USER)npa->user;
        usa[(int)abs(edgea)].bnd = 0.;
        usa[(int)abs(edgea)].ind = 0;
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
	memcpy(&f1,&npa->p[fs1].v,sizeof(f1));
	memcpy(&f2,&npb->p[fs2].v,sizeof(f2));
		if (key == SUB) {  
			f2.x = - f2.x;    f2.y = - f2.y;    f2.z = - f2.z;
		}
		if ( (gu = np_gru(npa,r,&f1,&f2)) == 0 ) return FALSE;
		if (pra == 0) {
			usa[ir].bnd = gu;
			if (fs2 == 0) usa[ir].ind = identb;
			else          usa[ir].ind = npb->ident;
			if (fs1 == 0) {           
				if (ba->ep == 2 || ba->ep == 3 ||
						ba->em == 2 || ba->em == 3) {              
					if ( !np_write_np_tmp(npb) ) return FALSE;
					if ( !(vnp = b_get_hnp(hindex_a, (short)num_index_a, identa)) ) {
						put_message(INTERNAL_ERROR,GetIDS(IDS_SG024),0);
						goto err;
					}
					if ( !read_np(&lista.bnp,vnp,npb) )  goto err;
					if ( !b_vr_idn(npb,*vn,*vk,npa,ir) )  goto err;
					if ( !overwrite_np(&lista.bnp,vnp,npb,TNPW) ) goto err;
					if ( !np_read_np_tmp(npb) ) return FALSE;
				}
			}
		}
		if (prb == 0) {
			usb[jr].bnd = gu;
			if (fs1 == 0) usb[jr].ind = identa;
			else          usb[jr].ind = npa->ident;
			if (fs2 == 0) {           
				if (bb->ep == 2 || bb->ep == 3 ||
						bb->em == 2 || bb->em == 3 ) {              
					if ( !np_write_np_tmp(npa) ) return FALSE;
					if ( !(vnp=b_get_hnp(hindex_b, (short)num_index_b, identb)) ) {
						put_message(INTERNAL_ERROR,GetIDS(IDS_SG025),0);
						goto err;
					}
					if ( !read_np(&listb.bnp,vnp,npa) )  goto err;
					if ( !b_vr_idn(npa,*vn,*vk,npb,jr) )  goto err;
					if ( !overwrite_np(&listb.bnp,vnp,npa,TNPW) ) goto err;
					if ( !np_read_np_tmp(npa) ) return FALSE;
				}
			}
    }
    return TRUE;
  }
	if (i1 == 2 && i2 == 2)
		put_message(TOO_MANY_FACES,NULL,0);
	else
		put_message(INTERNAL_ERROR,GetIDS(IDS_SG026),0);
	return FALSE;
err:
  np_free_np_tmp();
  return FALSE;
}
