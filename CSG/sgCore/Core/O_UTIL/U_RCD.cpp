#include "../sg.h"

BOOL rcd_test_expression(lpRECORDSET rcd, char *express, BOOL *res);
static BOOL rcd_load(lpRECORDSET rcd, int irec);
static BOOL rcd_find(lpRECORDSET rcd, char *str, int sirec, int eirec, short step);

BOOL rcd_create(lpRECORDSET rcd, char* name, lpRCDFIELD fld, short numfld)
{
short          i;
WORD           len, lrec;
char           *hdrec;
UCHAR          *hdfield;

	memset(rcd, 0, sizeof(RECORDSET));
	//  
	if(numfld <= 0 || numfld > 255)
		goto badfld;
	for(i = 0; i < numfld; i++){
		if(strlen(fld[i].name) > 10)
			goto badfld;
		switch(fld[i].type){
			case 'C':
			case 'c':
			case 'N':
			case 'n':
			case 'L':
			case 'l':
				break;
			default:
				goto badfld;
		}
		if(fld[i].size <= fld[i].prec)
			goto badfld;
	}

	if(!init_buf(&rcd->bd, name, BUF_NEW)){
		rcd->err = RCDERR_OPEN;
		return FALSE;
	}

	//    
	//    
	len = (WORD)(32*(numfld + 1) + 1);
	if((hdrec = (char*)SGMalloc(len)) == NULL) {
		rcd->err = RCDERR_NOMEM;
		return FALSE;
	}
	memset(hdrec, 0, len);
	hdfield = (UCHAR*)(hdrec + 32);
	//   
	lrec = 0;
	for(i = 0; i < numfld; i++){
		strcpy((char*)hdfield, fld[i].name);
		hdfield[11] = (UCHAR)toupper(fld[i].type);
		hdfield[16] = (BYTE)((hdfield[11] == 'L') ? 1: fld[i].size);
		hdfield[17] = (BYTE)((hdfield[11] == 'N') ? fld[i].prec : 0);
		lrec = (WORD)(lrec + fld[i].size);
		hdfield += 32;
	}
	*hdrec = 0x03;
  GetCurrentDateForDbf(hdrec + 1);
	*((int*)(hdrec + 4)) = 0; // 
	*((WORD*)(hdrec+ 8)) = len;
	*((WORD*)(hdrec + 10)) = lrec + 1;
	*(hdrec + len - 1) = 0x0D;
	if(!story_data(&rcd->bd, len, hdrec)){
		rcd->err = RCDERR_WRITE;
		SGFree(hdrec);
 //nb 		hdrec = NULL;
		rcd_close(rcd);
		return FALSE;
	}
	SGFree(hdrec);
 //	hdrec = NULL;
	if(!rcd_close(rcd))
		return FALSE;
	return rcd_open(rcd, name, NULL);
badfld:
	rcd->err = RCDERR_BADFIELD;
	return FALSE;
}

