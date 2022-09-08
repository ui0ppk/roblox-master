#include "../sg.h"

BOOL b_fa(lpNPW np)
{
	short        mf;
	short        face;
	lpNP_LOOPS l = NULL;
	short        loops = 0;      				
	BOOL       rt = TRUE, msg = TRUE;

	if ( (l = np_realloc_loops(l,&loops)) == NULL ) return FALSE;
	mf = np->nof;									           
	for (face = 1 ; face <= mf ; face++) {
		if ( !np_fa_one(np, face, &l, &loops, msg) ) {
			rt = FALSE;
			goto end;
		}
	}
	if (np->nof > mf) np_fa_ef(np,mf);
	mf = b_fa_lab1(np);
	if (mf == 0) { rt = FALSE; goto end; }
	if (mf != np->nof)
		if ( !(b_fa_lab2(np)) ) rt = FALSE;
end:
	SGFree(l);
	return rt;
}
