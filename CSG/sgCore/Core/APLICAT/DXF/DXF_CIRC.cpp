#include "../../sg.h"

BOOL dxf_circle(lpLISTH listh)
{
	sgFloat			height = 0;	
	D_POINT			v = {0,0,1};	
	GEO_CIRCLE	geo;
	BOOL				flag_h = FALSE;
	short				color = 0;
	int					num;
	LAYER				elem;

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
				geo.vc	= dxfg->xyz;
				continue;
			case 40:	
				geo.r	= dxfg->v;
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
	ocs_gcs(&v, dxfg->ocs_gcs, dxfg->gcs_ocs);
	o_hcncrd(dxfg->ocs_gcs, &geo.vc, &geo.vc);
	geo.n = v;
	if (geo.r < eps_d) return TRUE;	
	return dxf_extrude(OCIRCLE, color, (lpGEO_SIMPLE)&geo, listh,
										 flag_h, &v, height, FALSE);
}
