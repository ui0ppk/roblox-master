#include "../sg.h"

static NUM_LIST  LIST_ZUDINA = OBJ_LIST;

void copy_data(VADR dist,VADR source,WORD size)
{
  memcpy(dist, source, size);
}

void init_listh(lpLISTH listh)
{
  memset(listh,0,sizeof(LISTH));
}

void free_item_list(lpLISTH listh)
{
	free_tail_list_cur( listh, listh->hhead);
}

void free_tail_list(lpLISTH listh,VADR hpage)
{
	if ( hpage ) {
		get_next_item(hpage,&hpage);
		free_tail_list_cur( listh, hpage);
	}
}

void free_tail_list_cur(lpLISTH listh,VADR hpage)
{
	lpLIST lplist;
	VADR hnext,hprev;

  if ( !hpage ) return;
	get_prev_item(hpage,&hprev);
	if ( hprev) {
  	lplist = (lpLIST)hprev;
		lplist->hnext = NULL;
	} else
		listh->hhead = NULL;
	listh->htail = hprev;
	while (hpage) {
		lplist = (lpLIST)hpage;
		hnext = lplist->hnext;
		SGFree(hpage);
		hpage = hnext;
		listh->num--;
	}
}

WORD free_intermediate_list(lpLISTH listh, VADR hpage_beg, VADR hpage_end)
{
	lpLIST  	lplist;
	VADR 			hnext, hpage;
	WORD	    count;

	if ( hpage_beg == hpage_end ) return 0;
	lplist = (lpLIST)hpage_beg;
	hnext = lplist->hnext;
	if ( hnext == hpage_end ) return 0;

	hpage = hnext;
	count = (WORD)listh->num;
	while (hpage != hpage_end) {
		lplist = (lpLIST)hpage;
		hnext = lplist->hnext;
		SGFree(hpage);
		hpage = hnext;
		listh->num--;
	}
  lplist = (lpLIST)hpage_beg;
  lplist->hnext = hpage_end;

  lplist = (lpLIST)hpage_end;
  lplist->hprev = hpage_beg;
	return (count - (WORD)listh->num);
}

void   RecursiveAddChildrenToSceneTree(lpOBJ lpO)
{
	ISceneTreeControl::TREENODEHANDLE  tr_hndl = lpO->handle_of_tree_node;
	if (tr_hndl==NULL)
		return;

	hOBJ  list_elem = NULL;
	if (lpO->type==OGROUP)
		list_elem = ((lpGEO_GROUP)lpO->geo_data)->listh.hhead;
	else
		if (lpO->type==OPATH)
			list_elem = ((lpGEO_PATH)lpO->geo_data)->listh.hhead;
		else
		{
			assert(0);
			return;
		}

	while (list_elem) 
	{
		lpOBJ curlpO = (lpOBJ)list_elem;
		curlpO->handle_of_tree_node = 
			application_interface->GetSceneTreeControl()->AddNode(&curlpO->extendedClass,tr_hndl);
		if (curlpO->type==OPATH || curlpO->type==OGROUP)
		{
			RecursiveAddChildrenToSceneTree(curlpO);
		}
		get_next_item(list_elem,&list_elem);
	}
}

void attach_item_head(lpLISTH listh,VADR hitem)
{
	if (listh==&objects)
	{
		lpOBJ lpO = (lpOBJ)hitem;
		if (lpO->isAttachedToScene)
			return;
	}

  lpLIST    l;

  if ( listh->hhead ) {  //  
    l = (lpLIST)listh->hhead;
		if ( LIST_ZUDINA ) l += LIST_ZUDINA;
    l->hprev = hitem;
  } else {
    listh->htail = hitem;
  }
  l = (lpLIST)hitem;
	if ( LIST_ZUDINA ) l += LIST_ZUDINA;
  l->hnext = listh->hhead;
  l->hprev = NULL;
  listh->hhead = hitem;
	listh->num++;

  if (listh==&objects)
  {
	  lpOBJ lpO = (lpOBJ)hitem;
	  lpO->isAttachedToScene = true;
	  if (application_interface && 
		  application_interface->GetSceneTreeControl())
	  {
		  if (!application_interface->GetSceneTreeControl()->ShowNode(lpO->extendedClass))
		  {
			lpO->handle_of_tree_node = 
				application_interface->GetSceneTreeControl()->AddNode(&lpO->extendedClass,NULL);
			if (lpO->type==OPATH || lpO->type==OGROUP)
			{
				RecursiveAddChildrenToSceneTree(lpO);
			}
		  }
	  }
  }
}

