#include "../sg.h"

#define DIVIDE_BY_ZERO 1.e-35

BOOL   o_trans_vector(lpMATR t,lpD_POINT base, lpD_POINT vector)
{
	D_POINT pn;
	sgFloat	dist;

	dpoint_add(base,vector,&pn);
	dist = dpoint_distance(base, &pn);
	if ( !o_hcncrd(t,base,base) ) return FALSE;
	if ( !o_hcncrd(t,&pn,&pn) )   return FALSE;
	if (dist < eps_d) {
		memset(vector, 0, sizeof(D_POINT));
		return TRUE;
	}
	dpoint_sub(&pn,base,&pn);
	if (!dnormal_vector(&pn)) return FALSE;
	dpoint_scal(&pn, dist, vector);
	return TRUE;
}

BOOL   o_hcncrd(lpMATR t,lpD_POINT v,lpD_POINT vp)
{

	sgFloat vx, vy, vz;

	vx = v->x; vy = v->y; vz = v->z;

	vp->x = vx*t[0] + vy*t[1] + vz*t[2]  + t[3];
	vp->y = vx*t[4] + vy*t[5] + vz*t[6]  + t[7];
	vp->z = vx*t[8] + vy*t[9] + vz*t[10] + t[11];
	return TRUE;
}

BOOL   o_hcncrdf(lpMATR t,lpD_POINT v,lpF_POINT vp)
{
	sgFloat vx, vy, vz;

	vx = v->x; vy = v->y; vz = v->z;

	vp->x = (float)(vx*t[0] + vy*t[1] + vz*t[2]  + t[3]);
	vp->y = (float)(vx*t[4] + vy*t[5] + vz*t[6]  + t[7]);
	vp->z = (float)(vx*t[8] + vy*t[9] + vz*t[10] + t[11]);

	return TRUE;
}

BOOL   o_hcncrd2(lpMATR t,lpD_POINT v,lpF_PLPOINT vp)
{
	sgFloat vx, vy, vz;

	vx = v->x; vy = v->y; vz = v->z;

	vp->x = (float)(vx*t[0] + vy*t[1] + vz*t[2]  + t[3]);
	vp->y = (float)(vx*t[4] + vy*t[5] + vz*t[6]  + t[7]);
	return TRUE;
}

BOOL   o_move_xyz(lpMATR matr, lpD_POINT a,lpD_POINT p)
{
	sgFloat axp,azp;
	MATR m;

	o_hcunit(m);
	if ( !o_hcrot_angle(a,&axp,&azp) ) return FALSE;
	o_tdrot(m,-3,azp);
	o_tdrot(m,-1,axp);
	o_tdtran(m,p);
	o_tdrot(m,-1,-axp);
	o_tdrot(m,-3,-azp);
	o_hcmult(matr,m);
	return TRUE;
}

BOOL   o_rotate_xyz(lpMATR matr,
									lpD_POINT p,lpD_POINT a,
									sgFloat alfa)
{
	sgFloat axp,azp;
	sgFloat m[16];

	o_hcunit(m);
	p->x = -p->x;	p->y = -p->y;	p->z = -p->z;
	o_tdtran(m,p);
	if ( !o_hcrot_angle(a,&axp,&azp)) return FALSE;
	o_tdrot(m,-3,azp);
	o_tdrot(m,-1,axp);
	o_tdrot(m,-3,alfa);
	o_tdrot(m,-1,-axp);
	o_tdrot(m,-3,-azp);
	p->x = -p->x;	p->y = -p->y;	p->z = -p->z;
	o_tdtran(m,p);
	o_hcmult(matr,m);
	return TRUE;
}

