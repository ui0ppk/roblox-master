#include "../../sg.h"

IDENT_V   attr_work_irec = 0;    //  

lpVDIM    vd_record;             //  
lpVLD     vld_record;            //     

lpI_LISTH record_ilisth;         //  
lpI_LISTH free_record_ilisth;    //    

WORD  max_record_len;       //      vld 
ULONG record_free_space;    //     vld 

UCHAR obj_attr_flag = 0;    // ,       


BOOL record_soft_union(lpATTR_RECORD rec1, lpRECORD_ITEM irec1,
											 lpATTR_RECORD rec2, lpRECORD_ITEM irec2,
											 lpATTR_RECORD rec3, lpRECORD_ITEM irec3);

BOOL record_subst(lpATTR_RECORD rec1, lpRECORD_ITEM irec1,
									lpATTR_RECORD rec2, lpRECORD_ITEM irec2,
									lpATTR_RECORD rec3, lpRECORD_ITEM irec3);

static BOOL add_record_not_inline_data(lpATTR_RECORD rec, lpRECORD_ITEM ri);
static BOOL record_sb_mus(void);
static BOOL irec_tmo(short oper, IDENT_V rec1, IDENT_V rec2, lpIDENT_V rec3);

//       
//---------------------------------------------

BOOL get_attr_default_value(IDENT_V id_attr, void *value){
//    id_attr  
//   FALSE -   
ATTR attr;
	read_attr(id_attr, &attr);
	if(!get_attr_curr_value(&attr, value)) return FALSE;
	return TRUE;
}

BOOL set_attr_default_value(IDENT_V id_attr, void *value)

{
ATTR       attr;
lpATTR     lpattr;
IDENT_V    item_id;
sgFloat     *num;
BOOL_VALUE bv;

	read_attr(id_attr, &attr);
	if(attr.type == ATTR_BOOL){
		num = (sgFloat*)(value);
		if (*num != 0) bv.b = 1;
		else           bv.b = 0;
		bv.tb[0] = 0;
	}
	if(attr.type == ATTR_TEXT || value == NULL) return FALSE;
	if(!add_attr_value(id_attr, value, &item_id)) return FALSE;
	lpattr = lock_attr(id_attr);
	lpattr->curr = item_id;
	unlock_attr();
	return TRUE;
}

BOOL set_hobj_attr_value(IDENT_V id_attr, hOBJ hobj, void *value)
 
{
	IDENT_V       irec;
	ATTR_RECORD   rec;
	RECORD_ITEM   rp;
	BOOL          cod;

	memset(&rec, 0, sizeof(ATTR_RECORD));
	rec.num = 1;
	rp.attr = id_attr;
	if(!add_attr_value(id_attr, value, &rp.item)) return FALSE;
	if(!(irec = add_record(&rec, &rp))) return FALSE;
	cod = add_attr_to_hobj(hobj, irec);
	del_record(irec);
	
	if (id_attr==id_Name)
	{
		if (application_interface && application_interface->GetSceneTreeControl())
		{
			lpOBJ lpO = (lpOBJ)hobj;
			if (lpO->handle_of_tree_node)
				application_interface->GetSceneTreeControl()->UpdateNode(lpO->handle_of_tree_node);
		}
	}
	return cod;
}

BOOL get_hobj_attr_value(IDENT_V id_attr, hOBJ hobj, void *value)
 
{
IDENT_V       id_item;
	id_item = get_hobj_attr_item_id(id_attr, hobj);
	if(!id_item)
		return FALSE;
	get_attr_item_value(id_item, value);
	return TRUE;
}

IDENT_V get_hobj_attr_item_id(IDENT_V id_attr, hOBJ hobj)
  
{
IDENT_V       id_item, irec;
lpRECORD_ITEM recitm;
ATTR_RECORD   rec;
short         i;

	irec = get_hobj_irec(hobj);
	if(!irec) return 0;
	recitm = alloc_and_load_record(irec, &rec);
	if(!recitm) return 0;
	id_item = 0;
	for(i = 0; i < rec.num; i++){
		if(recitm[i].attr == id_attr){
			id_item = recitm[i].item;
			break;
		}
	}
	SGFree(recitm);
	return id_item;
}


void copy_obj_attrib(hOBJ hobjin, hOBJ hobjout)
 
