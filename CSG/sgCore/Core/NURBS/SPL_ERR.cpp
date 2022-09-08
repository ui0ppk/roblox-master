#include "../sg.h"

void nurbs_handler_err(short cod,...)
{
	char s[255], ss[20];
	short  len;

	strncpy(s, GetIDS(IDS_SG159), sizeof(s) - 1);
	len = sizeof(s) - strlen(s);
	switch (cod) {
		case SPL_NO_MEMORY:                       //    
			strncat(s,GetIDS(IDS_SG160),len);
			break;
		case SPL_NO_HEAP:													//    
			strncat(s,GetIDS(IDS_SG161),len);
			break;
		case SPL_MAX:															 //	   
			strncat(s,GetIDS(IDS_SG163),len);
			break;
		case SPL_INTERNAL_ERROR:										//    
			strncat(s,GetIDS(IDS_SG166),len);
			break;
		case SPL_ENPTY_DATA:												//   
			strncat(s,GetIDS(IDS_SG167),len);
			break;
		case SPL_BAD_POINT:                         //   
			strncat(s, GetIDS(IDS_SG168),len);
			break;
		default:
			strncat(s,GetIDS(IDS_SG186),len);
			//itoa(cod,ss,10);
			strncat(s,ss,sizeof(s) - strlen(s));
			break;
	}
  put_message(EMPTY_MSG, s, 0);
}
