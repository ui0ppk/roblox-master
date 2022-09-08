#include "../sg.h"

typedef struct {
  VI_LOCATION loc;            
  ULONG       len;         
}STR_IM;
typedef STR_IM * lpSTR_IM;

void init_vbd(lpVIRT_BD vbd)
{
  init_vld(&vbd->vld);
  init_vdim(&vbd->vdim, sizeof(STR_IM));
  vbd->ind_free   = 0;
  vbd->free_space = 0;
  vbd->max_len    = 1;
}

BOOL free_vbd(lpVIRT_BD vbd)
{
	free_vld_data(&vbd->vld);
	free_vdim(&vbd->vdim);
	vbd->ind_free   = 0;
	vbd->free_space = 0;
	vbd->max_len    = 0;
  return TRUE;
}

IDENT_V vbd_add_str(lpVIRT_BD vbd, char * str)
{
  return vbd_add_data(vbd, strlen(str) + 1, str);
}

BOOL  vbd_get_str(lpVIRT_BD vbd, IDENT_V ident, char * str)
{
STR_IM im;

	ident--;
  if(!read_elem(&vbd->vdim, ident, &im)) return FALSE;
  if(!load_vld_data(&im.loc, im.len, str)) return FALSE;
	return TRUE;
}
BOOL vbd_write_data(lpVIRT_BD vbd, IDENT_V ident, void * data)
{
STR_IM im;

	ident--;
  if(!read_elem(&vbd->vdim, ident, &im)) return FALSE;
	rezet_vld_data(&im.loc, im.len, data);
	return TRUE;
}
IDENT_V vbd_add_data(lpVIRT_BD vbd, ULONG len, void * data)
{
long      i;
lpSTR_IM  lpim;
STR_IM    im;

  if(len == 0xFFFFFFFF || len == 0) return 0;
	if(len > vbd->max_len) vbd->max_len = len;
	if(vbd->vld.mem)
	 if(((sgFloat)vbd->free_space)/(vbd->vld.mem) > .3 && vbd->vld.listh.num > 2)
		 if(!vbd_sborka_musora(vbd)) return 0;
  memset(&im, 0, sizeof(im));
  im.loc = vbd->vld.viloc;
	im.len = len;
  if(!add_vld_data(&vbd->vld, len, data)) return 0;
  if(!im.loc.hpage) {
    im.loc.hpage  = vbd->vld.listh.hhead;
		im.loc.offset	= sizeof(RAW);
	}
  i = vbd->ind_free;
  if(i >= vbd->vdim.num_elem) {
    if(!add_elem(&vbd->vdim, &im)) return 0;
    vbd->ind_free++;
	} else {
		if(0 ==(lpim = (STR_IM*)get_elem(&vbd->vdim, vbd->ind_free))) return 0;
    vbd->ind_free  = *(long*)lpim;
    *lpim          = im;
	}
  return i + 1;
}

BOOL  vbd_del_data(lpVIRT_BD vbd, IDENT_V ident)
{
lpSTR_IM lpim;

  ident--;
  if(0 ==(lpim = (STR_IM*)get_elem(&vbd->vdim, ident))) return FALSE;
  if(lpim->len == 0xFFFFFFFF) return FALSE;
  vbd->free_space += lpim->len;
  *(long*)lpim    = vbd->ind_free;
  lpim->len     = 0xFFFFFFFF;          // ,   
  vbd->ind_free = ident;
	return TRUE;
}

BOOL  vbd_load_data(lpVIRT_BD vbd, IDENT_V ident, void* data)
{
STR_IM im;
	ident--;
  if(!read_elem(&vbd->vdim, ident, &im)) return FALSE;
  if(!load_vld_data(&im.loc, im.len, data)) return FALSE;
	return TRUE;
}

BOOL  vbd_get_len(lpVIRT_BD vbd, IDENT_V ident, ULONG *len)
{
STR_IM im;
	ident--;
  if(!read_elem(&vbd->vdim, ident, &im)) return FALSE;
  *len = im.len;
	return TRUE;
}

BOOL vbd_sborka_musora(lpVIRT_BD vbd)
{
long          i;
VLD           vld;
char          *buf;
lpSTR_IM      im;
VI_LOCATION   loc;

  if(0 ==(buf = (char*)SGMalloc(vbd->max_len))) goto err;
  init_vld(&vld);

//   VLD
  begin_rw(&vbd->vdim, 0);
  for(i = 0; i < vbd->vdim.num_elem; i++) {
    im = (STR_IM*)get_next_elem(&vbd->vdim);
    if(im->len == 0xFFFFFFFF) continue;
    if(!load_vld_data(&im->loc, im->len, buf)) goto err;
    if(!add_vld_data(&vld, im->len, buf))      goto err;
  }
  SGFree(buf);
  end_rw(&vbd->vdim);
  free_vld_data(&vbd->vld);  //    

//   
  loc.hpage		= vld.listh.hhead;
  loc.offset	= sizeof(RAW);
  begin_rw(&vbd->vdim, 0);
  begin_read_vld(&loc);
  for(i = 0; i< vbd->vdim.num_elem; i++) {
    im = (STR_IM*)get_next_elem(&vbd->vdim);
    if(im->len == 0xFFFFFFFF) continue;
		get_read_loc(&im->loc);
    read_vld_data(im->len, NULL);
  }
  end_read_vld();
  end_rw(&vbd->vdim);

  vbd->free_space = 0;
  vbd->vld = vld;
  return TRUE;
err:
  if(buf)
    SGFree(buf);
  vbd->free_space = 0;
  free_vld_data(&vld);
  return FALSE;
}


BOOL vbd_copy(lpVIRT_BD OldVbd, lpVIRT_BD NewVbd)
{

long          i;
char          *buf;
lpSTR_IM      im;
VI_LOCATION   loc;

  init_vbd(NewVbd);
  if(NULL == (buf = (char*)SGMalloc(OldVbd->max_len))) goto err;

//   VLD
  begin_rw(&OldVbd->vdim, 0);
  for(i = 0; i < OldVbd->vdim.num_elem; i++) {
    im = (STR_IM*)get_next_elem(&OldVbd->vdim);
    if(!add_elem(&NewVbd->vdim, im)) goto err;
    if(im->len == 0xFFFFFFFF) continue;
    if(!load_vld_data(&im->loc, im->len, buf)) goto err;
    if(!add_vld_data(&NewVbd->vld, im->len, buf))      goto err;
  }
  SGFree(buf);
  end_rw(&OldVbd->vdim);

//   
  loc.hpage		= NewVbd->vld.listh.hhead;
  loc.offset	= sizeof(RAW);
  begin_rw(&NewVbd->vdim, 0);
  begin_read_vld(&loc);
  for(i = 0; i < NewVbd->vdim.num_elem; i++) {
    im = (STR_IM*)get_next_elem(&NewVbd->vdim);
    if(im->len == 0xFFFFFFFF) continue;
		get_read_loc(&im->loc);
    read_vld_data(im->len, NULL);
  }
  end_read_vld();
  end_rw(&NewVbd->vdim);

  NewVbd->ind_free = OldVbd->ind_free;
  NewVbd->free_space = 0;
  NewVbd->max_len    = OldVbd->max_len;
  return TRUE;
err:
  if(buf)
    SGFree(buf);
  NewVbd->free_space = 0;
  free_vld_data(&NewVbd->vld);
  return FALSE;
}