{
lpOBJ   obj;
IDENT_V id;
lpATTR_RECORD  rec;

	obj = (lpOBJ)hobjin;
	id = obj->irec;
	if(id){
		rec = lock_record(id);
		if(!rec->num){
		 unlock_record();
		 return;
		}
		rec->count++;
		unlock_record();
		obj = (lpOBJ)hobjout;
		obj->irec = id;
    SetModifyFlag(TRUE);
	}
}

IDENT_V get_block_irec(int bnum)

{
lpIBLOCK blk;
IDENT_V  irec;
lpATTR_RECORD  rec;

	if(0 ==(blk = (IBLOCK*)get_elem(&vd_blocks, bnum))) attr_exit();
	irec = blk->irec;
	if(irec){
		rec = lock_record(irec);
		if(!rec->num) irec = 0;
		unlock_record();
	}
	return irec;
}

IDENT_V get_hobj_irec(hOBJ hobj)

{
lpOBJ   obj;
IDENT_V irec;
lpATTR_RECORD  rec;

	obj = (lpOBJ)hobj;
	irec = obj->irec;
	if(irec){
		rec = lock_record(irec);
		if(!rec->num) irec = 0;
		unlock_record();
	}
	return irec;
}

BOOL add_attr_to_hobj(hOBJ hobj, IDENT_V rec_id)
  
{
lpOBJ   obj;
IDENT_V irecold, irecnew;

  SetModifyFlag(TRUE);
	obj_attr_flag = 1;
	obj = (lpOBJ)hobj;
	irecold = irecnew = obj->irec;
	if(!add_attr_to_objrec(rec_id, &irecnew)) return FALSE;
	if(irecnew != irecold){
		obj = (lpOBJ)hobj;
		obj->irec = irecnew;
	}
	return TRUE;
}

void change_hobj_record(hOBJ hobj, IDENT_V irecnew)

{
lpOBJ   obj;
IDENT_V irec;

	obj = (lpOBJ)hobj;
	irec = obj->irec;
	if(irec == irecnew) return;
	if(irec){
		modify_record_count(irec, -1);
		del_record(irec);
	}
	if(irecnew){
		modify_record_count(irecnew, 1);
		obj_attr_flag = 1;
	}
	obj = (lpOBJ)hobj;
	obj->irec = irecnew;
  SetModifyFlag(TRUE);
}

void change_block_record(int bnum, IDENT_V irecnew)
{
lpIBLOCK blk;
IDENT_V  irec;

	if(0 ==(blk = (IBLOCK*)get_elem(&vd_blocks, bnum))) attr_exit();
	irec = blk->irec;
	if(irec == irecnew) return;
	if(irec){
		modify_record_count(irec, -1);
		del_record(irec);
	}
	if(irecnew){
		modify_record_count(irecnew, 1);
		obj_attr_flag = 1;
	}
	if(0 ==(blk = (IBLOCK*)get_elem(&vd_blocks, bnum))) attr_exit();
	blk->irec = irecnew;
  SetModifyFlag(TRUE);
}

BOOL add_attr_to_block(int bnum, IDENT_V rec_id)
  
{
lpIBLOCK blk;
IDENT_V  irecold, irecnew;

  SetModifyFlag(TRUE);
	obj_attr_flag = 1;
	if(0 ==(blk = (IBLOCK*)get_elem(&vd_blocks, bnum))) attr_exit();
	irecold = irecnew = blk->irec;
	if(!add_attr_to_objrec(rec_id, &irecnew)) return FALSE;
	if(irecnew != irecold){
	if(0 ==(blk = (IBLOCK*)get_elem(&vd_blocks, bnum))) attr_exit();
		blk->irec = irecnew;
	}
	return TRUE;
}

BOOL add_attr_to_objrec(IDENT_V rec_id, lpIDENT_V objrec_id)
 
