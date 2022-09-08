#include "../../sg.h"

void handler_err(short cod,...)
{
	char s[256], ss[20];

	*s = 0;
	switch (cod) {
		case SAVE_EMPTY_BLOCK:   //   
			strncpy(s,GetIDS(IDS_SG229),sizeof(s)-1);
			break;
		case ERR_BAD_PARAMETER:   //    
			strncpy(s,GetIDS(IDS_SG230),sizeof(s)-1);
			break;
		case ERR_NO_VMEMORY:   //   
			strncpy(s,GetIDS(IDS_SG231),sizeof(s)-1);
			break;
		case ERR_NO_HEAP:      //  HEAP 
			strncpy(s,GetIDS(IDS_SG232),sizeof(s)-1);
			break;
		case ERR_NUM_OBJECT: //   
			strncpy(s,GetIDS(IDS_SG233),sizeof(s)-1);
			break;
		case ERR_NUM_FRAME:  //     
			strncpy(s,GetIDS(IDS_SG234),sizeof(s)-1);
			break;
		case ERR_NUM_BLOCK:   //   
			strncpy(s,GetIDS(IDS_SG235),sizeof(s)-1);
			break;
		case ERR_NP_MAX:         //    NP
			strncpy(s,GetIDS(IDS_SG236),sizeof(s)-1);
			break;
		case ERR_VERSION:     	
			strncpy(s,GetIDS(IDS_SG237),sizeof(s)-1);
			break;
		case ERR_DATABASE_FILE: 
			strncpy(s,GetIDS(IDS_SG238),sizeof(s)-1);
			break;
		case ERR_NUM_PRIMITIVE:  //   
			strncpy(s,GetIDS(IDS_SG239),sizeof(s)-1);
			break;
		case ERR_MAX_NP:  			//   
			strncpy(s,GetIDS(IDS_SG240),sizeof(s)-1);
			break;
		default:
			strncpy(s,GetIDS(IDS_SG241),sizeof(s)-1);
			//itoa(cod,ss,10);
			strncat(s,ss,sizeof(s) - strlen(s));
			break;
	}
	put_message(EMPTY_MSG, s, 0);
}
