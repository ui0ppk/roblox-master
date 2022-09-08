#include "../sg.h"

//int  b_np_max(lpNPW np);
static BOOL b_big_face(lpNPW np);
static BOOL b_put_index(hINDEX *hindex, short *num_index,lpNP_STR_LIST list_str,
								 lpNPW np, lpNP_STR str);

BOOL b_del_sv(lpNP_STR_LIST list_str, lpNP_STR str)
{

	short         i,j,jjj;
	short         bv_npw, ev_npw, bv_npd, ev_npd;
	short         num_index, num_index2;
	short         old_ident,ident;
	short         max,k;
	VNP				    vnpd;
	lpNPW         npw, npd;
	hINDEX        hindex, hindex2;
//	SVTYPE        status = BO_FIRST;
	lpNP_STR_LIST list, list2, listd;
	lpNP_BIT_FACE b;
	BOOL          rt = TRUE;
	short*	 			mem_ident;
	NP_STR 				str_new;

	if (object_number == 1) {
		hindex = hindex_a;  num_index = num_index_a;
		hindex2 = hindex_b; num_index2 = num_index_b;
		list = &lista;       list2 = &listb;
	}	else {
		hindex = hindex_b; num_index = num_index_b;
		hindex2 = hindex_a; num_index2 = num_index_a;
		list = &listb;  list2 = &lista;
	}
	if (!(read_np(&list_str->bnp,str->hnp,npa))) return FALSE;
	if ( !b_big_face(npa) ) return FALSE;	

	mem_ident = (short*)SGMalloc(sizeof(npa->ident)*(npa->noe));

	str_new = *str;
	npw = npa;
	for (;;) {
		if ( np_max(npw) == npa->nof )	
			break;         		 						
		if ( !np_write_np_tmp(npw) ) 	{ rt = FALSE; goto end; }  
		if ( !(np_del_face(npw)) ) 		{ rt = FALSE; goto end; }    
		b_move_gru(npw);
		if ( !(np_del_edge(npw)) ) 		{ rt = FALSE; goto end; }
		np_del_vertex(npw);
		np_del_loop(npw);
		if ( !(overwrite_np(&list_str->bnp,str_new.hnp,npw,TNPW)) )  
			{ rt = FALSE; goto end; }

		if ( !np_read_np_tmp(npw) ) { rt = FALSE; goto end; }       

		for (i=1 ; i <= npw->nof ; i++) {                           
			b = (NP_BIT_FACE*)&npw->f[i].fl;
			if (b->met == 1) b->met=0;
			else             b->met=1;
		}
		if ( !(np_del_face(npw)) ) { rt = FALSE; goto end; }        
		b_move_gru(npw);
		if ( !(np_del_edge(npw)) ) { rt = FALSE; goto end; }
		np_del_vertex(npw);
		np_del_loop(npw);

	
		old_ident = npw->ident;
		npw->ident = ident_c++;
		if ( !b_put_index(&hindex,&num_index,list,npw,&str_new) ) { rt = FALSE; goto end;}

// met:
		max = 0;
		for (i=1 ; i <= npw->noe ; i++) {               
			if (npw->efc[i].fp >= 0 && npw->efc[i].fm >= 0) continue;
			if (npw->efc[i].fp < 0) ident = npw->efc[i].ep;
			else                    ident = npw->efc[i].em;
			k = 0;
			while (k < max) {
				if (mem_ident[k] == ident) break;   
				k++;
			}
			if (k < max) continue;                
			mem_ident[max] = ident;
			max++;
			listd = list;
			vnpd = b_get_hnp(hindex, num_index, ident);
			if (vnpd == 0) {
				listd = list2;
				if ( !(vnpd=b_get_hnp(hindex2,num_index2,ident)) ) {
					put_message(INTERNAL_ERROR,GetIDS(IDS_SG019),0);
					rt = FALSE;
					goto end;
				}
			}
			if (!(read_np(&listd->bnp,vnpd,npb))) { rt = FALSE;goto end; }
			npd = npb;
			for (j=1 ; j <= npd->noe ; j++) {
				if (npd->efc[j].ep == old_ident || npd->efc[j].em == old_ident) {
					for (jjj=i ; jjj <= npw->noe ; jjj++) {
						if (npw->efc[jjj].ep == npd->ident ||
								npw->efc[jjj].em == npd->ident) {
							bv_npw = npw->efr[jjj].bv;
							ev_npw = npw->efr[jjj].ev;
							bv_npd = npd->efr[j].bv;
							ev_npd = npd->efr[j].ev;
							if ( !(dpoint_eq(&npw->v[bv_npw],&npd->v[bv_npd],eps_d)) &&
									 !(dpoint_eq(&npw->v[bv_npw],&npd->v[ev_npd],eps_d))) continue;
							if ( !(dpoint_eq(&npw->v[ev_npw],&npd->v[bv_npd],eps_d)) &&
									 !(dpoint_eq(&npw->v[ev_npw],&npd->v[ev_npd],eps_d))) continue;
							if (npd->efc[j].ep == old_ident) {
								npd->efc[j].ep = npw->ident;
								break;
							}
							if (npd->efc[j].em == old_ident) {
								npd->efc[j].em = npw->ident;
								break;
							}
						}
					}
				}
			}
			if ( !(overwrite_np(&listd->bnp,vnpd,npb,TNPW)) )
			{ rt = FALSE; goto end; }
		}
		if ( !(overwrite_np(&list_str->bnp,str_new.hnp,npw,TNPW)) ) { rt = FALSE; goto end;}
	}
end:
	
	if (object_number == 1) { hindex_a = hindex; num_index_a = num_index; }
	else                    { hindex_b = hindex; num_index_b = num_index; }

	if ( mem_ident ) SGFree(mem_ident);
	np_free_np_tmp();
	return rt;
}

