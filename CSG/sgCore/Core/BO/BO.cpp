#include "../sg.h"


typedef int (*fptr)(const void*, const void*);
static OSCAN_COD bo_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);
hINDEX b_index_np(lpNP_STR_LIST list, lpREGION_3D gab);
static int sort_function( const short *a, const short *b);
BOOL b_list_del(lpNP_STR_LIST list, hOBJ hobj);
BOOL b_list_lable(lpNP_STR_LIST list, hOBJ hobj);
static short b_np_label(NP_TYPE_POINT met);

static WORD app_idn_a;
static lpNP_STR_LIST list_str = NULL;

short         key, object_number, ident_c;
lpNPW         npa = NULL, npb = NULL;
VADR          hbo_mem = NULL;
hINDEX        hindex_a = NULL, hindex_b = NULL;
NP_STR_LIST   lista,listb;
VLD           vld_c;
int           num_index_a, num_index_b;
REGION_3D     gab_c = {{1.e35,1.e35,1.e35},{-1.e35,-1.e35,-1.e35}};
BOOL          brept1 = TRUE;   
BOOL 					Bo_Put_Message = TRUE;
lpLISTH			 	bo_line_listh; 			

OSCAN_COD boolean_operation(hOBJ hobj_a, hOBJ hobj_b, CODE_OPERATION code_operation,
														lpLISTH listh)
{
	int         	i, ii;
  int						num_proc = -1;
	NP_STR        str1 , str2;
	REGION_3D     gab_a = {{1.e35,1.e35,1.e35},{-1.e35,-1.e35,-1.e35}};
	REGION_3D     gab_b = {{1.e35,1.e35,1.e35},{-1.e35,-1.e35,-1.e35}};
	hOBJ          hobj_c = NULL;
	lpOBJ         obj_a, obj_b;
	lpGEO_BREP    lpgeobrep_a, lpgeobrep_b;
	WORD  			  app_idn_b;
	int*      	  b;
	BOOL          line_inter = FALSE;
	SCAN_CONTROL  sc;
	OSCAN_COD			rt = OSTRUE;
	NP_TYPE_POINT	answer;

	flg_exist = 1;
	if (hobj_a == NULL || hobj_b == NULL ||
			code_operation < UNION || code_operation > LINE_INTER /*PENETRATE*/) {
		put_message(EMPTY_MSG,GetIDS(IDS_SG028),0);
		return OSBREAK;
	}
//  if (code_operation == PENETRATE) point.met=0;
	obj_a = (lpOBJ)hobj_a;
	obj_b = (lpOBJ)hobj_b;
	lpgeobrep_a = (lpGEO_BREP)obj_a->geo_data;
	lpgeobrep_b = (lpGEO_BREP)obj_b->geo_data;
	if ( lpgeobrep_a->type == SURF ) brept1 = FALSE;
	else 								             brept1 = TRUE;
	if ( code_operation == LINE_INTER ) {
		if ( lpgeobrep_a->type == FRAME ||	lpgeobrep_b->type == FRAME ) {
			put_message(EMPTY_MSG,GetIDS(IDS_SG029),0);
			return OSBREAK;
		}
		goto bo;
	}
/*	if ( lpgeobrep_b->type != BODY ) {           
		put_message(EMPTY_MSG,GetIDS(IDS_SG030),0);
		return OSFALSE;
	}
*/
	if ( code_operation == UNION && lpgeobrep_a->type != BODY ) {
		put_message(EMPTY_MSG,GetIDS(IDS_SG031),0);
		return OSBREAK;
	}
bo:
//	init_listh(&line_listh);
	bo_line_listh = listh;
	gab_a.min = lpgeobrep_a->min;
	gab_a.max = lpgeobrep_a->max;
	gab_b.min = lpgeobrep_b->min;
	gab_b.max = lpgeobrep_b->max;

	
	f3d_gabar_project1(lpgeobrep_a->matr,&gab_a.min,&gab_a.max,
										 &gab_a.min,&gab_a.max);
	f3d_gabar_project1(lpgeobrep_b->matr,&gab_b.min,&gab_b.max,
										 &gab_b.min,&gab_b.max);

	np_gab_un(&gab_a, &gab_b, &gab_c);
	define_eps(&(gab_c.min), &(gab_c.max)); 
	if ((np_gab_inter(&gab_a,&gab_b)) == TRUE) {
		if ( Bo_Put_Message )
			put_message(EMPTY_MSG,GetIDS(IDS_SG032),1);
		if (code_operation == UNION) 	return OSFALSE;
		if (code_operation == SUB) { 
			if ( !o_copy_obj(hobj_a, &hobj_c,"") ) return OSFALSE;
			obj_a = (lpOBJ)hobj_c;
			
			obj_a->status &= (~(ST_SELECT|ST_BLINK));
			attach_item_tail(listh, hobj_c);
		}
		return rt;
	}
	key = code_operation;

	app_idn_b = lpgeobrep_b->ident_np + 32767;   
	app_idn_a = 32767 - lpgeobrep_a->ident_np;   
	if (app_idn_a < app_idn_b) {
		put_message(IDENT_OVER,NULL,0);             
		return OSBREAK;
	}
	app_idn_a = lpgeobrep_a->ident_np + 32767; 
	ident_c = lpgeobrep_a->ident_np + app_idn_b;

	init_scan(&sc);
	sc.user_geo_scan    = bo_scan;
	read_user_data_buf  = bo_read_user_data_buf;
	write_user_data_buf = bo_write_user_data_buf;
	read_user_data_mem  = bo_read_user_data_mem;   	
	add_user_data_mem   = bo_add_user_data_mem;    		
	size_user_data      = bo_size_user_data;
	expand_user_data    = bo_expand_user_data;

	init_vld(&vld_c);

	

/*	if (npwg->maxnov < 2*MAXNOV) nov = 2*MAXNOV - npwg->maxnov;
	else                         nov = 0;
	if (npwg->maxnoe < 2*MAXNOE) noe = 2*MAXNOE - npwg->maxnoe;
	else                         noe = 0;
	if (npwg->maxnof < 2*MAXNOF) nof = 2*MAXNOF - npwg->maxnof;
	else                         nof = 0;
	if (npwg->maxnoc < 2*MAXNOC) noc = 2*MAXNOC - npwg->maxnoc;
	else                         noc = 0;
	if ( !(o_expand_np(npwg, nov, noe, nof, noc)) ) return NULL;
*/
	npwg->type = TNPW;
	np_init(npwg);
	object_number = 1;
	if ( !np_init_list(&lista) )
    {
        rt = OSBREAK; goto norm;
    }
	list_str = &lista;
	if ( (rt = o_scan(hobj_a,&sc)) != OSTRUE )  goto norm;
	if (ctrl_c_press) {												 
		put_message(CTRL_C_PRESS, NULL, 0);
		rt = OSBREAK;
		goto norm;
	}
	
	if ((hindex_a = b_index_np(&lista,&gab_b)) == NULL)
    {
        rt = OSBREAK; goto norm;
    }
	num_index_a = lista.number_all;

	object_number = 2;
	if ( !np_init_list(&listb) )
    {
        rt = OSBREAK; goto norm;
    }
	list_str = &listb;
	if ( (rt = o_scan(hobj_b,&sc)) != OSTRUE )  goto norm;
	if (ctrl_c_press) {												
		put_message(CTRL_C_PRESS, NULL, 0);
		rt = OSBREAK;
		goto norm;
	}
	
	if ((hindex_b = b_index_np(&listb,&gab_a)) == NULL)
    {
        rt = OSBREAK; goto norm;
    }
	num_index_b = listb.number_all;

	
	if ((rt = (OSCAN_COD)np_str_form(np_gab_inter, &lista, &listb)) != OSTRUE)
        goto norm;
    
	np_gab_init(&gab_c);          

	npa = npwg;
	npwg->type = TNPW;
	if ( !(bo_new_user_data(npa)) )
    {
        rt = OSBREAK; goto norm;
    }
	if ( (npb = creat_np_mem(TNPW,MAXNOV,MAXNOE,MAXNOF,MAXNOF,MAXNOE))
				== NULL )
    {
        rt = OSBREAK; goto norm;
    }
	np_init(npb);
	if ( !(bo_new_user_data(npb)) )
    {
        rt = OSBREAK; goto norm;
    }

//	if ( !tb_setcolor(IDXRED) ) { rt = OSFALSE; goto norm; }
//	num_proc = start_grad(GetIDS(IDS_SG033), lista.number_all);

	for (ii=0; ii<lista.number_all; ii++) {
//		step_grad (num_proc , ii);
		read_elem(&lista.vdim,ii,&str1);
		if ( str1.len == 0 || abs(str1.lab) == 1 )  continue; 
		if (!(read_np(&lista.bnp,str1.hnp,npa)))
        {
            rt = OSBREAK; goto norm;
        }

		begin_rw(&lista.nb, str1.b);
		for (i=0 ; i < str1.len ; i++) {
			b = (int*)get_next_elem(&lista.nb);
			read_elem(&listb.vdim,*b,&str2);
			if (!(read_np(&listb.bnp,str2.hnp,npb)))
            {
                rt = OSBREAK; goto norm;
            }
			if ( !(b_lnp(&str1,&str2)) ) {  		   
				end_rw(&lista.nb);   ;//nb b = NULL;
				rt = OSSKIP;
				goto norm;
			}
			if (str2.lab == 2) { 
				line_inter = TRUE;
				if ( !(overwrite_np(&listb.bnp,str2.hnp,npb,TNPW)) )
                {
                    rt = OSBREAK; goto norm;
                }
			}
			write_elem(&listb.vdim,*b,&str2);
		}
		end_rw(&lista.nb);   //nb b = NULL;
		if (str1.lab == 2)  {
			if ( !(overwrite_np(&lista.bnp,str1.hnp,npa,TNPW)) )
            {
                rt = OSBREAK; goto norm;
            }
			line_inter = TRUE;
		}
		write_elem(&lista.vdim,ii,&str1);
		if (ctrl_c_press) {											
			put_message(CTRL_C_PRESS, NULL, 0);
			rt = OSBREAK;
			goto norm;
		}
	}
	read_user_data_mem  = NULL;
	add_user_data_mem   = NULL;

//	end_grad  (num_proc , ii);
	num_proc = -1;

	if ( code_operation == LINE_INTER ) {
		if ( listh->num ) {  								
			hobj_c = make_group(listh, TRUE);
			init_listh(listh);
			attach_item_tail(listh, hobj_c);
		}
		goto norm;
	}
	
	if ( line_inter == FALSE ) {              
		if ( code_operation == UNION ) {
//			object_number = 2;
			read_elem(&listb.vdim,0, &str1);
			if (!(read_np(&listb.bnp, str1.hnp, npa))) 
            {
                rt = OSBREAK; goto norm;
            }
			if ( !check_point_object(&npa->v[1], hobj_a, &answer) ) 
            {
                rt = OSBREAK; goto norm;
            }
			if ( answer == NP_IN ) goto fr; 
//			object_number = 1;
			read_elem(&lista.vdim,0, &str1);
			if (!(read_np(&lista.bnp, str1.hnp, npa)))
            {
                rt = OSBREAK; goto norm;
            }
			if ( !check_point_object(&npa->v[1], hobj_b, &answer) )
            {
                rt = OSBREAK; goto norm;
            }
			if ( answer == NP_IN ) { 
				if ( !o_copy_obj(hobj_b, &hobj_c,"") )
                {
                    rt = OSBREAK; goto norm;
                }
				obj_a = (lpOBJ)hobj_c;
				
				obj_a->status &= (~(ST_SELECT|ST_BLINK));
				attach_item_tail(listh, hobj_c);
				goto norm;
			} else {
                rt = OSSKIP;
				if ( Bo_Put_Message )
					put_message(EMPTY_MSG,GetIDS(IDS_SG034),1);
				goto norm;
			}
		} else {
//			object_number = 1;
			read_elem(&lista.vdim,0, &str1);
			if (!(read_np(&lista.bnp, str1.hnp, npa)))
            {
                rt = OSBREAK; goto norm;
            }
			if ( !check_point_object(&npa->v[1], hobj_b, &answer) )
            {
                rt = OSBREAK; goto norm;
            }
		}

        if (answer==NP_IN && (code_operation==SUB || code_operation==INTER))
        {
            rt = OSCONTAINS;
    		goto norm; 
        }

        if ( answer == NP_IN )
            goto norm;

        if (answer==NP_OUT && (code_operation==SUB || code_operation==INTER))
        {
    		goto norm; 
        }
fr:
		if ( !o_copy_obj(hobj_a, &hobj_c,"") )
        {
            rt = OSBREAK; goto norm;
        }
		obj_a = (lpOBJ)hobj_c;
		
		obj_a->status &= (~(ST_SELECT|ST_BLINK));
		attach_item_tail(listh, hobj_c);
		goto norm;
	}

	object_number = 1;
	if ( !(b_list_del(&lista, hobj_a)) )
    {
        rt = OSBREAK; goto norm;
    }
	if (ctrl_c_press) { 											  
		put_message(CTRL_C_PRESS, NULL, 0);
		rt = OSBREAK;
		goto norm;
	}

	if ( brept1 ) {
		object_number = 2;
		if ( !(b_list_del(&listb, hobj_b)) ) { rt = OSBREAK; goto norm; }
		if (ctrl_c_press) {											 
			put_message(CTRL_C_PRESS, NULL, 0);
			rt = OSBREAK;
			goto norm;
		}
	}
	//   

	object_number = 1;

	for (ii=0; ii<lista.number_all; ii++) {
		read_elem(&lista.vdim,ii,&str1);
		if ((str1.ort & ST_SV) || (str1.ort & ST_MAX)) {
			if ( !(b_del_sv(&lista, &str1)) )
            {
                rt = OSBREAK; goto norm;
            }
			str1.ort = 0;
			write_elem(&lista.vdim,ii,&str1);
		}
		if (ctrl_c_press) { 										
			put_message(CTRL_C_PRESS, NULL, 0);
			rt = OSBREAK;
			goto norm;
		}
	}

	if ( brept1 ) {
		object_number = 2;

		for (ii=0; ii<listb.number_all; ii++) {
			read_elem(&listb.vdim,ii,&str2);
			if ( (str2.ort & ST_SV) || (str2.ort & ST_MAX) ) {
				if ( !(b_del_sv(&listb, &str2)) )
                {
                    rt = OSBREAK; goto norm;
                }
				str2.ort = 0;
				write_elem(&listb.vdim,ii,&str2);
			}
			if (ctrl_c_press) { 											
				put_message(CTRL_C_PRESS, NULL, 0);
				rt = OSBREAK;
				goto norm;
			}
		}
	}

	
 	rt = (OSCAN_COD)b_form_model_list(listh, hobj_a);

norm:
//	tb_clear();
//	if (num_proc != -1 )  end_grad  (num_proc ,ii);
	bo_free_user_data(npwg);
	bo_free_user_data(npb);
	free_np_mem(&npb);
	if (hindex_a != NULL) { SGFree(hindex_a); hindex_a = NULL; }
	if (hindex_b != NULL) { SGFree(hindex_b); hindex_b = NULL; }
	if ( lista.number_all > 0) np_end_of_put(&lista,NP_CANCEL,0,NULL);
	if ( listb.number_all > 0) np_end_of_put(&listb,NP_CANCEL,0,NULL);
	read_user_data_buf  = NULL;
	write_user_data_buf	= NULL;
	size_user_data      = NULL;
	expand_user_data    = NULL;
	read_user_data_mem  = NULL;
	add_user_data_mem   = NULL;
	np_free_stack();
	return rt;
}
#pragma argsused
static OSCAN_COD bo_scan(hOBJ hobj,lpSCAN_CONTROL lpsc)
{
	register  short i;
	lpD_POINT v;

	if (ctrl_c_press) { 												
		put_message(CTRL_C_PRESS, NULL, 0);
		return OSBREAK;
	}
	if (lpsc->type != OBREP) return OSTRUE; 

	npwg->type = TNPW;
	v = npwg->v;
	for (i = 1; i <= npwg->nov ; i++) {     
		if ( !o_hcncrd(lpsc->m,&v[i],&v[i])) goto err;
	}
	if ( !np_cplane(npwg) ) goto err;        
	if (object_number == 2) {  								  
		npwg->ident += app_idn_a;
		for (i=1 ; i <= npwg->noe ; i++) {
			if (npwg->efc[i].fp < 0) { npwg->efc[i].ep += app_idn_a; continue; }
			if (npwg->efc[i].fm < 0) { npwg->efc[i].em += app_idn_a; continue; }
		}
	}
	if ( np_put(npwg,list_str) )   return OSTRUE;
err:
	return OSFALSE;
}

