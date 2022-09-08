#include "../../sg.h"

void np_handler_err(short cod,...)
{
	char 	s[255], ss[20];
	int		len;

	strncpy(s,GetIDS(IDS_SG214),sizeof(s)-1);
	len = sizeof(s) - strlen(s);
	switch (cod) {
		case NP_BAD_MODEL:
			strncat(s,GetIDS(IDS_SG215),len);
			break;
		case NP_BAD_PLANE:
			strncat(s,GetIDS(IDS_SG216),len);
			break;
		case NP_GRU:
			strncpy(s,GetIDS(IDS_SG217),sizeof(s)-1);
			break;
		case NP_NO_IDENT:
			strncat(s,GetIDS(IDS_SG218),len);
			break;
		case NP_DEL_EDGE:
			strncat(s,GetIDS(IDS_SG219),len);
			break;
		case NP_DEL_FACE:
			strncat(s,GetIDS(IDS_SG220),len);
			break;
		case NP_LOOP_IN_PLANE:
			strncat(s,GetIDS(IDS_SG221),len);
			break;
		case NP_AN_CNT:
			strncat(s,GetIDS(IDS_SG222),len);
			break;
		case NP_CNT_BREAK:
			strncat(s,GetIDS(IDS_SG223),len);
			break;
		case NP_NO_CORRECT:
			strncat(s,GetIDS(IDS_SG224),len);
			break;
		case NP_NO_EDGE:
			strncat(s,GetIDS(IDS_SG225),len);
			break;
		case NP_MESH4:
			strncat(s,GetIDS(IDS_SG226),len);
			break;
		case NP_DIV_FACE:
			strncat(s,GetIDS(IDS_SG227),len);
			break;
		default:
			strncat(s,GetIDS(IDS_SG228),len);
			//itoa(cod,ss,10);
			strncat(s,ss,sizeof(s) - strlen(s));
			break;
	}
  put_message(EMPTY_MSG, s, 0);
}
