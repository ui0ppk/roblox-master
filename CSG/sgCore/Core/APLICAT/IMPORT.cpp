#include "../sg.h"

#define MAXNUMFRAME 32500

static short count_frame;
static short count_frame_hobj;
static  hOBJ creat_frame_obj(lpVLD vld, short count);

void unmark_obj(hOBJ hobj){
	
	lpOBJ obj;

	obj = (lpOBJ)hobj;
	obj->status &= (~(ST_SELECT|ST_BLINK)); 
}


int import(DEV_TYPE dev, char * name, lpLISTH listh, BOOL frame, lpMATR matr,bool  solids_checking)
{
	hOBJ       hobj;
	BUFFER_DAT bd;
	int			 count;
	VLD        vld;

	count_frame = 0;
	count_frame_hobj = 0;
	init_vld(&vld);
	if (dev != DEV_WMF)
		if ( !init_buf(&bd,name,BUF_OLD) ) return FALSE;

	switch ( dev )
	{
		case DEV_HP:
			bd.flg_mess_eof = TRUE; 
			count = ImportHp(&bd, listh, &vld, frame);
			break;
		case DEV_FRG:
			bd.flg_mess_eof = TRUE; 
			count = 0;//ImportFrg(&bd, listh, &vld, frame);
			break;
		case DEV_PIC:
			bd.flg_mess_eof = TRUE; 
			assert(0);
			count = 0;//RA  ImportPic(&bd, listh, &vld, frame);
			break;
		case DEV_GEMMA:
			frame = FALSE;
			bd.flg_mess_eof = TRUE;
			count = ImportMesh(&bd, listh);
			break;
		case DEV_DXF:
			frame = FALSE;
			count = ImportDxf(&bd, listh);
			break;
		case DEV_STL:
			frame = FALSE;
			bd.flg_mess_eof = TRUE; 
			count = ImportStl(&bd, listh, matr,solids_checking);
			break;
		case DEV_WMF:
			bd.flg_mess_eof = TRUE; 
		//	count = ImportWMF(name, listh, &vld, frame);
			break;
	}
	if (dev != DEV_WMF)
		close_buf(&bd);
	if ( frame && vld.listh.hhead) {  
		if ( (hobj = creat_frame_obj(&vld,count_frame)) == NULL) {
			free_vld_data(&vld);
			return count_frame_hobj;
		}
		attach_item_tail(listh,hobj);
		count = count_frame_hobj;
	}
	return count;
}
static  hOBJ creat_frame_obj(lpVLD vld, short count)
{
	hOBJ       hobj;
	lpOBJ			 obj;
	D_POINT	 	 min, max;

	if ((hobj = o_alloc(OFRAME)) == NULL) {
		free_vld_data(vld);
		return NULL;
	}
	obj = (lpOBJ)hobj;
	((lpGEO_FRAME)obj->geo_data)->num = count;
	((lpGEO_FRAME)obj->geo_data)->vld = *vld;
	o_hcunit(((lpGEO_FRAME)obj->geo_data)->matr);
	if (!get_object_limits(hobj, &min, &max) ) {
		o_free(hobj,NULL);
		return NULL;
	}
	obj = (lpOBJ)hobj;
	((lpGEO_FRAME)obj->geo_data)->min = min;
	((lpGEO_FRAME)obj->geo_data)->max = max;

	count_frame_hobj += count_frame;
	count_frame = 0;  		

	return hobj;
}
BOOL cr_add_obj(OBJTYPE type, short cur_color, lpGEO_SIMPLE geo,
								lpLISTH listh, lpVLD vld, BOOL frame)
{
	hOBJ hobj;
	lpOBJ obj;

	if ( frame ) {
		count_frame++;
		if ( count_frame >= MAXNUMFRAME ) {
			if ( (hobj = creat_frame_obj(vld, (short)(count_frame-1))) == NULL )
				return FALSE;
			attach_item_tail(listh,hobj);
//			free_vld_data(vld);   
			init_vld(vld);
		}

		if ( !add_vld_data(vld, sizeof(type),&type)) return FALSE;
		if ( !add_vld_data(vld, sizeof(cur_color),&cur_color)) return FALSE;
		{UCHAR a = 0;
		if ( !add_vld_data(vld, sizeof(a),&a)) return FALSE;
		if ( !add_vld_data(vld, sizeof(a),&a)) return FALSE;
		}
		if ( !add_vld_data(vld, geo_size[type],geo)) return FALSE;
		return TRUE;
	}
	if ( (hobj = o_alloc(type)) == NULL ) return FALSE;

	obj = (lpOBJ)hobj;
	obj->color = (BYTE)cur_color;
	memcpy(obj->geo_data,geo,geo_size[type]);
	attach_item_tail(listh,hobj);
	return TRUE;
}
BOOL get_nums_from_string(char *line, sgFloat *values, short *num)
{
short   i = 0;

	while(TRUE){
    if(i + 1 > *num) break;
    while(*line == SPACE || *line == TAB) line++;
    if(!(*line)) break;
    if((line = z_atof(line, &values[i++])) == NULL) return FALSE;
		if (static_data->RoundOn)
			values[i-1] = round(values[i-1], export_floating_precision);
    while(*line == SPACE || *line == TAB) line++;
    if(*line == COMMA) line++;
  }
  *num = i;
	return TRUE;
}
