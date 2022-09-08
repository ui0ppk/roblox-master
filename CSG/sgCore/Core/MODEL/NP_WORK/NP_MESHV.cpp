#include "../../sg.h"

static  void	act_edge_removal(short *inde, short *num_act, short act_edge);
static  short		np_cr_face(void);
static  short		find_np_ver(short *indv, short v);
static  short		find_np_edge( short bv, short ev);
static  short		find_near_tr(short lv1, short lv2, lpVDIM vdtri, lpNPTRI tr);
static  short   find_free_tr(lpVDIM vdtri, lpNPTRI tr);
//static 	void  np_put_zero(short *inde,short num_act);
static  short		make_label( UCHAR constr, short i);
static  short		make_label_edge( lpNPTRI tri, short lv1, short lv2);


#pragma argsused
BOOL np_mesh3v(lpNP_STR_LIST list, lpNPTRP trp, short *ident, lpVDIM coor, sgFloat angle,
							lpVDIM vdtri)
{
	register short i,j;
	short*		indv = NULL;		//     Vdim
	short*		inde;//nb = NULL;		//     
	short			num_act = 0;	// -   
	short			cur_tr;//nb	= 0;	//   
	short			v1,v2,v3,nof,edge1,edge2,act_edge,lv1,lv2;
	NPTRI			tr;
	D_POINT		v;
	int     	num_proc = -1;
	short     edge;
	short     old_next_edge;
	short     old_v;

	if ((inde = (short*)SGMalloc(sizeof(*inde)*(MAXNOE+1))) == NULL) goto err;
	if ((indv = (short*)SGMalloc(sizeof(*indv)*(MAXNOV+1))) == NULL) goto err;

//	num_proc = start_grad(GetIDS(IDS_SG318), vdtri->num_elem);
new_np:                      //  
	num_act = 0;               // -   
	npwg->type = TNPW;
	np_init((lpNP)npwg);
	npwg->ident = *ident;
	(*ident)++;
	//  1- 
  i = find_free_tr(vdtri, &tr);
	if ( i == vdtri->num_elem ) goto end;//   
//  step_grad (num_proc , i);
	//    
	np_cr_face(); //   
	npwg->nov=3;
	npwg->noe=3;
	for ( i=0; i<3; i++ ) {
		read_elem(coor,tr.v[i],&v); //  
		npwg->v[i+1]	= v;             //
        if (trp)
        {
            npwg->vertInfo[i+1].r = trp->color[i].x;
            npwg->vertInfo[i+1].g = trp->color[i].y;
            npwg->vertInfo[i+1].b = trp->color[i].z;
        }
		indv[i+1]			= tr.v[i];
		inde[i]				= i+1;                //    

		npwg->efr[i+1].bv = i+1;    //  
		npwg->efr[i+1].ev = i+2;
		npwg->efc[i+1].ep = i+2;
		npwg->efc[i+1].em = 0;
		npwg->efc[i+1].fp = 1;
		npwg->efc[i+1].fm = 0;
		npwg->efr[i+1].el = make_label(tr.constr,i);
	}
	npwg->efr[3].ev = 1;          //  
	npwg->efc[3].ep = 1;
	npwg->f[1].fc = 1;            // 
	npwg->f[1].fl = 0;
	npwg->c[1].nc = 0;
	npwg->c[1].fe = 1;
	num_act = 3;
m1:
	if ( npwg->nov > MAXNOV/2 ||
			 npwg->noe > MAXNOE-5 ||
			 npwg->nof > MAXNOF-5 ) {
			//  
		if ( !np_cplane(npwg) ) goto err;      //  - -
		np_define_edge_label(npwg, (float)angle);   //      
																								//   
//		if ( angle != 0 ) np_approch_to_constr(npwg, (float)angle);   // .  
//		np_put_zero(inde,num_act);        //  0   . 
		if ( !(np_put(npwg,list)) ) goto err;
		goto new_np;
	}
next:                   //   
	if ( num_act <= 0 ) { //    -   
								 			  //     
		if (npwg->nov > 0) {
			if ( !np_cplane(npwg) ) 
				goto err;      //  - -
			np_define_edge_label(npwg, (float)angle);   //      
																									//   
//			if ( angle != 0 ) np_approch_to_constr(npwg, (float)angle);   // .  
//			np_put_zero(inde,num_act);        //  0   . 
			if ( !(np_put(npwg,list)) )
				goto err;
		}
		goto new_np;
	}
	//     
	act_edge = inde[0];
	num_act--;
	memcpy(inde,&inde[1],sizeof(*inde)*num_act);
	v1=npwg->efr[act_edge].bv;  //    
	v2=npwg->efr[act_edge].ev;
	lv1=indv[v1];               //    Vdim
	lv2=indv[v2];
	old_next_edge = abs(npwg->efc[act_edge].ep);
	old_v = npwg->efr[old_next_edge].ev;   //    
	//     
	if ( (cur_tr = find_near_tr(lv1, lv2, vdtri,&tr)) == -1 ) 
		goto next;//  .
	//    
	nof = np_cr_face(); //   
	npwg->c[nof].fe = -act_edge;  //   - 
	npwg->efc[act_edge].fm = nof;
	for ( j=0; j<3; j++ ) {
		if ( tr.v[j] != lv1 && tr.v[j] != lv2) {
	// J -   
			if ( ( v3 = find_np_ver(indv,tr.v[j])) == -1 ) {
				//  
				read_elem(coor,tr.v[j],&v); //  
				v3=++npwg->nov;
				npwg->v[v3]=v;              //
                if (trp)
                {
                    npwg->vertInfo[v3].r = trp->color[j].x;
                    npwg->vertInfo[v3].g = trp->color[j].y;
                    npwg->vertInfo[v3].b = trp->color[j].z;
                }

				indv[v3]=tr.v[j];           //     Vdim
				//  1 
				edge1=++npwg->noe;
				npwg->efr[edge1].bv = v1;    //  
				npwg->efr[edge1].ev = v3;
				npwg->efc[edge1].ep = edge1+1;
				npwg->efc[edge1].em = 0;
				npwg->efc[edge1].fp = nof;
				npwg->efc[edge1].fm = 0;
				npwg->efr[edge1].el = make_label_edge(&tr,lv1,tr.v[j]);
				npwg->efc[act_edge].em = edge1; //  
				inde[num_act]=edge1;            //    
				num_act++;
				//  2 
				edge2=++npwg->noe;
				npwg->efr[edge2].bv = v3;    //  
				npwg->efr[edge2].ev = v2;
				npwg->efc[edge2].ep = -act_edge;
				npwg->efc[edge2].em = 0;
				npwg->efc[edge2].fp = nof;
				npwg->efc[edge2].fm = 0;
				npwg->efr[edge2].el = make_label_edge(&tr,lv2,tr.v[j]);
				inde[num_act]=edge2;            //    
				num_act++;
			}
			else{ //  V3  ,    .
				if ( v3 == old_v) { //  .     
					npwg->efc[act_edge].fm = 0;
					npwg->nof--;
					npwg->noc--;
					goto next;
				}
				edge1 = find_np_edge( v1, v3);
				edge2 = find_np_edge( v3, v2);
				if ( edge1  != 0) {
					edge=abs(edge1);
						if ( ( edge1 > 0 ) ||
						   ((npwg->efc[edge].em != 0) && (npwg->efc[edge].ep != 0))) {
m_err:				 // !!      !
								npwg->efc[act_edge].fm = 0;
								npwg->nof--;
								npwg->noc--;
								{
									lpNPTRI tri;
									tri = (NPTRI*)get_elem(vdtri,cur_tr);
									tri->constr &= 0x7f;
								// cur_tr -   
								}
								num_act =  0;
								goto next;
						}
				}
				if ( edge2  != 0) {
					edge=abs(edge2);
						if ( ( edge2 > 0 ) ||
						   ((npwg->efc[edge].em != 0) && (npwg->efc[edge].ep != 0))) {
							 // 
								goto m_err;
						}
				}
				if ( edge1  == 0) {
					//  1 
					edge1=++npwg->noe;
					npwg->efr[edge1].bv = v1;    //  
					npwg->efr[edge1].ev = v3;
//				npwg->efc[edge1].ep = ??;
					npwg->efc[edge1].em = 0;
					npwg->efc[edge1].fp = nof;
					npwg->efc[edge1].fm = 0;
					npwg->efr[edge1].el = make_label_edge(&tr,lv1,tr.v[j]);
					npwg->efc[act_edge].em = edge1; //  
					inde[num_act]=edge1;            //    
					num_act++;
				}
				else{ //  1  
					if ( edge1 < 0 ) {   //   V3  V1  
//						npwg->efc[-edge1].em = ??;
						npwg->efc[-edge1].fm = nof;
						npwg->efc[act_edge].em = edge1; //  
					}
					else {               //   V1  V3   - !!!
						goto err2;
//						npwg->efc[edge1].fp = nof;
//						npwg->efc[act_edge].ep = edge1; //  
					}
					//      
					act_edge_removal(inde, &num_act, (short)(-edge1));
				}
				if ( edge2 == 0) {
					//  2 
					edge2=++npwg->noe;
					npwg->efr[edge2].bv = v3;    //  
					npwg->efr[edge2].ev = v2;
					npwg->efc[edge2].ep = -act_edge;
					npwg->efc[edge2].em = 0;
					npwg->efc[edge2].fp = nof;
					npwg->efc[edge2].fm = 0;
					npwg->efr[edge2].el = make_label_edge(&tr,lv2,tr.v[j]);
					if (edge1 < 0 )
						npwg->efc[-edge1].em = edge2; //  
					else
						npwg->efc[edge1].ep  = edge2; //  
					inde[num_act]=edge2;            //    
					num_act++;
				}
				else{ //  2  
					if ( edge2 < 0 ) {   //   V2  V3  
						npwg->efc[-edge2].em = -act_edge;
						npwg->efc[-edge2].fm = nof;
						if (edge1 < 0 )
							npwg->efc[-edge1].em = edge2; //  
						else
							npwg->efc[edge1].ep  = edge2; //  
					}
					else {               //   V2  V3   - !!!!
						goto err2;
//						npwg->efc[edge1].ep = -act_edge;
//						npwg->efc[edge1].fp = nof;
//						if (edge1 < 0 )
//							npwg->efc[-edge1].em = edge2; //  
//						else
//							npwg->efc[edge1].ep  = edge2; //  
					}
					//      
					act_edge_removal(inde, &num_act, (short)(-edge2));
				}
			}
		}
	}
	goto m1;
end:
	lv1 = TRUE;
me:
	if ( inde ) SGFree(inde);
	if ( indv ) SGFree(indv);

//	end_grad  (num_proc , vdtri->num_elem);
	return lv1;
err2:
	np_handler_err(NP_BAD_MODEL);
err:
	lv1 = FALSE;
	goto me;
}
static  short make_label_edge( lpNPTRI tri, short lv1, short lv2)
{
	short	label;

	if ( lv1 == tri->v[0] ) {
		if ( lv2 == tri->v[1] ) label = 0; // 0- 
		else										label = 2; // 2- 
	} else {
		if ( lv1 == tri->v[1] ) {
			if ( lv2 == tri->v[2] ) label = 1; // 0- 
			else										label = 0; // 2- 
		} else {
			if ( lv2 == tri->v[0] ) label = 2; // 0- 
			else										label = 1; // 2- 
		}
	}
	return make_label(tri->constr,label);
}