BOOL rcd_open(lpRECORDSET rcd, char* name, char*filter)
{
char              hdr[32];
char              *hdrec;
long              recnum, offset, irec;
WORD              lhdr, lrec, lcontrol;
short             fldnum, i;
RCDFIELD          dbfld;
RCDRECINFO        ri;

	memset(rcd, 0, sizeof(RECORDSET));
	if(!init_buf(&rcd->bd, name, BUF_OLD)){
		rcd->err = RCDERR_OPEN;
		return FALSE;
	}
	init_vdim(&rcd->vd, sizeof(RCDRECINFO));
	if(load_data(&rcd->bd, 32, hdr) < 32) goto badfile;
//   
	hdrec = hdr;
	if((*hdrec & 0x03) != (0x03)) goto badfile;
	hdrec += 4; //  
	recnum = *((int*)hdrec); //   
	if(recnum < 0 ) goto badfile;
	hdrec += 4;
	lhdr 	= *((WORD*)hdrec); //   
	fldnum = (short)((lhdr - 33)/32);
	if(fldnum <= 0 || fldnum > 255) goto badfile;
	if((fldnum + 1)*32 + 1 != lhdr) goto badfile;
	hdrec += 2; //   
	lrec 	= *((WORD*)hdrec); //   
	if(lrec == 0 || lrec > 4000) goto badfile;
	if((ULONG)(lrec*recnum + lhdr + ((recnum) ? 1:0)) != rcd->bd.len_file) goto badfile;
	if(hdr[15] != 0) goto codefile;
	if((rcd->record = (char*)SGMalloc(lrec)) == NULL) goto nomem;
	if((rcd->fld = (RCDFIELD*)SGMalloc(fldnum*sizeof(RCDFIELD))) == NULL) goto nomem;
	//   ,   
	lcontrol = 1;
	for(i = 0; i < fldnum; i++){
		if(load_data(&rcd->bd, 32, hdr) != 32) goto badfile;
		if(hdr[10] != 0) goto badfile;
		if(strlen(hdr) == 0) goto badfile;
//		if(!test_to_fldname(hdr)) goto badfile;
		strcpy(dbfld.name, hdr);
		dbfld.type = hdr[11];
		dbfld.size = (UCHAR)hdr[16];
		dbfld.prec = (UCHAR)hdr[17];
		lcontrol += dbfld.size;
		if(lcontrol > lrec) goto badfile;
		if(dbfld.prec >= dbfld.size) goto badfile;
		if(dbfld.type != 'N' && dbfld.type != 'F' && dbfld.prec != 0) goto badfile;
		rcd->fld[i] = dbfld;
	}
	if(load_data(&rcd->bd, 1, &hdr) != 1) goto badfile;
	if(hdr[0] != 0x0D) goto badfile;
	//   
	rcd->fldnum = fldnum;
	rcd->recnum = recnum;
	rcd->recsize = lrec;
	//  vdim c  
	rcd->boffset = get_buf_offset(&rcd->bd);
	for(irec = 0; irec < rcd->recnum; irec++){
		offset = get_buf_offset(&rcd->bd);
		if(load_data(&rcd->bd, rcd->recsize, rcd->record) != rcd->recsize) goto badfile;
		if(rcd->record[0] == 0x20){ // 
			ri.bmk = irec + 1;
			ri.offset = offset;
			if(!add_elem(&rcd->vd, &ri)) goto nomem;
		}
	}
	if(recnum){
		if(load_data(&rcd->bd, 1, hdr) != 1) goto badfile;
		if(hdr[0] != 0x1A) goto badfile;
	}
	rcd->realnum = rcd->vd.num_elem;
	rcd->irec = 0;
	if(rcd->realnum)
		if(!rcd_load(rcd, 1))
			goto err;
	if(filter)
		if(!rcd_filter(rcd, filter))
			goto err;
	 return TRUE;

badfile:
	rcd->err = RCDERR_BADFILE;
	rcd_close(rcd);
	return FALSE;
codefile:
	rcd->err = RCDERR_CODEFILE;
	rcd_close(rcd);
	return FALSE;
nomem:
	rcd->err = RCDERR_NOMEM;
err:
	rcd_close(rcd);
	return FALSE;
}

BOOL rcd_open_by_id(lpRECORDSET rcd, char* name,
										char* fldnm, sgFloat* id, short numid)

