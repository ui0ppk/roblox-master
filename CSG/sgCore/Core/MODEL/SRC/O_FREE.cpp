#include "../../sg.h"

extern DWORD alloc_obj_count;

static OSCAN_COD free_pre_scan  (hOBJ hobj,lpSCAN_CONTROL lpsc);
static OSCAN_COD free_post_scan (hOBJ hobj,lpSCAN_CONTROL lpsc);


void o_free(hOBJ hobj,lpLISTH listh)
{
	SCAN_CONTROL sc;

	if ( !hobj ) return;

  if ( listh ) detach_item(listh,hobj);

  if (application_interface && application_interface->GetSceneTreeControl())
  {
	  lpOBJ lpO = (lpOBJ)hobj;
	  if (lpO->handle_of_tree_node)
		  application_interface->GetSceneTreeControl()->RemoveNode(lpO->handle_of_tree_node);
  }

  init_scan(&sc);
  sc.user_pre_scan      = free_pre_scan;
	sc.user_post_scan     = free_post_scan;
	o_scan(hobj,&sc);
}

static OSCAN_COD free_pre_scan(hOBJ hobj,lpSCAN_CONTROL lpsc)
{
	if ( lpsc->type != OINSERT ) return OSTRUE;
	{
		int      num;
		lpIBLOCK  blk;
		lpOBJ     obj;
		VADR			hcsg;

		if ( free_extern_obj_data)
			free_extern_obj_data(hobj);//  

		//  CSG
		obj = (lpOBJ)hobj;
		hcsg = obj->hcsg;
		free_csg(hcsg);
//    FreeDnmInfo(hobj);  //   

		num = ((lpGEO_INSERT)obj->geo_data)->num;
    if ( (blk = (lpIBLOCK)get_elem(&vd_blocks,num)) != NULL ) {
      blk->count--;
    }
		SGFree(hobj);
    if(alloc_obj_count > 0)alloc_obj_count--;
    return OSSKIP;
	}
}
#pragma argsused
static OSCAN_COD free_post_scan(hOBJ hobj,lpSCAN_CONTROL lpsc)
{
	lpOBJ obj;
	hCSG  hcsg;
  BOOL  (*o_free_geo)(lpOBJ obj);

	if ( free_extern_obj_data)
		free_extern_obj_data(hobj);//  

	//  CSG
	obj = (lpOBJ)hobj;
	hcsg = obj->hcsg;
	free_csg(hcsg);

	DeleteExtendedClass(obj);

  o_free_geo = (BOOL (*)(lpOBJ obj))GetObjMethod(obj->type, OMT_FREE_GEO);
	if(!o_free_geo) {
		handler_err(ERR_NUM_OBJECT);
		return OSFALSE;
  }
	o_free_geo(obj);  //   

	SGFree(hobj);
  if(alloc_obj_count > 0)alloc_obj_count--;
	return OSTRUE;
}
#pragma argsused
BOOL  free_geo_simple_obj(lpOBJ obj)
{
	return TRUE;
}

BOOL  free_geo_text(lpOBJ obj)
{
	IDENT_V idf, idt;
	idf = ((lpGEO_TEXT)obj->geo_data)->font_id;
	idt = ((lpGEO_TEXT)obj->geo_data)->text_id;
	delete_ft_item(FTFONT,  idf);
	delete_ft_item(FTTEXT, idt);
	return TRUE;
}

BOOL  free_geo_dim(lpOBJ obj)
{
	IDENT_V idf, idt;
	idf = ((lpGEO_DIM)obj->geo_data)->dtstyle.font;
	idt = ((lpGEO_DIM)obj->geo_data)->dtstyle.text;
	delete_ft_item(FTFONT, idf);
	delete_ft_item(FTTEXT, idt);
	return TRUE;
}

BOOL  free_geo_brep(lpOBJ obj)
{
	free_np_brep(((lpGEO_BREP)obj->geo_data)->num);
	return TRUE;
}

BOOL  free_geo_spline(lpOBJ obj)
{
	if(((lpGEO_SPLINE)obj->geo_data)->hpoint)
	  SGFree( ((lpGEO_SPLINE)obj->geo_data)->hpoint);
	if(((lpGEO_SPLINE)obj->geo_data)->hderivates)
	  SGFree( ((lpGEO_SPLINE)obj->geo_data)->hderivates);
	if(((lpGEO_SPLINE)obj->geo_data)->hvector)
	SGFree( ((lpGEO_SPLINE)obj->geo_data)->hvector);
	return TRUE;
}

BOOL  free_geo_frame(lpOBJ obj)
{
	free_vld_data(&((lpGEO_FRAME)obj->geo_data)->vld);
	return TRUE;
}




