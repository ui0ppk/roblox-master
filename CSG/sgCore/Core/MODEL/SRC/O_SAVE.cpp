#include "../../sg.h"

static BOOL story_surf(lpBUFFER_DAT bd);

typedef struct {
  short  count;     // -   
  int number;    //    
}BLOCKS_MAP;
typedef BLOCKS_MAP * lpBLOCKS_MAP;

static char flg_list = 0;  //   

static BOOL calk_new_order_np_and_attr(lpLISTH listh, NUM_LIST list_zudina,
																				int *num);
static BOOL save_vd_brep_and_attr( lpBUFFER_DAT bd, lpLISTH listh,
																	 NUM_LIST list_zudina);

static BOOL  create_map_blocks(lpLISTH listh,NUM_LIST list_zudina,
							lpVDIM vdim, int *new_number);
static BOOL  save_blocks(lpBUFFER_DAT bd, int count);

static void init_attr_save_info(void);
static BOOL mark_record_to_save(IDENT_V rec_id);
static void mark_attr_to_save(IDENT_V attr_id);
static void mark_attr_item_to_save(IDENT_V item_id);
static void mark_ft_item_to_save(IDENT_V item_id);
static BOOL save_attr_info(lpBUFFER_DAT bd);
static IDENT_V get_new_irec_num_to_save(IDENT_V irec);
static IDENT_V get_new_ft_num_to_save(IDENT_V id);
static BOOL save_obj_types_map(lpBUFFER_DAT bd);


static lpVDIM vdim_map; //    

static OSCAN_COD save_pre_scan    (hOBJ hobj,lpSCAN_CONTROL lpsc);
static OSCAN_COD save_post_scan(hOBJ hobj, lpSCAN_CONTROL lpsc);

static OSCAN_COD  (**save_type_g)(lpBUFFER_DAT bd,lpOBJ obj);

