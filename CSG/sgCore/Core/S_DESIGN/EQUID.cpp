#include "../sg.h"

static OSCAN_COD equid_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);
static  void gru_null(lpNPW np);
static  BOOL equid_bok(lpTRI_BIAND trb);
static BOOL user_last(lpNPW np, short edge);

static lpD_POINT   vnew;              //  

BOOL equid(hOBJ hobj, sgFloat h, BOOL type_body, hOBJ *hrez)
{
	short        v, edge;
  int          i, num_proc = -1;
	NP_STR       str;
	lpOBJ        obj;
	lpGEO_BREP   lpgeobrep;
	SCAN_CONTROL sc;
	D_POINT      nv;     						         		   //   
	NP_STR_LIST  list_str;
	BOOL				 cod = FALSE;
	TRI_BIAND		 trb;
	LISTH				 listho;
	MATR				 matr;

	init_listh(&listho);
	init_scan(&sc);
	*hrez	= NULL;

	sc.user_geo_scan = equid_scan;
	sc.data = &list_str;
	if ( !np_init_list(&list_str) ) return FALSE;

	if ( (o_scan(hobj,&sc)) == OSFALSE ) goto end;

	if ( !begin_tri_biand(&trb) ) goto end;

	vnew = (D_POINT*)SGMalloc(MAXNOV*sizeof(npwg->v[0]));
	obj = (lpOBJ)hobj;
	lpgeobrep = (lpGEO_BREP)obj->geo_data;
	memcpy(matr, lpgeobrep->matr, sizeof(MATR));
	trb.ident = lpgeobrep->ident_np;

//	num_proc = start_grad(GetIDS(IDS_SG187), list_str.number_all);      //" 1/2"

	for (i = 0; i < list_str.number_all; i++) {
//		step_grad (num_proc , i);
		read_elem(&list_str.vdim,i,&str);
		if (!(read_np(&list_str.bnp,str.hnp,npwg))) goto end;

		for (v = 1 ; v <= npwg->nov ; v++) {
			for (edge = 1 ; edge <= npwg->noe ; edge++) {
				if (npwg->efr[edge].bv == v) { edge = -edge; break; }
				if (npwg->efr[edge].ev == v) break;
			}
      nv.x = nv.y = nv.z = 0.;
			np_vertex_normal(&list_str,npwg,edge,&nv,user_last,FALSE);
			nv.x *= h;
			nv.y *= h;
			nv.z *= h;
			dpoint_add(&npwg->v[v], &nv, &vnew[v]);
		}
		if ( type_body ) {                         			//  
			if ( !equid_bok(&trb) ) goto end; 						//  
			if ( !np_put(npwg, &trb.list_str) ) goto end; //  
		}

		memcpy(npwg->v,vnew,sizeof(vnew[0])*(npwg->nov+1));
		gru_null(npwg);
		if ( type_body ) 	npwg->ident = trb.ident++;
		if ( !np_cplane(npwg) ) goto end;
		if ( !np_put(npwg,&trb.list_str) ) goto end;    //  
	}
	SGFree(vnew);  vnew = NULL;
	np_end_of_put(&list_str, NP_CANCEL, 0, NULL);

	if ( type_body ) {
		cod = end_tri_biand(&trb, &listho);
		*hrez = listho.hhead;
	} else {
		cod = cinema_end(&trb.list_str, hrez);
		end_tri_biand(&trb, &listho);
	}
	o_trans(*hrez, matr);
end:
//	if (num_proc != -1 )  end_grad  (num_proc ,i);
	if ( vnew ) SGFree(vnew);
	if ( list_str.number_all > 0 ) np_end_of_put(&list_str, NP_CANCEL, 0, NULL);
	if ( trb.list_str.number_all > 0 ) np_end_of_put(&trb.list_str, NP_CANCEL, 0, NULL);
	return cod;
}
#pragma argsused
static OSCAN_COD equid_scan(hOBJ hobj,lpSCAN_CONTROL lpsc)
{

	if (ctrl_c_press) { 												//   Ctrl/C
		put_message(CTRL_C_PRESS, NULL, 0);
		return OSFALSE;
	}

	npwg->type = TNPW;
	if ( !np_cplane(npwg) ) return OSFALSE;        //  - -
	if ( np_put(npwg,(lpNP_STR_LIST)lpsc->data) )   return OSTRUE;
	return OSFALSE;
}
#pragma argsused
static BOOL user_last(lpNPW np, short edge)
{
	return FALSE;
}

static  void gru_null(lpNPW np)
{
	short edge;

	for (edge = 1 ; edge <= np->noe ; edge++) {
		if (np->efc[edge].fm < 0) {
			np->efc[edge].fm = 0;
			np->efc[edge].em = 0;
			continue;
		}
		if (np->efc[edge].fp < 0) {
			np->efc[edge].ep = 0;
			np->efc[edge].fp = 0;
			continue;
		}
	}
}

static BOOL equid_bok(lpTRI_BIAND trb)
{
	short 	edge;
	short		v1,v2;
	NPTRP		trp;

	trp.constr = 0;

	for (edge = 1 ; edge <= npwg->noe ; edge++) {
		if (npwg->efc[edge].fm != 0 && npwg->efc[edge].fp != 0) continue;
		if (npwg->efc[edge].fp == 0) {
			v1 = npwg->efr[edge].bv;
			v2 = npwg->efr[edge].ev;
		} else {
			v1 = npwg->efr[edge].ev;
			v2 = npwg->efr[edge].bv;
		}

		trp.v[0] = npwg->v[v1];
		trp.v[1] = vnew[v1];
		trp.v[2] = vnew[v2];
		if ( !put_tri(trb, &trp)) return FALSE;
		trp.v[1] = npwg->v[v2];
		if ( !put_tri(trb, &trp)) return FALSE;
	}

	return TRUE;
}

