#include "../sg.h"

//---     
void off_blink_selected(void)
{
	hOBJ	hobj;

	hobj = selected.hhead;
	while (hobj) {
		off_blink_one_obj(hobj);
		get_next_item_z(SEL_LIST,hobj,&hobj);
	}
}
void off_blink_one_obj(hOBJ hobj)
{
lpOBJ	obj;

	obj = (lpOBJ)hobj;
	obj->status &= (~(ST_SELECT|ST_BLINK)); //   
//	change_viobj_color(hobj,COBJ);
	//draw_obj(hobj,CDEFAULT);
}
