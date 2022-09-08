#include "../sg.h"

BOOL b_vr_line(lpD_POINT vn,lpD_POINT vk)
{
	lpGEO_LINE  geo_line;
	hOBJ				hobj;
	lpOBJ				obj;

	if((hobj = o_alloc(OLINE)) == NULL)
		return FALSE;
	obj = (lpOBJ)hobj;
	geo_line = (lpGEO_LINE)(obj->geo_data);
	geo_line->v1 = *vn;
	geo_line->v2 = *vk;
	attach_item_tail(bo_line_listh, hobj);

	return TRUE;
}
