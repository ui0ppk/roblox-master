#include "../../sg.h"

BOOL np_fa(lpNPW np, short face)
{
	short        mf;
	lpNP_LOOPS l = NULL;
	short        loops = 0;   				   // -    l
	BOOL       rt = TRUE, msg = FALSE;

	if ((l = np_realloc_loops(l,&loops)) == NULL) return FALSE;
	mf = np->nof;        									   //  - 
	if ( !np_fa_one(np, face, &l, &loops, msg) ) { rt = FALSE; goto end; }
	if (np->nof > mf) np_fa_ef(np,mf);

end:
	SGFree(l);
	return rt;
}

BOOL np_fa_one(lpNPW np, short face, lpNP_LOOPS * ll, short * loops,
							 BOOL msg)
{
	lpNP_LOOPS 	l = *ll;
	short     	 	lc,vl,vl1,vl_max;
	short    			i,im,j,loop,f;
	short   		  vc_max,fpc;
	short    			km,kp; // -    
	short   		  mt,pt; //     
	sgFloat  		sq;

	km = 0;  kp = 0;
	lc = 0;                              // ,  loop
	loop = np->f[face].fc;
	do {                                        //   
		if ( !np_ort_loop(np,face,loop,&sq) ) {
			np_handler_err(NP_CNT_BREAK);           		 //   
			if ( msg ) return FALSE;		 							//  
		}
		if (sq  < 0) {
			l[km].mm = sq;              //   
			l[km].cm = loop;
			km++;
			if (km == *loops)
				if ((l=np_realloc_loops(l,loops)) == NULL)  return FALSE;
		} else {             					//   
			if (kp == 0) {    							 //   
				fpc = loop;
				l[km].mm = sq;
				l[km].cm = loop;
				km++;   kp++;
				if (km == *loops)
					if ((l=np_realloc_loops(l,loops)) == NULL)  return FALSE;
				} else {                  	//    
					if ( !(f=np_new_face(np,loop)) ) return FALSE;
					//      loop
					l[kp].cp = f; //    (  )
					memcpy(&np->p[f].v,&np->p[face].v,sizeof(np->p[face].v));
  				np->f[f].fl = np->f[face].fl;
					l[kp].mp = sq;
					kp++;
					if (kp == *loops)
						if ((l=np_realloc_loops(l,loops)) == NULL)  return FALSE;
					np->c[lc].nc = np->c[loop].nc; //    
					np->c[loop].nc = 0;  loop = lc;
				}
		}
		lc = loop;
		loop = np->c[loop].nc;
	} while (loop != 0);
	if (kp == 1) 
	{
		*ll = l;
		return TRUE;      //     
	}

//       

	kp--;
	i = 0;
	mt = np->f[face].fc;         	 // mt -   
	lc = 0;                       	       // ,  mt
	do {
		if (l[i].mm > 0) {
			lc = mt;
			mt = np->c[mt].nc;
			i++;
			continue;
		}
		vl = 0;
		for (loop=1 ; loop <= kp ; loop++) {
			pt = np->f[l[loop].cp].fc; // pt -   
			if (-(l[i].mm) > l[loop].mp) continue;
			if ( np_include_ll(np, face, mt, pt) ) {
				l[vl].mem = pt;
				vl++;
				if (vl == *loops)
					if ((l=np_realloc_loops(l,loops)) == NULL)  return FALSE;
			}
		}
		if (vl == 0) {    						//     
			lc = mt;
			mt = np->c[mt].nc;
			i++;
			continue;
		}
		if (np_include_ll(np, face, mt, fpc) == TRUE) {
			l[vl].mem = fpc;
			vl++;
		}
		if (vl == 1) {          	  //    
			pt = l[0].mem;         		  //    pt ( fpc)
			if (lc == 0) np->f[face].fc = np->c[mt].nc;
			else   np->c[lc].nc = np->c[mt].nc;
			np->c[mt].nc = np->c[pt].nc;
			np->c[pt].nc = mt;
			if (lc == 0) mt = np->f[face].fc;
			else         mt = np->c[lc].nc;
			i++;
			continue;
		} else {

/*        */

				vl--;
				vl_max = 0;
				vc_max = l[0].mem;  	 //    
				for (im=0 ; im <= vl ; im++) {
					vl1 = 0;
					for (j = 0 ; j <= vl ; j++) {
						//   i     . 
						if (l[im].mem == l[j].mem) continue;
						if ( np_include_ll(np,face,l[im].mem,l[j].mem) ) vl1++;
					}
					if (vl1 > vl_max) { vl_max = vl1;  vc_max = l[im].mem; }
				}
				if (vc_max != fpc) {
					if (lc == 0) np->f[face].fc = np->c[mt].nc;  //  mt  face
					else         np->c[lc].nc   = np->c[mt].nc;
					np->c[mt].nc = np->c[vc_max].nc;     	//   vc_max
					np->c[vc_max].nc = mt;
					mt = np->c[lc].nc;
					i++;  continue;
				}
		}
		lc = mt;
		mt = np->c[mt].nc;
		i++;
	} while (mt > 0);

	*ll = l;
	return TRUE;
}

