#include "../sg.h"

void detach_all_group_obj(hOBJ hgroup, BOOL undoflag){
// Вн ве обек з гпп

lpOBJ       obj;
hOBJ        hobj, hnext;
lpGEO_GROUP geo;
LISTH       group;
OBJTYPE			type;
ODRAWSTATUS	dstatus;
OSCAN_COD		regen;

	obj = (lpOBJ)hgroup;
	type = obj->type;
	dstatus = obj->drawstatus & ST_TD_BOX;
	geo = (lpGEO_GROUP)obj->geo_data;
	memcpy(&group, &geo->listh, sizeof(LISTH));
	init_listh(&geo->listh);

	if (dstatus) {
		//draw_obj(hgroup, vport_bg);
		;//free_viobj(hgroup);
	}

	hobj = group.hhead;
	regen = OSTRUE;
	while (hobj) {
		get_next_item_z(OBJ_LIST, hobj, &hnext);
		obj = (lpOBJ)hobj;
		obj->hhold = NULL;
		if (type == OPATH) {
			obj->status &= (~ST_DIRECT);
			obj->status &= ~(ST_CONSTR1 | ST_CONSTR2);
		}
		attach_item_tail_z(OBJ_LIST, &objects, hobj);
		if (dstatus) {
			//if (regen == OSTRUE || regen == OSSKIP)
			//	regen = vi_regen(hobj, get_regim_edge() | get_switch_display());
		} else {
//			change_viobj_color(hobj, COBJ);
			;//draw_obj(hobj, CDEFAULT);
		}
		//if(undoflag)
		//	undo_to_one_remove(hobj);
		hobj = hnext;
	}
}

void attach_obj_to_group(hOBJ hgroup, hOBJ hobj){
// Позве дев п добавлен обека в гпп
lpOBJ obj;

	detach_item_z(OBJ_LIST, &objects, hobj);
	obj = (lpOBJ)hobj;
	obj->status &= (~(ST_SELECT|ST_BLINK));
	obj->hhold = hgroup;	      	// Слка на обек венего овн
}
