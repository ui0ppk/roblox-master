#include "../sg.h"

static BOOL b_grup(lpNP_STR_LIST list, lpLISTH list_brep, hOBJ hobj);
static BOOL b_put_new(lpNPW np, lpNP_STR_LIST list);

short  * mem_ident = NULL;

BOOL b_form_model_list(lpLISTH list_brep, hOBJ hobj)
{
	int         i; //, j, gru;
	NP_STR_LIST list_new;
	lpNP_STR    str;

	if ( (mem_ident = (short*)SGMalloc(sizeof(npwg->ident)*(lista.number_all +
				 listb.number_all))) == NULL ) return FALSE;

	if ( !np_init_list(&list_new) ) goto err;

	begin_rw(&lista.vdim,0);
	for (i = 0; i < lista.number_all; i++) {
		str = (NP_STR*)get_next_elem(&lista.vdim);
		if (str->lab != 1) continue;    		 
		if ( !read_np(&lista.bnp,str->hnp,npwg) ) {
			end_rw(&lista.vdim);
			goto err;
		}
		if ( !b_put_new(npwg,&list_new) )  {
			end_rw(&lista.vdim);
			goto err;
		}
	}
	end_rw(&lista.vdim);
	np_end_of_put(&lista,NP_CANCEL,0,NULL);

	if ( brept1 && flg_exist ) {       
		begin_rw(&listb.vdim,0);
		for (i = 0; i < listb.number_all; i++) {
			str = (NP_STR*)get_next_elem(&listb.vdim);
			if (str->lab != 1) continue;    		 
			if ( !read_np(&listb.bnp,str->hnp,npwg) ) {
				end_rw(&listb.vdim);
				goto err;
			}
			if ( !b_put_new(npwg,&list_new) )   {
				end_rw(&listb.vdim);
				goto err;
			}
		}
		end_rw(&listb.vdim);
		np_end_of_put(&listb,NP_CANCEL,0,NULL);
	}
	SGFree(mem_ident);

	if ( !list_new.number_all ) {
		np_end_of_put(&list_new,NP_CANCEL,0,NULL);
		return TRUE;
	}
	if ( !b_grup(&list_new, list_brep, hobj) ) return FALSE;

	return TRUE;
err:
	SGFree(mem_ident);
	np_end_of_put(&lista,NP_CANCEL,0,NULL);
	np_end_of_put(&listb,NP_CANCEL,0,NULL);
	np_end_of_put(&list_new,NP_CANCEL,0,NULL);
	return FALSE;
}

