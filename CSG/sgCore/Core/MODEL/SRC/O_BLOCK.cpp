#include "../../sg.h"

BOOL create_block(lpLISTH listh, char * name, lpMATR m,
									char type, int *number)
{
	hOBJ hobj, hobj_last;
	IBLOCK blk;
	lpIBLOCK lpblk;
	BOOL flag;
	REGION_3D g,g1;

	hobj = listh->hhead;

	np_gab_init(&g);
	memset(&blk,0,sizeof(blk));
	while ( hobj ) {
		o_trans(hobj,m);
		hobj_last = hobj;
		if ( !get_object_limits(hobj,&g1.min,&g1.max) ) goto err;
		np_gab_un(&g, &g1, &g);
		get_next_item(hobj,&hobj);
	}
	blk.listh = *listh;
	blk.min   = g.min;
	blk.max   = g.max;
	blk.type  = (BYTE)(type & BLK_EXTERN);
	strncpy(blk.name,name,MAXBLOCKNAME);
	blk.name[MAXBLOCKNAME - 1] = 0;

  if ( *number < 0 || *number >= vd_blocks.num_elem) {
    //      
    for(*number = 0; *number < vd_blocks.num_elem; (*number)++){
	  if(0 ==(lpblk=(IBLOCK*)get_elem(&vd_blocks, *number)) ) goto err;
      flag = (lpblk->stat == BS_DELETE);
      if(flag) break;
    }
    if (*number >= vd_blocks.num_elem){
      if ( !add_elem(&vd_blocks,&blk) ) goto err;
      *number = vd_blocks.num_elem - 1;
    }
    else
      if ( !write_elem(&vd_blocks,*number,&blk) ) goto err;

  }  else {
    if ( !write_elem(&vd_blocks,*number,&blk) ) goto err;
  }
  return TRUE;
err:
  {
    MATR  mi;
    if ( !o_minv(m,mi)) goto err1;
    hobj = listh->hhead;
    while ( hobj ) {
      o_trans(hobj,mi);
      get_next_item(hobj,&hobj);
      if ( hobj == hobj_last ) break;
    }
  }
err1:
  *number = -1;
  return FALSE;
}
static BOOL fcmp(void * elem, void * name){
	if ( !strncmp(((lpIBLOCK)elem)->name,(char*)name, MAXBLOCKNAME) ) {
		if  ( !((lpIBLOCK)elem)->stat ) return TRUE;
	}
	return FALSE;
}
BOOL find_block(char * name, int *num)
{
	if ( !find_elem(&vd_blocks,num,fcmp,name) )  return FALSE;
	return TRUE;
}
hOBJ create_insert_by_name(char * name, lpMATR m)
{
	int num = 0;

  if ( !find_block(name,&num) ) { handler_err(ERR_NUM_BLOCK); return NULL; }
	return create_insert_by_number(num,m);
}
hOBJ create_insert_by_number(int num, lpMATR m)
{
	hOBJ  hobj;
	lpOBJ obj;
	lpGEO_INSERT g;
	lpIBLOCK lpblk;
  IDENT_V  irec;

	if ( ( hobj = o_alloc(OINSERT)) == NULL ) return NULL;
	obj = (lpOBJ)hobj;
	g = (lpGEO_INSERT)obj->geo_data;
	memcpy(g->matr,m,sizeof(g->matr));
	g->num = num;
	if ( 0 ==(lpblk = (IBLOCK*)get_elem(&vd_blocks,num) )) goto err;
	g->min  = lpblk->min;
	g->max  = lpblk->max;
	lpblk->count++;
  irec = obj->irec = lpblk->irec;
  if(irec) modify_record_count(irec, 1);
  return hobj;
err:
  o_free(hobj,NULL);
  return NULL;
}
/*-----------------12/04/95 17:03-------------------
    
 --------------------------------------------------*/
void free_blocks_list(void)
{
  free_blocks_list_num(0L);
  free_vdim(&vd_blocks);
}
/*-----------------12/04/95 17:03-------------------
        num
 --------------------------------------------------*/
