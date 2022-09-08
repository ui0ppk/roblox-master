#include "../../sg.h"

static VERSION_MODEL	ver = (VERSION_MODEL)(LAST_VERSION - 1);
static int num_brep_prev = 0;
static char flg_list = 0;									//   
static APPL_NUMBER appl_number = APPL_SG; //  
static short appl_version = 0; 						//   

static lpVDIM vd_ident;

static BOOL load_one_block_dos(lpBUFFER_DAT bd);
static BOOL o_load_obj_dos(lpBUFFER_DAT bd,hOBJ * hobj);
static BOOL load_np_dos(lpBUFFER_DAT bd);


static BOOL load_vd_brep(lpBUFFER_DAT bd);
static BOOL load_attr_info(lpBUFFER_DAT bd, lpVDIM vd);
static IDENT_V get_new_irec_num_to_load(IDENT_V irec);

static BOOL  load_blocks_v1(lpBUFFER_DAT bd, char* title_grad);
static BOOL  load_blocks_v2(lpBUFFER_DAT bd, char* title_grad);

static OSCAN_COD load_pre_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);


static OSCAN_COD  (**load_type_g)(lpBUFFER_DAT bd, hOBJ hobj);

static hOBJ hhold = NULL;

BOOL o_load_list_dos(lpBUFFER_DAT bd,lpLISTH listh, NUM_LIST list_zudina,
								     short *num_obj, VERSION_MODEL verm, void* pProp)
{
	int            num,i;
	hOBJ            hobj;
	BOOL            cod = FALSE;
	VDIM            vd;
  short						tmps;

	*num_obj = 0;
	flg_list = 1;  //   
	num_brep_prev = vd_brep.num_elem;
	init_vdim(&vd, sizeof(IDENT_V));
	ver = verm;

	if ( ver >= VERSION_1_11 ) { //    
		if (load_data(bd,sizeof(tmps),&tmps) != sizeof(tmps)) goto err;
    appl_number_load = (APPL_NUMBER)tmps;
    appl_number = (APPL_NUMBER)tmps;
		if (load_data(bd,sizeof(tmps),&tmps) != sizeof(tmps))		goto err;
    appl_version = appl_version_load = tmps;
   /* if(pProp)
      ScenePropSetModelVersionsInfo((pSCENE_PROP)pProp,  ver, appl_number, appl_version);*/
	}
	if ( ver >= VERSION_1_10 )
		if ( !load_attr_info(bd, &vd) ) goto err;
	if ( ver >= VERSION_1_7 )
		if ( !load_vd_brep(bd) ) goto err;
//  
	switch ( ver ) {
		case VERSION_1_5:
			if ( !load_blocks_v1(bd,"Load (1/2)")) goto err;
			break;
		default:
			if ( ver >= VERSION_1_6 ){
				if ( !load_blocks_v2(bd,"Load (1/2)")) goto err;
				break;
			}
			handler_err(ERR_VERSION);
			goto err;
	}
//  
	if (load_data(bd,sizeof(num),&num) != sizeof(num))  goto err;
//	num_proc = start_grad("Load (2/2)", num);
	if (application_interface && application_interface->GetProgresser()) 
		application_interface->GetProgresser()->Progress(0);
	
	for (i=0; i< num; i++) {
//		step_grad (num_proc , i);
		if (application_interface && application_interface->GetProgresser()) 
			application_interface->GetProgresser()->Progress(100*i/num);
		if (!o_load_obj_dos(bd,&hobj)) goto end;
		if ( hobj ) {
			attach_item_tail_z(list_zudina,listh,hobj);
			(*num_obj)++;
		}
	}
	cod = TRUE;
end:
//	end_grad  (num_proc , i);
	if (application_interface && application_interface->GetProgresser()) 
		application_interface->GetProgresser()->Progress(100);
//	*num_obj = (short)i;
err:
	free_vdim(&vd);
	if ( !cod ) free_np_brep_list_num(num_brep_prev);
	flg_list = 0;
	num_brep_prev = 0;
	ver = (VERSION_MODEL)(LAST_VERSION - 1);
	bOemModelFlag = FALSE;
	return cod;
}
static BOOL load_vd_brep(lpBUFFER_DAT bd)
{
	register short j;
	LNP   lnp;
	VLD   vld;
	int  i,numbrep;

	num_brep_prev = (short)vd_brep.num_elem;
	if (load_data(bd,sizeof(numbrep),&numbrep) != sizeof(numbrep))  return FALSE;
	if ( !numbrep ) return TRUE;
	for (i=0; i<numbrep; i++) {
		memset(&lnp,0,sizeof(lnp));
		if ( ver >= VERSION_1_8 ) {
			if ( load_data(bd,sizeof(lnp.kind),&lnp.kind) != sizeof(lnp.kind))
				goto err;
			if ( lnp.kind != COMMON ) {
				lpCSG csg;

				if ( !load_csg(bd,&lnp.hcsg) ) goto err;
				csg = (lpCSG)lnp.hcsg;
				lnp.num_np = create_primitive_np((BREPKIND)lnp.kind,&lnp.listh,
																				(sgFloat *) csg->data);
				if ( !lnp.num_np ) goto err;
				if ( !add_elem(&vd_brep,&lnp) ) goto err;
				continue;
			}
    }
		if ( load_data(bd,sizeof(lnp.num_np),&lnp.num_np) != sizeof(lnp.num_np))
			goto err;
		init_vld(&vld);
		for (j=0; j<lnp.num_np; j++) {
			if ( !load_np_dos(bd) ) goto err;
			if ( !add_np_mem(&vld,npwg,(NPTYPE)npwg->type) ) goto err;
		}
		lnp.listh = vld.listh;
		if ( !add_elem(&vd_brep,&lnp) ) goto err;
	}
	return TRUE;
err:
	free_np_brep_list_num(num_brep_prev);
	return FALSE;
}