extern char MODEL_VERSION[];
BOOL o_save_list(lpBUFFER_DAT bd,lpLISTH listh, NUM_LIST list_zudina,
								 int *num_obj, MATR matr, MATR matr_inv)
{
	hOBJ          hobj;
	int           count = 0;
	int			      new_number;
	VERSION_MODEL ver = (VERSION_MODEL)(LAST_VERSION - 1);
	VDIM          vdim;   //  
	BOOL          cod = FALSE;
	UCHAR			    num_field= 0;
//  short			    tmp;

	*num_obj = 0;
	/*if (!story_data(bd,strlen(MODEL_VERSION),MODEL_VERSION))  return FALSE;
	{
		char tmp[5];
		ver = (VERSION_MODEL)(ver-LAST_DOS_VERSION);
		itoa(ver,tmp,10);
		if ( ver < 10 ) {
			tmp[1] = tmp[0];
			tmp[0] = '0';
		}
		if (!story_data(bd,2,tmp))  return FALSE;
	}
*/
  //      
  if(!save_obj_types_map(bd)) return FALSE;

	//if (!story_data(bd,sizeof(num_field),&num_field))  return FALSE;
	//  
	//if (!story_data(bd,sizeof(sgw_unit),&sgw_unit))  return FALSE;
  //tmp = o_appl_number;
	//if (!story_data(bd,sizeof(tmp),&tmp))  return FALSE;
  //tmp = o_appl_version;
	//if (!story_data(bd,sizeof(tmp),&tmp))  return FALSE;

//   
	if ( !save_vd_brep_and_attr( bd, listh, list_zudina) ) return FALSE;

	flg_list = 1;      //   
//  
	vdim_map = &vdim;
	if ( !create_map_blocks( listh, list_zudina, vdim_map, &new_number) )
		goto err;
	if ( !save_blocks(bd,new_number)) goto err;

//  
	if (!story_data(bd,sizeof(listh->num),&listh->num))  goto err;
	hobj = listh->hhead;
	while (hobj) {
		if ( matr ) o_trans(hobj,matr);  // ..lse
		cod = o_save_obj(bd,hobj);
		if ( matr ) o_trans(hobj,matr_inv); // ..lse
		if ( !cod ) {
			*num_obj = count;
			goto err;
		}
		get_next_item_z(list_zudina,hobj,&hobj);
		count++;
	}
	cod = TRUE;
	*num_obj = count;
err:
  flg_list = 0;
	free_vdim(&vdim);
	return cod;
}
//     
static BOOL  create_map_blocks( lpLISTH listh, NUM_LIST list_zudina,
												 lpVDIM vdim, int *new_number)
{
	int					i;
	BLOCKS_MAP    map;
	lpBLOCKS_MAP  lpmap;

	init_vdim(vdim, sizeof(BLOCKS_MAP));
	memset(&map,0,sizeof(map));
	if ( !create_count_blocks( listh, list_zudina, vdim, &map) ) goto err;
 //     
 //    0- - 
	*new_number = 0;
	begin_rw(vdim,0);
	for (i=0; i<vd_blocks.num_elem; i++) {
		if ((lpmap=(BLOCKS_MAP*)get_next_elem(vdim)) == NULL) goto err;
		switch (lpmap->count) {
			case -1:
				lpmap->number = -1;
				break;
			case 0:
				if ( list_zudina == SEL_LIST ){
					lpmap->number = -1;
					break;
				}
			default:
				lpmap->number = *new_number;
				(*new_number)++;
				break;
		}
	}
	end_rw(vdim);
	return TRUE;
err:
	free_vdim(vdim);
	return FALSE;
}
static BOOL  save_blocks(lpBUFFER_DAT bd, int count)
{
	int        i;
	IBLOCK      blk;
	BOOL        flg_idle;
	BLOCKS_MAP  map;


	if (!story_data(bd,sizeof(count),&count))	return FALSE;
	flg_idle = FALSE;
	for ( i=0; i<vd_blocks.num_elem; i++ ) {
		if (!read_elem(vdim_map,i,&map)) return FALSE;
		if ( map.number == -1 ) continue;   //  
		if (!read_elem(&vd_blocks,i,&blk)) return FALSE;
		if ( map.count == 0 ) flg_idle = TRUE; //  
		if ( !o_save_one_block(bd,&blk) ) return FALSE;
	}
	if ( flg_idle ) {    handler_err(SAVE_EMPTY_BLOCK);  }
	return TRUE;
}
BOOL  o_save_one_block(lpBUFFER_DAT bd, lpIBLOCK blk)
{
	int    offset, offset_save;
	IDENT_V irec;
	hOBJ		hobj;
	UCHAR		len;

//  if (!story_data(bd,sizeof(blk->name),blk->name))      return FALSE;
	len = (UCHAR)strlen(blk->name);
	if (!story_data(bd,1,&len))							      				return FALSE;
	if (!story_data(bd,len,blk->name))      							return FALSE;
	if (!story_data(bd,sizeof(blk->type),&blk->type))      return FALSE;
	if (!story_data(bd,sizeof(blk->count),&blk->count))   return FALSE;
	if (!story_data(bd,sizeof(blk->min),&blk->min))       return FALSE;
	if (!story_data(bd,sizeof(blk->max),&blk->max))       return FALSE;
	if (!story_data(bd,sizeof(blk->stat),&blk->stat))     return FALSE;
	if (!story_data(bd,sizeof(blk->listh.num),&blk->listh.num))  return FALSE;
	//  ------------------------------------------------
	irec = (flg_list) ? get_new_irec_num_to_save(blk->irec) : blk->irec;
	if (!story_data(bd, sizeof(irec), &irec)) return FALSE;
	//------------------------------------------------------------------
	offset_save = get_buf_offset(bd);    offset = -1;
	//     
	if (!story_data(bd,sizeof(offset),&offset))     return FALSE;
	if ( blk->type == 0 ) { // 
		hobj = blk->listh.hhead;
		while ( hobj ) {
			if (!o_save_obj(bd,hobj)) return FALSE;
			get_next_item(hobj,&hobj);
		}
	}
	offset = get_buf_offset(bd);
	if ( !seek_buf(bd, offset_save) )                return FALSE;
	if ( !story_data(bd,sizeof(offset),&offset))     return FALSE;
	if ( !seek_buf(bd, -1) )                         return FALSE;
	return TRUE;
}

