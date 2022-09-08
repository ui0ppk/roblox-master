#include "../sg.h"

OBJTYPE get_obj_type(EL_OBJTYPE eltype)

{
OBJTYPE i;

  if(!eltype || !pElTypes)
    return NULL_OBJECTS;
  for(i = 0; i < GeoObjTypesCount; i++){
    if(pElTypes[i] == eltype)
      return i;
  }
	return NULL_OBJECTS;
}

EL_OBJTYPE get_el_type(OBJTYPE type)
{
  if(type < 0 || type >= GeoObjTypesCount)
	  return ENULL;
  return pElTypes[type];
}

void set_filter_msk(OFILTER filter, EL_OBJTYPE msk)
{
OBJTYPE i;
	add_type(filter, NULL_OBJECTS);
  for(i = 0; i < GeoObjTypesCount; i++){
    if(pElTypes[i] & msk)
			add_type(filter, i);
  }
}
