#include "../../sg.h"


short np_calc_gru(lpNPW np, short e)
{
	D_POINT axe;
	float		angle;
	short   v1, v2, f;

	e = (short)abs(e);
	if (np->efc[e].fp < 0) {
		v2 = np->efr[e].bv;
		v1 = np->efr[e].ev;
		f  = np->efc[e].fm;
	} else {
		v1 = np->efr[e].bv;
		v2 = np->efr[e].ev;
		f  = np->efc[e].fp;
	}
	angle = np->gru[e] - (float)(M_PI);
	dpoint_sub(&np->v[v2], &np->v[v1], &axe);
	o_rot(&np->p[f].v, &axe, angle, &np->p[0].v);
	np->p[0].d = -(np->p[0].v.x*np->v[v1].x +
								 np->p[0].v.y*np->v[v1].y +
			           np->p[0].v.z*np->v[v1].z);
	return (0);
}

BOOL np_constr_to_approch(lpNPW np, float angle)
{
	short edge;
	short fp,fm;
	float gru;

//	angle = angle*M_PI/180;                      //   
  for (edge = 1 ; edge <= np->noe ; edge++) {
		if (np->efc[edge].fp == 0 || np->efc[edge].fm == 0) continue;
//				np->efr[edge].el = ST_TD_CONSTR;       //  
//		else {
			if ( !(np->efr[edge].el & ST_TD_CONSTR) ) continue; //   
			gru = 0;
			if (np->efc[edge].fp < 0) gru = np->gru[-np->efc[edge].fp];
			if (np->efc[edge].fm < 0) gru = np->gru[-np->efc[edge].fm];
			if (gru == 0) {
				fm = np->efc[edge].fm;
				fp = np->efc[edge].fp;
				if ((gru=np_gru1(np,edge,&np->p[fp].v,&np->p[fm].v)) == 0) continue;
			}
			if ( M_PI - angle <= gru && gru <= angle + M_PI)  {
				np->efr[edge].el &= (~ST_TD_CONSTR);  //  
				np->efr[edge].el |= ST_TD_APPROCH;    //  
			}
//		}
	}
	return TRUE;
}

BOOL np_approch_to_constr(lpNPW np, float angle)
{
	short edge;
	short fp,fm;
	float gru;

//	angle = angle*M_PI/180;                      //   
	for (edge = 1 ; edge <= np->noe ; edge++) {
		if (np->efc[edge].fp == 0 || np->efc[edge].fm == 0) continue;
//				np->efr[edge].el = ST_TD_CONSTR;       //  
//		else {
			if ( np->efr[edge].el & ST_TD_CONSTR ) continue; // 
//			if ( !(np->efr[edge].el & ST_TD_APPROCH || np->efr[edge].el == 0) )
			gru = 0;
			if (np->efc[edge].fp < 0) gru = np->gru[-np->efc[edge].fp];
			if (np->efc[edge].fm < 0) gru = np->gru[-np->efc[edge].fm];
			if (gru == 0) {
				fm = np->efc[edge].fm;
				fp = np->efc[edge].fp;
				gru = np_gru1(np,edge,&np->p[fp].v,&np->p[fm].v);
			}
			if ( fabs(gru-M_PI) < 1.e-5)
				np->efr[edge].el = ST_TD_DUMMY;
			else
				if ( gru < M_PI - angle || gru > angle + M_PI)  {
					np->efr[edge].el = ST_TD_CONSTR;  //  
				} else
					if ( np->efr[edge].el == 0 ) np->efr[edge].el = ST_TD_APPROCH;
//		}
	}
	return TRUE;
}

void np_define_edge_label(lpNPW np, float angle)
{
	short edge;
	short fp,fm;
	float gru;

	for (edge = 1 ; edge <= np->noe ; edge++) {
		if ( np->efr[edge].el ) continue;						 //   
		if (np->efc[edge].fp == 0 || np->efc[edge].fm == 0) continue; //  
		gru = 0;
		if (np->efc[edge].fp < 0) gru = np->gru[-np->efc[edge].fp];
		if (np->efc[edge].fm < 0) gru = np->gru[-np->efc[edge].fm];
		if (gru == 0) {
			fm = np->efc[edge].fm;
			fp = np->efc[edge].fp;
			gru = np_gru1(np,edge,&np->p[fp].v,&np->p[fm].v);
		}
		if ( fabs(gru-M_PI) < 1.e-5) {
			np->efr[edge].el = ST_TD_DUMMY;
			continue;
		}
		if ( gru < M_PI - angle || gru > angle + M_PI)
			np->efr[edge].el = ST_TD_CONSTR;  //  
		else
			np->efr[edge].el = ST_TD_APPROCH;
	}
}

