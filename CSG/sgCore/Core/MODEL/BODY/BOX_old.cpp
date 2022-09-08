#include "../../sg.h"
//#include <o_model.h>

#pragma argsused
short create_box_np(lpLISTH listh, sgFloat  *par)
{
/*
	VLD       vld;

	if ( !o_create_box(par) ) return 0;
	init_vld(&vld);
	if ( !add_np_mem(&vld,npwg,TNPB)) return 0;
	*listh = vld.listh;
	return 1;
*/
	init_listh(listh);
	return 1;
}
BOOL o_create_box(sgFloat  *par)
{
	sgFloat    rx = par[0], ry = par[1], rz = par[2];
   
	lpNPW 		np;

	short       i,j;
	int				nov=8,noe=12,nof=6,noc=6;
	short       bv[]={0,  1,  1,  1,  2,  2,  3,  3,  4,  5,  5,  6,  7};
	short       ev[]={0,  2,  4,  5,  3,  6,  4,  7,  8,  6,  8,  7,  8};
	short       fp[]={0,  1,  3,  2,  1,  4,  1,  5,  3,  2,  6,  4,  5};
	short       ep[]={0,  4,  8,  9,  6, 11, -2, 12,-10, -5,-12, -7, -8};
	short       fm[]={0,  2,  1,  3,  4,  2,  5,  4,  5,  6,  3,  6,  6};
	short       em[]={0,  3,  1,  2,  5, -1,  7, -4, -6, 10, -3, -9,-11};
	short       fe[]={0,  1, -1,  2,  5,  7, 10};

//  
  if ( fabs(rx) <= eps_n ) rx += 10*eps_d;
  if ( fabs(ry) <= eps_n ) ry += 10*eps_d;
  if ( fabs(rz) <= eps_n ) rz += 10*eps_d;
/*
	//    
  if (fabs(rx) <= eps_n && ry >eps_n  && rz > eps_n ||
      fabs(ry) <= eps_n && rx >eps_n  && rz > eps_n ||
      fabs(rz) <= eps_n && rx >eps_n  && ry > eps_n) {
		box2(rx, ry, rz);
		return TRUE;
	} else 	if (rx <= eps_n || ry <= eps_n  || rz <= eps_n) {
    handler_err(ERR_BAD_PARAMETER);
		return FALSE;
	}
*/
  np = npwg;

	//   
	np->v[1].y = 0.0;
	np->v[1].z = 0.0;
	np->v[2].y = 0.0;
	np->v[2].z =  rz;
	np->v[3].y =  ry;
	np->v[3].z =  rz;
	np->v[4].y =  ry;
	np->v[4].z = 0.0;
	j = nov/2+1;
	for (i = 1; i <= nov/2; i++) {
		np->v[i  ].x = 0.0;
		np->v[j  ].y = np->v[i].y;
		np->v[j  ].z = np->v[i].z;
		np->v[j++].x = rx;
	}
	np->nov = nov;

	//  
	for (i = 1; i <= noe; i++) {
		np->efr[i].bv = bv[i];
		np->efr[i].ev = ev[i];
		np->efc[i].fp = fp[i];
		np->efc[i].ep = ep[i];
		np->efc[i].fm = fm[i];
		np->efc[i].em = em[i];
		np->efr[i].el = ST_TD_CONSTR;
	}
	np->noe = noe;

	//    
	for (i = 1; i <= nof; i++) {
		np->f[i].fc = i;
		np->f[i].fl = ST_FACE_PLANE;
		np->c[i].fe = fe[i];
		np->c[i].nc = 0;
	}
	np->nof 	= nof;
	np->noc	 	= noc;
	np->ident = -32767;
  np->type  = TNPB;

  return TRUE;
}
/*
void box2(sgFloat dx, sgFloat dy, sgFloat dz)
{
	lpNPW  np = npwg;

	np->nov = np->noe = 4;
	np->nof = np->noc = 2;
	np->ident = -32767;
  np->type  = TNPB;

	//   
	np->v[1].x = np->v[1].y = np->v[1].z = 0.0;

	if (dx < eps_n) {	np->v[2].x = 0.0;		np->v[2].y = dy;	}
	else {					  np->v[2].x = dx;		np->v[2].y = 0.0;	}
	np->v[2].z = 0.0;

	np->v[3].x = dx;
	np->v[3].y = dy;
	np->v[3].z = dz;

	np->v[4].x = 0.0;
	if (dz < eps_n) { np->v[4].y = dy;	np->v[4].z = 0.0; }
	else            { np->v[4].y = 0.0;	np->v[4].z = dz;  }

	//  
	np->efr[1].bv = np->efr[4].ev = 1;
	np->efr[1].ev = np->efr[2].bv = 2;
	np->efr[2].ev =	np->efr[3].bv = 3;
	np->efr[3].ev = np->efr[4].bv = 4;
	np->efc[1].ep =  2;
	np->efc[2].ep =  3;
	np->efc[3].ep =  4;
	np->efc[4].ep =  1;
	np->efc[1].em = -4;
	np->efc[2].em = -1;
	np->efc[3].em = -2;
	np->efc[4].em = -3;
	np->efc[1].fp = np->efc[2].fp = np->efc[3].fp = np->efc[4].fp = 1;
	np->efc[1].fm = np->efc[2].fm = np->efc[3].fm = np->efc[4].fm = 2;

	//    
	np->f[1].fc = 1;
	np->f[2].fc = 2;
	np->c[1].fe = 1;
	np->c[2].fe =-1;
	np->f[1].fl = np->f[2].fl = 0;
	np->c[1].nc = np->c[2].nc = 0;

}
*/