{
WORD len;
BOOL code = FALSE;
BOOL soft_flag = FALSE;
ATTR_RECORD rec;
ATTR_RECORD oldrec;
ATTR_RECORD newrec;
lpRECORD_ITEM irec;
lpRECORD_ITEM oldirec = NULL;
lpRECORD_ITEM newirec = NULL;


	if(rec_id < 0){ // 
		soft_flag = TRUE;
		rec_id = -rec_id;
	}
	//   rec_id
	if(NULL ==(irec = alloc_and_load_record(rec_id, &rec))) goto fr;

	if(*objrec_id){ //     
		//   rec_id
	if(NULL ==(oldirec = alloc_and_load_record(*objrec_id, &oldrec))) goto fr;
		//     
		if(0 ==(newirec = (RECORD_ITEM*)SGMalloc((rec.num + oldrec.num)*sizeof(RECORD_ITEM)))){
			attr_handler_err(AT_HEAP);
			goto fr;
		}
		if(soft_flag){
			if(!record_soft_union(&oldrec, oldirec, &rec, irec, &newrec, newirec)){
				code = TRUE;
				goto fr;
			}
		}
		else{
			if(!record_union(&oldrec, oldirec, &rec, irec, &newrec, newirec)){
				code = TRUE;
				goto fr;
			}
		}
		if(oldrec.count > 1){ //     
			oldrec.count--;
			write_record(*objrec_id, &oldrec);
			newrec.count = 1;
			*objrec_id = add_record(&newrec, newirec);
			if(*objrec_id) code = TRUE;
		}
		else{
			if(oldrec.num == newrec.num){ //   vld
				rezet_vld_data(&oldrec.vbd.loc, oldrec.vbd.len, newirec);
			}
			else{ //     vld
				len = (WORD)oldrec.vbd.len;
				oldrec.num = newrec.num;
				if(!add_record_not_inline_data(&oldrec, newirec)) goto fr;
				write_record(*objrec_id, &oldrec);
				record_free_space += len;
			}
			code = TRUE;
		}
	}
	else{ //      -    irec
		rec.count++;
		write_record(rec_id, &rec);
		*objrec_id = rec_id;
		code = TRUE;
	}
fr:
	if(irec)    SGFree(irec);
	if(oldirec) SGFree(oldirec);
	if(newirec) SGFree(newirec);
	return code;
}

void modify_record_count(IDENT_V irec, short inc)
//      irec
{
lpATTR_RECORD  rec;

	if(irec){
		rec = lock_record(irec);
		rec->count = rec->count + inc;
		unlock_record();
	}
}


lpATTR_RECORD lock_record(IDENT_V rec_id)
//    
{
lpATTR_RECORD rec;
	if(NULL ==(rec = (ATTR_RECORD*)get_elem(vd_record, rec_id - 1))) attr_exit();
	return rec;
}

void unlock_record(void)
//    
{
}

void read_record(IDENT_V rec_id, lpATTR_RECORD rec)
//    
{
	if(!read_elem(vd_record, rec_id - 1, rec)) attr_exit();
}

void write_record(IDENT_V rec_id, lpATTR_RECORD rec)
//    
{
	if(!write_elem(vd_record, rec_id - 1, rec)) attr_exit();
}

IDENT_V get_next_record(IDENT_V rec_id)
//     
{
IDENT_V next_id;
	if(!il_get_next_item(vd_record, rec_id, &next_id)) attr_exit();
	return next_id;
}

IDENT_V add_record(lpATTR_RECORD rec, lpRECORD_ITEM ri)

{
IDENT_V id;
	if(!add_record_not_inline_data(rec, ri)) return FALSE;
	if(!il_add_elem(vd_record, free_record_ilisth, record_ilisth, rec, 0, &id)){
		if(id) attr_exit();
		record_free_space += rec->vbd.len;
	}
	return id;
}
#pragma argsused
void del_record(IDENT_V rec_id)
//   rec_id   (    = 0)
{
/*
lpATTR_RECORD rec;
WORD          len, count;
	if(rec_id <= 0) return; // ,   
	rec = lock_record(rec_id);
	len   = (WORD)rec->vbd.len;
	count = rec->count;
	unlock_record();
	if(!count){
		record_free_space += len;
		if(!il_detach_item(vd_record, record_ilisth, rec_id)) attr_exit();
		if(!il_attach_item_tail(vd_record, free_record_ilisth, rec_id))attr_exit();
	}
*/
}

BOOL create_work_record(lpVDIM vd, lpIDENT_V rec_id)
  