static  short make_label( UCHAR constr, short i)
{
	constr >>= i<<1;
	constr &= 3;
	switch ( constr )
	{
		case TRP_CONSTR:	return ST_TD_CONSTR;
		case TRP_APPROCH:	return ST_TD_APPROCH;
		case TRP_DUMMY:		return ST_TD_DUMMY;
//		default:					return ST_TD_APPROCH;
		default:					return 0;   //      
																	//       ( == 0 ).
	}
}

static  void act_edge_removal(short *inde, short *num_act, short act_edge)
{
	register short i;
	for (i = 0; i < *num_act; i++) {
		if (inde[i] == act_edge) {
			(*num_act)--;
			memcpy(&inde[i],&inde[i+1],sizeof(*inde)*(*num_act-i));
			return;
		}
	}
	//  
}
static  short np_cr_face()
{
	npwg->noc++;
	npwg->nof++;
	npwg->f[npwg->nof].fc = npwg->noc;
	npwg->f[npwg->nof].fl = 0;
	npwg->c[npwg->nof].nc = 0;
	npwg->c[npwg->nof].fe = 0;
  return npwg->nof;
}

static  short  find_np_ver(short *indv, short v)
{
	register short i;
	for (i = 1; i <= npwg->nov; i++) {
		if (indv[i] == v) return i;
	}
	return -1;
}

