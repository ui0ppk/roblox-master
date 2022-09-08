#include "../../sg.h"

short dxf_color(short color)
{
	short     acad_color[16] = { 0, 12, 14, 10, 11,  9, 13,  7,
														 8,  4,  6,  2,  3,  1,  5, 15 };

	if (color > 15) color = 0;
	color = acad_color[color];
//	if (color == vport_bg || color == blink_color) color = obj_attrib.color;
//	if (color == vport_bg) color = obj_attrib.color;
	return color;
}

BOOL dxf_cmp_func(void *elem, void *data)
{
	if (!strcmp(((lpLAYER)elem)->name, (char*)data)) return TRUE;
	return FALSE;
}

BOOL skip_vertex(void)
{
	while (TRUE) {
		if (!get_group(dxfg)) return FALSE;
		if (dxfg->gr == 0 && !memcmp(dxfg->str,dxf_keys[DXF_SEQEND],6)) break;
	}
	while (TRUE) {
		if (!get_group(dxfg)) return FALSE;
		if (dxfg->gr == 0) break;
	}
	return TRUE;
}

short dxf_convert_color(short color)
{
  switch ( color )
  {
    case IDXLIGHTRED:      return 1;
    case IDXYELLOW:        return 2;
    case IDXLIGHTGREEN:    return 3;
    case IDXLIGHTCYAN:     return 4;
    case IDXLIGHTBLUE:     return 5;
		case IDXLIGHTMAGENTA:  return 6;
		case IDXBLACK:         return 7;
    case IDXDARKGRAY:      return 8;
    case IDXRED:           return 9;
    case IDXBROWN:         return 10;
    case IDXGREEN:         return 11;
    case IDXCYAN:          return 12;
    case IDXBLUE:          return 13;
    case IDXMAGENTA:       return 14;
    case IDXWHITE:         return 15;
    default:               return 15;
  }
}

void dxf_err(CODE_MSG code)
{
	char str[80], num[20];

	if (dxfg->binary) {
		strcpy(str, get_message(DXF_ADDRESS)->str);
	//	ltoa(dxfg->bd->p_beg + dxfg->bd->cur_buf, num, 10);
	} else {
		strcpy(str, get_message(DXF_NUM_LINE)->str);
	//	ltoa(dxfg->bd->cur_line, num, 10);
	}
	strcat(str, num);
	put_message(code, str, 0);
}