BOOL o_save_obj(lpBUFFER_DAT bd,hOBJ hobj)
{
	BOOL cod = TRUE;
	SCAN_CONTROL sc;

  save_type_g = (OSCAN_COD(**)(lpBUFFER_DAT bd, lpOBJ obj))GetMethodArray(OMT_SAVE);

	init_scan(&sc);
	sc.user_pre_scan  = save_pre_scan;
	sc.user_post_scan = save_post_scan;
	sc.data		        = bd;
	if ( o_scan(hobj,&sc) == OSFALSE ) cod = FALSE;
  return cod;
}
#pragma argsused
static OSCAN_COD save_post_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	int offset_end, len;
  lpBUFFER_DAT bd = (BUFFER_DAT*)lpsc->data;

	offset_end = get_buf_offset(bd);  //   
	len = offset_end - lpsc->lparam;
	if ( !seek_buf(bd,lpsc->lparam-sizeof(int)) ) return OSFALSE;
	if( !story_data(bd,sizeof(len),&len))  return OSFALSE;
	if ( !seek_buf(bd,offset_end) ) return OSFALSE;
	return OSTRUE;
}
static OSCAN_COD save_pre_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	lpOBJ 		obj;
	hCSG  		hcsg;
	lpBUFFER_DAT bd = (lpBUFFER_DAT)lpsc->data;
	IDENT_V 	irec;
	OSCAN_COD cod;
	int			  offset = 0;
	short		  type;

	if ( !save_type_g[lpsc->type] ) {
		handler_err(ERR_NUM_OBJECT);
		return OSFALSE;
	}
	obj = (lpOBJ)hobj;
	type = (short)obj->type;
	if( !story_data(bd,sizeof(type),&type))		               goto err;
	if( !story_data(bd,sizeof(offset),&offset))			         goto err;
	lpsc->lparam = get_buf_offset(bd);  //   
	if( !story_data(bd,sizeof(obj->layer),&obj->layer))      goto err;
	if( !story_data(bd,sizeof(obj->color),&obj->color))      goto err;
	if( !story_data(bd,sizeof(obj->ltype),&obj->ltype))      goto err;
	if( !story_data(bd,sizeof(obj->lthickness),&obj->lthickness))    goto err;
	if( !story_data(bd,sizeof(obj->status),&obj->status) )   goto err;
	if( !story_data(bd,sizeof(obj->drawstatus),&obj->drawstatus) )   goto err;
	//  ---------------------------------------
	irec = (flg_list) ? get_new_irec_num_to_save(obj->irec) : obj->irec;
	if (!story_data(bd, sizeof(irec), &irec) ) goto err;
	//---------------------------------------------------------
	hcsg = obj->hcsg;
	if ( !save_csg(bd,hcsg) ) return OSFALSE;
	{
	int tmp_int = (int)lpsc->status;
	tmp_int &= ~(ST_ORIENT|ST_DIRECT);
	lpsc->status = (OSTATUS)tmp_int;//&=~(ST_ORIENT|ST_DIRECT);  //   
	}
	obj = (lpOBJ)hobj;
	cod = save_type_g[lpsc->type](bd,obj);
	return cod;