static  short find_np_edge( short bv, short ev)
{
	register short i;
	for (i = 1; i <= npwg->noe; i++) {
		if (npwg->efr[i].bv == bv && npwg->efr[i].ev == ev) return i;
		if (npwg->efr[i].bv == ev && npwg->efr[i].ev == bv) return -i;
	}
	return 0;
}

static  short  find_near_tr(short lv1, short lv2, lpVDIM vdtri, lpNPTRI tr)
{
	register short i,j;
	char		c = 0;
	lpNPTRI trtmp;

	begin_rw(vdtri,0);
	for ( i=0; i< vdtri->num_elem; i++ ,c = 0){
		trtmp = (NPTRI*)get_next_elem(vdtri);
		if ( (trtmp->constr&0x80) ) continue;
		for ( j=0; j<3; j++ ) {
			if ( trtmp->v[j] == lv1 ) c |= 1;
			if ( trtmp->v[j] == lv2 ) c |= 2;
		}
		if ( c != 3 ) continue;
		*tr = *trtmp;
		trtmp->constr |= 0x80;  //   
		break;
	}
	end_rw(vdtri);
	if ( c == 3 ) return	i;
	else					return -1;
}

static  short  find_free_tr(lpVDIM vdtri, lpNPTRI tr)
{
	register short i;
	lpNPTRI trtmp;

	begin_rw(vdtri,0);
	for ( i=0; i< vdtri->num_elem; i++ ){
		trtmp = (NPTRI*)get_next_elem(vdtri);
		if ( (trtmp->constr & 0x80) ) continue;
		*tr = *trtmp;
		trtmp->constr |= 0x80;  //   
		break;
	}
	end_rw(vdtri);
	return	i;
}