{
char              hdr[32];
char              *hdrec;
long              recnum, offset, irec;
WORD              lhdr, lrec, lcontrol;
short             fldnum, i, fldpos, fldsize, fldid = -1;
sgFloat            num;
RCDFIELD          dbfld;
RCDRECINFO        ri;

	memset(rcd, 0, sizeof(RECORDSET));
	if(!init_buf(&rcd->bd, name, BUF_OLD)){
		rcd->err = RCDERR_OPEN;
		return FALSE;
	}
	init_vdim(&rcd->vd, sizeof(RCDRECINFO));
	if(load_data(&rcd->bd, 32, hdr) < 32) goto badfile;
//   
	hdrec = hdr;
	if((*hdrec & 0x03) != (0x03)) goto badfile;
	hdrec += 4; //  
	recnum = *((int*)hdrec); //   
	if(recnum < 0 ) goto badfile;
	hdrec += 4;
	lhdr 	= *((WORD*)hdrec); //   
	fldnum = (short)((lhdr - 33)/32);
	if(fldnum <= 0 || fldnum > 255) goto badfile;
	if((fldnum + 1)*32 + 1 != lhdr) goto badfile;
	hdrec += 2; //   
	lrec 	= *((WORD*)hdrec); //   
	if(lrec == 0 || lrec > 4000) goto badfile;
	if((ULONG)(lrec*recnum + lhdr + ((recnum) ? 1:0)) != rcd->bd.len_file) goto badfile;
	if(hdr[15] != 0) goto codefile;
	if((rcd->record = (char*)SGMalloc(lrec)) == NULL) goto nomem;
	if((rcd->fld = (RCDFIELD*)SGMalloc(fldnum*sizeof(RCDFIELD))) == NULL) goto nomem;
	//   ,   
	lcontrol = 1;
	for(i = 0; i < fldnum; i++){
		if(load_data(&rcd->bd, 32, hdr) != 32) goto badfile;
		if(hdr[10] != 0) goto badfile;
		if(strlen(hdr) == 0) goto badfile;
//		if(!test_to_fldname(hdr)) goto badfile;
		strcpy(dbfld.name, hdr);
		dbfld.type = hdr[11];
		dbfld.size = (UCHAR)hdr[16];
		dbfld.prec = (UCHAR)hdr[17];
		lcontrol += dbfld.size;
		if(lcontrol > lrec) goto badfile;
		if(dbfld.prec >= dbfld.size) goto badfile;
		if(dbfld.type != 'N' && dbfld.type != 'F' && dbfld.prec != 0) goto badfile;
		rcd->fld[i] = dbfld;
		if(fldid < 0){
			if((!strcmp(dbfld.name, fldnm)) && (dbfld.type == 'N')){
				fldid = i;
				fldsize = dbfld.size;
				fldpos = (short)(lcontrol - dbfld.size);
			}
		}
	}
	if(load_data(&rcd->bd, 1, &hdr) != 1) goto badfile;
	if(hdr[0] != 0x0D) goto badfile;
	//   
	rcd->fldnum = fldnum;
	rcd->recnum = recnum;
	rcd->recsize = lrec;
	//  vdim c  
	rcd->boffset = get_buf_offset(&rcd->bd);
	for(irec = 0; irec < rcd->recnum; irec++){
		offset = get_buf_offset(&rcd->bd);
		if(load_data(&rcd->bd, rcd->recsize, rcd->record) != rcd->recsize) goto badfile;
		if(rcd->record[0] != 0x20) continue; // 
		if(fldid < 0) continue;
		rcd->record[fldpos+fldsize] = 0;
		if(!z_text_to_sgFloat(&rcd->record[fldpos], &num, ((short*)&i))) continue;
		for(i = 0; i < numid; i++){
			if(id[i] == num)
				goto metadd;
		}
		continue;
metadd:
		ri.bmk = irec + 1;
		ri.offset = offset;
		if(!add_elem(&rcd->vd, &ri)) goto nomem;
	}
	if(recnum){
		if(load_data(&rcd->bd, 1, hdr) != 1) goto badfile;
		if(hdr[0] != 0x1A) goto badfile;
	}
	rcd->realnum = rcd->vd.num_elem;
	rcd->irec = 0;
	if(rcd->realnum)
		if(!rcd_load(rcd, 1))
			goto err;
	return TRUE;

badfile:
	rcd->err = RCDERR_BADFILE;
	rcd_close(rcd);
	return FALSE;
codefile:
	rcd->err = RCDERR_CODEFILE;
	rcd_close(rcd);
	return FALSE;
nomem:
	rcd->err = RCDERR_NOMEM;
err:
	rcd_close(rcd);
	return FALSE;
}


