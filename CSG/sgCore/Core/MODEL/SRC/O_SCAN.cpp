#include "../../sg.h"
 
static  void make_frame(lpSCAN_CONTROL lpsc)
{
	npwg->type = TNPF;
	npwg->nof  = 0;
	npwg->noc  = 0;
	lpsc->breptype = FRAME;
}
static  short scan_np_init(lpSCAN_CONTROL lpsc,lpLNP lnp)
{
	switch ( lnp->kind )
	{
		case BOX:
			memset(&lpsc->viloc,0,sizeof(lpsc->viloc));
			return 1;
		default:
			get_first_np_loc( &lnp->listh, &lpsc->viloc);
			return lnp->num_np;
	}
}

static  BOOL scan_get_np(lpSCAN_CONTROL lpsc, lpLNP lnp,
								lpVI_LOCATION viloc)
{
	BOOL	bcod;

	switch (lnp->kind) {
		case BOX:
			{
				lpCSG csg;
				csg = (lpCSG)lnp->hcsg;
                sgFloat  boxSizes[3];
                memcpy(boxSizes, (sgFloat *)csg->data, sizeof(sgFloat)*3);
				if ( !o_create_box(/*(sgFloat *)csg->data*/boxSizes) ) return OSFALSE;
			}
			return TRUE;
		default:
			lpsc->viloc = *viloc;
			begin_read_vld(&lpsc->viloc);
			bcod = read_np_mem(npwg);
			get_read_loc(viloc);
			end_read_vld();
			return bcod;
	}
}
void init_scan(lpSCAN_CONTROL lpsc)
{
	memset(lpsc,0, sizeof(SCAN_CONTROL));
	lpsc->color     = CTRANSP;
	lpsc->layer     = -1;
	lpsc->breptype  = (BREPTYPE)(-1);
}

