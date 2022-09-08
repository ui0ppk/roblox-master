#ifndef __TXT_UTILS_HDR__
#define __TXT_UTILS_HDR__


#define MAX_sgFloat_DIGITS  16
#define MAX_sgFloat_STRING  MAX_sgFloat_DIGITS+8  // -.e+1230
#define ID_ALFA_EXT        "_$"

extern char rus_up[];
extern char rus_dn[];

char z_tolower(char c);
char z_toupper(char c);
BOOL z_isupper(char c);
BOOL z_islower(char c);
short z_charicmp(char c1, char c2);
short z_stricmp(char *s1, char *s2);
BOOL z_isalpha(char c);
BOOL z_isalnum(char c);
BOOL z_isalpha_id(char c);
BOOL z_isalnum_id(char c);
BOOL z_isalpha_ext(char c, char *ext);
BOOL z_isalnum_ext(char c, char *ext);
BOOL z_text_to_long(char *txtnum, long *num, short *errcode);
BOOL z_text_to_int(char *txtnum, int *num, short *errcode);
BOOL z_text_to_short(char *txtnum, short *num, short *errcode);
BOOL z_text_to_sgFloat(char *txtnum, sgFloat *num, short *errcode);
char* z_atof(char *txtnum, sgFloat *num);
char *sgFloat_output(sgFloat num, int size, int precision, BOOL delspace,
                    BOOL upper, char *txt);
char *sgFloat_output1(sgFloat num, int size, int precision,
                     BOOL delspace, BOOL delnull, BOOL upper, char *txt);
void z_str_decompress(char *strin, char *strout, short size, BOOL delbspace);
void z_OemToAnsi(char *oemstr, char *ansistr, ULONG len);
void z_AnsiToOem(char *ansistr, char *oemstr, ULONG len);
char *z_trim_space(char *txt);
void z_str_to_str_constant(char *str, char *sc);
BOOL z_text_to_COLORREF(char *txtnum, COLORREF *num, short *errcode);
char *z_COLORREF_to_text(COLORREF num, char *txtnum);
BOOL z_GetFilePath(char* path, char* pathonly);
BOOL z_GetFileTitle(char* path, char* title);


#endif