BOOL rcd_move(lpRECORDSET rcd, int rnum, RCDBOOKMARK bmk)
{
int       irec;
RCDRECINFO ri;

	rcd->newrec = 0;
	if(bmk != 0){  //  
		if(bmk > 0 && bmk <= rcd->recnum){
			for(irec = 1; irec <= rcd->realnum; irec++){
				if(!read_elem(&rcd->vd, irec - 1, &ri)){
					rcd->err = RCDERR_INTERNAL;
					return FALSE;
				}
				if(ri.bmk == bmk){
					if(!seek_buf(&rcd->bd, ri.offset))goto rerr;
					if(load_data(&rcd->bd, rcd->recsize, rcd->record) != rcd->recsize) goto rerr;
					rcd->irec = irec;
					rcd->bmk = ri.bmk;
					return TRUE;
				}
			}
		}
		rcd->err = RCDERR_BOOKMARK;
		return FALSE;
	}
	//  
	if((rcd_BOF(rcd) && rnum < 0) || (rcd_EOF(rcd) && rnum > 0)) {
		rcd->err = RCDERR_NOCURRPOS;
		return FALSE;
	}
	irec = rcd->irec + rnum;
	if(irec <= 0){
		rcd->irec = 0;
		rcd->bmk = 0;
		return TRUE;
	}
	if(irec > rcd->realnum){
		rcd->irec = rcd->realnum + 1;
		rcd->bmk = 0;
		return TRUE;
	}
	return rcd_load(rcd, irec);
rerr:
	rcd->err = RCDERR_REED;
	return FALSE;
}

static BOOL rcd_load(lpRECORDSET rcd, int irec)
{
RCDRECINFO ri;

	rcd->newrec = 0;
	if(irec <= 0 || irec > rcd->realnum) {
		rcd->err = RCDERR_NOCURRPOS;
		return FALSE;
	}
	if(!read_elem(&rcd->vd, irec - 1, &ri)){
		rcd->err = RCDERR_INTERNAL;
		return FALSE;
	}
	if(!seek_buf(&rcd->bd, ri.offset))goto rerr;
	if(load_data(&rcd->bd, rcd->recsize, rcd->record) != rcd->recsize) goto rerr;
	rcd->irec = irec;
	rcd->bmk = ri.bmk;
	return TRUE;
rerr:
	rcd->err = RCDERR_REED;
	return FALSE;
}

BOOL rcd_move_first(lpRECORDSET rcd)
{
	return rcd_load(rcd, 1);
}

BOOL rcd_move_last(lpRECORDSET rcd)
{
	return rcd_load(rcd, rcd->realnum);
}
BOOL rcd_move_next(lpRECORDSET rcd)
{
	return rcd_move(rcd, 1, 0);
}
BOOL rcd_move_previous(lpRECORDSET rcd)
{
	return rcd_move(rcd, -1, 0);
}

BOOL rcd_BOF(lpRECORDSET rcd){
	if(rcd->irec <= 0 || rcd->realnum == 0) return TRUE;
	return FALSE;
}

BOOL rcd_EOF(lpRECORDSET rcd){
	if(rcd->irec > rcd->realnum || rcd->realnum == 0) return TRUE;
	return FALSE;
}

RCDBOOKMARK rcd_bookmark(lpRECORDSET rcd){
	if(rcd->irec <= 0 || rcd->irec > rcd->realnum || rcd->realnum == 0){
		rcd->err = RCDERR_NOCURRPOS;
		return 0;
	}

	return rcd->bmk;
}


BOOL rcd_getvalue_bynum(lpRECORDSET rcd, short fieldnum, void * value)
{
char  buf[256];
short ifld, i, errcode;

	if(!rcd_bookmark(rcd) && !rcd->newrec) return FALSE;
	if(fieldnum < 0 || fieldnum >= rcd->fldnum){
		rcd->err = RCDERR_BADFIELDNUM;
		return FALSE;
	}
	ifld = 1;
	for(i = 0; i < fieldnum; i++)
		ifld += rcd->fld[i].size;
	memcpy(buf, &rcd->record[ifld], rcd->fld[fieldnum].size);
	buf[rcd->fld[fieldnum].size] = 0;
	switch(rcd->fld[fieldnum].type){
		case 'C':
		case 'c':
			z_OemToAnsi(buf, buf, rcd->fld[fieldnum].size);
			for(i = (short)(rcd->fld[fieldnum].size - 1); i >= 0; i--){
				if(buf[i] == 0x20) buf[i] = 0;
				else break;
			}
			strcpy((char*)value, buf);
			break;
		case 'N':
		case 'n':
			if(!z_text_to_sgFloat(buf, (sgFloat*)value, ((short*)&errcode))){
				if(errcode == 0) rcd->err = RCDERR_UNKFIELDVALUE;
				else             rcd->err = RCDERR_BADVALUE;
				return FALSE;
			}
			break;
		case 'L':
		case 'l':
			if(buf[0] == 'T' || buf[0] == 't' || buf[0] == 'Y' || buf[0] == 'y')
				*((sgFloat*)value) = 1;
			else
				*((sgFloat*)value) = 0;
			break;
		default:
			rcd->err = RCDERR_UNKFIELDTYPE;
			return FALSE;
	}
	return TRUE;
}

