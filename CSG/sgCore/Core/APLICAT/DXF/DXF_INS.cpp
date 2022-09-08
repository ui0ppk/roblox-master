#include "../../sg.h"

BOOL dxf_insert(lpLISTH listh, int last_block)
{
	D_POINT 		v = {0,0,1};                  
	D_POINT 		point, p, min, max;              
  hOBJ				hobj;
	MATR 				A;
	char 				name[MAXBLOCKNAME];
	short 				i, j;
	int				cur_blk;
	sgFloat 			sx = 1., sy = 1., sz = 1., az = 0.;
	short 				nx = 1, ny = 1;
	sgFloat 			dx = 0., dy = 0.;
	GEO_INSERT	geo;
	IBLOCK			blk;

	while (1) {
		if (!get_group(dxfg)) return FALSE;
		switch (dxfg->gr) {
			case 2:
				strncpy(name, dxfg->str, MAXBLOCKNAME);
				name[MAXBLOCKNAME - 1] = 0;
				cur_blk = last_block;
				if (!find_elem(&vd_blocks, &cur_blk, dxf_cmp_func, name)) {
					dxf_err(NO_SUCH_BLOCK);
					return TRUE;
				}
				continue;
			case 10:
				point = dxfg->xyz;
				continue;
			case 41:
				sx = dxfg->v;
				continue;
			case 42:
				sy = dxfg->v;
				continue;
			case 43:
				sz = dxfg->v;
				continue;
			case 44:
				dx = dxfg->v;
				continue;
			case 45:
				dy = dxfg->v;
				continue;
			case 50:
				az = dxfg->a;
				continue;
			case 70:
				nx = dxfg->f;
				continue;
			case 71:
				ny = dxfg->f;
				continue;
			case 210:                 
				v = dxfg->xyz;
				continue;
			case 0:
				break;

			default:
				continue;
		}
		break;
	}

	ocs_gcs(&v, dxfg->ocs_gcs, dxfg->gcs_ocs);

	for (i = 0; i < nx; i++) {
		for (j = 0; j < ny; j++) {

			o_hcunit(A);
			o_tdscal(A, 1, sx);
			o_tdscal(A, 2, sy);
			o_tdscal(A, 3, sz);
			p.x = i * dx;
			p.y = j * dy;
			p.z = 0;
			o_tdtran(A, &p);
			o_tdrot(A, 3, az);
			o_tdtran(A, &point);

			o_hcmult(A, dxfg->ocs_gcs);

			memcpy(geo.matr, A, sizeof(MATR));
			geo.num = cur_blk;
			geo.min.x = geo.min.y = geo.min.z =  GAB_NO_DEF;
			geo.max.x = geo.max.y = geo.max.z = -GAB_NO_DEF;
			if (!read_elem(&vd_blocks, cur_blk, &blk)) return FALSE;
			hobj = blk.listh.hhead;
			while (hobj) {
				if (!get_object_limits_lsk(hobj, &min, &max, A)) return FALSE;
				union_limits(&min, &max, &geo.min, &geo.max);
				get_next_item(hobj, &hobj);
			}

			if ( !cr_add_obj(OINSERT, CTRANSP, (lpGEO_SIMPLE)&geo, listh,
											 NULL, FALSE)) return FALSE;
		}
	}
	return TRUE;
}
