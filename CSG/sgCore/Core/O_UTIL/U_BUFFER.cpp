#include "../sg.h"
static ULONG total_stored = 0l, total_stored_last = 0l;
static BOOL show_progress = FALSE;

extern int errno;
int buf_level = 0;        //   

BOOL init_buf(lpBUFFER_DAT bd,char* name, BUFTYPE regim)
{
	int n_access = O_RDWR;
	UINT mode =  0;
	int	code;

	if (regim == BUF_NEW)
		n_access |= O_CREAT | O_TRUNC;
  else if (regim == BUF_NEWINMEM)
  {
    n_access |= O_CREAT | O_TRUNC;	mode = 1;
  }
	else {
		if (nb_access( name, 6) == -1)
			n_access  = O_RDONLY;
	}

	buf_level++;
	memset(bd,0,sizeof(BUFFER_DAT));  bd->handle = 0;
	if ((bd->handle=nb_open(name,n_access, mode)) == 0) {
		switch ( errno ) {
			case EACCES: //	Permission denied
				code = UE_OPEN_ACCES;
				break;
/*
			case EINVACC://	Invalid access code
				code = UE_OPEN_EINVACC;
				break;
*/
			case EMFILE: //	Too many open files
				code = UE_OPEN_EMFILE;
				break;
			case ENOENT: //	No such file or directory
			default:
				code = UE_OPEN_ENOENT;
				break;
		}
//		u_handler_err(UE_OPEN_FILE,name);
		u_handler_err(code);
		return FALSE;
	}
	if ( (bd->buf = (char *)SGMalloc(SIZE_VPAGE)) == NULL ) {
		nb_close(bd->handle);
		u_handler_err(UE_NO_VMEMORY);
		return FALSE;
	}
	bd->len_file = nb_filelength(bd->handle);
	return TRUE;
}

void turn_story_data_progress(BOOL on)
{
	 show_progress = on;
	 total_stored = total_stored_last = 0l;
}

BOOL story_data(lpBUFFER_DAT bd, ULONG len, void* data)
{
	ULONG  lentmp, cur_data;
	char  *pdata;

	if (show_progress)  {
		total_stored += len;
		if (total_stored - total_stored_last > 50000L) {
//			wsprintf(st, "%ld", total_stored);
//	   	AddDelim(st, GetIDS(IDS_MSG161)[0]);
//			StatusBarShow(0, st, RGB_MAGENTA, DT_RIGHT);
			total_stored_last = total_stored;
		}
	}
	pdata = (char*)data;
	cur_data = 0;
	while (len) {
    bd->flg_modify = 1;
    if ( len + bd->cur_buf <= SIZE_VPAGE) {
      memcpy(&bd->buf[bd->cur_buf],&pdata[cur_data],len);
			bd->cur_buf += (WORD)len;
      if (bd->len_buf < bd->cur_buf) bd->len_buf = bd->cur_buf;
			break;
		}
		lentmp = SIZE_VPAGE- bd->cur_buf;
    memcpy(&bd->buf[bd->cur_buf],&pdata[cur_data],lentmp);
    nb_lseek(bd->handle,bd->p_beg,SEEK_SET);
    if (nb_write(bd->handle, bd->buf, SIZE_VPAGE) != SIZE_VPAGE) { /*   */
			u_handler_err(UE_WRITE_FILE);
			return FALSE;
		}
    bd->p_beg += SIZE_VPAGE;
    bd->flg_modify = 0;
    bd->cur_buf = 0;
    len -= lentmp;
    cur_data += lentmp;
    bd->len_buf = 0;
  }
	if ( bd->p_beg + bd->cur_buf > bd->len_file)
		bd->len_file = bd->p_beg + bd->cur_buf;
  return TRUE;
}

ULONG load_data(lpBUFFER_DAT bd, ULONG len, void* data)
{
  ULONG rezult;
  int  lentmp, cur_data;
  int  num_bytes;
  char  *pdata;

  rezult = 0;
	pdata = (char*)data;
	cur_data = 0;
  if ( bd->flg_no_read ) data = NULL;

	while (len) {
		if ( len + bd->cur_buf <= bd->len_buf) {
      if ( data ) memcpy(&pdata[cur_data],&bd->buf[bd->cur_buf],len);
			rezult += len;
			bd->cur_buf += ((WORD)len);
			break;
		}
		if ( bd->len_buf <= bd->cur_buf ) {
			lentmp = 0;
			goto m;
		}
		lentmp = bd->len_buf - bd->cur_buf;
    if ( data ) memcpy(&pdata[cur_data],&bd->buf[bd->cur_buf],lentmp);
		rezult += lentmp;
m:
		if ( !flush_buf(bd) ) return FALSE;
		if (lentmp) bd->p_beg += bd->len_buf;
		else        bd->p_beg += bd->cur_buf;
		if (nb_lseek(bd->handle,bd->p_beg,SEEK_SET) == 0xFFFFFFFF) return FALSE;
		if ( (num_bytes = nb_read(bd->handle,bd->buf,SIZE_VPAGE)) == 0xFFFFFFFF) { 
			u_handler_err(UE_READ_FILE);
			return FALSE;
		}
    if ( num_bytes == 0 && len != 0 ) {
			if ( !bd->flg_mess_eof ) u_handler_err(UE_END_OFF_FILE);
      return 0;
    }
		bd->cur_buf = 0;
		len -= lentmp;
		cur_data += lentmp;
		bd->len_buf = num_bytes;
	}
	return rezult;
}

