#include "../sg.h"

#define FF(r,m)   ((r) > 0 ? m->efc[r].fm : m->efc[-r].fp)

static BOOL check_up(lpNPW np1, lpNPW np2);
static BOOL check_ff1(lpNPW np);
static BOOL check_ff2(lpNPW np1, lpNPW np2);

BOOL check_np(lpNPW np1, lpNPW np2, lpNP_STR str1,lpNP_STR str2)
{
	short         i;
	NP_BIT_FACE *b;
	BO_BIT_EDGE *be;
	lpDA_POINT  w1, w2;
	BOOL        rt;//nb  = TRUE;

	if ( !(rt=np_ver_alloc(&vera)) ) goto end;
	if ( !(rt=np_ver_alloc(&verb)) ) goto end;

	if (np1 != np2) {
		i=0;
		for (faceb = 1 ; faceb <= np2->nof ; faceb++) {
			b = (NP_BIT_FACE*)&np2->f[faceb].fl;
			if (np_gab_pl(&np1->gab,&np2->p[faceb],eps_d) == TRUE) {
				b->flg = 1;          
				i++;                  
			} else	b->flg = 0;
		}
		if (i == np2->nof) return TRUE;
	}

	for (facea = 1 ; facea <= np1->nof ; facea++) {
		if (np_gab_pl(&np2->gab,&np1->p[facea],eps_d) == TRUE) 	continue;
		for (faceb = 1 ; faceb <= np2->nof ; faceb++) {
			if (np1 == np2)  {                  
//				if (facea == faceb) continue;
				if (facea >= faceb) continue;
				if ( check_ff1(np1) ) continue; 
			}	else {
				b = (NP_BIT_FACE*)&np2->f[faceb].fl;
				if (b->flg == 1) continue;
				if ( check_ff2(np1,np2) ) continue;
			}
			if (fabs(np1->p[facea].v.x - np2->p[faceb].v.x) <= eps_n &&
					fabs(np1->p[facea].v.y - np2->p[faceb].v.y) <= eps_n &&
					fabs(np1->p[facea].v.z - np2->p[faceb].v.z) <= eps_n ||
					fabs(np1->p[facea].v.x + np2->p[faceb].v.x) <= eps_n &&
					fabs(np1->p[facea].v.y + np2->p[faceb].v.y) <= eps_n &&
					fabs(np1->p[facea].v.z + np2->p[faceb].v.z) <= eps_n ) continue;
			vera.uk = 0;    verb.uk = 0;
			np1->nov_tmp = np1->maxnov;
			np2->nov_tmp = np2->maxnov;
			if ( !(rt=np_pl(np1,facea,&vera,&np2->p[faceb])) ) goto end;
			if (vera.uk == 0) continue;     
			if ( !(rt=np_pl(np2,faceb,&verb,&np1->p[facea])) ) goto end;
			if (verb.uk == 0) continue;     
			np_axis = np_defc(&np1->p[facea].v,&np2->p[faceb].v);
			np_sort_ver(np1->v,&vera,np_axis);
			np_sort_ver(np2->v,&verb,np_axis);
			w1 = (lpDA_POINT)np1->v;       
			w2 = (lpDA_POINT)np2->v;
			if (w1[vera.v[0].v][np_axis] > w2[verb.v[verb.uk-1].v][np_axis] ||
					w2[verb.v[0].v][np_axis] > w1[vera.v[vera.uk-1].v][np_axis] ) continue;
			//    
			if ( !(rt=check_up(np1, np2)) ) goto end;  
		}
	}
	if (str1->lab != 2) {                    
		for (i = 1 ; i <= np1->noe ; i++) {
			be = (BO_BIT_EDGE*)&np1->efr[i].el;
			if (be->flg == 0) continue;
			str1->lab = 2;                      
			break;
		}
	}
	if ( np1 != np2 ) {
		if (str2->lab != 2) {                    
			for (i = 1 ; i <= np2->noe ; i++) {
				be = (BO_BIT_EDGE*)&np2->efr[i].el;
				if (be->flg == 0) continue;
				str2->lab = 2;                      
				break;
			}
		}
	}

end:
	np_free_ver(&vera);
	np_free_ver(&verb);
	return rt;
}
static BOOL check_ff1(lpNPW np)
{
	short loop, edge, fedge;

	loop = np->f[facea].fc;
	do {
		fedge = edge = np->c[loop].fe;
		do {
			if (faceb == FF(edge,np)) return TRUE;
			edge = SL(edge,np);
		} while (edge != fedge);
		loop = np->c[loop].nc;
	} while (loop > 0);
	return FALSE;
}
static BOOL check_ff2(lpNPW np1, lpNPW np2)
{
	short v1, v2, w1, w2, ident;
	short loop1, edge1, fedge1;
	short loop2, edge2, fedge2;

	loop1 = np1->f[facea].fc;
	do {
		fedge1 = edge1 = np1->c[loop1].fe;
		do {
			if ( FF(edge1,np1) < 0 ) { 				
				if (edge1 < 0) ident = np1->efc[-edge1].ep;
				else           ident = np1->efc[ edge1].em;
				if (ident == np2->ident) {
					loop2 = np2->f[faceb].fc;
					do {
						fedge2 = edge2 = np2->c[loop2].fe;
						do {
							if ( (ident=FF(edge2,np2)) < 0 ) { 				
								if (edge2 < 0) ident = np2->efc[-edge2].em;
								else           ident = np2->efc[ edge2].ep;
								if (ident == np1->ident) {
									v1 = np1->efr[edge1].bv;
									v2 = np1->efr[edge1].ev;
									w1 = np2->efr[edge2].bv;
									w2 = np2->efr[edge2].ev;
									if (dpoint_eq(&np1->v[v1],&np2->v[w1],eps_d) &&
											dpoint_eq(&np1->v[v2],&np2->v[w2],eps_d) ||
											dpoint_eq(&np1->v[v2],&np2->v[w1],eps_d) &&
											dpoint_eq(&np1->v[v1],&np2->v[w2],eps_d))
										return TRUE;
								}
							}
							edge2 = SL(edge2,np2);
						} while (edge2 != fedge2);
						loop2 = np2->c[loop2].nc;
					} while (loop2 > 0);

					return TRUE;
				}
			}
			edge1 = SL(edge1,np1);
		} while (edge1 != fedge1);
		loop1 = np1->c[loop1].nc;
	} while (loop1 > 0);
	return FALSE;

}
static BOOL check_up(lpNPW np1, lpNPW np2)
{
	short pa = -1, pb = -1;
	short pra = -1,prb = -1;
	short pta = -1,ptb = -1;
	short uka = 0, ukb = 0;
	short maxnov1, maxnov2, nov1, nov2, i;
	lpDA_POINT w1, w2;
	BOOL lea=FALSE,leb=FALSE; 
														
	lpNP_VERTEX  vn1,vk1,vn2,vk2;
	D_POINT vn, vk;

	maxnov1 = np1->maxnov;  nov1 = np1->nov;
	maxnov2 = np2->maxnov;  nov2 = np2->nov;
	w1 = (lpDA_POINT)np1->v;
	w2 = (lpDA_POINT)np2->v;
	do {
		if (maxnov1 < np1->maxnov) {         
			w1 = (lpDA_POINT)np1->v;
			for (i = uka ; i < vera.uk ; i++) {
				if ( vera.v[i].v > nov1 ) vera.v[i].v += MAXNOV/4;
			}
			maxnov1 = np1->maxnov;  nov1 = np1->nov;
			if ( np1 == np2 ) {
				for (i = ukb ; i < verb.uk ; i++) {
					if ( verb.v[i].v > nov2 ) verb.v[i].v += MAXNOV/4;
				}
				maxnov2 = np2->maxnov;  nov2 = np2->nov;
			}
		}
		if (maxnov2 < np2->maxnov) {         
			w2 = (lpDA_POINT)np2->v;
			for (i = ukb ; i < verb.uk ; i++) {
				if ( verb.v[i].v > nov2 ) verb.v[i].v += MAXNOV/4;
			}
			maxnov2 = np2->maxnov;  nov2 = np2->nov;
		}
		if (w1[vera.v[uka].v][np_axis] < w2[verb.v[ukb].v][np_axis]) {
			np_av(vera.v[uka].m,&pa,&pra,&pta,&lea);
			uka++;
		} else {
			np_av(verb.v[ukb].m,&pb,&prb,&ptb,&leb);
			ukb++;
		}
		if ((pa + pb) == 2 &&
				fabs(w1[vera.v[uka].v][np_axis] - w2[verb.v[ukb-1].v][np_axis]) > eps_d &&
        fabs(w1[vera.v[uka-1].v][np_axis] - w2[verb.v[ukb].v][np_axis]) > eps_d) {
			  vn1 = &(vera.v[uka-1]);  vk1 = &(vera.v[uka]);
			  vn2 = &(verb.v[ukb-1]);  vk2 = &(verb.v[ukb]);
			  
				if (w1[vn1->v][np_axis] < w2[vn2->v][np_axis])
							vn = np2->v[vn2->v];
				else 	vn = np1->v[vn1->v];
				
				if (w1[vk1->v][np_axis] < w2[vk2->v][np_axis])
						 vk = np1->v[vk1->v];
				else vk = np2->v[vk2->v];
//				tb_put_otr(&vn,&vk);
				check = FALSE;
				if ( !check_vr(np1,np2, &vn, &vk, uka,ukb,lea,leb) ) return FALSE;
		}

	} while (uka < vera.uk  &&  ukb < verb.uk);
	return TRUE;
}
