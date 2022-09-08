#include "../sg.h"

typedef int (*fptr)(const void*, const void*);
static OSCAN_COD check_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);
static hINDEX check_index_np(void);
static int sort_function( const short *a, const short *b);
static BOOL check_list_del(void);
static BOOL check_list_lable(lpNP_STR_LIST list);

static int ii, num_proc = -1;
BOOL check = TRUE;


BOOL check_and_edit_model(hOBJ hobj, hOBJ *hnew_obj, BOOL * pr)
{
	int 				 	i, num_np;
	int *    	b;//nb = NULL;
	lpOBJ        	obj;//nb  = NULL;
	lpGEO_BREP   	lpgeobrep;
	SCAN_CONTROL 	sc;
	NP_STR       	str1, str2;
	REGION_3D    	gab;
	BOOL       	 	rt = TRUE;
	lpLNP				 	lnp;
	LISTH     		list_brep = {NULL,NULL,0};
	GS_CODE				ret;
	BOOL		      value = TRUE;

	check = TRUE;
	*hnew_obj = NULL;
	flg_exist = 0;

	obj = (lpOBJ)hobj;
	lpgeobrep = (lpGEO_BREP)obj->geo_data;
	if (lpgeobrep->type != BODY) {
		put_message(MSG_CHECK_002,NULL,0);
		return FALSE;
	}

	key = UNION;

	lnp = (LNP*)get_elem(&vd_brep,lpgeobrep->num);
	num_np  = lnp->num_np;
	gab.min = lpgeobrep->min;
	gab.max = lpgeobrep->max;

	 
	f3d_gabar_project1(lpgeobrep->matr,&gab.min,&gab.max,&gab.min,&gab.max);
	define_eps(&(gab.min), &(gab.max)); 

	init_scan(&sc);
	sc.user_geo_scan 		= check_scan;
	read_user_data_buf  = bo_read_user_data_buf;
	write_user_data_buf = bo_write_user_data_buf;
	read_user_data_mem  = bo_read_user_data_mem;   	
	add_user_data_mem   = bo_add_user_data_mem;    		
	size_user_data      = bo_size_user_data;
	expand_user_data    = bo_expand_user_data;

	npwg->type = TNPW;
	np_init(npwg);
	if ( !(bo_new_user_data(npwg)) ) goto err;

//	num_proc = start_grad(GetIDS(IDS_SG170), num_np);
	ii = 0;

	if ( !np_init_list(&lista) ) goto err;
	if ( (o_scan(hobj,&sc)) != OSTRUE )  goto err;
//	end_grad  (num_proc ,ii);


	if ((hindex_a = check_index_np()) == NULL) goto err;
	num_index_a = lista.number_all;
	hindex_b = hindex_a;
	num_index_b = lista.number_all;
	listb = lista;

	
	if (!(np_str_form(np_gab_inter, &lista, &lista))) goto err;

	npa = npwg;
	npwg->type = TNPW;
	if ( (npb = creat_np_mem(TNPW,MAXNOV,MAXNOE,MAXNOF,MAXNOF,MAXNOE))
				== NULL ) goto err;
	np_init(npb);
	if ( !(bo_new_user_data(npb)) ) goto err;


//	num_proc = start_grad(GetIDS(IDS_SG171), lista.number_all);

	for (ii=0; ii<lista.number_all; ii++) {
//		step_grad (num_proc , ii);
		read_elem(&lista.vdim,ii,&str1);
		if ( str1.len == 0 )  continue; //  
		if (!(read_np(&lista.bnp,str1.hnp,npa))) goto err;
		if ( !check_np(npa, npa, &str1, &str1) ) goto err;

		begin_rw(&lista.nb, str1.b);
		for (i=0 ; i < str1.len ; i++) {
			b = (int*)get_next_elem(&lista.nb);
			read_elem(&lista.vdim,*b,&str2);
			if ( npa->ident > str2.ident ) continue;		
			if (!(read_np(&lista.bnp,str2.hnp,npb))) goto err;
			if ( !(check_np(npa, npb, &str1, &str2)) ) {    
				end_rw(&lista.nb);  //nb b = NULL;
				goto end;
			}
			if (str2.lab == 2) { 
				if ( !(overwrite_np(&lista.bnp,str2.hnp,npb,TNPW)) ) goto err;
			}
			write_elem(&lista.vdim,*b,&str2);
			if (ctrl_c_press) { 
				put_message(CTRL_C_PRESS, NULL, 0);
				end_rw(&lista.nb);  //nb b = NULL;
				goto err;
			}
		}
		end_rw(&lista.nb);  //nb b = NULL;
		if (str1.lab == 2)  {
			if ( !(overwrite_np(&lista.bnp,str1.hnp,npa,TNPW)) ) goto err;
		}
		write_elem(&lista.vdim,ii,&str1);
	}
	read_user_data_mem  = NULL;
	add_user_data_mem   = NULL;

//	end_grad  (num_proc , ii);
	num_proc = -1;
	

	if ( check ) goto end;            					

//	ret = get_bool_value(&value, get_prompt(PRO_CHECK_001)->str);
	ret = G_OK; 
	if (ret != G_OK || !value) goto end;

	if ( !(check_list_del()) ) goto err; 	

	

	object_number = 1;

	for (ii=0; ii<lista.number_all; ii++) {
		read_elem(&lista.vdim,ii,&str1);
		if ((str1.ort & ST_SV) || (str1.ort & ST_MAX)) {
			if ( !(b_del_sv(&lista, &str1)) ) goto err;
			str1.ort = 0;
			write_elem(&lista.vdim,ii,&str1);
		}
		if (ctrl_c_press) { 										
			put_message(CTRL_C_PRESS, NULL, 0);
			goto end;
		}
	}
	

	if ( !b_form_model_list(&list_brep, hobj) ) goto err;
	*hnew_obj = list_brep.hhead;

	goto end;
err:
	put_message(MSG_CHECK_001,NULL,0);
	rt = FALSE;
end:
	*pr = check;
	flg_exist = 1;
//	if (num_proc != -1 )  end_grad  (num_proc ,ii);
	bo_free_user_data(npwg);
	bo_free_user_data(npb);
	if ( npb ) free_np_mem(&npb);
	if (hindex_a != NULL) { SGFree(hindex_a); hindex_a = NULL; hindex_b = NULL; }
	np_free_stack();
	np_end_of_put(&lista,NP_CANCEL,0,NULL);
	read_user_data_buf  = NULL;
	write_user_data_buf	= NULL;
	size_user_data      = NULL;
	expand_user_data    = NULL;
	read_user_data_mem  = NULL;
	add_user_data_mem   = NULL;
	return rt;
}
#pragma argsused
static OSCAN_COD check_scan(hOBJ hobj,lpSCAN_CONTROL lpsc)
{
	register  short i;
	lpD_POINT v;

	if (ctrl_c_press) { 
		put_message(CTRL_C_PRESS, NULL, 0);
		return OSFALSE;
	}
//	step_grad (num_proc , ii++);

	npwg->type = TNPW;
	v = npwg->v;
	for (i = 1; i <= npwg->nov ; i++) {      
		if ( !o_hcncrd(lpsc->m,&v[i],&v[i])) goto err;
	}
	for (i = 1; i <= npwg->noe ; i++) {     
		npwg->efr[i].el &= ST_TD_CLEAR;
	}
	if ( !np_cplane(npwg) ) goto err;        
	if ( np_put(npwg,&lista) )   return OSTRUE;
err:
	return OSFALSE;
}