BOOL   o_get_param(lpMATR matr,
													 lpD_POINT p,
													 sgFloat  *az1,sgFloat  *ax,sgFloat  *az2,
													 sgFloat  *sx,sgFloat  *sy,sgFloat  *sz)
{
	sgFloat m[16];
	D_POINT pw,pwz;

	memcpy(m,matr,sizeof(m));
	pw.x = pw.y = pw.z = 0;
	o_hcncrd(m,&pw,p);
	pw.x = -p->x; pw.y = -p->y; pw.z = -p->z;
	o_tdtran(m,&pw);    /*   */

	pw.x = pw.y = 0; pw.z = 1;
	o_hcncrd(m,&pw,&pwz);
	if ( !o_hcrot_angle( &pwz, ax, az1)) return FALSE; /*  2  */
	o_tdrot(m,-3,*az1);
	o_tdrot(m,-1,*ax);

	pw.x = 1; pw.y = 0; pw.z = 0;
	o_hcncrd(m,&pw,&pwz);
	*az2 = -atan2(pwz.y,pwz.x);
	o_tdrot(m,-3,*az2);

	pw.x = 1; pw.y = 0; pw.z = 0;
	o_hcncrd(m,&pw,&pwz);
	o_tdscal(m,1,pwz.x);
	*sx = pwz.x;
	pw.x = 0; pw.y = 1; pw.z = 0;
	o_hcncrd(m,&pw,&pwz);
	o_tdscal(m,2,pwz.y);
	*sy = pwz.y;
	pw.x = 0; pw.y = 0; pw.z = 1;
	o_hcncrd(m,&pw,&pwz);
	o_tdscal(m,3,pwz.z);
	*sz = pwz.z;
	return TRUE;
}

BOOL   o_rot(lpD_POINT a,lpD_POINT c, sgFloat  fi,
										lpD_POINT b)
{
	sgFloat cs,s;

/*   B = [ C , A ]  */
		 b->x = c->y*a->z - c->z*a->y;
		 b->y = c->z*a->x - c->x*a->z;
		 b->z = c->x*a->y - c->y*a->x;
		 if ( !dnormal_vector (b) )     return FALSE;
		 cs = cos(fi);
		 s = sin(fi);

/*  B=COS(FI)*A+SIN(FI)*B   */

		 b->x = cs*a->x + s*b->x;
		 b->y = cs*a->y + s*b->y;
		 b->z = cs*a->z + s*b->z;
		 return TRUE;
}


BOOL   o_minv(lpMATR a,lpMATR ai)
{

	sgFloat d, biga, hold;
	short nk, kk, ki, k, kj;
	short i, ij, ik, iz;
	short j, ji, jk, jp, jq, jr;
  short n = 4;  //  
  short l[4], m[4]; //    "n"

	memcpy(ai,a,sizeof(sgFloat)*16);

  //   
	d  = 1.0;
  nk = -n;
	for (k = 0; k < n; k++) { //      do 80 k=1,n
    nk=nk+n;
    l[k]=k;
		m[k]=k;
		kk=nk+k;
		biga=ai[kk];
		for (j = k; j < n; j++) { //      do 20 j=k,n
			iz=n*j;
			for (i = k; i < n; i++) { //      do 20 i=k,n
				ij=iz+i;
				if (fabs(biga) >= fabs(ai[ij])) continue;
				biga=ai[ij];
        l[k]=i;
        m[k]=j;
      }
		} // 20     continue

    //   
    j=l[k];
    if (j > k) {
      ki=k-n;
      for (i = 0; i < n; i++) { //    do 30 i=1,n
        ki=ki+n;
        hold=-ai[ki];
				ji=ki-k+j;
        ai[ki]=ai[ji];
        ai[ji]=hold;
			}
    }

    //  
    i=m[k];
    if (i > k) {
      jp=n*i;
      for (j = 0; j < n; j++) { //    do 40 j=1,n
				jk=nk+j;
        ji=jp+j;
				hold=-ai[jk];
        ai[jk]=ai[ji];
        ai[ji]=hold;
      }
    }

    //       (
    //     biga)
		if (fabs(biga) < DIVIDE_BY_ZERO) {
			 // stop ' '
			return FALSE;
		}
		for (i = 0; i < n; i++) { //      do 55 i=1,n
			if (i == k) continue;
			ik=nk+i;
			ai[ik]=ai[ik]/(-biga);
		} // 55     continue

		//  
		for (i = 0; i < n; i++) { //      do 65 i=1,n
			ik=nk+i;
      hold=ai[ik];
      ij=i-n;
      for (j = 0; j < n; j++) { //      do 65 j=1,n
        ij=ij+n;
        if (i == k) continue;
				if (j == k) continue;
        kj=ij-i+k;
        ai[ij]=hold*ai[kj]+ai[ij];
      }
    } // 65     continue

    //     
    kj=k-n;
    for (j = 0; j < n; j++) { //    do 75 j=1,n
      kj=kj+n;
      if (j == k) continue;
      ai[kj]=ai[kj]/biga;
		} // 75     continue

    //   
    d=d*biga;

    //      
    ai[kk]=1.0/biga;
  } //   80   continue

  //     
  k=n-1;
  while (1) {
    k=k-1;
    if (k < 0)
			break;
    i=l[k];
    if (i > k) {
      jq=n*k;
      jr=n*i;
      for (j = 0; j < n; j++) { // do 110 j=1,n
				jk=jq+j;
        hold=ai[jk];
        ji=jr+j;
        ai[jk]=-ai[ji];
        ai[ji]=hold;
      }
    }
		j=m[k];
    if (j <= k) continue;
    ki=k-n;
    for (i = 0; i < n; i++) { // do 130 i=1,n
      ki=ki+n;
      hold=ai[ki];
      ji=ki-k+j;
      ai[ki]=-ai[ji];
      ai[ji]=hold;
    }
  }
	return TRUE;
}

