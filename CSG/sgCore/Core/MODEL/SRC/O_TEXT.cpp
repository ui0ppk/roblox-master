#include "../../sg.h"

#pragma warning(disable:4018)

static void get_next_txy(short dx, short dy,
												 sgFloat gdx, sgFloat gdy, sgFloat sx,
												 void *d);
static char* get_sym_form(lpFONT font, WORD sym);
void ft_alloc_mem(void);
static UCHAR* get_real_txt_sym(UCHAR *c, WORD *sym, BOOL *up, BOOL *dn,
															 BOOL *sps);
WORD tsym_OemToAnsi(WORD sym);
static BOOL draw_sym1(lpFONT font, WORD sym,
										  BOOL vstatus, sgFloat gdx, sgFloat gdy, sgFloat sx,
											lpD_POINT p,
											BOOL(*sym_line)(lpD_POINT pb, lpD_POINT pe, void* user_data),
											void* user_data);
static BOOL draw_shp(lpFONT font, WORD sym,
										 BOOL vstatus, sgFloat gdx, sgFloat gdy, sgFloat sx,
										 lpD_POINT p,
										 BOOL(*sym_line)(lpD_POINT pb, lpD_POINT pe, void* user_data),
										 void* user_data);
static BOOL place_ft_value(lpFT_ITEM ft_item, void  *value, ULONG len);
static BOOL ft_bd_sb_mus(void);
static void free_ft_value(lpFT_ITEM attr_item);
static BOOL faf_sym_line(lpD_POINT pb, lpD_POINT pe,
												 BOOL (*sym_line)(lpD_POINT pb, lpD_POINT pe, void* user_data),
												 void* user_data);
static void set_max_sym_len(lpFONT font);
static BOOL load_inline_shp(FTTYPE fttype, void* shp);
BOOL sym_line_geo(lpD_POINT pb, lpD_POINT pe, void* user_data);
BOOL idle_sym_line(lpD_POINT pb, lpD_POINT pe, void* user_data);
IDENT_V insert_ft_value(FTTYPE fttype, IDENT_V bid, void* value, ULONG len);
BOOL ftbd_after_replase(void);

struct u_d_t {
	lpGEO_TEXT geo;
	BOOL (*sym_line)(lpD_POINT pb, lpD_POINT pe, void* user_data);
	void* user_data;
};

TSTYLE  style_for_init = {0, 5., 100., 0., 0., 50.};
lpFT_BD ft_bd = NULL;
UCHAR   faf_flag = 0;
char*   tmp_text_str = NULL;


void init_draft_info(/*const char* applpath, const char* applname*/)
//      
{
short   errcode;
//char  path[MAXPATH+MAXEXT];

	// 
/*	strncpy(path, applpath, MAXPATH);
	path[MAXPATH - 1] = 0;
	strncpy(ft_bd->exe_path, applpath, MAXPATH);
	ft_bd->exe_path[MAXPATH - 1] = 0;
	//strncat(path, applname, MAXPATH - strlen(path) - 1);
	strncat(path, "ssg", MAXPATH - strlen(path) - 1);
	strncat(path, FONT_EXT, MAXPATH - strlen(path) - 1);
	ft_bd->def_font = set_font(path, &errcode);
	if(!ft_bd->def_font){
		return; // Tlx -  : "?"
	}
	ft_bd->curr_font = ft_bd->def_font;*/

	// draft_info
	 
   //         -   DTI

	/*strncpy(ft_bd->draft_path, applpath, MAXPATH);
	ft_bd->draft_path[MAXPATH - 1] = 0;
	strncat(ft_bd->draft_path, applname, MAXPATH - strlen(ft_bd->draft_path) - 1);
	strncat(ft_bd->draft_path, DRAFT_EXT, MAXPATH - strlen(ft_bd->draft_path) - 1);*/
	load_draft_info(&errcode);

	//   
	dim_info_init();
}

GS_CODE get_text_geo_par(lpGEO_TEXT gtext)
//     
{
	GS_CODE  ret=G_OK;
	INDEX_KW keys[] = {0, 0, KW_UNKNOWN};
	D_POINT  p[10];
	D_POINT  gp;
	char     dx[] = {0, 0, 0, 1, 2, 2, 2, 1, 1};
	char     dy[] = {0, 1, 2, 2, 2, 1, 0, 0, 1};
	short    flags[3];
	short    i;
	MATR     m;
	IDENT_V  curr_font;
	lpFONT   font = NULL;
	UCHAR    *text;
	ULONG    len;
	RTT_UI   ud;
	UCHAR    comflg = 0;


	memset(&gcond, 0, sizeof(GEO_COND));
	gcond.flags = flags;
	gcond.p     = p;
	gcond.m[0]  = gtext->matr;
	gcond.m[1]  = m;
	gcond.user  = &ud;

	curr_font = 0;
	if((text = (UCHAR*)alloc_and_get_ft_value(gtext->text_id, &len)) == NULL){
		ret = G_CANCEL;
	}
	ud.text = text;

	//    
	flags[1] = 0;

	o_hcunit(gtext->matr);
	gtext->min.x = gtext->min.y = gtext->min.z =  GAB_NO_DEF;
	gtext->max.x = gtext->max.y = gtext->max.z = -GAB_NO_DEF;

//	if(!draw_geo_text(gtext, text_gab_lsk, &gtext->min)) return G_CANCEL;
/*	if(gtext->min.x == GAB_NO_DEF) {
		put_message(EMPTY_TEXT_MSG, NULL, 0);
		return G_CANCEL;
	}*/

	p[3] = gtext->min;
	p[4] = gtext->min;
	p[5] = gtext->max;
	dpoint_sub(&p[5], &p[4], &gp);

	p[6] = gtext->min;
	p[7] = gtext->min; p[7].x = gtext->max.x;
	p[8] = gtext->max;
	p[9] = gtext->max; p[9].x = gtext->min.x;

	i = flags[1];
	p[3].x = p[4].x + 0.5*dx[i]*gp.x;
	p[3].y = p[4].y + 0.5*dy[i]*gp.y;
	flags[0] = 1;
	if(curr_font != gtext->font_id){
		if(font) SGFree(font);
		if((font = (FONT*)alloc_and_get_ft_value(gtext->font_id, &len)) == NULL){
			return G_CANCEL;
		}
		curr_font = gtext->font_id;
	}
	ud.font = font;
	ud.style = &(gtext->style);

	if(font) SGFree(font);
	if(text) SGFree(text);
	return ret;
}




