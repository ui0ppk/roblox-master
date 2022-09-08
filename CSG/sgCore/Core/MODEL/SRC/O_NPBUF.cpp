#include "../../sg.h"

static WORD get_size_np(lpNPW npw, NPTYPE type);

static BOOL write_np_buf(lpNP_BUF np_buf,lpNPW npw, NPTYPE type);
static BOOL read_np_buf(lpNP_BUF np_buf,lpNPW npw);

BOOL begin_rw_np(lpNP_BUF np_buf)
{
	memset(np_buf,0,sizeof(NP_BUF));
/*
	np_buf->name[0] = '#';
	itoa(buf_level,&np_buf->name[1],10);
	_fstrcat(np_buf->name,".swp");
*/
	{
		/*char Buffer[MAX_PATH]; 	// address of buffer for temp. path
		GetTempPath(MAX_PATH, Buffer );
	   GetTempFileName(Buffer, "pge", 0, np_buf->name);*/
		strcpy(np_buf->name, tempnam("/usr/temp","pge"));		
   }
	if ( !init_buf(&np_buf->bd,np_buf->name,BUF_NEWINMEM) ) return FALSE;

/*
	if ( !add_vld_data(&np_buf->vld,sizeof(fname),&fname) ) //   1- 
		return FALSE;
*/
	init_vdim(&np_buf->dim,sizeof(NP_LOC));

	return TRUE;
}
BOOL end_rw_np(lpNP_BUF np_buf)
{
	free_vdim(&np_buf->dim);

	close_buf(&np_buf->bd);
	nb_unlink(np_buf->name);

//  free_vld_data(&np_buf->vld);
	return TRUE;
}
VNP write_np(lpNP_BUF np_buf,lpNPW npw, NPTYPE type)
{
	NP_LOC np_loc;

	if ( !seek_buf(&np_buf->bd,-1) ) return FALSE;
	np_loc.offset = get_buf_offset(&np_buf->bd);
	np_loc.size   = get_size_np(npw, type);
	if ( !write_np_buf(np_buf, npw, type) ) return NULL;
	if ( !add_elem(&np_buf->dim,&np_loc) ) return NULL;
/*
  memcpy(&np_loc.viloc,&np_buf->vld.viloc,sizeof(VI_LOCATION));
  np_loc.size   = get_size_np(npw, type);
  if ( !write_np_mem(np_buf, npw) ) return NULL;
  if ( !add_elem(&np_buf->dim,&np_loc) ) return NULL;
*/
	return (VNP)np_buf->dim.num_elem;
}