void free_blocks_list_num(int num)
{
	IBLOCK blk;
  int i;

  if ( num < 0 || num >= vd_blocks.num_elem ) return;
  for ( i=num; i<vd_blocks.num_elem; i++ ) {
		if (!read_elem(&vd_blocks,i,&blk)) return;
    if(blk.stat != BS_DELETE) free_block(&blk);
  }
  delete_elem_list(&vd_blocks, num);
}
/*-----------------25/10/95 ------------------------
     num
 --------------------------------------------------*/
void free_block_num(int num)
{
	IBLOCK blk;

  if ( num < 0 || num >= vd_blocks.num_elem ) return;
  if (!read_elem(&vd_blocks,num,&blk)) return;
  free_block(&blk);
}
/*-----------------16/10/95 18:40-------------------
  
 --------------------------------------------------*/
void free_block(lpIBLOCK blk)
{
	hOBJ hobj, hobjtmp;

  if(blk->stat == BS_DELETE) return;
  hobj = blk->listh.hhead;
  while ( hobj ) {
    get_next_item(hobj,&hobjtmp);
//    fc_id_mark(hobj, UC_FREE);   //     o_model.h
		if ( free_extern_obj_blk_data)
			free_extern_obj_blk_data(hobj);
    o_free(hobj,NULL);
    hobj = hobjtmp;
  }
  init_listh(&blk->listh);
}
BOOL set_block_status(int number, char stat)
{
  lpIBLOCK blk;

	if ( 0 ==(blk=(IBLOCK*)get_elem(&vd_blocks, number)) ) return FALSE;
  blk->stat = stat;
  return TRUE;
}

BOOL mark_block_delete(int number)
{
  lpIBLOCK blk;
  BOOL     flag;

	if ( 0 ==(blk=(IBLOCK*)get_elem(&vd_blocks, number)) ) return FALSE;
  blk->stat = BS_DELETE;
  //     
  if (number == vd_blocks.num_elem - 1) {
    while(number){
      number--;
	  if ( 0 ==(blk=(IBLOCK*)get_elem(&vd_blocks, number)) ) return FALSE;
      flag = (blk->stat == BS_DELETE);
      if ( flag ) continue;
      number++;
      break;
    }
    delete_elem_list(&vd_blocks, number);
  }
  return TRUE;
}

static OSCAN_COD create_pre_scan(hOBJ hobj, lpSCAN_CONTROL lpsc);
BOOL  create_count_blocks( lpLISTH listh, NUM_LIST list_zudina,
												 lpVDIM vdim, void * elem)
{
	register short i;
	SCAN_CONTROL  sc;
	hOBJ          hobj;
	IBLOCK        blk;


	if ( !vd_blocks.num_elem ) return TRUE;

	//   
	for (i=0; i<vd_blocks.num_elem; i++) {
		if (!read_elem(&vd_blocks,i,&blk)) goto err;
		if ( blk.stat ) *(short * )elem = -1;
		else            *(short * )elem = 0;
		if ( (blk.type & BLK_HOLDER) ) *(short * )elem = -1;
		if ( !add_elem(vdim,elem)) goto err;
	}

	//  -   
	init_scan(&sc);
	sc.user_pre_scan = create_pre_scan;
	sc.data          = vdim;
	hobj = listh->hhead;
	while (hobj) {
		if (!o_scan(hobj,&sc))  return FALSE;
		get_next_item_z(list_zudina,hobj,&hobj);
	}
	return TRUE;
err:
	free_vdim(vdim);
	return FALSE;
}
static OSCAN_COD create_pre_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	if ( lpsc->type != OINSERT ) return OSTRUE;
	{
		lpOBJ         obj;
		int          num;
		short *      count;

		obj = (lpOBJ)hobj;
		num = ((lpGEO_INSERT)(obj->geo_data))->num;
		if ( 0 ==(count=(short*)get_elem((lpVDIM)lpsc->data,num)) ) return OSFALSE;
		if (*count != -1) (*count)++;
	}
	return OSTRUE;
}
