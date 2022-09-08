#include "../sg.h"

char rus_up[] = "";
char rus_dn[] = "";

static  short  z_charint(char c);
static  void compress_sgFloat_gtext(char *txt, char *buf, char f);
static  void compress_sgFloat_ftext(char *txt, char *buf,
                                       BOOL delspace, BOOL delnull);


static unsigned char	ANSI2OEM_Nice[256] = {
	// 0     1	 2	   3     4     5	 6     7
	// ==============================================
	0,    1,	  2,    3,    4,    5,	  6,    7,  //	 0
		8,    9,	 10,   11,   12,   13,	 14,   15,  //	 8
		16,   17,	 18,   19,   20,   21,	 22,   23,  //	16
		24,   25,	 26,   27,   28,   29,	 30,   31,  //	24
		32,   33,	 34,   35,   36,   37,	 38,   39,  //	32
		40,   41,	 42,   43,   44,   45,	 46,   47,  //	40
		48,   49,	 50,   51,   52,   53,	 54,   55,  //	48
		56,   57,	 58,   59,   60,   61,	 62,   63,  //	56
		64,   65,	 66,   67,   68,   69,	 70,   71,  //	64
		72,   73,	 74,   75,   76,   77,	 78,   79,  //	72
		80,   81,	 82,   83,   84,   85,	 86,   87,  //	80
		88,   89,	 90,   91,   92,   93,	 94,   95,  //	88
		96,   97,	 98,   99,  100,  101,	102,  103,  //	96
		104,  105,	106,  107,  108,  109,	110,  111,  // 104
		112,  113,	114,  115,  116,  117,	118,  119,  // 112
		120,  121,	122,  123,  124,  125,	126,  127,  // 120

		219,  254,	254,  254,  254,  254,	254,  254,  // 128
		254,  254,	254,  254,  254,  254,	254,  254,  // 136
		254,  254,	254,  254,  254,  176,	249,  254,  // 144
		254,  253,	254,  254,  254,  254,	254,  254,  // 152
		254,  246,	247,  254,  254,  254,	254,  254,  // 160
		240,  254,	244,  254,  251,  254,	254,  242,  // 168

		248,  254,	254,  254,  254,  254,	254,  250,  // 176
		241,  252,	245,  254,  254,  254,	255,  243,  // 184
		128,  129,	130,  131,  132,  133,	134,  135,  // 192
		136,  137,	138,  139,  140,  141,	142,  143,  // 200
		144,  145,	146,  147,  148,  149,	150,  151,  // 208
		152,  153,	154,  155,  156,  157,	158,  159,  // 216

		160,  161,	162,  163,  164,  165,	166,  167,  // 224
		168,  169,	170,  171,  172,  173,	174,  175,  // 232

		224,  225,	226,  227,  228,  229,	230,  231,  // 240
		232,  233,	234,  235,  236,  237,	238,  239   // 248
};