err:
	return OSFALSE;
}
OSCAN_COD simple_save(lpBUFFER_DAT bd, lpOBJ obj)
{
	return (OSCAN_COD)story_data(bd,geo_size[obj->type],obj->geo_data);
}
OSCAN_COD text_save(lpBUFFER_DAT bd, lpOBJ obj)
{
  GEO_TEXT gt;
  memcpy(&gt, obj->geo_data, geo_size[obj->type]);
	if(flg_list){
    gt.font_id = get_new_ft_num_to_save(gt.font_id);
    gt.text_id = get_new_ft_num_to_save(gt.text_id);
  }
  return (OSCAN_COD)story_data(bd,geo_size[obj->type],&gt);
}
OSCAN_COD dim_save(lpBUFFER_DAT bd, lpOBJ obj)
{
	GEO_DIM gt;

	memcpy(&gt, obj->geo_data, geo_size[obj->type]);
	if(flg_list){
		gt.dtstyle.font = get_new_ft_num_to_save(gt.dtstyle.font);
		gt.dtstyle.text = get_new_ft_num_to_save(gt.dtstyle.text);
	}
	return (OSCAN_COD)story_data(bd,geo_size[obj->type],&gt);
}
OSCAN_COD path_save(lpBUFFER_DAT bd, lpOBJ obj)
{
	lpGEO_PATH	p = (lpGEO_PATH)obj->geo_data;
	short				num_obj;

	num_obj = (short)p->listh.num;
	if( !story_data(bd,sizeof(p->matr),p->matr))  	return OSFALSE;
	if( !story_data(bd,sizeof(p->min),&p->min) )  	return OSFALSE;
  if( !story_data(bd,sizeof(p->max),&p->max) )  	return OSFALSE;
  if( !story_data(bd,sizeof(p->kind),&p->kind) )  return OSFALSE;
  if( !story_data(bd,sizeof(num_obj),&num_obj))		return OSFALSE;
  return OSTRUE;
}
OSCAN_COD brep_save( lpBUFFER_DAT bd, lpOBJ obj )
{
	lpGEO_BREP p = (lpGEO_BREP)obj->geo_data;
	int	       num;
	LNP        lnp;

	if( !story_data(bd,sizeof(p->matr),p->matr))            goto err;
	if( !story_data(bd,sizeof(p->type),&p->type) )          goto err;
	if( !story_data(bd,sizeof(p->ident_np),&p->ident_np) )  goto err;
	if( !story_data(bd,sizeof(p->min),&p->min) )            goto err;
	if( !story_data(bd,sizeof(p->max),&p->max) )            goto err;
	num = p->num;

  if ( flg_list ) { //      
		if( !read_elem(&vd_brep,num,&lnp) )                 goto err;
		num = lnp.num;    //    
  }
  if( !story_data(bd,sizeof(num),&num) )     goto err;

	return OSTRUE;
err:
	return OSFALSE;
}
OSCAN_COD bgroup_save( lpBUFFER_DAT bd, lpOBJ obj )
{
	lpGEO_GROUP p = (lpGEO_GROUP)obj->geo_data;
	short       num_obj;

	num_obj = (short)p->listh.num;
	if( !story_data(bd,sizeof(p->matr),p->matr))         goto err;
  if( !story_data(bd,sizeof(p->min),&p->min) )         goto err;
  if( !story_data(bd,sizeof(p->max),&p->max) )         goto err;
  if( !story_data(bd,sizeof(num_obj),&num_obj))        goto err;
	return OSTRUE;
err:
  return OSFALSE;
}
OSCAN_COD insert_save( lpBUFFER_DAT bd, lpOBJ obj )
{
	lpGEO_INSERT p = (lpGEO_INSERT)obj->geo_data;
  BLOCKS_MAP  map;

  //    
  if( !story_data(bd,sizeof(p->matr),p->matr))  goto err;
  if( !story_data(bd,sizeof(p->min),&p->min) )  goto err;
  if( !story_data(bd,sizeof(p->max),&p->max) )  goto err;
	map.number = p->num;
	if ( flg_list ) { //    ,  
    if (!read_elem(vdim_map,p->num,&map)) return OSFALSE;
	}
  if( !story_data(bd,sizeof(map.number),&map.number) )  goto err;
  return OSSKIP;
err:
	return OSFALSE;
}
OSCAN_COD spline_save(lpBUFFER_DAT bd, lpOBJ obj)
{
	WORD      		len;
	void *			p;
	lpGEO_SPLINE 	g = (lpGEO_SPLINE)obj->geo_data;
	BOOL 					cod;

	if ( !story_data(bd,sizeof(g->type),&g->type)) return OSFALSE;
  if ( !story_data(bd,sizeof(g->nump),&g->nump)) return OSFALSE;
  if ( !story_data(bd,sizeof(g->numd),&g->numd)) return OSFALSE;
	if ( !story_data(bd,sizeof(g->numu),&g->numu)) return OSFALSE;
  if ( !story_data(bd,sizeof(g->degree),&g->degree)) return OSFALSE;

  p = (void*)g->hpoint;
  len = g->nump * sizeof(DA_POINT);
	cod = story_data(bd,len,p);
	if ( !cod )	goto errs;

	if (g->numd > 0) {
		len = g->numd * sizeof(SNODE) ;
		p = (void*)g->hderivates;
		cod = story_data(bd,len,p);
		if ( !cod )	goto errs;
	}

	len = g->numu * sizeof(sgFloat);
	p = (void*)g->hvector;
	cod = story_data(bd,len,p);
	if ( !cod )	goto errs;

	return OSTRUE;
errs:
	return OSFALSE;
}
OSCAN_COD frame_save(lpBUFFER_DAT bd, lpOBJ obj)
{
	lpGEO_FRAME g = (lpGEO_FRAME)obj->geo_data;
	GEO_OBJ     geo;
	OBJTYPE     type;
	short       color,tmps;
	UCHAR       ltype, lthick;

//  if ( !story_data(bd,sizeof(GEO_FRAME),obj->geo_data)) return OSFALSE;

	if( !story_data(bd,sizeof(g->matr),g->matr))  return OSFALSE;
	if( !story_data(bd,sizeof(g->min),&g->min) )  return OSFALSE;
	if( !story_data(bd,sizeof(g->max),&g->max) )  return OSFALSE;
	if ( !story_data(bd,22,obj->geo_data)) return OSFALSE;

	if (!story_data(bd,sizeof(g->num),&g->num)) 		return OSFALSE;
	frame_read_begin(g);
	while ( frame_read_next(&type, &color, &ltype, &lthick, &geo)) {
  	tmps = type;
		if (!story_data(bd,sizeof(tmps),&tmps)) 			return OSFALSE;
		if (!story_data(bd,sizeof(color),&color)) 		return OSFALSE;
		// Zframe-------------------------
		if (!story_data(bd,sizeof(ltype),&ltype)) 		return OSFALSE;
		if (!story_data(bd,sizeof(lthick),&lthick)) 	return OSFALSE;
    //---------------------------------
		if (!story_data(bd,geo_size[type],&geo)) 			return OSFALSE;
	}
	return OSTRUE;
}

