#include "../sg.h"

BOOL  bo_new_user_data(lpNPW np)
{
	if ( (np->user = SGMalloc(sizeof(BO_USER)*(np->maxnoe+1))) == NULL) 	return FALSE;
	return TRUE;
}

void  bo_free_user_data(lpNPW np)
{
	if ( np ) {
		if ( np->user ) SGFree(np->user);
		np->user = NULL;
	}
}
BOOL bo_copy_user_data(lpNPW dist, lpNPW source)
{
	BO_USER *us_dist, *us_source;

	us_dist   = (BO_USER*)dist->user;
	us_source = (BO_USER*)source->user;
	if (us_source == NULL || us_dist == NULL) return TRUE;
//	if (us_dist == NULL) return FALSE;
	memcpy(us_dist,us_source,sizeof(us_source)*source->noe);
	return TRUE;
}
#pragma argsused
BOOL bo_expand_user_data(lpNPW np, short nov, short noe, short nof, short noc)
{
	lpBO_USER user_new;

		if (noe == 0 || np->user == NULL) return TRUE;
	if ( (user_new = (BO_USER*)SGMalloc(sizeof(BO_USER)*(np->maxnoe+noe+1))) == NULL) return FALSE;
	memcpy(user_new,np->user,sizeof(BO_USER)*(np->noe+1));
	SGFree(np->user);
	np->user = user_new;
	return TRUE;
}
BOOL bo_write_user_data_buf(lpNP_BUF np_buf,lpNPW npw)
{
  lpBO_USER user = (BO_USER*)npw->user;
	lpBUFFER_DAT bd = &np_buf->bd;
	char flg = 0;

	if (npw->user == NULL) {
  	if ( !story_data(bd,sizeof(char),&flg) ) return FALSE;
		return TRUE;
	}
	flg = 1;
  if ( !story_data(bd,sizeof(char),&flg) ) return FALSE;
  if ( !story_data(bd,sizeof(BO_USER)*(npw->noe+1),user) ) return FALSE;
	return TRUE;
}
BOOL bo_read_user_data_buf(lpNP_BUF np_buf,lpNPW npw)
{
  lpBO_USER user = (BO_USER*)npw->user;
	WORD size;
	lpBUFFER_DAT bd = &np_buf->bd;
	char flg;

  if ( load_data(bd,sizeof(flg),&flg) != sizeof(flg))  return FALSE;
	if ( !flg ) {
		memset(user,0,sizeof(BO_USER)*(npw->maxnoe+1));
		return TRUE;
	}
  size = sizeof(BO_USER)*(npw->noe+1);
  if ( load_data(bd,sizeof(BO_USER)*(npw->noe+1),user) != size)  return FALSE;
	return TRUE;
}
BOOL bo_add_user_data_mem(lpVLD vld,lpNPW npw)
{
	lpBO_USER user = (BO_USER*)npw->user;

	if ( !add_vld_data(vld,sizeof(BO_USER)*(npw->noe+1),user) ) return FALSE;
	return TRUE;
}
#pragma argsused
BOOL bo_read_user_data_mem(lpNPW npw)
{
	lpBO_USER user = (BO_USER*)npw->user;
	WORD size;

	size = sizeof(BO_USER)*(npw->noe+1);
	read_vld_data(size,user);
	return TRUE;
}

WORD bo_size_user_data(lpNPW npw)
{
	WORD size;

  if (npw->user == NULL)
  	size = sizeof(char);
	else
  	size = sizeof(char) + sizeof(BO_USER)*(npw->noe+1);

  return size;
}
