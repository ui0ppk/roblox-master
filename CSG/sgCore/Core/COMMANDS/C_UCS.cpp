#include "../sg.h"


void create_ucs_matr1(short flag, lpD_POINT p0, lpD_POINT p1, lpD_POINT p2,
											MATR matr, short First, short Second)
{
	D_POINT 	p;
	DA_POINT	pa;
	int				Three = 3 - (First + Second);
	sgFloat		angle;

	if (flag == 1) {
		p.x = p.y = p.z = 0;
		ucs_to_gcs(&p, &p);
		memcpy(matr, m_ucs_gcs, sizeof(MATR));
		o_tdtran(matr, dpoint_sub(p0, &p, &p));
	} else {
		o_hcunit(matr);
		o_tdtran(matr, dpoint_inv(p0, &p));
		o_hcrot1(matr, dpoint_sub(p1, p0, &p));
		switch (First) {
			case 0:
				o_tdrot(matr, -2, M_PI / 2);
				o_tdrot(matr, -1, M_PI / 2);
				break;
			case 1:
				o_tdrot(matr, -1, -M_PI / 2);
				o_tdrot(matr, -2, -M_PI / 2);
				break;
		}
		if (flag > 2) {
			o_hcncrd(matr, p2, (lpD_POINT)pa);
			angle = -atan2(pa[Three], pa[Second]);
      if (Second != (First + 1) % 3) angle = -angle;
			o_tdrot(matr, -(First + 1), angle);
		}
		o_minv(matr, matr);
	}
}


BOOL place_ucs_to_lcs(hOBJ hobj, MATR matr_lcs_gcs)
{
	return place_ucs_to_lcs_common(hobj, matr_lcs_gcs, FALSE);
}
BOOL place_ucs_to_lcs_common(hOBJ hobj, MATR matr_lcs_gcs, BOOL lcs_path)
{
	OBJTYPE 			type;
	lpOBJ					obj;
	lpGEO_CIRCLE	circle;
	lpGEO_POINT		point;
	lpGEO_LINE		line;
	D_POINT				p;

	obj = (lpOBJ)hobj;
	type = obj->type;
	switch (type) {
		case OBREP:
		case OGROUP:
		case OINSERT:
		case OFRAME:
		case OTEXT:
			obj = (lpOBJ)hobj;
			memcpy(matr_lcs_gcs, obj->geo_data, sizeof(MATR));
			break;
		case ODIM:
			obj = (lpOBJ)hobj;
			memcpy(matr_lcs_gcs, ((lpGEO_DIM)obj->geo_data)->matr, sizeof(MATR));
			break;
		case OARC:
		case OCIRCLE:
			obj = (lpOBJ)hobj;
			circle = (lpGEO_CIRCLE)obj->geo_data;
			o_hcunit(matr_lcs_gcs);
			o_tdtran(matr_lcs_gcs, dpoint_inv(&circle->vc, &p));
			o_hcrot1(matr_lcs_gcs, &circle->n);
			o_minv(matr_lcs_gcs, matr_lcs_gcs);
			break;
		case OPOINT:
			obj = (lpOBJ)hobj;
			point = (lpGEO_POINT)obj->geo_data;
			create_ucs_matr1(1, point, NULL, NULL, matr_lcs_gcs, 0, 1);
			break;
		case OLINE:
			obj = (lpOBJ)hobj;
			line = (lpGEO_LINE)obj->geo_data;
			create_ucs_matr1(2, &line->v1, &line->v2, NULL, matr_lcs_gcs, 0, 1);
			break;
		case OPATH:
    	//++ SVP 19/09/2002
      {
        void* 	    par = NULL;
        lpGEO_PATH	path;
        hOBJ				hobj1;
				D_POINT			p[3];

        switch (get_primitive_param_2d(hobj, &par)) {
        	case RECTANGLE:
            obj 	= (lpOBJ)hobj;
            path 	= (lpGEO_PATH)obj->geo_data;
            hobj1 = path->listh.hhead;
            obj 	= (lpOBJ)hobj1;
            line 	= (lpGEO_LINE)obj->geo_data;
            p[0]	= line->v1;
            p[1]	= line->v2;
            hobj1 = path->listh.htail;
            obj 	= (lpOBJ)hobj1;
            line 	= (lpGEO_LINE)obj->geo_data;
            p[2]		= line->v1;
						create_ucs_matr1(3, &p[0], &p[2], &p[1], matr_lcs_gcs, 0, 1);
		      	break;
        	case POLYGON:
          	{
          	short 	NumSide;
            sgFloat 	OutRadius;
            UCHAR		color;

            memcpy(&NumSide, par, sizeof(NumSide));
            memcpy(&OutRadius, ((char*)par)+sizeof(NumSide), sizeof(OutRadius));
						CalcPoxyPolygon(hobj, OutRadius, &p[0], &p[1], &p[2], &color);
						create_ucs_matr1(3, &p[0], &p[1], &p[2], matr_lcs_gcs, 0, 1);
		      	break;
            }
        }
        if (par) {
        	SGFree(par); par = NULL;
          break;
        }
      }
      //--
		case OSPLINE:
			if (lcs_path) {
				obj = (lpOBJ)hobj;
				memcpy(matr_lcs_gcs, obj->geo_data, sizeof(MATR));
			} else {
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
				if (!status) { 
					create_ucs_matr1(2, &p[0], &p[1], NULL, matr_lcs_gcs, 0, 1);
				} else { 
					if (!set_flat_on_path(hobj, &plane)) return FALSE;
					dvector_product(&plane.v, dpoint_sub(&p[1],&p[0],&p[2]), &p[2]);
					dpoint_add(&p[2], &p[0], &p[2]);
					create_ucs_matr1(3, &p[0], &p[1], &p[2], matr_lcs_gcs, 0, 1);
				}
			}
			break;
		default:
			o_hcunit(matr_lcs_gcs);
			break;
	}
	{	
		D_POINT 	p0,	px, py, pz;
		D_POINT 	ex, ey, ez, exy;
		sgFloat		alpha;
		MATR			matr;

		memset(&p0,0,sizeof(p0));
		memset(&px,0,sizeof(px));
		px.x = 1;
		memset(&py,0,sizeof(py));
		py.y = 1;
		memset(&pz,0,sizeof(pz));
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
		if (alpha < 0)	{ 
			o_hcunit(matr);
			o_tdscal(matr, 2, -1);
			o_hcmult(matr, matr_lcs_gcs);
			memcpy(matr_lcs_gcs, matr, sizeof(MATR));
		}
	}
	{	
		D_POINT p0, px;
		sgFloat	scale;
		MATR		matr;

		memset(&p0, 0, sizeof(D_POINT));
		px =p0;
		px.x = 1.;
		o_hcncrd(matr_lcs_gcs, &p0, &p0);
		o_hcncrd(matr_lcs_gcs, &px, &px);
		scale = dpoint_distance(&p0, &px);
		if (fabs(scale - 1.) > eps_d) {
			o_hcunit(matr);
//			o_tdscal(matr, 4, 1. / scale);
			o_scale(matr, 1. / scale);
			o_hcmult(matr, matr_lcs_gcs);
			memcpy(matr_lcs_gcs, matr, sizeof(MATR));
		}
	}
	return TRUE;
}


