#include "../../sg.h"

typedef struct {
	CODE_PROMPT	code;		
	VI_LOCATION viloc; 
} DATA;
typedef DATA * lpDATA;
typedef struct {
	CODE_PROMPT		code;		
	char          *adr;   
} DATA_P;
typedef DATA_P * lpDATA_P;

static 	int 			SG_PROMPT_num = 0;			
				void *	SG_PROMPT_code = NULL;	
				char *	SG_PROMPT_char = NULL;	


lpSG_PROMPT get_prompt(CODE_PROMPT code)
{
					lpDATA_P 		data;
	static 	SG_PROMPT  	prompt;
	static 	char 				empty[] = "";

	if (!SG_PROMPT_code || !SG_PROMPT_char) {
		
		prompt.code = code;
		prompt.str = empty;
	}
	data = (lpDATA_P)bsearch(&code, SG_PROMPT_code, SG_PROMPT_num,
												 sizeof(DATA_P), NULL);
	if (!data) {
		
		prompt.code = code;
		prompt.str = empty;
	} else {
		prompt.code = data->code;
		prompt.str  = data->adr;
	}
	return &prompt;
}

lpSG_MESSAGE get_message(CODE_MSG code)
{
	static 	char				message[256];
	static 	SG_MESSAGE	msg;
	int					len=0;

	
	strcpy(message,"Error");
	
	//message[len] = 0;
	msg.code = code;
	msg.str = message;
	return &msg;
}

