#include "../../sg.h"

static BOOL load_surf(lpBUFFER_DAT bd, lpVLD vld);

short  blk_level = 0;
BOOL   bOemModelFlag = FALSE;   


VERSION_MODEL ver_load = (VERSION_MODEL)(LAST_VERSION - 1);
APPL_NUMBER   appl_number_load  = APPL_SG;        //  
short		      appl_version_load = 0; 							//   
hOBJ          hhold_load        = NULL;
char          flg_list_load     = 0;              //   

static BOOL          the_are_extern_block = FALSE; //    
static int           num_brep_prev = 0;
static lpOBJTYPE     pTypesTable = NULL;
static lpVDIM vd_ident;



typedef struct {
	char					_flg_oem;
	VERSION_MODEL	_ver;
	char					_flg_list;
	APPL_NUMBER		_appl_number;
	short					_appl_version;
	lpVDIM				_vd_ident;
  lpOBJTYPE     _pTypesTable;
//	hOBJ					_hhold;
//	int					_num_brep_prev;
//	int						_num_blk_prev;
//	int						_blk_level;
} LOAD_DATA, * lpLOAD_DATA;

static  void save_const(lpLOAD_DATA data);
static  void restore_const(lpLOAD_DATA data);


static BOOL load_vd_brep(lpBUFFER_DAT bd);
static BOOL load_attr_info(lpBUFFER_DAT bd, lpVDIM vd);
static IDENT_V get_new_irec_num_to_load(IDENT_V irec);

static BOOL  load_blocks_v2(lpBUFFER_DAT bd, char* title_grad);

static OSCAN_COD load_pre_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);

static OSCAN_COD  (**load_type_g)(lpBUFFER_DAT bd, hOBJ hobj);
static BOOL load_attr_data_len32(lpBUFFER_DAT bd, ULONG *len);
static BOOL create_objtypes_map(lpBUFFER_DAT bd);


void appl_init(void)
{
	appl_number_load  = o_appl_number;
	appl_version_load = o_appl_version;
}

char MODEL_VERSIOND[] = "O_MODEL V1";
char MODEL_VERSION[] =	"WINDOWS V1";

VERSION_MODEL get_version_model(lpBUFFER_DAT bd)
{
VERSION_MODEL   VerMod;

  if(!LoadModelVersion(bd, &VerMod)){
	  handler_err(ERR_VERSION);
	  return (VERSION_MODEL)(-1);
  }
  return VerMod;
}

BOOL LoadModelVersion(lpBUFFER_DAT bd, VERSION_MODEL *VerMod)
{
char 		TmpTitle[sizeof(MODEL_VERSION)];
short   shortTmp;
UINT    uintLen;

	uintLen = strlen(MODEL_VERSION);
	if(load_data(bd, uintLen, TmpTitle) != uintLen)
    return FALSE;
	if(memcmp(TmpTitle, MODEL_VERSION, uintLen)){ //  WINDOWS  .   DOS ?
		if(memcmp(TmpTitle, MODEL_VERSIOND, uintLen))   //  DOS   
       return FALSE;
		if(load_data(bd, 2, &shortTmp) != 2)
      return FALSE;
		if(shortTmp < VERSION_1_5 || shortTmp >= LAST_VERSION)
      return FALSE;
    *VerMod = (VERSION_MODEL)shortTmp;
		return TRUE;
	}
  // WINDOWS - 
	if(load_data(bd, 2, TmpTitle) != 2) //  
    return FALSE; 
	TmpTitle[2] = 0;
	shortTmp = (short)atoi(TmpTitle) + LAST_DOS_VERSION; 
	if(shortTmp < LAST_DOS_VERSION || shortTmp >= LAST_VERSION)
    return FALSE;
  *VerMod = (VERSION_MODEL)shortTmp;
	return TRUE;
}


