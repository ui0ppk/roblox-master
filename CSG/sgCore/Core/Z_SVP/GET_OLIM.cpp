#include "../sg.h"

void union_limits(lpD_POINT mn1, lpD_POINT mx1,
									lpD_POINT mn2, lpD_POINT mx2)
{
	mn2->x = min(mn1->x, mn2->x);
	mn2->y = min(mn1->y, mn2->y);
	mn2->z = min(mn1->z, mn2->z);
	mx2->x = max(mx1->x, mx2->x);
	mx2->y = max(mx1->y, mx2->y);
	mx2->z = max(mx1->z, mx2->z);
}

//---    
BOOL get_scene_limits(void)
{
	hOBJ    		hobj = objects.hhead;
	D_POINT 		min, max;
	BOOL    		ret;
	lpOBJ				obj;
	ODRAWSTATUS	st;

	scene_min.x = scene_min.y = scene_min.z =  GAB_NO_DEF;
	scene_max.x = scene_max.y = scene_max.z = -GAB_NO_DEF;
	while (hobj) {
		obj = (lpOBJ)hobj;
		st = obj->drawstatus;
		if (!(st & ST_HIDE)) {
			if (!(ret = get_object_limits(hobj,&min,&max))) break;
			union_limits(&min,&max,&scene_min,&scene_max);
		}
		get_next_item(hobj,&hobj);
	}
	if (scene_min.x == GAB_NO_DEF) {
		scene_min.x = scene_min.y = scene_min.z =  -100;
		scene_max.x = scene_max.y = scene_max.z =   100;
		ret = TRUE;
	}
	if (fabs(scene_min.x - scene_max.x) < eps_d) {
		scene_min.x -= 100 * eps_d;
		scene_max.x += 100 * eps_d;
	}
	if (fabs(scene_min.y - scene_max.y) < eps_d) {
		scene_min.y -= 100 * eps_d;
		scene_max.y += 100 * eps_d;
	}
	if (fabs(scene_min.z - scene_max.z) < eps_d) {
		scene_min.z -= 100 * eps_d;
		scene_max.z += 100 * eps_d;
	}
	define_eps(&scene_min, &scene_max);
	return ret;
}
//---	     
void modify_limits(lpD_POINT min, lpD_POINT max)
{
//	sgFloat dx, dy, dz;

	if (min->x != GAB_NO_DEF) {
		union_limits(min,max,&scene_min,&scene_max);
		define_eps(&scene_min, &scene_max);
		/*if (!limits_tog) {
			limits_min = scene_min;
			limits_max = scene_max;
			dx = (limits_max.x - limits_min.x);// * MARGIN_VPORT;
			dy = (limits_max.y - limits_min.y);// * MARGIN_VPORT;
			dz = (limits_max.z - limits_min.z);// * MARGIN_VPORT;
			limits_min.x -= dx; limits_max.x += dx;
			limits_min.y -= dy; limits_max.y += dy;
			limits_min.z -= dz; limits_max.z += dz;
		}*/
	}
}
