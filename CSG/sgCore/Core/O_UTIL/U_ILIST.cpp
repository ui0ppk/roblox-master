#include "../sg.h"

void il_init_listh(lpI_LISTH ilisth)
{
  memset(ilisth, 0, sizeof(I_LISTH));
}

BOOL il_attach_item_tail(lpVDIM vd, lpI_LISTH ilisth, IDENT_V item)
{
lpI_LIST  l;

  if(ilisth->tail) {  //    
	if(0 ==(l = (lpI_LIST)get_elem(vd, ilisth->tail - 1))) return FALSE;
    l->next = item;
  } else {
    ilisth->head = item;
  }
  if(0 ==(l = (lpI_LIST)get_elem(vd, item - 1))) return FALSE;
  l->prev = ilisth->tail;
  l->next = 0;
  ilisth->tail = item;
  ilisth->num++;
  return TRUE;
}

BOOL il_attach_item_head(lpVDIM vd, lpI_LISTH ilisth, IDENT_V item)
{
  if(!ilisth->head)  //   
    return il_attach_item_tail(vd, ilisth, item);
  return il_attach_item_befo(vd, ilisth, ilisth->head, item);
}

BOOL il_attach_item_befo(lpVDIM vd, lpI_LISTH ilisth, IDENT_V bitem,
                         IDENT_V item)
{
lpI_LIST  l;
IDENT_V   prev;

  if(!bitem)
    return il_attach_item_tail(vd, ilisth, item);

  if(0 ==(l = (lpI_LIST)get_elem(vd, bitem - 1))) return FALSE;
  prev = l->prev;
  l->prev = item;
  if(0 ==(l = (lpI_LIST)get_elem(vd, item - 1))) return FALSE;
  l->prev = prev;
  l->next = bitem;
  if(prev){
	if(0 ==(l = (lpI_LIST)get_elem(vd, prev - 1))) return FALSE;
    l->next = item;
  }
  else
		ilisth->head = item;

	ilisth->num++;
	return TRUE;
}

BOOL il_attach_list_befo(lpVDIM vd, lpI_LISTH ilisth, IDENT_V bitem,
                         lpI_LISTH ilisthins)
{
lpI_LIST  l;
IDENT_V   next, prev;

  if(!ilisthins->num) return TRUE; //   
  if(!il_break(vd, ilisthins, 0)) return FALSE; // 

  if(!bitem){
    if(ilisth->tail) {  //    
	  if(0 ==(l = (lpI_LIST)get_elem(vd, ilisth->tail - 1))) return FALSE;
      next = l->next;
      l->next = ilisthins->head;
      if(next){
		if(0 ==(l = (lpI_LIST)get_elem(vd, next - 1))) return FALSE;
        l->prev = ilisthins->tail;
		if(0 ==(l = (lpI_LIST)get_elem(vd, ilisthins->tail - 1))) return FALSE;
        l->next = next;
      }
      else {
        ilisth->tail = ilisthins->tail;
      }
      ilisth->num += ilisthins->num;
    }
    else{
      *ilisth = *ilisthins;
    }
    return TRUE;
  }

  if(0 ==(l = (lpI_LIST)get_elem(vd, bitem - 1))) return FALSE;
  prev = l->prev;
  l->prev = ilisthins->tail;
  if(0 ==(l = (lpI_LIST)get_elem(vd, ilisthins->tail - 1))) return FALSE;
  l->next = bitem;
  if(prev){
	if(0 ==(l = (lpI_LIST)get_elem(vd, prev - 1))) return FALSE;
    l->next = ilisthins->head;
	if(0 ==(l = (lpI_LIST)get_elem(vd, ilisthins->head - 1))) return FALSE;
    l->prev = prev;
  }
  else
    ilisth->head = ilisthins->head;

  ilisth->num += ilisthins->num;
	return TRUE;
}

BOOL il_close(lpVDIM vd, lpI_LISTH ilisth)
{
lpI_LIST  l;

	if(ilisth->head == ilisth->tail) return TRUE; //    

	if(0 ==(l = (lpI_LIST)get_elem(vd, ilisth->head - 1))) return FALSE;
	l->prev = ilisth->tail;
	if(0 ==(l = (lpI_LIST)get_elem(vd, ilisth->tail - 1))) return FALSE;
	l->next = ilisth->head;
	ilisth->tail = ilisth->head;

	return TRUE;
}

