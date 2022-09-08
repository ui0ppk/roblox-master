#include "../sg.h"

/**********************************************************
*  Free memory for matrix m x n
*/
void free_matrix( sgFloat **L, short n ){
	while( n > 0 ) SGFree( L[--n] );
	SGFree(L);
}

/**********************************************************
* Expand memory for matrix m x n
* m - number of rows
* n - number of colomns
* return =**L - if memory expand; or = NULL
*/
sgFloat ** mem_matrix( short m, short n ){
	short i;
	sgFloat **L;

	if( (L = (sgFloat**)SGMalloc( sizeof(sgFloat*) * m) ) == NULL) goto err;
	for( i = 0; i < m; i++ ) {
		if( (L[i] = (sgFloat*)SGMalloc(sizeof(sgFloat) * n) ) == NULL ){
			free_matrix( L, i );
			goto err;
		}
	}
	return L;
err:
	nurbs_handler_err(SPL_NO_MEMORY);
	return NULL;
}

//-------------------------------------->>>>>>>>>>>>
/**********************************************************
*  Free memory for matrix m x n
*/
void free_matrix_point( lpD_POINT *L, short n ){
	while( n > 0 ) SGFree( L[--n] );
	SGFree(L);
}

/**********************************************************
* Expand memory for matrix m x n
* m - number of rows
* n - number of colomns
* return =**L - if memory expand; or = NULL
*/
lpD_POINT * mem_matrix_point( short m, short n ){
	short i;
	lpD_POINT *L;

	if( (L = (D_POINT**)SGMalloc( sizeof(lpD_POINT) * m) ) == NULL) goto err;
	for( i = 0; i < m; i++ ) {
		if( (L[i] = (D_POINT*)SGMalloc(sizeof(D_POINT) * n) ) == NULL ){
			free_matrix_point( L, i );
			goto err;
		}
	}
	return L;
err:
	nurbs_handler_err(SPL_NO_MEMORY);
	return NULL;
}