BOOL b_grup(lpNP_STR_LIST list, lpLISTH list_brep, hOBJ hobj)
{
	short       i,fi,index;
	short       ident_np;
  int					number, number_old, istr;
	lpNP_STR    str_t1;
	NP_STR      str_t,str;
	int *    b = NULL;
	hOBJ        hbrep  = NULL;
	lpOBJ       brep, obj;
	lpGEO_BREP  lpgeobrep;
	VLD         vld;
	REGION_3D   g = {{1.e35,1.e35,1.e35},{-1.e35,-1.e35,-1.e35}};
	LNP					lnp;

//	init_listh(list_brep);       
	do {
		list->number_np = list->number_all;
		begin_rw(&list->vdim,0);
		fi = 0;
		while (1) {
			str_t1 = (lpNP_STR)get_next_elem(&list->vdim);
			if ( str_t1->lab == 0 ) break;
			fi++;
		}
		str_t1->lab = 1;                 
		end_rw(&list->vdim);

 //nb 		number_old = number = 0;
		ident_np = -32767;
		number = 0;
		init_vld(&vld);

		do {                                   
			number_old = number;
			number = 0;

			for (istr = fi; istr < list->number_all; istr++) {
				read_elem(&list->vdim,istr,&str);
				if (str.lab == 2)  continue;                     
				if (str.lab == 0)  continue;
				number++;
				if (str.ort == 1)  continue;            
				str.ort = 1;                          
				write_elem(&list->vdim,istr,&str);
				if (str.len == 0)  goto mod;           
				begin_rw(&list->nb, str.b);
				for (i=0 ; i < str.len ; i++) {
					b = (int*)get_next_elem(&list->nb);
					if ( !(np_get_str(list,(short)(*b),&str_t,&index)) ) goto err;
					if (str_t.lab == 1) continue;
					str_t.lab = 1; str_t.ort = 0;              
					write_elem(&list->vdim,index,&str_t);
				}
				end_rw(&list->nb);   b = NULL;
			}
		} while (number != list->number_np && number != number_old);
mod:
		if ( ( hbrep = o_alloc(OBREP)) == NULL)	goto err;
		brep = (lpOBJ)hbrep;
		obj = (lpOBJ)hobj;
		brep->color 		 = obj->color;
		brep->ltype 		 = obj->ltype;
		brep->lthickness = obj->lthickness;
		lpgeobrep = (lpGEO_BREP)brep->geo_data;
		o_hcunit(lpgeobrep->matr);
		number = 0;
		list->number_np = 0;
		for (istr = 0; istr < list->number_all; istr++) {
			read_elem(&list->vdim,istr,&str);
			if (str.lab == 2)  continue;
			if ( str.lab == 0 ) { list->number_np++; continue; }
			number++;
			if (ident_np <= str.ident) ident_np = str.ident;
			if (!(read_np(&list->bnp,str.hnp,npwg))) goto err;
			np_gab_init(&npwg->gab);
			dpoint_minmax(&npwg->v[1], npwg->nov, &npwg->gab.min, &npwg->gab.max);
			np_gab_un(&g, &npwg->gab, &g);
			add_np_mem(&vld,npwg,TNPB);
			str.lab = 2;                           
			write_elem(&list->vdim,istr,&str);
		}
		lpgeobrep->min = g.min;
		lpgeobrep->max = g.max;
		lpgeobrep->ident_np = ident_np + 1; 
		if  ( brept1 ) lpgeobrep->type = BODY;     
		else           lpgeobrep->type = SURF;
		memset(&lnp,0,sizeof(lnp));
		lnp.kind = COMMON;
		lnp.listh = vld.listh;
		lnp.num_np = number;
		if ( !put_np_brep(&lnp,&lpgeobrep->num)) {
//		if ( !put_np_brep(&lpgeobrep->npd,&vld.listh,number,COMMON,NULL)) {
			goto err;
		}
		copy_obj_attrib(hobj, hbrep);
		attach_item_tail(list_brep, hbrep);

	} while (list->number_np != 0);                      
	number = (short)list_brep->num;
	goto cancel;

err:
	number = 0;
	if (b != NULL) 		 end_rw(&list->nb);
	if (hbrep != NULL) o_free(hbrep,NULL);
cancel:
	np_str_free(list);
	return number;
}

BOOL b_put_new(lpNPW np, lpNP_STR_LIST list)
{
	short    k, edge;
	int * b;
	int      ident;
	NP_STR   str;
	VNP      vnp;

	dpoint_minmax(&np->v[1], np->nov, &np->gab.min, &np->gab.max);

	memset(&str,0,sizeof(str));
	str.ident = np->ident;
	str.gab = np->gab;

	str.b = list->nb.num_elem;              
	str.len = 0;
	for (edge = 1 ; edge <= np->noe ; edge++) {
		if (np->efc[edge].fp >= 0 && np->efc[edge].fm >= 0) continue;
		if (np->efc[edge].fp < 0)	ident = np->efc[edge].ep;
		else              		   	ident = np->efc[edge].em;
		if ( str.len ) {
			begin_rw(&list->nb,str.b);
			k = 0;
			while (k < str.len) {
				b = (int*)get_next_elem(&list->nb);
				if ( *b == ident ) break; 
				k++;
			}
			end_rw(&list->nb);
			if (k < str.len) continue;
		}
		add_elem(&list->nb, &ident);     
		str.len++;
	}

	if ( !(vnp = write_np(&list->bnp,np,TNPW)) ) return FALSE;
	str.hnp = vnp;
	list->number_all++;
	return ( add_elem(&list->vdim,&str) );
}