static BOOL load_attr_info(lpBUFFER_DAT bd, lpVDIM vd)
//    
{
ATTR          attr;
lpATTR        lpattr;
ATTR_RECORD   rec;
lpRECORD_ITEM irec;
IDENT_V       attr_id, item_id, rec_id, idft;
lpIDENT_V     lpid;
WORD          i, len;
UCHAR         l1;
UCHAR         *text = NULL;
char          *value;
char          prec1;
char          font_name[MAXFILE + MAXEXT + 1];
BOOL          flag_difftype;
short         errcode;

	vd_ident = vd;

	while(TRUE){ //  
		if(load_data(bd, sizeof(l1), &l1) != sizeof(l1)) return FALSE;
		if(!l1) break;
		memset(&attr, 0, sizeof(ATTR));
		if(load_data(bd, l1, attr.name) != l1)           return FALSE;
		test_to_oem(attr.name, l1);
		if(load_data(bd, sizeof(l1), &l1) != sizeof(l1)) return FALSE;
		if(load_data(bd, l1, attr.text) != l1)           return FALSE;
		test_to_oem(attr.text, l1);
		if(load_data(bd, sizeof(attr.type), &attr.type) != sizeof(attr.type))
			return FALSE;
		if(load_data(bd, sizeof(attr.status), &attr.status) != sizeof(attr.status))
			return FALSE;
		if(load_data(bd, sizeof(attr.size), &attr.size) != sizeof(attr.size))
			return FALSE;
		if(ver >= VERSION_1_11){
			if(load_data(bd, sizeof(attr.precision), &attr.precision) !=
				 sizeof(attr.precision)) return FALSE;
		}
		else{
			if(load_data(bd, 1, &prec1) != 1) return FALSE;
			attr.precision = prec1;
		}
		//    
		if(!add_attr(&attr, &attr_id)) return FALSE;
		flag_difftype = FALSE;
		if(attr_id < 0){ //   ,   
			flag_difftype = TRUE;
			attr_id = -attr_id;
		}
		//     
		if(!add_elem(vd_ident, &attr_id)) return FALSE;
		//     
		while(TRUE){
			if(load_data(bd, sizeof(l1), &l1)   != sizeof(l1))   return FALSE;
			if(!l1) break;
			if(load_data(bd, sizeof(len), &len) != sizeof(len))  return FALSE;
			if((value = (char*)SGMalloc(len)) == NULL){
				attr_handler_err(AT_HEAP);
				return FALSE;
			}
			if(load_data(bd, len, value) != len){SGFree(value);    return FALSE;}
			if(flag_difftype){
				item_id = 0;
				SGFree(value);
			}
			else{
				test_attr_value_to_oem((ATTR_TYPE)attr.type, value, len);
				if(!add_attr_value(attr_id, value, &item_id)){
					SGFree(value);
					return FALSE;
				}
				SGFree(value);
				if(l1 == 2) { //   
					lpattr = lock_attr(attr_id);
					if(!lpattr->curr) lpattr->curr = item_id;
					unlock_attr();
				}
			}
			if(!add_elem(vd_ident, &item_id)) return FALSE;
		}
	}
	//    
	if(load_data(bd, sizeof(len), &len) != sizeof(len)) return FALSE;
	if(!len) goto met_font;
	if((irec = (RECORD_ITEM*)SGMalloc(len)) == NULL) {
		attr_handler_err(AT_HEAP);
		return FALSE;
	}
	while(TRUE){
		memset(&rec, 0, sizeof(ATTR_RECORD));
		if(load_data(bd, sizeof(rec.num), &rec.num) != sizeof(rec.num)) goto err;
		if(!rec.num) break;
		len = get_record_size(&rec);
		if(load_data(bd, len, irec) != len) goto err;
		//   
		for(i = 0; i < rec.num; i++){
			if((lpid = (long*)get_elem(vd_ident, irec[i].attr - 1)) == NULL) attr_exit();
			irec[i].attr = *lpid;
			if(irec[i].item){
				if((lpid = (long*)get_elem(vd_ident, irec[i].item - 1)) == NULL) attr_exit();
				irec[i].item = *lpid;
			}
		}
		if(!(rec_id = add_record(&rec, irec))) goto err;
		if(!add_elem(vd_ident, &rec_id))       goto err;
	}
	SGFree(irec);
met_font:
	if(ver < VERSION_1_12) return TRUE;
	if(load_data(bd, sizeof(len), &len) != sizeof(len)) goto err1;
	if(!len) return TRUE;
	//  
	while(TRUE){
		if(load_data(bd, sizeof(len), &len) != sizeof(len)) goto err1;
		if(!len) break;
		if(load_data(bd, len, font_name) != len) goto err1;
		//   
		idft = set_font(font_name, &errcode);
		if(!idft) idft = ft_bd->def_font; //   -   
		if(!add_elem(vd_ident, &idft)) goto err1;
	}
	//  
	while(TRUE){
		if(load_data(bd, sizeof(len), &len) != sizeof(len)) goto err1;
		if(!len) break;
		if((text = (UCHAR*)SGMalloc(len)) == NULL){
			attr_handler_err(AT_HEAP);
			return FALSE;
		}
		if(load_data(bd, len, text) != len) goto err1;
		test_attr_value_to_oem(ATTR_TEXT, (char*)text, len);
		if(!(idft = add_ft_value(FTTEXT, text, len))) goto err1;
		SGFree(text); text = NULL;
		if(!add_elem(vd_ident, &idft)) goto err1;
	}
	return TRUE;
err1:
	if(text) SGFree(text);
	return FALSE;
err:
	SGFree(irec);
	return FALSE;
}

