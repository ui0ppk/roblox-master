#include "../sg.h"


BOOL b_np_lab(lpNP_STR_LIST list_str, lpNP_STR str, lpNPW np)
{
	short      	index;
	short      	i, k, max=0, ident;
	NP_STR   		str_n;
	BOOL     		rt = TRUE;
	short * 	mem_ident;

	if ( (mem_ident = (short*)SGMalloc(sizeof(np->ident)*(np->noe))) == NULL )	return FALSE;

	for (i=1 ; i <= np->noe ; i++) {
		if (np->efc[i].fp >= 0 && np->efc[i].fm >= 0) continue;
		if (np->efc[i].fp < 0) ident = np->efc[i].ep;
		else                   ident = np->efc[i].em;
		k = 0;
		while (k < max) {
			if (mem_ident[k] == ident) break;
			k++;
		}
		if (k < max) continue;
		mem_ident[max] = ident;
		max++;

		if ( !(np_get_str(list_str,ident,&str_n,&index)) ) { rt = FALSE; goto end; }
		if (abs(str_n.lab) == 1)  {            
			str->lab = str_n.lab;
			if (str->lab == 1 && object_number == 2 && key == SUB)
				np_inv(np,NP_ALL);
			goto end;
		}
	}
end:
	SGFree(mem_ident);
	return rt;
}