static BOOL b_big_face(lpNPW np)
{
	short        noe, nov, noc;
	short        edge, fedge, iedge, v, face, loop;
	BO_BIT_EDGE *bo;
	NP_BIT_FACE *bfp;
	char        *lv, *lc;
	BOOL        rt = TRUE;

	noe = nov = noc =0;

	lv = (char*)SGMalloc(sizeof(char)*(np->maxnov+1));
	lc = (char*)SGMalloc(sizeof(char)*(np->maxnoc+1));

	for (face = 1; face <= np->nof; face++) {  
		bfp = (NP_BIT_FACE*)&np->f[face].fl;
		bfp->met = 0;
		loop = np->f[face].fc;
		lc[loop] = 1;   
		noc++;
		do {
			if (lc[loop] == 0 ) { lc[loop] = 1; noc++; }
			edge = fedge = np->c[loop].fe; v = BE(edge,np);
			do {
				if ( lv[v] == 0 ) { lv[v] = 1; nov++; }
				iedge = abs(edge);
				bo = (BO_BIT_EDGE*)&np->efr[iedge].el;
				if (bo->flg == 0)  { bo->flg = 1; noe++; }
				edge = SL(edge,np); v = BE(edge,np);
			} while (edge != fedge);
			loop = np->c[loop].nc;
		} while (loop > 0);

		if (nov >= MAXNOV || noe > MAXNOE || noc >= MAXNOC ) { 
			if ( !b_divide_face(np, face) ) { rt = FALSE; goto end; }
			face--;
		}
		noe = nov = noc = 0;
		memset(lv,0,sizeof(char)*(np->maxnov+1));
		memset(lc,0,sizeof(char)*(np->maxnoc+1));
	}
end:
	for (edge=1 ; edge <= np->noe ; edge++) {
		bo = (BO_BIT_EDGE*)&np->efr[edge].el;
		bo->flg = 0;
	}
	SGFree(lc);
	SGFree(lv);
	return rt;
}

static BOOL b_put_index(hINDEX *hindex, short *num_index,
												lpNP_STR_LIST list_str, lpNPW np, lpNP_STR str)
{
	short     i, number;
//	NP_STR  str;
	VNP     vnp;
	hINDEX  hindex_new;
	lpINDEX index, index_new;

	
	dpoint_minmax(&np->v[1], np->nov, &np->gab.min, &np->gab.max);
	memset(str,0,sizeof(NP_STR));
	str->ident = np->ident;
	str->lab = 1;
	memcpy(&str->gab,&np->gab,sizeof(REGION_3D));
	if ( !(vnp = write_np(&list_str->bnp,np,TNPW)) ) return FALSE;
	str->hnp = vnp;
	if ( !( add_elem(&list_str->vdim,str)) ) return FALSE;
	list_str->number_all++;

	
	number = list_str->number_all - 1;
	if ( *num_index == number)  {                 
		hindex_new = SGMalloc(sizeof(INDEX)*(number+10));
		index_new = (lpINDEX)hindex_new;
		index = (lpINDEX)(*hindex);
		memcpy(index_new,index,sizeof(INDEX)*(*num_index));
		for (i=*num_index ; i < *num_index+10 ; i++)
			index_new[i].ident = MAXSHORT;
		SGFree(*hindex);
		*num_index += 10;
		index = index_new;
		*hindex = hindex_new;
	} else index = (lpINDEX)(*hindex);

	index[number].ident = np->ident;
	index[number].hnp   = vnp;
	return TRUE;
}