BOOL story_np(lpBUFFER_DAT bd)
{
  WORD    size;
	short		type;

	type = npwg->type;
  if ( type == TNPW ) type = TNPB;

  if ( !story_data(bd,sizeof(type),&type) ) return FALSE;
  if ( !story_data(bd,sizeof(npwg->nov),&npwg->nov) ) return FALSE;
  if ( !story_data(bd,sizeof(npwg->noe),&npwg->noe) ) return FALSE;
	if ( !story_data(bd,sizeof(npwg->nof),&npwg->nof) ) return FALSE;
  if ( !story_data(bd,sizeof(npwg->noc),&npwg->noc) ) return FALSE;

  if ( !story_data(bd,sizeof(npwg->ident),&npwg->ident) ) return FALSE;
  if ( !story_data(bd,sizeof(npwg->surf),&npwg->surf) ) return FALSE;			// 31/05/2001
  if ( !story_data(bd,sizeof(npwg->or_surf),&npwg->or_surf) ) return FALSE;	// 31/05/2001

	size = sizeof(D_POINT) * npwg->nov;
  if ( !story_data(bd,size,&npwg->v[1]) ) return FALSE;

  	size = sizeof(VERTINFO) * npwg->nov;
  if ( !story_data(bd,size,&npwg->vertInfo[1]) ) return FALSE;

  size = sizeof(EDGE_FRAME) * npwg->noe;
	if ( !story_data(bd,size,&npwg->efr[1]) ) return FALSE;

  if ( type == TNPF ) return TRUE;

  size = sizeof(EDGE_FACE) * npwg->noe;
  if ( !story_data(bd,size,&npwg->efc[1]) ) return FALSE;

	size = sizeof(FACE) * npwg->nof;
  if ( !story_data(bd,size,&npwg->f[1]) ) return FALSE;

  size = sizeof(CYCLE) * npwg->noc;
  if ( !story_data(bd,size,&npwg->c[1]) ) return FALSE;

  size = sizeof(GRU) * npwg->noe;
  if ( !story_data(bd,size,&npwg->gru[1]) ) return FALSE;

	return TRUE;
}

static BOOL save_vd_brep_and_attr( lpBUFFER_DAT bd, lpLISTH listh,
                                   NUM_LIST list_zudina)
{
	register short j;
	LNP         lnp;
	int        i;
	VI_LOCATION viloc;
	int        new_number;

	if ( !calk_new_order_np_and_attr(listh, list_zudina, &new_number) )
		return FALSE;
  //    ,   ----------
  if( !save_attr_info(bd) ) return FALSE;
  //----------------------------------------------------------------
	if( !story_data(bd,sizeof(new_number),&new_number) )   return FALSE;
//    
	for ( i=0; i< vd_brep.num_elem; i++ ) {
		if ( !(read_elem(&vd_brep,i,&lnp)) ) return FALSE;
		if ( lnp.num == -1 ) continue;
    if( !story_data(bd,sizeof(lnp.kind),&lnp.kind) )   return FALSE;
		if ( lnp.kind != COMMON ) {
			if ( !save_csg(bd, lnp.hcsg) ) return FALSE;
      continue;
    }
		if( !story_data(bd,sizeof(lnp.num_np),&lnp.num_np) )   return FALSE;
		get_first_np_loc( &lnp.listh, &viloc);
		begin_read_vld(&viloc);
		for ( j=0; j<lnp.num_np; j++ ) {
			if ( !read_np_mem(npwg) ) {
				end_read_vld();
				return FALSE;
			}
			if ( !story_np(bd) )  return FALSE;
		}
		end_read_vld();
    // 17/05/2001
		if( !story_data(bd,sizeof(lnp.num_surf),&lnp.num_surf) )   return FALSE;
		get_first_np_loc( &lnp.surf, &viloc);
		begin_read_vld(&viloc);
		for ( j=0; j<lnp.num_surf; j++ ) {
			if ( !story_surf(bd) ) {
				end_read_vld();
				return FALSE;
			}
		}
		end_read_vld();
	}
	return TRUE;
}

static OSCAN_COD attr_and_np_pre_scan(hOBJ hobj, lpSCAN_CONTROL lpsc);
static OSCAN_COD np_pre_scan(hOBJ hobj, lpSCAN_CONTROL lpsc);
static BOOL  calk_new_order_np_and_attr(lpLISTH listh, NUM_LIST list_zudina,
                                        int *num)
{
	register int i;
	lpLNP         lplnp;
	int          number;//nb = 0;
	SCAN_CONTROL	sc;
	hOBJ          hobj;
	IBLOCK				blk;


//      
  init_attr_save_info();

//    
//     	  
/*-
  if ( list_zudina == OBJ_LIST ) { //   
		number = 0;
		if ( !begin_rw(&vd_brep,0)) return FALSE;
    for ( i=0; i< vd_brep.num_elem; i++ ) {
      if ( !(lplnp = get_next_elem(&vd_brep))) return FALSE;
			if ( lplnp->count ) lplnp->num = number++;
      else                lplnp->num = -1;
    }
    end_rw(&vd_brep);
    *num = number;
    return TRUE;
  }
-*/
	if ( !begin_rw(&vd_brep,0)) return FALSE;
	for ( i=0; i< vd_brep.num_elem; i++ ) {
		if ((lplnp = (LNP*)get_next_elem(&vd_brep)) == NULL) return FALSE;
    lplnp->num = -1;
	}
	end_rw(&vd_brep);

  //   BREP  
	init_scan(&sc);
	sc.user_pre_scan = attr_and_np_pre_scan;

	if ( list_zudina == OBJ_LIST ) {
		//        
		//           
		for ( i=0; i< vd_blocks.num_elem; i++ ) {
			if (!read_elem(&vd_blocks,i,&blk)) return FALSE;
			if ( (blk.type&BLK_HOLDER) ) continue;
			if ( blk.stat != BS_ENABLE ) continue;
			//    
			if(!mark_record_to_save(blk.irec)) return FALSE;
			if ( (blk.type&BLK_EXTERN) ) continue; //  
			hobj = blk.listh.hhead;
			while (hobj) {
				if (!o_scan(hobj,&sc))  return FALSE;
				get_next_item(hobj,&hobj);
			}
		}
	}
	hobj = listh->hhead;
	while (hobj) {
		if (!o_scan(hobj,&sc))  return FALSE;
		get_next_item_z(list_zudina,hobj,&hobj);
	}
//    
	number = 0;
	if ( !begin_rw(&vd_brep,0)) return FALSE;
	for ( i=0; i< vd_brep.num_elem; i++ ) {
		if ((lplnp = (LNP*)get_next_elem(&vd_brep)) == NULL) return FALSE;
		if ( lplnp->num != -1 ) lplnp->num = number++;
	}
	end_rw(&vd_brep);
	*num = number;
	return TRUE;
}