BOOL np_cplane(lpNPW np)
{
	short   face;

	if (np->nov <= 0 ||
			np->noe <= 0 ||
			np->nof <= 0 ||
			np->noc <= 0 ) {
      np_handler_err(NP_BAD_MODEL);  /*     */
			return FALSE;
	}
	for (face = 1; face <= np->nof; face++) {
		if (!(np_cplane_face(np,face))) return FALSE;    /*    */
	}
  return TRUE;
}

BOOL np_cplane_face(lpNPW np, short face)
{
	short     v;
	short     e1, e2, edge_cnt;
	short     contur, contur_cnt;
	short     ip;
	D_POINT c = {0.,0.,0.}, r, a, b, d;

	contur = np->f[face].fc;
	if (contur <= 0 || contur > np->noc) goto err;
  v = np->efr[(int)abs(np->c[contur].fe)].ev;
	if (v <= 0 || v > np->nov) goto err;
	r = np->v[v];
	contur_cnt = 0;

	while (contur > 0) {                  //    
    if (++contur_cnt > np->noc) goto err;
		e1 = np->c[contur].fe;
		if (e1 == 0 || abs(e1) > np->noe) goto err;
		v = BE(e1,np);                             // v -   e1
		if (v <= 0 || v > np->nov) goto err;
		dpoint_sub(&(np->v[v]), &r, &b);
		e2 = e1;
		edge_cnt = 0;

		while (TRUE) {                       //    
			if (++edge_cnt > np->noe) goto err;
			a = b;
			v = EE(e2,np);                            // v -   e2
			e2 = SL(e2,np);
			if (e2 == 0 || abs(e2) > np->noe ||
					v  <= 0 ||       v > np->nov ) goto err;
			dpoint_sub(&(np->v[v]), &r, &b);
			dvector_product(&a, &b, &d);
			c.x = c.x + d.x;
			c.y = c.y + d.y;
			c.z = c.z + d.z;
      if ( e2 == e1) break;
    }

    contur = np->c[contur].nc;
    if (contur > np->noc || contur < 0) goto err;
		if (contur == 0) break;
  }
	ip = dnormal_vector (&c);
	np->p[face].v = c;
	if (ip == FALSE) {
    np_handler_err(NP_BAD_PLANE);                   //  
		return FALSE;
	}
	np->p[face].d =  - r.x*np->p[face].v.x - r.y*np->p[face].v.y
									 - r.z*np->p[face].v.z;
	return TRUE;

err:
	np_handler_err(NP_BAD_MODEL);            //    
	return FALSE;
}

float np_gru(lpNPW np, short r, lpD_POINT f1, lpD_POINT f2)
{
	float gru;
	if ( (gru=np_gru1(np, r, f1, f2)) == 0 ) {
		np_handler_err(NP_GRU);
	}
	return gru;
}
float np_gru1(lpNPW np, short r, lpD_POINT f1, lpD_POINT f2)
{
	short v1,v2;
	sgFloat cs,alfa,ip;
	D_POINT r1,r2;

	cs = dskalar_product(f1, f2);
	if (cs <= eps_n - 1.) goto met;
	if (cs >= 1. - eps_n) alfa = M_PI;
	else {
		alfa = acos(cs);
		v1 = BE(r,np);
		v2 = EE(r,np);
		dpoint_sub(&np->v[v2],&np->v[v1],&r1);     //   r
		if (!(dnormal_vector(&r1))) goto met;      // 
		dvector_product(f1, f2, &r2);   //    
		if (!(dnormal_vector(&r2)))  goto met;     // 
		ip = dskalar_product(&r1, &r2);
		if (ip < 0.) alfa = M_PI - alfa;
		else         alfa = M_PI + alfa;
	}
	return (float)(alfa);
met:
	return (0);
}