WORD load_str(lpBUFFER_DAT bd, WORD len, char * data)
{
	register WORD lentmp, rezult, cur_data;
	short num_bytes, flg_eos = 0;
	char  *pdata, * ptmp;

	rezult = 0;
	pdata = (char *)data;
	cur_data = 0;
	if ( bd->flg_no_read ) data = NULL;

//	len--;			//    ( 26.01.98)
	len--;			//   
	while (len) {
		if ( len + bd->cur_buf <= bd->len_buf) {
			ptmp = (char*)memchr(&bd->buf[bd->cur_buf], '\n', len);
			if ( ptmp ) {
				flg_eos = 1;
				len = (WORD)(ptmp - &bd->buf[bd->cur_buf]) + 1;
			}
m_read:
			if ( data ) memcpy(&pdata[cur_data],&bd->buf[bd->cur_buf],len);
			rezult += len;
			bd->cur_buf += len;
			break;
		}
		if ( bd->len_buf <= bd->cur_buf ) {
			lentmp = 0;
			goto m;
		}
		lentmp = bd->len_buf - bd->cur_buf;
		ptmp = (char*)memchr(&bd->buf[bd->cur_buf], '\n', lentmp);
		if ( ptmp ) {
			flg_eos = 1;
			len = (WORD)(ptmp - &bd->buf[bd->cur_buf]) + 1;
			goto m_read;
		}
		if ( data ) memcpy(&pdata[cur_data],&bd->buf[bd->cur_buf],lentmp);
		rezult += lentmp;
m:
		if ( !flush_buf(bd) ) return FALSE;
		if (lentmp) bd->p_beg += bd->len_buf;
		else        bd->p_beg += bd->cur_buf;
		if (nb_lseek(bd->handle,bd->p_beg,SEEK_SET) == 0xFFFFFFFF) return FALSE;
		if ( (num_bytes = (short)nb_read(bd->handle,bd->buf,SIZE_VPAGE)) == 0xFFFFFFFF) { /*   */
			u_handler_err(UE_READ_FILE);
			return FALSE;
		}
		if ( num_bytes == 0 && len != 0 ) {
			bd->cur_buf += rezult;
			if ( rezult ) goto end;
			if ( !bd->flg_mess_eof ) u_handler_err(UE_END_OFF_FILE);
			return 0;
		}
		bd->cur_buf = 0;
		len -= lentmp;
		cur_data += lentmp;
		bd->len_buf = num_bytes;
	}
end:
	if ( flg_eos )
	{
		rezult -= 2;
		if ( data[rezult] != 0x0D )
			rezult++;
	}
	data[rezult] = 0;

	bd->cur_line++;
	return rezult+1;
}


BOOL flush_buf(lpBUFFER_DAT bd)
{
	if ( !bd->flg_modify ) return TRUE;

	nb_lseek(bd->handle,bd->p_beg,SEEK_SET);
  if (nb_write(bd->handle,bd->buf,bd->len_buf) != bd->len_buf) { /*   */
		u_handler_err(UE_WRITE_FILE);
		return FALSE;
	}
	bd->flg_modify = 0;
	return TRUE;
}
BOOL seek_buf(lpBUFFER_DAT bd,int offset)
{
  if ( offset == -1 )  offset = bd->len_file;
  if ( ! (bd->p_beg <= (ULONG)offset && offset <= (int)(bd->p_beg + (ULONG)bd->len_buf)  ) ) {
		if ( !flush_buf(bd) ) return FALSE;
		bd->p_beg = offset;
    if (nb_lseek(bd->handle,offset,SEEK_SET) == 0xFFFFFFFF) {
			u_handler_err(UE_SEEK_FILE);
      return FALSE;
    }
		bd->len_buf = 0;
	}
	bd->cur_buf = (short)(offset - bd->p_beg);
	return TRUE;
}
int get_buf_offset(lpBUFFER_DAT bd)
{
  return (bd->p_beg + bd->cur_buf );
}
BOOL close_buf(lpBUFFER_DAT bd)
{
	BOOL cod;

	cod = flush_buf(bd);
	SGFree(bd->buf);
	if (!nb_close(bd->handle)) {
		u_handler_err(UE_CLOSE_FILE);
		cod = FALSE;
	}
	memset(bd,0,sizeof(BUFFER_DAT));  bd->handle = 0;
	buf_level--;
	return cod;
}