{
WORD          i, num_attr;
lpIDENT_V     index;
ATTR_RECORD   rec;
lpRECORD_ITEM ri;
lpATTR        attr;

	*rec_id = 0;
	if(!vd->num_elem) return TRUE;
	num_attr = 0;
	begin_rw(vd, 0);
	for(i = 0; i < vd->num_elem; i++) {
	  if(NULL ==(index = (long*)get_next_elem(vd))) attr_exit();
		if(*index) num_attr++;
	}
	end_rw(vd);
	if(!num_attr) return TRUE;
	if(NULL ==(ri = (RECORD_ITEM*)SGMalloc(num_attr*sizeof(RECORD_ITEM)))){
		attr_handler_err(AT_HEAP);
		return FALSE;
	}

	memset(&rec, 0, sizeof(ATTR_RECORD));
	begin_rw(vd, 0);
	for(i = 0; i < vd->num_elem; i++) {
	if(NULL ==(index = (long*)get_next_elem(vd))) attr_exit();
		if(*index){
			attr = lock_attr(*index);
			ri[rec.num].attr = *index;
			ri[rec.num].item = attr->curr;
			rec.num++;
			unlock_attr();
		}
	}
	end_rw(vd);
	*rec_id = add_record(&rec, ri);
	SGFree(ri);
	return (*rec_id) ? TRUE : FALSE;
}

lpRECORD_ITEM alloc_and_load_record(IDENT_V rec_id, lpATTR_RECORD rec)

{
lpRECORD_ITEM ri;

	read_record(rec_id, rec);
	if(rec->num){
		if(NULL ==(ri = (RECORD_ITEM*)SGMalloc(rec->num*sizeof(RECORD_ITEM)))){
			attr_handler_err(AT_HEAP);
			return NULL;
		}
		if(!load_vld_data(&rec->vbd.loc, rec->vbd.len, ri)) attr_exit();
	}
	else {
		if(NULL ==(ri = (RECORD_ITEM*)SGMalloc(sizeof(RECORD_ITEM)))){
			attr_handler_err(AT_HEAP);
			return NULL;
		}
		ri->attr = ri->item = 0;
		rec->vbd.len = 0;
	}
	return ri;
}

void load_record(IDENT_V rec_id, lpATTR_RECORD rec, lpRECORD_ITEM ri)

{
	read_record(rec_id, rec);
	if(!rec->num) return;
	if(!load_vld_data(&rec->vbd.loc, rec->vbd.len, ri)) attr_exit();
}


WORD get_record_size(lpATTR_RECORD record){
//     
	return record->num*sizeof(RECORD_ITEM);
}

BOOL record_union(lpATTR_RECORD rec1, lpRECORD_ITEM irec1,
									lpATTR_RECORD rec2, lpRECORD_ITEM irec2,
									lpATTR_RECORD rec3, lpRECORD_ITEM irec3)
{
BOOL   code = FALSE;
short  i, j;

	//     
	for(i = 0; i < rec1->num; i++)
		irec3[i] = irec1[i];
	rec3->num   = rec1->num;
	rec3->count = 0;
	//    
	for(i = 0; i < rec2->num; i++){
		for(j = 0; j < rec1->num; j++){
			if(irec1[j].attr == irec2[i].attr){
				if(irec2[i].item != irec1[j].item){
					irec3[j].item = irec2[i].item;
					code = TRUE;
				}
				goto met;
			}
		}
		//     
		irec3[(rec3->num)++] = irec2[i];
		code = TRUE;
met:continue;
	}
	return code;
}


BOOL record_soft_union(lpATTR_RECORD rec1, lpRECORD_ITEM irec1,
											 lpATTR_RECORD rec2, lpRECORD_ITEM irec2,
											 lpATTR_RECORD rec3, lpRECORD_ITEM irec3)
{
BOOL code = FALSE;
short  i, j;

	//     
	for(i = 0; i < rec1->num; i++)
		irec3[i] = irec1[i];
	rec3->num   = rec1->num;
	rec3->count = 0;
	//    
	for(i = 0; i < rec2->num; i++){
		for(j = 0; j < rec1->num; j++){
			if(irec1[j].attr == irec2[i].attr){
				//       -  
				if(irec2[i].item != irec1[j].item &&
					 irec2[i].item != 0){
					irec3[j].item = irec2[i].item;
					code = TRUE;
				}
				goto met;
			}
		}
		//     
		irec3[(rec3->num)++] = irec2[i];
		code = TRUE;
met:continue;
	}
	return code;
}

