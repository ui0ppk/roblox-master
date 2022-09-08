#include "../sg.h"
static void first_deriv( short num_of_eqv, short p_c, sgFloat *u_c, sgFloat *U_c,
													sgFloat **A, short i, short num, short beg_end );
static void second_deriv( short num_of_eqv, short p_c, sgFloat *u_c, sgFloat *U_c,
													sgFloat **A, short i, short num, short beg_end );
static void mash_string(sgFloat **A, short num_of_eqv, short string,
												lpW_NODE P, short type );
/**********************************************************
*  Create matrix for nurbs curve
*  n_c - number of points on curve
*  p_c - degree of curve
*  U_c - knot vector for curve
*  A   - matrix
*  gran = 0 - free NURBS;                1 - closed NURBS
*         2 - free NURBS with derivates; 3 - closed NURBS with derivates
*/
void nurbs_matrix_create( lpSPLY_DAT sply_dat, short p_c, sgFloat **A, short gran ){
	short i, j, k, num_of_eqv, n_c;

	n_c = sply_dat->numk-1;
	num_of_eqv=n_c+sply_dat->numd;
	if( gran==1 || gran==3 )	num_of_eqv += 1;

	switch( gran){
		case 0:  //free NURBS
			A[0][0] = 1;
			for( i=1 ; i<n_c; i++ )
				for( j=0; j<=num_of_eqv; j++ )
					if( sply_dat->U[j] <= sply_dat->u[i] &&
							sply_dat->u[i] <= sply_dat->U[j+p_c+1] )
						A[i][j] = nurbs_basis_func( j, p_c, sply_dat->U, sply_dat->u[i] );
			A[n_c][n_c] = 1;
			break;
		case 1:  //closed NURBS
			A[0][0] = 1;
//first derivates into begining point
			first_deriv( num_of_eqv+1, p_c, sply_dat->u, sply_dat->U, A, 0, 1, 0 );
//first derivates into end point
			first_deriv( num_of_eqv+1, p_c, sply_dat->u, sply_dat->U, A, n_c, 1, 1 );

			mash_string ( A, num_of_eqv, 1, &sply_dat->P[1], 0 );

			for( i=2 ; i<n_c; i++ )
				for( j=0; j<=num_of_eqv; j++ )
					if( sply_dat->U[j]   <= sply_dat->u[i-1] &&
							sply_dat->u[i-1] <= sply_dat->U[j+p_c+1] )
						A[i][j] = nurbs_basis_func( j, p_c, sply_dat->U, sply_dat->u[i-1] );

//because have changed U-vector
			for( j=0; j<=num_of_eqv+1; j++ )
					if( sply_dat->U[j]   <= sply_dat->u[i-1] &&
							sply_dat->u[i-1] <= sply_dat->U[j+p_c+1] )
						A[n_c][j%(n_c+2)] = nurbs_basis_func( j, p_c, sply_dat->U, sply_dat->u[i-1] );

//second derivates into begining point
			second_deriv( num_of_eqv+1, p_c, sply_dat->u, sply_dat->U, A, 0, n_c+1, 0 );
//second derivates into end point
			second_deriv( num_of_eqv+1, p_c, sply_dat->u, sply_dat->U, A, n_c, n_c+1, 1 );

			mash_string ( A, num_of_eqv, n_c+1, &sply_dat->P[n_c], 0  );

			break;
		case 2://free NURBS with derivates
			for( k=0, i=0 ; i<n_c; i++, k++ ){
				for( j=0; j<=num_of_eqv; j++ )
					if( sply_dat->U[j] <= sply_dat->u[i] &&
							sply_dat->u[i] <= sply_dat->U[j+p_c+1] )
						A[k][j] = nurbs_basis_func( j, p_c, sply_dat->U, sply_dat->u[i] );
				if( sply_dat->derivates[i].num ){
					k++;
					first_deriv( num_of_eqv, p_c, sply_dat->u, sply_dat->U, A, i, k, 0 );
					for( j=0; j<=num_of_eqv; j++ ) A[k][j] *=p_c;
					mash_string ( A, num_of_eqv, k, &sply_dat->P[k], 1  );
				}
			}
			if( sply_dat->derivates[n_c].num ){
//				k++;
				first_deriv( num_of_eqv, p_c, sply_dat->u, sply_dat->U, A, n_c, k, 0 );
				for( j=0; j<=num_of_eqv; j++ ) A[k][j] *=p_c;
				mash_string ( A, num_of_eqv, k, &sply_dat->P[k], 1  );
			}
			A[num_of_eqv][num_of_eqv]=1;
			break;
		case 3://closed NURBS with derivates
			A[0][0] = 1;
//first derivates into begining point
			first_deriv( num_of_eqv+1, p_c, sply_dat->u, sply_dat->U, A, 0, 1, 0 );
//first derivates into end point
			first_deriv( num_of_eqv+1, p_c, sply_dat->u, sply_dat->U, A, n_c, 1, 1 );
			mash_string ( A, num_of_eqv, 1, &sply_dat->P[1], 0 );
			k=2;
			if( sply_dat->derivates[0].num ){
				first_deriv( num_of_eqv, p_c, sply_dat->u, sply_dat->U, A, 0, k, 0 );
				for( j=0; j<=num_of_eqv; j++ ) A[k][j] *=p_c;
				mash_string ( A, num_of_eqv, k, &sply_dat->P[k], 1  );
				k++;
			}

			for( i=1; i<n_c; i++, k++ ){
				for( j=0; j<=num_of_eqv; j++ )
					if( sply_dat->U[j] <= sply_dat->u[i] &&
							sply_dat->u[i] <= sply_dat->U[j+p_c+1] )
						A[k][j] = nurbs_basis_func( j, p_c, sply_dat->U, sply_dat->u[i] );
				if( sply_dat->derivates[i].num ){
					k++;
					first_deriv( num_of_eqv, p_c, sply_dat->u, sply_dat->U, A, i, k, 0 );
					for( j=0; j<=num_of_eqv; j++ ) A[k][j] *=p_c;
					mash_string ( A, num_of_eqv, k, &sply_dat->P[k], 1  );
				}
			}

/*			if( sply_dat->derivates[n_c].num ){
//				k++;
				first_deriv( num_of_eqv, p_c, sply_dat->u, sply_dat->U, A, n_c, k, 0 );
				for( j=0; j<=num_of_eqv; j++ ) A[k][j] *=p_c;
				mash_string ( A, num_of_eqv, k, &sply_dat->P[k], 1  );
				k++;
			}
*/
//second derivates into begining point
			second_deriv( num_of_eqv+1, p_c, sply_dat->u, sply_dat->U, A, 0, k, 0 );
//second derivates into end point
			second_deriv( num_of_eqv+1, p_c, sply_dat->u, sply_dat->U, A, n_c, k, 1 );
			mash_string ( A, num_of_eqv, k, &sply_dat->P[k], 0  );
			break;
	}
}