static IDENT_V get_new_irec_num_to_load(IDENT_V irec)
{
IDENT_V id;
	if(!irec) return irec;
	if(!read_elem(vd_ident, irec - 1, &id)) attr_exit();
	return id;
}

static BOOL  load_blocks_v1(lpBUFFER_DAT bd, char* title_grad)
{
	int    i,j,num;
	short  numblk;
	hOBJ   hobj;
	IBLOCK blk;
	BOOL   cod = FALSE;

	num_blk_prev = (short)vd_blocks.num_elem;
	if (load_data(bd,sizeof(numblk),&numblk) != sizeof(numblk))  return FALSE;
	if ( !numblk ) return TRUE;
//	num_proc = start_grad(title_grad, numblk);
//	if (progress_function) 
//		progress_function(true,false,0.0);

	for (i=0; i< numblk; i++) {
//		step_grad (num_proc , i);
//		if (progress_function) 
//			progress_function(false,false, 100.0f*(float)i/(float)numblk);

		memset(&blk,0,sizeof(blk));
		if ( load_data(bd,sizeof(blk.name),blk.name) != sizeof(blk.name)) goto end;
		test_to_oem(blk.name, sizeof(blk.name));
		if ( load_data(bd,sizeof(blk.min),&blk.min) != sizeof(blk.min))   goto end;
		if ( load_data(bd,sizeof(blk.max),&blk.max) != sizeof(blk.max))   goto end;
		if ( load_data(bd,sizeof(num),&num) != sizeof(num))  goto end;
		for ( j=0; j<num; j++ ) {
			if (!o_load_obj_dos(bd,&hobj)) goto end;
			attach_item_tail(&blk.listh,hobj);
		}
		if ( !add_elem(&vd_blocks,&blk) ) goto end;
	}
	cod = TRUE;
end:
//	end_grad  (num_proc , i);
//	if (progress_function) 
//		progress_function(false,true,100.0);
	return cod;
}