void np_init(lpNP np)
{
	lpNPW npw;

  np->nov = np->noe = np->nof = np->noc = 0;
  np->nov_tmp = np->maxnov;
	memset(np->v,0,sizeof(D_POINT)*(np->maxnov+1));
	memset(np->efr,0,sizeof(EDGE_FRAME)*(np->maxnoe+1));
	if (np->type == TNPF) return;

	memset(np->efc,0,sizeof(EDGE_FACE)*(np->maxnoe+1));
	memset(np->f,0,sizeof(FACE)*(np->maxnof+1));
	memset(np->c,0,sizeof(CYCLE)*(np->maxnoc+1));
	memset(np->gru,0,sizeof(GRU)*(np->maxnoe+1));
	if (np->type == TNPB) return;

	npw = (lpNPW)np;
	memset(npw->p,0,sizeof(D_PLANE)*(npw->maxnof+1));

  np_gab_init(&npw->gab);
}

void np_inv(lpNPW np, REGIMTYPE regim)
{
	EDGE_FACE  *efc;
	D_POINT    *f;
	CYCLE      *c;
	short        i,to,from,nxt;

	for (i=1 ; i <= np->noe ; i++) {
		efc     = &np->efc[i];
		nxt     = efc->fp;
		efc->fp = efc->fm;
		efc->fm = nxt;
		nxt     = - efc->ep;
		efc->ep = - efc->em;
		efc->em = nxt;
		if (efc->fp < 0)  efc->ep = -efc->ep;
		if (efc->fm < 0)  efc->em = -efc->em;
	}
	if (regim == NP_ALL) {
  	for (i=1 ; i <= np->noe ; i++) {
			if (np->gru[i] == 0) continue;
			np->gru[i] = (float)(M_PI*2) - np->gru[i];
		}
	}
	for (i=1 ; i <= np->noc ; i++) {
		c = &np->c[i];
		if (c->fe == 0) continue;
		c->fe = - c->fe;
		to =  c->fe;
		from=SL(to,np);
		do {
            efc = &np->efc[(int)abs(from)];
			if (from > 0) {
				nxt   = efc->ep;
				efc->ep = to;
			} else {
				nxt   = efc->em;
				efc->em = to;
			}
			to   = from;
			from = nxt;
		} while (to != c->fe);
	}
	if (np->type == TNPW) {
		for (i=1 ; i <= np->nof ; i++) {
			f = &np->p[i].v;
			f->x = - f->x;
			f->y = - f->y;
			f->z = - f->z;
		}
	}
}

short np_max(lpNPW np)
{
	short       nof, noe, nov, noc;
	short       edge, fedge, iedge, v, face, loop;
	short       fp, fm, fs;
	float     gu;
	lpNP_USER_GRU user;
	NP_BIT_FACE *bo;
	NP_BIT_FACE *bfp, *bfm;
	char      *lv, *lc;

	nof = noe = nov = noc =0;

  if ( (lv = (char*)SGMalloc(sizeof(char)*(np->nov+1))) == NULL) return 0;
	if ( (lc = (char*)SGMalloc(sizeof(char)*(np->noc+1))) == NULL) {
		SGFree(lv);
    return 0;
	}

	user = (lpNP_USER_GRU)np->user;  // !!!!     BO !!!
	memset(user,0,sizeof(NP_USER_GRU)*(np->maxnoe+1));

//    

	face = 1;
	while (1) {
sv:
		loop = np->f[face].fc;
		lc[loop] = 1;   // 
		noc++;
		do {
			if (lc[loop] == 0 ) { lc[loop] = 1; noc++; }
			edge = fedge = np->c[loop].fe; v = BE(edge,np);
			do {
				if ( lv[v] == 0 ) { lv[v] = 1; nov++; }
				iedge = abs(edge);
				bo = (NP_BIT_FACE*)&np->efr[iedge].el;
				if (bo->flg == 0)  { bo->flg = 1; noe++; }
				edge = SL(edge,np); v = BE(edge,np);
			} while (edge != fedge);
			loop = np->c[loop].nc;
		} while (loop > 0);

		if (nov <= MAXNOV && noe <= MAXNOE && noc <= MAXNOC && nof < MAXNOF) {
			bfp = (NP_BIT_FACE*)&np->f[face].fl;
			bfp->met = 1;
			nof++;
			if (nof == MAXNOF) goto gru;
		} else  goto gru;
//       

		for (face = 1; face <= np->nof; face++) {
			bfp = (NP_BIT_FACE*)&np->f[face].fl;
			if (bfp->met == 0)	continue;
			loop = np->f[face].fc;
			do {
				edge = fedge = np->c[loop].fe;
				do {
					if (edge < 0)	fs = np->efc[-edge].fp;
					else          fs = np->efc[ edge].fm;
					if (fs > 0) {   //   
						bfp = (NP_BIT_FACE*)&np->f[fs].fl;
						if (bfp->met == 0)	{ face = fs; goto sv; }
					}
					edge = SL(edge,np); //nb v = BE(edge,np);
				} while (edge != fedge);
				loop = np->c[loop].nc;
			} while (loop > 0);
		}
		goto end;        //   
	}
//   
gru:
	for (edge = 1; edge <= np->noe ; edge++) {
		fp = np->efc[edge].fp;
		fm = np->efc[edge].fm;
		if (fp <= 0 || fm <= 0) continue;
		bfp = (NP_BIT_FACE*)&np->f[fp].fl;
		bfm = (NP_BIT_FACE*)&np->f[fm].fl;
		if (bfp->met == bfm->met) continue;
		if ((gu = np_gru(np,edge,&np->p[fp].v,&np->p[fm].v)) == 0) {
			nof = 0;
			goto end;
		}
		user[edge].bnd = gu;
		user[edge].ind = np->ident;
	}
end:
	for (edge=1 ; edge <= np->noe ; edge++) {
		bo = (NP_BIT_FACE*)&np->efr[edge].el;
		bo->flg = 0;
	}
	SGFree(lc);
	SGFree(lv);
	return nof;
}