/***********************************************************
* first derivates into point
* beg_num = 0 - points from 0 till n-1
* beg_num = 1 - point n
*/
static void first_deriv( short num_of_eqv, short p_c, sgFloat *u_c, sgFloat *U_c,
													sgFloat **A, short i, short num, short beg_end ){
//													sgFloat A[25][25], short i, short num, short beg_end ){
short j;
sgFloat t;
	for( j=1; j<=num_of_eqv; j++ ){
		if( U_c[j] <= u_c[i] && u_c[i] <= U_c[j+p_c] ){
			if( (t = U_c[j+p_c]-U_c[j] ) >= eps_n )
					 t = nurbs_basis_func( j, p_c-1, U_c, u_c[i] )/t;
			else t = 0.;
			if(!beg_end){
				A[num][j-1] -= t;
				A[num][j  ] += t;
			}else{
				A[num][j-1]          +=t;
				A[num][j%num_of_eqv] -=t;
			}
		}
	}
}

/***********************************************************
* second derivates into point
* beg_num = 0 - points from 0 till n-1
* beg_num = 1 - point n
*/
static void second_deriv( short num_of_eqv, short p_c, sgFloat *u_c, sgFloat *U_c,
													sgFloat **A, short k, short num, short beg_end ){
//													sgFloat A[25][25], short k, short num, short beg_end ){
short i, j;
sgFloat t, t1, t2;
	for( j=2; j<=num_of_eqv; j++ ){
		if( U_c[j] <= u_c[k] && u_c[k] <= U_c[j+p_c-1] ){
			i=j-1;
			if( (t = U_c[i+p_c]-U_c[i+1]) >= eps_n )
         		t = nurbs_basis_func( j, p_c-2, U_c, u_c[k] )/t;
      	else	t = 0.;
			if( (t1 = U_c[i+p_c+1]-U_c[i+1]) >= eps_n )	 t1 = 1./t1;
         else	t1 = 0.;
			if( (t2 = U_c[i+p_c  ]-U_c[i  ]) >= eps_n )  t2 = 1./t2;
         else	t2 = 0.;
			if(!beg_end){
				A[num][j-2] += t*t2;
				A[num][j-1] -= t*(t1+t2);
				A[num][j]   += t*t1;
			}else{
				A[num][j-2]       -= t*t2;
				A[num][j-1]	      += t*(t1+t2);
				A[num][j%num_of_eqv] -= t*t1;
			}
		}
	}
}