void * rcd_alloc_and_getvalue_bynum(lpRECORDSET rcd, short fieldnum)
{
void * value;
short     size;

	if(!rcd_bookmark(rcd) && !rcd->newrec) return NULL;
	if(fieldnum < 0 || fieldnum >= rcd->fldnum){
		rcd->err = RCDERR_BADFIELDNUM;
		return NULL;
	}
	switch(rcd->fld[fieldnum].type){
		case 'C':
		case 'c':
			size = (short)(rcd->fld[fieldnum].size + 1);
			break;
		case 'N':
		case 'n':
		case 'L':
		case 'l':
			size = sizeof(sgFloat);
			break;
		default:
			rcd->err = RCDERR_UNKFIELDTYPE;
			return NULL;
	}
	value = SGMalloc(size);
	if(value == NULL){
		 rcd->err = RCDERR_NOMEM;
		 return NULL;
	}
	if(!rcd_getvalue_bynum(rcd, fieldnum, value)){
		SGFree(value);
		return NULL;
	}
	return value;
}

BOOL rcd_setvalue_bynum(lpRECORDSET rcd, short fieldnum, void * value)
{
char   buf[256];
short  ifld, i, len;
sgFloat num;



	if(!rcd_bookmark(rcd) && !rcd->newrec) return FALSE;
	if(fieldnum < 0 || fieldnum >= rcd->fldnum){
		rcd->err = RCDERR_BADFIELDNUM;
		return FALSE;
	}
	ifld = 1;
	for(i = 0; i < fieldnum; i++)
		ifld += rcd->fld[i].size;

	memset(buf, 0x20, rcd->fld[fieldnum].size);

	buf[rcd->fld[fieldnum].size] = 0;
	if(value){
		switch(rcd->fld[fieldnum].type){
			case 'C':
			case 'c':
				len = (short)strlen((char*)value);
				if(len > rcd->fld[fieldnum].size) len = rcd->fld[fieldnum].size;
				strncpy(buf, (char*)value, len);
				z_AnsiToOem(buf, buf, len);
				break;
			case 'N':
			case 'n':
//nb				num = *((sgFloat*)value);
				sgFloat_output1(*((sgFloat*)value), rcd->fld[fieldnum].size,
																					rcd->fld[fieldnum].prec,
											 FALSE, FALSE, FALSE, buf);
				break;
			case 'L':
			case 'l':
				num = *((sgFloat*)value);
				buf[0] = (BYTE)((num != 0.) ? 'T':'F');
				break;
			default:
				rcd->err = RCDERR_UNKFIELDTYPE;
				return FALSE;
		}
	}
	memcpy(&rcd->record[ifld], buf, rcd->fld[fieldnum].size);
	return TRUE;
}

void rcd_addnew(lpRECORDSET rcd)
{
	rcd->newrec = 1;
	memset(rcd->record, ' ', rcd->recsize);
}

