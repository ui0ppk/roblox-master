#include "../sg.h"

static OSCAN_COD sew_pre_scan(hOBJ hobj, lpSCAN_CONTROL lpsc);
static OSCAN_COD sew_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);
static short ident_np;
static WORD  app_ident;

 
BOOL sew(lpLISTH list, hOBJ *hrez, sgFloat precision)
{
	hOBJ         hobj, hnext;
	SCAN_CONTROL sc;
	NP_STR_LIST	 list_str;
	BOOL				 cool;
	LISTH				 listh;

	ident_np = -32767;
	*hrez		 = NULL;

	init_scan(&sc);
	sc.data = &list_str;
	sc.user_pre_scan = sew_pre_scan;
	sc.user_geo_scan = sew_scan;

	if ( !np_init_list(&list_str) ) return FALSE; //NULL;
	hobj = list->hhead;
	while (hobj != NULL) {
		get_next_item_z(SEL_LIST,hobj,&hnext);
		if ( (o_scan(hobj,&sc)) == OSFALSE ) return FALSE; //NULL;
		hobj = hnext;
	}

	init_listh(&listh);
	cool = np_end_of_put_1(&list_str, NP_ALL, (float)c_smooth_angle, &listh, precision);
	*hrez = make_group(&listh, TRUE);
	if ( cool ) return TRUE;
	else				return FALSE;
}
#pragma argsused
static OSCAN_COD sew_pre_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	lpOBJ      	obj;
	lpGEO_BREP 	lpgeobrep;
  WORD  			app_ident1;

  obj = (lpOBJ)hobj;
	lpgeobrep = (lpGEO_BREP)obj->geo_data;

	app_ident1 = lpgeobrep->ident_np + 32767;  
	app_ident = 32767 - ident_np;             
	if (app_ident < app_ident1) {
		put_message(IDENT_OVER,NULL,0);         
		return OSFALSE;
	}
	app_ident = ident_np + 32767; 						 
	ident_np = ident_np + app_ident1;
	return OSTRUE;
}
#pragma argsused
static OSCAN_COD sew_scan(hOBJ hobj,lpSCAN_CONTROL lpsc)
{
	lpD_POINT v;
	short 		i;

	if (ctrl_c_press) { 												//   Ctrl/C
		put_message(CTRL_C_PRESS, NULL, 0);
		return OSFALSE;
	}
	if (lpsc->type != OBREP) return OSTRUE; //   

	npwg->type = TNPW;
	v = npwg->v;
	for (i = 1; i <= npwg->nov ; i++) {     //  
		if ( !o_hcncrd(lpsc->m,&v[i],&v[i])) return OSFALSE;
	}
	if ( !np_cplane(npwg) ) return OSFALSE;        //  - -

	for (i=1 ; i <= npwg->noe ; i++)       //     
		if (npwg->efc[i].fp == 0 || npwg->efc[i].fm == 0) npwg->efr[i].el = 0;

	if ( ident_np > -32767) {  			    				   //  ident
		npwg->ident += app_ident;
		for (i=1 ; i <= npwg->noe ; i++) {
			if (npwg->efc[i].fp < 0) { npwg->efc[i].ep += app_ident; continue; }
			if (npwg->efc[i].fm < 0) { npwg->efc[i].em += app_ident; continue; }
		}
	}
	if ( np_put(npwg,(lpNP_STR_LIST)lpsc->data) )   return OSTRUE;
	return OSFALSE;
}
