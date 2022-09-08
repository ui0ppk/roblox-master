#include "../../sg.h"


BOOL dxf_3dface(lpVDIM faces)
{
	FACE4 	face;
	short		color = 0;
	int			num = 0;
	LAYER		elem;

	memset(&face, 0, sizeof(face));
	while (TRUE) {
		if (!get_group(dxfg)) return FALSE;
		switch (dxfg->gr) {
			case 8:
				if (find_elem(&dxfg->layers, &num, dxf_cmp_func, (void*)dxfg->str)) {
					if (!read_elem(&dxfg->layers, num, &elem)) return FALSE;
					color = elem.color;
				} else {
					strncpy(elem.name, dxfg->str, LEN_NAME_LAYER);
					elem.name[LEN_NAME_LAYER] = 0;
					elem.color = 0;
					if (!add_elem(&dxfg->layers, &elem)) return FALSE;
					num = dxfg->layers.num_elem - 1;
				}
				continue;
			case 62:
				color = dxfg->p;
				continue;
			case 10:
				face.p1 = dxfg->xyz;
				continue;
			case 11:
				face.p2 = dxfg->xyz;
				continue;
			case 12:
				face.p3 = dxfg->xyz;
				continue;
			case 13:
				face.p4 = dxfg->xyz;
				continue;
			case 70:                  
				face.flag_v = dxfg->f;
				continue;
			case 0:
				break;
			default:
				continue;
		}
		face.color = dxf_color(color);
		break;
	}
	face.layer = (short)num;
	face.flag = TRUE;
	return add_elem(faces, &face);
}