static BOOL  load_blocks_v2(lpBUFFER_DAT bd, char* title_grad)
{
	int    i;
	int    numblk;
	BOOL   cod = FALSE;

	num_blk_prev = (short)vd_blocks.num_elem;
	if (load_data(bd,sizeof(numblk),&numblk) != sizeof(numblk))  return FALSE;
	if ( !numblk ) return TRUE;
//	num_proc = start_grad(title_grad, numblk);
//	if (progress_function) 
//		progress_function(true,false,0.0);

	for (i=0; i< numblk; i++) {
//		step_grad (num_proc , i);
//		if (progress_function) 
//			progress_function(false,false, 100.0f*(float)i/(float)numblk);
		if ( !load_one_block_dos(bd)) goto end;
	}
	cod = TRUE;
end:
//	end_grad  (num_proc , i);
//	if (progress_function) 
//		progress_function(false,true,100.0);
	return cod;
}
// =======================================//
//	       
// =======================================//
static BOOL  load_one_block_dos(lpBUFFER_DAT bd)
{
	hOBJ    hobj;
	IBLOCK 	blk;
	short   attrib;
	int     j,num, offset;

	memset(&blk,0,sizeof(blk));
	if ( load_data(bd, 13, blk.name) != 13 ) return FALSE;
	test_to_oem(blk.name, sizeof(blk.name));
	if ( load_data(bd,sizeof(blk.count),&blk.count) != sizeof(blk.count))
		return FALSE;
	if ( load_data(bd,sizeof(blk.min),&blk.min) != sizeof(blk.min))   return FALSE;
	if ( load_data(bd,sizeof(blk.max),&blk.max) != sizeof(blk.max))   return FALSE;
	if ( load_data(bd,sizeof(blk.stat),&blk.stat) != sizeof(blk.stat))return FALSE;
	if ( load_data(bd,sizeof(num),&num) != sizeof(num))               return FALSE;
	if ( ver >= VERSION_1_10 ){
		if ( load_data(bd,sizeof(blk.irec),&blk.irec) != sizeof(blk.irec))
			return FALSE;
			blk.irec = get_new_irec_num_to_load(blk.irec);
			if(blk.irec){
				modify_record_count(blk.irec, 1);
				obj_attr_flag = 1;
			}
	}
	else
		if ( load_data(bd,sizeof(attrib),&attrib) != sizeof(attrib)) return FALSE;
	if ( load_data(bd,sizeof(offset),&offset) != sizeof(offset))      return FALSE;
	for ( j=0; j<num; j++ ) {
		if (!o_load_obj_dos(bd,&hobj)) return FALSE;
		attach_item_tail(&blk.listh,hobj);
	}
	if ( !add_elem(&vd_blocks,&blk) ) return FALSE;
	return TRUE;
}

