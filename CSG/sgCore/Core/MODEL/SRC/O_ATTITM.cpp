#include "../../sg.h"

lpVDIM    vd_attr_item;          //    
lpVLD     vld_attr_item;         //     

lpI_LISTH free_attr_item_ilisth; //    


ULONG  max_attr_item_len;    //      vld 
ULONG  attr_item_free_space; //     vld 



static BOOL attr_item_sb_mus(void);


ULONG attr_size_value[NUM_ATTR] = {
	MAX_ATTR_STRING,    //ATTR_STR
	sizeof(sgFloat),     //ATTR_sgFloat
	sizeof(BOOL_VALUE), //ATTR_BOOL
  2*MAXSHORT,         //ATTR_TEXT
};


static ULONG text_value_len(ATTR_TYPE type, void* value);
static ULONG str_value_len(ATTR_TYPE type, void* value);
static ULONG inline_value_len(ATTR_TYPE type, void* value);
static short str_compare(ATTR_TYPE type, lpATTR_ITEM attr_item, void* value);
static short bool_compare(ATTR_TYPE type, lpATTR_ITEM attr_item, void* value);
static short sgFloat_compare(ATTR_TYPE type, lpATTR_ITEM attr_item, void* value);
static ULONG attr_item_value(lpATTR_ITEM attr_item, void* value);

static short (*attr_item_compare[NUM_ATTR])(ATTR_TYPE type,
																					  lpATTR_ITEM attr_item,
																					  void* value) = {
		str_compare,           //ATTR_STR
		sgFloat_compare,        //ATTR_sgFloat
		bool_compare,          //ATTR_BOOL
		NULL,                  //ATTR_TEXT
};

static  ULONG (*attr_value_len[NUM_ATTR])(ATTR_TYPE type, void* value)={
		str_value_len,         //ATTR_STR
		inline_value_len,      //ATTR_sgFloat
		inline_value_len,      //ATTR_BOOL
		text_value_len,        //ATTR_TEXT
};

//       
//---------------------------------------------------------

lpATTR_ITEM lock_attr_item(IDENT_V item_id)
   
{
lpATTR_ITEM attr_item;
	if(0 ==(attr_item = (ATTR_ITEM*)get_elem(vd_attr_item, item_id - 1))) attr_exit();
	return attr_item;
}

void unlock_attr_item(void)
   
{
}

void read_attr_item(IDENT_V item_id, lpATTR_ITEM attr_item)
  
{
	if(!read_elem(vd_attr_item, item_id - 1, attr_item)) attr_exit();
}

void write_attr_item(IDENT_V item_id, lpATTR_ITEM attr_item)
//    
{
	if(!write_elem(vd_attr_item, item_id - 1, attr_item)) attr_exit();
}

IDENT_V get_next_attr_item(IDENT_V item_id)
    
{
IDENT_V next_id;
	if(!il_get_next_item(vd_attr_item, item_id, &next_id)) attr_exit();
	return next_id;
}


BOOL  add_attr_value(IDENT_V attr_id, void* value, lpIDENT_V item_id)

{
IDENT_V       id;
ATTR          attr;
ATTR_ITEM     attr_item;
short         eq;

	if(!read_elem(vd_attr, attr_id - 1, &attr)) attr_exit();
	id = 0;
	if(attr.status & ATT_ENUM){ // 
    id = attr.ilisth.head;
    while(id){ //   
			if(!read_elem(vd_attr_item, id - 1, &attr_item)) attr_exit();
      eq = attr_item_compare[attr.type]((ATTR_TYPE)attr.type, &attr_item, value);
      if(!eq){ // 
				*item_id = id;
        return TRUE;
      }
      if(eq > 0) break;
			if(!il_get_next_item(vd_attr_item, id, &id)) attr_exit();
    }
  }
	attr_item.attr   = attr_id;
	attr_item.type   = attr.type;
	if(!place_attr_value(&attr_item, value)) return FALSE;

	//      id
	if(!il_add_elem(vd_attr_item, free_attr_item_ilisth, &attr.ilisth,
									&attr_item, id, item_id)){
		if(*item_id) attr_exit();
		free_attr_value(&attr_item);
		return FALSE;
	}
	if ( attr.type == ATTR_STR && attr.precision < (short)strlen((char*)value))
		attr.precision = strlen((char*)value);
	if(!write_elem(vd_attr, attr_id - 1, &attr)) attr_exit();
	return TRUE;
}

