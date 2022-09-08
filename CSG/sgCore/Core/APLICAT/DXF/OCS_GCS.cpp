
#include "../../sg.h"

void ocs_gcs(lpD_POINT N, MATR A, MATR iA)
{
	D_POINT Wy = {0, 1, 0}, Wz = {0, 0, 1}, Ax, Ay;
	sgFloat c = 1. / 64.;

	
	if (fabs(N->x) < c && fabs(N->y) < c) dvector_product(&Wy, N, &Ax);
	else 																	dvector_product(&Wz, N, &Ax);
	dnormal_vector(&Ax);
	dvector_product(N, &Ax, &Ay);
	dnormal_vector(&Ay);
	o_hcunit(A);
	A[0]  = Ax.x;
	A[1]  = Ay.x;
	A[2]  = N->x;
	A[4]  = Ax.y;
	A[5]  = Ay.y;
	A[6]  = N->y;
	A[8]  = Ax.z;
	A[9]  = Ay.z;
	A[10] = N->z;
	o_minv(A, iA);
}
