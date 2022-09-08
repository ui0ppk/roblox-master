#include "../../sg.h"

#define	NPM_VER		0x01
#define	NPM_VERT	0x02
#define	NPM_EFR		0x04
#define	NPM_EFC		0x08
#define	NPM_GRU		0x10
#define	NPM_FACE	0x20
#define	NPM_PLANE	0x40
#define	NPM_CYCLE	0x80

static BOOL check_np_max(lpNPW np, short nov, short noe, short nof, short noc)
{
	if ( sizeof(*np->v)*(int)(np->maxnov+nov+1) >= 65535L ) 	goto err;
	if ( sizeof(*np->efr)*(int)(np->maxnoe+noe+1) >= 65535L ) goto err;
	if ( sizeof(*np->efc)*(int)(np->maxnoe+noe+1) >= 65535L ) goto err;
	if ( sizeof(*np->f)*(int)(np->maxnof+nof+1) >= 65535L ) 	goto err;
	if ( sizeof(*np->p)*(int)(np->maxnof+nof+1) >= 65535L ) 	goto err;
	if ( sizeof(*np->c)*(int)(np->maxnoc+noc+1) >= 65535L ) 	goto err;
	if ( sizeof(*np->gru)*(int)(np->maxnoe+noe+1) >= 65535L ) goto err;
	return TRUE;
err:
	handler_err(ERR_MAX_NP);
	return FALSE;
}
BOOL o_expand_np(lpNPW np, short nov, short noe, short nof, short noc)
{
	short       nov_tmp;
	WORD				size;
	UCHAR				npm = 0, npm2 = 0;
	VLD					vld;
	VI_LOCATION	loc;

	if ( !check_np_max( np, nov, noe, nof, noc) ) return FALSE;
	init_vld(&vld);
	if ( expand_user_data )
			if ( !expand_user_data (np, nov, noe, nof, noc) ) goto err;

	if (np->v)   npm2 |= NPM_VER;
	if (np->efr) npm2 |= NPM_EFR;
	if (np->efc) npm2 |= NPM_EFC;
	if (np->gru) npm2 |= NPM_GRU;
	if (np->f)   npm2 |= NPM_FACE;
	if (np->p)   npm2 |= NPM_PLANE;
	if (np->c)   npm2 |= NPM_CYCLE;
//   
	if ( np->v && np->nov ) {
		if ( !add_vld_data(&vld,sizeof(D_POINT)*(np->nov),&np->v[1])) goto err;
  		if ( !add_vld_data(&vld,sizeof(VERTINFO)*(np->nov),&np->vertInfo[1])) goto err;
		npm |= NPM_VER;
		if ( np->nov_tmp < np->maxnov ) {
			if ( !add_vld_data(&vld, sizeof(D_POINT)*(np->maxnov - np->nov_tmp),
					&np->v[np->nov_tmp+1]))		goto err;
			if ( !add_vld_data(&vld, sizeof(VERTINFO)*(np->maxnov - np->nov_tmp),
					&np->vertInfo[np->nov_tmp+1]))		goto err;
			npm |= NPM_VERT;
		}

	}
	if ( np->efr && np->noe ) {
		if ( !add_vld_data(&vld,sizeof(EDGE_FRAME)*(np->noe),&np->efr[1])) goto err;
		npm |= NPM_EFR;
		if ( np->efc && np->gru && np->type == TNPW ) {
			if ( !add_vld_data(&vld,sizeof(EDGE_FACE)*(np->noe),&np->efc[1]))
				goto err;
			if ( !add_vld_data(&vld,sizeof(GRU)*(np->noe),&np->gru[1])) goto err;
			npm |= NPM_EFC | NPM_GRU;
		}
	}
	if (np->f && np->nof) {
		if ( !add_vld_data(&vld,sizeof(FACE)*(np->nof),&np->f[1])) goto err;
		npm |= NPM_FACE;
		if ( np->p && np->type == TNPW) {
			if ( !add_vld_data(&vld,sizeof(D_PLANE)*(np->nof),&np->p[1])) goto err;
			npm |= NPM_PLANE;
		}
	}
	if (np->c && np->noc) {
		if ( !add_vld_data(&vld,sizeof(CYCLE)*(np->noc),&np->c[1])) goto err;
			npm |= NPM_CYCLE;
	}


//  
	if(np->v)     SGFree(np->v);
	if(np->vertInfo)     SGFree(np->vertInfo);
	if(np->efr)   SGFree(np->efr);
	if ( np->efc)	SGFree(np->efc);
	if ( np->gru)	SGFree(np->gru);
	if ( np->f )	SGFree(np->f);
	if ( np->p )	SGFree(np->p);
	if ( np->c )	SGFree(np->c);
	np->v		= NULL;
	np->vertInfo = NULL;
	np->efr	= NULL;
	np->efc =	NULL;
	np->gru = NULL;
	np->f		= NULL;
	np->p 	= NULL;
	np->c		=	NULL;

//   
	if ( npm2 & NPM_VER ) {
		size = sizeof(D_POINT)*(np->maxnov+nov+1);
		if ( 0 ==(np->v = (D_POINT*)SGMalloc(size))) goto err1;
		size = sizeof(VERTINFO)*(np->maxnov+nov+1);
		if ( 0 ==(np->vertInfo = (VERTINFO*)SGMalloc(size))) goto err1;
	}

	if ( npm2 & NPM_EFR ) {
		size = sizeof(EDGE_FRAME)*(np->maxnoe+noe+1);
		if ( 0 ==(np->efr = (EDGE_FRAME*)SGMalloc(size))) goto err1;
	}

	if ( npm2 & NPM_EFC ) {
		size = sizeof(EDGE_FACE)*(np->maxnoe+noe+1);
		if ( 0 ==(np->efc = (EDGE_FACE*)SGMalloc(size))) goto err1;
	}

	if ( npm2 & NPM_GRU ) {
		size = sizeof(GRU)*(np->maxnoe+noe+1);
		if ( 0 ==(np->gru = (float*)SGMalloc(size))) goto err1;
	}

	if ( npm2 & NPM_FACE ) {
		size = sizeof(FACE)*(np->maxnof+nof+1);
		if ( 0 ==(np->f = (FACE*)SGMalloc(size))) goto err1;
	}

	if ( npm2 & NPM_PLANE ) {
		size = sizeof(D_PLANE)*(np->maxnof+nof+1);
		if ( 0 ==(np->p = (D_PLANE*)SGMalloc(size))) goto err1;
	}

	if ( npm2 & NPM_CYCLE ) {
		size = sizeof(CYCLE)*(np->maxnoc+noc+1);
		if ( 0 ==(np->c = (CYCLE*)SGMalloc(size))) goto err1;
	}

	if ( !npm ) {
		np->nov_tmp = np->maxnov + nov;
		goto m;
	}

//   
	loc.hpage		= vld.listh.hhead;
	loc.offset	= sizeof(RAW);
	begin_read_vld(&loc);
	if ( npm & NPM_VER ) {
		size = sizeof(D_POINT)*(np->nov);
		read_vld_data(size,&np->v[1]);
		size = sizeof(VERTINFO)*(np->nov);
		read_vld_data(size,&np->vertInfo[1]);
	}
	if ( npm & NPM_VERT ) {
		nov_tmp = np->nov_tmp + nov;
		size = sizeof(D_POINT)*(np->maxnov - np->nov_tmp);
		read_vld_data(size,&np->v[nov_tmp+1]);
		size = sizeof(VERTINFO)*(np->maxnov - np->nov_tmp);
		read_vld_data(size,&np->vertInfo[nov_tmp+1]);
		np->nov_tmp = nov_tmp;
	} else
		np->nov_tmp = np->maxnov + nov;

	if ( npm & NPM_EFR ) {
		size = sizeof(EDGE_FRAME)*(np->noe);
		read_vld_data(size,&np->efr[1]);
	}
	if ( npm & NPM_EFC ) {
		size = sizeof(EDGE_FACE)*(np->noe);
		read_vld_data(size,&np->efc[1]);
	}
	if ( npm & NPM_GRU ) {
		size = sizeof(GRU)*(np->noe);
		read_vld_data(size,&np->gru[1]);
	}
	if ( npm & NPM_FACE ) {
		size = sizeof(FACE)*(np->nof);
		read_vld_data(size,&np->f[1]);
	}
	if ( npm & NPM_PLANE ) {
		size = sizeof(D_PLANE)*(np->nof);
		read_vld_data(size,&np->p[1]);
	}
	if ( npm & NPM_CYCLE ) {
		size = sizeof(CYCLE)*(np->noc);
		read_vld_data(size,&np->c[1]);
	}
	end_read_vld();
	free_vld_data(&vld);

m:
	np->maxnov += nov;
	np->maxnoe += noe;
	np->maxgru += noe;
	np->maxnof += nof;
	np->maxnoc += noc;

	return TRUE;
err:
	handler_err(ERR_NO_VMEMORY);
	free_vld_data(&vld);
	return FALSE;
err1:
	handler_err(ERR_NO_HEAP);
	free_vld_data(&vld);
	return FALSE;
}

