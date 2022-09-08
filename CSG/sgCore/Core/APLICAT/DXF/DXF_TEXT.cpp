#include "../../sg.h"

BOOL dxf_text(lpLISTH listh)
{
GEO_TEXT 	gtext;
sgFloat		height;		
D_POINT		vc;
D_POINT		vn = {0,0,1};	
sgFloat		a;
short			color = 0;
LAYER			elem;
BOOL      flag_h;
int				num;


	gtext.text_id = 0;
	gtext.font_id = ft_bd->curr_font;
	gtext.style.status = 0;
	gtext.style.height = 5.;
	gtext.style.scale = 100.;
	gtext.style.angle = 0;
	gtext.style.dhor = 0;
	gtext.style.dver = 50;

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
				vc	= dxfg->xyz;
				continue;
			case 50: 
				a	= d_radian(dxfg->a);
				continue;
			case 51: 
				gtext.style.angle	= dxfg->a;
				continue;
			case 40:	
				gtext.style.height = dxfg->v/get_grf_coeff();
				continue;
			case 41:	
				gtext.style.scale = dxfg->v*100;
				continue;
			case 210:	
				vn = dxfg->xyz;
				continue;
			case 39:	
				continue;
			case 1:	
				gtext.text_id = add_ft_value(FTTEXT, dxfg->str, strlen(dxfg->str) + 1);
				continue;
			case 0:	
				break;
			default:
				continue;
		}
		color = dxf_color(color);
		break;
	}
	if(!gtext.text_id)
		return TRUE;
	o_hcunit(gtext.matr);
	gtext.min.x = gtext.min.y = gtext.min.z =  GAB_NO_DEF;
	gtext.max.x = gtext.max.y = gtext.max.z = -GAB_NO_DEF;

	if(!draw_geo_text(&gtext, text_gab_lsk, &gtext.min)) return TRUE;
	if(gtext.min.x == GAB_NO_DEF) return TRUE;

	ocs_gcs(&vn, dxfg->ocs_gcs, dxfg->gcs_ocs);
	o_tdrot(gtext.matr, -3, a);
	o_hcmult(gtext.matr, dxfg->ocs_gcs);
	o_tdtran(gtext.matr, &vc);

	flag_h = FALSE;
	height = 0;
	return dxf_extrude(OTEXT, color, (lpGEO_SIMPLE)&gtext, listh,
										 flag_h, &vn, height, FALSE);
}