void ft_alloc_mem(void)
//          
// !!!    WinMain  InitsBeforeMain() 

{
short i;

	if(0 ==(ft_bd = (FT_BD*)SGMalloc(sizeof(FT_BD)))) goto err;
	init_vdim(&ft_bd->vd,  sizeof(FT_ITEM));
	init_vld(&ft_bd->vld);
	for (i = 0; i < FTNUMTYPES; i++)
		il_init_listh(&ft_bd->ft_listh[i]);
	il_init_listh(&ft_bd->free_listh);
	ft_bd->free_space = 0;
	ft_bd->max_len    = 0;
	ft_bd->def_font = ft_bd->curr_font = 0;
	ft_bd->curr_style = style_for_init;
	strcpy(ft_bd->draft_title, DRAFT_TITLE);
	if((ft_bd->buf = (char*)SGMalloc(MAX_SHP_LEN)) == NULL) goto err;
//    
	if((dim_info = (DIMINFO*)SGMalloc(sizeof(DIMINFO))) == NULL) goto err;
	return;
err:
	if(ft_bd) SGFree(ft_bd);
  FatalStartupError(1);
}


IDENT_V set_font(char *name, short *errcode)

{
char       drive [MAXDRIVE];
char       dir   [MAXDIR];
char       file  [MAXFILE];
char       ext   [MAXEXT];
char       fname [MAXFILE + MAXEXT];
char       addname[MAXPATH];
char*      lname;
lpFONT     font = NULL;
IDENT_V    id;
ULONG      len;

	fnsplit(name, drive, dir, file, ext);
	strcpy(fname, file);
	strcat(fname, ext);
	//    
	//id = ft_bd->ft_listh[FTTEXT].head;
	id = ft_bd->ft_listh[FTFONT].head;

	while(id){
		if(0 ==(font = (FONT *)alloc_and_get_ft_value(id, &len))) goto err3;
		if(!strcmp(fname, font->name)){
			SGFree(font);
			return id;
		}
		SGFree(font); font = NULL;
		if(!il_get_next_item(&ft_bd->vd, id, &id)) attr_exit();
	}
	//    -  
	if(strlen(drive) || strlen(dir)){
	//   -   
		if (nb_access(name, 4) == -1) goto err6;
		lname = name;
	}
	else{
		//    
		lname = fname;
		if(nb_access(fname, 4) != -1) goto met_load;
		if(ft_bd->font_path[0]){
			//     
			fnsplit(ft_bd->font_path, drive, dir, file, ext);
			if(strlen(drive) || strlen(dir)){
				strcpy(addname, drive);
				strcat(addname, dir);
				strcat(addname, fname);
				lname = addname;
				if(nb_access(addname, 4) != -1) goto met_load;
			}
		}
		if(ft_bd->exe_path[0]){
			//   ,   exe
			fnsplit(ft_bd->exe_path, drive, dir, file, ext);
			if(strlen(drive) || strlen(dir)){
				strcpy(addname, drive);
				strcat(addname, dir);
				strcat(addname, fname);
				lname = addname;
				if(nb_access(addname, 4) != -1) goto met_load;
			}
		}
		goto err6;
	}
met_load:
	if(0 ==(font = load_font(lname, &len, NULL, 0, errcode))) goto err;

	if(!(id = add_ft_value(FTFONT, font, len))) goto err7;
	SGFree(font);
	return id;
err3:
	*errcode = 3;
	goto err;
err6:
	*errcode = 6;
	goto err;
err7:
	*errcode = 7;
err:
	if(font) SGFree(font);
	return 0;
}

lpFONT load_font(char *name, ULONG *lenfont, char *comment, short commlen,
								 short *errcode)