unsigned char	OEM2ANSI_Nice[256] = {
	// 0     1	 2	   3     4     5	 6     7
	// ==============================================
	0,    1,	  2,    3,    4,    5,	  6,    7,  //	 0
		8,    9,	 10,   11,   12,   13,	 14,   15,  //	 8
		16,   17,	 18,   19,   20,   21,	 22,   23,  //	16
		24,   25,	 26,   27,   28,   29,	 30,   31,  //	24
		32,   33,	 34,   35,   36,   37,	 38,   39,  //	32
		40,   41,	 42,   43,   44,   45,	 46,   47,  //	40
		48,   49,	 50,   51,   52,   53,	 54,   55,  //	48
		56,   57,	 58,   59,   60,   61,	 62,   63,  //	56
		64,   65,	 66,   67,   68,   69,	 70,   71,  //	64
		72,   73,	 74,   75,   76,   77,	 78,   79,  //	72
		80,   81,	 82,   83,   84,   85,	 86,   87,  //	80
		88,   89,	 90,   91,   92,   93,	 94,   95,  //	88
		96,   97,	 98,   99,  100,  101,	102,  103,  //	96
		104,  105,	106,  107,  108,  109,	110,  111,  // 104
		112,  113,	114,  115,  116,  117,	118,  119,  // 112
		120,  121,	122,  123,  124,  125,	126,  127,  // 120

		192,  193,	194,  195,  196,  197,	198,  199,  // 128
		200,  201,	202,  203,  204,  205,	206,  207,  // 136
		208,  209,	210,  211,  212,  213,	214,  215,  // 144
		216,  217,	218,  219,  220,  221,	222,  223,  // 152
		224,  225,	226,  227,  228,  229,	230,  231,  // 160
		232,  233,	234,  235,  236,  237,	238,  239,  // 168

		149,  149,	149,  124,   43,   43,	 43,   43,  // 176
		43,   43,	124,   43,   43,   43,	 43,   43,  // 184
		43,   43,	 43,   43,   45,   43,	 43,   43,  // 192
		43,   43,	 43,   43,   43,   45,	 43,   43,  // 200
		43,   43,	 43,   43,   43,   43,	 43,   43,  // 208
		43,   43,	 43,  128,  128,  128,	128,  128,  // 216

		240,  241,	242,  243,  244,  245,	246,  247,  // 224
		248,  249,	250,  251,  252,  253,	254,  255,  // 232

		168,  184,	175,  191,  170,  186,	161,  162,  // 240
		176,  150,	183,  172,  185,  153,	182,  190   // 248
};

//           

char z_tolower(char c){
    
char* d;
  if(c && (d = strchr(rus_up, c)) != NULL)
		return rus_dn[(short)(d - rus_up)];
	if(isascii(c))if(isupper(c))
		return (BYTE)(_tolower(c));
	return c;
}

char z_toupper(char c){
   
char* d;
  if(c && (d = strchr(rus_dn, c)) != NULL)
    return rus_up[(short)(d - rus_dn)];
  if(isascii(c))if(islower(c))
    return (BYTE)(_toupper(c));
	return c;
}

BOOL z_isupper(char c){
    
	if(c && strchr(rus_up, c) != NULL) return TRUE;
	if(isascii(c))if(isupper(c))        return TRUE;
	return FALSE;
}

BOOL z_islower(char c){
     
	if(c && strchr(rus_dn, c) != NULL) return TRUE;
	if(isascii(c))if(islower(c))        return TRUE;
  return FALSE;
}

short z_charicmp(char c1, char c2){
    
	return z_charint(z_tolower(c1)) - z_charint(z_tolower(c2));
}


short z_stricmp(char *s1, char *s2){
     
short i;
	while(!(i = z_charicmp(*s1, *s2)) && (*s1 || *s2)){
		s1++;
		s2++;
	}
	return i;
}

BOOL z_isalpha(char c)
  
{
  if(isalpha(c)) return TRUE;
  if(!c) return FALSE;
	if(strchr(rus_up, c)) return TRUE;
	if(strchr(rus_dn, c)) return TRUE;
	return FALSE;
}

BOOL z_isalnum(char c)
     
{
  if(isdigit(c)) return TRUE;
  return z_isalpha(c);
}

BOOL z_isalpha_id(char c)

{
  return z_isalpha_ext(c, ID_ALFA_EXT);
}

BOOL z_isalnum_id(char c)

{
  return z_isalnum_ext(c, ID_ALFA_EXT);
}

BOOL z_isalpha_ext(char c, char *ext)

{
  if(z_isalpha(c))   return TRUE;
  if(c && strchr(ext, c)) return TRUE;
	return FALSE;
}

BOOL z_isalnum_ext(char c, char *ext)

{
	if(isdigit(c)) return TRUE;
	return z_isalpha_ext(c, ext);
}

char* z_trim_space(char *txt){

char *inp;
short  len;
	inp = txt;
	while (*inp == ' ' || *inp == '\t') inp++;
	len = strlen(inp);
	memcpy(txt, inp, len + 1);
	while (--len >= 0)
	 if(txt[len] != ' ' && txt[len] != '\t') break;
	txt[len + 1] = 0;

	return txt;
}