static BOOL o_load_obj_dos(lpBUFFER_DAT bd,hOBJ * hobj)
{
	OBJTYPE				type;
	BOOL					cod = FALSE;
	OSCAN_COD			oscod;
	SCAN_CONTROL	sc;
	int					  offset;
  short					tmps;

	*hobj = NULL;

	load_type_g = (OSCAN_COD(**)(lpBUFFER_DAT bd, hOBJ hobj))GetMethodArray(OMT_LOAD_DOS);

	if ( load_data(bd,sizeof(tmps),&tmps) != sizeof(tmps))  goto m;
  type = tmps;
  type = FindObjTypeByOldType(type);
	if ( ver >= VERSION_1_11 ) {
		if ( load_data(bd,sizeof(offset),&offset) != sizeof(offset))  goto m;
//    type = FindObjTypeByOldType(type);
		if (type < 0) {
			if ( !seek_buf(bd,offset) ) goto m;  //   
			return FALSE;
//			return TRUE;
		}
	}
	if ( (*hobj = o_alloc(type)) == NULL ) goto m;
	init_scan(&sc);
	sc.user_pre_scan	= load_pre_scan;
	sc.data		        = bd;
	oscod = o_scan(*hobj,&sc);
	if ( oscod == OSFALSE) {
		o_free(*hobj,NULL);
		*hobj = 0;
		goto m;
	}
	cod = TRUE;
m:
	return cod;
}
#pragma argsused
static OSCAN_COD load_pre_scan(hOBJ hobj,lpSCAN_CONTROL lpsc)
{
	lpOBJ					obj;
	hCSG					hcsg = NULL;
	lpBUFFER_DAT	bd = (BUFFER_DAT*)lpsc->data;
	IDENT_V				irec = 0;

	obj = (lpOBJ)hobj;
	obj->hhold = hhold;
	if( load_data(bd,sizeof(obj->layer),&obj->layer) != sizeof(obj->layer))
			goto err;
	if( load_data(bd,sizeof(obj->color),&obj->color) != sizeof(obj->color))
			goto err;
	if( load_data(bd,sizeof(obj->status),&obj->status) != sizeof(obj->status))
			goto err;
	if( load_data(bd,sizeof(obj->drawstatus),&obj->drawstatus)
			!= sizeof(obj->drawstatus))		goto err;
	if ( ver >= VERSION_1_10 ){
		if ( load_data(bd,sizeof(irec),&irec) != sizeof(irec)) return OSFALSE;//FALSE;
		if(flg_list) irec = get_new_irec_num_to_load(irec);
		if(irec){
			modify_record_count(irec, 1);
			obj_attr_flag = 1;
		}
	}
	if ( !load_csg(bd,&hcsg) ) return OSFALSE;
	obj = (lpOBJ)hobj;
	obj->hcsg = hcsg;
	obj->irec = irec;

	return load_type_g[lpsc->type](bd,hobj);
err:
	return OSFALSE;
}

#pragma argsused
OSCAN_COD  dos_text_load  ( lpBUFFER_DAT bd, hOBJ hobj)
{
WORD       size;
lpOBJ      obj;
IDENT_V    itext, ifont;
lpGEO_TEXT geo;

	obj = (lpOBJ)hobj;
	size = geo_size[obj->type];
	if ( load_data(bd,size,(void*)obj->geo_data) != size ) {
		return OSFALSE;
	}
	geo = (lpGEO_TEXT)(obj->geo_data);
	ifont = geo->font_id;
	itext = geo->text_id;
	if(flg_list){
		ifont = get_new_irec_num_to_load(ifont);
		itext = get_new_irec_num_to_load(itext);
		geo->font_id = ifont;
		geo->text_id = itext;
	}
	geo->style.height /= get_grf_coeff();

	return OSTRUE;
}

