#include "../../sg.h"

lpVDIM    vd_attr;              

lpI_LISTH attr_ilisth;           
lpI_LISTH free_attr_ilisth;       

UCHAR max_attr_name;      
UCHAR max_attr_text;      

void (*u_attr_exit)  (void) = NULL;


void alloc_attr_bd(void);


void alloc_attr_bd(void)
//         
// !!!    WinMain  InitsBeforeMain() 
{

  if(0 ==(vd_attr       = (VDIM*)SGMalloc(sizeof(VDIM)))) goto err;
	if(0 ==(vd_attr_item  = (VDIM*)SGMalloc(sizeof(VDIM)))) goto err;
	if(0 ==(vd_record     = (VDIM*)SGMalloc(sizeof(VDIM)))) goto err;
  if(0 ==(vld_attr_item = (VLD*)SGMalloc(sizeof(VLD))))  goto err;
  if(0 ==(vld_record    = (VLD*)SGMalloc(sizeof(VLD))))  goto err;

  if(0 ==(attr_ilisth           = (I_LISTH*)SGMalloc(sizeof(I_LISTH)))) goto err;
  if(0 ==(free_attr_ilisth      = (I_LISTH*)SGMalloc(sizeof(I_LISTH)))) goto err;
  if(0 ==(free_attr_item_ilisth = (I_LISTH*)SGMalloc(sizeof(I_LISTH)))) goto err;
  if(0 ==(record_ilisth         = (I_LISTH*)SGMalloc(sizeof(I_LISTH)))) goto err;
  if(0 ==(free_record_ilisth    = (I_LISTH*)SGMalloc(sizeof(I_LISTH)))) goto err;

  init_attr_bd();

	return;
err:
  FatalStartupError(1);
}

void init_attr_bd(void)
//    
{
	init_vdim(vd_attr,      sizeof(ATTR));
	init_vdim(vd_attr_item, sizeof(ATTR_ITEM));
	init_vdim(vd_record,    sizeof(ATTR_RECORD));
	init_vld(vld_attr_item);
	init_vld(vld_record);
	il_init_listh(attr_ilisth);
	il_init_listh(free_attr_ilisth);
	il_init_listh(free_attr_item_ilisth);
	il_init_listh(record_ilisth);
	il_init_listh(free_record_ilisth);

	attr_work_irec    = 0;
	max_attr_name     = 0;
	max_attr_text     = 0;
	max_record_len    = 0;
	max_attr_item_len = 0;
	record_free_space = 0;
	attr_item_free_space = 0;
}

void free_attr_bd(void)
//      
{
	free_vdim(vd_attr);
	free_vdim(vd_attr_item);
	free_vdim(vd_record);
	free_vld_data(vld_attr_item);
	free_vld_data(vld_record);
	il_init_listh(attr_ilisth);
	il_init_listh(free_attr_ilisth);
	il_init_listh(free_attr_item_ilisth);
	il_init_listh(record_ilisth);
	il_init_listh(free_record_ilisth);

	attr_work_irec    = 0;
	max_attr_name     = max_attr_text        = 0;
	max_record_len    = 0;
	max_attr_item_len = 0;
	record_free_space = attr_item_free_space = 0;
}

void free_attr_bd_mem(void)
//      
{

	free_attr_bd();

	SGFree(vd_attr);
	SGFree(vd_attr_item);
	SGFree(vd_record);
	SGFree(vld_attr_item);
	SGFree(vld_record);

	SGFree(attr_ilisth);
	SGFree(free_attr_ilisth);
	SGFree(free_attr_item_ilisth);
	SGFree(record_ilisth);
	SGFree(free_record_ilisth);
}
//      
//-----------------------------------------------------
void attr_exit(void){
	if(u_attr_exit)  u_attr_exit();
	exit(1);
}

void attr_handler_err(short cod,...)
{
char    *s = NULL;
va_list ap;

	if(cod == AT_ERR_FILE){
		va_start(ap, cod);
		s = va_arg(ap, char *);
		va_end(ap);
	}
//  user_attr_message(cod, s);
}

//      
//------------------------------------------------

IDENT_V find_attr_by_name(char *name)
//       .
//    0 -    
{
lpATTR        wattr;
IDENT_V       index;

	index = attr_ilisth->head;
	while(index){
	if(0 ==(wattr = (ATTR*)get_elem(vd_attr, index - 1))) attr_exit();
		if(!z_stricmp(name, wattr->name))
			break;
		if(!il_get_next_item(vd_attr, index, &index)) attr_exit();
	}
	return index;
}