BOOL read_np(lpNP_BUF np_buf,VNP vnp,lpNPW npw)
{
  NP_LOC np_loc;
//  BOOL cod;
  if ( !read_elem(&np_buf->dim,vnp-1,&np_loc) ) 
  {
	  return FALSE;
  }
  if ( !seek_buf(&np_buf->bd,np_loc.offset) ) 
  {
	  return FALSE;
  }
  if ( !read_np_buf(np_buf,npw) ) 
  {
	  return FALSE;
  }
  return TRUE;
/*
	begin_read_vld(&np_loc.viloc);
  cod = read_np_mem(np_buf,npw);
  end_read_vld();
  return cod;
*/
}
BOOL overwrite_np(lpNP_BUF np_buf,VNP vnp, lpNPW npw, NPTYPE type)
{
  NP_LOC np_loc;
  lpNP_LOC lpnp_loc;
  WORD size;
//  BOOL cod;

  if ( !read_elem(&np_buf->dim,vnp-1,&np_loc) ) return FALSE;
  size = get_size_np(npw, type);

  if ( size > np_loc.size ) {
    if ( !seek_buf(&np_buf->bd,-1) ) return FALSE;
    np_loc.offset = get_buf_offset(&np_buf->bd);
		if ( !write_np_buf(np_buf,npw,type) ) return FALSE;
		lpnp_loc = (lpNP_LOC)get_elem(&np_buf->dim,vnp-1);
    lpnp_loc->size = size;
    lpnp_loc->offset = np_loc.offset;
		return TRUE;
  }
  if ( !seek_buf(&np_buf->bd,np_loc.offset) ) return FALSE;
	if ( !write_np_buf(np_buf,npw, type) ) return FALSE;
  return TRUE;
/*
  if ( size != np_loc.size ) {
    memcpy(&np_loc.viloc,&np_buf->vld.viloc,sizeof(VI_LOCATION));
    np_loc.size   = size;
    if ( !write_np_mem(np_buf, npw) ) return FALSE; //  NP
    if ( !write_elem(&np_buf->dim,vnp-1,&np_loc) ) return FALSE;
    return TRUE;
  }
  begin_rezet_vld(&np_buf->vld, &np_loc.viloc);
  cod =write_np_mem(np_buf, npw); //  NP
  end_rezet_vld(&np_buf->vld);

  return cod;
*/
}
// type == TNPF || TNPW
static BOOL write_np_buf(lpNP_BUF np_buf,lpNPW npw, NPTYPE type)
{
  WORD size;
  lpBUFFER_DAT bd = &np_buf->bd;

  if ( !story_data(bd,sizeof(short),&type) )      return FALSE;
	if ( !story_data(bd,sizeof(npw->nov),&npw->nov) )        return FALSE;
	if ( !story_data(bd,sizeof(npw->noe),&npw->noe) )        return FALSE;
	if ( !story_data(bd,sizeof(npw->nof),&npw->nof) )        return FALSE;
	if ( !story_data(bd,sizeof(npw->noc),&npw->noc) )        return FALSE;
//	if ( !story_data(bd,sizeof(npw->maxgru),&npw->maxgru) )  return FALSE;
	if ( !story_data(bd,sizeof(npw->ident),&npw->ident) )    return FALSE;
	if ( !story_data(bd,sizeof(npw->gab),&npw->gab) )        return FALSE;

  if ( npw->nov>npw->maxnov||npw->noe>npw->maxnoe||npw->noc>npw->maxnoc||
       npw->nof >npw->maxnof) {
		handler_err(ERR_NP_MAX);
    return FALSE;
  }


  size = sizeof(D_POINT) * npw->nov;
  if ( !story_data(bd,size,&npw->v[1]) ) return FALSE;

  size = sizeof(VERTINFO) * npw->nov;
  if ( !story_data(bd,size,&npw->vertInfo[1]) ) return FALSE;

  size = sizeof(EDGE_FRAME) * npw->noe;
  if ( !story_data(bd,size,&npw->efr[1]) ) return FALSE ;

	if ( type == TNPF ) goto albl;	//return TRUE;

  size = sizeof(EDGE_FACE) * npw->noe;
  if ( !story_data(bd,size,&npw->efc[1]) ) return FALSE;

  size = sizeof(FACE) * npw->nof;
  if ( !story_data(bd,size,&npw->f[1]) ) return FALSE;

  size = sizeof(CYCLE) * npw->noc;
  if ( !story_data(bd,size,&npw->c[1]) ) return FALSE;

  size = sizeof(GRU) * npw->noe;
  if ( !story_data(bd,size,&npw->gru[1]) ) return FALSE;

  size = sizeof(D_PLANE) * npw->nof;
  if ( !story_data(bd,size,&npw->p[1]) ) return FALSE;
albl:
	if ( write_user_data_buf )
		if (!write_user_data_buf(np_buf,npw) ) return FALSE;

	return TRUE;
}
static BOOL read_np_buf(lpNP_BUF np_buf,lpNPW npw)
{
	WORD size;
	lpBUFFER_DAT bd = &np_buf->bd;
	short	nov,noe,nof,noc,appnov,appnoe,appnof,appnoc;

	npw->nov = npw->noe = npw->nof = npw->noc = 0;
	npw->nov_tmp = npw->maxnov;
	if (load_data(bd,sizeof(npw->type),&npw->type) != sizeof(npw->type)) return FALSE;
	if (load_data(bd,sizeof(npw->nov),&nov) != sizeof(npw->nov)) return FALSE;
	if (load_data(bd,sizeof(npw->noe),&noe) != sizeof(npw->noe)) return FALSE;
	if (load_data(bd,sizeof(npw->nof),&nof) != sizeof(npw->nof)) return FALSE;
	if (load_data(bd,sizeof(npw->noc),&noc) != sizeof(npw->noc)) return FALSE;
//	if (load_data(bd,sizeof(npw->maxgru),&npw->maxgru) != sizeof(npw->maxgru)) return FALSE;
	if (load_data(bd,sizeof(npw->ident),&npw->ident) != sizeof(npw->ident)) return FALSE;
	if (load_data(bd,sizeof(npw->gab),&npw->gab) != sizeof(npw->gab)) return FALSE;

	size = 0;
	if ( nov > npw->maxnov ) { appnov = nov - npw->maxnov; size = 1; }
	else 	                   appnov = 0;
	if ( noe > npw->maxnoe ) { appnoe = noe - npw->maxnoe; size = 1; }
	else 	                   appnoe = 0;
	if ( nof > npw->maxnof ) { appnof = nof - npw->maxnof; size = 1; }
	else 	                   appnof = 0;
	if ( noc > npw->maxnoc ) { appnoc = noc - npw->maxnoc; size = 1; }
	else 	                   appnoc = 0;

	{
		WORD	size;
		size = sizeof(D_POINT)*(npw->maxnov+1);
		memset(npw->v,0,size);
	}

	if ( size ) //   
		if ( !o_expand_np(npw, appnov, appnoe, appnof, appnoc) ) return FALSE;
	npw->nov = nov;
	npw->noe = noe;
	npw->nof = nof;
	npw->noc = noc;

  size = sizeof(D_POINT) * npw->nov;
  if ( load_data(bd,size,&npw->v[1]) != size ) return FALSE;

  size = sizeof(VERTINFO) * npw->nov;
  if ( load_data(bd,size,&npw->vertInfo[1]) != size ) return FALSE;

  size = sizeof(EDGE_FRAME) * npw->noe;
  if ( load_data(bd,size,&npw->efr[1]) != size) return FALSE;

	if ( npw->type == TNPF ) goto albl;	//return TRUE;

  size = sizeof(EDGE_FACE) * npw->noe;
  if ( load_data(bd,size,&npw->efc[1]) != size ) return FALSE;

  size = sizeof(FACE) * npw->nof;
  if ( load_data(bd,size,&npw->f[1]) != size ) return FALSE;

  size = sizeof(CYCLE) * npw->noc;
  if ( load_data(bd,size,&npw->c[1]) != size ) return FALSE;

	size = sizeof(GRU) * npw->noe;
  if ( load_data(bd,size,&npw->gru[1]) != size ) return FALSE;

  size = sizeof(D_PLANE) * npw->nof;
  if ( load_data(bd,size,&npw->p[1]) != size ) return FALSE;
albl:
	if ( read_user_data_buf )
		if (!read_user_data_buf(np_buf,npw) ) return FALSE;

	return TRUE;
}

