#include "../sg.h"

static 	VI_LOCATION 	loc1;
static 	lpRAW	 		 		lprawg = NULL;

BOOL  add_vpage(lpVLD lpvld);

void init_vld(lpVLD vld)
{
  memset(vld, 0, sizeof(VLD));
}
void  rezet_vld_data(lpVI_LOCATION lploc, ULONG len, void* p)
{
	WORD      len_tmp;
	lpRAW     lpraw;
	char*     ptmp;
	VADR      hpage, hnext;
	WORD      offset;

	offset = lploc->offset;
	hpage  = lploc->hpage;
	lpraw = (lpRAW)hpage;
	ptmp = (char*)lpraw + offset;
	while (1) {
		len_tmp = SIZE_VPAGE - offset;
		if ( len_tmp > len ) len_tmp = (WORD)len;
		memcpy( ptmp,p,len_tmp);
		len -= len_tmp;
		hnext = lpraw->list.hnext;
		if ( !len ) break;
		hpage = hnext;
		lpraw = (lpRAW)hpage;
		offset = sizeof(RAW);
		ptmp = (char*)lpraw + offset;
		reinterpret_cast<char * &>(p) += len_tmp;
	}
}
void  rezet_vld_data_f(ULONG len, void* p )
{
	WORD      len_tmp;
	lpRAW     lpraw;
	char*     ptmp;
	VADR      hnew_page;

	lpraw = lprawg;
	while (1) {
		ptmp = (char*)lpraw + loc1.offset;
		len_tmp = SIZE_VPAGE - loc1.offset;
		if ( len_tmp > len ) len_tmp = (WORD)len;
		if ( p )
      memcpy( ptmp,p,len_tmp);
		len -= len_tmp;
		loc1.offset += len_tmp;
		if ( !len ) break;
		hnew_page = lpraw->list.hnext;
    lpraw = (lpRAW)hnew_page;
		lprawg = lpraw;
		loc1.hpage  = hnew_page;
		loc1.offset = sizeof(RAW);
    reinterpret_cast<char * &>(p) += len_tmp;
	}
}
BOOL  add_vld_data(lpVLD lpvld, ULONG len, void* p )
{
	ULONG       len_tmp;
	lpRAW       lpraw;
	char*       ptmp;
	VADR        hnext;
  VI_LOCATION vsave;

  if ( lpvld->lprawg ) lpraw = lpvld->lprawg;
	else {
    if ( !lpvld->viloc.hpage ) {
      if ( !add_vpage(lpvld) ) return FALSE;
      lpvld->viloc.hpage = lpvld->listh.htail;
      lpvld->viloc.offset = sizeof(RAW);
	    lpvld->mem = 0;
		}
    lpraw = (lpRAW)lpvld->viloc.hpage;
	}
  ptmp = (char*)lpraw + lpvld->viloc.offset;
  vsave = lpvld->viloc;      //   
	while (1) {
    len_tmp = SIZE_VPAGE - lpvld->viloc.offset;
		if ( len_tmp > len ) len_tmp = len;
		memcpy( ptmp,p,len_tmp);
		len -= len_tmp;
		lpraw->free_mem -= (WORD)len_tmp;
    lpvld->viloc.offset += (WORD)len_tmp;
    lpvld->mem += len_tmp;

		if ( !len ) {
      if ( lpvld->lprawg ) break;  //    
			break;
		}
		hnext = lpraw->list.hnext;
		if ( !hnext ) {
      if ( !add_vpage(lpvld) ) goto err;
      hnext = lpvld->listh.htail;
		}
		lpraw = (lpRAW)hnext;
    lpvld->viloc.hpage  = hnext;
    lpvld->viloc.offset = sizeof(RAW);
    if ( lpvld->lprawg ) lpvld->lprawg = lpraw;
		lpraw->free_mem     = SIZE_VPAGE - sizeof(RAW);
		ptmp = (char*)lpraw + sizeof(RAW);
		reinterpret_cast<char * &>(p) += len_tmp;
	}
	return TRUE;
err:
  ptmp = (char*)vsave.hpage;
  ptmp += vsave.offset;
  *ptmp = 0;
  return FALSE;
}

