#include "../sg.h"

hOBJ  create_simple_obj_loc(OBJTYPE type, void  *geo) 
{
//       .
	hOBJ				hobj;
	lpOBJ				obj;

	if((hobj = o_alloc(type)) == NULL)
		return NULL;
	obj = (lpOBJ)hobj;
	//set_obj_nongeo_par(obj);
	memcpy(obj->geo_data, geo, geo_size[type]);
	return hobj;
}

hOBJ  create_simple_obj(OBJTYPE type, void  *geo) {
//   , ..   
//  : type -  
//           geo  -    
//                    .
//     

//      NULL,  
//      

	return create_simple_obj_to_prot(NULL, type, geo);
}

hOBJ  create_simple_obj_to_prot(lpOBJ prot, OBJTYPE type, void  *geo) {
//   ,  

	hOBJ				hobj;
	lpOBJ				obj;

	if((hobj = o_alloc(type)) == NULL)
		return NULL;
	obj = (lpOBJ)hobj;
	if(prot)
		copy_obj_nongeo_par(prot, obj);
	else{
    set_obj_nongeo_par(obj);
	  fc_assign_curr_ident(hobj);
	}

	memcpy(obj->geo_data, geo, geo_size[type]);

	if(!prot)
    attach_and_regen(hobj);
	return hobj;
}

void set_obj_nongeo_par(lpOBJ obj){
	if(check_type(struct_filter, obj->type) && obj_attrib.flg_transp )
			obj->color  = CTRANSP;
	else
			obj->color  = obj_attrib.color;
	obj->layer  = obj_attrib.layer;
	obj->ltype  = obj_attrib.ltype;
	obj->lthickness  = obj_attrib.lthickness;
}

void copy_obj_nongeo_par(lpOBJ prot, lpOBJ obj){
	obj->layer = prot->layer;
	obj->color = prot->color;
	obj->drawstatus = prot->drawstatus;
	obj->status = prot->status;
	obj->ltype  = prot->ltype;
	obj->lthickness  = prot->lthickness;
}

OBJTYPE get_simple_obj_geo(hOBJ hobj, void  *geo) {
//      .
//           hobj -   
//  :
//           geo  -    
//                    .

lpOBJ		 obj;
OBJTYPE  type;

  obj = (lpOBJ)hobj;
	type = obj->type;
  memcpy(geo, obj->geo_data, geo_size[type]);
	return type;
}

void get_simple_obj(hOBJ hobj, lpOBJ nongeo, void  *geo) {
//     

	lpOBJ				obj;

  obj = (lpOBJ)hobj;
  memcpy(nongeo, obj, sizeof(OBJ));
  memcpy(geo, obj->geo_data, geo_size[obj->type]);
}