{

char          drive[MAXDRIVE];
char          dir  [MAXDIR];
char          file [MAXFILE];
char          ext  [MAXEXT];

BUFFER_DAT    bd;
char          tytle1[] = {"AutoCAD-86 shapes 1.0\x0D\x0A\x1A"};
char          buf[30];
SHORT	        fdata[3];
UCHAR         *tab, *c_out;
WORD          *itab;
lpFONT        fontt = NULL;
lpFONT        font  = NULL;
short         len, rlen = 0, tablen, sdw, hran_len, i;

	if(!init_buf(&bd, name, BUF_OLD)){
		*errcode = 2;
		return NULL;
	}
	len = strlen(tytle1);
	if(bd.len_file < len + sizeof(fdata))      goto err1; //  
	if(load_data(&bd, len, buf) != len)        goto err2; //  
	rlen += len;
	if(memcmp(buf, tytle1, len)){
		buf[20] = '0';
		if(memcmp(buf, tytle1, len))             goto err1;
	}
	if(load_data(&bd, sizeof(fdata), fdata) !=
		 sizeof(fdata))                          goto err2;
	rlen += sizeof(fdata);
	if(fdata[0] < 0 || fdata[1] <= 0 || fdata[2] <= 0 ||
		 fdata[0] > fdata[1])                    goto err1;
	//   
	tablen = 4*fdata[2];
	if(bd.len_file - rlen < tablen)             goto err1;
	if(0 ==(fontt = (FONT *)SGMalloc(sizeof(FONT) + tablen))) goto err3; // 
//  tab = ((UCHAR*)fontt) + (sizeof(FONT) - 1);
	tab = &(fontt->table[0]);
	if(load_data(&bd, tablen, tab) != tablen)   goto err2;
	rlen += tablen;

	//        
	//   
	itab = (WORD*)tab;
//nb	c_out = tab;
	sdw = (WORD)(4*fdata[2]);        //   
	hran_len = 0;                    //   
	for(i = 0; i < 2*fdata[2]; i += 2){
		if((len = itab[i + 1]) > 2000)              goto err1;
		itab[i + 1] = sdw + hran_len;
		if((int)sdw + hran_len + sizeof(FONT) - 1 >= 2L*MAXSHORT)
			goto err5; //   
		hran_len += len;
	}
	if(bd.len_file - rlen < hran_len + 3)         goto err1;
	*lenfont = sdw + hran_len + sizeof(FONT) - 1;
	if(0 ==(font = (FONT *)SGMalloc(*lenfont)))
		goto err3;
	memcpy(font, fontt, sizeof(FONT) + sdw);
	SGFree(fontt); fontt = NULL;
	//tab = ((UCHAR*)font) + (sizeof(FONT) - 1);
  tab = &(font->table[0]);
	if(load_data(&bd, hran_len, tab + sdw) != hran_len)  goto err2;
	if(fdata[0] == 0){ //  0-     
		if(fdata[2] == 1)                           goto err4; //  
		c_out = tab + *((WORD*)(tab + 2));
		len = strlen((char*)c_out);
		if(comment){
			if(len < commlen - 1) commlen = len + 1;
			strncpy(comment, (char*)c_out, commlen - 1);
			comment[commlen - 1] = 0;
		}
		c_out += (len + 1);
		font->num_up = *(c_out++);
		if(!font->num_up)                           goto err1;
		font->num_dn = *(c_out++);
		font->status = *(c_out++);
		if(*c_out)                                  goto err1;
	}
	else{
		font->num_up = font->num_dn = font->status = 0;
	}
	font->beg_sym = fdata[0];
	font->end_sym = fdata[1];
	font->num_sym = fdata[2];

	fnsplit(name, drive, dir, file, ext);
	strcpy(font->name, file);
	strcat(font->name, ext);
	close_buf(&bd);
	set_max_sym_len(font);
	return font;
err1:
	*errcode = 1;
	goto err;
err2:
	*errcode = 2;
	goto err;
err3:
	*errcode = 3;
	goto err;
err4:
	*errcode = 4;
	goto err;
err5:
	*errcode = 5;
err:
	close_buf(&bd);
	if(font)  SGFree(font);
	if(fontt) SGFree(fontt);
	*lenfont = 0;
	return NULL;
}

static void set_max_sym_len(lpFONT font)
//     
{
WORD     *tab;
WORD     i, sym;
sgFloat   d = 0., dx;
D_POINT  p, p0 = {0., 0., 0.};

	if(!font->num_up){
		font->lsym = 0.;
		return;
	}
	tab = (WORD*)font->table;
	for(i = 0; i < font->num_sym; i++, tab += 2){
		sym = tsym_OemToAnsi((WORD)*tab);
		if(sym == 0) continue;
		p = p0;
		if(draw_sym1(font, sym, FALSE, 1., 1., 0., &p, idle_sym_line, NULL)){
			dx = p.x - p0.x;
			if(dx > 0) d += dx;
		}
	}
	d = d/font->num_sym;
	font->lsym = d/font->num_up;
}

BOOL draw_geo_text(lpGEO_TEXT geo,
					 BOOL (*sym_line)(lpD_POINT pb, lpD_POINT pe, void* user_data),
					 void* user_data)
  
{
struct       u_d_t ud;
lpFONT       font;
UCHAR        *text = NULL;
BOOL         ret;
ULONG        len;

	if(!geo->font_id) return FALSE;
	if(!geo->text_id){
		if(!tmp_text_str) return FALSE;
		text = (UCHAR*)tmp_text_str;
	}
	ud.geo       = geo;
	ud.sym_line  = sym_line;
	ud.user_data = user_data;

	if(0 ==(font = (FONT *)alloc_and_get_ft_value(geo->font_id, &len))) return FALSE;
	if(!text){
		if(0 ==(text = (UCHAR*)alloc_and_get_ft_value(geo->text_id, &len))) {
			SGFree(font);
			return FALSE;
		}
	}
	ret = draw_text(font, &geo->style, text, sym_line_geo, &ud);
	if(geo->text_id) SGFree(text);
	SGFree(font);
	return ret;
}

BOOL sym_line_geo(lpD_POINT pb, lpD_POINT pe, void* user_data){
struct u_d_t *ud = (struct u_d_t*)user_data;
D_POINT wpb, wpe;
	o_hcncrd(ud->geo->matr, pb, &wpb);
	o_hcncrd(ud->geo->matr, pe, &wpe);
	return ud->sym_line(&wpb, &wpe, ud->user_data);
}

#pragma argsused
BOOL idle_sym_line(lpD_POINT pb, lpD_POINT pe, void* user_data){
	return TRUE;
}

BOOL draw_text(lpFONT font, lpTSTYLE style, UCHAR *text,
					 BOOL (*sym_line)(lpD_POINT pb, lpD_POINT pe, void* user_data),
					 void* user_data)
{
BOOL    vstatus, flag = FALSE;
sgFloat  gdx, gdy, sx, ds, dt, xbeg, ybeg, a, height;
D_POINT p = {0., 0., 0.};

	//       
	height = style->height*get_grf_coeff();
	if(font->num_up)
		gdy = height/font->num_up;
	else
		gdy = height;
	gdx = gdy*(style->scale/100);
	a = ((90 - style->angle)*M_PI)/180;
	sx  = 1./tan(a);
	vstatus = (style->status & DRAW_TEXT_VERT) ? TRUE : FALSE;
	if(!font->status) vstatus = FALSE;
	ds = (style->dhor*height)/100;
	if(vstatus)
		dt = gdx*font->num_up + (style->dver*height)/100;
	else
		dt = gdy*(font->num_up + font->num_dn) + (style->dver*height)/100;
	xbeg = p.x;
	ybeg = p.y;
	if(*text == ML_S_C) {
		flag = TRUE;
		text++;
	}
	while(*text != ML_S_C){
		//   
		if(!draw_string(font, text, style->status, gdx, gdy, sx, ds, &p, sym_line,
										user_data)) return FALSE;
		//  
		if(vstatus){
			p.x += dt;
			p.y = ybeg;
		}
		else{
			p.y -= dt;
			p.x = xbeg;
		}
		if(!flag) break;
		text += (strlen((char*)text) + 1);
	}
	return TRUE;
}