BOOL record_subst(lpATTR_RECORD rec1, lpRECORD_ITEM irec1,
									lpATTR_RECORD rec2, lpRECORD_ITEM irec2,
									lpATTR_RECORD rec3, lpRECORD_ITEM irec3)
{
BOOL  code = FALSE;
short i, j;

	//     
	for(i = 0; i < rec1->num; i++)
		irec3[i] = irec1[i];
	rec3->num   = rec1->num;
	rec3->count = 0;
	//     
	for(i = 0; i < rec3->num; i++){
		for(j = 0; j < rec2->num; j++){
			if(irec3[i].attr == irec2[j].attr){
				//       -  
				if(irec2[j].item != irec3[i].item){
					 irec3[i].item = irec2[j].item;
					code = TRUE;
				}
			}
		}
	}
	return code;
}


BOOL record_inter(lpATTR_RECORD rec1, lpRECORD_ITEM irec1,
									lpATTR_RECORD rec2, lpRECORD_ITEM irec2,
									lpATTR_RECORD rec3, lpRECORD_ITEM irec3)
{
short i, j;

	rec3->num   = 0;
	rec3->count = 0;
	//      
	for(i = 0; i < rec2->num; i++){
		for(j = 0; j < rec1->num; j++){
			if(irec2[i].attr == irec1[j].attr){
				irec3[(rec3->num)++] = irec1[j];
				goto met;
			}
		}

met:continue;
	}
	if(rec1->num != rec3->num) return TRUE;
//    
	for(i = 0; i < rec1->num; i++)
			if(irec1[i].attr != irec3[i].attr) return TRUE;

	return FALSE;
}


BOOL record_sub(lpATTR_RECORD rec1, lpRECORD_ITEM irec1,
								lpATTR_RECORD rec2, lpRECORD_ITEM irec2,
								lpATTR_RECORD rec3, lpRECORD_ITEM irec3)
{
short i, j;
	rec3->num   = 0;
	rec3->count = 0;
	//       
	for(i = 0; i < rec1->num; i++){
		for(j = 0; j < rec2->num; j++){
			if(irec1[i].attr == irec2[j].attr) goto met;
		}
		//    
		irec3[(rec3->num)++] = irec1[i];
met:continue;
	}
	return (rec1->num != rec3->num) ? TRUE : FALSE;
}

BOOL irec_inter(IDENT_V rec1, IDENT_V rec2, lpIDENT_V rec3)
{
	return irec_tmo(1, rec1, rec2, rec3);
}

BOOL irec_sub(IDENT_V rec1, IDENT_V rec2, lpIDENT_V rec3)
{
	return irec_tmo(0, rec1, rec2, rec3);
}

BOOL irec_subst(IDENT_V rec1, IDENT_V rec2, lpIDENT_V rec3)
{
	return irec_tmo(2, rec1, rec2, rec3);
}


static BOOL irec_tmo(short oper, IDENT_V rec1, IDENT_V rec2, lpIDENT_V rec3)
{
BOOL code = FALSE, noteq;
ATTR_RECORD r1;
ATTR_RECORD r2;
ATTR_RECORD r3;
lpRECORD_ITEM irec1;
lpRECORD_ITEM irec2 = NULL;
lpRECORD_ITEM irec3 = NULL;
short         num;


	//   rec1
	if(0 ==(irec1 = alloc_and_load_record(rec1, &r1))) goto fr;
	if(0 ==(irec2 = alloc_and_load_record(rec2, &r2))) goto fr;
		//    
	num = (oper == 1) ? r2.num : r1.num;
	if(!num) num = 1;
	if(0 ==(irec3 = (RECORD_ITEM*)SGMalloc(num*sizeof(RECORD_ITEM)))){
			attr_handler_err(AT_HEAP);
			goto fr;
	}
	switch(oper){
		case 1:
			noteq = record_inter(&r1, irec1, &r2, irec2, &r3, irec3);
			break;
		case 0:
			noteq = record_sub(&r1, irec1, &r2, irec2, &r3, irec3);
			break;
		default:
			noteq = record_subst(&r1, irec1, &r2, irec2, &r3, irec3);
			break;
	}
	if(!noteq){
		*rec3 = rec1;
		code = TRUE;
		goto fr;
	}
	if(!r3.num){
		*rec3 = 0;
		code = TRUE;
	}
	else{
		*rec3 = add_record(&r3, irec3);
		if(*rec3) code = TRUE;
	}
fr:
	if(irec1)    SGFree(irec1);
	if(irec2)    SGFree(irec2);
	if(irec3)    SGFree(irec3);
	return code;
}