#pragma argsused
OSCAN_COD  dos_simple_load  ( lpBUFFER_DAT bd, hOBJ hobj)
{
	WORD			size;
	lpOBJ			obj;

	obj = (lpOBJ)hobj;
	size = geo_size[obj->type];
	if ( load_data(bd,size,(void*)obj->geo_data) != size ) {
		return OSFALSE;
	}
	return OSTRUE;
}
OSCAN_COD  dos_spline_load  ( lpBUFFER_DAT bd, hOBJ hobj)
{
	lpOBJ			obj;
	lpD_POINT p;
	void*     d;
	WORD		  len;
	lpGEO_SPLINE	g;
	BOOL			small1 = FALSE;

	obj = (lpOBJ)hobj;
	g = (lpGEO_SPLINE)obj->geo_data;
	if ( ver < VERSION_1_12 ) {
		if ( load_data(bd,sizeof(g->type),&g->type) != sizeof(g->type) ) goto err;
		if ( load_data(bd,4,NULL) != 4)  goto err; //  !!!!!
		if ( load_data(bd,sizeof(g->nump),&g->nump) != sizeof(g->nump) ) goto err;
		if ( load_data(bd,sizeof(g->numd),&g->numd) != sizeof(g->numd) ) goto err;
	} else {
		if ( load_data(bd,sizeof(g->type),&g->type) != sizeof(g->type) ) goto err;
		if ( load_data(bd,sizeof(g->nump),&g->nump) != sizeof(g->nump) ) goto err;
		if ( load_data(bd,sizeof(g->numd),&g->numd) != sizeof(g->numd) ) goto err;
		if ( load_data(bd,sizeof(g->degree),&g->degree) != sizeof(g->degree) )
				goto err;
	}
	g->numu=0;
	g->hvector=0;

	if (g->nump == 2) {
		//   :   
		small1 = TRUE;
		g->nump = 3;
	}
	len = g->nump * sizeof(D_POINT);
	if ((g->hpoint = SGMalloc(len)) == NULL) goto errs;
	p = (lpD_POINT)g->hpoint;
	if (small1) len -= sizeof(D_POINT);
	if (load_data(bd,len,p) != len) goto errs;
	if (small1) {
		p[2] = p[1];
		dpoint_parametr(&p[0], &p[2], 0.5, &p[1]);
	}

	if (ver < VERSION_1_12 ) {
		len = g->numd * sizeof(SNODE);
		if (load_data(bd,len,NULL) != len) goto errs;
		g->hderivates = 0;
	} else {
		if (g->numd > 0) {
			if (g->type & SPLY_CUB) {	//   DOS-
				len = g->numd * sizeof(SNODE);
				if (load_data(bd,len,NULL) != len) goto errs;
				g->numd = 0;
				g->degree = 2;
				g->type &= ~SPLY_CUB;
				g->type |= SPLY_NURBS;
				g->hderivates = 0;
			} else {
				len = g->numd * ((g->type & SPLY_APPR) ? sizeof(SNODE) : sizeof(SNODE));
				if ((g->hderivates = SGMalloc(len)) == NULL) goto errs;
				d = (void*)g->hderivates;
				if (load_data(bd,len,d) != len) goto errs;
			}
		} else g->hderivates = 0;
	}
	if (g->nump < g->degree + 2) {
		//       "+2" 
		g->degree = g->nump - 2;
		if (g->degree < 1) return OSFALSE; //   
	}
	return OSTRUE;
err:
	return OSFALSE;
errs:
	if ( g->hpoint ) 			SGFree(g->hpoint);
	if ( g->hderivates ) 	SGFree(g->hderivates);
	goto err;
}
OSCAN_COD  dos_frame_load  ( lpBUFFER_DAT bd, hOBJ hobj)
{
	lpOBJ			obj;
	lpGEO_FRAME g;
	GEO_OBJ			geo;
	OBJTYPE			type;
	short				i,color;


	obj = (lpOBJ)hobj;
	g		= (lpGEO_FRAME)obj->geo_data;

	if( load_data(bd,sizeof(g->matr),&g->matr) != sizeof(g->matr))
			goto err;
	if( load_data(bd,sizeof(g->min),&g->min) != sizeof(g->min) )
			goto err;
	if( load_data(bd,sizeof(g->max),&g->max) != sizeof(g->max) )
			goto err;
	if( load_data(bd,22,NULL) != 22 ) 
			goto err;

  init_vld(&g->vld);
  if ( load_data(bd,sizeof(g->num),&g->num)!= sizeof(g->num))  goto errf;
  for ( i=0; i< g->num; i++) {
  	type = 0;
    if ( load_data(bd,sizeof(short),&type)   != sizeof(short))   goto errf;
		if ( load_data(bd,sizeof(color),&color) !=sizeof(color))   goto errf;
		if ( load_data(bd,geo_size[type],&geo)  != geo_size[type]) goto errf;
		if ( !add_vld_data(&g->vld, sizeof(type),&type))           goto errf;
		if ( !add_vld_data(&g->vld, sizeof(color),&color)) 				 goto errf;
		// Zframe-------------------------------
		{UCHAR a = 0;
		if ( !add_vld_data(&g->vld, sizeof(a),&a)) 				 goto errf;
		if ( !add_vld_data(&g->vld, sizeof(a),&a)) 				 goto errf;
		}
		// -------------------------------------
		if ( !add_vld_data(&g->vld, geo_size[type],&geo))          goto errf;
	}
	return OSTRUE;
err:
	return OSFALSE;
errf:
	free_vld_data(&g->vld);
	goto err;
}
OSCAN_COD  dos_path_load   ( lpBUFFER_DAT bd, hOBJ hobj )
{
	lpOBJ					obj;
	lpGEO_PATH		p;
	hOBJ					hobjn;
	register short	i;
	short 					num_obj;
	BOOL					cod;
	LISTH					listh;

	obj = (lpOBJ)hobj;
	p = (lpGEO_PATH)obj->geo_data;
	if( load_data(bd,sizeof(p->matr),&p->matr) != sizeof(p->matr))
			goto err;
	if( load_data(bd,sizeof(p->min),&p->min) != sizeof(p->min) )
			goto err;
	if( load_data(bd,sizeof(p->max),&p->max) != sizeof(p->max) )
			goto err;
  if ( ver >= VERSION_1_12 ) { //    
		if( load_data(bd,sizeof(p->kind),&p->kind) != sizeof(p->kind) )
				goto err;
	}
	init_listh(&listh);
	if( load_data(bd,sizeof(num_obj),&num_obj) != sizeof(num_obj)) return OSFALSE;
	for (i=0; i<num_obj; i++) {
		hhold = hobj;
		cod = o_load_obj_dos(bd,&hobjn);
		hhold = NULL;
		if ( !cod ) return OSFALSE;
		attach_item_tail(&listh,hobjn);
	}
	obj = (lpOBJ)hobj;
	p = (lpGEO_PATH)obj->geo_data;
	p->listh = listh;
	return OSSKIP;
err:
//nb err1:
 /// free_obj_list(&listh);   //   
	return OSFALSE;
}
OSCAN_COD  dos_brep_load   ( lpBUFFER_DAT bd, hOBJ hobj )
{
	lpOBJ					obj;
	lpGEO_BREP		p;
	int				i;
	lpCSG					csg;
	VLD						vld;
	lpLNP					lplnp;
	LNP						lnp;

	memset(&lnp,0,sizeof(lnp));
	lnp.kind = COMMON;
	obj = (lpOBJ)hobj;
	p = (lpGEO_BREP)obj->geo_data;
	if( load_data(bd,sizeof(p->matr),&p->matr) != sizeof(p->matr))
			goto err;
	if( load_data(bd,sizeof(p->type),&p->type) != sizeof(p->type))
			goto err;
	if ( ver < VERSION_1_8 )
		if( load_data(bd,sizeof(lnp.kind),&lnp.kind) != sizeof(lnp.kind))  goto err;
	if( load_data(bd,sizeof(p->ident_np),&p->ident_np) != sizeof(p->ident_np) )
			goto err;
	if( load_data(bd,sizeof(p->min),&p->min) != sizeof(p->min) )
			goto err;
	if( load_data(bd,sizeof(p->max),&p->max) != sizeof(p->max) )
			goto err;
	if ( lnp.kind == COMMON || !flg_list) {
		if ( ver <= VERSION_1_6 ) {
			if( load_data(bd,sizeof(lnp.num_np),&lnp.num_np) != sizeof(lnp.num_np) )
					goto err;
			init_vld(&vld);
			for (i=0; i<lnp.num_np; i++) {
				if ( !load_np_dos(bd) ) goto err1;
				if ( !add_np_mem(&vld,npwg,(NPTYPE)npwg->type) ) goto err1;
			}
			lnp.listh = vld.listh;
			if ( !put_np_brep( &lnp, &p->num) ) goto err1;
//      if ( !put_np_brep(&p->npd, &vld.listh, num_np, kind, NULL) ) goto err1;
		} else {
			if( load_data(bd,sizeof(p->num),&p->num) != sizeof(p->num) )  goto err;
			if ( ver <= VERSION_1_9 ) {
				if( load_data(bd,sizeof(lnp.num_np),&lnp.num_np) != sizeof(lnp.num_np))
						goto err;  //  -   ..     LNP
//				if( load_data(bd,sizeof(p->npd),&p->npd) != sizeof(p->npd) )  goto err;
			}
			if ( flg_list )  { //   
				p->num += num_brep_prev;
			}
			if ((lplnp = (LNP*)get_elem(&vd_brep, p->num)) == NULL) return OSFALSE;
			lplnp->count++;
		}
	} else {
		lnp.hcsg = obj->hcsg;
		csg = (lpCSG)obj->hcsg;
		lnp.num_np = create_primitive_np((BREPKIND)lnp.kind,&lnp.listh,(sgFloat *) csg->data);
		if ( !lnp.num_np ) goto err;
		if ( !put_np_brep(&lnp,&p->num) ) goto err1;
		obj->hcsg = NULL;
	}
	return OSSKIP;
err:
	return OSFALSE;
err1:
	free_vld_data(&vld);
	goto err;
}
#pragma argsused
OSCAN_COD  dos_group_load ( lpBUFFER_DAT bd, hOBJ hobj )
{
	lpOBJ obj;
	lpGEO_GROUP p;
	hOBJ  hobjn;
	register short i;
	short num_obj;
	BOOL cod;
	LISTH listh;


	obj = (lpOBJ)hobj;
	p = (lpGEO_GROUP)obj->geo_data;
	if( load_data(bd,sizeof(p->matr),&p->matr) != sizeof(p->matr))
			goto err;
  if( load_data(bd,sizeof(p->min),&p->min) != sizeof(p->min) )
			goto err;
	if( load_data(bd,sizeof(p->max),&p->max) != sizeof(p->max) )
			goto err;
	init_listh(&listh);
	if( load_data(bd,sizeof(num_obj),&num_obj) != sizeof(num_obj)) return OSFALSE;
	for (i=0; i<num_obj; i++) {
		hhold = hobj;
		cod = o_load_obj_dos(bd,&hobjn);
		hhold = NULL;
		if ( !cod ) return OSFALSE;
		attach_item_tail(&listh,hobjn);
	}
	obj = (lpOBJ)hobj;
	p = (lpGEO_GROUP)obj->geo_data;
	p->listh = listh;
	return OSSKIP;
err:
//nb err1:
 /// free_obj_list(&listh);   //   
	return OSFALSE;
}
OSCAN_COD  dos_insert_load   ( lpBUFFER_DAT bd, hOBJ hobj )
{
	lpOBJ obj;
	lpGEO_INSERT p;
  lpIBLOCK     blk;
	int         num_blk;

	obj = (lpOBJ)hobj;
	p = (lpGEO_INSERT)obj->geo_data;
	if( load_data(bd,sizeof(p->matr),&p->matr) != sizeof(p->matr))  goto err;
	if( load_data(bd,sizeof(p->min),&p->min)   != sizeof(p->min) )  goto err;
	if( load_data(bd,sizeof(p->max),&p->max)   != sizeof(p->max) )  goto err;
	if( load_data(bd,sizeof(p->num),&p->num)   != sizeof(p->num) )  goto err;
	if ( flg_list )  { //   
		p->num += num_blk_prev;
	}
	num_blk = p->num;
	if ((blk = (IBLOCK*)get_elem(&vd_blocks, num_blk)) == NULL) return OSFALSE;
	blk->count++;
	return OSSKIP;
err:
	return OSFALSE;
}