BOOL add_np_mem(lpVLD vld,lpNPW npw, NPTYPE type)
{
	WORD size;

  if ( !add_vld_data(vld,sizeof(short),&type) )      return FALSE;
  if ( !add_vld_data(vld,sizeof(npw->nov),&npw->nov) )        return FALSE;
  if ( !add_vld_data(vld,sizeof(npw->noe),&npw->noe) )        return FALSE;
  if ( !add_vld_data(vld,sizeof(npw->nof),&npw->nof) )        return FALSE;
  if ( !add_vld_data(vld,sizeof(npw->noc),&npw->noc) )        return FALSE;
//  if ( !add_vld_data(vld,sizeof(npw->maxgru),&npw->maxgru) )  return FALSE;
  if ( !add_vld_data(vld,sizeof(npw->ident),&npw->ident) )    return FALSE;

  if ( npw->nov>npw->maxnov||npw->noe>npw->maxnoe||npw->noc>npw->maxnoc||
       npw->nof>npw->maxnof) {
		handler_err(ERR_NP_MAX);
    return FALSE;
  }

  size = sizeof(D_POINT) * npw->nov;
  if ( !add_vld_data(vld,size,&npw->v[1]) ) return FALSE;

  size = sizeof(VERTINFO) * npw->nov;
  if ( !add_vld_data(vld,size,&npw->vertInfo[1]) ) return FALSE;

  size = sizeof(EDGE_FRAME) * npw->noe;
  if ( !add_vld_data(vld,size,&npw->efr[1]) ) return FALSE ;

  if ( type == TNPF ) return TRUE;

  size = sizeof(EDGE_FACE) * npw->noe;
  if ( !add_vld_data(vld,size,&npw->efc[1]) ) return FALSE;

  size = sizeof(FACE) * npw->nof;
  if ( !add_vld_data(vld,size,&npw->f[1]) ) return FALSE;

  size = sizeof(CYCLE) * npw->noc;
  if ( !add_vld_data(vld,size,&npw->c[1]) ) return FALSE;

  size = sizeof(GRU) * npw->noe;
  if ( !add_vld_data(vld,size,&npw->gru[1]) ) return FALSE;

	if ( type == TNPB ) return TRUE;

  if ( !add_vld_data(vld,sizeof(npw->gab),&npw->gab) )        return FALSE;

  size = sizeof(D_PLANE) * npw->nof;
  if ( !add_vld_data(vld,size,&npw->p[1]) ) return FALSE;

	if ( add_user_data_mem )
    if (!add_user_data_mem(vld,npw) ) return FALSE;

	return TRUE;
}
void rezet_np_mem(lpVI_LOCATION lploc,lpNPW npw, NPTYPE type)
{
	WORD size;
	begin_rezet_vld(lploc);

  rezet_vld_data_f(sizeof(short),&type);
	rezet_vld_data_f(sizeof(npw->nov),&npw->nov);
	rezet_vld_data_f(sizeof(npw->noe),&npw->noe);
	rezet_vld_data_f(sizeof(npw->nof),&npw->nof);
	rezet_vld_data_f(sizeof(npw->noc),&npw->noc);
//	rezet_vld_data_f(sizeof(npw->maxgru),&npw->maxgru);
	rezet_vld_data_f(sizeof(npw->ident),&npw->ident);

	if ( npw->nov>npw->maxnov||npw->noe>npw->maxnoe||npw->noc>npw->maxnoc||
       npw->nof>npw->maxnof) {
		handler_err(ERR_NP_MAX);
    return;
  }

  size = sizeof(D_POINT) * npw->nov;
  rezet_vld_data_f(size,&npw->v[1]);

  size = sizeof(VERTINFO) * npw->nov;
  rezet_vld_data_f(size,&npw->vertInfo[1]);

  size = sizeof(EDGE_FRAME) * npw->noe;
  rezet_vld_data_f(size,&npw->efr[1]);

  if ( type == TNPF ) goto end;

  size = sizeof(EDGE_FACE) * npw->noe;
  rezet_vld_data_f(size,&npw->efc[1]);

  size = sizeof(FACE) * npw->nof;
  rezet_vld_data_f(size,&npw->f[1]);

  size = sizeof(CYCLE) * npw->noc;
  rezet_vld_data_f(size,&npw->c[1]);

  size = sizeof(GRU) * npw->noe;
  rezet_vld_data_f(size,&npw->gru[1]);

  if ( type == TNPB ) goto end;

	rezet_vld_data_f(sizeof(npw->gab),&npw->gab);

  size = sizeof(D_PLANE) * npw->nof;
  rezet_vld_data_f(size,&npw->p[1]);

	if ( rezet_user_data_mem )
		rezet_user_data_mem(npw);

end:
	end_rezet_vld();
}
void get_first_np_loc( lpLISTH listh, lpVI_LOCATION viloc)
{
  viloc->hpage  = listh->hhead;
  viloc->offset = sizeof(RAW);
}
BOOL read_np_mem(lpNPW npw)
{
	WORD size;

  read_vld_data(sizeof(npw->type),&npw->type);
	read_vld_data(sizeof(npw->nov),&npw->nov);
	read_vld_data(sizeof(npw->noe),&npw->noe);
	read_vld_data(sizeof(npw->nof),&npw->nof);
  read_vld_data(sizeof(npw->noc),&npw->noc);
//  read_vld_data(sizeof(npw->maxgru),&npw->maxgru);
	read_vld_data(sizeof(npw->ident),&npw->ident);

	if ( npw->nov>npw->maxnov||npw->noe>npw->maxnoe||npw->noc>npw->maxnoc||
       npw->nof>npw->maxnof)
  {
		handler_err(ERR_NP_MAX);
    return FALSE;
  }
  size = sizeof(D_POINT) * npw->nov;
  read_vld_data(size,&npw->v[1]);

  size = sizeof(VERTINFO) * npw->nov;
  read_vld_data(size,&npw->vertInfo[1]);

  size = sizeof(EDGE_FRAME) * npw->noe;
  read_vld_data(size,&npw->efr[1]);

  if ( npw->type == TNPF ) return TRUE;

  size = sizeof(EDGE_FACE) * npw->noe;
  read_vld_data(size,&npw->efc[1]);

  size = sizeof(FACE) * npw->nof;
  read_vld_data(size,&npw->f[1]);

  size = sizeof(CYCLE) * npw->noc;
  read_vld_data(size,&npw->c[1]);

  size = sizeof(GRU) * npw->noe;
  read_vld_data(size,&npw->gru[1]);

  if ( npw->type == TNPB ) return TRUE;

  read_vld_data(sizeof(npw->gab),&npw->gab);

  size = sizeof(D_PLANE) * npw->nof;
  read_vld_data(size,&npw->p[1]);

  if ( read_user_data_mem )
		if (!read_user_data_mem(npw) ) goto err;

  return TRUE;
err:
  return FALSE;
}

static WORD get_size_np(lpNPW npw, NPTYPE type)
{
	WORD size;

	size = sizeof(npw->nov) + sizeof(npw->noe) + sizeof(npw->nof) +
				 sizeof(npw->noc) + sizeof(npw->ident) +
				 sizeof(npw->gab) + sizeof(D_POINT)*(npw->nov+1) +
                 sizeof(VERTINFO)*(npw->nov+1) +
         sizeof(EDGE_FRAME)*(npw->noe+1);

  if ( type == TNPF ) return size;

  size += sizeof(EDGE_FACE)*npw->noe + sizeof(FACE)*npw->nof +
          sizeof(CYCLE)*npw->noc + sizeof(D_PLANE)*npw->nof;
//  if ( npw->maxgru )  size += sizeof(GRU) * npw->noe;
  size += sizeof(GRU) * npw->noe;
  if ( size_user_data ) size += size_user_data(npw);

  return size;
}