static BOOL add_record_not_inline_data(lpATTR_RECORD rec, lpRECORD_ITEM ri)
//     vld 
{
WORD len;

	if(vld_record->mem)
		if(((sgFloat)record_free_space)/(vld_record->mem) > .3 &&
			 vld_record->listh.num > 2)
			if(!record_sb_mus()) return FALSE;
	len = rec->num*sizeof(RECORD_ITEM);
	if(!len){
		rec->vbd.len = 0;
		return TRUE;
	}
	if(!add_not_inline_data(vld_record, &rec->vbd, len, ri)) return FALSE;

	if(len > max_record_len) max_record_len = len;
	return TRUE;
}

static BOOL record_sb_mus(void)
//    vld 
{
IDENT_V       id;
VLD           vld;
char          *buf;
ATTR_RECORD   rec;
VI_LOCATION   loc;

	init_vld(&vld);
	if(0 ==(buf = (char*)SGMalloc(max_record_len))) goto err;

//   VLD
	id = record_ilisth->head;
	while(id){
		if(!read_elem(vd_record, id - 1, &rec)) attr_exit();
		if(rec.num){
			if(!load_vld_data(&rec.vbd.loc, rec.vbd.len, buf)) attr_exit();
			if(!add_vld_data(&vld, rec.vbd.len, buf)) goto err;
		}
		if(!il_get_next_item(vd_record, id, &id)) attr_exit();
	}
	SGFree(buf);
	free_vld_data(vld_record);

//   
	loc.hpage		= vld.listh.hhead;
	loc.offset	= sizeof(RAW);
	id = record_ilisth->head;
	begin_read_vld(&loc);
	while(id){
		if(!read_elem(vd_record, id - 1, &rec)) attr_exit();
		if(rec.num){
			get_read_loc(&rec.vbd.loc);
			if(!write_elem(vd_record, id - 1, &rec)) attr_exit();
			read_vld_data(rec.vbd.len, NULL);
		}
		if(!il_get_next_item(vd_record, id, &id)) attr_exit();
	}
	end_read_vld();

	record_free_space = 0;
	*vld_record = vld;
	return TRUE;
err:
	record_free_space = 0;
	if(buf) SGFree(buf);
	free_vld_data(&vld);
	return FALSE;
}


BOOL delete_attr_in_all_records(lpRECORD_ITEM dri, WORD numattr)
//     dri   
{
IDENT_V       id;
ATTR_RECORD   rec;
lpRECORD_ITEM ri;
WORD          i, j, k, len;

	id = record_ilisth->head;
	while(id){
		if((ri = alloc_and_load_record(id, &rec)) == NULL) goto err;
		for(i = 0, j = 0; i < rec.num; i++){
			for(k = 0; k < numattr; k++){
				if(ri[i].attr == dri[k].attr) goto next;
			}
			ri[j] = ri[i];
			j++;
next:
			continue;
		}
		if(rec.num != j){ //  
			if(obj_attr_flag)
        SetModifyFlag(TRUE);
			len = (WORD)rec.vbd.len;
			if((rec.num = j) > 0){ //  - 
				if(!add_record_not_inline_data(&rec, ri)) goto err;
			}
			else
				rec.vbd.len = 0;
			record_free_space += len;
			write_record(id, &rec);
		}
		if(ri) SGFree(ri);
		id = get_next_record(id);
	}
	return TRUE;
err:
	if(ri) SGFree(ri);
	return FALSE;
}

BOOL replace_attr_item_in_all_records(IDENT_V item_id, IDENT_V newid)
//      
{
IDENT_V       id;
ATTR_RECORD   rec;
lpRECORD_ITEM ri;
WORD          i;

	id = record_ilisth->head;
	while(id){
		if((ri = alloc_and_load_record(id, &rec)) == NULL) goto err;
		for(i = 0; i < rec.num; i++){
			if(ri[i].item == item_id){
				ri[i].item = newid;
				rezet_vld_data(&rec.vbd.loc, rec.vbd.len, ri);
				if(obj_attr_flag)
          SetModifyFlag(TRUE);
				break;
			}
		}
		if(ri) SGFree(ri);
		id = get_next_record(id);
	}
	return TRUE;
err:
	if(ri) SGFree(ri);
	return FALSE;
}