static BOOL load_np_dos(lpBUFFER_DAT bd)
{
	WORD  size;
	lpNPW n = npwg;

	if ( load_data(bd,sizeof(n->type),&n->type) != sizeof(n->type)) return FALSE;
	if ( load_data(bd,sizeof(n->nov),&n->nov) != sizeof(n->nov)) return FALSE;
	if ( load_data(bd,sizeof(n->noe),&n->noe) != sizeof(n->noe)) return FALSE;
	if ( load_data(bd,sizeof(n->nof),&n->nof) != sizeof(n->nof)) return FALSE;
	if ( load_data(bd,sizeof(n->noc),&n->noc) != sizeof(n->noc)) return FALSE;

	if ( load_data(bd,sizeof(n->ident),&n->ident) != sizeof(n->ident)) return FALSE;
	if ( n->nov > MAXNOV || n->noe > MAXNOE ||
			 n->noc > MAXNOC || n->nof > MAXNOF) {
		handler_err(ERR_DATABASE_FILE);
		return FALSE;
	}

	size = sizeof(D_POINT) * n->nov;
  if ( load_data(bd,size,&n->v[1]) != size) return FALSE;

    size = sizeof(VERTINFO) * n->nov;
    if ( load_data(bd,size,&n->vertInfo[1]) != size) return FALSE;

	size = sizeof(EDGE_FRAME) * n->noe;
	if ( load_data(bd,size,&n->efr[1]) != size) return FALSE;

	if ( n->type == TNPF ) return TRUE;

	size = sizeof(EDGE_FACE) * n->noe;
	if ( load_data(bd,size,&n->efc[1]) != size ) return FALSE;

	size = sizeof(FACE) * n->nof;
	if ( load_data(bd,size,&n->f[1]) != size ) return FALSE;

	size = sizeof(CYCLE) * n->noc;
	if ( load_data(bd,size,&n->c[1]) != size ) return FALSE;

	size = sizeof(GRU) * n->noe;
	if ( load_data(bd,size,&n->gru[1]) != size ) return FALSE;

	return TRUE;
}

