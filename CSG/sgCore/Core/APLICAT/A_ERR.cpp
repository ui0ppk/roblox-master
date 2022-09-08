#include "../sg.h"


/************************************************
 *            --- handler_err ---
 ************************************************/
void a_handler_err(short cod,...)
{
	va_list ap;
	char 		s[255];
	char 		buf[20];
	long   	num;

	*s =  0;
	switch (cod) {
		case AE_NO_VMEMORY:   
			strcpy(s, GetIDS(IDS_SG296));	
			break;
		case AE_BAD_FILE:     
			strcpy(s, GetIDS(IDS_SG297));	
			break;
		case AE_ERR_DATA:     
			strcpy(s, GetIDS(IDS_SG298));	
			break;
		case AE_ERR_LOAD_STRING:   
			strcpy(s, GetIDS(IDS_SG299));	
			goto m;
		case AE_BAD_STRING:  
			strcpy(s, GetIDS(IDS_SG300));	
			goto m;
m:
			va_start(ap, cod);
			num = va_arg(ap,long);
			va_end(ap);
		//	strcat(s,ltoa(num,buf,10));
			break;
		default:
			strcpy(s, GetIDS(IDS_SG301));	
		//	itoa(cod,&s[strlen(s)],10);
			break;
	}
  put_message(EMPTY_MSG, s, 0);
}