BOOL   o_hcrot1(lpMATR t,lpD_POINT p)
{
	sgFloat ax,az;
	if ( !o_hcrot_angle( p, &ax,&az) ) return FALSE;

	o_tdrot(t,-3,az);
	o_tdrot(t,-1,ax);
	return TRUE;
}

BOOL   o_hcrot_angle(lpD_POINT p,sgFloat * axp,sgFloat * azp)
{
D_POINT v = *p;
sgFloat  d;

	*azp = 0.; *axp = 0.;
	if(!dnormal_vector(&v)) return FALSE;
	if((d = hypot(v.x, v.y)) < eps_n) *azp =  M_PI;
	else                              *azp = -M_PI/2. - v_angle(v.x, v.y);
	*axp = M_PI/2. - v_angle(-d, v.z);
	return TRUE;
}
/*
BOOL   o_hcrot_angle(lpD_POINT p,sgFloat * axp,sgFloat * azp)
{
	sgFloat ax,az;

	*axp=0.;
	*azp=0.;

	ax=sqrt(p->x*p->x+p->y*p->y);
	if ( ax < eps_d )     ax=0.;
	else                  ax=atan2(fabs(p->z),ax);
	az=fabs(p->y);
	if ( az < eps_d )     az=0.;
	else                  az=atan2(fabs(p->x),az);
	if( fabs(p->y) < eps_d ){
		if( fabs(p->x) < eps_d ){
			if( fabs(p->z) < eps_d ) return FALSE;
			ax=M_PI/2;
			az=M_PI;	//    0.;
			goto met;
		}
		else {
			az=M_PI/2;
		}
	}
	if( p->x <= 0. ) {
		if( p->y <= 0. ) goto met;
		if( p->x != 0. ) {
			az=M_PI-az;
			goto met;
		}
		else {
			az=az+M_PI;
			goto met;
		}
	}
	if ( p->y < 0. ) az=-az;
	else          az=az+M_PI;
met:
	if ( p->z < 0. ) ax=-ax;
	*axp=ax  - M_PI/2;
	*azp=az;
	return TRUE;
}
*/

