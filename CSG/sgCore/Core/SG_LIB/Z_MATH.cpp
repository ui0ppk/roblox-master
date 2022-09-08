#include "../sg.h"

BOOL eq_line(lpD_POINT p1, lpD_POINT p2, sgFloat *a, sgFloat *b, sgFloat *c) {

	sgFloat r;

	*a = (p1->y - p2->y);
	*b = (p2->x - p1->x);
	r = hypot(*a, *b);
	if (r < MINsgFloat*10) return FALSE;
	*a /= r; *b /= r;
	*c = - *a * p1->x - *b * p1->y;
	return TRUE;
}

//    
BOOL eq_pe_line(lpD_POINT p1, lpD_POINT p2, sgFloat *a, sgFloat *b, sgFloat *c) {
sgFloat w;

	if(!eq_line(p1, p2, a, b, c)) return FALSE;
	*c  = ((*a)*(p1->y + p2->y) - (*b)*(p1->x + p2->x))/2.;
	w = *a; *a = *b; *b = -w;
	return TRUE;
}

//----------------------------------------------
//    
short p_int_lines(sgFloat a1, sgFloat b1, sgFloat c1,
								sgFloat a2, sgFloat b2, sgFloat c2,
								lpD_POINT p) {

	sgFloat d;

	d = a1 * b2 - a2 * b1;
	if (fabs(d) < eps_n * eps_n) return FALSE;
	p->x = (- c1 * b2 + c2 * b1) / d;
	p->y = (- a1 * c2 + a2 * c1) / d;
	p->z = 0.;
	return TRUE;
}

//-------------------------------------------------
BOOL normv(BOOL flag_3d, lpD_POINT v, lpD_POINT n)

{
sgFloat r, z = 0.;
	if(flag_3d)
		z = v->z;
	if((r = sqrt(v->x*v->x + v->y*v->y + z*z)) < MINsgFloat*10){
    n->x = n->y = n->z = 0.;
    return FALSE;
	}
  n->x = v->x/r;
  n->y = v->y/r;
  n->z = z/r;
	return TRUE;
}

//-------------------------------------------------------------
sgFloat v_angle(sgFloat x, sgFloat y)
{
sgFloat a;
	if(fabs(x) < MINsgFloat*10 && fabs(y) < MINsgFloat*10)
		return 0.;
	a = atan2(y, x);
	if(fabs(a) < eps_n) a = 0.;
	else
		while(a < 0.)
			a += 2.*M_PI;
	return a;
}

//----------------------------------------------------------
void min_max_2(sgFloat x1, sgFloat x2, sgFloat *mn, sgFloat *mx)
//        
{
	if(x1 > x2){
		*mx = x1;
		*mn = x2;
	}
	else{
		*mx = x2;
		*mn = x1;
	}
}

//--------------------------------------------------------------------
sgFloat dist_p_l(lpD_POINT vl, lpD_POINT cl, lpD_POINT p, lpD_POINT pl)

{
sgFloat d;

	d = - cl->x*p->x - cl->y*p->y - cl->z*p->z;
	intersect_3d_pl(cl, d, vl, cl, pl);
	return dpoint_distance(p, pl);
}

//----------------------------------------------------------------------
short intersect_3d_pl(lpD_POINT np, sgFloat dp, lpD_POINT vl, lpD_POINT cl,
										lpD_POINT p)
      
{
sgFloat c, d;

	c = np->x*cl->x + np->y*cl->y + np->z*cl->z;
	d = np->x*vl->x + np->y*vl->y + np->z*vl->z + dp;
	if(fabs(c) < eps_n){
		if(fabs(d) < eps_d)
			return 0;
		return -1;
	}
	d = -d/c;
	dpoint_add(vl, dpoint_scal(cl, d, p), p);
	return 1;
}

//----------------------------------------------------
BOOL eq_plane(lpD_POINT p1, lpD_POINT p2, lpD_POINT p3,
						 lpD_POINT np, sgFloat *dp)
 
{
	np->x = (p1->y - p2->y)*(p1->z - p3->z) - (p1->y - p3->y)*(p1->z - p2->z);
	np->y = (p1->z - p2->z)*(p1->x - p3->x) - (p1->z - p3->z)*(p1->x - p2->x);
	np->z = (p1->x - p2->x)*(p1->y - p3->y) - (p1->x - p3->x)*(p1->y - p2->y);
	*dp = hypot(hypot(np->x, np->y), np->z);
	if(*dp < MINsgFloat*10)
		return FALSE;
	dpoint_scal(np, 1./(*dp), np);
	*dp = -p1->x*(np->x) - p1->y*(np->y) - p1->z*(np->z);
	return TRUE;
}

