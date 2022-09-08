#include "../sg.h"

// ROBLOX change
#if (SG_CURRENT_PLATFORM!=SG_PLATFORM_IOS)
   sgFloat eps_d = 1.e-5, eps_n = 1.e-3;    //  if sgFloat = double
#else
   sgFloat eps_d = 1.e-10, eps_n = 1.e-6;      //  if sgFloat = float
#endif

short isg(short r)  {

  if (r < 0) return -1;
  if (r > 0) return  1;
  return 0;
}

short sg(sgFloat r, sgFloat eps)
{
  if (r < -eps) return -1;
  if (r >  eps) return  1;
  return 0;
}

void  d_minmax(sgFloat * b, short numb,
               sgFloat * min, sgFloat * max)
{
  register short i;

  *min= 1.e35;
  *max=-1.e35;
  for ( i=0 ; i < numb ; i++ ) {
    if ( *min > b[i] ) *min=b[i];
    if ( *max < b[i] ) *max=b[i];
  }
}

void  dpoint_minmax(lpD_POINT b, short numb,
                    lpD_POINT min, lpD_POINT max)
{
  register short i;

  min->x= 1.e35;
  min->y= 1.e35;
  min->z= 1.e35;
  max->x=-1.e35;
  max->y=-1.e35;
  max->z=-1.e35;
  for ( i=0 ; i < numb ; i++ ) {
		if ( min->x > b[i].x ) min->x=b[i].x;
    if ( max->x < b[i].x ) max->x=b[i].x;
    if ( min->y > b[i].y ) min->y=b[i].y;
    if ( max->y < b[i].y ) max->y=b[i].y;
    if ( min->z > b[i].z ) min->z=b[i].z;
    if ( max->z < b[i].z ) max->z=b[i].z;
  }
}

BOOL dnormal_vector (lpD_POINT v)
{
	sgFloat cs;

	cs = sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
//	if ( cs < eps_d/10000) {
	if ( cs < MINsgFloat*1000) {

		v->x = v->y = v->z = 0.;
		return FALSE;   
	}
	v->x /= cs;                     
	v->y /= cs;
	v->z /= cs;
	return TRUE;
}

BOOL dnormal_vector1 (lpD_POINT v)
{
	sgFloat cs;

	cs = sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
	if ( cs < eps_d/10000) {
//	if ( cs < MINsgFloat*1000) {

		v->x = v->y = v->z = 0.;
		return FALSE;   //  
	}
	v->x /= cs;                      //  
	v->y /= cs;
	v->z /= cs;
	return TRUE;
}

BOOL dpoint_eq(lpD_POINT p1, lpD_POINT p2, sgFloat eps)
{
  if (fabs(p1->x - p2->x) > eps ) return FALSE;
  if (fabs(p1->y - p2->y) > eps ) return FALSE;
  if (fabs(p1->z - p2->z) > eps ) return FALSE;
	return TRUE;
}

lpF_PLPOINT dpoint_to_plpoint(lpD_POINT p1, lpF_PLPOINT p2)
{
	p2->x = (float)p1->x;
	p2->y = (float)p1->y;
	return p2;
}

lpD_POINT dpoint_inv(lpD_POINT p1, lpD_POINT p2)
{
	p2->x = - p1->x;
	p2->y = - p1->y;
	p2->z = - p1->z;
	return p2;
}

lpD_POINT dpoint_sub(lpD_POINT p1, lpD_POINT p2, lpD_POINT p3)
{
	p3->x = p1->x - p2->x;
	p3->y = p1->y - p2->y;
	p3->z = p1->z - p2->z;
	return p3;
}

lpD_POINT dpoint_add(lpD_POINT p1, lpD_POINT p2, lpD_POINT p3)
{
	p3->x = p1->x + p2->x;
	p3->y = p1->y + p2->y;
	p3->z = p1->z + p2->z;
	return p3;
}

lpD_POINT dpoint_parametr(lpD_POINT p1, lpD_POINT p2, sgFloat t, lpD_POINT p3)
{
	p3->x = p1->x + (p2->x - p1->x)*t;
	p3->y = p1->y + (p2->y - p1->y)*t;
	p3->z = p1->z + (p2->z - p1->z)*t;
	return p3;
}

sgFloat dpoint_distance(lpD_POINT p1, lpD_POINT p2)
{
	sgFloat pr;

	pr = sqrt((p1->x-p2->x)*(p1->x-p2->x)+
						(p1->y-p2->y)*(p1->y-p2->y)+
						(p1->z-p2->z)*(p1->z-p2->z));
	return pr;
}

sgFloat dskalar_product(lpD_POINT v1, lpD_POINT v2)
{
	sgFloat pr;

	pr = v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
	return pr;
}

lpD_POINT dvector_product(lpD_POINT v1, lpD_POINT v2, lpD_POINT v3)
{
		D_POINT v;

		v.x = v1->y * v2->z - v1->z * v2->y;
		v.y = v1->z * v2->x - v1->x * v2->z;
		v.z = v1->x * v2->y - v1->y * v2->x;
		*v3 = v;
		return v3;
	
}