BOOL draw_string(lpFONT font, UCHAR *str,
								 UCHAR status, sgFloat gdx, sgFloat gdy, sgFloat sx, sgFloat dh,
								 lpD_POINT p,
								 BOOL(*sym_line)(lpD_POINT pb, lpD_POINT pe, void* user_data),
								 void* user_data)
//   
{
sgFloat    xbeg, xavg, xsdv, cf;
UCHAR*    c = str;
BOOL      up = FALSE, dn = FALSE, spsflag;
WORD      sym;
D_POINT   pold, pb, pe;
D_PLPOINT dp;
BOOL      vstatus, fstatus;

	vstatus = (status & DRAW_TEXT_VERT) ? TRUE : FALSE;
	fstatus = (status & TEXT_FIXED_STEP) ? TRUE : FALSE;
	if(!font->num_up) fstatus = FALSE;

	xavg = gdx*(font->lsym)*(font->num_up);
	xbeg = p->x;

	while(*c){
		c = get_real_txt_sym(c, &sym, &up, &dn, &spsflag);
		cf = 1.;
		if(spsflag){ // 
			pold = *p;
			if(fstatus && (!vstatus)){
				if(!draw_shp(font, sym, vstatus, gdx, gdy, sx, p, idle_sym_line, NULL))
					return FALSE;
				xsdv = p->x - pold.x;
				if(xsdv > xavg) cf = xavg/xsdv;
				*p = pold;
			}
			if(!draw_shp(font, sym, vstatus, gdx*cf, gdy, sx, p, sym_line, user_data))
				return FALSE;
		}
		else{
			if(!sym){	c++; continue;}
			if(sym == 13){c++; p->x = xbeg; continue;}
			pold = *p;
			if(fstatus && (!vstatus)){
				if(!draw_sym1(font, sym, vstatus, gdx, gdy, sx, p, idle_sym_line, NULL))
					return FALSE;
				xsdv = p->x - pold.x;
				if(xsdv > xavg) cf = xavg/xsdv;
				*p = pold;
			}
			if(!draw_sym1(font, sym, vstatus, gdx*cf, gdy, sx, p, sym_line, user_data))
				return FALSE;
		}
		if(vstatus){ //   
			p->y -= dh;
			p->x  = xbeg;
		}
		else if(fstatus){
			p->x = pold.x + xavg + dh;
		}
		else{
		 p->x += dh;
		}

		if(up){  //
			pb = pold; pe = *p;
			get_next_txy(0, font->num_up + 2, gdx*cf, gdy, sx*cf, &dp);
			pb.x += dp.x; pb.y += dp.y;
			pe.x += dp.x; pe.y += dp.y;
			if(!faf_sym_line(&pb, &pe, sym_line, user_data)) return FALSE;
		}
		if(dn){  //
			pb = pold; pe =*p;
			get_next_txy(0, -font->num_dn, gdx*cf, gdy, sx*cf, &dp);
			pb.x += dp.x; pb.y += dp.y;
			pe.x += dp.x; pe.y += dp.y;
			if(!faf_sym_line(&pb, &pe, sym_line, user_data)) return FALSE;
		}
		c++;
	}
	return TRUE;
}

static UCHAR* get_real_txt_sym(UCHAR *c, WORD *sym, BOOL *up, BOOL *dn,
															 BOOL *sps)
//      
{
char s[4] = {'%','%', 0};
UCHAR ss[6] = {'d','p','c','s','o','u'};
UCHAR *c1;
short  i;

	*sps = FALSE;
	*sym = 0;
	if(!strncmp((char*)c, s, 2)){// 
		if(*(c+2) == '%'){ // 
			*sym = (WORD)'%';
			return c + 2;
		}
		for(i = 0; i < 3; i++){//   
			if(*(c+2) == ss[i]){
				*sym = i;
				*sps = TRUE;
				return c + 2;
			}
		}
		if(*(c+2) == ss[3]){//   
			c1 = c + 3;
			i = 0;
			while(*c1 >= '0' && *c1 <= '9' && i < 3){
				*sym = *sym*10 + *c1 - '0';
				c1++; i++;
			}
			*sps = TRUE;
			return c1 - 1;
		}
		if(*(c+2) == ss[4]){ //
			*up = !*up;
			return c + 2;
		}
		if(*(c+2) == ss[5]){ //
			*dn = !*dn;
			return c + 2;
		}
		c1 = c + 2;
		i = 0;
		while(*c1 >= '0' && *c1 <= '9' && i < 3){
			*sym = *sym*10 + *c1 - '0';
			c1++; i++;
		}
		if(*sym != 0){
			*sym = tsym_OemToAnsi(*sym);
			return c1 - 1;
		}
	}
	*sym = *c;
	return c;
}

WORD tsym_OemToAnsi(WORD sym)
{
UCHAR csym;

	if(sym < 256){
		csym = (UCHAR)sym;
		z_OemToAnsi((char*)&csym, (char*)&csym, 1);
		sym = csym;
	}
	return sym;
}


static BOOL draw_sym1(
					 lpFONT font, WORD sym,
					 BOOL vstatus, sgFloat gdx, sgFloat gdy, sgFloat sx,
					 lpD_POINT p,
					 BOOL(*sym_line)(lpD_POINT pb, lpD_POINT pe, void* user_data),
					 void* user_data)