BOOL z_text_to_long(char *txtnum, long *num, short *errcode){

sgFloat wnum;

  if(!z_text_to_sgFloat(txtnum, &wnum, errcode)) return FALSE;
	if(wnum != floor(wnum))
    {*errcode = 2; return FALSE;}
	if(fabs(wnum) > (sgFloat)(MAXLONG))
    {*errcode = 3; return FALSE;}

	*num = (long)wnum;
	return TRUE;
}

BOOL z_text_to_int(char *txtnum, int *num, short *errcode){

long lnum;
BOOL ret;

  ret = z_text_to_long(txtnum, &lnum, errcode);
  if(ret)
    *num = (int)lnum;
  return ret; 
}


BOOL z_text_to_short(char *txtnum, short *num, short *errcode){

sgFloat wnum;

  if(!z_text_to_sgFloat(txtnum, &wnum, errcode)) return FALSE;
	if(wnum != floor(wnum))
    {*errcode = 2; return FALSE;}
	if(fabs(wnum) > (sgFloat)(MAXSHORT))
    {*errcode = 3; return FALSE;}

	*num = (short)wnum;
	return TRUE;
}

BOOL z_text_to_sgFloat(char *txtnum, sgFloat *num, short *errcode){

char   *inp;
sgFloat wnum;
	inp = txtnum;
	while (*inp == ' ' || *inp == 9) inp++;
  if(!(*inp)){ *errcode = 0; return FALSE;}
	if(0 ==(inp = z_atof(inp, &wnum))) { *errcode = 1; return FALSE;}
	while (*inp == ' ' || *inp == 9) inp++;
  if(*inp) {*errcode = 1; return FALSE;}
	*num = wnum;
	return TRUE;
}

char* z_atof(char *txtnum, sgFloat *num)
{
BOOL flagd     = FALSE;
BOOL flagdp    = FALSE;
BOOL flagnonul = FALSE;
BOOL flagp     = FALSE;
BOOL flagpoint = FALSE;
char*  inp;
char   ch;
sgFloat wnum;

  inp = txtnum;
  if(*inp == '+' || *inp == '-') inp++;
  for( ; ; inp++){
		if(*inp <= '9' && *inp >= '0'){
      if(!flagp){
        flagd = TRUE;
        if(*inp != '0') flagnonul = TRUE;
			}
      else flagdp = TRUE;
			continue;
		}
    if(*inp == '.'){
      if(flagpoint) return NULL;
			else {
       flagpoint = TRUE;
			 continue;
			}
    }
		if(flagp) break;
		if(*inp == 'e' || *inp == 'E'){
			flagp = TRUE;
			inp++;
			if(!(*inp == '+') && !(*inp == '-')) inp--;
			continue;
		}
		break;
	}
  if(inp == txtnum || !flagd) return NULL;   //  
  if(flagp && !flagdp)        return NULL;   //    
  ch = *inp; *inp = 0; wnum = atof(txtnum); *inp = ch;
  if(wnum == 0. && flagnonul) return NULL;   //   atof
  *num = wnum;
	return inp;
}

char *sgFloat_output(sgFloat num, int size, int precision, BOOL delspace,
                    BOOL upper, char *txt)
   
{
  return sgFloat_output1(num, size, precision, delspace, TRUE, upper, txt);
}