BOOL fnormal_vector (lpF_POINT v)
{
	float cs;

	cs = (float)sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
	if ( cs < MINFLOAT*10) return FALSE;   //  
	v->x /= cs;                      //  
	v->y /= cs;
	v->z /= cs;
	return TRUE;
}

float fskalar_product(lpF_POINT v1, lpF_POINT v2)
{
	float pr;

	pr = v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
	return pr;
}

lpF_POINT fvector_product(lpF_POINT v1, lpF_POINT v2, lpF_POINT v3)
{
	F_POINT v;

	v.x = v1->y * v2->z - v1->z * v2->y;
	v.y = v1->z * v2->x - v1->x * v2->z;
	v.z = v1->x * v2->y - v1->y * v2->x;
	*v3 = v;
	return v3;
}

void  define_eps(lpD_POINT min,lpD_POINT max)
{
	sgFloat x;

  x=fabs(min->x);
  if (x < fabs(min->y)) x=fabs(min->y);
  if (x < fabs(min->z)) x=fabs(min->z);
  if (x < fabs(max->x)) x=fabs(max->x);
  if (x < fabs(max->y)) x=fabs(max->y);
  if (x < fabs(max->z)) x=fabs(max->z);

  // ROBLOX change
  #if (SG_CURRENT_PLATFORM!=SG_PLATFORM_IOS)
     eps_d = x * 1.e-5;    //   if sgFloat = double
  #else
    eps_d = x * 1.e-5;    //   if sgFloat = float
  #endif
}

BOOL dpoint_cplane(lpD_POINT p1, lpD_POINT p2, lpD_POINT p3,
                        lpD_POINT pr)
{
  pr->x=(p2->y-p1->y)*(p3->z-p1->z)-(p2->z-p1->z)*(p3->y-p1->y);
  pr->y=(p2->z-p1->z)*(p3->x-p1->x)-(p2->x-p1->x)*(p3->z-p1->z);
  pr->z=(p2->x-p1->x)*(p3->y-p1->y)-(p2->y-p1->y)*(p3->x-p1->x);
  if ( !dnormal_vector (pr)) return FALSE;
  return TRUE;
}

float calc_body_angle(lpD_POINT p1, lpD_POINT p2, lpD_POINT p3,
                      lpD_POINT p4)
{
	sgFloat cs,alfa,ip;
	D_POINT r1,r2;
  D_POINT f1;           //    ,
  D_POINT f2;           //    ,

  if ( !dpoint_cplane(p1, p2, p3, &f1)) return 0.;
  if ( !dpoint_cplane(p3, p2, p4, &f2)) return 0.;

  cs = dskalar_product(&f1, &f2);
  if (cs <= eps_n - 1.) return 0;
	if (cs >= 1. - eps_n) alfa = M_PI;
	else {
		alfa = acos(cs);
    dpoint_sub(p3,p2,&r1);     //   r
    if (!(dnormal_vector(&r1))) return 0;      // 
    dvector_product(&f1, &f2, &r2);   //    
    if (!(dnormal_vector(&r2)))  return 0;     // 
 		ip = dskalar_product(&r1, &r2);
		if (ip < 0.) alfa = M_PI - alfa;
		else         alfa = M_PI + alfa;
	}
	return (float)alfa;
}

void modify_limits_by_point(lpD_POINT p, lpD_POINT min, lpD_POINT max)
{
	if (p->x < min->x) min->x = p->x;
	if (p->y < min->y) min->y = p->y;
	if (p->z < min->z) min->z = p->z;
	if (p->x > max->x) max->x = p->x;
	if (p->y > max->y) max->y = p->y;
	if (p->z > max->z) max->z = p->z;
}

sgFloat round(sgFloat din, short precision)
{
	sgFloat DEGREE, d;

	DEGREE = pow(10.0, precision);
	d = din * DEGREE + 0.5;
	return floor(d)/DEGREE;
}

BOOL gauss_classic( sgFloat **A, sgFloat *y, int u ){
int i, j, k, l, n, m;
sgFloat s;

	u--;
	for(n=0; n<=u; n++){
		for( k=-1, l=n; l<=u; l++ ){
			if( fabs(A[l][n]) > eps_n ){
				k=l;
				break;
			}
		}
		if( k == -1 ) return FALSE;
		if( k != n ){
			for( m=n; m<=u; m++ ) change_AB(&A[n][m], &A[k][m]);
			change_AB(&y[n], &y[k]);
		}
		s=1./A[n][n];
		for(j=u; j>=n; j-- ) A[n][j] *= s;
		y[n] *= s;
		for( i=k+1; i<=u; i++){
			for( j=n+1; j<=u; j++) A[i][j] -= A[i][n]*A[n][j];
			y[i] -= A[i][n]*y[n];
		}
	}
	for( i=u; i>=0; i--)
		for( k=i-1; k>=0; k-- )	if( fabs(A[k][i]) > eps_n )	y[k] -= A[k][i]*y[i];

	return TRUE;
}