hINDEX b_index_np(lpNP_STR_LIST list, lpREGION_3D gab)
{
	short      number_np,i;
	lpNP_STR str;
	hINDEX   hindex;//nb  = NULL;
	lpINDEX  index;

	number_np = list->number_all;       
	if ( (hindex = SGMalloc(sizeof(INDEX)*number_np)) == NULL) 		return NULL;
	index = (lpINDEX)hindex;

	begin_rw(&list->vdim,0);
	for (i=0; i<list->number_all; i++) {             
		str = (NP_STR*)get_next_elem(&list->vdim);
		index[i].ident = str->ident;
		index[i].hnp   = str->hnp;
		if ((np_gab_inter(&str->gab,gab)) == TRUE) {
			
    	str->lab = -2;

		}
		write_elem(&list->vdim,i,str);
	}
	end_rw(&list->vdim);
	qsort((void*)index,i,sizeof(index[0]),(fptr)sort_function);
	return hindex;
}
int sort_function( const short *a, const short *b)
{
	if ( (*(INDEX*)a).ident < (*(INDEX*)b).ident )   return -1;
	if ( (*(INDEX*)a).ident == (*(INDEX*)b).ident )  return 0;
	return 1;
}

VNP b_get_hnp(hINDEX hindex, short num, short ident)
{
	VNP     hnp;
	lpINDEX index, p;

	index = (lpINDEX)hindex;
	p = (lpINDEX)bsearch(&ident,index,num,sizeof(index[0]),(fptr)sort_function);
	if ( !p ) return 0;
	hnp = p->hnp;
	return hnp;
}

