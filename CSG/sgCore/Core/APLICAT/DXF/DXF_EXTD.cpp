#include "../../sg.h"

BOOL dxf_extrude(OBJTYPE type, short color, lpGEO_SIMPLE geo, lpLISTH listh,
								 BOOL flag_h, lpD_POINT v, sgFloat height, BOOL closed)
{
	LISTH 		listh2;
	hOBJ			hobj, htail;
	lpOBJ			obj;
	OSCAN_COD	cod;
	BOOL			Bool = TRUE;
	MATR			matr;

	if ( !cr_add_obj(type, color, geo, listh, NULL, FALSE)) return FALSE;
	if (type == OPATH || type == OSPLINE) {
		htail = listh->htail;
		if (type == OPATH) {
			obj = (lpOBJ)htail;
			listh2 = ((lpGEO_PATH)obj->geo_data)->listh;
			hobj = listh2.hhead;
			while (hobj) {
				obj = (lpOBJ)hobj;
				obj->hhold = htail;
				get_next_item(hobj, &hobj);
			}
			if (!set_contacts_on_path(htail)) return FALSE;
		}
		obj = (lpOBJ)htail;
		if (is_path_on_one_line(htail))
		{
			obj->status |= ST_ON_LINE;
			obj->status &= ~ST_FLAT;
		}
		else
		{
			if (set_flat_on_path(htail, NULL))
				obj->status |= ST_FLAT;
			else      
				obj->status &= ~ST_FLAT;
		}
		//if (!set_flat_on_path(htail, NULL)) return FALSE;
		if ((cod = test_self_cross_path(htail,NULL)) == OSFALSE) return FALSE;
//cod = OSTRUE;
		if (cod == OSTRUE) {
			obj = (lpOBJ)htail;
			obj->status |= ST_SIMPLE;
		} else closed = FALSE;	
	}
	if (flag_h) {
		htail = listh->htail;
		init_listh(&listh2);
		detach_item(listh, htail);
		attach_item_head(&listh2, htail);
		dpoint_scal(v, height, v);
		o_hcunit(matr);
		o_tdtran(matr, v);
		Bool = extrud_mesh(&listh2, 0, matr, closed, &hobj);
		o_free(htail, NULL);
		if (!hobj) return FALSE;
		obj = (lpOBJ)hobj;
		obj->color = (BYTE)color;
		attach_item_tail(listh, hobj);
	}
	return Bool;
}
