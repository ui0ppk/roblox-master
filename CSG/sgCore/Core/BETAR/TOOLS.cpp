#include "../sg.h"

//
// Show a message box
//
UINT Message(UINT uiBtns, LPSTR lpFormat, ...)
{
/*		char buf[256];

		wvsprintf(buf, lpFormat, (LPSTR)(&lpFormat+1));
		MessageBeep(uiBtns ? uiBtns : MB_ICONEXCLAMATION);
		return (UINT) MessageBox(NULL,
														 buf,
														 "oo",
														 uiBtns ? uiBtns : MB_OK|MB_ICONEXCLAMATION);*/return 0;
}



//
// 
//
int 
Error(LPSTR msg)
{
/*	MessageBeep(0);
	return MessageBox(NULL, msg, GetSystemName(), MB_OK);*/return 0;
} 

////////////////////////////////////////////////////////////////
//  atox -- Convert hexadecimal string to integer	      //
////////////////////////////////////////////////////////////////
UINT
atox(LPSTR str)
{
    unsigned int   	result, x1, x2;
    int  			ch;

    while ((*str == '\t') || (*str == 0x20))
		str++;

		result = 0;
    while(1) 
    {
		ch = *str;
		if ((ch >= 'a') && (ch <= 'f'))
			ch = ch - 'a' + 'A';
		if ((ch >= '0') && (ch <= 'F')) {
			x1 = (ch - '0');
			if (x1 > 9) x1 -= 7;
			x2 = (result << 4);
		    result = x2 + x1;            
		}		    
		else
		    return(result);
		str++;
		}
}


//f+
//
//
char *	GetIDS(unsigned int	id)
{
	//LoadString(this_hinstanse, id, ids, 250);
	strcpy(ids,"Error");
	return ids;
}

//f+
//
//
char *	GetOemIDS(unsigned int	id)
{
//	LoadString(hInstLD, id, ids, 250);
//	AnsiToOem(ids, ids);
	return NULL;
}

//
//
//
int GetShortDateStr(int day, int mon, int year, LPSTR	datestr)
{
int  i;
char sym;

  datestr[0] = 0;
  for(i = 0; i < 3; i++){
    if(i > 0)
      strcat(datestr, GetIDS(IDS_MSG159));  
    sym = GetIDS(IDS_MSG162 + i)[0];
    switch(sym){
      case 'Y':
	      sprintf(&datestr[strlen(datestr)], "%04d", year);
        continue;
      case 'M':
	      sprintf(&datestr[strlen(datestr)], "%02d", mon);
        continue;
      case 'D':
	      sprintf(&datestr[strlen(datestr)], "%02d", day);
        continue;
      default:
        continue;
    }
  }
	return strlen(datestr);
}

//
//
//
int GetShortTimeStr(int hour, int min, int sec, LPSTR timestr)
{
char fm[] = {"%02d:%02d:%02d"};
char sym;

  sym = GetIDS(IDS_MSG160)[0];
  if(sym)
    fm[4] = fm[9] = sym;
  if(sec < 0){
    fm[9] = 0; 
    sprintf(timestr, fm, hour, min);
  }
  else
    sprintf(timestr, fm, hour, min, sec);
	return strlen(timestr);
}  

long GetCurrentDateTime()
{
	time_t 			long_time;

	tzset();
	time(&long_time);
	return (long) long_time;
}

