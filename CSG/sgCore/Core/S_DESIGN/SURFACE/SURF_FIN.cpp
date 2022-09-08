#include "../../sg.h"
static BOOL check_line1(lpF_PLPOINT min,lpF_PLPOINT max,
								lpF_PLPOINT p1, lpF_PLPOINT p2);

BOOL find_surf_knot(lpD_POINT vp, lpSURF_DAT surf, short *numk_u, short *numk_v){
	short 	 	i, j, number;
	sgFloat    l1, l2, w;
	F_PLPOINT min, max, fp1, fp2;
	D_POINT   p1, p2, p3, p4;
  W_NODE		node;

//  
//	l1 = f_tfm_x(&curr_w3->fwind, trap_size/2 + 1) - f_tfm_x(&curr_w3->fwind, 0);
  l1 = 2.0;
	min.x = (float)(vp->x - l1);
	min.y = (float)(vp->y - l1);
	max.x = (float)(vp->x + l1);
	max.y = (float)(vp->y + l1);

	for( number=0, j=0; j<surf->P.m; j++, number += surf->P.n ){
		read_elem( &surf->P.vdim, number, &node);
    Create_3D_From_4D( &node, (sgFloat *)&p1, &w);
		gcs_to_vcs(&p1, &p1); fp1.x = (float)p1.x; fp1.y = (float)p1.y;
		if( j==surf->P.m-1 ){
			for(i=1; i<surf->P.n; i++ ){
//				gcs_to_vcs(&p1, &p1); fp1.x = (float)p1.x; fp1.y = (float)p1.y;
				read_elem( &surf->P.vdim, number+i, &node);
        Create_3D_From_4D( &node, (sgFloat *)&p2, &w);
				gcs_to_vcs(&p2, &p2); fp2.x = (float)p2.x; fp2.y = (float)p2.y;
//    
				if(check_line1(&min, &max, &fp1, &fp2)){
					l1 = dpoint_distance_2d(vp, &p1);
					l2 = dpoint_distance_2d(vp, &p2);
					*numk_u = (l1 > l2) ? i : i-1;
					*numk_v = surf->P.m-1;
					return TRUE;
				}
				p1=p2;
			}
		}else{
			for( i=0; i<surf->P.n; i++ ){
//				gcs_to_vcs(&p1, &p1); fp1.x = (float)p1.x; fp1.y = (float)p1.y;
				if( i==0 ){
					read_elem( &surf->P.vdim, number+surf->P.n, &node);
          Create_3D_From_4D( &node, (sgFloat *)&p4, &w);
					gcs_to_vcs(&p4, &p4); fp2.x = (float)p4.x; fp2.y = (float)p4.y;
//    
					if(check_line1(&min, &max, &fp1, &fp2)){
						l1 = dpoint_distance_2d(vp, &p1);
						l2 = dpoint_distance_2d(vp, &p4);
						*numk_u = 0;
						*numk_v = (l1 > l2) ? j+1 : j;
						return TRUE;
					}
				}else{
					read_elem( &surf->P.vdim, number+i, &node);
	        Create_3D_From_4D( &node, (sgFloat *)&p2, &w);
  				gcs_to_vcs(&p2, &p2); fp2.x = (float)p2.x; fp2.y = (float)p2.y;
//    
					if(check_line1(&min, &max, &fp1, &fp2)){
						l1 = dpoint_distance_2d(vp, &p1);
						l2 = dpoint_distance_2d(vp, &p2);
						*numk_u = (l1 > l2) ? i : i-1;
						*numk_v = j;
						return TRUE;
					}

					read_elem( &surf->P.vdim, number+surf->P.n+i, &node);
          Create_3D_From_4D( &node, (sgFloat *)&p3, &w);
					gcs_to_vcs(&p3, &p3); fp1.x = (float)p3.x; fp1.y = (float)p3.y;
//    
					if(check_line1(&min, &max, &fp1, &fp2)){
						l1 = dpoint_distance_2d(vp, &p2);
						l2 = dpoint_distance_2d(vp, &p3);
						*numk_u = i;
						*numk_v = (l1 > l2) ? j+1 : j;
						return TRUE;
					}
				}
				p1=p2;
				fp1=fp2;
			}
		}
	}

//	if(spl->numk == 1)	return check_point1(&min, &max, &fp1);
	return FALSE;
}

static BOOL check_line1(lpF_PLPOINT min,lpF_PLPOINT max,
								lpF_PLPOINT p1, lpF_PLPOINT p2){
	float x1,y1,x2,y2,rc;
	register short i,j,k;

	x1=p1->x;
	y1=p1->y;
	x2=p2->x;
	y2=p2->y;
	if (x1 < min->x )  i=1;
	else {
		if (x1 > max->x) i=2;
		else             i=0;
	}
	if (y1 < min->y)   i |=4;
	else {
		if(y1 > max->y)  i |=8;
	}

	if (x2 < min->x )  j=1;
	else {
		if (x2 > max->x) j=2;
		else             j=0;
	}
	if (y2 < min->y)   j |=4;
	else {
		if(y2 > max->y)  j |=8;
	}
	if ( !i || !j )    return TRUE;
	if ( (i & j) != 0) return FALSE;
	for (;;) {
		if ( !i || !j ) return TRUE;
		if ( (i & j))   return FALSE;
		if ( !i ) {
			k=i;  i=j; j=k;
			rc=x1; x1=x2; x2=rc;
			rc=y1; y1=y2; y2=rc;
		}
		if ( (i & 1) ) {
			y1=y1+(y2-y1)*(min->x-x1)/(x2-x1);
			x1=min->x;
		} else	if ( i & 2 ) {
			y1=y1+(y2-y1)*(max->x-x1)/(x2-x1);
			x1=max->x;
		}
		else	if ( i & 4 ) {
			x1=x1+(x2-x1)*(min->y-y1)/(y2-y1);
			y1=min->y;
		}
		else	if ( i & 8 ) {
			x1=x1+(x2-x1)*(max->y-y1)/(y2-y1);
			y1=max->y;
		}
		if (x1 < min->x )  i=1;
		else {
			if (x1 > max->x) i=2;
			else             i=0;
		}
		if (y1 < min->y)   i |=4;
		else {
			if(y1 > max->y)  i |=8;
		}
	}
}

