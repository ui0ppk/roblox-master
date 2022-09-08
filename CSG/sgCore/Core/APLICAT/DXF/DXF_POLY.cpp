#include "../../sg.h"

BOOL dxf_poly(lpLISTH listh)
{
	short   m = 0, m2 = 0, flag = 0;
//	int		n = 0, n2 = 0;
	int			num;
	short		type = 0;
	sgFloat  height = 0;      
	BOOL		flag_h = FALSE;
	D_POINT v = {0,0,1}, p10;
	short   color = 0;
	LAYER		elem;

	while (1) {
		if (!get_group(dxfg)) return FALSE;
		switch (dxfg->gr) {
			case 10:                 
				p10 = dxfg->xyz;
				continue;
			case 8:
				num = 0;
				if (find_elem(&dxfg->layers, &num, dxf_cmp_func, (void*)dxfg->str)) {
					if (!read_elem(&dxfg->layers, num, &elem)) return FALSE;
					color = elem.color;
				}
				continue;
			case 62:
				color = dxfg->p;
				continue;
			case 70:
				flag = dxfg->f;
				continue;
//			case 71:
//				n = dxfg->f;
//				continue;
			case 72:
				m = dxfg->f;
				continue;
//			case 73:
//				n2 = dxfg->f;
//				continue;
			case 74:
				m2 = dxfg->f;
				continue;
			case 75:
				type = dxfg->f;	
				continue;
			case 210:                
				v = dxfg->xyz;
				continue;
			case 39:                  
				height = dxfg->v;
				if (height > eps_d) flag_h = TRUE;
				continue;
			case 0:
				break;
			default:
				continue;
		}
		color = dxf_color(color);
		break;
	}
	if (flag &  16) { 
		if (flag & 4) {
			m = m2;	// n = n2;
		}
		return poly_mesh(m, flag, color, listh);
	}
	if (flag &  64) return poly_mesh64(color, listh);	
	ocs_gcs(&v, dxfg->ocs_gcs, dxfg->gcs_ocs);
	
	return poly_path(height, flag, type, color,
									 listh, flag_h, &v, &p10);
}
