#include "../sg.h"

void u_handler_err(short cod,...)
{
	char s[MAXPATH+80];
	va_list ap;
	char *  name;

	strcpy(s,GetIDS(IDS_SG196)); //	 
	switch (cod) {
		case UE_OPEN_ACCES:
			strcat(s,GetIDS(IDS_SG255));
			break;
		case UE_OPEN_EINVACC:
			strcat(s,GetIDS(IDS_SG256));
			break;
		case UE_OPEN_EMFILE:
			strcat(s,GetIDS(IDS_SG257));
			break;
		case UE_OPEN_ENOENT:
			strcat(s,GetIDS(IDS_SG258));
			break;

		case UE_NO_VMEMORY:   //   
			strcat(s,GetIDS(IDS_SG197));
			break;
		case UE_READ_FILE:    //     
			strcpy(s,GetIDS(IDS_SG198));
			break;
		case UE_WRITE_FILE:    //      
			strcpy(s,GetIDS(IDS_SG199));
			break;
		case UE_OPEN_FILE:     //    
			va_start(ap, cod);
			name = va_arg(ap,char *);
			va_end(ap);
			strcpy(s,GetIDS(IDS_SG200));
			strncat(s,name,MAXPATH);
			break;
		case UE_OVERFLOW_VDIM:         //  VDIM
			strcat(s,GetIDS(IDS_SG201));
			break;
		case UE_CLOSE_FILE:     //    
			strcpy(s,GetIDS(IDS_SG202));
			break;
		case UE_END_OFF_FILE:     //   
			strcat(s,GetIDS(IDS_SG203));
			break;
		case UE_SEEK_FILE:     //   
			strcat(s,GetIDS(IDS_SG204));
			break;
		default:
			strcat(s,GetIDS(IDS_SG205));
			//itoa(cod,&s[strlen(s)],10);
			break;
	}
  put_message(EMPTY_MSG, s, 0);
}