char *sgFloat_output1(sgFloat num, int size, int precision,
                     BOOL delspace, BOOL delnull, BOOL upper, char *txt)
{
int  lp = 1, s, p;
char eformat[] = {"%-.*lE"};
char fformat[] = {"%*.*lf"};
char buf[MAX_sgFloat_STRING + 5];
BOOL eflag = FALSE;

  eformat[5] = (BYTE)((upper) ? 'E' : 'e');
  if(size <= 0){
    delspace = TRUE;
    size = MAX_sgFloat_STRING - 1;
  }
  if(size > MAX_sgFloat_STRING - 1) size = MAX_sgFloat_STRING - 1;

  if(precision < 0){  //     "E"
    precision = -precision;
    eflag = TRUE;
  }

	s = size;
  if(num != floor(num) || !delnull) s--; // 
  if(num < 0.) s--;  //  "-"; s =    

  p = (fabs(num) > 0.) ? (short)(log10(fabs(num))) : 0; // 
  if(abs(p) > 10) lp++; if(abs(p) > 100) lp++;        //   

  if(precision > MAX_sgFloat_DIGITS) precision = MAX_sgFloat_DIGITS;

  if(eflag) goto met_e;

  if(p < 0) p = 0;
  if(++p <= s) { //    "F"
    p = s - p;
    if(p > precision) p = precision;
    sprintf(buf, fformat, size, p , num);
    compress_sgFloat_ftext(buf, txt, delspace, delnull);
    return txt;
  }
met_e:   //    "E"
  p = s - lp - 3;     // E+123
  if(p > precision) p = precision;
  while(TRUE){     //   
    if(p <= 0){    //       
      memset(txt, '*', size);
      txt[size] = 0;
      return txt;
    }
    sprintf(buf, eformat, p, num);
    compress_sgFloat_gtext(buf, txt, eformat[5]);
    lp = strlen(txt);
    if(lp <= size) break;
    p--;
  }
  if(!delspace){
    while(lp < size) txt[lp++] = ' ';
    txt[lp] = 0;
  }
  return txt;
}

static  void compress_sgFloat_gtext(char *txt, char *buf, char f)
{
char *t, *e;
  e = strchr(txt, f);
  if(e) *e = 0;
	if(strchr(txt, '.')) { //    -   
		t = txt + strlen(txt);
		while(*(--t) == '0');
		if(*t != '.') t++;
    *t = 0;
  }
  t = txt;
	if(!strcmp(txt, "-0")) t++;
  strcpy(buf, t);
  if(e){
    *e = f; t = e + 1;
    if     (*t == '+') strcpy(t, t + 1);
    else if(*t == '-') t++;
    while(*t == '0') strcpy(t, t + 1); //     
    if(*t) strcat(buf, e);
  }
}

static  void compress_sgFloat_ftext(char *txt, char *buf,
                                       BOOL delspace, BOOL delnull)
{
char s, *b, *t;

  b = txt;
  while(*b == ' ') b++;

  t = txt + strlen(txt);
  if(strchr(txt, '.') && delnull){//    -   
		while(*(--t) == '0');
		if(*t != '.') t++;
  }
  s = *t; *t = 0;
  if(!strcmp(b, "-0")){
    if(delspace) b++;
    else         *b = ' ';
  }
  if(!delspace){
    b = txt;
    *t = s;
    while(*t) *(t++) = ' ';
  }
  strcpy(buf, b);
}

static  short z_charint(char c){
short i;
	i = (UCHAR)c;
	return i;
}

void z_str_decompress(char *strin, char *strout, short size, BOOL delbspace)
{
short  lenstr, lspace, nspace, lwst, nadd, i, j;
char c, *s, *sb, *se, *sout;

  memset(strout, ' ', size);
  strout[size] = 0;
  sb = strin;
  while(*sb == ' ') sb++; //   
  lenstr = strlen(sb);
  if(!lenstr) return;
  se = sb + (lenstr - 1);
  while(*se == ' ') se--; //   
  se++; c = *se; *se = 0;

  if(!delbspace){
    i = (short)(sb - strin);
    sout = strout + i;
    size -= i;
  }
  else
    sout = strout;

  lenstr = strlen(sb);

  if(lenstr > size) { //   
    strncpy(sout, sb, size);
    goto ret;
  }

  if(!(lspace = size - lenstr)) goto met; //    

  s = sb; nspace = 0;
  while((s = strchr(s, ' ')) != NULL){ //  
    nspace++;           //   
    while(*s == ' ') s++;
  }
  if(nspace){
    lwst = lspace/nspace;         //    
    nadd = lspace - lwst*nspace;  //     nadd 

		s = sb; i = 0;
		while(*s){
      while((*s) && (*s != ' ')) { sout[i++] = *s; s++;}
			if(!(*s)) break;
			for(j = 0; j < lwst; j++) i++;
			if(nadd >= nspace) i++;
			while(*s == ' ') { i++; s++;}
			nspace--;
		}
		return;
	}
met:
  strncpy(sout, sb, lenstr);
ret:
	*se = c;
	return;
}

