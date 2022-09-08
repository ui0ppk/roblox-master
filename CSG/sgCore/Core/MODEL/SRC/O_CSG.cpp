#include "../../sg.h"

BOOL load_csg(lpBUFFER_DAT bd,hCSG * hcsg1)
{
	hCSG hcsg;
	lpCSG csg;
	WORD  size;

	if( load_data(bd,sizeof(size),&size) != sizeof(size))  return FALSE;
	if ( size == 0xFFFF ) return TRUE;
	if ( (hcsg=SGMalloc(size+2)) == NULL) return FALSE;
	csg = (lpCSG)hcsg;
	csg->size = size;
	if( load_data(bd,size,csg->data) != size)  goto err;
	*hcsg1 = hcsg;
	return TRUE;
err:
	SGFree(hcsg);
	return FALSE;
}
BOOL save_csg(lpBUFFER_DAT bd,hCSG hcsg)
{
	lpCSG csg;
	BOOL  cod;
	WORD  dummy = 0xFFFF;

	if ( !hcsg ) {
		cod = story_data(bd,2,&dummy);
		return cod;
	}
	csg = (lpCSG)hcsg;
	cod = story_data(bd,csg->size+2,csg);
	return cod;
}
void free_csg(hCSG hcsg)
{
	if ( !hcsg ) return;
	SGFree(hcsg);
}