//   
{
static  char dx[]={2, 2, 2, 1, 0,-1,-2,-2,-2,-2,-2,-1, 0, 1, 2, 2};
static  char dy[]={0, 1, 2, 2, 2, 2, 2, 1, 0,-1,-2,-2,-2,-2,-2,-1};
char*   form;
UCHAR   code;
BOOL    pen = TRUE, dflag;
short   b, c;
short   lstc = 0;
D_POINT ps[4], pb, pe, pb1, pe1;



	if(!sym) return TRUE;
	form = get_sym_form(font, sym);
	if(!form) return TRUE;
	pb = *p;
	pb.z = pe.z = 0.;
	while(TRUE){
		dflag = TRUE;
met_next:
		code = (UCHAR)(*form);
		switch(code){
			case 0:     //   
				*p = pb;
				return TRUE;
			case 1:     //  
				if(dflag) pen = TRUE;
				break;
			case 2:
				if(dflag) pen = FALSE;
				break;
			case 3:
				form++;
				if(dflag){
					gdx /= ((UCHAR)(*form));
					gdy /= ((UCHAR)(*form));
				}
				break;
			case 4:
				form++;
				if(dflag){
					gdx *= ((UCHAR)(*form));
					gdy *= ((UCHAR)(*form));
				}
				break;
			case 5:
				if(dflag && lstc < 4) ps[lstc++] = pb;
				break;
			case 6:
				if(dflag && lstc > 0) pb = ps[--lstc];
				break;
			case 7:
				form++;
				if(dflag)
					if(!draw_sym1(font, ((WORD)*(++form)), vstatus, gdx, gdy, sx,
												&pb, sym_line, user_data)) return FALSE;
				break;
			case 8:
			case 9:
				form++;
				while(*form || *(form + 1)){
					if(dflag){
						get_next_txy((short)*form, (short)*(form + 1), gdx, gdy, sx, &pe);
						dpoint_add(&pe, &pb, &pe);
						if(pen){
							pb1 = pb; pe1 = pe;
							if(!faf_sym_line(&pb1, &pe1, sym_line, user_data)) return FALSE;
						}
						pb = pe;
					}
					if(code == 8) break;
					form += 2;
				}
				form++;
				break;
			case 10:  //   10  13   
				form += 2;
				break;
			case 11:
				form += 5;
				break;
			case 12:
				form += 3;
				break;
			case 13:
				form++;
				while(*form || *(form + 1))
					form += 3;
				form++;
				break;
			case 14:
				if(!vstatus){
					dflag = FALSE;
					form++;
					goto met_next;
				}
			case 15:
				break;

			default: // 
				if(dflag){
					c = ((short)code) >> 4;
					b = (short)(code & 0x0f);
					get_next_txy(c*dx[b]/2, c*dy[b]/2, gdx, gdy, sx, &pe);
					dpoint_add(&pe, &pb, &pe);
					if(pen){
						pb1 = pb; pe1 = pe;
						if(!faf_sym_line(&pb1, &pe1, sym_line, user_data)) return FALSE;
					}
					pb = pe;
				}
				break;
		}
		form++;
	}
}


static BOOL draw_shp(lpFONT font, WORD sym,
										BOOL vstatus, sgFloat gdx, sgFloat gdy, sgFloat sx,
										lpD_POINT p,
										BOOL(*sym_line)(lpD_POINT pb, lpD_POINT pe, void* user_data),
										void* user_data)
{
sgFloat     coeff;
lpSPSSHP   shp = (lpSPSSHP)ft_bd->buf;
ULONG      len;


	if(!shp) return TRUE;
	if(!load_ft_sym(FTSPS, sym, shp, &len))	return FALSE;
	if(!len) return TRUE;

	coeff = (font->num_up) ?  ((sgFloat)font->num_up)/shp->ngrid : 1.;
	gdx *= coeff; gdy *= coeff;

	return draw_shp1(shp, vstatus, gdx, gdy, sx, p, sym_line, user_data);
}

BOOL draw_shp1(lpSPSSHP shp, BOOL vstatus, sgFloat gdx, sgFloat gdy, sgFloat sx,
							 lpD_POINT p,
							 BOOL(*sym_line)(lpD_POINT pb, lpD_POINT pe, void* user_data),
							 void* user_data)
{
short      i = 0;
BOOL       first;
char       c, cx, cy;
D_POINT    pbeg, pb, pe, ps = {0, 0, 0};


	pb = pe = ps;
	pbeg = *p;

	if(!shp->ang) sx = 0.;

	//   
	if(vstatus){
		cx = shp->x[2];
		cy = shp->x[3];
		get_next_txy((short)cx, (short)cy, gdx, gdy, sx, &pe);
		dpoint_sub(&pbeg, &pe, &pbeg);
	}
	while(TRUE){
		 //  
		 c = shp->table[i++];
		 switch(c){
			 default:
			 case 0:
				 break;
			 case 1: //  
				 while(TRUE){
					 cx = shp->table[i++];
					 if(cx > 126 || cx < -126) break;
					 cy = shp->table[i++];
					 get_next_txy((short)cx, (short)cy, gdx, gdy, sx, &pb);
					 cx = shp->table[i++];
					 cy = shp->table[i++];
					 get_next_txy((short)cx, (short)cy, gdx, gdy, sx, &pe);
					 dpoint_add(&pb, &pbeg, &pb);
					 dpoint_add(&pe, &pbeg, &pe);
					 if(!faf_sym_line(&pb, &pe, sym_line, user_data)) return FALSE;
				 }
				 continue;
			 case 2: // 
			 case 3: // 
			 case 4: //  
				 first = TRUE;
				 while(TRUE){
					 cx = shp->table[i++];
					 if(cx > 126 || cx < -126) break;
					 cy = shp->table[i++];
					 if(first){
						 get_next_txy((short)cx, (short)cy, gdx, gdy, sx, &pb);
						 dpoint_add(&pb, &pbeg, &pb);
						 ps = pb;
						 first = FALSE;
						 continue;
					 }
					 get_next_txy((short)cx, (short)cy, gdx, gdy, sx, &pe);
					 dpoint_add(&pe, &pbeg, &pe);
					 if(!faf_sym_line(&pb, &pe, sym_line, user_data)) return FALSE;
					 pb = pe;
				 }
				 if(c != 2)
					 if(!faf_sym_line(&pb, &ps, sym_line, user_data)) return FALSE;
				 continue;
		 }
		 break;
	 }
	 if(vstatus){
		 cx = shp->x[4];
		 cy = shp->x[5];
		 get_next_txy((short)cx, (short)cy, gdx, gdy, sx, &pe);
		 dpoint_add(&pe, &pbeg, p);
	 }
	 else{
		 cx = shp->x[0];
		 cy = shp->x[1];
		 get_next_txy((short)cx, (short)cy, gdx, gdy, sx, &pe);
		 dpoint_add(p, &pe, p);
	 }
	 return TRUE;
}

