#include "../../sg.h"

static VI_LOCATION viloc;
static int num_elem = 0;
static int cur_elem = 0;

void frame_read_begin(lpGEO_FRAME geo)
{
	viloc.hpage  = geo->vld.listh.hhead;
	viloc.offset = sizeof(RAW);
	num_elem     = geo->num;
	cur_elem = 0;
}
BOOL frame_read_next(OBJTYPE* type, short* color, UCHAR* ltype, UCHAR* lthick,
										 lpGEO_OBJ  geo)
{
	if ( cur_elem >= num_elem ) return FALSE;
	begin_read_vld(&viloc);
	read_vld_data(sizeof(*type),type);
	read_vld_data(sizeof(*color),color);
	read_vld_data(sizeof(*ltype),ltype);
	read_vld_data(sizeof(*lthick),lthick);
	if ( !check_type(frame_filter,*type )) {
		handler_err(ERR_NUM_FRAME);
		end_read_vld();
		return FALSE;
	}
	read_vld_data(geo_size[*type],geo);
	get_read_loc(&viloc);
	end_read_vld();
	cur_elem++;
	return TRUE;
}

BOOL explode_frame(hOBJ hobj, lpLISTH listh)
{
	hOBJ				hobjn, htail;
	lpOBJ				obj;
	GEO_OBJ     geo;
	OBJTYPE     type;
	short         /*nbcount   = 0,*/ color;
	MATR				matr;
	UCHAR       ltype;
	UCHAR       lthick;

	obj = (lpOBJ)hobj;
	if ( obj->type != OFRAME) return FALSE;
	frame_read_begin((lpGEO_FRAME)obj->geo_data);
	memcpy(matr,((lpGEO_FRAME)obj->geo_data)->matr, sizeof(matr));

	htail = listh->htail;
	while ( frame_read_next(&type, &color, &ltype, &lthick, &geo)) {
		if ( (hobjn = o_alloc(type)) == NULL ) goto err;
		obj = (lpOBJ)hobjn;
		obj->color = (BYTE)color;
		obj->ltype = ltype;
		obj->lthickness = lthick;
		memcpy(obj->geo_data,&geo, geo_size[type]);
 //nb		count++;
		if (!o_trans(hobjn, matr)) goto err;
		attach_item_tail(listh, hobjn);
	}
	return TRUE;
err:
	if ( htail )	get_next_item(htail,&hobjn);
	else					hobjn = listh->hhead;

	while ( hobjn ) {
		get_next_item(hobjn,&htail);
		o_free( hobjn, listh);
		hobjn = htail;
	}
	return FALSE;
}


