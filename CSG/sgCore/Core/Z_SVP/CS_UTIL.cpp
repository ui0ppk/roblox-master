#include "../sg.h"
/*
	        OX  OY 
	      
*/
void lcs_oxy(lpD_POINT p0, lpD_POINT px, lpD_POINT py, MATR m)
{
	D_POINT x, y, z;

	m[3] = p0->x; m[7] = p0->y; m[11] = p0->z; m[15] = 1;
	dnormal_vector(dpoint_sub(px, p0, &x));
	dnormal_vector(dpoint_sub(py, p0, &y));
	dnormal_vector(dvector_product(&x, &y, &z));
	m[0] = x.x; m[1] = y.x; m[ 2] = z.x;
	m[4] = x.y; m[5] = y.y; m[ 6] = z.y;
	m[8] = x.z; m[9] = y.z; m[10] = z.z;
	m[12] = m[13] = m[14] = 0;
}

/*
	    ->   ,
	    ,  - XOY ||  . 
*/
void lcs_cpl_bp(MATR m)
{
	D_POINT n, p;

	normal_cpl(&n);
	o_hcunit(m);
	o_tdtran(m, dpoint_inv(&curr_point, &p));
	o_hcrot1(m, &n);
	o_minv(m, m);
}

/*
	    ->   ,
	    ,   OZ ||  n
*/
void lcs_normal_bp(MATR m, lpD_POINT n)
{
	D_POINT p;

	o_hcunit(m);
	o_tdtran(m, dpoint_inv(&curr_point, &p));
	o_hcrot1(m, n);
	o_minv(m, m);
}

/*
		   :  rez_geo.m[0]:  -> 
		p1 -     
*/
void box_fixed_lsk(lpD_POINT p1)
{
	MATR    mi;
	D_POINT pp1;

	matr_gcs_to_default_cs(rez_geo.m[0]);
	o_hcncrd(rez_geo.m[0], p1, &pp1);
	matr_default_cs_to_gcs(mi);
	o_hcunit(rez_geo.m[0]);
	o_tdtran(rez_geo.m[0], &pp1);
	o_hcmult(rez_geo.m[0], mi);		// rez_geo.m[0]:  -> 
}

/*---       , 
			 ,       p0,
			  pz    OZ. ( 
			OX  OY " ")
*/
void lcs_axis_z(lpD_POINT p0, lpD_POINT pz, MATR m)
{
	D_POINT p;

	o_hcunit(m);
	o_hcrot1(m, dpoint_sub(pz, p0, &p));
	o_minv(m, m);
	o_tdtran(m, p0);
}

/*---       , 
			 ,       p0,
			  OX      p0  px
			  -   
			 p0  px   .
*/
void lcs_o_px(lpD_POINT p0, lpD_POINT px, MATR m)
{
	D_POINT pp0, ppx, ppy;
	sgFloat	a, b, c;

	gcs_to_vcs(p0, &pp0);
	gcs_to_vcs(px, &ppx);
	ppx.z = pp0.z;
	if (!eq_line(&pp0, &ppx, &a, &b, &c)) {
		o_hcunit(m);
		return;
	}
	ppy.x = pp0.x + a;
	ppy.y = pp0.y + b;
	ppy.z = pp0.z;
	vcs_to_gcs(&ppx, &ppx);
	vcs_to_gcs(&ppy, &ppy);
	lcs_oxy(p0, &ppx, &ppy, m);
}

//         
void matr_default_cs_to_gcs(MATR matr)
{
//	switch (CURR_TYPE_CS) {
	//	case CS_VIEW:
			//memcpy(matr, curr_w3->matri, sizeof(MATR));
	//		break;
	//	case CS_USER:
			memcpy(matr, m_ucs_gcs, sizeof(MATR));
	//		break;
	//	default:
	//		o_hcunit(matr);
	//		break;
	//}
}

//         
void matr_gcs_to_default_cs(MATR matr)
{
//	switch (CURR_TYPE_CS) {
//		case CS_VIEW:
			//memcpy(matr, curr_w3->matrp, sizeof(MATR));
	//		break;
	//	case CS_USER:
			memcpy(matr, m_gcs_ucs, sizeof(MATR));
	//		break;
	//	default:
	//		o_hcunit(matr);
	//		break;
	//}
}

//   (0-x, 1-y, 2-z)    
short get_index_invalid_axis(void)
{
	D_POINT p0 = {0,0,0}, p1 = {0,0,1}, p2, p3;
	sgFloat  d, d1, angle = 10;

	gcs_to_vcs(&p0, &p0);
	gcs_to_vcs(&p2, &p2);
	dpoint_sub(&p2, &p0, &p3);
	dnormal_vector(&p3);
	d = dskalar_product(&p3, &p1);
	d = fabs(90 - d_degree(acos(d)));
	if (d > angle) return 2;	//  XOY 
	p2.x = 1; p2.y = 0; p2.z = 0;
	gcs_to_vcs(&p2, &p2);
	dpoint_sub(&p2, &p0, &p3);
	dnormal_vector(&p3);
	d = dskalar_product(&p3, &p1);
	d1 = fabs(90 - d_degree(acos(d)));
	p2.x = 0; p2.y = 1; p2.z = 0;
	gcs_to_vcs(&p2, &p2);
	dpoint_sub(&p2, &p0, &p3);
	dnormal_vector(&p3);
	d = dskalar_product(&p3, &p1);
	d = fabs(90 - d_degree(acos(d)));
	if (d1 > d) return 0;	//  YOZ 
	return 1;	//  ZOX
}
