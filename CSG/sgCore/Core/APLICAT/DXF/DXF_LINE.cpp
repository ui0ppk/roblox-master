#include "../../sg.h"

BOOL dxf_line(lpLISTH listh)
{
	GEO_LINE	line;
	sgFloat		height = 0;		
	D_POINT		tmp, v = {0,0,1};	
	BOOL			flag_h = FALSE;
	short			color = 0;
	int			  num;
	LAYER			elem;

	while (1) {
		if ( !get_group(dxfg)) return FALSE;
		switch (dxfg->gr) {
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
			case 10:
				line.v1 = dxfg->xyz;
				continue;
			case 11:
				line.v2 = dxfg->xyz;
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
	if (dpoint_eq(&line.v1, &line.v2, eps_d))
		return TRUE;	
	if (flag_h) {
		if (coll_tst(&v, TRUE, dpoint_sub(&line.v2, &line.v1, &tmp), FALSE))
			flag_h = FALSE;	
	}
	return dxf_extrude(OLINE, color, (lpGEO_SIMPLE)&line, listh,
										 flag_h, &v, height, FALSE);
}