static BOOL faf_sym_line(lpD_POINT pb, lpD_POINT pe,
												 BOOL (*sym_line)(lpD_POINT pb, lpD_POINT pe, void* user_data),
												 void* user_data)
{
	if(faf_flag){
		D_POINT p1, p2;
		faf_exe(pb, &p1);
		faf_exe(pe, &p2);
		return sym_line(&p1, &p2, user_data);
	}
	else
		return sym_line(pb, pe, user_data);
}

static void get_next_txy(short dx, short dy,
												sgFloat gdx, sgFloat gdy, sgFloat sx,
												void  *d)
//        
//   
{
lpD_PLPOINT p = (lpD_PLPOINT)d;
 p->y = dy*gdy;
 p->x = dx*gdx + p->y*sx;
}

static char* get_sym_form(lpFONT font, WORD sym)
//       
{
WORD*  tab;
WORD   i;
char*  f;
UCHAR  csym;

	if(sym < 256){
		csym = (UCHAR)sym;
		z_AnsiToOem((char*)&csym, (char*)&csym, 1);
		sym = csym;
	}
	tab = (WORD*)font->table;
	for(i = 0; i < font->num_sym; i++, tab += 2){
		if(*tab != sym && *tab != font->end_sym) continue;
		f = (char*)(font->table + *(tab + 1));
		f += (strlen(f) + 1);
		return f;
	}
	return NULL;
}

BOOL text_gab_lsk(lpD_POINT pb, lpD_POINT pe, void* gab)
{
lpD_POINT g = (lpD_POINT)gab;

	modify_limits_by_point(pb, &g[0], &g[1]);
	modify_limits_by_point(pe, &g[0], &g[1]);
	return TRUE;
}

void ft_free_mem(void)
//      
{
short i;
	free_dim_info();  // 
	if(ft_bd){
		free_vdim(&ft_bd->vd);
		free_vld_data(&ft_bd->vld);
		if(ft_bd->buf) SGFree(ft_bd->buf);
		for(i = FTARROW; i < FTNUMTYPES; i++)
			if(ft_bd->idlist[i - FTARROW]) SGFree(ft_bd->idlist[i - FTARROW]);
		SGFree(ft_bd);
	}
}

void ft_clear(void)
//     
//      
{
lpFONT    font;
ULONG     len;
IDENT_V   id;
short     i;

	if(0 ==(font = (FONT *)alloc_and_get_ft_value(ft_bd->def_font, &len))) return;

	free_vdim(&ft_bd->vd);
	free_vld_data(&ft_bd->vld);
	for (i = 0; i < FTARROW; i++)
		il_init_listh(&ft_bd->ft_listh[i]);
	il_init_listh(&ft_bd->free_listh);
	ft_bd->free_space = 0;
	ft_bd->max_len    = len;
	ft_bd->def_font = ft_bd->curr_font = 0;
	id = add_ft_value(FTFONT, font, len);
	ft_bd->def_font = ft_bd->curr_font = id;
	SGFree(font);
}

BOOL load_ft_value(IDENT_V item_id, void* value, ULONG *len)
//    ft_bd  id
//     
{
FT_ITEM  ft_item;

	if(!read_elem(&ft_bd->vd, item_id - 1, &ft_item)) return FALSE;
	*len = (ft_item.v.status == A_NOT_INLINE) ?
				 ft_item.v.value.ni.len : ft_item.v.status;
	ft_item_value(&ft_item, value);
	return TRUE;
}

void* alloc_and_get_ft_value(IDENT_V item_id, ULONG *len)
//      ft_bd   
//       id
//     
{
FT_ITEM  ft_item;
void*    value;

	if(!read_elem(&ft_bd->vd, item_id - 1, &ft_item)) attr_exit();
	*len = (ft_item.v.status == A_NOT_INLINE) ?
				 ft_item.v.value.ni.len : ft_item.v.status;
	if(0 ==(value = SGMalloc(*len))){
		attr_handler_err(AT_HEAP);
		return NULL;
	}
	ft_item_value(&ft_item, value);
	return value;
}


IDENT_V add_ft_value(FTTYPE fttype, void* value, ULONG len)
//       ft_bd
// fttype -  
//   -  
//   0 -  .;   .
{
	return insert_ft_value(fttype, 0, value, len);
}


IDENT_V insert_ft_value(FTTYPE fttype, IDENT_V bid, void* value, ULONG len)
//       ft_bd
// fttype -  
//   -  
//   0 -  .;   .
{
IDENT_V  id;
FT_ITEM  ft_item;

	memset(&ft_item, 0, sizeof(FT_ITEM));
	if(!place_ft_value(&ft_item, value, len)) return 0;

	//      id
	if(!il_add_elem(&ft_bd->vd, &ft_bd->free_listh,
		 &ft_bd->ft_listh[fttype], &ft_item, bid, &id)){
		if(id) attr_exit();
		free_ft_value(&ft_item);
		return 0;
	}
	return id;
}

