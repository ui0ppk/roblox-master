#include "../sg.h"


short get_primitive_param(hOBJ hobj, void **par, sgFloat *scale)
{
	lpOBJ 				obj;
	lpGEO_BREP 		brep;
	LNP						lnp;
	int						ret;
	lpCSG					csg;
	D_POINT				p0, p1;

	memset(&p0, 0, sizeof(D_POINT));
	memset(&p1, 0, sizeof(D_POINT));
	p1.x = 1.;
	obj = (lpOBJ)hobj;
	switch (obj->type) {
		case OBREP:
			brep = (lpGEO_BREP)obj->geo_data;
			
			o_hcncrd(brep->matr, &p0, &p0);
			o_hcncrd(brep->matr, &p1, &p1);
			*scale = dpoint_distance(&p0, &p1);
      //--
			if (!read_elem(&vd_brep, brep->num, &lnp)) ret = -1;
			else {
				csg = (lpCSG)lnp.hcsg;
				if ((*par = SGMalloc(csg->size)) != NULL) {
					ret = lnp.kind;
					memcpy(*par, csg->data, csg->size);
				} else ret = -1;
			}
			break;
		default:
			ret = -1;
			break;
	}
	return ret;
}

BOOL CalcPoxyPolygon(hOBJ hobj, sgFloat OutRadius,
										 lpD_POINT p0, lpD_POINT pX, lpD_POINT pY, UCHAR *color)
{
  D_POINT			v1, v2, v3, n, t1, t2, t3, t4;
  MATR				matr2, matr2i;	//, matr;
  lpGEO_PATH	path;
  lpGEO_LINE	lpLine;
  lpOBJ				obj;
  hOBJ				hobj1;
  sgFloat			scale;

	memset(&t1, 0, sizeof(D_POINT));
	memset(&t2, 0, sizeof(D_POINT));
	t2.x = 1.;
	obj = (lpOBJ)hobj;
	path = (lpGEO_PATH)obj->geo_data;
	o_hcncrd(path->matr, &t1, &t1);
	o_hcncrd(path->matr, &t2, &t2);
	scale = dpoint_distance(&t1, &t2);
  obj 		= (lpOBJ)hobj;
  path 		= (lpGEO_PATH)obj->geo_data;
  hobj1 	= path->listh.hhead;
  obj  		= (lpOBJ)hobj1;
  *color		= obj->color;	
  lpLine 	= (lpGEO_LINE)obj->geo_data;
  *pX 			= lpLine->v1;
  *pY 			= lpLine->v2;
  hobj1 	= path->listh.htail;
  obj  		= (lpOBJ)hobj1;
  lpLine 	= (lpGEO_LINE)obj->geo_data;
  v3 			= lpLine->v1;
  //
  
  //
 
  if (!dpoint_cplane(pX, pY, &v3, &n)) return FALSE;

  o_hcunit(matr2);
  o_tdtran(matr2, dpoint_inv(pX, &t1));
  o_hcrot1(matr2, &n);
  o_hcncrd(matr2, pX, &v1);
  o_hcncrd(matr2, pY, &v2);
  o_hcncrd(matr2, &v3, &v3);
  dpoint_add(&v2, dpoint_scal(dpoint_sub(&v3, &v2, &t2), 0.5, &t3), &t1);
  
  dnormal_vector1(dpoint_sub(&t1, &v1, &t4));
  dpoint_add(&v1, dpoint_scal(&t4, OutRadius * scale, &t3), p0);
  o_minv(matr2, matr2i);
  o_hcncrd(matr2i, p0, p0);
  
  dnormal_vector1(dpoint_sub(pX, p0, &t1));
  dnormal_vector1(dpoint_sub(pY, p0, &t2));
  dvector_product(&t1, &t2, &t3);
  dnormal_vector1(&t3);
  dvector_product(&t3, &t1, pY);
  dpoint_add(p0, dpoint_scal(pY, OutRadius * scale, &t4), pY);
  
  /*
  o_minv(path->matr, matr);
  o_hcncrd(matr, p0, p0);
  o_hcncrd(matr, pX, pX);
  o_hcncrd(matr, pY, pY);
  */
  return TRUE;
}
//--