static hINDEX check_index_np(void)
{
  short      number_np,i;
	lpNP_STR str;
	hINDEX   hindex;//nb  = NULL;
	lpINDEX  index;

	number_np = lista.number_all;        				
	if ( (hindex = SGMalloc(sizeof(INDEX)*number_np)) == NULL) 		return NULL;
	index = (lpINDEX)hindex;

	begin_rw(&lista.vdim,0);
	for (i=0; i<lista.number_all; i++) {            
		str = (NP_STR*)get_next_elem(&lista.vdim);
		index[i].ident = str->ident;
		index[i].hnp   = str->hnp;
		write_elem(&lista.vdim,i,str);
	}
	end_rw(&lista.vdim);
	qsort((void*)index,i,sizeof(index[0]),(fptr)sort_function);
	return hindex;
}
static int sort_function( const short *a, const short *b)
{
	if ( (*(INDEX*)a).ident < (*(INDEX*)b).ident )   return -1;
	if ( (*(INDEX*)a).ident == (*(INDEX*)b).ident )  return 0;
	return 1;
}

static BOOL check_list_del(void)
{
	short   pr = 0;
  int			ii;
	NP_STR	str;

//	num_proc = start_grad(GetIDS(IDS_SG035), lista.number_all);
	for (ii=0; ii<lista.number_all; ii++) {
//		step_grad (num_proc , ii);
		read_elem(&lista.vdim,ii,&str);
		if (abs(str.lab) == 2)  {               
			if (!(read_np(&lista.bnp,str.hnp,npa))) goto err;
			 
			if ( !(check_del(&str)) ) goto err;
			if (npa->nof == 0) str.lab = -1;     
			else             { str.lab =  1;       
				if ( !(overwrite_np(&lista.bnp,str.hnp,npa,TNPW)) ) goto err;
			}
			write_elem(&lista.vdim,ii,&str);
		} else pr = 1;
		if (ctrl_c_press) { 											
			put_message(CTRL_C_PRESS, NULL, 0);
			goto err;
		}
	}
//	end_grad  (num_proc , ii);
	if ( pr == 0 ) return TRUE;   
	
	if ( check_list_lable(&lista) ) return TRUE;
err:
//	end_grad  (num_proc , ii);
	return FALSE;
}

static BOOL check_list_lable(lpNP_STR_LIST list)
{
	int    ii;
	NP_STR str;
	int    count_old, count_np;

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
			if (!(read_np(&list->bnp,str.hnp,npa))) return FALSE;
			if ( !(b_np_lab(list,&str,npa)) ) return FALSE;
			if (str.lab != 0)  {
				if (str.lab == 1 && object_number == 2 && key == SUB)
					if ( !(overwrite_np(&list->bnp,str.hnp,npa,TNPW)) ) return FALSE;
				write_elem(&list->vdim,ii,&str);
				count_np++;
			}
		}

		if (ctrl_c_press) { 											
			put_message(CTRL_C_PRESS, NULL, 0);
			return FALSE;
		}
	} while (count_np != list->number_all && count_np != count_old);
	if (count_np == 0) {
		put_message(MSG_CHECK_003,NULL,1);
		return FALSE;
	}
	if (count_np != list->number_all) {
		put_message(MSG_CHECK_004,NULL,0);
		return FALSE;
	}
	return TRUE;
}
