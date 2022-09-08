#include "../../sg.h"

//       
BOOL o_lcs(hOBJ hobj, MATR matr_lcs_gcs, BOOL *left)
{
lpOBJ			obj;
D_POINT 	p0,	px, py, pz;
D_POINT 	ex, ey, ez, exy;
sgFloat		alpha;
BOOL      (*get_lcs)(hOBJ hobj, MATR matr_lcs_gcs);

	obj = (lpOBJ)hobj;
  get_lcs = (BOOL (*)(hOBJ hobj, MATR matr_lcs_gcs))GetObjMethod(obj->type, OMT_GETLCS);
  if(!get_lcs)
    return FALSE;
  if(!get_lcs(hobj, matr_lcs_gcs))
    return FALSE;

	memset(&p0, 0, sizeof(p0));
	memset(&px, 0, sizeof(px));
	px.x = 1;
	memset(&py, 0, sizeof(py));
	py.y = 1;
	memset(&pz, 0, sizeof(pz));
	pz.z = 1;
	o_hcncrd(matr_lcs_gcs, &p0, &p0);
	o_hcncrd(matr_lcs_gcs, &px, &px);
	o_hcncrd(matr_lcs_gcs, &py, &py);
	o_hcncrd(matr_lcs_gcs, &pz, &pz);
	dpoint_sub(&px, &p0, &ex);
	dnormal_vector(&ex);
	dpoint_sub(&py, &p0, &ey);
	dnormal_vector(&ey);
	dpoint_sub(&pz, &p0, &ez);
	dnormal_vector(&ez);
	alpha = dskalar_product(dvector_product(&ex, &ey, &exy), &ez);
	*left = (alpha < 0);
  return TRUE;
}

BOOL lcs_complex(hOBJ hobj, MATR matr_lcs_gcs)
{
lpOBJ	obj;

	obj = (lpOBJ)hobj;
	memcpy(matr_lcs_gcs, obj->geo_data, sizeof(MATR));
  return TRUE;
}

BOOL lcs_dim(hOBJ hobj, MATR matr_lcs_gcs)
{
lpOBJ	obj;

	obj = (lpOBJ)hobj;
	memcpy(matr_lcs_gcs, ((lpGEO_DIM)obj->geo_data)->matr, sizeof(MATR));
  return TRUE;
}

BOOL lcs_circle_and_arc(hOBJ hobj, MATR matr_lcs_gcs)
{
lpOBJ	        obj;
lpGEO_CIRCLE	circle;
D_POINT 	    p;

	obj = (lpOBJ)hobj;
	circle = (lpGEO_CIRCLE)obj->geo_data;
	o_hcunit(matr_lcs_gcs);
	o_tdtran(matr_lcs_gcs, dpoint_inv(&circle->vc, &p));
	o_hcrot1(matr_lcs_gcs, &circle->n);
	o_minv(matr_lcs_gcs, matr_lcs_gcs);
  return TRUE;
}

BOOL lcs_point(hOBJ hobj, MATR matr_lcs_gcs)
{
lpOBJ	        obj;
lpGEO_POINT   point;

	obj = (lpOBJ)hobj;
	point = (lpGEO_POINT)obj->geo_data;
	create_ucs_matr1(1, point, NULL, NULL, matr_lcs_gcs, 0, 1);
  return TRUE;
}

BOOL lcs_line(hOBJ hobj, MATR matr_lcs_gcs)
{
lpOBJ	        obj;
lpGEO_LINE    line;

	obj = (lpOBJ)hobj;
	line = (lpGEO_LINE)obj->geo_data;
	create_ucs_matr1(2, &line->v1, &line->v2, NULL, matr_lcs_gcs, 0, 1);
  return TRUE;
}

BOOL lcs_spline_and_path(hOBJ hobj, MATR matr_lcs_gcs)
{
lpOBJ	        obj;
SCAN_CONTROL 	sc;
D_POINT				p[3];
D_PLANE				plane;
OSTATUS       status;

  init_scan(&sc);
	sc.user_geo_scan  = ucs_geo_scan;
	sc.data 	        = p;
	if (o_scan(hobj, &sc) != OSBREAK) return FALSE;
	obj = (lpOBJ)hobj;
	status = (OSTATUS)(obj->status & ST_FLAT);
	if (!status) { //   
		create_ucs_matr1(2, &p[0], &p[1], NULL, matr_lcs_gcs, 0, 1);
	} else { //  
	  if (!set_flat_on_path(hobj, &plane)) return FALSE;
	  dvector_product(&plane.v, dpoint_sub(&p[1],&p[0],&p[2]), &p[2]);
	  dpoint_add(&p[2], &p[0], &p[2]);
	  create_ucs_matr1(3, &p[0], &p[1], &p[2], matr_lcs_gcs, 0, 1);
	}
  return TRUE;
}

#pragma argsused
BOOL lcs_idle(hOBJ hobj, MATR matr_lcs_gcs)
{
  o_hcunit(matr_lcs_gcs);
  return TRUE;
}