short compare_attr_item(IDENT_V item1, IDENT_V item2)
   
{
ATTR_ITEM     attr_item;
char          value[MAX_ATTR_STRING + 1];

	if(item1 == item2) return 0;
	read_attr_item(item1, &attr_item);
	if(!attr_item_compare[attr_item.type]) return 1;
	get_attr_item_value(item2, value);
	return attr_item_compare[attr_item.type]((ATTR_TYPE)attr_item.type, &attr_item, value);
}

short compare_attr_item_and_new_value(IDENT_V item, void* value)
 
{
ATTR_ITEM     attr_item;

	if(!item) return 1;
	read_attr_item(item, &attr_item);
	if(!attr_item_compare[attr_item.type]) return 1;
	return attr_item_compare[attr_item.type]((ATTR_TYPE)attr_item.type, &attr_item, value);
}

IDENT_V attr_to_var(IDENT_V attr_id, IDENT_V item_id)
 
{
return 0;
}

void* alloc_and_get_attr_value(IDENT_V item_id, UCHAR *type, ULONG *len)
     
{
ATTR_ITEM  attr_item;
void*      value;

	read_attr_item(item_id, &attr_item);
	*len = (attr_item.v.status == A_NOT_INLINE) ?
				 attr_item.v.value.ni.len : attr_item.v.status;
	if(0 ==(value = SGMalloc(*len))){
		attr_handler_err(AT_HEAP);
		return NULL;
	}
	attr_item_value(&attr_item, value);
	*type = attr_item.type;
	return value;
}

ULONG get_attr_item_value(IDENT_V item_id, void* value)
{
ATTR_ITEM     attr_item;

	read_attr_item(item_id, &attr_item);
	return attr_item_value(&attr_item, value);
}

char* get_attr_item_string(IDENT_V attr_id, IDENT_V item_id, BOOL format,
													 char *string)
   
{
ATTR          attr;
ATTR_ITEM     attr_item;
sgFloat        num;
BOOL_VALUE    bv;
char          *value, *c;
ULONG         len;
UCHAR         type;

	read_attr(attr_id, &attr);
	read_attr_item(item_id, &attr_item);
	switch(attr.type){
		case ATTR_TEXT:
		if(NULL == (value = (char*)alloc_and_get_attr_value(item_id, &type, &len)))
				return NULL;
			if((UCHAR)(*value) == ML_S_C){//    
				 c = value + 1;
				 while(*c != ML_S_C){
					 strcpy(string, c);
					 if((len = strlen(string)) != 0) break;
					 c += (len + 1);
				 }
			}
			else
				strcpy(string, value);
			SGFree(value);
			return string;
		case ATTR_STR:
			attr_item_value(&attr_item, string);
			return string;
		case ATTR_sgFloat:
			attr_item_value(&attr_item, &num);
			if(attr.status&ATT_RGB)
				return z_COLORREF_to_text((COLORREF)num, string);
			return sgFloat_output1(num, attr.size, attr.precision,
														!format, !format, FALSE, string);
		case ATTR_BOOL:
			attr_item_value(&attr_item, &bv);
			strcpy(string, bv.tb);
			return string;
		default:
			string[0] = 0;
			return string;
	}
}

/*NB++*/
BOOL get_attr_item_sgFloat(IDENT_V attr_id, IDENT_V item_id, sgFloat *val)
//     
{
ATTR          attr;
ATTR_ITEM     attr_item;

	read_attr(attr_id, &attr);
	read_attr_item(item_id, &attr_item);
	switch(attr.type){
		case ATTR_TEXT:
		case ATTR_STR:
		case ATTR_BOOL:
		default:
			*val = 0.0;
			return FALSE;

		case ATTR_sgFloat:
			attr_item_value(&attr_item, val);
			return TRUE;
	}
}
/*NB--*/