static OSCAN_COD attr_and_np_pre_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
lpOBJ    obj;
IDENT_V  irec, ifont = 0, itext = 0;
int     bnum;
lpIBLOCK lblk;
BOOL			flg_skip = FALSE;

	obj = (lpOBJ)hobj;
	irec = obj->irec;
	if(lpsc->type == OINSERT)
		bnum = ((lpGEO_INSERT)(obj->geo_data))->num;
	if(lpsc->type == OTEXT){
		ifont = ((lpGEO_TEXT)(obj->geo_data))->font_id;
		itext = ((lpGEO_TEXT)(obj->geo_data))->text_id;
	}
	if(lpsc->type == ODIM){
		ifont = ((lpGEO_DIM)(obj->geo_data))->dtstyle.font;
		itext = ((lpGEO_DIM)(obj->geo_data))->dtstyle.text;
	}
	//   
	if(!mark_record_to_save(irec)) return OSFALSE;
	if(lpsc->type == OINSERT){ //   
		if((lblk = (IBLOCK*)get_elem(&vd_blocks, bnum)) == NULL) return OSFALSE;
		if ( lblk->type & BLK_EXTERN ) flg_skip = TRUE;
		irec = lblk->irec;
		if(!mark_record_to_save(irec)) return OSFALSE;
	}
	//     
	mark_ft_item_to_save(ifont);
	mark_ft_item_to_save(itext);

	if ( flg_skip ) return OSSKIP;
	return np_pre_scan(hobj, lpsc);
}

static OSCAN_COD np_pre_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	if ( lpsc->type != OBREP ) return OSTRUE;
	{
		lpOBJ         obj;
		int          num;
		lpLNP         lnp;

		obj = (lpOBJ)hobj;
		num = ((lpGEO_BREP)(obj->geo_data))->num;
		if ((lnp=(LNP*)get_elem(&vd_brep,num)) == NULL) return OSFALSE;
		lnp->num = 1;
  }
	return OSTRUE;
}

static void init_attr_save_info(void)
//   VDIM    sinfo
{
short           j, n = 3;
register int i;
lpVDIM        vd[4];
lpATTR        lpattr;

	vd[0]  = vd_attr; vd[1]  = vd_attr_item; vd[2]  = vd_record;
  if(ft_bd){ vd[3] = &ft_bd->vd; n = 4;}
	for(j = 0; j < n; j++){
    if(vd[j]->num_elem){
      if(!begin_rw(vd[j], 0)) attr_exit();
      for(i = 0; i < vd[j]->num_elem; i++){
				if((lpattr = (ATTR*)get_next_elem(vd[j])) == NULL) attr_exit();
        lpattr->sinfo = 0;
			}
			end_rw(vd[j]);
    }
  }
}

static BOOL mark_record_to_save(IDENT_V rec_id)
//     rec_id    
//   
{
ATTR_RECORD   rec;
lpRECORD_ITEM irec;//nb = NULL;
short           i;

  if(!rec_id) return TRUE;
  //   rec_id
	if((irec = alloc_and_load_record(rec_id, &rec)) == NULL) return FALSE;
	if(rec.num && (!rec.sinfo)){ //     
    rec.sinfo = 1;
    //   
    for(i = 0; i < rec.num; i++){
      mark_attr_to_save(irec[i].attr);
			mark_attr_item_to_save(irec[i].item);
		}
    write_record(rec_id, &rec);
  }
  SGFree(irec);
  return TRUE;
}

