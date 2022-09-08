#include "../sg.h"

void delete_obj(hOBJ hobj){
//    
//  
	detach_item_z(OBJ_LIST,&objects,hobj); //    
//	free_viobj_scan(hobj);                 //  
//  undo_to_delete(hobj);                  //   
  SetModifyFlag(TRUE);                   //  .  
}

void autodelete_obj(hOBJ hobj){
  //draw_obj(hobj,vport_bg);
	delete_obj(hobj);
}