BOOL create_dtt_by_attr(IDENT_V attr_id, IDENT_V item_id, lpDTED_TEXT dtt)
//     dtt   attr_id 
//  item_id
{
ATTR   attr;
char   *txt, *c;
ULONG  tlen;

	 if(!attr_id || !item_id) return FALSE;

	 read_attr(attr_id, &attr);

	 if(attr.type != ATTR_TEXT){
		 if((txt = (char*)SGMalloc(MAX_ATTR_STRING + 1)) == NULL){
			 attr_handler_err(AT_HEAP);
			 return FALSE;
		 }
		 if(!get_attr_item_string(attr_id, item_id, FALSE, txt)){
			 SGFree(txt);
			 return FALSE;
		 }
	 }
	 else{
		 if((txt = (char*)alloc_and_get_attr_value(item_id, &attr.type, &tlen)) == NULL)
			 return FALSE;
	 }

	 if(!ted_text_init(dtt, 0)){
		 SGFree(txt);
		 return FALSE;
	 }
	 c = txt;
	 if((UCHAR)(*c) != ML_S_C) {
		 if(!ted_text_add(dtt, c)) goto metedtt;
	 }
	 else{
		 c++;
		 while((UCHAR)(*c) != ML_S_C){
			 if(!ted_text_add(dtt, c)) goto metedtt;
			 c += (strlen(c) + 1);
		 }
	 }
metedtt:
	 SGFree(txt);
	 return TRUE;
}

BOOL create_attr_by_dtt(lpDTED_TEXT dtt, IDENT_V attr_id, lpIDENT_V item_id)
{
ULONG   numchar, realchar;
int     numstr, len;
char    *txt, *c;
char    str[MAX_ATTR_STRING + 2];
BOOL    tflag;
IDENT_V id;

	//     
	ted_get_text_size(dtt, &numchar, &numstr);
	if(!numstr){
		*item_id = 0;
		return TRUE;
	}
	numchar += numstr;
	if(numstr > 1) numchar += 2;
	if((txt = (char*)SGMalloc(numchar)) == NULL){
		attr_handler_err(AT_HEAP);
		return FALSE;
	}
	if(numstr == 1)
		ted_get_next_str(dtt, 0, txt);
	else{
		id = 0;
		c  = txt;
		*c = ML_S_C; c++;
		realchar = 0;
		do {
			id = ted_get_next_str(dtt, id, str);
			len = strlen(str);
			realchar += (len + 1);
			if(realchar + 1 > numchar) break;
			strcpy(c, str);
			c += (len + 1);
		} while(id > 0);
		*c = ML_S_C;
	}
	tflag = add_attr_value(attr_id, txt, item_id);
	SGFree(txt);
	return tflag;
}

ULONG get_attr_curr_value(lpATTR attr, void* value)
{
ATTR_ITEM     attr_item;

	if(!attr->curr) return 0;
	read_attr_item(attr->curr, &attr_item);
	return attr_item_value(&attr_item, value);
}

void free_attr_value(lpATTR_ITEM attr_item)
//        attr_item
{
	if(attr_item->v.status != A_NOT_INLINE) return;

	attr_item->v.status = 0;
	attr_item_free_space += attr_item->v.value.ni.len;
}


BOOL place_attr_value(lpATTR_ITEM attr_item, void* value)
//    attr_item  value
{
ULONG len;

	if(vld_attr_item->mem)
		if(((sgFloat)attr_item_free_space)/(vld_attr_item->mem) > .3 &&
			 vld_attr_item->listh.num > 2)
			if(!attr_item_sb_mus()) return FALSE;

	len = attr_value_len[attr_item->type]((ATTR_TYPE)attr_item->type, value);
	if(len > max_attr_item_len) max_attr_item_len = len;
	//  
	return add_data_to_avalue(vld_attr_item, &attr_item->v, len, value);
}