static void mark_attr_to_save(IDENT_V attr_id)
//       attr_id
{
ATTR attr;

  if(!attr_id) return;

  read_attr(attr_id, &attr);
  if(!attr.sinfo){ //     
    attr.sinfo = 1;
    if(attr.type == ATTR_BOOL){
      mark_attr_item_to_save(attr.ilisth.head);
      mark_attr_item_to_save(attr.ilisth.tail);
    }
		if(attr.curr) mark_attr_item_to_save(attr.curr);
		write_attr(attr_id, &attr);
	}
}

static void mark_attr_item_to_save(IDENT_V item_id)
//       item_id
{
lpATTR_ITEM attr_item;

  if(!item_id) return;
	attr_item = lock_attr_item(item_id);
	attr_item->sinfo = 1;
  unlock_attr_item();
}

static void mark_ft_item_to_save(IDENT_V item_id)
//      item_id     

{
lpFT_ITEM ft_item;

	if(!item_id) return;
	if((ft_item = (FT_ITEM*)get_elem(&ft_bd->vd, item_id - 1)) == NULL) attr_exit();
	ft_item->sinfo = 1;
}

static BOOL save_attr_info(lpBUFFER_DAT bd)
//    ,   
{
ATTR          attr;
lpATTR        lpattr;
ATTR_ITEM     attr_item;
FT_ITEM       ft_item;
lpATTR_ITEM   lpattr_item;
ATTR_RECORD   rec;
lpRECORD_ITEM irec;
IDENT_V       attr_id, item_id, rec_id, ft_id, id_num = 0;
WORD          i;
ULONG         len;
char          c0 = 0;
char          *value;
UCHAR         type;
lpFONT        font;
BOOL          font_flag;

//    
  attr_id = attr_ilisth->head;
	while(attr_id){
		read_attr(attr_id, &attr);
		if(attr.sinfo){
    //   
      attr.sinfo = ++id_num;
      //   
			if(!story_attr_string(bd,                 attr.name))      return FALSE;
			if(!story_attr_string(bd,                 attr.text))      return FALSE;
			if(!story_data(bd, sizeof(attr.type),     &attr.type))     return FALSE;
			if(!story_data(bd, sizeof(attr.status),   &attr.status))   return FALSE;
      if(!story_data(bd, sizeof(attr.size),     &attr.size))     return FALSE;
			if(!story_data(bd, sizeof(attr.precision),&attr.precision))return FALSE;
			//      
      item_id = attr.ilisth.head;
      while(item_id){
        read_attr_item(item_id, &attr_item);
        if(attr_item.sinfo){
          attr_item.sinfo = ++id_num;
          c0 = (BYTE)((item_id == attr.curr) ? 2 : 1);
          if(!story_data(bd, 1, &c0))     return FALSE;
          value = (char*)alloc_and_get_attr_value(item_id, &type, &len);
          if(!value) return FALSE;
          if(!story_data(bd, sizeof(len), &len)) return FALSE;
					if(!story_data(bd, len, value)) return FALSE;
					write_attr_item(item_id, &attr_item);
          SGFree(value);
				}
        item_id = get_next_attr_item(item_id);
      }
      c0 = 0;
      if(!story_data(bd, 1, &c0)) return FALSE;
      write_attr(attr_id, &attr);
    }
		attr_id = get_next_attr(attr_id);
  }
	if(!story_data(bd, 1, &c0)) return FALSE;

  if(!story_data(bd, sizeof(max_record_len), &max_record_len)) return FALSE;
  if(!max_record_len) goto met_font;
  //    
	if((irec = (RECORD_ITEM*)SGMalloc(max_record_len)) == NULL){
    attr_handler_err(AT_HEAP);
    return FALSE;
  }
  rec_id = record_ilisth->head;
  while(rec_id){
		load_record(rec_id, &rec, irec);
		if(rec.sinfo){
      //  
      rec.sinfo = ++id_num;
			if(!story_data(bd, sizeof(rec.num), &rec.num)) goto err;
      for(i = 0; i < rec.num; i++){
        lpattr = lock_attr(irec[i].attr);
        irec[i].attr = lpattr->sinfo;
        unlock_attr();
        if(irec[i].item){
					lpattr_item = lock_attr_item(irec[i].item);
					irec[i].item = lpattr_item->sinfo;
					unlock_attr_item();
        }
      }
      if(!story_data(bd, get_record_size(&rec), irec)) goto err;
      write_record(rec_id, &rec);
    }
    rec_id = get_next_record(rec_id);
  }
  SGFree(irec);
  rec.num = 0;
	if(!story_data(bd, sizeof(rec.num), &rec.num)) return FALSE;
met_font:  //    
  if(!story_data(bd, sizeof(ft_bd->max_len), &ft_bd->max_len)) return FALSE;
  if(!ft_bd->max_len) return TRUE;
  //    
  if((font = (FONT*)SGMalloc(ft_bd->max_len)) == NULL){
		attr_handler_err(AT_HEAP);
		return FALSE;
	}
	//  
	font_flag = TRUE;
	ft_id = ft_bd->ft_listh[FTFONT].head;
met_text:
	while(ft_id){
    if(!read_elem(&ft_bd->vd, ft_id - 1, &ft_item)) attr_exit();
    if(ft_item.sinfo){
      // 
      ft_item.sinfo = ++id_num;
      len = ft_item_value(&ft_item, font);
      if(font_flag) len = strlen(font->name) + 1;
      if(!story_data(bd, sizeof(len), &len)) goto err1;
      if(!story_data(bd, len, font))   goto err1;
      if(!write_elem(&ft_bd->vd, ft_id - 1, &ft_item)) attr_exit();
    }
		if(!il_get_next_item(&ft_bd->vd, ft_id, &ft_id))attr_exit();
  }
  len = 0;
  if(!story_data(bd, sizeof(len), &len)) goto err1;
  if(font_flag){ //  
		font_flag = FALSE;
		ft_id = ft_bd->ft_listh[FTTEXT].head;
		goto met_text;
	}
	SGFree(font);
	return TRUE;
err1:
  SGFree(font);
	return FALSE;
err:
  SGFree(irec);
  return FALSE;
}


