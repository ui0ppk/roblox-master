#include "../sg.h"

BOOL define_cond_plane(short cnum, short *plflag,
											 lpD_POINT norm, lpD_PLANE pl, lpD_POINT vp,
											 short *plstatus){


short i;

	switch(*plflag){ 
		case PL_FIXED:    
			if(test_cond_to_plane(cnum)) goto met_create;
			return FALSE;  
		case PL_CURR:      
			if((dpoint_eq(&pl->v, norm, eps_n))) { 
				if(test_cond_to_plane(cnum)) goto met_create;
				goto met_pl_to_cond;
			}
			goto met_cur_pln; 
		case PL_DEF:       
			
			if(test_cond_to_plane(cnum)) goto met_create;
			goto met_pl_to_cond;
		case PL_NODEF:     
			
met_cur_pln:
			dpoint_copy(&pl->v, norm);
			pl->d = dcoef_pl(&vp[0], norm);
			for(i = 0; i <= cnum; i++){
				if(!test_cond_to_plane(i)) goto met_pl_to_cond;
			}
			*plflag = PL_CURR;
			*plstatus = 2;
			return TRUE;
	}
met_create:
	*plstatus = 0;
	return TRUE;
met_pl_to_cond:
	*plstatus = 1;
	return TRUE;
}