static  BOOL attr_item_sb_mus(void)
//    vld 
{
int           i;
VLD           vld;
char          *buf;
lpATTR_ITEM   attr_item;
VI_LOCATION   loc;

	init_vld(&vld);
	if(0 ==(buf = (char*)SGMalloc(max_attr_item_len))) goto err;

//   VLD
	begin_rw(vd_attr_item, 0);
	for(i = 0; i < vd_attr_item->num_elem; i++) {
	if(0 ==(attr_item = (ATTR_ITEM*)get_next_elem(vd_attr_item))) attr_exit();
		if(attr_item->v.status != A_NOT_INLINE) continue;
		if(!load_vld_data(&attr_item->v.value.ni.loc, attr_item->v.value.ni.len,
											buf)) attr_exit();
		if(!add_vld_data(&vld, attr_item->v.value.ni.len, buf)){
			end_rw(vd_attr_item);
			goto err;
		}
	}
	SGFree(buf);
	end_rw(vd_attr_item);
	free_vld_data(vld_attr_item);  //    

//   
	loc.hpage		= vld.listh.hhead;
	loc.offset	= sizeof(RAW);
	begin_rw(vd_attr_item, 0);
	begin_read_vld(&loc);
	for(i = 0; i < vd_attr_item->num_elem; i++) {
	if(0 ==(attr_item = (ATTR_ITEM*)get_next_elem(vd_attr_item))) attr_exit();
		if(attr_item->v.status != A_NOT_INLINE) continue;
		get_read_loc(&attr_item->v.value.ni.loc);
		read_vld_data(attr_item->v.value.ni.len, NULL);
	}
	end_read_vld();
	end_rw(vd_attr_item);

	attr_item_free_space = 0;
	*vld_attr_item = vld;
	return TRUE;
err:
	attr_item_free_space = 0;
	if(buf) SGFree(buf);
	free_vld_data(&vld);
	return FALSE;
}


#pragma argsused
static  ULONG str_value_len(ATTR_TYPE type, void* value){
  return strlen((char*)value) + 1;
}

#pragma argsused
static  ULONG text_value_len(ATTR_TYPE type, void* value){
char  *c;
ULONG len, lens;
	c = (char*)value;
	if((UCHAR)(*c) != ML_S_C) return (strlen(c) + 1);
	len = 1;
	c = c + len;
	while((UCHAR)(*c) != ML_S_C){
		lens = strlen(c) + 1;
		len += lens;
		c = c + lens;
	}
	return (len + 1);
}

#pragma argsused
static  ULONG inline_value_len(ATTR_TYPE type, void* value){
	return attr_size_value[type];
}

#pragma argsused
static short str_compare(ATTR_TYPE type, lpATTR_ITEM attr_item,
														void* value)
{
char buf[MAX_ATTR_STRING + 1];

	if(attr_item->v.status != A_NOT_INLINE){ //  
		return strcmp(attr_item->v.value.cv, (char*)value);
	}
	else{
		if(!load_vld_data(&attr_item->v.value.ni.loc, attr_item->v.value.ni.len,
											buf)) attr_exit();
		return strcmp(buf, (char*)value);
	}
}

#pragma argsused
static short bool_compare(ATTR_TYPE type, lpATTR_ITEM attr_item,
														 void* value){
lpBOOL_VALUE b1, b2;
	b1 = (lpBOOL_VALUE)attr_item->v.value.cv;
	b2 = (lpBOOL_VALUE)value;
	if(b1->b != 0) b1->b = 1;
	if(b2->b != 0) b2->b = 1;
	return b1->b - b2->b;
}

#pragma argsused
static short sgFloat_compare(ATTR_TYPE type, lpATTR_ITEM attr_item,
															 void* value){
sgFloat di, dv;
	di = attr_item->v.value.dv;
	dv = *((sgFloat*)value);
	if(di - dv < 0.) return -1;
	if(di - dv > 0.) return  1;
  return 0;
}

#pragma argsused
static  ULONG attr_item_value(lpATTR_ITEM attr_item, void* value)
{
ULONG len;

	if(!load_data_from_avalue(&attr_item->v, &len, value)) attr_exit();
	return len;
}

BOOL story_attr_string(lpBUFFER_DAT bd, char *str)
//     
{
UCHAR len;
 len = (UCHAR)(strlen(str) + 1);
 if(!story_data(bd, 1,   &len)) return FALSE;
 if(!story_data(bd, len, str)) return FALSE;
 return TRUE;
}