lpNPW  creat_np_mem(NPTYPE type,short nov,short noe, short noc, short nof, short nogru)
{
	WORD     len;
	lpNPW    np;

	len = sizeof(NPW);
	if ( 0 ==(np = (lpNPW)SGMalloc(len)) ) {
		handler_err(ERR_NO_HEAP);
		return NULL;
	}
	memset(np,0,len);
	if ( !check_np_max( np, nov, noe, nof, noc) ) {
		SGFree(np);
		return NULL;
	}
	np->type = type;
	np->maxnov = nov;
	np->maxnoe = noe;
	np->maxnof = nof;
	np->maxnoc = noc;
	np->nov_tmp = np->maxnov;


	if ( 0 ==(np->v = (lpD_POINT)SGMalloc(sizeof(D_POINT)*(nov+1)))) goto err;
	if ( 0 ==(np->vertInfo = (lpVERTINFO)SGMalloc(sizeof(VERTINFO)*(nov+1)))) goto err;
	if ( 0 ==(np->efr = (lpEDGE_FRAME)SGMalloc(sizeof(EDGE_FRAME)*(noe+1)))) goto err;
	if ( type != TNPF){
		if ( 0 ==(np->efc = (lpEDGE_FACE)SGMalloc(sizeof(EDGE_FACE)*(noe+1)))) goto err;
		if ( 0 ==(np->f   = (lpFACE)SGMalloc(sizeof(FACE)*(nof+1)))) goto err;
		if ( 0 ==(np->c   = (lpCYCLE)SGMalloc(sizeof(CYCLE)*(noc+1)))) goto err;
		if ( type == TNPW){
			if ( 0 ==(np->p = (lpD_PLANE)SGMalloc(sizeof(D_PLANE)*(nof+1)))) goto err;
			if ( nogru ) {
				if ( 0 ==(np->gru = (lpGRU)SGMalloc(sizeof(GRU)*(noe+1)))) goto err;
				np->maxgru = noe;
			}
		} else {
			if ( nogru > 0)
				if ( 0 ==(np->gru = (lpGRU)SGMalloc(sizeof(GRU)*(nogru+1)))) goto err;
			np->maxgru = nogru;
		}
	}
	return np;
err:
	free_np_mem(&np);
	handler_err(ERR_NO_HEAP);
	return NULL;
}
void free_np_mem(lpNPW * npw1)
{
	lpNPW npw = *npw1;

	if ( !npw ) return;
	if ( npw->v )   SGFree(npw->v);
	if ( npw->vertInfo )   SGFree(npw->vertInfo);
	if ( npw->efr ) SGFree(npw->efr);
	if ( npw->efc) 	SGFree(npw->efc);
	if ( npw->c  ) 	SGFree(npw->c);
	if ( npw->gru) 	SGFree(npw->gru);
	if ( npw->f  ) 	SGFree(npw->f);
	if ( npw->p ) 	SGFree(npw->p);
  SGFree(npw);
	*npw1 = NULL;
}
static short create_level = 0;
static char flg_create = 0;
BOOL create_np_mem_tmp(void)
{
  create_level++;
  if ( npwg ) return TRUE;
	if ( 0 ==(npwg = creat_np_mem(TNPW,MAXNOV,MAXNOE,MAXNOC,MAXNOF,MAXNOE)) )
    return FALSE;
  flg_create = 1;
  return TRUE;

}
void free_np_mem_tmp(void)
{
  create_level--;
  if ( !create_level && flg_create) {
    free_np_mem(&npwg);
    flg_create = 0;
  }
}
BOOL 	defrag_np_mem_tmp(void)
{
	if (  npwg->maxnov == MAXNOV &&
				npwg->maxnoe == MAXNOE &&
				npwg->maxnof == MAXNOF &&
				npwg->maxnoc == MAXNOC ) return TRUE;
	free_np_mem(&npwg);
	if ( 0 ==(npwg = creat_np_mem(TNPW,MAXNOV,MAXNOE,MAXNOC,MAXNOF,MAXNOE)) )
		return FALSE;
	return TRUE;
}