BOOL il_break(lpVDIM vd, lpI_LISTH ilisth, IDENT_V item)
{
lpI_LIST  l;
IDENT_V   prev;

  //    
	if(ilisth->head != ilisth->tail || ilisth->num <= 1) return TRUE;
  if(!item) item = ilisth->head;
  if(item < 0)
    if(!il_get_next_item(vd, -item, &item)) return FALSE;

  if(0 ==(l = (lpI_LIST)get_elem(vd, item - 1))) return FALSE;
  prev = l->prev;
  l->prev = 0;
  if(0 ==(l = (lpI_LIST)get_elem(vd, prev - 1))) return FALSE;
  l->next = 0;
  ilisth->head = item;
  ilisth->tail = prev;

	return TRUE;
}

BOOL il_get_next_item(lpVDIM vd, IDENT_V item, lpIDENT_V next)
{
lpI_LIST l;

  if(0 ==(l = (lpI_LIST)get_elem(vd, item - 1))) return FALSE;
  *next = l->next;
  return TRUE;
}

BOOL il_get_prev_item(lpVDIM vd, IDENT_V item, lpIDENT_V prev)
{
lpI_LIST l;

  if(0 ==(l = (lpI_LIST)get_elem(vd, item - 1))) return FALSE;
  *prev = l->prev;
	return TRUE;
}

BOOL il_detach_item(lpVDIM vd, lpI_LISTH ilisth, IDENT_V item)
{
lpI_LIST l;
IDENT_V  prev, next;
BOOL     flclose;

	flclose = (ilisth->num > 1 && ilisth->head == ilisth->tail);
	if(0 ==(l = (lpI_LIST)get_elem(vd, item - 1))) return FALSE;
	prev = l->prev;
	next = l->next;
	if(flclose && (ilisth->head == item))
		ilisth->head = ilisth->tail = next;

	if(next) {
		if(0 ==(l = (lpI_LIST)get_elem(vd, next - 1))) return FALSE;
		l->prev = prev;
	}
	if(prev) {
		if(0 ==(l = (lpI_LIST)get_elem(vd, prev - 1))) return FALSE;
		l->next = next;
	} else {
		if(ilisth->head != item ) return TRUE;  //   
		ilisth->head = next;
	}
	if(!next) ilisth->tail = prev;
	ilisth->num--;
	if(flclose && ilisth->num == 1){ //     
		if(0 ==(l = (lpI_LIST)get_elem(vd, ilisth->head - 1))) return FALSE;
		l->prev = l->next = 0;
	}
	return TRUE;
}

BOOL il_add_elem(lpVDIM vd, lpI_LISTH filisth, lpI_LISTH ilisth,
								 void  *value, IDENT_V bitem, lpIDENT_V item)
{
IDENT_V id;

	id = filisth->head;
	*item = -id;
	if(!id){
		//       -  
		if(!add_elem(vd, value))               return FALSE;
		id = vd->num_elem;
		*item = -id;
	}
	else{
		if(!il_detach_item(vd, filisth, id))  return FALSE;
		if(!write_elem(vd, id - 1, value))     return FALSE;
	}
  //   
  if(!il_attach_item_befo(vd, ilisth, bitem, id)) return FALSE;
  *item = id;
  return TRUE;
}


BOOL GetIndexByID(lpI_LISTH pIListh, lpVDIM pVd, IDENT_V id, long *index)
{
long    i = 0;
IDENT_V idw;

  idw = pIListh->head;
  while(idw){
    if(idw == id){
      *index = i;
      return TRUE;
    }
    if(!il_get_next_item(pVd, idw, &idw))
      return FALSE;
    i++;
  }
  return FALSE;
}

BOOL GetIDByIndex(lpI_LISTH pIListh, lpVDIM pVd, long index, IDENT_V* id)
{
long    i = 0;
IDENT_V idw;

  if(index < 0)
    return FALSE;
  idw = pIListh->head;
  while(i < index){
    if(!idw)
      return FALSE;
    if(!il_get_next_item(pVd, idw, &idw))
      return FALSE;
    i++;
  }
  *id = idw;
  return TRUE;
}


