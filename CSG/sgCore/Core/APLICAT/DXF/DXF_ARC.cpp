#include "../../sg.h"

BOOL dxf_arc(lpLISTH listh)
{
	sgFloat		height = 0;		
	D_POINT		vc, vb, ve, tmp;
	D_POINT		v = {0,0,1};	
	GEO_ARC		geo;
	sgFloat		ab, ae, r;
	MATR			gcs_pl, pl_gcs;
	BOOL			flag_h = FALSE;
	short			color = 0;
	int			num;
	LAYER			elem;

	ab = 0;
	ae = 2*M_PI;

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
			case 10:	// center
				vc	= dxfg->xyz;
				continue;
			case 50: /* begin angle */
				ab	= d_radian(dxfg->a);
				continue;
			case 51: /* end angle */
				ae	= d_radian(dxfg->a);
				continue;
			case 40:	// radius
				r	= dxfg->v;
				continue;
			case 210:	// ort
				v = dxfg->xyz;
				continue;
			case 39:	/* extrus */
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
	if (r < eps_d) return TRUE;

	vb.x = vc.x + r * cos(ab);
	vb.y = vc.y + r * sin(ab);
	vb.z = vc.z;
	ve.x = vc.x + r * cos(ae);
	ve.y = vc.y + r * sin(ae);
	ve.z = vc.z;
	
	ocs_gcs(&v, dxfg->ocs_gcs, dxfg->gcs_ocs);
	o_hcncrd(dxfg->ocs_gcs, &vc, &vc);
	o_hcncrd(dxfg->ocs_gcs, &vb, &vb);
	o_hcncrd(dxfg->ocs_gcs, &ve, &ve);
	
	o_hcunit(gcs_pl);
	o_tdtran(gcs_pl, dpoint_inv(&vc, &tmp));
	o_hcrot1(gcs_pl, &v);
	o_minv(gcs_pl, pl_gcs);
	o_hcncrd(gcs_pl, &vc, &vc);
	o_hcncrd(gcs_pl, &vb, &vb);
	o_hcncrd(gcs_pl, &ve, &ve);
	
	if (!arc_b_e_c(&vb, &ve, &vc,
								 TRUE, FALSE, FALSE, &geo)) return TRUE;
	
	o_hcncrd(pl_gcs, &geo.vc, &geo.vc);
	o_hcncrd(pl_gcs, &geo.vb, &geo.vb);
	o_hcncrd(pl_gcs, &geo.ve, &geo.ve);
	geo.n = v;
	return dxf_extrude(OARC, color, (lpGEO_SIMPLE)&geo, listh,
										 flag_h, &v, height, FALSE);
}
