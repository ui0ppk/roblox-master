#include "../../sg.h"


#define DG_UNK   0
#define DG_STR   1
#define DG_DBL   2
#define DG_INT   3

static short   dxf_gr_bnum[] =
  {0,      10,     60,     140,    170,    210,    999,    1010,   1060};
static short   dxf_gr_enum[] =
  {9,      59,     79,     147,    175,    239,    1009,   1059,   1079};
static UCHAR dxf_gr_type[] =
  {DG_STR, DG_DBL, DG_INT, DG_DBL, DG_INT, DG_DBL, DG_STR, DG_DBL, DG_INT};

extern bool dxf_binary_flag;

BOOL put_vertex_to_dxf(lpBUFFER_DAT bd, int gr_num, lpD_POINT p)

{
short    i;
sgFloat *d = (sgFloat*)p;
  for(i = 0; i < 3; i++, gr_num += 10)
    if(!put_dxf_group(bd, gr_num, &d[i])) return FALSE;
  return TRUE;
}

BOOL put_dxf_group(lpBUFFER_DAT bd, int GrNum, void *par)

{
char   buf[MAX_sgFloat_STRING];
char   pk[2]  = {'\r','\n'};
short    j, gr_type = DG_UNK;
char   *c;
short    *i;
sgFloat *d;
UCHAR  ng = 255;
short gr_num;

  gr_num = (short)GrNum;

	if(dxf_binary_flag){
		if(gr_num == 999) return TRUE;
		if(gr_num >  255){
			if(!story_data(bd, sizeof(UCHAR), &ng)) return FALSE;
			if(!story_data(bd, sizeof(short), &gr_num)) return FALSE;
		}
		else{
			ng = (BYTE)gr_num;
			if(!story_data(bd, sizeof(UCHAR), &ng)) return FALSE;
		}
	}
	else{
		sprintf(buf, "%3d", gr_num);
		if(!story_data(bd, strlen(buf), buf)) return FALSE;
		if(!story_data(bd, 2, pk))            return FALSE;
	}


	for(j = 0; j < sizeof(dxf_gr_bnum)/sizeof(short); j++)
		if(gr_num >= dxf_gr_bnum[j] && gr_num <= dxf_gr_enum[j]){
			gr_type = dxf_gr_type[j];
			break;
		}

	switch(gr_type){
		case DG_STR:
			c = (char*)par;
			if(dxf_binary_flag) return story_data(bd, strlen(c) + 1, c);
			if(!story_data(bd, strlen(c), c)) return FALSE;
			break;
		case DG_INT:
			i = (short*)par;
			if(dxf_binary_flag) return story_data(bd, sizeof(short), i);
			sprintf(buf, "%6d", *i);
			if(!story_data(bd, strlen(buf), buf)) return FALSE;
			break;
		case DG_DBL:
			d = (sgFloat*)par;
			if(dxf_binary_flag) return story_data(bd, sizeof(sgFloat), d);
      sgFloat_to_text_for_export(buf, *d);
			if(!story_data(bd, strlen(buf), buf)) return FALSE;
			break;
		default:
			put_message(INTERNAL_ERROR, "DXFOUT", 0);
			return FALSE;
	}
	if(!story_data(bd, 2, pk)) return FALSE;

	return TRUE;
}
