#include "../sg.h"

void free_all_sg_mem(void)
{
hOBJ 	hobj, hnext;

  if(pExtFreeAllMem)
    pExtFreeAllMem();


  free_extern_obj_data = NULL;
	hobj = objects.hhead;
	while (hobj) {
		get_next_item(hobj, &hnext);
		o_free(hobj,&objects);
		hobj = hnext;
	}
  
  SGFree(static_data);    static_data = NULL;

	free_np_mem_tmp();
	free_blocks_list();
	free_np_brep_list();
	free_attr_bd_mem();

	ft_free_mem();

  GeoObjectsFreeMem();

}