//--------------------------------------------------
sgFloat dist_p_pl(lpD_POINT p, lpD_POINT n, sgFloat d)
//     (p)  - (n, d)
{
	return n->x*p->x + n->y*p->y + n->z*p->z + d;
}
//--------------------------------------------------
sgFloat dcoef_pl(lpD_POINT p, lpD_POINT n)
{
	return -(n->x*p->x + n->y*p->y + n->z*p->z);
}

//---------------------------
short best_crd_pln(lpD_POINT n)
{
sgFloat amn, a1;
short ret = 3;

  if((a1 = fabs(n->x) + fabs(n->z)) < (amn = fabs(n->x) + fabs(n->y))){
		amn = a1;
		ret = 2;
	}
	if(fabs(n->y) + fabs(n->z) < amn)
		ret = 1;
	return ret;
}

short coll_tst(lpD_POINT v1, BOOL norm1,
             lpD_POINT v2, BOOL norm2)
{

D_POINT w1, w2;

	if(!norm1){
		if(!normv(TRUE, v1, &w1))
			return 1;
	}
	else
    w1 = *v1;
	if(!norm2){
		if(!normv(TRUE, v2, &w2))
			return 1;
	}
	else
    w2 = *v2;

	if(dpoint_eq(&w1, &w2, eps_n))
		return 1;
	if(dpoint_eq(&w1, dpoint_inv(&w2, &w2), eps_n))
		return -1;
	return 0;
}

sgFloat det3_3(sgFloat *a, sgFloat *b, sgFloat *c){

	return  a[0]*b[1]*c[2] + b[0]*c[1]*a[2] + c[0]*a[1]*b[2] -
          c[0]*b[1]*a[2] - a[0]*c[1]*b[2] - b[0]*a[1]*c[2];
}

short qwadr_eq(sgFloat a, sgFloat b, sgFloat c, sgFloat* x){
//   a*x*x + b*x +c = 0
sgFloat d;

	if(fabs(a) < eps_n){
		if(fabs(b) < eps_d) return 0;
		x[0] = -c/b;
		return 1;
	}
	b /= a; c /= a;
	if(fabs(d = (b*b)/4 - c) < eps_d*eps_d) d = 0.;
	if(d < 0.) return 0;
	x[0] = -b/2;
	d = sqrt(d);
	if(fabs(d) < eps_d) return 1;
	x[1] = x[0] - d; x[0] = x[0] + d;
	return 2;
}

sgFloat dpoint_distance_2d(lpD_POINT p1, lpD_POINT p2){

	 return hypot(p1->x - p2->x, p1->y - p2->y);
}

lpD_POINT dpoint_scal(lpD_POINT p1, sgFloat r, lpD_POINT p2){
  
	p2->x = p1->x*r;
	p2->y = p1->y*r;
	p2->z = p1->z*r;
	return p2;
}

sgFloat dist_point_to_segment(lpD_POINT p, lpD_POINT v1, lpD_POINT v2,
													lpD_POINT p_min)
{
	int				i;              //    
	sgFloat		f1, f2;
	D_POINT		n;							//   
	D_POINT		pp_min;
	DA_POINT  vn, vk, vp;

	dpoint_sub(v1, v2, &n);         //   
	if ( !dnormal_vector(&n) ) {		//  
		*p_min = *v1;
		return ( dpoint_distance(v1, p) );
	}
	f1 = dist_p_l( v1, &n, p, &pp_min);   //   

	i = best_crd_pln(&n) - 1;
	memcpy(&vn,v1,sizeof(vn));
	memcpy(&vk,v2,sizeof(vk));
	if ( vn[i] > vk[i]) {
		memcpy(&vn,v2,sizeof(vn));
		memcpy(&vk,v1,sizeof(vk));
	}

	memcpy(&vp,&pp_min,sizeof(vp));
	if ( vp[i] >= vn[i] && vp[i] <= vk[i] ) { //   
		*p_min = pp_min;
		return ( f1 );
	} else {			//    
		f1 = dpoint_distance(v1, p);
		f2 = dpoint_distance(v2, p);
		if ( f1 > f2 ) { *p_min = *v2; return f2; }
		else           { *p_min = *v1; return f1; }
	}
}

