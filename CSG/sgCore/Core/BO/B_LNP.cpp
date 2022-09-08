#include "../sg.h"

#include <vector>

static BOOL b_up(void);
static int is_faces_equals(lpNPW np1, short face1, lpNPW np2, short face2,
						   short&  edge_on_1, short&  edge_on_2);

static bool is_faces_on_one_plane(lpNPW np1, short face1, lpNPW np2, short face2);

short            facea,faceb;
NP_VERTEX_LIST vera = {0,0,NULL,NULL};
NP_VERTEX_LIST verb = {0,0,NULL,NULL};

BOOL b_lnp(lpNP_STR str1,lpNP_STR str2)
{
	short i;
  NP_BIT_FACE *b;
	BO_BIT_EDGE *be;
	lpDA_POINT  w1, w2;
	BOOL        rt=FALSE;//nb  = TRUE;

	if ( !(rt = np_ver_alloc(&vera)) ) goto end;
	if ( !(rt = np_ver_alloc(&verb)) ) goto end;

	i=0;
	for (faceb = 1 ; faceb <= npb->nof ; faceb++) {
		b = (NP_BIT_FACE*)&npb->f[faceb].fl;
		if (np_gab_pl(&npa->gab,&npb->p[faceb],eps_d)) {
			b->flg = 1;           
			i++;                  
		} else
			b->flg = 0;
	}
	if (i == npb->nof) goto end;

		for (facea = 1 ; facea <= npa->nof ; facea++) {
			if (np_gab_pl(&npb->gab,&npa->p[facea],eps_d))  
			continue;
			for (faceb = 1 ; faceb <= npb->nof ; faceb++) {
				b = (NP_BIT_FACE*)&npb->f[faceb].fl;
				if (b->flg == 1) continue;
				if (is_faces_on_one_plane(npa,facea,npb,faceb)) 
				{
					continue;
				}
				
				vera.uk = 0;    verb.uk = 0;
				npa->nov_tmp = npa->maxnov;
				npb->nov_tmp = npb->maxnov;
				if ( !(rt=np_pl(npa,facea,&vera,&npb->p[faceb])) ) goto end;
				if (vera.uk == 0) continue;     /*   */
				if ( !(rt=np_pl(npb,faceb,&verb,&npa->p[facea])) ) goto end;
				if (verb.uk == 0) continue;     /*   */
				np_axis = np_defc(&npa->p[facea].v,&npb->p[faceb].v);
				np_sort_ver(npa->v,&vera,np_axis);
//				qsort((void*)vera.v,vera.uk,sizeof(VERTEX),(fptr)sort_function);
				np_sort_ver(npb->v,&verb,np_axis);
//				qsort((void*)verb.v,verb.uk,sizeof(VERTEX),(fptr)sort_function);
				w1 = (lpDA_POINT)npa->v;       
				w2 = (lpDA_POINT)npb->v;
				if (w1[vera.v[0].v][np_axis] > w2[verb.v[verb.uk-1].v][np_axis] ||
						w2[verb.v[0].v][np_axis] > w1[vera.v[vera.uk-1].v][np_axis] ) continue;
				
				if ( !(rt=b_up()) ) 
				{
					rt=FALSE;
					goto end;
				}
			}
		}

		if (str1->lab != 2) {                   
			for (i = 1 ; i <= npa->noe ; i++) {
				be = (BO_BIT_EDGE*)&npa->efr[i].el;
				if (be->flg == 0) continue;
				str1->lab = 2;                     
				break;
			}
		}
		if (str2->lab != 2) {                    
			for (i = 1 ; i <= npb->noe ; i++) {
				be = (BO_BIT_EDGE*)&npb->efr[i].el;
				if (be->flg == 0) continue;
				str2->lab = 2;                     
				break;
			}
		}
end:
	np_free_ver(&vera);
	np_free_ver(&verb);
  return rt;
}

BOOL b_up(void)
{
	short pa = -1, pb = -1;
	short pra = -1,prb = -1;
	short pta = -1,ptb = -1;
	short uka = 0, ukb = 0;
	short maxnova, maxnovb, nova, novb, i;
	lpDA_POINT w1, w2;
	BOOL lea=FALSE,leb=FALSE; 
														
	maxnova = npa->maxnov;  nova = npa->nov;
	maxnovb = npb->maxnov;  novb = npb->nov;
//	w1 = (lpDA_POINT)npa->v;
//	w2 = (lpDA_POINT)npb->v;
	do {
		w1 = (lpDA_POINT)npa->v;
		w2 = (lpDA_POINT)npb->v;
		if (maxnova < npa->maxnov) {        
			w1 = (lpDA_POINT)npa->v;
			for (i = uka ; i < vera.uk ; i++) {
				if ( vera.v[i].v > nova ) vera.v[i].v += MAXNOV/4;
			}
			maxnova = npa->maxnov;  nova = npa->nov;
		}
		if (maxnovb < npb->maxnov) {         
			w2 = (lpDA_POINT)npb->v;
			for (i = ukb ; i < verb.uk ; i++) {
				if ( verb.v[i].v > novb ) verb.v[i].v += MAXNOV/4;
			}
			maxnovb = npb->maxnov;  novb = npb->nov;
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
				fabs(w1[vera.v[uka-1].v][np_axis] - w2[verb.v[ukb].v][np_axis]) > eps_d)
			if ( !(b_vr(uka,ukb,lea,leb)) ) 
				return FALSE;
	} while (uka < vera.uk  &&  ukb < verb.uk);
	return TRUE;
}