static BOOL place_ft_value(lpFT_ITEM ft_item, void* value, ULONG len)
//    ft_item  value
{
	if(ft_bd->vld.mem)
		if(((sgFloat)ft_bd->free_space)/(ft_bd->vld.mem) > 1. &&
			 ft_bd->vld.listh.num > 2)
			if(!ft_bd_sb_mus()) return FALSE;

	if(len > ft_bd->max_len) ft_bd->max_len = len;
	//  
	return add_data_to_avalue(&ft_bd->vld, &ft_item->v, len, value);
}

static BOOL ft_bd_sb_mus(void)
//    vld     
{
int           i;
VLD           vld;
char          *buf;
lpFT_ITEM     ft_item;
VI_LOCATION   loc;

	init_vld(&vld);
	if(0 ==(buf = (char*)SGMalloc(ft_bd->max_len))) goto err;

//   VLD
	begin_rw(&ft_bd->vd, 0);
	for(i = 0; i < ft_bd->vd.num_elem; i++) {
	if(0 ==(ft_item = (FT_ITEM *)get_next_elem(&ft_bd->vd))) attr_exit();
		if(ft_item->v.status != A_NOT_INLINE) continue;
		if(!load_vld_data(&ft_item->v.value.ni.loc, ft_item->v.value.ni.len,
											buf)) attr_exit();
		if(!add_vld_data(&vld, ft_item->v.value.ni.len, buf)){
			end_rw(&ft_bd->vd);
			goto err;
		}
	}
	SGFree(buf);
	end_rw(&ft_bd->vd);
	free_vld_data(&ft_bd->vld);  //    

//   
	loc.hpage		= vld.listh.hhead;
	loc.offset	= sizeof(RAW);
	begin_rw(&ft_bd->vd, 0);
	begin_read_vld(&loc);
	for(i = 0; i < ft_bd->vd.num_elem; i++) {
		if(0 ==(ft_item = (FT_ITEM *)get_next_elem(&ft_bd->vd))) attr_exit();
		if(ft_item->v.status != A_NOT_INLINE) continue;
		get_read_loc(&ft_item->v.value.ni.loc);
		read_vld_data(ft_item->v.value.ni.len, NULL);
	}
	end_read_vld();
	end_rw(&ft_bd->vd);

	ft_bd->free_space = 0;
	ft_bd->vld = vld;
	return TRUE;
err:
	ft_bd->free_space = 0;
	if(buf) SGFree(buf);
	free_vld_data(&vld);
	return FALSE;
}

static  void free_ft_value(lpFT_ITEM ft_item)
//        ft_item
{
	if(ft_item->v.status != A_NOT_INLINE) return;

	ft_item->v.status = 0;
	ft_bd->free_space += ft_item->v.value.ni.len;
}

ULONG ft_item_value(lpFT_ITEM ft_item, void* value)
{
ULONG len;

	if(!load_data_from_avalue(&ft_item->v, &len, value)) attr_exit();
	return len;
}

void delete_ft_item(FTTYPE fttype, IDENT_V id)
//   id      
//  font = TRUE,  ,  
{
FT_ITEM ft_item;
	if(!id) return;
	if(fttype < FTARROW) return; // 
	if(!read_elem(&ft_bd->vd, id - 1, &ft_item)) attr_exit();
	free_ft_value(&ft_item);
	if(!il_detach_item(&ft_bd->vd, &ft_bd->ft_listh[fttype],
										 id)) attr_exit();
	if(!il_attach_item_tail(&ft_bd->vd, &ft_bd->free_listh, id)) attr_exit();
}


BOOL reload_draft_item(FTTYPE fttype, void* shp, ULONG len)
//     ft_bd  
{
IDENT_V id;
char    *name;
ULONG   len1;

	id = ft_bd->ft_listh[fttype].head;
	while(id){
		if((name = (char*)alloc_and_get_ft_value(id, &len1)) == NULL) return FALSE;
		if((len1 != len) || (strcmp(name, (char*)shp))){
			SGFree(name);
			if(!il_get_next_item(&ft_bd->vd, id, &id)) return FALSE;
			continue;
		}
		SGFree(name);
		break;
	}
	if(id){
		if(!insert_ft_value(fttype, id, shp, len)) return FALSE;
		delete_ft_item(fttype, id);
		return TRUE;
	}
	else
		return (add_ft_value(fttype, shp, len)) ? TRUE : FALSE;
}


BOOL save_draft_info(short *errcode)
//     
{
IDENT_V       id;
WORD          len, i;
ULONG         llen; 
char          c;
char          *shp;
BUFFER_DAT    bd;

	if(!ftbd_after_replase()) goto err1;

	if(!init_buf(&bd, ft_bd->draft_path, BUF_NEW)){
		*errcode = 3;
		return FALSE;
	}

	shp = ft_bd->buf;

	// 
	len = strlen(ft_bd->draft_title);
	if(!story_data(&bd, len, ft_bd->draft_title))  goto err2;
	// 
	c = (UCHAR)DRAFT_VERSION;
	if(!story_data(&bd, 1, &c))  goto err2;
	for(i = FTARROW; i < FTNUMTYPES; i++){
		c = (char)i;
		if(!story_data(&bd, 1, &c))  goto err2;
		id = ft_bd->ft_listh[i].head;
		while(id){
			if(!load_ft_value(id, shp, &llen)) goto err1;
			len = (WORD)llen;
			if(!story_data(&bd, 2, &len))	goto err2;
			if(!story_data(&bd, len, shp)) goto err2;
			if(!il_get_next_item(&ft_bd->vd, id, &id)) goto err1;
		}
		len = 0;
		if(!story_data(&bd, 2, &len))	goto err2;
	}
	c = 0;
	if(!story_data(&bd, 1, &c))  goto err2;
	close_buf(&bd);
	return TRUE;
err1:
	*errcode = 1;
	goto err;
err2:
	*errcode = 2;
	goto err;
err:
	close_buf(&bd);
	return FALSE;
}

#include "..//..//DraftData.h"