BOOL o_load_list(lpBUFFER_DAT bd,lpLISTH listh, NUM_LIST list_zudina,
								 short *num_obj, UCHAR Header_Version, void* pProp)
{
	int				  num,i;
//	short       tmp_short;
//	WORD        len;
	hOBJ				hobj;
	BOOL        cod = FALSE;
	VDIM        vd;
//	UCHAR				num_field;
	LOAD_DATA		data;
	MATR				matr;
//	sgFloat			tmp;

	save_const(&data);     //    load
	file_unit = sgw_unit;


	/*if ( (ver_load = get_version_model(bd)) == -1 ) goto err1;
	if ( ver_load <= LAST_DOS_VERSION ) {
		cod = o_load_list_dos( bd, listh, list_zudina, num_obj, ver_load, pProp);
		restore_const(&data);  //    load
		return cod;
	}
  //     16  32  
	if(ver_load >= WINDOWS_V408)
	  if(Header_Version <= VER_HEAD_1_2)
	    ver_load=(VERSION_MODEL)(ver_load+1);*/

	*num_obj = 0;
	flg_list_load = 1;  //   
	num_brep_prev = vd_brep.num_elem;
	init_vdim(&vd, sizeof(IDENT_V));

  if(/*ver_load >= WINDOWS_V503*/true){  // C     
    if(!create_objtypes_map(bd))
      goto err;
  }


	//if (load_data(bd,sizeof(num_field),&num_field) != sizeof(num_field))
	//		goto err;

	/*for (i=0; i<num_field; i++ ) { //   
		if ( load_data(bd,sizeof(len),&len) != sizeof(len))	goto err;
		if ( load_data(bd,len,NULL) != len)	goto err;
	}*/

	/*if (load_data(bd,sizeof(file_unit),&file_unit) != sizeof(file_unit))
			goto err;

	if ( file_unit != sgw_unit ) {
		tmp = file_unit / sgw_unit;
		o_hcunit(matr);
		o_scale(matr,tmp);
	}*/


	//if (load_data(bd,sizeof(tmp_short),&tmp_short) != sizeof(tmp_short))
	//		goto err;
   //appl_number_load = (APPL_NUMBER)tmp_short;
//	if (load_data(bd,sizeof(tmp_short),&tmp_short)!=sizeof(tmp_short))
//			goto err;
 //  appl_version_load = tmp_short;
 // if(pProp)
  //  ScenePropSetModelVersionsInfo((pSCENE_PROP)pProp,  ver_load, appl_number_load, appl_version_load);

	if ( !load_attr_info(bd, &vd) ) goto err;
	if ( !load_vd_brep(bd) ) goto err;
//  
	if ( !load_blocks_v2(bd,"Load (1/2)")) goto err;

//  
	if (load_data(bd,sizeof(num),&num) != sizeof(num))  goto err;
//	num_proc = start_grad("Load (2/2)", num);
	if (application_interface && application_interface->GetProgresser()) 
		application_interface->GetProgresser()->Progress(0);
	
	for (i=0; i< num; i++) {
//		step_grad (num_proc , i);
		if (application_interface && application_interface->GetProgresser()) 
			application_interface->GetProgresser()->Progress(100*i/num);
		if (!o_load_obj(bd,&hobj)) goto end;
		if ( file_unit != sgw_unit )
			if ( !o_trans(hobj,matr)) goto end;
		if ( hobj ) {
			attach_item_tail_z(list_zudina,listh,hobj);
			(*num_obj)++;
		}
	}
//   
//          !!!!!
	if ( the_are_extern_block ) {
		IBLOCK      blk;
		short			result;
		LISTH		listh_save;

		the_are_extern_block = FALSE;
		for ( i=num_blk_prev; i<vd_blocks.num_elem; i++ ) {
			if (!read_elem(&vd_blocks,i,&blk)) goto end;
			if ( !(blk.type & BLK_EXTERN ) ) continue;
			while (TRUE) {
				if (nb_access(blk.name,4)) {
					//       !
					assert(0);
					/*RA  switch (Tlx_GetNewFileLocation(blk.name)) {
						case D_OK:			//   
							continue;
						case D_REFUSAL:	// ""  ""
							continue;			// ------------------   !
						default:				//    
							goto end;
					}*/
				}
				break;
			}
			blk_level++;
			listh_save = *listh;
			result = SGLoadModel(blk.name, &blk.min, &blk.max, /*NULL,*/ NULL,
													 &blk.listh);
			*listh = listh_save;
			blk_level--;
			if (!result) continue; // goto end;
			if(!write_elem(&vd_blocks, i, &blk)) goto end;

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
  if(pTypesTable)
    SGFree(pTypesTable);
err1:
	restore_const(&data);  //    load
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
		if ( load_data(bd,sizeof(lnp.num_np),&lnp.num_np) != sizeof(lnp.num_np))
			goto err;
		init_vld(&vld);
		for (j=0; j<lnp.num_np; j++) {
			if ( !load_np(bd) ) goto err;
			if ( !add_np_mem(&vld,npwg,(NPTYPE)npwg->type) ) goto err;
		}
		lnp.listh = vld.listh;
	 if (ver_load < WINDOWS_V501) {	// 17/05/2001
		lnp.num_surf = 0;
		init_listh(&lnp.surf);
	 } else {
		if ( load_data(bd,sizeof(lnp.num_surf),&lnp.num_surf) != sizeof(lnp.num_surf))
		  goto err;
		init_vld(&vld);
		for (j=0; j<lnp.num_surf; j++) {
		  if ( !load_surf(bd, &vld) ) goto err;
		}
		lnp.surf = vld.listh;
	 }
		if ( !add_elem(&vd_brep,&lnp) ) goto err;
	}
	return TRUE;
err:
	free_np_brep_list_num(num_brep_prev);
	return FALSE;
}

static BOOL create_objtypes_map(lpBUFFER_DAT bd)
{
int    i, TypesCount;
UCHAR  l1;
char   TypeName[256];

	if(load_data(bd, sizeof(TypesCount), &TypesCount) != sizeof(TypesCount))
		return FALSE;
  if(TypesCount <= 0 || TypesCount > MAX_OBJECTS_TYPES)
		return FALSE;
  pTypesTable = (lpOBJTYPE)SGMalloc(TypesCount*sizeof(OBJTYPE));
  if(!pTypesTable)
    return FALSE;
//      
  for(i = 0; i < TypesCount; i++){
		if(load_data(bd, sizeof(l1), &l1) != sizeof(l1))
      return FALSE;
		if(l1 < 2)
      return FALSE;
		if(load_data(bd, l1, TypeName) != l1)
      return FALSE;
		if(TypeName[l1 - 1])
      return FALSE;
    pTypesTable[i] = FindObjTypeByName(TypeName);
  }
  return TRUE;
}

OBJTYPE DecodeObjType(OBJTYPE type)
{
  if(flg_list_load){
    if(ver_load >= WINDOWS_V503)  //    
      return pTypesTable[type];
    return FindObjTypeByOldType(type);
  }
  return type;
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
WORD          i, len16;
ULONG         len;
UCHAR         l1;
UCHAR         *text = NULL;
char          *value;
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
		if(load_data(bd, sizeof(attr.precision), &attr.precision) !=
			 sizeof(attr.precision)) return FALSE;
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
      if(!load_attr_data_len32(bd, &len)) return FALSE;
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
	if(load_data(bd, sizeof(len16), &len16) != sizeof(len16)) return FALSE;
	if(!len16) goto met_font;
	if((irec = (RECORD_ITEM*)SGMalloc(len16)) == NULL) {
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
  if(!load_attr_data_len32(bd, &len)) goto err1;
	if(!len) return TRUE;
	//  
	while(TRUE){
		if(!load_attr_data_len32(bd, &len)) goto err1;
		if(!len) break;
		if(load_data(bd, len, font_name) != len) goto err1;
		//   
		idft = set_font(font_name, &errcode);
		if(!idft) idft = ft_bd->def_font; //   -   
		if(!add_elem(vd_ident, &idft)) goto err1;
	}
	//  
	while(TRUE){
		if(!load_attr_data_len32(bd, &len)) goto err1;
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


static BOOL load_attr_data_len32(lpBUFFER_DAT bd, ULONG *len)
{
WORD len16;
  if(ver_load < WINDOWS_V502) {
	  if(load_data(bd, sizeof(len16), &len16) != sizeof(len16)) return FALSE;
			*len = len16;
	} else {
	 if(load_data(bd, sizeof(ULONG), len) != sizeof(ULONG)) return FALSE;
  }
  return TRUE;
}

static IDENT_V get_new_irec_num_to_load(IDENT_V irec)
{
IDENT_V id;
	if(!irec) return irec;
	if(!read_elem(vd_ident, irec - 1, &id)) attr_exit();
	return id;
}

static BOOL  load_blocks_v2(lpBUFFER_DAT bd, char* title_grad)
{
	int    i;
	int    numblk;
	BOOL    cod = FALSE;

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
		if ( !load_one_block(bd)) goto end;
	}
	cod = TRUE;
end:
//	end_grad  (num_proc , i);
//	if (progress_function) 
//		progress_function(false,true,100.0);
	return cod;
}

BOOL  load_one_block(lpBUFFER_DAT bd)
{
	hOBJ    hobj;
	IBLOCK 	blk;
	int    j,num, offset;
	UCHAR		len;
//	int			result;

	memset(&blk,0,sizeof(blk));
//	if ( load_data(bd,sizeof(blk.name),blk.name) != sizeof(blk.name)) return FALSE;
	if ( ver_load < WINDOWS_V102 ) {
		if ( load_data(bd,13,blk.name) != 13 ) return FALSE;
	} else {
		if ( load_data(bd,1,&len) != 1 ) return FALSE; // len -    256
		if ( load_data(bd,len,blk.name) != len ) return FALSE;
		if ( load_data(bd, sizeof(blk.type), &blk.type) != sizeof(blk.type) )
			return FALSE;
	}
	if ( blk_level ) blk.type |= BLK_HOLDER;
	test_to_oem(blk.name, sizeof(blk.name));
	if ( load_data(bd,sizeof(blk.count),&blk.count) != sizeof(blk.count))
		return FALSE;
	if ( load_data(bd,sizeof(blk.min),&blk.min) != sizeof(blk.min))   return FALSE;
	if ( load_data(bd,sizeof(blk.max),&blk.max) != sizeof(blk.max))   return FALSE;
	if ( load_data(bd,sizeof(blk.stat),&blk.stat) != sizeof(blk.stat))return FALSE;
	if ( load_data(bd,sizeof(num),&num) != sizeof(num))               return FALSE;
	if ( load_data(bd,sizeof(blk.irec),&blk.irec) != sizeof(blk.irec))return FALSE;
	blk.irec = get_new_irec_num_to_load(blk.irec);
	if(blk.irec){
		modify_record_count(blk.irec, 1);
		obj_attr_flag = 1;
	}
	if ( load_data(bd,sizeof(offset),&offset) != sizeof(offset))      return FALSE;
	if ( (blk.type & BLK_EXTERN ) ) {
		the_are_extern_block = TRUE;

/*
		while (TRUE) {
			if (nb_access(blk.name,4)) {
				//       !
				switch (Tlx_GetNewFileLocation(blk.name)) {
					case D_OK:			//   
						continue;
					case D_REFUSAL:	// ""  ""
						continue;			// ------------------   !
					default:				//    
						return FALSE;
				}
			}
			break;
		}
		blk_level++;
		result = SGLoadModel(blk.name, &blk.min, &blk.max, NULL, NULL,
												 &blk.listh);
		blk_level--;
		if (!result) return FALSE;
*/
	} else {  //     
		for ( j=0; j<num; j++ ) {
			if (!o_load_obj(bd,&hobj)) return FALSE;
			attach_item_tail(&blk.listh,hobj);
		}
	}
	if ( !add_elem(&vd_blocks,&blk) ) return FALSE;
	return TRUE;
}

BOOL o_load_obj(lpBUFFER_DAT bd,hOBJ * hobj)
{
	OBJTYPE			 type;
	BOOL				 cod = FALSE;
	OSCAN_COD		 oscod;
	SCAN_CONTROL sc;
	int				   offset;
  short		     tmp_short;

	*hobj = NULL;

	load_type_g = (OSCAN_COD(**)(lpBUFFER_DAT bd, hOBJ hobj))GetMethodArray(OMT_LOAD);

	if ( load_data(bd,sizeof(tmp_short),&tmp_short) != sizeof(tmp_short))  goto m;
  type = tmp_short;
	if ( load_data(bd,sizeof(offset),&offset) != sizeof(offset))  goto m;
	type = DecodeObjType(type);
	if(type < 0){
		if(!seek_buf(bd, offset))goto m;  //   
		return FALSE;
//			return TRUE;   ????
	}
	if ( (*hobj = o_alloc(type)) == NULL ) goto m;
	init_scan(&sc);
	sc.user_pre_scan	= load_pre_scan;
	sc.data		        = bd;
	oscod = o_scan(*hobj, &sc);
	if ( oscod == OSFALSE) {
		o_free(*hobj, NULL);
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

	obj=(lpOBJ)hobj;
	obj->hhold = hhold_load;
	if( load_data(bd,sizeof(obj->layer),&obj->layer) != sizeof(obj->layer))
			goto err;
	if( load_data(bd,sizeof(obj->color),&obj->color) != sizeof(obj->color))
			goto err;
	if(ver_load >= WINDOWS_V103){
		if( load_data(bd,sizeof(obj->ltype),&obj->ltype) != sizeof(obj->ltype))
				goto err;
		if( load_data(bd,sizeof(obj->lthickness),&obj->lthickness) !=
									sizeof(obj->lthickness))
				goto err;
	}
	if( load_data(bd,sizeof(obj->status),&obj->status) != sizeof(obj->status))
			goto err;
	if( load_data(bd,sizeof(obj->drawstatus),&obj->drawstatus)
			!= sizeof(obj->drawstatus))		goto err;
	if ( load_data(bd,sizeof(irec),&irec) != sizeof(irec)) return OSFALSE;//FALSE;
	if(flg_list_load) irec = get_new_irec_num_to_load(irec);
	if(irec){
		modify_record_count(irec, 1);
		obj_attr_flag = 1;
	}
	if ( !load_csg(bd,&hcsg) ) return OSFALSE;
	if (hcsg) {
    if (pExtReLoadCSG)
      pExtReLoadCSG(hobj, &hcsg);
	}
	obj=(lpOBJ)hobj;
	obj->hcsg = hcsg;
	obj->irec = irec;

	return load_type_g[lpsc->type](bd,hobj);
//	return OSTRUE;
err:
	return OSFALSE;
}

#pragma argsused
OSCAN_COD  text_load  ( lpBUFFER_DAT bd, hOBJ hobj)
{
WORD       size;
lpOBJ      obj;
IDENT_V    itext, ifont;
lpGEO_TEXT geo;

	obj = (lpOBJ)hobj;
	size = geo_size[obj->type];
	if ( load_data(bd,size,(void *)obj->geo_data) != size ) {
		return OSFALSE;
	}
	geo = (lpGEO_TEXT)(obj->geo_data);
	ifont = geo->font_id;
	itext = geo->text_id;
	if(flg_list_load){
		ifont = get_new_irec_num_to_load(ifont);
		itext = get_new_irec_num_to_load(itext);
		geo->font_id = ifont;
		geo->text_id = itext;
	}
	if(ver_load < WINDOWS_V103)
		geo->style.height /= get_grf_coeff();

	return OSTRUE;
}

#pragma argsused
OSCAN_COD  dim_load  ( lpBUFFER_DAT bd, hOBJ hobj)
{
WORD      size;
lpOBJ     obj;
IDENT_V   itext, ifont;

	obj = (lpOBJ)hobj;
	size = geo_size[obj->type];
	if ( load_data(bd,size,(void *)obj->geo_data) != size ) {
		return OSFALSE;
	}
	ifont = ((lpGEO_DIM)(obj->geo_data))->dtstyle.font;
	itext = ((lpGEO_DIM)(obj->geo_data))->dtstyle.text;
	if(flg_list_load){
		ifont = get_new_irec_num_to_load(ifont);
		itext = get_new_irec_num_to_load(itext);
		((lpGEO_DIM)(obj->geo_data))->dtstyle.font = ifont;
		((lpGEO_DIM)(obj->geo_data))->dtstyle.text = itext;
	}
	return OSTRUE;
}

#pragma argsused
OSCAN_COD  simple_load  ( lpBUFFER_DAT bd, hOBJ hobj)
{
	WORD			size;
	lpOBJ			obj;

	obj = (lpOBJ)hobj;
	size = geo_size[obj->type];
	if ( load_data(bd,size,(void *)obj->geo_data) != size ) {
		return OSFALSE;
	}
	return OSTRUE;
}
OSCAN_COD  spline_load  ( lpBUFFER_DAT bd, hOBJ hobj)
{
	lpOBJ			    obj;
	void          *p, *d, *u;
	WORD		 	    len;
	lpGEO_SPLINE	g;

	obj = (lpOBJ)hobj;
	g = (lpGEO_SPLINE)obj->geo_data;
	if ( load_data(bd,sizeof(g->type),&g->type) != sizeof(g->type) ) goto err;
	if ( load_data(bd,sizeof(g->nump),&g->nump) != sizeof(g->nump) ) goto err;
	if ( load_data(bd,sizeof(g->numd),&g->numd) != sizeof(g->numd) ) goto err;
	if ( ver_load >= WINDOWS_V405){
		if ( load_data(bd,sizeof(g->numu),&g->numu) != sizeof(g->numu) ) goto err;}
  else g->numu=0;
	if ( load_data(bd,sizeof(g->degree),&g->degree) != sizeof(g->degree) )
				goto err;

  len=g->nump * sizeof(DA_POINT);
	if ((g->hpoint = SGMalloc(len)) == NULL) goto errs;
	p = (void*)g->hpoint;
	if (load_data(bd,len,p) != len) goto errs;

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
			len = g->numd * sizeof(SNODE);
			if ((g->hderivates = SGMalloc(len)) == NULL) goto errs;
			d = (void*)g->hderivates;
			if (load_data(bd,len,d) != len) goto errs;
		}
	} else g->hderivates = 0;

	if ( ver_load >= WINDOWS_V405 && g->numu > 0){    //zzz
		len = g->numu * sizeof(sgFloat);
		if ((g->hvector = SGMalloc(len)) == NULL) goto errs;
		u = (void*)g->hvector;
		if (load_data(bd,len,u) != len) goto errs;
	} else g->hvector=0;

	return OSTRUE;
err:
	return OSFALSE;
errs:
	if ( g->hpoint ) 			SGFree(g->hpoint);
	if ( g->hderivates ) 	SGFree(g->hderivates);
	if ( ver_load >= WINDOWS_V405){
		if ( g->hvector ) 	SGFree(g->hvector);
	}
	goto err;

}
OSCAN_COD  frame_load  ( lpBUFFER_DAT bd, hOBJ hobj)
{
	lpOBJ			  obj;
	lpGEO_FRAME g;
	GEO_OBJ     geo;
	OBJTYPE			type;
	short       i,color;
	UCHAR       ltype = 0;
	UCHAR       ltick = 0;


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

//	len = geo_size[obj->type];
//  if ( load_data(bd,len,(void *)obj->geo_data) != len ) goto err;

	init_vld(&g->vld);
	if ( load_data(bd,sizeof(g->num),&g->num)!= sizeof(g->num))  goto errf;
	for ( i=0; i< g->num; i++) {
  	type = 0;  //    sizeof(type)
		if ( load_data(bd,sizeof(short),&type)   != sizeof(short))  goto errf;
		if ( load_data(bd,sizeof(color),&color) !=sizeof(color))   goto errf;
		// Zframe-------------------------------
		if( ver_load >= WINDOWS_V103){
			if ( load_data(bd,sizeof(ltype),&ltype) !=sizeof(ltype))   goto errf;
			if ( load_data(bd,sizeof(ltick),&ltick) !=sizeof(ltick))   goto errf;
		}
		// -------------------------------------
		if ( load_data(bd,geo_size[type],&geo)  != geo_size[type]) goto errf;
		if ( !add_vld_data(&g->vld, sizeof(type),&type))           goto errf;
		if ( !add_vld_data(&g->vld, sizeof(color),&color)) 				 goto errf;
		// Zframe-------------------------------
		if ( !add_vld_data(&g->vld, sizeof(ltype),&ltype)) 				 goto errf;
		if ( !add_vld_data(&g->vld, sizeof(ltick),&ltick)) 				 goto errf;
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
OSCAN_COD  path_load   ( lpBUFFER_DAT bd, hOBJ hobj )
{
	lpOBJ				obj;
	lpGEO_PATH	p;
	hOBJ				hobjn;
	short				i;
	short 			num_obj;
	BOOL				cod;
	LISTH				listh;

	obj = (lpOBJ)hobj;
	p = (lpGEO_PATH)obj->geo_data;
	if( load_data(bd,sizeof(p->matr),&p->matr) != sizeof(p->matr))
			goto err;
	if( load_data(bd,sizeof(p->min),&p->min) != sizeof(p->min) )
			goto err;
	if( load_data(bd,sizeof(p->max),&p->max) != sizeof(p->max) )
			goto err;
	if( load_data(bd,sizeof(p->kind),&p->kind) != sizeof(p->kind) )
				goto err;
	init_listh(&listh);
	if( load_data(bd,sizeof(num_obj),&num_obj) != sizeof(num_obj)) return OSFALSE;
	for (i=0; i<num_obj; i++) {
		hhold_load = hobj;
		cod = o_load_obj(bd,&hobjn);
		hhold_load = NULL;
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
OSCAN_COD  brep_load   ( lpBUFFER_DAT bd, hOBJ hobj )
{
	lpOBJ					obj;
	lpGEO_BREP		p;
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
	if( load_data(bd,sizeof(p->ident_np),&p->ident_np) != sizeof(p->ident_np) )
			goto err;
	if( load_data(bd,sizeof(p->min),&p->min) != sizeof(p->min) )
			goto err;
	if( load_data(bd,sizeof(p->max),&p->max) != sizeof(p->max) )
			goto err;
	if ( lnp.kind == COMMON || !flg_list_load) {
		if( load_data(bd,sizeof(p->num),&p->num) != sizeof(p->num) )  goto err;
		if ( flg_list_load )  { //   
			p->num += num_brep_prev;
		}
		if ((lplnp = (LNP*)get_elem(&vd_brep, p->num)) == NULL) return OSFALSE;
		lplnp->count++;
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
OSCAN_COD  group_load ( lpBUFFER_DAT bd, hOBJ hobj )
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
		hhold_load = hobj;
		cod = o_load_obj(bd,&hobjn);
		hhold_load = NULL;
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
OSCAN_COD  insert_load   ( lpBUFFER_DAT bd, hOBJ hobj )
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
	if ( flg_list_load )  { //   
		p->num += num_blk_prev;
	}
	num_blk = p->num;
	if ((blk = (IBLOCK*)get_elem(&vd_blocks, num_blk)) == NULL) return OSFALSE;
	blk->count++;
	return OSSKIP;
err:
	return OSFALSE;
}

BOOL load_np(lpBUFFER_DAT bd)
{
	WORD  size;
	lpNPW n = npwg;

	if ( load_data(bd,sizeof(n->type),&n->type) != sizeof(n->type)) return FALSE;
	if ( load_data(bd,sizeof(n->nov),&n->nov) != sizeof(n->nov)) return FALSE;
	if ( load_data(bd,sizeof(n->noe),&n->noe) != sizeof(n->noe)) return FALSE;
	if ( load_data(bd,sizeof(n->nof),&n->nof) != sizeof(n->nof)) return FALSE;
	if ( load_data(bd,sizeof(n->noc),&n->noc) != sizeof(n->noc)) return FALSE;

	if ( load_data(bd,sizeof(n->ident),&n->ident) != sizeof(n->ident)) return FALSE;
   if (ver_load >= WINDOWS_V501) {	// 31/05/2001
		if ( load_data(bd,sizeof(n->surf),&n->surf) != sizeof(n->surf)) return FALSE;
		if ( load_data(bd,sizeof(n->or_surf),&n->or_surf) != sizeof(n->or_surf)) return FALSE;
   }
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

void test_to_oem(char* str, short len){
	if(bOemModelFlag) z_OemToAnsi(str, str, len);
}

void test_attr_value_to_oem(ATTR_TYPE type, char *value, ULONG len)
{
ULONG        i;
lpBOOL_VALUE bv;

	if (bOemModelFlag){
		switch(type){
			case ATTR_TEXT:
				for (i = 0; i < len; i++){
					if(value[i] == ML_S_C) value[i] = ' ';
					if((UCHAR)value[i] == 255) value[i] = ML_S_C;
				}
			case ATTR_STR:
				z_OemToAnsi(value, value, len);
				break;
			case ATTR_BOOL:
				bv = (lpBOOL_VALUE)value;
				z_OemToAnsi(bv->tb, bv->tb, 0);
				break;
			default:
				break;
		}
	}
}
//****************************************************8
//    load_list
//****************************************************8

static  void save_const(lpLOAD_DATA data)
{
	data->_flg_oem 				= bOemModelFlag;
	data->_ver 						= ver_load;
	data->_flg_list				= flg_list_load;				  //   
	data->_appl_number		= appl_number_load;				//  
	data->_appl_version 	= appl_version_load;			//   
	data->_vd_ident				= vd_ident;
  data->_pTypesTable    = pTypesTable;

//	data->_num_brep_prev	= num_brep_prev;
//	data->_hhold					=	hhold_load;
//	data->_num_blk_prev   = num_blk_prev;
//	data->_blk_level      = blk_level;
}
static  void restore_const(lpLOAD_DATA data)
{
	bOemModelFlag		  = data->_flg_oem;
	ver_load				  = data->_ver;
	flg_list_load			= data->_flg_list;				//   
	appl_number_load  = data->_appl_number;			//  
	appl_version_load = data->_appl_version; 		//   
	vd_ident				  = data->_vd_ident;
  pTypesTable       = data->_pTypesTable;
//	num_brep_prev		= data->_num_brep_prev;
//	hhold_load			=	data->_hhold;
//	num_blk_prev    = data->_num_blk_prev;
//	blk_level       = data->_blk_level;
}

// 17.05.2001
static BOOL load_surf(lpBUFFER_DAT bd, lpVLD vld)
{
  int 		    size;
  VADR			  hBuf;
  void *   lpBuf;
  GEO_SURFACE	surf;
  BOOL			  ret;

	if ( load_data(bd, sizeof(surf.type), &surf.type) != sizeof(surf.type)) return FALSE;
  if (!add_vld_data(vld, sizeof(surf.type), &surf.type)) return FALSE;

	if ( load_data(bd, sizeof(surf.numpu), &surf.numpu) != sizeof(surf.numpu)) return FALSE;
  if (!add_vld_data(vld, sizeof(surf.numpu), &surf.numpu)) return FALSE;

	if ( load_data(bd, sizeof(surf.numpv), &surf.numpv) != sizeof(surf.numpv)) return FALSE;
  if (!add_vld_data(vld, sizeof(surf.numpv), &surf.numpv)) return FALSE;

	if ( load_data(bd, sizeof(surf.numd), &surf.numd) != sizeof(surf.numd)) return FALSE;
  if (!add_vld_data(vld, sizeof(surf.numd), &surf.numd)) return FALSE;

	if ( load_data(bd, sizeof(surf.num_curve), &surf.num_curve) != sizeof(surf.num_curve)) return FALSE;
  if (!add_vld_data(vld, sizeof(surf.num_curve), &surf.num_curve)) return FALSE;

	if ( load_data(bd, sizeof(surf.degreeu), &surf.degreeu) != sizeof(surf.degreeu)) return FALSE;
  if (!add_vld_data(vld, sizeof(surf.degreeu), &surf.degreeu)) return FALSE;

	if ( load_data(bd, sizeof(surf.degreev), &surf.degreev) != sizeof(surf.degreev)) return FALSE;
  if (!add_vld_data(vld, sizeof(surf.degreev), &surf.degreev)) return FALSE;

	if ( load_data(bd, sizeof(surf.orient), &surf.orient) != sizeof(surf.orient)) return FALSE;
  if (!add_vld_data(vld, sizeof(surf.orient), &surf.orient)) return FALSE;

  int sz1 = sizeof(D_POINT) * surf.numpu * surf.numpv;
  int sz2 = sizeof(MNODE) * surf.numd;

  size = max(sz1 , sz2 );
  hBuf = SGMalloc(size);
  if (!hBuf) return FALSE;

  if ((lpBuf = (void*)hBuf) == NULL) { SGFree(hBuf); return FALSE; }

  size = sizeof(D_POINT) * surf.numpu * surf.numpv;
	if ( load_data(bd, size, lpBuf) != size) goto err;
  if (!add_vld_data(vld, size, lpBuf)) goto err;

  size = sizeof(sgFloat) * (surf.numpu+surf.degreeu+1+surf.numpv+surf.degreev+1);
	if ( load_data(bd, size, lpBuf) != size) goto err;
  if (!add_vld_data(vld, size, lpBuf)) goto err;

  if (surf.numd > 0) {
	  size = sizeof(MNODE) * surf.numd;
		if ( load_data(bd, size, lpBuf) != size) goto err;
	  if (!add_vld_data(vld, size, lpBuf)) goto err;
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

