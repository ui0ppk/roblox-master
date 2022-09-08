#include "../sg.h"

BOOL add_data_to_avalue(lpVLD vld, lpA_VALUE av, ULONG len, void* value)
{

  if(len <= MAX_INLINE_VALUE){ //    
    memcpy(av->value.cv, value, len);
    av->status = (UCHAR)len;
    return TRUE;
  }
  else{ //  -    vld
    av->status = A_NOT_INLINE;
    return add_not_inline_data(vld, &av->value.ni, len, value);
  }
}

BOOL load_data_from_avalue(lpA_VALUE av, ULONG *len, void* value)
{
  if(av->status == A_NOT_INLINE){
    if(!load_vld_data(&av->value.ni.loc, av->value.ni.len, value))
      return FALSE;
     *len = av->value.ni.len;
  }
  else{
    memcpy(value, av->value.cv, av->status);
		*len = av->status;
  }
  return TRUE;
}


BOOL add_not_inline_data(lpVLD vld, lpNOT_INLINE ni, ULONG len,
                         void* data)
{
	memset(ni, 0, sizeof(NOT_INLINE));
  ni->loc = vld->viloc;
  ni->len = len;
  if(!add_vld_data(vld, len, data)) return FALSE;
  if(!ni->loc.hpage) {
    ni->loc.hpage  = vld->listh.hhead;
    ni->loc.offset = sizeof(RAW);
	}
  return TRUE;
}
