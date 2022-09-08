#include "../sg.h"

static BOOL check_move_gru(lpNPW np);

BOOL check_del(lpNP_STR str)
{
	short         i,k,met,max=0;
	BO_BIT_EDGE *bo;
	NP_BIT_FACE *bf;
	NP_STR      str_n;
	short         ident,edge,face,index;
	short *   mem_ident;//nb  = NULL;

	

	if ( !(b_as(npa)) ) return FALSE;
	if ( !(b_fa(npa)) ) return FALSE;

	if ( (mem_ident = (short*)SGMalloc(sizeof(npa->ident)*(npa->noe))) == NULL)
		return FALSE;

	
	for (i = 1 ; i <= npa->noe ; i++) {
		if (npa->efc[i].fp >= 0 && npa->efc[i].fm >= 0) continue;
		if (npa->efc[i].fp < 0) {
			ident = npa->efc[i].ep;
			face  = npa->efc[i].fm;
			edge = i;
		} else {
			ident = npa->efc[i].em;
			face  = npa->efc[i].fp;
			edge = -i;
		}
        bo = (BO_BIT_EDGE*)&npa->efr[(int)abs(i)].el;
		if (bo->flg != 0) {                  
			if (edge > 0)  met = bo->ep;
			else           met = bo->em;
			met = b_face_label(met);
		} else {
			bf = (NP_BIT_FACE*)&npa->f[face].fl; 
			met = bf->met;
		}
		k = 0;
		while (k < max) {
			if (mem_ident[k] == ident) break;        
			k++;
		}
		if (k < max) continue;
		mem_ident[max] = ident;
		max++;
		if (met == 0) met = -1;
		if ( !(np_get_str(&lista, ident, &str_n, &index)) ) goto err;
		if (str_n.lab == 0 || str_n.lab == -2)  {
			str_n.lab = met;                           
			write_elem(&lista.vdim, index, &str_n);
		}
		continue;
	}
	SGFree(mem_ident);

	
	if ( !(np_del_face(npa)) ) return FALSE;
	if (npa->nof == 0) {
		str->lab = -1;                    
		return TRUE;
	}
	if ( !check_move_gru(npa) ) return FALSE;
	if ( !(np_del_edge(npa)) ) return FALSE;
	np_del_vertex(npa);
	np_del_loop(npa);
	for (i=1 ; i <= npa->noe ; i++) {
		bo = (BO_BIT_EDGE*)&npa->efr[i].el;
		bo->flg = 0;
		bo->em  = 0;
		bo->ep  = 0;
	}

	if ( !b_sv(npa) )  str->ort |= ST_SV;             
	else if (npa->nov > MAXNOV || npa->noe > MAXNOE ||
					 npa->nof > MAXNOF || npa->noc > MAXNOC) {
		str->ort |= ST_MAX;              //  > max
	}
	return TRUE;
err:
	SGFree(mem_ident);
	return FALSE;
}

