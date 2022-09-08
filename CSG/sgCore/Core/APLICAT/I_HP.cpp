#include "../sg.h"

typedef enum { PU, PD, SP, NOTDEF} CMD_HP;
typedef CMD_HP * lpCMD_HP;
static char *cmd_m[] = {
	"PU",
	"PD",
  "SP",
};
static  BOOL skip_cmd(lpBUFFER_DAT bd);
static  BOOL get_cmd(lpBUFFER_DAT bd, lpCMD_HP cmd);
static  BOOL get_number(lpBUFFER_DAT bd, short * number);

int ImportHp(lpBUFFER_DAT bd, lpLISTH listh, lpVLD vld, BOOL frame)
{
	short         cur_color;
	GEO_SIMPLE  geo;
	int        count = 0;
  CMD_HP      cmd;
  short         x_cur = 0, y_cur = 0;
	short         number, x, y;

//	num_proc = start_grad(GetIDS(IDS_SG177), bd->len_file);
	cur_color = IDXWHITE;
	while (1) {
		if ( !get_cmd(bd, &cmd) ) goto err;
//    step_grad (num_proc , bd->p_beg+bd->cur_buf);
    switch ( cmd )
		{
      case PU:
				if ( !get_number(bd, &x_cur) ) goto err;
        if ( !get_number(bd, &y_cur) ) goto err;
        continue;
      case PD:
        if ( !get_number(bd, &x) ) goto err;
        if ( !get_number(bd, &y) ) goto err;
				#define PltUnitMM 0.02488        
        geo.line.v1.x = x_cur * PltUnitMM;  geo.line.v1.y = y_cur * PltUnitMM;
        geo.line.v1.z = 0;
				geo.line.v2.x = x * PltUnitMM;      geo.line.v2.y = y * PltUnitMM;
				geo.line.v2.z = 0;
				if ( !cr_add_obj(OLINE,cur_color,&geo,listh,vld,frame) ) goto err;
				count++;
				x_cur=x; y_cur=y;
        continue;
      case SP:
        if ( !get_number(bd, &number) ) goto err;
				cur_color = (number - 1) % 16;
				continue;
			case NOTDEF:
				if ( !skip_cmd(bd) ) goto err;
				continue;
		}
	}
err:
//	end_grad  (num_proc , bd->p_beg+bd->cur_buf);
	return count;
}

static  BOOL skip_cmd(lpBUFFER_DAT bd)
{
	char c = 0;
	while ( c != ';' && c != ':' )
		if ( load_data(bd,1,&c) != 1 ) return FALSE;
	return TRUE;
}

static  BOOL get_cmd(lpBUFFER_DAT bd, lpCMD_HP cmd)
{
	register short i;
	char c[2];

m:
	if ( load_data(bd,2,c) != 2 ) return FALSE;
	if ( *(short*)c == 0x0A0D ) goto m;
	for (i=0; i< NOTDEF; i++ ) {
		if ( *(short*)c == *(short*)cmd_m[i]) break;
	}
	*cmd = (CMD_HP)i;
	return TRUE;
}

static  BOOL get_number(lpBUFFER_DAT bd, short * number)
{
#define MAX_STR 80
	char s[MAX_STR];
	short  i = 0;

	*number = 0;
	if ( load_data(bd,1,&s[0]) != 1 ) return FALSE;
	while ( s[i] != ';' && s[i] != ',' && s[i] != ' ' ) {
		if ( i== MAX_STR ) break;
		i++;
		if ( load_data(bd,1,&s[i]) != 1 ) return FALSE;
	}
	s[i] = 0;
	*number = atoi(s);
	return TRUE;
}