void   o_hcmult(lpMATR a, lpMATR b)
{
	sgFloat c[4];
	register short i,j,k;
	sgFloat aw,bw;

	for ( i=0 ; i < 4 ; i++ ) {
	for ( j=0 ; j < 4 ; j++ ) {
		c[j]=0;
		for ( k=0 ; k < 4 ; k++ ) {
		aw=a[i+k*4];
		bw=b[k+j*4];
		c[j]=c[j]+aw*bw;
		}
	}
	for ( j=0 ; j < 4 ; j++ )
		a[i+j*4]=c[j];
	}
}

void   o_tdrot(lpMATR t, int naxes, sgFloat alpha)
{

  int i[] = {1,2,0,1};
  int k, l, naxe;
  sgFloat GFa[16];

  naxe = abs(naxes);
  if (naxes > 0 ) alpha = M_PI/180*alpha;
	o_hcunit(GFa);
	k = i[naxe-1];
  l = i[naxe];
	GFa[k+k*4] = cos(alpha);
  GFa[l+l*4] = cos(alpha);
  GFa[k+l*4] = sin(alpha);
  GFa[l+k*4] = -sin(alpha);
  o_hcmult(t,GFa);
}

void   o_tdscal(lpMATR t, int naxis, sgFloat scale)
{
  sgFloat GFa[16];

	if (naxis == 4) scale = 1.0/scale;
  o_hcunit(GFa);
  GFa[5*naxis-5] = scale;
	o_hcmult(t,GFa);
}

void   o_scale( lpMATR t,sgFloat scale)
{
	o_tdscal(t,1, scale);
  o_tdscal(t,2, scale);
  o_tdscal(t,3, scale);
}

void   o_tdtran(lpMATR t,lpD_POINT v)
{
	sgFloat GFa[16];
	o_hcunit(GFa);
	GFa[3]  = v->x;
	GFa[7]  = v->y;
	GFa[11] = v->z;
	o_hcmult(t,GFa);
}

void   o_hcunit(lpMATR matr)
{
  memset(matr,0,sizeof(sgFloat)*16);
  matr[0] = matr[5] = matr[10] = matr[15] = 1;
}

BOOL   o_hcprsp(lpMATR matr,sgFloat h)
{
	if ( fabs(h) < eps_d ) return FALSE;
	matr[14] = -1/h;
	return TRUE;
}


void   f3d_gabar_project1(lpMATR matr, lpD_POINT min1, lpD_POINT max1,
						  lpD_POINT min2, lpD_POINT max2)
						  //    (min2, max2),
						  //    (min1, max1),   
						  //   matr (    )
{
	D_POINT p, pmin, pmax;
	short     i;

	o_hcncrd(matr, min1, &pmin);
	pmax = pmin;
	for(i = 1; i < 8; i++){
		get_box_vertex(min1, max1, i, &p);
		o_hcncrd(matr, &p, &p);
		if(p.x < pmin.x) pmin.x = p.x;
		if(p.y < pmin.y) pmin.y = p.y;
		if(p.z < pmin.z) pmin.z = p.z;
		if(p.x > pmax.x) pmax.x = p.x;
		if(p.y > pmax.y) pmax.y = p.y;
		if(p.z > pmax.z) pmax.z = p.z;
	}
	*min2 = pmin;
	*max2 = pmax;
}

void get_box_vertex(lpD_POINT p1, lpD_POINT p2, short num, lpD_POINT v)
{                                    //            1---------5 |
	v->x = (num & 4) ? p2->x : p1->x;  //         | |       | |
	v->y = (num & 2) ? p2->y : p1->y;  //         z |       | |
	v->z = (num & 1) ? p2->z : p1->z;  //         | 2-------|-6
	
}																		
/*BOOL   o_view_init(lpMATR t,sgFloat r)
{

	o_hcunit(t);
	if ( fabs(r) < eps_d ) return FALSE;
	t[14]=-1/r;
	return TRUE;
}*/

/*BOOL   o_view(lpMATR matr,lpD_POINT p)
{

  o_hcrot1(matr,p);
	if ( !o_hcprsp(matr,sqrt(p->x*p->x+p->y*p->y+p->z*p->z)) ) return FALSE;
	return TRUE;
}*/