BOOL  add_attr(lpATTR attr, lpIDENT_V index)
{
lpATTR   wattr;
IDENT_V  id;
UCHAR    len;


	il_init_listh(&attr->ilisth);

	if(!(id = find_attr_by_name(attr->name))){ //    - 
		if(!il_add_elem(vd_attr, free_attr_ilisth, attr_ilisth, attr, 0, &id)){
			if(id) attr_exit();
			return FALSE;
		}
		if((len = (UCHAR)strlen(attr->name)) > max_attr_name) max_attr_name = (BYTE)len;
		if((len = (UCHAR)strlen(attr->text)) > max_attr_text) max_attr_text = (BYTE)len;
    *index = id;
		return TRUE;
	}
	if(0 ==(wattr = (ATTR*)get_elem(vd_attr, id - 1))) attr_exit();
//	if((strcmp(attr->text, wattr->text)) || (attr->type != wattr->type))
	if(attr->type != wattr->type)
		id = -id; //   
	*index = id;
	return TRUE;
}

IDENT_V create_sys_attr(char *name, char *text,
												UCHAR type, UCHAR size, short prec,
												WORD addstatus, void *value)
{
ATTR       attr;
lpATTR     lpattr;
IDENT_V    attr_id, item_id;
sgFloat     *num;
BOOL_VALUE bv;
UCHAR      bcurr;
void*      v;

	memset(&attr, 0, sizeof(ATTR));
	strcpy(attr.name, name);
	strcpy(attr.text, text);
	attr.type = type;
	attr.size = size;
	attr.precision = prec;
	if(attr.type == ATTR_BOOL){
		attr.size = 0;
		attr.precision = 0;
		attr.status |= ATT_ENUM;
	}
	if(attr.type != ATTR_TEXT && (addstatus&ATT_ENUM)) attr.status |= ATT_ENUM;
	if(attr.type == ATTR_sgFloat && (addstatus&ATT_RGB)) attr.status |= ATT_RGB;
	if(attr.type == ATTR_STR && (addstatus&ATT_PATH)) attr.status |= ATT_PATH;
	if(addstatus&ATT_SYSTEM) attr.status |= ATT_SYSTEM;

	if(!add_attr(&attr, &attr_id)) return 0;
	if(attr_id < 0) return 0;
	v = value;
	if(attr.type == ATTR_BOOL){ //   
		bcurr = 2; attr.curr = 0;
		if(value != NULL){
			num = (sgFloat*)(value);
			if (*num != 0) bcurr = 1;
			else bcurr = 0;
		}
		bv.tb[0] = 0;
		bv.b = 0;
		if(!add_attr_value(attr_id, &bv, &item_id)) goto err;
		if(bcurr == bv.b) attr.curr = item_id;
		bv.b = 1;
		if(!add_attr_value(attr_id, &bv, &item_id)) goto err;
		if(bcurr == bv.b) attr.curr = item_id;
		if(attr.curr){
			lpattr = lock_attr(attr_id);
			lpattr->status |= ATT_DEFAULT;
			lpattr->curr = attr.curr;
			unlock_attr();
		}
	}
	else if(attr.type != ATTR_TEXT && value != NULL){
		if(!add_attr_value(attr_id, v, &item_id)) goto err;
		lpattr = lock_attr(attr_id);
		lpattr->status |= ATT_DEFAULT;
		lpattr->curr = item_id;
		unlock_attr();
	}
	return attr_id;
err:
	//   
	del_attr(attr_id);
	return 0;
}

void del_attr(IDENT_V attr_id)
{
ATTR      attr;
ATTR_ITEM attr_item;
IDENT_V   id, idnext;

	//     
	if(!read_elem(vd_attr, attr_id - 1, &attr)) attr_exit();
	//     
	id = attr.ilisth.head;
	while(id){
		if(!read_elem(vd_attr_item, id - 1, &attr_item)) attr_exit();
		free_attr_value(&attr_item);  //  
		if(!il_get_next_item(vd_attr_item, id, &idnext)) attr_exit();
		if(!il_detach_item(vd_attr_item, &attr.ilisth, id)) attr_exit();
		if(!il_attach_item_tail(vd_attr_item, free_attr_item_ilisth, id))
			attr_exit();
		id = idnext;
	}
	if(!il_detach_item(vd_attr, attr_ilisth, attr_id)) attr_exit();
	if(!il_attach_item_tail(vd_attr, free_attr_ilisth, attr_id)) attr_exit();
}