BOOL rcd_update(lpRECORDSET rcd)
{
int offset, irec;
char c;
RCDRECINFO ri;

	if(rcd->newrec){
		offset = rcd->boffset + rcd->recnum*rcd->recsize;
		if(!seek_buf(&rcd->bd, offset))
			goto rerr;
		if(!story_data(&rcd->bd, rcd->recsize, rcd->record))
			goto rerr;
		c = 0x1A;
		if(!story_data(&rcd->bd, 1, &c))
			goto rerr;
		irec = rcd->recnum + 1;
		if(!seek_buf(&rcd->bd, 4))
			goto rerr;
		if(!story_data(&rcd->bd, 4, &irec))
			goto rerr;
		ri.bmk = rcd->realnum+1;
		ri.offset = offset;
		if(!add_elem(&rcd->vd, &ri))
			goto nomem;
		rcd->recnum++;
		rcd->realnum++;
		rcd->newrec = 0;
		if(rcd_bookmark(rcd))
			return rcd_load(rcd, rcd->irec);
		return TRUE;
	}
	else {
		if(!rcd_bookmark(rcd))
			return FALSE;
		if(!read_elem(&rcd->vd, rcd->irec - 1, &ri)){
			rcd->err = RCDERR_INTERNAL;
			return FALSE;
		}

		if(!seek_buf(&rcd->bd, ri.offset))goto rerr;
		if(!story_data(&rcd->bd, rcd->recsize, rcd->record))
			goto rerr;
		return TRUE;
	}
rerr:
	rcd->err = RCDERR_WRITE;
	return FALSE;
nomem:
	rcd->err = RCDERR_NOMEM;
	return FALSE;
}



BOOL rcd_delete(lpRECORDSET rcd)
{
int i;
char c = '*';
RCDRECINFO ri;

	if(!rcd_bookmark(rcd))
		return FALSE;
	if(!read_elem(&rcd->vd, rcd->irec - 1, &ri)){
		rcd->err = RCDERR_INTERNAL;
		return FALSE;
	}
	if(!seek_buf(&rcd->bd, ri.offset))
		goto rerr;
	if(!story_data(&rcd->bd, 1, &c))
		goto rerr;
	for(i = rcd->irec; i < rcd->vd.num_elem; i++){
		if(!read_elem(&rcd->vd, i, &ri)){
			rcd->err = RCDERR_INTERNAL;
			return FALSE;
		}
		if(!write_elem(&rcd->vd, i - 1, &ri)){
			rcd->err = RCDERR_INTERNAL;
			return FALSE;
		}
	}
	delete_elem_list(&rcd->vd, rcd->realnum - 1);
	rcd->realnum--;

	if(!rcd->realnum){
		rcd->irec = 0;
		rcd->bmk = 0;
		return TRUE;
	}
	if(rcd->irec > rcd->realnum){
		rcd->irec = rcd->realnum + 1;
		rcd->bmk = 0;
		return TRUE;
	}
	return rcd_load(rcd, rcd->irec);
rerr:
	rcd->err = RCDERR_WRITE;
	return FALSE;
//nb nomem:
//	rcd->err = RCDERR_NOMEM;
//	return FALSE;
}

BOOL rcd_getfieldnum(lpRECORDSET rcd, char *fieldname, short* fieldnum)
{
short i;

	for(i = 0; i < rcd->fldnum; i++){
		if(!strcmp(rcd->fld[i].name, fieldname)){
			*fieldnum = i;
			return TRUE;
		}
	}
	rcd->err = RCDERR_BADFIELDNAME;
	return FALSE;
}


BOOL rcd_getvalue(lpRECORDSET rcd, char *fieldname, void * value)
{
short fieldnum;
	if(!rcd_getfieldnum(rcd, fieldname, &fieldnum)) return FALSE;
	return rcd_getvalue_bynum(rcd, fieldnum, value);
}

BOOL rcd_setvalue(lpRECORDSET rcd, char *fieldname, void * value)
{
short fieldnum;
	if(!rcd_getfieldnum(rcd, fieldname, &fieldnum)) return FALSE;
	return rcd_setvalue_bynum(rcd, fieldnum, value);
}


void * rcd_alloc_and_getvalue(lpRECORDSET rcd, char *fieldname)
{
short fieldnum;
	if(!rcd_getfieldnum(rcd, fieldname, &fieldnum)) return NULL;
	return rcd_alloc_and_getvalue_bynum(rcd, fieldnum);
}

short rcd_fields_count(lpRECORDSET rcd)
{
	return rcd->fldnum;
}

int rcd_records_count(lpRECORDSET rcd)
{
	return rcd->realnum;
}

BOOL rcd_field_info(lpRECORDSET rcd, short fieldnum, lpRCDFIELD fieldinfo)
{
	if(fieldnum < 0 || fieldnum >= rcd->fldnum){
		rcd->err = RCDERR_BADFIELDNUM;
		return FALSE;
	}
	*fieldinfo = rcd->fld[fieldnum];
	return TRUE;
}