void np_fa_ef(lpNPW np, short face_old)
{
  short i,loop;
  short r,rf;

  for (i = face_old+1 ; i <= np->nof ; i++) {
		loop = np->f[i].fc;
    do {
      r = rf = np->c[loop].fe;
			do {
        if (r > 0)  np->efc[ r].fp = i;
        else        np->efc[-r].fm = i;
        r = SL(r,np);
      } while (r != rf && r!= 0);
      loop = np->c[loop].nc;
    } while (loop > 0);
  }
}
lpNP_LOOPS np_realloc_loops(lpNP_LOOPS lold, short * loops)
{
	lpNP_LOOPS lnew;//nb  = NULL;

	*loops += 20;
	if ((lnew = (lpNP_LOOPS)SGMalloc(sizeof(NP_LOOPS)*(*loops))) == NULL) return FALSE;
	if (lold == NULL) return lnew;                 //  
	memcpy(lnew,lold,sizeof(NP_LOOPS)*(*loops-20));
	SGFree(lold);
	return lnew;
}

BOOL np_point_belog_to_face(lpNPW np, lpD_POINT v, short face)
{
	short 				r,rf;
	short 				v1,v2;
	int				 	i, k1, k2;
	short 				loop;
	sgFloat 			xx,yy,xp;
	lpDA_POINT 	w, wv;

	w = (lpDA_POINT)np->v;
	wv = (lpDA_POINT)v;

	if (fabs(np->p[face].v.x) > fabs(np->p[face].v.y)) {
		if (fabs(np->p[face].v.x) > fabs(np->p[face].v.z)) {
							k1 = 1; 	k2 = 2;
		} else {  k1 = 0; 	k2 = 1; }
	} else {
		if (fabs(np->p[face].v.y) > fabs(np->p[face].v.z)) {
							k1 = 0; 	k2 = 2;
		} else {  k1 = 0; 	k2 = 1; }
	}

	loop = np->f[face].fc;
	i = 0;
	yy = wv[0][k2];     xx = wv[0][k1];
	do {
		r = np->c[loop].fe;  rf = r;
		v1 = BE(r,np);    v2 = EE(r,np);
		do {
			if ( dpoint_eq(&np->v[v1], v, eps_d) ) return TRUE;
			if (!(w[v1][k2] <  yy && w[v2][k2] <  yy ||
						w[v1][k2] >= yy && w[v2][k2] >= yy))  {
				xp = w[v1][k1] + ((w[v2][k1] - w[v1][k1])*(w[v1][k2] - yy))/
						(w[v1][k2] - w[v2][k2]);
				if (xp > xx) i++;
			}
			r = SL(r,np); v1 = v2;  v2 = EE(r,np);
		} while(r != rf && r != 0);
		loop = np->c[loop].nc;
	} while (loop);
//  if (i == 0) return FALSE;
	if ((i/2)*2 == i) return FALSE;
  return TRUE;
}

NP_TYPE_POINT np_point_belog_to_face1(lpNPW np, lpD_POINT v, short face,
																			short* edge, short* vv)
{
	short 				r,rf;
	short 				v1,v2;
	int				 	i, k1, k2;
	short 				loop;
	sgFloat 			xx,yy,xp;
	lpDA_POINT 	w, wv;

	w = (lpDA_POINT)np->v;
	wv = (lpDA_POINT)v;
	*vv = 0; 				*edge = 0;

	if (fabs(np->p[face].v.x) > fabs(np->p[face].v.y)) {
		if (fabs(np->p[face].v.x) > fabs(np->p[face].v.z)) {
							k1 = 1; 	k2 = 2;
		} else {  k1 = 0; 	k2 = 1; }
	} else {
		if (fabs(np->p[face].v.y) > fabs(np->p[face].v.z)) {
							k1 = 0; 	k2 = 2;
		} else {  k1 = 0; 	k2 = 1; }
	}

	loop = np->f[face].fc;
	i = 0;
	yy = wv[0][k2];     xx = wv[0][k1];
	do {
		r = np->c[loop].fe;  rf = r;
		v1 = BE(r,np);    v2 = EE(r,np);
		do {
			if ( dpoint_eq(&np->v[v1], v, eps_d) ) { *vv = v1; return NP_ON; }
			if ( fabs(w[v1][k2] - yy) < eps_d && fabs(w[v2][k2] - yy) < eps_d) {
				if ( w[v1][k1] < xx && w[v2][k1] > xx ||
						 w[v1][k1] > xx && w[v2][k1] < xx ) {
					 *edge = r;
					 return NP_ON; //   
				}
			}
			if (!(w[v1][k2] <  yy && w[v2][k2] <  yy ||
						w[v1][k2] >= yy && w[v2][k2] >= yy))  {
				xp = w[v1][k1] + ((w[v2][k1] - w[v1][k1])*(w[v1][k2] - yy))/
						(w[v1][k2] - w[v2][k2]);
				if (xp > xx-eps_d && xp < xx+eps_d) { *edge = abs(r); return NP_ON; }
				if (xp > xx) i++;
			}
			r = SL(r,np); v1 = v2;  v2 = EE(r,np);
		} while(r != rf && r != 0);
		loop = np->c[loop].nc;
	} while (loop);
	if ((i/2)*2 == i) return NP_OUT;
	return NP_IN;
}