static BOOL check_move_gru(lpNPW np)
{
	short 			i, j, edge, loop;
//	short 			noe, last_edge;
	lpBO_USER us;
	sgFloat 		con = c_smooth_angle;   

	us = (lpBO_USER)np->user;
	for (i = 1 ; i <= np->noe ; i++) {
		if (np->efc[i].fp > 0 && np->efc[i].fm <= 0 && us[i].bnd > 0) {
			//   -
			if ( np->ident != us[i].ind ) {
				np->gru[i] = us[i].bnd;
				np->efc[i].em = us[i].ind;
				np->efc[i].fm = -i;
				if (fabs(us[i].bnd - M_PI) < eps_n)  {
					np->efr[i].el = ST_TD_DUMMY;
					continue;
				}
				if (fabs(us[i].bnd - M_PI) < con)  {
					np->efr[i].el = ST_TD_APPROCH | ST_TD_COND;
				}
				continue;
			} else {                
				j = (short)us[i].bnd;
				if ( np->efr[i].bv == np->efr[j].bv && np->efr[i].ev == np->efr[j].ev ) {
				
					if (np->efc[j].fm == 0) {
						put_message(EMPTY_MSG,GetIDS(IDS_SG357),1);
						return FALSE;
					}
					np->efc[i].fm = np->efc[j].fm;
					np->efc[i].em = np->efc[j].em;
					j = -j;
				} else {
					if ( np->efr[i].bv == np->efr[j].ev && np->efr[i].ev == np->efr[j].bv ) {
					
						if (np->efc[j].fp == 0) {
							put_message(EMPTY_MSG,GetIDS(IDS_SG358),1);
							return FALSE;
						}
						np->efc[i].fm = np->efc[j].fp;
						np->efc[i].em = np->efc[j].ep;
					} else {
						put_message(EMPTY_MSG,GetIDS(IDS_SG359),1);
						return FALSE;
					}
				}

				for( edge = 1; edge <= np->noe ; edge++ ) {
					if ( abs(np->efc[edge].ep) == abs(j) ) {
						np->efc[edge].ep = i*isg(np->efc[edge].ep)*isg(j);
						continue;
					}
					if ( abs(np->efc[edge].em) == abs(j) ) {
						np->efc[edge].em = i*isg(np->efc[edge].em)*isg(j);
						continue;
					}
				}

                np->efc[(int)abs(j)].fp = np->efc[(int)abs(j)].ep = 0;
                np->efc[(int)abs(j)].fm = np->efc[(int)abs(j)].em = 0;
				for( loop = 1; loop <= np->noc ; loop++ ) {
					if (np->c[loop].fe == j)  {
						np->c[loop].fe = -i;
						break;
					}
				}
				continue;
			}
		}
		if (np->efc[i].fm > 0 && np->efc[i].fp <= 0 && us[i].bnd > 0) {
			
			if ( np->ident != us[i].ind ) {
				np->gru[i] = us[i].bnd;
				np->efc[i].ep = us[i].ind;
				np->efc[i].fp = - i;
				if (fabs(us[i].bnd  - M_PI) < eps_n)   {
					np->efr[i].el = ST_TD_DUMMY;
					continue;
				}
				if (fabs(us[i].bnd - M_PI) < con)  {
					np->efr[i].el = ST_TD_APPROCH | ST_TD_COND;
				}
				continue;
			} else {               
				j = (short)us[i].bnd;
				if ( np->efr[i].bv == np->efr[j].bv && np->efr[i].ev == np->efr[j].ev ) {
				
					if (np->efc[j].fp == 0) {
						put_message(EMPTY_MSG,GetIDS(IDS_SG314),1);
						return FALSE;
					}
					np->efc[i].fp = np->efc[j].fp;
					np->efc[i].ep = np->efc[j].ep;
				} else {
					if ( np->efr[i].bv == np->efr[j].ev && np->efr[i].ev == np->efr[j].bv ) {
					
						if (np->efc[j].fm == 0) {
							put_message(EMPTY_MSG,GetIDS(IDS_SG314),1);
							return FALSE;
						}
						np->efc[i].fp = np->efc[j].fm;
						np->efc[i].ep = np->efc[j].em;
						j = -j;
					} else {
						put_message(EMPTY_MSG,GetIDS(IDS_SG315),1);
						return FALSE;
					}
				}



				for( edge = 1; edge <= np->noe ; edge++ ) {
					if ( abs(np->efc[edge].ep) == abs(j) ) {
						np->efc[edge].ep = i*isg(np->efc[edge].ep)*isg(j);
						continue;
					}
					if ( abs(np->efc[edge].em) == abs(j) ) {
						np->efc[edge].em = i*isg(np->efc[edge].em)*isg(j);
						continue;
					}
				}

                np->efc[(int)abs(j)].fp = np->efc[(int)abs(j)].ep = 0;
                np->efc[(int)abs(j)].fm = np->efc[(int)abs(j)].em = 0;
				for( loop = 1; loop <= np->noc ; loop++ ) {
					if (np->c[loop].fe == j)  {
						np->c[loop].fe = i;
//						break;
					}
				}
				continue;
			}
		}
		if (np->efc[i].fm > 0 && np->efc[i].fp > 0)  {
			if (np->efr[i].el & ST_TD_APPROCH) continue;
			facea = np->efc[i].fp;
			faceb = np->efc[i].fm;
			if (fabs(np->p[facea].v.x - np->p[faceb].v.x) <= eps_n &&
					fabs(np->p[facea].v.y - np->p[faceb].v.y) <= eps_n &&
					fabs(np->p[facea].v.z - np->p[faceb].v.z) <= eps_n ) {
				np->efr[i].el = ST_TD_DUMMY;
			}
		}
	}
	return TRUE;
}