int OemToAnsiBuffNice(LPSTR from,	LPSTR to, UINT	len)
{
	int		c;
	UINT	i;

	for (i = 0; i < len; i++)
	{
		c = *from++;
		if (c < 0)
			c += 256;
		*to++ = OEM2ANSI_Nice[c];
	}
	return -1;
}

void z_OemToAnsi(char *oemstr, char *ansistr, ULONG len)
{
//nb+
	if (len)
		OemToAnsiBuffNice(oemstr, ansistr, len);
	else
		OemToAnsiBuffNice(oemstr, ansistr, strlen(oemstr));
//nb-

}

int AnsiToOemBuffNice(LPSTR from,	LPSTR to,	UINT	len)
{
	int		c;
	UINT	i;

	for (i = 0; i < len; i++)
	{
		c = *from++;
		if (c < 0)
			c += 256;
		*to++ = ANSI2OEM_Nice[c];
	}
	return -1;
}

void z_AnsiToOem(char *ansistr, char *oemstr, ULONG len)
{
//nb+
	if (len)
		AnsiToOemBuffNice(ansistr, oemstr, len);
	else
		AnsiToOemBuffNice(ansistr, oemstr, strlen(oemstr));
//nb-

}

void z_str_to_str_constant(char *str, char *sc)
//   sc  str   
{
	*(sc++) = '\"';
	while(*str){
		if(*str == '\"' || *str == '\\')
			*(sc++) = '\\';
		*(sc++) = *(str++);
	}
	*(sc++) = '\"';
	*sc = 0;
}


BOOL z_text_to_COLORREF(char *txtnum, COLORREF *num, short *errcode){

char   *inp;
sgFloat wnum;
BYTE   bnum[3] = {0, 0, 0};
short    i;

	inp = txtnum;
	for (i = 0; i < 3; i++){
		while (*inp == ' ' || *inp == 9) inp++;
		if(!(*inp))break;
		if(0 ==(inp = z_atof(inp, &wnum))) { *errcode = 1; return FALSE;}
		if(wnum != floor(wnum))
			{*errcode = 2; return FALSE;}
		if(wnum > (sgFloat)(255) || wnum < 0)
			{*errcode = 3; return FALSE;}
		bnum[i] = (BYTE)wnum;
	}
	if(i == 0) {*errcode = 0; return FALSE;}
	if(i == 3){
		while (*inp == ' ' || *inp == 9) inp++;
		if(*inp) {*errcode = 1; return FALSE;}
	}
	*num = RGB(bnum[0], bnum[1], bnum[2]);
	return TRUE;
}

char *z_COLORREF_to_text(COLORREF num, char *txtnum){
 if (!sprintf(txtnum, "%u %u %u", GetRValue(num),
																	GetGValue(num),
																	GetBValue(num)))
	 txtnum[0]= 0;
 return txtnum;
}

BOOL z_GetFilePath(char* path, char* pathonly)
      
{
char  fname[129];
short i, l, elen;

//	if(GetFileTitle(path, fname, 128))
		return FALSE;
	l = strlen(fname);
	if(!l)
		return FALSE;
	elen = 0;
	for(i = l - 1; i >= 0; i--){
		if(fname[i] == '.')
			goto met1;
		if(l - i  > 3)
			break;
	}
	//    
	l = strlen(path);
	if(!l)
		return FALSE;
	for(i = l - 1; i >= 0; i--){
		if(path[i] == '.'){
			elen = l - i;
			break;
		}
		if(l - i  > 3)
			break;
	}
met1:
	l = strlen(path) - strlen(fname) - elen;
	if(l > 0)
		strncpy(pathonly, path, l);
	pathonly[l] = 0;
	return TRUE;
}

BOOL z_GetFileTitle(char* path, char* title)
//       
{
char  pathonly[256];

	if(!z_GetFilePath(path, pathonly))
		return FALSE;
	strcpy(title, &path[strlen(pathonly)]);
	return TRUE;
}