#pragma argsused
BOOL  put_np_brep(lpLNP lnp, int *num)
{
	register int i;
	lpLNP lplnp;

//  npd->listh  = *listh;
//	npd->num_np = num_np;
	lnp->count = 1;
	if ( !begin_rw(&vd_brep,0)) return FALSE;
	for ( i=0; i< vd_brep.num_elem; i++ ) {
		if ( 0 ==(lplnp = (LNP*)get_next_elem(&vd_brep))) return FALSE;
		if ( lplnp->count ) continue;
		*lplnp = *lnp;
		end_rw(&vd_brep);
		*num = i;
		return TRUE;
	}
	end_rw(&vd_brep);
	if ( !add_elem(&vd_brep,lnp) ) return FALSE;
	*num = vd_brep.num_elem -1;
	return TRUE;
}

BOOL	free_np_brep(int num)
{
	lpLNP	lnp;

	if ( 0 ==(lnp=(LNP*)get_elem(&vd_brep,num)) ) return FALSE;
	lnp->count--;
	if ( !lnp->count ) {
		free_item_list(&lnp->listh);
		if ( lnp->hcsg ) SGFree(lnp->hcsg);
			if (lnp->num_surf > 0)
			free_item_list(&lnp->surf);
		memset(lnp,0,sizeof(*lnp));
	}
	return TRUE;
}

BOOL  free_np_brep_list_num(int num)
{
	register int i;
  lpLNP     lnp;

	if ( !begin_rw(&vd_brep,num) ) return FALSE;
	for ( i=num; i< vd_brep.num_elem; i++ ) {
		if ( 0 ==(lnp=(LNP*)get_next_elem(&vd_brep)) ) {
			end_rw(&vd_brep);
			return FALSE;
		}
		free_item_list(&lnp->listh);
	 if ( lnp->hcsg ) SGFree(lnp->hcsg);
		memset(lnp,0,sizeof(*lnp));
	}
	end_rw(&vd_brep);
	delete_elem_list(&vd_brep,num);
	return TRUE;
}
void  free_np_brep_list(void)
{
  free_np_brep_list_num(0L);
  free_vdim(&vd_brep);
}