sgFloat floor_crd_par(sgFloat crd, sgFloat step){
short sign = 1;

	if(crd < 0.) {
		crd = -crd;
		sign = -1;
	}
	return sign*floor(crd/step + 0.5)*step;
}

#define MAX_FAF 4

static char      faf_num;
static char      faf_type[MAX_FAF];
static D_PLPOINT faf_p[MAX_FAF];

void faf_init(void){
// 
	faf_num = 0;
}
void faf_set(short type, sgFloat x, sgFloat y){

	faf_p[faf_num].x = x;
	faf_p[faf_num].y = y;
	faf_type[faf_num++] = (BYTE)type;
}
void faf_point(lpD_POINT p1, lpD_POINT p2){

short    num = faf_num - 1;
sgFloat d;

	*p2 = *p1;
	if(faf_num){
		switch(faf_type[num]){
			case FAF_MOVE:
				p2->x = p1->x + faf_p[num].x;
				p2->y = p1->y + faf_p[num].y;
				break;
			case FAF_ROTATE:
				d     = p1->x*faf_p[num].y - p1->y*faf_p[num].x;
				p2->y = p1->x*faf_p[num].x + p1->y*faf_p[num].y;
				p2->x = d;
		}
	}
	return;
}
void faf_line(lpD_POINT p1, lpD_POINT p2){
  
short    num = faf_num - 1;
sgFloat d;
	*p2 = *p1;
	if(faf_num){
		switch(faf_type[num]){
			case FAF_MOVE:
				p2->x = p1->x;
				p2->y = p1->y;
				p2->z = p1->z - p1->x*faf_p[num].x - p1->y*faf_p[num].y;
				break;
			case FAF_ROTATE:
				d     = p1->x*faf_p[num].y - p1->y*faf_p[num].x;
				p2->y = p1->x*faf_p[num].x + p1->y*faf_p[num].y;
				p2->x = d;
				d = hypot(p2->x, p2->y);
				p2->x /= d;
				p2->y /= d;
				p2->z = p1->z/d;
		}
	}
	return;
}
void faf_inv(lpD_POINT p1, lpD_POINT p2){
     
short    num = faf_num;
sgFloat d;
	*p2 = *p1;
	if(faf_num){
		do{
			switch(faf_type[--num]){
				case FAF_MOVE:
					p2->x = p2->x - faf_p[num].x;
					p2->y = p2->y - faf_p[num].y;
					break;
				case FAF_ROTATE:
					d     =  p2->x*faf_p[num].y + p2->y*faf_p[num].x;
					p2->y = -p2->x*faf_p[num].x + p2->y*faf_p[num].y;
					p2->x = d;
			}
		}while (num);
	}
	return;
}


void faf_exe(lpD_POINT p1, lpD_POINT p2){
    
short    i;
sgFloat d;

	*p2 = *p1;
	for(i = 0; i < faf_num; i++){
		switch(faf_type[i]){
			case FAF_MOVE:
				p2->x = p2->x + faf_p[i].x;
				p2->y = p2->y + faf_p[i].y;
				break;
			case FAF_ROTATE:
				d     = p2->x*faf_p[i].y - p2->y*faf_p[i].x;
				p2->y = p2->x*faf_p[i].x + p2->y*faf_p[i].y;
				p2->x = d;
		}
	}
	return;
}


sgFloat get_point_par(lpD_POINT pb, lpD_POINT pe, lpD_POINT p)
{
sgFloat lx, ly, a;

	lx = pe->x - pb->x;
	ly = pe->y - pb->y;
	if(fabs(lx) > fabs(ly)){
	 if(fabs(lx) < eps_d) return 0.;
	 a = (p->x - pb->x)/lx;
	}
	else{
		if(fabs(ly) < eps_d) return 0.;
		a = (p->y - pb->y)/ly;
	}
	return a;
}

sgFloat get_point_par_3d(lpD_POINT pb, lpD_POINT pe, lpD_POINT p)
{
	sgFloat lx, ly, lz, a;

	lx = pe->x - pb->x;
	ly = pe->y - pb->y;
	lz = pe->z - pb->z;
	if ( fabs(lx) > fabs(ly) ) {
		if ( fabs(lx) > fabs(lz) ) {
			if ( fabs(lx) < eps_d ) return 0.;
			a = (p->x - pb->x)/lx;
		} else {
			if ( fabs(lz) < eps_d ) return 0.;
			a = (p->z - pb->z)/lz;
		}
	} else {
		if ( fabs(ly) > fabs(lz) ) {
			if ( fabs(ly) < eps_d ) return 0.;
			a = (p->y - pb->y)/ly;
		} else {
			if ( fabs(lz) < eps_d ) return 0.;
			a = (p->z - pb->z)/lz;
		}
	}
	return a;
}

