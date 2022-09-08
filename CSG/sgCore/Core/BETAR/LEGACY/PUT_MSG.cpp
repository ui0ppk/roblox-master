#include "../../sg.h"

//
//
//
#pragma argsused
void
put_message(CODE_MSG	code,
		    char* 		s,
			UCHAR 		level)
{
	return;
	//	int 		x;
	//	UCHAR 	fg, bg;
	//	HDC			hDC;
	//	COLORREF	rgb;

	if(code == CTRL_C_PRESS) 
		return;

	lpSG_MESSAGE	lpmsg = get_message(code);
	char sz[256];

	if ((level != 0) && (level <= message_level))
		level = 2;
	if (strlen(lpmsg->str))
	{
		strncpy(sz, lpmsg->str, sizeof(sz)-1);
		if (s && *s){
			strncat(sz, ":", sizeof(sz)-strlen(sz)-1);
			strncat(sz, s, sizeof(sz)-strlen(sz)-1);
		}
	}
	else
		if (s && *s)
			strncpy(sz, s, sizeof(sz)-1);
		else
			*sz = 0;

	

	//--------------------------------------------------------------------------
	/*if (level > message_level)
		StatusBarShow(0, sz, RGB_MAGENTA, DT_LEFT);
	else
		Tlx_puts(tw[TW_CMD].hWnd, sz, level);
	if(level == 0 || level == 255)
		MessageBeep((UINT)-1);*/
//	if (messager)
//		messager->PutMessage(IMessager::MT_MESSAGE,sz);
	return;
}