void lock_vld_data(lpVLD vld)
{
  vld->lprawg =  (lpRAW)vld->viloc.hpage;
}

void unlock_vld_data(lpVLD vld)
{
	if ( vld->lprawg ) {
  	vld->lprawg = NULL;
	}
}

void begin_read_vld(lpVI_LOCATION lploc)
{
	lprawg = (lpRAW)lploc->hpage;
  loc1 = *lploc;
}

void end_read_vld(void)
{
	lprawg = NULL;
	memset(&loc1,0,sizeof(loc1));
}
void begin_rezet_vld(lpVI_LOCATION lploc)
{
  lprawg = (lpRAW)lploc->hpage;
	loc1 = *lploc;
}
void end_rezet_vld(void)
{
	lprawg = NULL;
	memset(&loc1,0,sizeof(loc1));
}
void get_read_loc(lpVI_LOCATION lploc)
{
	*lploc = loc1;
}
void set_read_loc(lpVI_LOCATION lploc)
{
  loc1 = *lploc ;
}

void  read_vld_data(ULONG len, void* p )
{
	WORD    len_tmp;
	lpRAW   lpraw;
  char    *ptmp,  *p1;
	VADR    hnew_page;

  if ( !lprawg ) goto err;
  p1 = (char*)p;
	lpraw = lprawg;
	while (1) {
		ptmp = (char*)lpraw + loc1.offset;
		len_tmp = SIZE_VPAGE - loc1.offset;
		if ( len_tmp > len ) len_tmp = (WORD)len;
		if ( p )
			memcpy( p,ptmp,len_tmp);
		len -= len_tmp;
		loc1.offset += len_tmp;

		if ( !len ) break;

		hnew_page = lpraw->list.hnext;
    lprawg = NULL;
    if ( !hnew_page ) goto err;
		lpraw = (lpRAW)hnew_page;
		lprawg = lpraw;
		loc1.hpage  = hnew_page;
		loc1.offset = sizeof(RAW);
		if ( p )
			reinterpret_cast<char * &>(p) += len_tmp;
	}
  return;
err:
	if ( p1 )	memset(p1,0,len);
}

BOOL  load_vld_data(lpVI_LOCATION loc1, ULONG len, void* p )
{
	WORD    	  len_tmp;
	lpRAW     	lpraw;
  char* 	    ptmp;
	VI_LOCATION loc;
	VADR				hnew_page;

	loc = *loc1;
	while (1) {
		lpraw = (lpRAW)loc.hpage;
		ptmp = (char*)lpraw + loc.offset;
		len_tmp = SIZE_VPAGE - loc.offset;
		if ( len_tmp > len ) len_tmp = (WORD)len;
		if ( p )
			memcpy( p,ptmp,len_tmp);
		len -= len_tmp;

		hnew_page = lpraw->list.hnext;
		if ( !len ) break;

		if ( !hnew_page ) return FALSE;
		loc.hpage  = hnew_page;
		loc.offset = sizeof(RAW);
		if ( p )
			reinterpret_cast<char * &>(p) += len_tmp;
	}
	return TRUE;
}

void free_vld_data(lpVLD vld)
{
	 free_item_list(&vld->listh);
  init_vld(vld);
}

BOOL add_vpage(lpVLD lpvld)
{
	lpRAW lpraw;
	VADR  hpage;

	if ( (hpage = SGMalloc(SIZE_VPAGE)) == NULL ) return FALSE;
	lpraw = (lpRAW)hpage;
  memset(lpraw,0,SIZE_VPAGE);
	lpraw->free_mem = SIZE_VPAGE - sizeof(RAW);
  lpraw->list.hprev = lpvld->listh.htail;
  attach_item_tail(&lpvld->listh,hpage);
	return TRUE;
}
