#include "../sg.h"
static BOOL check_line1(lpF_PLPOINT min,lpF_PLPOINT max,
								lpF_PLPOINT p1, lpF_PLPOINT p2);
static BOOL check_point1(lpF_PLPOINT min, lpF_PLPOINT max, lpF_PLPOINT p);

BOOL find_sply_knot(lpD_POINT vp, lpSPLY_DAT spl, short *numint, short *numknot){
	sgFloat    l1=0.0, l2, w;
	F_PLPOINT min, max, fp1, fp2;
	D_POINT   p1, p2;
	BOOL      knot = TRUE;
	short				i;

//  
//	l1 = f_tfm_x(&curr_w3->fwind, trap_size/2 + 1) - f_tfm_x(&curr_w3->fwind, 0);
	min.x = (float)(vp->x - l1);
	min.y = (float)(vp->y - l1);
	max.x = (float)(vp->x + l1);
	max.y = (float)(vp->y + l1);

//  
	*numint = *numknot = 0;
	if (spl->sdtype & SPL_INT) {
		get_first_sply_point(spl, 0., FALSE, &p1);
	} else {
    Create_3D_From_4D( &spl->P[0], (sgFloat *)&p1, &w);
		i = 1;
	}
	gcs_to_vcs(&p1, &p1); fp1.x = (float)p1.x; fp1.y = (float)p1.y;

	if(spl->numk == 1)	return check_point1(&min, &max, &fp1);

	l1 = 0.;
	while (TRUE) {
		if (spl->sdtype & SPL_INT) {
			if (!get_next_sply_point_and_knot(spl, &p2, &knot)) break;
		} else {
			if (i >= spl->nump) break;
	    Create_3D_From_4D( &spl->P[i++], (sgFloat *)&p2, &w);
			knot = TRUE;
		}
		gcs_to_vcs(&p2, &p2); fp2.x = (float)p2.x; fp2.y = (float)p2.y;
//    
		if(check_line1(&min, &max, &fp1, &fp2))goto met_find;
		l1 += dpoint_distance_2d(&p1, &p2);
		if (knot) {
			(*numint)++;
			l1 = 0.;
		}
		p1  = p2; fp1 = fp2;
	}
	return FALSE;

met_find:
	l1 += dpoint_distance_2d(vp, &p1);
	l2  = dpoint_distance_2d(vp, &p2);
	while (!knot) {
		p1  = p2;
		get_next_sply_point_and_knot(spl, &p2, &knot);
		gcs_to_vcs(&p2, &p2);
		l2 += dpoint_distance_2d(&p1, &p2);
	}
	*numknot = (l1 > l2) ? *numint + 1 : *numint;
	return TRUE;
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

static BOOL check_point1(lpF_PLPOINT min, lpF_PLPOINT max, lpF_PLPOINT p){
	if ( p->x >= min->x && p->y >= min->y  &&
			 p->x <= max->x && p->y <= max->y)  return  TRUE;
	return  FALSE;
}