static BOOL save_obj_types_map(lpBUFFER_DAT bd)
//     
{
int i; 
//   
  if(!story_data(bd, sizeof(GeoObjTypesCount), &GeoObjTypesCount))
    return FALSE;
//   
  for(i = 0; i < GeoObjTypesCount; i++){
		if(!story_attr_string(bd, ppGeoObjNames[i]))
      return FALSE;
  }
  return TRUE;
}

static IDENT_V get_new_irec_num_to_save(IDENT_V irec)
{
ATTR_RECORD rec;
	if(!irec) return irec;
	read_record(irec, &rec);
  if(!rec.num) return 0;
  return rec.sinfo;
}

static IDENT_V get_new_ft_num_to_save(IDENT_V id)
{
FT_ITEM ft;
  if(!id) return id;
  if(!read_elem(&ft_bd->vd, id - 1, &ft)) attr_exit();
  return ft.sinfo;
}

// 17.05.2001
static BOOL story_surf(lpBUFFER_DAT bd)
{
  int           size;
  VADR			hBuf;
  void * lpBuf;
  GEO_SURFACE	surf;
  BOOL			ret;

  read_vld_data(sizeof(surf.type),&surf.type);
	if (!story_data(bd, sizeof(surf.type), &surf.type)) return FALSE;

  read_vld_data(sizeof(surf.numpu), &surf.numpu);
	if (!story_data(bd, sizeof(surf.numpu), &surf.numpu)) return FALSE;

  read_vld_data(sizeof(surf.numpv), &surf.numpv);
	if (!story_data(bd, sizeof(surf.numpv), &surf.numpv)) return FALSE;

  read_vld_data(sizeof(surf.numd), &surf.numd);
	if (!story_data(bd, sizeof(surf.numd), &surf.numd)) return FALSE;

  read_vld_data(sizeof(surf.num_curve), &surf.num_curve);
	if (!story_data(bd, sizeof(surf.num_curve), &surf.num_curve)) return FALSE;

  read_vld_data(sizeof(surf.degreeu), &surf.degreeu);
	if (!story_data(bd, sizeof(surf.degreeu), &surf.degreeu)) return FALSE;

  read_vld_data(sizeof(surf.degreev), &surf.degreev);
	if (!story_data(bd, sizeof(surf.degreev), &surf.degreev)) return FALSE;

  read_vld_data(sizeof(surf.orient), &surf.orient);
	if (!story_data(bd, sizeof(surf.orient), &surf.orient)) return FALSE;

	int sz1 = sizeof(D_POINT) * surf.numpu * surf.numpv;
	int sz2 = sizeof(MNODE) * surf.numd;

  size = max(sz1 , sz2 );
  hBuf = SGMalloc(size);
  if (!hBuf) return FALSE;

  if ((lpBuf = (void*)hBuf) == NULL) { SGFree(hBuf); return FALSE; }

  size = sizeof(D_POINT) * surf.numpu * surf.numpv;
  read_vld_data(size, lpBuf);
	if (!story_data(bd, size, lpBuf)) goto err;

  size = sizeof(sgFloat) * (surf.numpu+surf.degreeu+1+surf.numpv+surf.degreev+1);
  read_vld_data(size, lpBuf);
	if (!story_data(bd, size, lpBuf)) goto err;

  if (surf.numd > 0) {
	  size = sizeof(MNODE) * surf.numd;
	  read_vld_data(size, lpBuf);
		if (!story_data(bd, size, lpBuf)) goto err;
  }

  if (surf.num_curve > 0) {	//  
  }

  ret = TRUE;
ex:
  SGFree(hBuf);
	return ret;
err:
	ret = FALSE;
 	goto ex;
}