BOOL retype_attr(IDENT_V attr_id, UCHAR newtype)
{
ATTR       attr, attr1;
ATTR_ITEM  attr_item;
IDENT_V    id, idnext, id_true, id_false, idnew;
void       *value;
char       str[MAX_ATTR_STRING + 1];
sgFloat     num;
BOOL_VALUE bv;
short      errcode;

	read_attr(attr_id, &attr);
	if(newtype == ATTR_BOOL){
		attr1 = attr;
		il_init_listh(&attr1.ilisth);
		attr1.type = newtype;
		attr1.curr = 0;
		attr1.status |= ATT_ENUM;
		write_attr(attr_id, &attr1);
		bv.b = 0;
		bv.tb[0] = 0;
		if(!add_attr_value(attr_id, &bv, &id_false)) goto err;
		bv.b = 1;
		if(!add_attr_value(attr_id, &bv, &id_true)) goto err;
	}

	//     
	id = attr.ilisth.head;
	while(id){
		read_attr_item(id, &attr_item);
		idnew = 0;
		switch(newtype){
			case ATTR_STR:
			case ATTR_TEXT:
				if(!get_attr_item_string(attr_id, id, FALSE, str)) goto nodef;
				value = (void*)str;
				goto def_ok;
			case ATTR_sgFloat:
				if(attr.type == ATTR_BOOL){
					if(!get_attr_item_value(id, &bv)) goto nodef;
					num = bv.b;
				}
				else{
					if(!get_attr_item_string(attr_id, id, FALSE, str)) goto nodef;
					if(!z_text_to_sgFloat(str, &num, &errcode)) goto nodef;
				}
				value = (void*)(&num);
				goto def_ok;
			case ATTR_BOOL:
				if(attr.type != ATTR_sgFloat) goto nodef;
				if(!get_attr_item_value(id, &num)) goto nodef;
				idnew = ((num != 0.) ? id_true : id_false);
				goto nodef;
		}
def_ok:
		free_attr_value(&attr_item);
		attr_item.type = newtype;
		if(!place_attr_value(&attr_item, value)) goto nodef1;
		write_attr_item(id, &attr_item);
		id = get_next_attr_item(id);
		continue;
nodef:
		free_attr_value(&attr_item);
nodef1:
		idnext = get_next_attr_item(id);
		if(!il_detach_item(vd_attr_item, &attr.ilisth, id)) attr_exit();
		if(!il_attach_item_tail(vd_attr_item, free_attr_item_ilisth, id))
			attr_exit();
		//     
		replace_attr_item_in_all_records(id, idnew);
		if(attr.curr == id) attr.curr = idnew;
		id = idnext;
	}
	//  
	switch(newtype){
		case ATTR_STR:
			attr.size = 15;
			attr.precision  = 80;
			break;
		case ATTR_TEXT:
			attr.size = 30;
			attr.precision  = 80;
			attr.status &= (~(ATT_DEFAULT|ATT_ENUM));
			attr.curr = 0;
			break;
		case ATTR_sgFloat:
			attr.size = 10;
			attr.precision  = 3;
			break;
		case ATTR_BOOL:
			attr.size = 0;
			attr.precision  = 0;
			attr.status |= ATT_ENUM;
			attr.ilisth = attr1.ilisth;
			break;
	}
	if(!attr.curr) attr.status &= (~ATT_DEFAULT);
	attr.type = newtype;
	write_attr(attr_id, &attr);
	return TRUE;
err:
	write_attr(attr_id, &attr);
	return FALSE;
}

lpATTR lock_attr(IDENT_V attr_id)
//    
{
lpATTR    attr;
	if(0 ==(attr = (ATTR*)get_elem(vd_attr, attr_id - 1))) attr_exit();
	return attr;
}

void unlock_attr(void)
//    
{
}

void read_attr(IDENT_V attr_id, lpATTR attr)
//    
{
  if(!read_elem(vd_attr, attr_id - 1, attr)) attr_exit();
}

void write_attr(IDENT_V attr_id, lpATTR attr)
//    
{
  if(!write_elem(vd_attr, attr_id - 1, attr)) attr_exit();
}

IDENT_V get_next_attr(IDENT_V attr_id)
//     
{
IDENT_V next_id;
	if(!il_get_next_item(vd_attr, attr_id, &next_id)) attr_exit();
  return next_id;
}