OSCAN_COD o_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
  OSCAN_COD     (*o_scan_geo)(hOBJ hobj, lpSCAN_CONTROL lpsc);
	lpOBJ         obj;
	OSCAN_COD     cod = OSTRUE;
	SCAN_CONTROL  sc;
	short         bit;

	if ( !hobj ) return OSTRUE;
	scan_level++;
	sc = *lpsc;
	obj = (lpOBJ)hobj;
	sc.type       = obj->type;
	sc.status     = (OSTATUS)obj->status;
	sc.drawstatus = obj->drawstatus;
	sc.hobj				= hobj;

	if ( sc.color == CTRANSP )  {
		sc.color = obj->color;
		if ( sc.color == CTRANSP && sc.type == OINSERT ) sc.color = CTRANSPBLK;
	}
	if ( sc.color == CTRANSPBLK && !check_type(struct_filter,sc.type) )
		sc.color = obj->color;
	if ( obj->status&ST_BLINK && sc.scan_draw)    sc.color = blink_color;

	sc.ltype = obj->ltype;
	sc.lthickness = obj->lthickness;

	if ( obj->type == OBREP ) sc.breptype = (BREPTYPE)((lpGEO_BREP)obj->geo_data)->type;
	else                      sc.breptype = (BREPTYPE)(-1);
	
  o_scan_geo = (OSCAN_COD (*)(hOBJ hobj, lpSCAN_CONTROL lpsc))GetObjMethod(sc.type, OMT_SCAN);
	if(!o_scan_geo){
		handler_err(ERR_NUM_OBJECT);
		scan_level--;     // ZZZ 27.3.00
		return OSFALSE;
	}

	bit = (sc.status & (ST_DIRECT | ST_ORIENT)) ^
				(lpsc->status & (ST_DIRECT | ST_ORIENT));
	int tmp_int = (int)sc.status;
	tmp_int &= ~(ST_DIRECT | ST_ORIENT);
	tmp_int|= bit;
	sc.status =(OSTATUS)tmp_int;/*&= ~(ST_DIRECT | ST_ORIENT);
	sc.status |= bit;*/


	if ( sc.scan_draw ) {   //    

		if ( lpsc->drawstatus & ST_TD_FRAME) sc.drawstatus |= ST_TD_FRAME;

		if ( sc.drawstatus & ST_HIDE ){
			scan_level--;                 // ZZZ 27.3.00
			return OSSKIP;
		}
		if ( sc.drawstatus & ST_TD_BOX && !scan_true_vision) {
			//   
			lpGEO_BREP  lpgeobrep;
			D_POINT     p;
			MATR        matr;
			OBJTYPE     type_save;

			if ( !(check_type(complex_filter,sc.type)) ) goto m;
			if ( sc.color == CTRANSP || sc.color == CTRANSPBLK )
				sc.color = obj_attrib.color;
			if ( sc.user_pre_scan )
				if ( (cod=sc.user_pre_scan(hobj,&sc))   != OSTRUE ) goto mcod;

			if ( sc.user_geo_scan ) {
				obj = (lpOBJ)hobj;
				lpgeobrep = (lpGEO_BREP)(obj->geo_data);
				dpoint_sub(&lpgeobrep->max, &lpgeobrep->min, &p);
				o_hcunit(matr);
				o_tdtran(matr,&lpgeobrep->min);
				o_hcmult(matr, lpgeobrep->matr);
				if ( sc.m )  o_hcmult(matr, sc.m);
				if ( !o_create_box((sgFloat  *)&p) ){
					scan_level--;                 // ZZZ 27.3.00
					return OSFALSE;
				}
				sc.m = matr;
				sc.breptype = BODY;
				if ( sc.drawstatus&ST_TD_FRAME)  make_frame(&sc);
				memset(&sc.viloc,0,sizeof(sc.viloc));
				scan_flg_np_begin = TRUE;    scan_flg_np_end   = TRUE;
				type_save = sc.type;
				sc.type = OBREP;
				sc.user_geo_scan( hobj,&sc);
				sc.type = type_save;
				if ( cod != OSTRUE ) goto mcod;
				scan_flg_np_begin = FALSE;    scan_flg_np_end   = FALSE;
			}

			if ( sc.user_post_scan )
				if ( (cod=sc.user_post_scan(hobj,&sc))  != OSTRUE ) goto mcod;
			goto mcod;
		}
/*================================================================*/
		else {      //  
// 				     ,  
//           
			if (check_type(struct_filter, obj->type) && obj->type != OINSERT) {
				cod = o_scan_geo(hobj,&sc);
				goto mcod;
			}
		}  //   
/*=======================================================================*/
	}
m:
	if ( sc.user_pre_scan )
		if ( (cod=sc.user_pre_scan(hobj,&sc)) 	!= OSTRUE ) goto mcod;

	scan_flg_np_begin = TRUE;       //   
	scan_flg_np_begin = TRUE;       //    
	if ( (cod=o_scan_geo(hobj,&sc) ) != OSTRUE ) goto mcod;

	if ( sc.user_post_scan )
		if ( (cod=sc.user_post_scan(hobj,&sc)) 	!= OSTRUE ) goto mcod;

mcod:
	scan_level--;
	return cod;
}

#pragma argsused
OSCAN_COD  scan_simple_obj(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	if ( !lpsc->user_geo_scan ) return OSTRUE;
	return ( lpsc->user_geo_scan( hobj,lpsc) );
}

OSCAN_COD  scan_path(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	hOBJ				 hobjtmp,hnext;
	lpOBJ        obj;
	OSCAN_COD    cod;
  void        (*get_item)(VADR hitem, VADR * hnext);


  obj = (lpOBJ)hobj;
	if ( lpsc->status&ST_DIRECT ) {
    hobjtmp  = ((lpGEO_PATH)obj->geo_data)->listh.htail;
    get_item = get_prev_item;
  } else {
    hobjtmp  = ((lpGEO_PATH)obj->geo_data)->listh.hhead;
    get_item = get_next_item;
  }
	while (hobjtmp) {
    get_item(hobjtmp,&hnext);
    if ( (cod=o_scan(hobjtmp,lpsc)) != OSTRUE ) {
      if ( cod != OSSKIP ) return cod;
    }
    hobjtmp = hnext;
	}
  return OSTRUE;
}