void attach_item_tail(lpLISTH listh,VADR hitem)
{
	if (listh==&objects)
	{
		lpOBJ lpO = (lpOBJ)hitem;
		if (lpO->isAttachedToScene)
			return;
	}

  lpLIST    l;

  if ( listh->htail ) {  //    
    l = (lpLIST)listh->htail;
		if ( LIST_ZUDINA ) l += LIST_ZUDINA;
    l->hnext = hitem;
  } else {
    listh->hhead = hitem;
  }
  l = (lpLIST)hitem;
	if ( LIST_ZUDINA ) l += LIST_ZUDINA;
  l->hprev = listh->htail;
  l->hnext = NULL;
  listh->htail = hitem;
	listh->num++;

	if (listh==&objects)
	{
		lpOBJ lpO = (lpOBJ)hitem;
		lpO->isAttachedToScene = true;
		if (application_interface && 
			application_interface->GetSceneTreeControl())
		{
			if (!application_interface->GetSceneTreeControl()->ShowNode(lpO->extendedClass))
			{
				lpO->handle_of_tree_node = 
					application_interface->GetSceneTreeControl()->AddNode(&lpO->extendedClass,NULL);
				if (lpO->type==OPATH || lpO->type==OGROUP)
				{
					RecursiveAddChildrenToSceneTree(lpO);
				}
			}
		}
	}
}

void get_next_item(VADR hitem,VADR * hnext)
{
  lpLIST l;

  l = (lpLIST)hitem;
	if ( LIST_ZUDINA ) l += LIST_ZUDINA;
  *hnext = l->hnext;
}

void get_prev_item(VADR hitem,VADR * hprev)
{
  lpLIST l;

  l = (lpLIST)hitem;
	if ( LIST_ZUDINA ) l += LIST_ZUDINA;
  *hprev = l->hprev;
}

void detach_item(lpLISTH listh,VADR hitem)
{
	if (listh==&objects)
	{
		lpOBJ lpO = (lpOBJ)hitem;
		if (!lpO->isAttachedToScene)
			return;
	}

  lpLIST l;
  VADR hprev,hnext;

  l = (lpLIST)hitem;
	if ( LIST_ZUDINA ) l += LIST_ZUDINA;
  hprev = l->hprev;
  hnext = l->hnext;

  if ( hnext ) {
    l = (lpLIST)hnext;
		if ( LIST_ZUDINA ) l += LIST_ZUDINA;
    l->hprev = hprev;
  }
  if ( hprev ) {
    l = (lpLIST)hprev;
		if ( LIST_ZUDINA ) l += LIST_ZUDINA;
    l->hnext = hnext;
  } else {
    if ( listh->hhead != hitem ) return;  //   
    listh->hhead = hnext;
  }
  if ( !hnext ) listh->htail = hprev;
	listh->num--;
	
	if (listh==&objects)
	{
		lpOBJ lpO = (lpOBJ)hitem;
		lpO->isAttachedToScene = false;
		if (application_interface && application_interface->GetSceneTreeControl())
		{
			if (lpO->handle_of_tree_node)
				application_interface->GetSceneTreeControl()->HideNode(lpO->handle_of_tree_node);
		}
	}
}

void attach_item_head_z(NUM_LIST num, lpLISTH listh, VADR hitem){
	LIST_ZUDINA = num;
	attach_item_head(listh, hitem);
  LIST_ZUDINA = OBJ_LIST;
}
void attach_item_tail_z(NUM_LIST num, lpLISTH listh, VADR hitem){
	LIST_ZUDINA = num;
	attach_item_tail(listh, hitem);
  LIST_ZUDINA = OBJ_LIST;
}
void get_next_item_z(NUM_LIST num, VADR hitem, VADR * hnext){
	LIST_ZUDINA = num;
	get_next_item(hitem, hnext);
  LIST_ZUDINA = OBJ_LIST;
}
void get_prev_item_z(NUM_LIST num, VADR hitem, VADR * hprev){
	LIST_ZUDINA = num;
	get_prev_item(hitem, hprev);
  LIST_ZUDINA = OBJ_LIST;
}
void detach_item_z(NUM_LIST num, lpLISTH listh, VADR hitem)
{
	LIST_ZUDINA = num;
	detach_item(listh, hitem);
	if (num==OBJ_LIST)
	{
		lpOBJ lpO = (lpOBJ)hitem;
		SG_POINT pMax,pMin;
		lpO->extendedClass->GetGabarits(pMin,pMax);
		if (fabs(pMin.x-scene_min.x)<0.0000001 ||
			fabs(pMin.y-scene_min.y)<0.0000001 ||
			fabs(pMin.z-scene_min.z)<0.0000001 ||
			fabs(pMax.x-scene_max.x)<0.0000001 ||
			fabs(pMax.y-scene_max.y)<0.0000001 ||
			fabs(pMax.z-scene_max.z)<0.0000001)
				get_scene_limits();
	}
    LIST_ZUDINA = OBJ_LIST;
}