BOOL rcd_close(lpRECORDSET rcd)
//   
{
	if(rcd->fld) SGFree(rcd->fld);
	rcd->fld = NULL;
	if(rcd->record) SGFree(rcd->record);
	rcd->record = NULL;
	if(rcd->filter) SGFree(rcd->filter);
	rcd->filter = NULL;
	free_vdim(&rcd->vd);
	if(!close_buf(&rcd->bd)){
		rcd->err = RCDERR_CLOSE;
		return FALSE;
	}
	return TRUE;
}



void rcd_put_message(lpRECORDSET rcd)
{
	put_message(MSG_RCDERR_NOERR + rcd->err, NULL, 0);
}

static BOOL rcd_find(lpRECORDSET rcd, char *str, int sirec, int eirec, short step)
{
BOOL res;
int irec;

	rcd->newrec = 0;
	rcd->nomatch = 0;
	irec = rcd->irec;
	if(!rcd->realnum)
		goto nomatch;
	if(sirec < eirec && step < 0)
		goto nomatch;
	if(sirec > eirec && step > 0)
		goto nomatch;

	do {
		if(sirec != rcd->irec)
			if(!rcd_load(rcd, sirec))
				return FALSE;
		if(!rcd_test_expression(rcd, str, &res))
			return FALSE;
		if(res)
			return TRUE;
		sirec = sirec + step;
	}	while(sirec != eirec + step);
nomatch:
	rcd->nomatch = 1;
	if(irec <= 0){
		rcd->irec = 0;
		rcd->bmk = 0;
		return TRUE;
	}
	if(irec > rcd->realnum){
		rcd->irec = rcd->realnum + 1;
		rcd->bmk = 0;
		return TRUE;
	}
	return rcd_load(rcd, irec);
}

BOOL rcd_find_first(lpRECORDSET rcd, char *str)
{
	return rcd_find(rcd, str, 1, rcd->realnum, 1);
}

BOOL rcd_find_next(lpRECORDSET rcd, char *str)
{
	return rcd_find(rcd, str, rcd->irec + 1, rcd->realnum, 1);
}

BOOL rcd_find_previous(lpRECORDSET rcd, char *str)
{
	return rcd_find(rcd, str, rcd->irec - 1, 1, -1);
}

BOOL rcd_find_last(lpRECORDSET rcd, char *str)
{
	return rcd_find(rcd, str, rcd->realnum, 1, -1);
}

BOOL rcd_nomatch(lpRECORDSET rcd)
{
	return (rcd->nomatch) ? TRUE:FALSE;
}

BOOL rcd_filter(lpRECORDSET rcd, char *str)
{
short      l;
BOOL       res, oldfilter = FALSE;
int       irec, irecf, offset;
char       hdr;
RCDRECINFO ri;
	rcd->newrec = 0;

	if(rcd->filter){
		oldfilter = TRUE;
		SGFree(rcd->filter);
		rcd->filter = NULL;
	}
	if(str != NULL){
		l = (short)strlen(str);
		if(l != 0){
			if((rcd->filter = (char*)SGMalloc(l + 1)) == NULL) goto merr;
			strcpy(rcd->filter, str);
		}
	}
	if(oldfilter){
		free_vdim(&rcd->vd);
		init_vdim(&rcd->vd, sizeof(RCDRECINFO));
		if(!seek_buf(&rcd->bd, rcd->boffset))goto rerr;
		for(irec = 0; irec < rcd->recnum; irec++){
			offset = get_buf_offset(&rcd->bd);
			if(load_data(&rcd->bd, rcd->recsize, rcd->record) != rcd->recsize) goto badfile;
			if(rcd->record[0] == 0x20){ // 
				ri.bmk = irec + 1;
				ri.offset = offset;
				if(!add_elem(&rcd->vd, &ri)) goto merr;
			}
		}
		if(rcd->recnum){
			if(load_data(&rcd->bd, 1, &hdr) != 1) goto badfile;
			if(hdr != 0x1A) goto badfile;
		}
		rcd->realnum = rcd->vd.num_elem;
		rcd->irec = 0;
		rcd->bmk = 0;
	}
	if(!rcd->realnum)
		return TRUE;
	if(!rcd->filter)
		return rcd_move_first(rcd);
	//     
	irecf = 0;
	for(irec = 0; irec < rcd->realnum; irec++){
		if(!rcd_load(rcd, irec + 1))
			goto err;
		if(!rcd_test_expression(rcd, rcd->filter, &res))
			goto err;
		if(res){
			if(irecf != irec){
				if(!read_elem(&rcd->vd, irec, &ri))
					goto ierr;
				if(!write_elem(&rcd->vd, irecf, &ri))
					goto ierr;
			}
			irecf++;
		}
	}
	rcd->irec = 0;
	if(irecf < rcd->realnum){
		delete_elem_list(&rcd->vd, irecf);
		rcd->realnum = rcd->vd.num_elem;
		rcd->irec = 0;
		rcd->bmk = 0;
		if(!rcd->realnum) return TRUE;
	}
	return rcd_move_first(rcd);
badfile:
	rcd->err = RCDERR_BADFILE;
	return FALSE;
merr:
	rcd->err = RCDERR_NOMEM;
	return FALSE;
ierr:
	rcd->err = RCDERR_INTERNAL;
	return FALSE;
rerr:
	rcd->err = RCDERR_REED;
err:
	return FALSE;
}