OSCAN_COD  scan_brep(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	lpOBJ       obj;
	OSCAN_COD   cod = OSTRUE;
	MATR				m;
	VI_LOCATION viloc;
	short 				i,num_np;
	LNP					lnp;
	int				num;

	if ( lpsc->user_geo_scan ) {
		obj = (lpOBJ)hobj;

		memcpy( &m,&((lpGEO_BREP)(obj->geo_data))->matr, sizeof(m));
		num = ((lpGEO_BREP)(obj->geo_data))->num;
		if ( lpsc->m ) o_hcmult(m,lpsc->m);
		lpsc->m = m;
		if ( !read_elem(&vd_brep,num,&lnp) ) return OSFALSE;//FALSE;
		if ( !(num_np=scan_np_init(lpsc,&lnp)) ) return OSFALSE;
		viloc = lpsc->viloc;
		scan_flg_np_begin = TRUE;       //   
		scan_flg_np_end   = FALSE;      //   
		for ( i=0; i<num_np; i++ ) {
			if ( !scan_get_np(lpsc,&lnp,&viloc) ) return OSFALSE;
			if ( lpsc->status & ST_ORIENT )
				if ( npwg->type != TNPF)  np_inv(npwg,NP_FIRST);
			if ( i == num_np-1) scan_flg_np_end = TRUE; //   -
			if ( lpsc->drawstatus&ST_TD_FRAME && lpsc->scan_draw)  make_frame(lpsc);
			cod=lpsc->user_geo_scan( hobj,lpsc);
			scan_flg_np_begin = scan_flg_np_end   = FALSE;
			if ( cod == OSTRUE ) continue;
			break;
		}
		memset(&lpsc->viloc,0,sizeof(lpsc->viloc));
		lpsc->m    = NULL;
	}
	return cod;
}

OSCAN_COD  scan_bgroup(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	hOBJ				 hobjtmp,hnext;
	lpOBJ        obj;
	OSCAN_COD    cod;

	obj = (lpOBJ)hobj;
	hobjtmp = ((lpGEO_GROUP)obj->geo_data)->listh.hhead;
	while (hobjtmp) {
		get_next_item(hobjtmp,&hnext);
		if ( (cod=o_scan(hobjtmp,lpsc)) != OSTRUE ) {
			if ( cod != OSSKIP ) return cod;
		}
		hobjtmp = hnext;
	}
	return OSTRUE;
}

OSCAN_COD  scan_insert(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	hOBJ			hobjtmp,hnext;
	lpOBJ     obj;
	OSCAN_COD cod;
	MATR      m;
	IBLOCK    blk;
	int			num;

	obj = (lpOBJ)hobj;
	num = ((lpGEO_INSERT)obj->geo_data)->num;
	memcpy( &m,&((lpGEO_INSERT)(obj->geo_data))->matr, sizeof(m));
	if ( !read_elem(&vd_blocks,num,&blk) ) return OSFALSE;
	hobjtmp = blk.listh.hhead;
	if ( lpsc->m ) o_hcmult (m,lpsc->m);
	lpsc->m = m;
	while (hobjtmp) {
		get_next_item(hobjtmp,&hnext);
    lpsc->block_level++;
    cod=o_scan(hobjtmp,lpsc);
    lpsc->block_level--;
    switch ( cod ) {
      case OSTRUE:
      case OSSKIP:
        break;
      default:
				return cod;
		}
		hobjtmp = hnext;
	}
	return OSTRUE;
}

OSCAN_COD  scan_frame(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	MATR      m;
	lpOBJ     obj;
	OSCAN_COD cod;


	if ( lpsc->user_geo_scan ) {
		obj = (lpOBJ)hobj;
		memcpy( &m,&((lpGEO_FRAME)(obj->geo_data))->matr, sizeof(m));
		if ( lpsc->m ) o_hcmult (m,lpsc->m);
		lpsc->m = m;
		if ( (cod=lpsc->user_geo_scan(hobj,lpsc)) != OSTRUE ) return cod;
  }
	return OSTRUE;
}