OSCAN_COD ucs_geo_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	lpD_POINT     	point = (lpD_POINT)lpsc->data;
	UCHAR						direct;
	GEO_OBJ					curr_geo;
	OBJ							curr_obj;
	SPLY_DAT				sply_dat;
	D_POINT					curr_v[2], curr_p[2];
	void *       geo;

	direct = (BYTE)((lpsc->status & ST_DIRECT) ? 1 : 0);
	get_simple_obj(hobj, &curr_obj, &curr_geo);
	if (curr_obj.type != OSPLINE && direct)	
		pereor_geo(curr_obj.type, &curr_geo);
	if (curr_obj.type == OSPLINE) {
		if (!begin_use_sply((lpGEO_SPLINE)&curr_geo, &sply_dat)) return OSFALSE;
		geo = &sply_dat;
	} else {
		geo = &curr_geo;
	}
	get_geo_endpoints_and_vectors(curr_obj.type, geo, curr_p, curr_v);
	if (curr_obj.type == OSPLINE) {
		end_use_sply((lpGEO_SPLINE)&curr_geo, &sply_dat);
		if (direct) {	
			curr_p[0] = curr_p[1];
			dpoint_inv(&curr_v[1], &curr_v[0]);
		}
	}
	point[0] = curr_p[0];	
	dpoint_add(&curr_p[0], &curr_v[0], &point[1]);
																						
	return OSBREAK; 	
}

short get_primitive_param_2d(hOBJ hobj, void* *par)
{
	lpOBJ  obj;
	short	 ret;
	lpCSG	 csg;
	short	 partype;

	partype = sizeof(short);
	obj = (lpOBJ)hobj;
	switch (obj->type) {
		case OPATH:
				if (obj->hcsg) {
					csg = (lpCSG)(obj->hcsg);
					*par = SGMalloc(csg->size-partype);
					ret = *((short*)(LPSTR)csg->data);
					memcpy(*par, (LPSTR)(csg->data+partype), csg->size-partype);
				} else ret = -1;
			break;
		default:
			ret = -1;
			break;
	}
	return ret;
}
//--
