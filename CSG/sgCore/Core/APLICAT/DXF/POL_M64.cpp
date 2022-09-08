#include "../../sg.h"

static BOOL get_vertex(lpVDIM vertex, int num, int count, lpD_POINT p);

BOOL poly_mesh64(short color, lpLISTH listh)
{
	short      		count;     
	D_POINT 		v;
	int					flag_v;
	TRI_BIAND		trb;
	BOOL				ret;
	OBJ_ATTRIB	save_obj_attrib;
	VDIM				vertex;
	int					num1, num2, num3, num4, flag;
	D_POINT			p1, p2, p3, p4;
	GEO_LINE		line;

	if (!init_vdim(&vertex, sizeof(D_POINT))) return FALSE;

	if ( !begin_tri_biand(&trb) ) goto fat_err;
	count = 0;	
	for (;;) {
		if (!memcmp(dxfg->str,dxf_keys[DXF_VERTEX],6))  {
			num1 = num2 = num3 = num4 = 0;
			while (1) {
				if (!get_group(dxfg)) goto err2;
				switch (dxfg->gr) {
					case 10:
						v = dxfg->xyz;
						continue;
					case 70:
						flag_v = dxfg->f;
						continue;
					case 71:
						num1 = dxfg->f;
						continue;
					case 72:
						num2 = dxfg->f;
						continue;
					case 73:
						num3 = dxfg->f;
						continue;
					case 74:
						num4 = dxfg->f;
						continue;
					case 0:
						break;
					default:
						continue;
				}
				break;
			}
			if (flag_v & 64) {	
				if (!add_elem(&vertex, &v)) goto err2;
				count++;
				continue;
			}
			if (!num1) continue;
			if (!get_vertex(&vertex, num1, count, &p1)) goto err2;
			if (!num2) continue;
			if (!get_vertex(&vertex, num2, count, &p2)) goto err2;
			if (!num3) {
				line.v1 = p1;
				line.v2 = p2;
				if ( !cr_add_obj(OLINE, color, (lpGEO_SIMPLE)&line,
												 listh, NULL, FALSE)) goto err2;
				continue;
			}
			if (!get_vertex(&vertex, num3, count, &p3)) goto err2;
			if (num4) {
				if (!get_vertex(&vertex, num4, count, &p4)) goto err2;
			} else p4 = p3;
			flag = 0;
			flag |= (num1 < 0) ? TRP_DUMMY      : TRP_APPROCH;
			flag |= (num2 < 0) ? TRP_DUMMY << 2 : TRP_APPROCH << 2;
			flag |= (num3 < 0) ? TRP_DUMMY << 4 : TRP_APPROCH << 4;
			flag |= (num4 < 0) ? TRP_DUMMY << 6 : TRP_APPROCH << 6;
			if (!put_face_4(&p1, &p2, &p3, &p4, flag, &trb)) goto err2;
			continue;
		}
		if (memcmp(dxfg->str,dxf_keys[DXF_SEQEND],6) == 0) break;
		put_message(INTERNAL_ERROR,"pol_m64.c",0);
		goto err2;	
	}
	free_vdim(&vertex);
	save_obj_attrib = obj_attrib;
	obj_attrib.color = (BYTE) color;
	obj_attrib.flg_transp	= TRUE;
	ret = end_tri_biand(&trb, listh);
	obj_attrib = save_obj_attrib;
	return ret;

err2:
	save_obj_attrib = obj_attrib;
	obj_attrib.color = (BYTE) color;
	obj_attrib.flg_transp	= TRUE;
	/*nb ret = */end_tri_biand(&trb, listh);
	obj_attrib = save_obj_attrib;
fat_err:
	free_vdim(&vertex);
	skip_vertex();
	return FALSE;
}

static BOOL get_vertex(lpVDIM vertex, int num, int count, lpD_POINT p)
{
	int	n = abs(num);

	if (n > count) {
		dxf_err(DXF_ERROR);
		return FALSE;
	}
	if (!read_elem(vertex, n - 1, p)) return FALSE;
	return TRUE;
}