BOOL get_point_proection(lpD_POINT pb, lpD_POINT pe, lpD_POINT p,
												 lpD_POINT pp)
{
sgFloat a, b, c, c1;

	if(!eq_line(pb, pe, &a, &b, &c))
		return FALSE;
	c1 = -(p->x*(-b) + p->y*a);
	if(!p_int_lines(a, b, c,  -b, a, c1,  pp))
		return FALSE;
	return TRUE;
}

BOOL  correct_line_tails(lpD_POINT p1, lpD_POINT p2, sgFloat h1, sgFloat h2)

{
sgFloat  l, a1, a2;
D_POINT wp1, wp2;
	l = dpoint_distance(p1, p2);
	if (l < eps_d) return FALSE;
	a1 = h1/l;
	a2 = h2/l + 1;
	if((a2 - a1) < eps_n) return FALSE;
	dpoint_parametr(p1, p2, a1, &wp1);
	dpoint_parametr(p1, p2, a2, &wp2);
	*p1 = wp1;
	*p2 = wp2;
	return TRUE;
}


sgFloat z_poly(sgFloat x, int degree, sgFloat* coeffs)
//   poly  Borlnda
{
int    i   = 1;
sgFloat num;

  if(degree < 0)
    return 0.;
  num = coeffs[0];
  while(i <= degree){
    num += pow(x, i)*coeffs[i];
    i++;
  }
  return num;
}


bool Is2DPointInside2DPolygon(const SG_POINT *polygon,int N, const SG_POINT& p)
{
	int counter = 0;
	int i;
	sgFloat xinters;
	SG_POINT p1,p2;
	p1 = polygon[0];
	for (i=1;i<=N;i++) 
	{
		p2 = polygon[i % N];
		if (p.y > min(p1.y,p2.y)) 
		{
			if (p.y <= max(p1.y,p2.y)) 
			{
				if (p.x <= max(p1.x,p2.x)) 
				{
					if (fabs(p1.y-p2.y)>eps_d) 
					{
						xinters = (p.y-p1.y)*(p2.x-p1.x)/(p2.y-p1.y)+p1.x;
						if (fabs(p1.x-p2.x)<eps_d || p.x <= xinters)
							counter++;
					}
				}
			}
		}
		p1 = p2;
	}
	if (counter % 2 == 0)
		return false;
	else
		return true;
}

bool CalcPointOf3PlanesInter(lpD_PLANE pPln1, lpD_PLANE pPln2, lpD_PLANE pPln3, lpD_POINT pPnt)
{
    sgFloat  a[3], b[3], c[3], d[3], det;
    D_POINT Pnt;

    a[0] = pPln1->v.x; a[1] = pPln2->v.x;   a[2] = pPln3->v.x;
    b[0] = pPln1->v.y; b[1] = pPln2->v.y;   b[2] = pPln3->v.y;
    c[0] = pPln1->v.z; c[1] = pPln2->v.z;   c[2] = pPln3->v.z;
    d[0] = pPln1->d;   d[1] = pPln2->d;     d[2] = pPln3->d;

    det = det3_3(a, b, c);
    if(fabs(det) < eps_n)
        return false;
    Pnt.x = -det3_3(d, b, c)/det;
    Pnt.y = -det3_3(a, d, c)/det;
    Pnt.z = -det3_3(a, b, d)/det;
    *pPnt = Pnt;
    return true;
}

bool CalcAxisOfPlanesInter(lpD_PLANE pPln1, lpD_PLANE pPln2, pD_AXIS pAx)
{
    D_AXIS  Ax;
    D_PLANE Pln;

    dvector_product(&pPln1->v, &pPln2->v, &Ax.v);
    if(!normv(TRUE, &Ax.v, &Ax.v))
        return false;
    Pln.v = Ax.v; Pln.d = 0.;
    if(!CalcPointOf3PlanesInter(pPln1, pPln2, &Pln, &Ax.p))
        return false;
    *pAx = Ax;
    return true;
}


