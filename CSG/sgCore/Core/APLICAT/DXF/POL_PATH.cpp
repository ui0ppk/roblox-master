#include "../../sg.h"



BOOL poly_path(sgFloat height, short flag, short type, short color,
							lpLISTH listh2, BOOL  flag_h, lpD_POINT v, lpD_POINT p10)
{
	D_POINT 		first, prev, next, last, min, max, vc, vb, ve, tmp, p0, p1, p01;
	lpD_POINT		point, pnt;
	sgFloat 			tang4, save_tang4, r;	//, derivate;
	sgFloat 			xm, ym, zm, h, ss, cc, xc, yc;
	short 			ff, closing, flag_v = 0, i;
	BOOL				ret;
	LISTH				listh;
	GEO_LINE		line;
	GEO_ARC			arc;
	GEO_PATH		path;
	GEO_SPLINE	spline;
	VDIM				vdim;
	MATR				gcs_pl, pl_gcs;
	hOBJ				hobj;
	lpOBJ				obj;
	int					nump, step, j, k, degree;

	init_listh(&listh);
	init_vdim(&vdim, sizeof(D_POINT));
	if (flag & 4) ff = 1; else ff = 0;
	closing = 0;
	for (;;) {
		if(memcmp(dxfg->str,dxf_keys[DXF_SEQEND],6) == 0) {
			if (flag & 4) break;
			if (flag & 1) {         
				closing = 1;
				memcpy(&next,&first,sizeof(first));
				goto cls;
			}
			break;
		}

		if( !memcmp(dxfg->str,dxf_keys[DXF_VERTEX],6) )  {

			tang4 = 0.;
//			derivate = 0;
			while (1) {
				if (!get_group(dxfg)) goto err;
				switch (dxfg->gr) {
					case 10:
						next = dxfg->xyz;
						if (!(flag & 8))
							next.z = p10->z;	
						continue;
					case 0:
						break;
					case 42:
						tang4 = dxfg->v;
						continue;
					case 70:
						flag_v = dxfg->f;
						continue;
//					case 50:	
//						derivate = dxfg->a;
//						continue;
					default:
						continue;
				}
				break;
			}
			if ((flag & 4) && (flag_v & 8)) 
				continue;
			if ( ff == 0) {
				memcpy(&prev,&next,sizeof(prev));
				save_tang4 = tang4;
				memcpy(&first,&prev,sizeof(prev));
				ff=1;
				continue;
			}
cls:
			if (flag & 4) {	
				o_hcncrd(dxfg->ocs_gcs, &next, &prev);
				if (!vdim.num_elem || !dpoint_eq(&last, &prev, eps_d)) {
					if (!add_elem(&vdim, &prev)) goto err;
					last = prev;
				}
			} else {	
				if (save_tang4 == 0.) {
					o_hcncrd(dxfg->ocs_gcs, &prev, &line.v1);
					o_hcncrd(dxfg->ocs_gcs, &next, &line.v2);
					if (!dpoint_eq(&line.v1, &line.v2, eps_d)) {
						if ( !cr_add_obj(OLINE, color, (lpGEO_SIMPLE)&line,
														 &listh, NULL, FALSE)) goto err;
					}
				} else {
					if (!dpoint_eq(&prev, &next, eps_d)) {
						xm = 0.5 * (prev.x + next.x);
						ym = 0.5 * (prev.y + next.y);
						zm = 0.5 * (prev.z + next.z);
						r  = sqrt((xm-prev.x)*(xm-prev.x) +
											(ym-prev.y)*(ym-prev.y) +
											(zm-prev.z)*(zm-prev.z));
						save_tang4 = 1.0 / save_tang4;
						h  = 0.5 * r * (save_tang4 * save_tang4 - 1.0) / save_tang4;
						ss = (prev.y - ym) / r;
						cc = (prev.x - xm) / r;
						xc = xm + h * ss;
						yc = ym - h * cc;
 //nb						r = sqrt((xc-prev.x)*(xc-prev.x)+(yc-prev.y)*(yc-prev.y));
						
						vc.x = xc; vc.y = yc; vc.z = prev.z;
						o_hcncrd(dxfg->ocs_gcs, &vc, &vc);
						o_hcncrd(dxfg->ocs_gcs, &prev, &vb);
						o_hcncrd(dxfg->ocs_gcs, &next, &ve);
						
						if (save_tang4 < 0) {
							tmp = vb; vb = ve; ve = tmp;
						}
						
						o_hcunit(gcs_pl);
						o_tdtran(gcs_pl, dpoint_inv(&vc, &tmp));
						o_hcrot1(gcs_pl, v);
						o_minv(gcs_pl, pl_gcs);
						o_hcncrd(gcs_pl, &vc, &vc);
						o_hcncrd(gcs_pl, &vb, &vb);
						o_hcncrd(gcs_pl, &ve, &ve);
						
						if (!arc_b_e_c(&vb, &ve, &vc,
													 TRUE, FALSE, FALSE, &arc)) goto err2;
						
						o_hcncrd(pl_gcs, &arc.vc, &arc.vc);
						o_hcncrd(pl_gcs, &arc.vb, &arc.vb);
						o_hcncrd(pl_gcs, &arc.ve, &arc.ve);
						arc.n = *v;
						if ( !cr_add_obj(OARC, color, (lpGEO_SIMPLE)&arc,
														 &listh, NULL, FALSE)) goto err;
						
						if (save_tang4 < 0) {
							hobj = listh.htail;
							obj = (lpOBJ)hobj;
							obj->status |= ST_DIRECT;
						}
					}
				}
			}
			if (closing) break;
			memcpy(&prev,&next,sizeof(prev));
			save_tang4 = tang4;
			continue;
		}
		if (!get_group(dxfg)) goto err;
	}
	if (flag & 4) {
		degree = (type == 5) ? 2 : 3;
		if (vdim.num_elem == 2) {	
			if (!read_elem(&vdim, 0, &p0)) goto err;
			if (!read_elem(&vdim, 1, &p1)) goto err;
			dpoint_parametr(&p0, &p1, 0.5, &p01);
			if (!add_elem(&vdim, &p1)) goto err;
			if (!write_elem(&vdim, 1, &p01)) goto err;
		}
		if (vdim.num_elem < degree + 2) {
			degree = (short)vdim.num_elem - 2;
			if (degree < 1) goto err2;	
		}
		spline.type = SPLY_NURBS | SPLY_APPR;
		spline.degree = degree;
		if (flag & 1) {
			spline.type |= SPLY_CLOSE;
			if (!read_elem(&vdim, 0, &p0)) goto err;
			if (!read_elem(&vdim, vdim.num_elem - 1, &p1)) goto err;
			if (!dpoint_eq(&p0, &p1, eps_d)) {
				if (!add_elem(&vdim, &p0)) goto err;
			}
		}
		nump = (short)vdim.num_elem;
		if (nump > MAX_POINT_ON_SPLINE) {
			step = (nump - 1) / MAX_POINT_ON_SPLINE + 1;
			j = nump / step;
			if (1 + j * step > nump) j--;
			if (1 + j * step < nump) j++;
			nump = j + 1;
		} else step = 1;
		spline.nump = nump;
		spline.numd = 0;
		if ((spline.hpoint = SGMalloc(sizeof(D_POINT)*nump)) == NULL) goto err;
		spline.hderivates = 0;
		if (!begin_rw(&vdim, 0)) {
err4:
			SGFree(spline.hpoint);
			goto err;
		}
		point = (lpD_POINT)spline.hpoint;
		for (i = 0, j = 0, k = 0; i < vdim.num_elem; i++) {
			if ((pnt = (D_POINT*)get_next_elem(&vdim)) == NULL) {
				end_rw(&vdim);
				goto err4;
			}
			if (!j || i == vdim.num_elem - 1) {
				if (k < nump)	point[k++] = *pnt;
			}
			j++;
			if (j == step) j = 0;
		}
		end_rw(&vdim);
		ret = dxf_extrude(OSPLINE, color, (lpGEO_SIMPLE)&spline, listh2,
											flag_h, v, height, FALSE);
		if (!ret) goto err;
	} else {
		if (!listh.num) goto err2;
		memset(&path, 0, sizeof(path));
		o_hcunit(path.matr);
		path.listh = listh;
		hobj = listh.hhead;
		path.min.x = path.min.y =  path.min.z =  GAB_NO_DEF;
		path.max.x = path.max.y =  path.max.z = -GAB_NO_DEF;
		while (hobj) {
			if (!get_object_limits(hobj, &min, &max)) goto err;
			union_limits(&min, &max, &path.min, &path.max);
			get_next_item(hobj, &hobj);
		}
		ret = dxf_extrude(OPATH, color, (lpGEO_SIMPLE)&path, listh2,
											flag_h, v, height, FALSE);
	}
	goto ex;
err:
	ret = FALSE;
	goto ex;
err2:
	ret = TRUE;
ex:
	free_vdim(&vdim);
	return ret;
}
