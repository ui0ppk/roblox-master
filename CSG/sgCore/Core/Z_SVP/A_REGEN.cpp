#include "../sg.h"
/*---
			    objects,
			  ,
			     
			
*/
BOOL no_attach_and_regen(hOBJ hobj)
{
	D_POINT 	min, max;

	if (!get_object_limits(hobj,&min,&max)) return FALSE;
	modify_limits(&min, &max);
	return TRUE;//vi_regen(hobj,get_regim_edge() | get_switch_display());
}
BOOL attach_and_regen(hOBJ hobj)
{
	if (objects.num==0)
	{
		scene_min.x = scene_min.y = scene_min.z =  GAB_NO_DEF;
		scene_max.x = scene_max.y = scene_max.z = -GAB_NO_DEF;
	}
	attach_item_tail(&objects,hobj);
	return no_attach_and_regen(hobj);
}

