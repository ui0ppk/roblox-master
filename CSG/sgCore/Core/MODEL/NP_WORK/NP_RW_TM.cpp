#include "../../sg.h"

static VLD vld;

void np_free_np_tmp(void)
{
	free_vld_data(&vld);
}
BOOL np_read_np_tmp(lpNPW npw)
{
	VI_LOCATION viloc;
	WORD 				size;
	BOOL cod;
	if (npw != NULL) {
    get_first_np_loc( &vld.listh, &viloc);
		begin_read_vld(&viloc);
		cod = read_np_mem(npw);
		if ( cod ) {
	  	read_vld_data(sizeof(size),&size);
			npw->nov_tmp = npw->maxnov - size;
			if ( size ) {
				read_vld_data(sizeof(D_POINT)*size,&npw->v[npw->nov_tmp+1]);
			}
		}
		end_read_vld();
	}
	free_vld_data(&vld);
	return cod;
}
BOOL np_write_np_tmp(lpNPW npw)
{
	WORD	size = 0;
  init_vld(&vld);
	if ( !add_np_mem(&vld, npw, TNPW) ) return FALSE;
	size = npw->maxnov - npw->nov_tmp;
	if ( !add_vld_data(&vld,sizeof(size),&size) )  return FALSE;
	if ( size ) {
		if ( !add_vld_data(&vld,sizeof(D_POINT)*size,
				&npw->v[npw->nov_tmp+1]) )  return FALSE;
	}
	return TRUE;
}
BOOL  np_new_user_data(lpNPW np)
{
	if ( (np->user = SGMalloc(sizeof(NP_USER_GRU)*(np->maxnoe+1))) == NULL) 	return FALSE;
	memset(np->user,0,sizeof(NP_USER_GRU)*(np->maxnoe+1));
	return TRUE;
}
void  np_free_user_data(lpNPW np)
{
	if ( np->user ) SGFree(np->user);
	np->user = NULL;
}
BOOL np_add_user_data_mem(lpVLD vld,lpNPW npw)
{
	lpNP_USER_GRU user = (NP_USER_GRU*)npw->user;

	if ( !add_vld_data(vld,sizeof(NP_USER_GRU)*(npw->noe+1),user) ) return FALSE;
	return TRUE;
}
BOOL np_read_user_data_mem(lpNPW npw)
{
	lpNP_USER_GRU user = (NP_USER_GRU*)npw->user;
	WORD size;

	size = sizeof(NP_USER_GRU)*(npw->noe+1);
	read_vld_data(size,user);
	return TRUE;
}

