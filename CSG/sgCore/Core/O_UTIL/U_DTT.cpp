#include "../sg.h"

void (*u_ted_exit)  (void) = NULL;

BOOL ted_text_init(lpDTED_TEXT dtt, short max_line)
    
{
char *m;

	memset(dtt, 0, sizeof(DTED_TEXT));
	//     
	if(0 ==(m = (char*)SGMalloc(sizeof(VDIM)    +
									sizeof(VLD)))) return FALSE;

	dtt->vd     = (lpVDIM)   (m);
	dtt->vld    = (lpVLD)    (m + sizeof(VDIM));
	init_vdim(dtt->vd, sizeof(DTED_STR));
	init_vld(dtt->vld);
	dtt->maxline = max_line;
	return TRUE;
}

void static_text_init(lpDTED_TEXT dtt, short max_line)
  
{
	init_vdim(dtt->vd, sizeof(DTED_STR));
	init_vld(dtt->vld);
	dtt->maxline   = max_line;
	dtt->gmod_flag = 0;
}

void static_text_free(lpDTED_TEXT dtt)

{
	free_vdim(dtt->vd);
	free_vld_data(dtt->vld);
	init_vdim(dtt->vd, sizeof(DTED_STR));
	init_vld(dtt->vld);
	dtt->maxline   = 0;
	dtt->gmod_flag = 0;
}


void ted_text_free(lpDTED_TEXT dtt)
 
{
	if(dtt->vd){
		free_vdim(dtt->vd);
		free_vld_data(dtt->vld);
		SGFree(dtt->vd);
	}
	memset(dtt, 0, sizeof(DTED_TEXT));
}


BOOL ted_text_add(lpDTED_TEXT dtt, char *str)
    
{
DTED_STR tstr;
WORD     len;
char     c = 0;
BOOL     cod;
short    imax = C_MAX_STRING_CONSTANT;

	if(dtt->vd->num_elem > MAXSHORT - 100) return FALSE;
	len = strlen(str);
	if(len > imax) {
		c = str[imax];
		str[imax] = 0;
		len = imax;
	}
	if(len > dtt->maxline)  dtt->maxline = len;
	len++;
	memset(&tstr, 0, sizeof(DTED_STR));
	cod = add_data_to_avalue(dtt->vld, &tstr.v, len, str);
	if(c) str[imax] = c;
	if(!cod) return FALSE;
	if(!add_elem(dtt->vd, &tstr))
		return FALSE;
	return TRUE;
}

void ted_load_str(void  *ted, IDENT_V id, char *str, WORD *len)
{
DTED_STR    tstr;
lpDTED_TEXT dtt = (lpDTED_TEXT)ted;
ULONG       llen; 

	if(!read_elem(dtt->vd, id - 1, &tstr))         ted_exit();
	if(!load_data_from_avalue(&tstr.v, &llen, str))  ted_exit();
  *len = (WORD)llen;
}

IDENT_V ted_get_next_str(void  *ted, IDENT_V id, char *str)

{
DTED_STR    tstr;
lpDTED_TEXT dtt = (lpDTED_TEXT)ted;
ULONG       len;

	if(!id){
	 if(!dtt->vd->num_elem){
		str[0] = 0;
		return -1;
	 }
	 id = 1;
	}
	if(!read_elem(dtt->vd, id - 1, &tstr))          ted_exit();
	if(!load_data_from_avalue(&tstr.v, &len, str))  ted_exit();
	if(id >= dtt->vd->num_elem) id = 0;
	else id++;
	return id;
}

void ted_get_text_size(void  *ted, ULONG *numchar, int *numstr)
  
{
DTED_STR    tstr;
lpA_VALUE   av;
lpDTED_TEXT dtt = (lpDTED_TEXT)ted;
IDENT_V     id;

	*numchar = 0;
	av = &tstr.v;
	for(id = 0; id < dtt->vd->num_elem; id++){
		if(!read_elem(dtt->vd, id, &tstr))     ted_exit();
		if(av->status == A_NOT_INLINE)
			*numchar += (av->value.ni.len - 1);
		else
			*numchar += (av->status - 1);
	}
	*numstr = (int)dtt->vd->num_elem;
	return;
}

BOOL create_dtt_by_file(lpBUFFER_DAT bd, lpDTED_TEXT dtt, BOOL dosflag, BOOL *trimflag)

{
char  buf[C_MAX_STRING_CONSTANT + 2];
char  pk[3]  = {'\r','\n', '\r'};
short l, i, lserv, lmax = C_MAX_STRING_CONSTANT;
int   lread, lnoread, numchar;
BOOL  trim;

	trim = *trimflag;
	*trimflag = FALSE;

	l = lmax + 2;
	lread = get_buf_offset(bd);

	numchar = 0;
	while((lnoread = bd->len_file - lread) > 0){
		if(l > lnoread) l = (short)lnoread;
		if(!seek_buf(bd, lread))       return FALSE;
		if(load_data(bd, l, buf) != l) return FALSE;
		lserv = 0;
		for(i = 0; i < l; i++){
			if(!buf[i] || ((UCHAR)buf[i]) == ML_S_C || buf[i] == 9) buf[i] = ' ';
			if(l - i >= 2){
				if(!memcmp(&buf[i], pk, 2) || !memcmp(&buf[i], &pk[1], 2)){
					lserv = 2;
					break;
				}
			}
			if(buf[i] == pk[0] || buf[i] == pk[1]) {
				lserv = 1;
				break;
			}
		}
		if(i > lmax) i = lmax;
		buf[i] = 0;
		lread += (strlen(buf) + lserv);
		if(dosflag)
			z_OemToAnsi(buf, buf, 0);
		if(trim){
			numchar += (strlen(buf) + 2);
			if(numchar > TED_MAX_TEXT_LEN){
				*trimflag = TRUE;
				return TRUE;
			}
		}
		if(!ted_text_add(dtt, buf)){
			*trimflag = TRUE;
			return TRUE;
		}
		continue;
	}
	return TRUE;
}

BOOL create_file_by_dtt(lpBUFFER_DAT bd, lpDTED_TEXT dtt, BOOL dosflag)

{
char    pk[2]  = {'\r','\n'};
IDENT_V id = 0;
ULONG   numchar;
int     numstr;
char    buf[C_MAX_STRING_CONSTANT + 1];

	ted_get_text_size(dtt, &numchar, &numstr);
	if(numstr){
		do{
			id = ted_get_next_str(dtt, id, buf);
			if(dosflag)
				z_AnsiToOem(buf, buf, 0);
			if(!story_data(bd, strlen(buf), buf)) return FALSE;
			if(!story_data(bd, 2, pk))            return FALSE;
		}while(id);
	}
	return TRUE;
}

void ted_exit(void){

	if(u_ted_exit)  u_ted_exit();
	exit(1);
}
