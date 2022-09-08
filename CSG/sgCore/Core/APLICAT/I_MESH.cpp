#include "../sg.h"

static  hOBJ load_mesh(lpBUFFER_DAT bd);

short ImportMesh(lpBUFFER_DAT bd, lpLISTH listh)
{
  hOBJ  hobj;
  short   count=0;

  while ( (hobj=load_mesh(bd)) != NULL ) {
    attach_item_tail(listh,hobj);
    count++;
  }
  return count;
}
static  hOBJ load_mesh(lpBUFFER_DAT bd)
{
	MESHDD  g;
	MNODE   node;
	sgFloat  number[3];
	char    string[80];
	short     i, j, num, color;
	LISTH   listh;
	lpOBJ		obj;

	init_vdim(&g.vdim,sizeof(MNODE));
once:
	if ( !(i=load_str( bd, 80,string)) ) return NULL;
	if ( i == 1 ) goto once;
	num = 3;
	
	if ( !get_nums_from_string(string, number, &num) ) goto err;
	if ( num != 3 ) goto err;
	g.m = (short)number[0];  g.n = (short)number[1]; color = (short)number[2];
	if (0 > color || color > 15) color = obj_attrib.color;
//	if (color == vport_bg || color == blink_color) color = obj_attrib.color;
	//if (color == vport_bg) color = obj_attrib.color;
	for(i = 0; i < g.m; i++){
		for(j = 0; j < g.n; j++){
			if ( !load_str( bd, 80,string) ) {
				a_handler_err(AE_ERR_LOAD_STRING,bd->cur_line);
				goto err1;
			}
			num = 3;
			if ( !get_nums_from_string(string, (sgFloat*)&node.p, &num) )goto err;
			if ( num != 3 ) goto err;
			node.num = COND_U | COND_V;
			if(!add_elem( &g.vdim, &node)) goto err1;
		}
	}
	init_listh(&listh);
	if(!np_create_brep(&g, &listh)) goto err1;
	free_vdim(&g.vdim);
	obj = (lpOBJ)listh.hhead;
	obj->color = (BYTE)color;
	return listh.hhead;
err:
	a_handler_err(AE_BAD_STRING,bd->cur_line);
err1:
	free_vdim(&g.vdim);
	return NULL;
}
