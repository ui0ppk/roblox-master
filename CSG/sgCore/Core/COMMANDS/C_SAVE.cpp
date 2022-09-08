#include "../sg.h"

bool RealSaveModel(char *FileName, BOOL SelFlag, 
				   const void* userData, unsigned long userDataSize)

{
//char 	MsgStr[_MAX_PATH + 100];
//char 	Buf[15];
int   Count = 0;
bool  Code;
bool  BakFlag = FALSE;

	if(!nb_access(FileName, 0))	  
    BakFlag = CreateBakFile(FileName);

  if(bool_var[SAVE_ON_UCS])   
		Code = sg_save_model(FileName, SelFlag, &Count, /*pFileProp,*/ m_gcs_ucs, m_ucs_gcs,
									userData,userDataSize);
	else      	               
		Code = sg_save_model(FileName, SelFlag, &Count, /*pFileProp,*/ NULL, NULL,
									userData,userDataSize);

  if(!Code){  
		nb_unlink(FileName);
    if(BakFlag)
      RestoreFromBakFile(FileName);
    return false;
  }
	
/*	memset(MsgStr, 0, sizeof(MsgStr));
	strncpy(MsgStr, get_message(TO_FILE)->str, sizeof(MsgStr) - 1);
	strncat(MsgStr, FileName, sizeof(MsgStr) - strlen(MsgStr) - 1);
	strncat(MsgStr, get_message(SAVE_OBJECTS)->str, sizeof(MsgStr) - strlen(MsgStr) - 1);
	strncat(MsgStr, itoa(Count, Buf, 10), sizeof(MsgStr) - strlen(MsgStr) - 1);
	put_message(EMPTY_MSG, MsgStr, 2);*/
  return true;
}
