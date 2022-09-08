#include "../../sg.h"


static BOOL put_tri2(lpTRI_BIAND trb,
										 lpD_POINT p1, lpD_POINT p2, lpD_POINT p3,
										 int f1, int f2, int f3);

BOOL put_face_4(lpD_POINT p1, lpD_POINT p2, lpD_POINT p3, lpD_POINT p4,
								int flag_v, lpTRI_BIAND trb)
{
	D_POINT s1, s2, s3, s4, e12, e13, e14, e23, e24;
	sgFloat 	r1, r2, r3, r4, c1, c2, eps_d2 = eps_d * eps_d;
	BOOL		ret = TRUE;
	int			f1, f2, f3, f4;
	BOOL 		l1, l2, l3, l4;

	f1 = flag_v        & 3;
	f2 = (flag_v >> 2) & 3;
	f3 = (flag_v >> 4) & 3;
	f4 = (flag_v >> 6) & 3;
	dpoint_sub(p2, p1, &e12);
	dpoint_sub(p3, p1, &e13);
	dpoint_sub(p4, p1, &e14);
	dvector_product(&e12, &e13, &s1);
	r1 = dskalar_product(&s1, &s1);
	dvector_product(&e13, &e14, &s2);
	r2 = dskalar_product(&s2, &s2);
	dpoint_sub(p3, p2, &e23);
	dpoint_sub(p4, p2, &e24);
	dvector_product(&e12, &e14, &s3);
	r3 = dskalar_product(&s3, &s3);
	dvector_product(&e23, &e24, &s4);
	r4 = dskalar_product(&s4, &s4);
	dnormal_vector(&s1);
	dnormal_vector(&s2);
	dnormal_vector(&s3);
	dnormal_vector(&s4);
	c1 = dskalar_product(&s1, &s2);
	c2 = dskalar_product(&s3, &s4);
	l1 = (fabs(r1) < eps_d2) ? FALSE : TRUE;
	l2 = (fabs(r2) < eps_d2) ? FALSE : TRUE;
	l3 = (fabs(r3) < eps_d2) ? FALSE : TRUE;
	l4 = (fabs(r4) < eps_d2) ? FALSE : TRUE;
	if (l1 && l2 && c1 <= 0) l1 = l2 = FALSE;
	if (l3 && l4 && c2 <= 0) l3 = l4 = FALSE;
	if ((!l1 && !l2) && (!l3 && !l4))
		return TRUE;	
	if ((!l1 || !l2) && (l3 && l4)) {
		ret = put_tri2(trb, p1, p2, p4, f1, TRP_DUMMY, f4);
		if (ret) ret = put_tri2(trb, p2, p3, p4, f2, f3, TRP_DUMMY);
	} else if ((!l3 || !l4) && (l1 && l2)) {
		ret = put_tri2(trb, p1, p2, p3, f1, f2, TRP_DUMMY);
		if (ret) ret = put_tri2(trb, p1, p3, p4, TRP_DUMMY, f3, f4);
	} else if (l1 && l2 & l3 & l4) { 
		r1 = min(r1, r2);
		r3 = min(r3, r4);
		if (r1 < r3) {
			ret = put_tri2(trb, p1, p2, p4, f1, TRP_DUMMY, f4);
			if (ret) ret = put_tri2(trb, p2, p3, p4, f2, f3, TRP_DUMMY);
		} else {
			ret = put_tri2(trb, p1, p2, p3, f1, f2, TRP_DUMMY);
			if (ret) ret = put_tri2(trb, p1, p3, p4, TRP_DUMMY, f3, f4);
		}
	} else if(l2 && l4) {		// p1 == p2
		ret = put_tri2(trb, p2, p3, p4, f2, f3, f4);
	} else if (l2 && l3) {	// p2 == p3
		ret = put_tri2(trb, p1, p2, p4, f1, f3, f4);
	} else if (l1 && l3) { 	// p3 == p4
		ret = put_tri2(trb, p1, p2, p3, f1, f2, f4);
	} else if (l1 && l4) { 	// p1 == p4
		ret = put_tri2(trb, p1, p2, p3, f1, f2, f3);
	}
	return ret;
}

static BOOL put_tri2(lpTRI_BIAND trb,
										 lpD_POINT p1, lpD_POINT p2, lpD_POINT p3,
										 int f1, int f2, int f3)
{
	NPTRP		trp;

	trp.constr = (UCHAR)(f1 | (f2 << 2) | (f3 << 4));
	trp.v[0] = *p1;
	trp.v[1] = *p2;
	trp.v[2] = *p3;
	return put_tri(trb, &trp);
}
