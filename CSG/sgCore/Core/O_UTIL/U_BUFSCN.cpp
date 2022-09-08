#include "../sg.h"


static BOOL set_sb_buf_index(lpSB_INFO sbi);
static BOOL set_sb_next_buf_string(lpSB_INFO sbi);
static BOOL get_sb_next_var(lpSB_INFO sbi, short* type, void* value);

BOOL init_scan_buf(lpSB_INFO sbi, char* name)
{
	if(!init_buf(&sbi->bd, name, BUF_OLD)) return FALSE;
	sbi->lread = get_buf_offset(&sbi->bd);
	sbi->buf[0] = 0;
	sbi->index  = 0;
	sbi->fprogon = TRUE;
	return TRUE;
}

BOOL close_scan_buf(lpSB_INFO sbi)
{
	return close_buf(&sbi->bd);
}

static BOOL set_sb_buf_index(lpSB_INFO sbi)
{
	while(sbi->buf[sbi->index] == ' ' || sbi->buf[sbi->index] == 9)
		sbi->index++;
	if(sbi->buf[sbi->index] == ','){
		if(sbi->fprogon) return FALSE;
		sbi->index++;
		while(sbi->buf[sbi->index] == ' ' || sbi->buf[sbi->index] == 9)
			sbi->index++;
	}
	sbi->fprogon = FALSE;
	return TRUE;
}

static BOOL set_sb_next_buf_string(lpSB_INFO sbi)
{
short l, i, lserv, lmax = C_MAX_STRING_CONSTANT;
char  pk[3]  = {'\r','\n', '\r'};
long  lnoread;

	l = lmax + 2;
	lnoread = sbi->bd.len_file - sbi->lread;
	if(l > lnoread) l = (short)lnoread;
	if(!seek_buf(&sbi->bd, sbi->lread))       return FALSE;
	if(load_data(&sbi->bd, l, sbi->buf) != l) return FALSE;
	lserv = 0;
	for(i = 0; i < l; i++){
		if(sbi->buf[i] == 9) sbi->buf[i] = ' ';
		if(l - i >= 2){
			if(!memcmp(&sbi->buf[i], pk, 2) || !memcmp(&sbi->buf[i], &pk[1], 2)){
				lserv = 2;
				break;
			}
		}
		if(sbi->buf[i] == pk[0] || sbi->buf[i] == pk[1]) {
			lserv = 1;
			break;
		}
	}
	if(i > lmax) i = lmax;
	sbi->buf[i] = 0;
	sbi->lread += (strlen(sbi->buf) + lserv);
	sbi->index = 0;
	sbi->fprogon = TRUE;
	return TRUE;
}

static BOOL get_sb_next_var(lpSB_INFO sbi, short* type, void* value)
{
/*UC_RESULT cres;
ULONG     rlen;
long      lnoread;


	while(TRUE){
		if(!set_sb_buf_index(sbi)) return FALSE;
		if(!sbi->buf[sbi->index]){
			lnoread = sbi->bd.len_file - sbi->lread;
			if(!lnoread) return FALSE;
			if(!set_sb_next_buf_string(sbi)) return FALSE;
			continue;
		}
		if(!uc_calc_express(sbi->buf, &sbi->index, FALSE, FALSE, &cres, &rlen, value))
			return FALSE;
		if(cres.status != UC_NORMAL)
			return FALSE;
		if((cres.type != UC_NUM) && (cres.type != UC_STRING))
			return FALSE;
		*type = cres.type;
		return TRUE;
	}*/return FALSE;
}


BOOL get_sb_sgFloat(lpSB_INFO sbi, BOOL poz, sgFloat *num)
{
/*short type;
	if(!get_sb_next_var(sbi, &type, &gs_result.r)) return FALSE;
	if(type != UC_NUM) return FALSE;
	if(poz && gs_result.r.d <= 0.) return FALSE;
	*num = gs_result.r.d;
	return TRUE;*/return FALSE;
}

BOOL get_sb_int(lpSB_INFO sbi, BOOL poz, short *num)
{
/*short type;
	if(!get_sb_next_var(sbi, &type, &gs_result.r)) return FALSE;
	if(type != UC_NUM) return FALSE;
	if(poz && gs_result.r.d <= 0.) return FALSE;
	if(fabs(gs_result.r.d) > (sgFloat)(MAXSHORT)) return FALSE;
	*num = (short)gs_result.r.d;
	return TRUE;*/return FALSE;
}

BOOL get_sb_long(lpSB_INFO sbi, BOOL poz, int *num)
{
/*short type;
	if(!get_sb_next_var(sbi, &type, &gs_result.r)) return FALSE;
	if(type != UC_NUM) return FALSE;
	if(poz && gs_result.r.d <= 0.) return FALSE;
	if(fabs(gs_result.r.d) > (sgFloat)(MAXLONG)) return FALSE;
	*num = (int)gs_result.r.d;
	return TRUE;*/return FALSE;
}

BOOL get_sb_text(lpSB_INFO sbi, BOOL empty)
{
/*short type;
	if(!get_sb_next_var(sbi, &type, &gs_result.r)) return FALSE;
	if(type != UC_STRING) return FALSE;
	if(!empty && strlen(gs_lex) == 0) return FALSE;
	return TRUE;*/return FALSE;
}

BOOL fc_assign_curr_ident(hOBJ hobj)
//        
//   
{
lpOBJ obj;

//  if(!uc_assign_curr_ident(UC_hOBJ, &hobj)) return FALSE;
  obj = (lpOBJ)hobj;
	obj->status |= ST_IDENT;
	return TRUE;
}