BOOL b_list_del(lpNP_STR_LIST list, hOBJ hobj)
{
	short     ii, pr = 0;
	NP_STR	str;

//	num_proc = start_grad(GetIDS(IDS_SG035), lista.number_all);
	for (ii=0; ii<list->number_all; ii++) {
//		step_grad (num_proc , ii);
		read_elem(&list->vdim,ii,&str);
		if ( str.lab == 2 )  {               
			if (!(read_np(&list->bnp,str.hnp,npa))) goto err;
			
			if ( !(b_del(list,npa, &str)) ) goto err;
			if (npa->nof == 0) str.lab = -1;     
			else             { str.lab =  1;       
				if ( !(overwrite_np(&list->bnp,str.hnp,npa,TNPW)) ) goto err;
			}
			write_elem(&list->vdim,ii,&str);
		} else pr = 1;
		if (ctrl_c_press) { 											
			put_message(CTRL_C_PRESS, NULL, 0);
			goto err;
		}
	}
//	end_grad  (num_proc , ii);
	if ( pr == 0 ) return TRUE;
	if ( b_list_lable(list, hobj) ) return TRUE;     
err:
//	end_grad  (num_proc , ii);
	return FALSE;
}

BOOL b_list_lable(lpNP_STR_LIST list, hOBJ hobj)
{
	short    				ii;
	NP_STR 				str;
	short    				count_old, count_np;
	NP_TYPE_POINT	answer;

	/*nb count_old = */count_np = 0;
	list->number_np = list->number_all;
	do {
		count_old = count_np;    count_np = 0;

		for (ii=0; ii<list->number_all; ii++) {
			read_elem(&list->vdim,ii,&str);
			if (abs(str.lab) == 1) {  
				count_np++ ;
				continue;
			}
			str.lab = 0;
			if (!(read_np(&list->bnp,str.hnp,npa))) return FALSE;
			if ( !(b_np_lab(list,&str,npa)) ) return FALSE;
			if (str.lab != 0)  {
				if (str.lab == 1 && object_number == 2 && key == SUB)
					if ( !(overwrite_np(&list->bnp,str.hnp,npa,TNPW)) ) return FALSE;
				write_elem(&list->vdim,ii,&str);
				count_np++;
			}
		}
	} while (count_np != list->number_all && count_np != count_old);

	if (count_np != list->number_all) {
		for (ii=0; ii<list->number_all; ii++) {
			read_elem(&list->vdim,ii,&str);
			if (abs(str.lab) == 1) {  
				continue;
			}
			if (!(read_np(&list->bnp,str.hnp,npa))) return FALSE;
			if ( !check_point_object(&npa->v[1], hobj, &answer) ) return FALSE;
			str.lab = b_np_label(answer);
			if (str.lab != 0)  {
				if (str.lab == 1 && object_number == 2 && key == SUB)
					if ( !(overwrite_np(&list->bnp,str.hnp,npa,TNPW)) ) return FALSE;
				write_elem(&list->vdim,ii,&str);
//nb 				count_np++;
			}
		}
	}
	return TRUE;
}

static short b_np_label(NP_TYPE_POINT met)
{
	switch (key) {
		case  UNION:
			switch (met) {
				case NP_OUT:
					return 1;
				case NP_IN:
					return -1;
				default:
					return 0;
			}
		case INTER:
			switch (met) {
				case NP_OUT:
					return -1;
				case NP_IN:
					return 1;
				default:
					return 0;
			}
		case  SUB:
				if (object_number == 1) {
					if (met == NP_OUT) return 1;
					else               return -1;
				} else if (met == NP_OUT) return -1;
							 else          			return 1;
		default:
		return 0;
	}
}
