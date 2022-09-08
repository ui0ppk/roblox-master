#include "../sg.h"


void b_av(short mt, short *p,short *pr,short *pt, BOOL *le)
{

  if (abs(mt) == 1) {
		*p=-*p;
		return;
	}
	if (abs(mt) == 4) return;
	if (mt == 2) {
		if (*pr <  0) {   
			*pt=*p;  *p=1;  *pr=1;
			*le = TRUE;
			return;
    } else {         
			*p=*pt;  *pr=-1;
			*le = FALSE;
			return;
		}
	} else {  /* mt=3 */
		if (*pr < 0) {  
			*pt=*p;  *p=1;  *pr=1;
			*le = TRUE;
			return;
		} else {       
			*p=-*pt;  *pr=-1;
			*le = FALSE;
			return;
		}
	}
}
