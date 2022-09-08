#include "../sg.h"

/************************************************
 *            --- void b_del ---
 *  
 ************************************************/
BOOL b_del(lpNP_STR_LIST list_str, lpNPW np, lpNP_STR str)
{
	short       i,k,met,max=0;
	BO_BIT_EDGE *bo;
	NP_BIT_FACE *bf;
	NP_STR      str_n;
	short       ident,edge,face,index;
	short    *mem_ident;//nb  = NULL;

	

	if ( !(b_as(np)) ) return FALSE;
	if ( !(b_fa(np)) ) return FALSE;

	if ( (mem_ident = (short*)SGMalloc(sizeof(np->ident)*(np->noe))) == NULL )
		return FALSE;

	
	for (i = 1 ; i <= np->noe ; i++) {
		if (np->efc[i].fp >= 0 && np->efc[i].fm >= 0) continue;
		if (np->efc[i].fp < 0) {
			ident = np->efc[i].ep;
			face  = np->efc[i].fm;
			edge = i;
		} else {
			ident = np->efc[i].em;
			face  = np->efc[i].fp;
			edge = -i;
		}
        bo = (BO_BIT_EDGE*)&np->efr[(int)abs(i)].el;
		if (bo->flg != 0) {                 
			if (edge > 0)  met = bo->ep;
			else           met = bo->em;
			met = b_face_label(met);
		} else {
			bf = (NP_BIT_FACE*)&np->f[face].fl; 
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
		if ( !(np_get_str(list_str,ident,&str_n,&index)) ) goto err;
		if (str_n.lab == 0 || str_n.lab == -2)  {
			str_n.lab = met;                            
			write_elem(&list_str->vdim,index,&str_n);
			if (object_number == 2 && key == SUB) {
				if (!(read_np(&list_str->bnp,str_n.hnp,npb))) goto err;
				np_inv(npb,NP_ALL);
				if ( !(overwrite_np(&list_str->bnp,str_n.hnp,npb,TNPW)) ) goto err;
			}
		}
		continue;
	}
	SGFree(mem_ident);

	
	if (object_number == 2 && key == SUB) np_inv(np,NP_ALL);
	if ( !(np_del_face(np)) ) return FALSE;
	if (np->nof == 0) {
		str->lab = -1;                     
		return TRUE;
	}
//	if ( brept1 ) 	b_move_gru(np);
 	b_move_gru(np);
	if ( !(np_del_edge(np)) ) return FALSE;
	np_del_vertex(np);
	np_del_loop(np);
	for (i=1 ; i <= np->noe ; i++) {
		bo = (BO_BIT_EDGE*)&np->efr[i].el;
		bo->flg = 0;
		bo->em  = 0;
		bo->ep  = 0;
	}

	if ( !b_sv(np) )  str->ort |= ST_SV;             
	else if (np->nov > MAXNOV || np->noe > MAXNOE ||
					 np->nof > MAXNOF || np->noc > MAXNOC) {
		str->ort |= ST_MAX;              
	}
	return TRUE;
err:
	SGFree(mem_ident);
	return FALSE;
}

void b_move_gru(lpNPW np)
{
	short i;
	lpBO_USER us;
	sgFloat con = c_smooth_angle;   
//	sgFloat con = d_radian(c_smooth_angle);   

	us = (lpBO_USER)np->user;
	for (i = 1 ; i <= np->noe ; i++) {
		if (np->efc[i].fp > 0 && np->efc[i].fm <= 0 && us[i].bnd > 0) {
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
		}
		if (np->efc[i].fm > 0 && np->efc[i].fp <= 0 && us[i].bnd > 0) {
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
}
