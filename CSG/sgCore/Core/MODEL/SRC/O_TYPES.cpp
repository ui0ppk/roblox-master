#include "../../sg.h"

short otype_num(OBJTYPE type)
{
  if (type < 0 || type >= GeoObjTypesCount) {
    handler_err(ERR_NUM_OBJECT);
    return -1;
  }
  return type;
}
unsigned char maska[8] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
void add_type_list(OFILTER ofilter,short num, ...)
{
  va_list ap;
	OBJTYPE type;
	register short i;

	va_start(ap,num);
	for (i=0;i<num;i++) {
		type = va_arg(ap,OBJTYPE);
		add_type(ofilter,type);
	}
	va_end(ap);
}
void add_type(OFILTER ofilter,OBJTYPE type)
{
  WORD num_byte,num_bit;

	if(type == ALL_OBJECTS)
    memcpy(ofilter, enable_filter, sizeof(OFILTER));
	else if(type == NULL_OBJECTS)
    memset(ofilter, 0x00, sizeof(OFILTER));
  else{
    num_byte = (WORD)(type>>3);
    num_bit =  (WORD)(type&7);
    ofilter[num_byte] |= maska[num_bit];
  }
	return;
}

BOOL check_type(OFILTER ofilter,OBJTYPE type)
{
  WORD num_byte,num_bit;

  num_byte = (WORD)(type>>3);
  num_bit =  (WORD)(type&7);
	return ( (BOOL)(ofilter[num_byte] & maska[num_bit]));
}