BOOL np_del_sv( lpNP_STR_LIST  list_str, short *num_np , lpNPW np )
{
	short           i;
	lpNP_BIT_FACE b;
	BOOL          rt = TRUE;
	NP_BIT_FACE *bf;


	if ( !np_new_user_data(np) ) return FALSE;
	while (1)	{
		if ( np->nof == np_max(np) ) {          //   
			for (i=1; i <= np->nof; i++) {
				bf=(NP_BIT_FACE*)&np->f[i].fl;
				bf->met = 0;
				bf->flg = 0;
			}
			if ( !np_put( np, list_str ) ) rt = FALSE;
			goto end;
		}
		if ( !np_write_np_tmp(np) ) { rt = FALSE; goto end; } //  

		if ( !np_del_face(np) ) { rt = FALSE; goto end; }     //  
		if ( !np_del_edge(np) ) { rt = FALSE; goto end; }
		np_del_vertex(np);
		np_del_loop(np);

		//    
		if ( !np_put(np, list_str) ) { rt = FALSE; goto end; }

		if ( !np_read_np_tmp(np) ) { rt = FALSE; goto end; }
		np->ident = (*num_np)++;

		for (i=1; i <= np->nof; i++) {
			b = (NP_BIT_FACE*)&np->f[i].fl;
			b->met = ( b->met == 1 ) ? 0 : 1;
		}
		if ( !np_del_face(np) ) { rt = FALSE; goto end; }
		if ( !np_del_edge(np) ) { rt = FALSE; goto end; }
		np_del_vertex( np );
		np_del_loop  ( np );
	}
end:
	np_free_user_data(np);
	np_free_np_tmp();
	return rt;
}

short  np_pv(lpNPW np1, short r, lpD_PLANE pl)
{
	short    v1,v2;
	short    gr,p;
	D_POINT x2,x3;
	sgFloat  dl,f;

	v1 = BE(r,np1);  v2 = EE(r,np1);
	if (r > 0)     gr = np1->efc[ r].fp;
	else           gr = np1->efc[-r].fm;
	if (gr < 0) gr = 0;
	dpoint_sub(&(np1->v[v2]), &(np1->v[v1]), &x2);
	dnormal_vector (&x2);
	dl = 1;  //eps_d*1.e7;
	dvector_product(&(np1->p[gr].v),&x2, &x3);
	x3.x *= dl; 	x3.y *= dl; 	x3.z *= dl;
	x3.x += np1->v[v1].x;
	x3.y += np1->v[v1].y;
	x3.z += np1->v[v1].z;
	f = (pl->v.x*x3.x + pl->v.y*x3.y + pl->v.z*x3.z + pl->d);
	p = sg(f,eps_d);
	return p;
}