static int is_faces_equals(lpNPW np1, short face1, lpNPW np2, short face2,
						   short&  edge_on_1, short&  edge_on_2)
{
	D_POINT*  face1_vert, *face1_vert_2;
	D_POINT*  face2_vert/*, *face2_vert_2*/;

	int  check_res = 0;

	short loop_1 = np1->f[face1].fc;
	short fedge_1 = np1->c[loop_1].fe;
	short v_1 = BE(fedge_1,np1);
	short edge_1 = fedge_1;

	D_POINT* eq_pnts[2];

	face1_vert = &(np1->v[v_1]);
	do {

		short loop_2 = np2->f[face2].fc;
		short fedge_2 = np2->c[loop_2].fe;
		short v_2 = BE(fedge_2,np2);
		short edge_2 = fedge_2;

		face2_vert = &(np2->v[v_2]);
		do {

			if (dpoint_eq(face1_vert,face2_vert,eps_d))
			{
				check_res++;
				if (check_res==1)
				{
					eq_pnts[0] = face1_vert;
				}
				else
				{
					if (check_res==2)
					{
						eq_pnts[1] = face1_vert;
					}
				}
				break;
			}
			if (check_res>2)
				return 3;

			edge_2 = SL(edge_2,np2); 
			v_2 = BE(edge_2,np2); 
			face2_vert = &(np2->v[v_2]);
		} while (edge_2 != fedge_2);


		edge_1 = SL(edge_1,np1); 
		v_1 = BE(edge_1,np1); 
		face1_vert = &(np1->v[v_1]);
	} while (edge_1 != fedge_1);

	if (check_res==2)
	{
		loop_1 = np1->f[face1].fc;
		fedge_1 = np1->c[loop_1].fe;
		edge_1 = fedge_1;

		do {

			face1_vert = &(np1->v[BE(edge_1,np1)]);
			face1_vert_2 = &(np1->v[EE(edge_1,np1)]);

			if ((dpoint_eq(face1_vert,eq_pnts[0],eps_d) && dpoint_eq(face1_vert_2,eq_pnts[1],eps_d))||
				(dpoint_eq(face1_vert,eq_pnts[1],eps_d) && dpoint_eq(face1_vert_2,eq_pnts[0],eps_d)))
			{
				edge_on_1 = edge_1;
				break;
			}

			edge_1 = SL(edge_1,np1); 
		} while (edge_1 != fedge_1);

		loop_1 = np2->f[face2].fc;
		fedge_1 = np2->c[loop_1].fe;
		edge_1 = fedge_1;

		do {

			face1_vert = &(np2->v[BE(edge_1,np2)]);
			face1_vert_2 = &(np2->v[EE(edge_1,np2)]);

			if ((dpoint_eq(face1_vert,eq_pnts[0],eps_d) && dpoint_eq(face1_vert_2,eq_pnts[1],eps_d))||
				(dpoint_eq(face1_vert,eq_pnts[1],eps_d) && dpoint_eq(face1_vert_2,eq_pnts[0],eps_d)))
			{
				edge_on_2 = edge_1;
				break;
			}

			edge_1 = SL(edge_1,np2); 
		} while (edge_1 != fedge_1);

	}

	return (check_res);
}

static bool is_faces_on_one_plane(lpNPW np1, short face1, lpNPW np2, short face2)
{
	return ((fabs(npa->p[facea].v.x - npb->p[faceb].v.x) <= eps_n &&
		fabs(npa->p[facea].v.y - npb->p[faceb].v.y) <= eps_n &&
		fabs(npa->p[facea].v.z - npb->p[faceb].v.z) <= eps_n) ||
		(fabs(npa->p[facea].v.x + npb->p[faceb].v.x) <= eps_n &&
		fabs(npa->p[facea].v.y + npb->p[faceb].v.y) <= eps_n &&
		fabs(npa->p[facea].v.z + npb->p[faceb].v.z) <= eps_n)/* ||
		fabs(dskalar_product(&npa->p[facea].v,&npb->p[faceb].v))>1.0-0.0001*/);
}