BOOL load_draft_info(short *errcode)
//     
{
WORD          i, j, len, maxlen = 0;
lpIDENT_V     idlist;
IDENT_V       id;
FTTYPE        fttype;
char          c;
char          *shp;
//BUFFER_DAT    bd;
int           curPos = 0;

	shp = ft_bd->buf;
	/*if(!init_buf(&bd, ft_bd->draft_path, BUF_OLD)){
		goto met_inline;
	}*/

	// 
	len = strlen(ft_bd->draft_title);
	//if(load_data(&bd, len, ft_bd->draft_title) != len)  goto err2;
	memcpy(ft_bd->draft_title, DraftData+curPos, len);
	curPos+=len;
	// 
	c = (UCHAR)DRAFT_VERSION;
	//if(load_data(&bd, 1, &c) != 1)  goto err2;
	memcpy(&c, DraftData+curPos, 1);
	curPos+=1;

	if(c != DRAFT_VERSION)  goto err2;
	while (TRUE){
		//if(load_data(&bd, 1, &c) != 1)  goto err2;
		memcpy(&c, DraftData+curPos, 1);
		curPos+=1;
		if(!c) break;
		if(c < FTARROW || c >= FTNUMTYPES) goto err2;
		fttype = (FTTYPE)c;
		while(TRUE){
			//if(load_data(&bd, 2, &len) != 2)  goto err2;
			memcpy(&len, DraftData+curPos, 2);
			curPos+=2;
			if(!len) break;
			if(len > MAX_SHP_LEN) goto err2;
			//if(load_data(&bd, len, shp) != len)  goto err2;
			memcpy(shp, DraftData+curPos, len);
			curPos+=len;
			if(maxlen < len) maxlen = len;
			if(!add_ft_value(fttype, shp, len)) goto err1;
		}
	}
//met_inline:
	//close_buf(&bd);
	for(i = FTARROW; i < FTNUMTYPES; i++){
		len = (WORD)ft_bd->ft_listh[i].num;
		if(!len){
			if(!load_inline_shp((FTTYPE)i, shp)) continue;
		}
		len = (WORD)ft_bd->ft_listh[i].num;
		if(!len) continue;
		if(ft_bd->idlist[i - FTARROW]) SGFree(ft_bd->idlist[i - FTARROW]);
		ft_bd->idlist[i - FTARROW] = (long*)SGMalloc(sizeof(IDENT_V)*(len + 1));
		idlist = ft_bd->idlist[i - FTARROW];
		if(!idlist) goto err1;
		id = ft_bd->ft_listh[i].head;
		j = 0;
		while(id){
			idlist[j++] = id;
			if(!il_get_next_item(&ft_bd->vd, id, &id)){
				SGFree(idlist);
				ft_bd->idlist[i - FTARROW] = NULL;
				goto err1;
			}
		}
	}
	return TRUE;
err1:
	*errcode = 1;
	goto err;
err2:
	*errcode = 2;
	goto err;
err:
	//close_buf(&bd);
	return FALSE;
}


BOOL ftbd_after_replase(void)
{
WORD       i, j, len;
lpIDENT_V  idlist;
IDENT_V    id;

	for(i = FTARROW; i < FTNUMTYPES; i++){
		len = (WORD)ft_bd->ft_listh[i].num;
		if(ft_bd->idlist[i - FTARROW]) SGFree(ft_bd->idlist[i - FTARROW]);
		if(!len) ft_bd->idlist[i - FTARROW] = NULL;
		else     ft_bd->idlist[i - FTARROW] = (long*)SGMalloc(sizeof(IDENT_V)*(len + 1));
		idlist = ft_bd->idlist[i - FTARROW];
		if(!idlist) continue;
		id = ft_bd->ft_listh[i].head;
		j = 0;
		while(id){
			idlist[j++] = id;
			if(!il_get_next_item(&ft_bd->vd, id, &id)){
				SGFree(idlist);
				ft_bd->idlist[i - FTARROW] = NULL;
				return FALSE;
			}
		}
	}
	return TRUE;
}

BOOL load_ft_sym(FTTYPE fttype, WORD sym, void* shp, ULONG *len)
{
lpIDENT_V  idlist;
short      i;

	*len = 0;
	i = fttype - FTARROW;
	idlist = ft_bd->idlist[i];
	if(!idlist) return TRUE;
	if(ft_bd->ft_listh[fttype].num < sym + 1) return TRUE;
	return load_ft_value(idlist[sym], shp, len);
}

static BOOL load_inline_shp(FTTYPE fttype, void* shp)
{
char name[MAX_DRAFT_NAME + 1];
char *c = (char*)shp;
WORD len = 0;

	strcpy(name, ((fttype != FTSPS) ? "Default" : "%%d"));
	memcpy(c, name, MAX_DRAFT_NAME + 1);
	c += (MAX_DRAFT_NAME + 1);
	len += (MAX_DRAFT_NAME + 1);
	switch(fttype){
		case FTARROW:
		{char tbl[]  = {100, 0, 100, -15, 15, 80, 0, 1,
										3, 100, 15, 0, 0, 100, -15, 80, 0, 127, 0};
		 memcpy(c, tbl, sizeof(tbl));
		 len += sizeof(tbl);
		 break;
		}
		case FTSPS:
		{char tbl[]  = {90, 1, 50, 0, 15, 90, 15, 0,
		 3, 0, 75, 4, 85, 15, 90, 26, 85, 30, 75, 26, 65, 15, 60, 4, 65, 127, 0};
		 memcpy(c, tbl, sizeof(tbl));
		 len += sizeof(tbl);
		 break;
		}
		case FTLTYPE:
		{sgFloat s = 1;
		 *c = 1; c++;
		 memcpy(c, &s, sizeof(sgFloat));
		 len += (sizeof(sgFloat) + 1);
		 break;
		}
		case FTHATCH:
		{sgFloat  x[5] = {0, 0, 0, 0, 5};
		 *c = 1; c++; *c = 5; c++;
		 memcpy(c, x, sizeof(x));
		 len += (sizeof(x) + 2);
		 break;
		}
	}
	if(!add_ft_value(fttype, shp, len)) return FALSE;
	return TRUE;
}