BOOL rcd_test_expression(lpRECORDSET rcd, char *express, BOOL *res)
  
{
/*int            ibegvar, ibegarr, ibeglvar, ibeglarr, iglb;
ULONG          len;
UCHAR          type, s;
void        *value;
lpBOOL_VALUE   bv;
BOOL           ret = FALSE;
short            i;

	*res = FALSE;

//   
	ibegvar = ucd->vd.num_elem;
	ibegarr = ucd->vda.num_elem;
	ibeglvar = ucd->vdl.num_elem;
	ibeglarr = ucd->vdla.num_elem;
	iglb = ucd->vdglb.num_elem;

//     
	for(i = 0; i < rcd->fldnum; i++){
		value = rcd_alloc_and_getvalue_bynum(rcd, (short)i);
		if(!value){
			if(rcd->err != RCDERR_UNKFIELDVALUE)
				goto err;
			continue;
		}
		switch(rcd->fld[i].type){
			case 'C':
			case 'c':
				type = UC_STRING;
				break;
			case 'N':
			case 'n':
			case 'L':
			case 'l':
				type = UC_NUM;
				break;
			default:
				continue;
		}
		if(!uc_assign_variable(rcd->fld[i].name, type, value)){
			rcd->err = RCDERR_CALCULATION;
			goto err;
		}
		SGFree(value);
 //nb 		value = NULL;
	}
//nb met:
	value = uc_alloc_and_get_express_value_s(express, &type, &len, &s);
	if(!value) {
		if(ucd->err == UC_NODEFINE) goto metend;
		rcd->err = RCDERR_SYNTAX;
		goto err;
	}
	if(s != UC_NORMAL){
		put_message(ERR_LOGICAL_EXPRESSION, NULL, 0);
		rcd->err = RCDERR_SYNTAX;
		goto err;
	}
	switch(type){
		case UC_NUM:
			if(*((sgFloat*)value) != 0.) *res = TRUE;
			break;
		case UC_BOOL:
			bv = (lpBOOL_VALUE)value;
			if(bv->b) *res = TRUE;
			break;
		default:
			rcd->err = RCDERR_SYNTAX;
			goto err;
	}
metend:
	ret = TRUE;
err:
	if(value)	SGFree(value);
	uc_free_var_list(ibegvar, ibegarr);
	uc_free_loc_var_list(ibeglvar, ibeglarr, iglb);
	return ret;*/return FALSE;
}


void GetCurrentDateForDbf(char* sd)
{
struct tm 	*newtime;
time_t 			long_time;

	tzset();
	long_time = GetCurrentDateTime();
	newtime = localtime( &long_time );
  sd[0] = (char)newtime->tm_year;
  sd[1] = (char)newtime->tm_mon + 1;
  sd[2] = (char)newtime->tm_mday;
  return;
}
