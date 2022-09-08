#include "../sg.h"


void cinema_handler_err(short cod,...)
{
	char s[255], ss[20];
	short  len;

	strncpy(s, GetIDS(IDS_SG209), sizeof(s) - 1);
	len = sizeof(s) - strlen(s);
	switch (cod) {
		case CIN_BIG_LOOP:
			strncat(s,GetIDS(IDS_SG181),len);
			break;
		case CIN_NO_CORRECT_OZ:
			strncat(s,GetIDS(IDS_SG182),len);
			break;
		case CIN_NO_CORRECT_NZ:
			strncat(s,GetIDS(IDS_SG183),len);
			break;
		case CIN_BAD_PATH:
			strncat(s,GetIDS(IDS_SG184),len);
			break;
		case CIN_OVERFLOW_MDD:
			strncat(s,GetIDS(IDS_SG185),len);
			break;
		case CIN_OUT_RADIUS_BAD:
			strncat(s, GetIDS(IDS_SG208),len);
			break;
		default:
			strncat(s,GetIDS(IDS_SG186),len);
			//itoa(cod,ss,10);
			strncat(s,ss,sizeof(s) - strlen(s));
			break;
	}
  put_message(EMPTY_MSG, s, 0);
}