/***********************************************************
* masht. of string coeffisients to be <= 1
*/
static void mash_string(sgFloat **A, short num_of_eqv, short string,
												lpW_NODE P, short type ){
short    j;
sgFloat max;
	for( max=0., j=0; j<=num_of_eqv; j++ )
		if( fabs(A[string][j]) > max ) max=fabs(A[string][j]);
	if( fabs(max) <= eps_n ) return;
	for( j=0; j<=num_of_eqv; j++ ) A[string][j] /= max;
	if( type ) for( j=0; j<3; j++ ) P->vertex[j] /=max;
//  (*P)[j] /= max;
}

/**********************************************************
* Solution of equations sistem A*x=y, range of A = u
*/
BOOL gauss( sgFloat **A, lpW_NODE y, short u ){
//BOOL gauss( sgFloat A[25][25], lpDA_POINT y, short u ){
short i, j, k, l, n, m, t;
sgFloat s;

	for(n=0; n<=u; n++){
		for( k=-1, l=n; l<=u; l++ ){
			if( A[l][n] != 0 ){
				k=l;
				break;
			}
		}
		if( k == -1 ) return FALSE;
		if( k != n ){
			for( m=n; m<=u; m++ ) change_AB(&A[n][m], &A[k][m]);
			for( t=0; t<3; t++ )  change_AB(&y[n].vertex[t], &y[k].vertex[t]);
		}
		s=1/A[n][n];
		for( t=0; t<3; t++ ) y[n].vertex[t] *= s;
		for(j=u; j>=n; j-- ) A[n][j] *= s;
		for( i=k+1; i<=u; i++){
			for( j=n+1; j<=u; j++)
				A[i][j] -= A[i][n]*A[n][j];
			for( t=0; t<3; t++ )   y[i].vertex[t] -= A[i][n]*y[n].vertex[t];
		}
	}
	for( i=u; i>=0; i--)
		for( k=i-1; k>=0; k-- ){
			if( fabs(A[k][i]) > eps_n )
				for( t=0; t<3; t++ )	y[k].vertex[t] = y[k].vertex[t] - A[k][i]*y[i].vertex[t];
		}
	return TRUE;
}

//-------------------------------------------------------->
void change_AB( sgFloat *i, sgFloat *j ){
	sgFloat k;
	k = *i;	*i = *j;	*j = k;
}
