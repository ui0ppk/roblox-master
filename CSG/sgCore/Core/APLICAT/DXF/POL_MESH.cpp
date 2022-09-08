#include "../../sg.h"

static BOOL write_sloy(lpD_POINT v1, lpD_POINT v2, int num, lpTRI_BIAND trb);

BOOL poly_mesh(short m, short flag, short color, lpLISTH listh)
{
	D_POINT 		*sl = NULL; 
	D_POINT 		*v1;
	D_POINT 		*v2;
	short      		count;      
	D_POINT 		v;
	int					flag_v;
	int					first = 1;
	TRI_BIAND		trb;
	BOOL				ret;
	int					m1;
	OBJ_ATTRIB	save_obj_attrib;

	if (flag & 32) m1 = m + 1;	
	else           m1 = m;
	if ((v1 = (D_POINT*)SGMalloc(m1 * sizeof(D_POINT))) == NULL) {
skip1:
		put_message(NOT_ENOUGH_HEAP, NULL, 0);
		return skip_vertex();
	}
	if ((v2 = (D_POINT*)SGMalloc(m1 * sizeof(D_POINT))) == NULL) {
skip2:
		SGFree(v1);
 //nb 		v1 = NULL;
		goto skip1;
	}
	if (flag & 1) {	
		if ((sl =(D_POINT*)SGMalloc(m1 * sizeof(D_POINT))) == NULL) {
//nb skip3:
			SGFree(v2);
 //nb 			v2 = NULL;
			goto skip2;
		}
	}

	if ( !begin_tri_biand(&trb) ) goto fat_err;
	count = 0;	
	for (;;) {
		if( !memcmp(dxfg->str,dxf_keys[DXF_VERTEX],6) )  {
			while (1) {
				if (!get_group(dxfg)) goto fat_err;
				switch (dxfg->gr) {
					case 10:
						v = dxfg->xyz;
						continue;
					case 70:
						flag_v = dxfg->f;
						continue;
					case 0:
						break;
					default:
						continue;
				}
				break;
			}
			if ((flag & 4) && (flag_v & 16))
				continue; 
			memcpy(&v2[count++], &v, sizeof(D_POINT));
			if (count == m) {
				count = 0;
				if (flag & 32) v2[m] = v2[0];
				if (first && (flag & 1)) {
					memcpy(sl, v2, m1 * sizeof(D_POINT));
				}
				if (!first) {
					if (!write_sloy(v1, v2, m1, &trb)) goto fat_err;
				}
				memcpy(v1, v2, m1 * sizeof(D_POINT));
				first = 0;
			}
			continue;
		}
		if (memcmp(dxfg->str,dxf_keys[DXF_SEQEND],6) == 0) {
			if (flag & 1) {         
				memcpy(v2, sl, m1 * sizeof(D_POINT));
				if (!write_sloy(v1, v2, m1, &trb)) goto fat_err;
			}
			
			while (TRUE) {
				if (!get_group(dxfg)) goto fat_err;
				if (dxfg->gr == 0) break;
			}
			break;
		}
		goto fat_err;	
	}
	if (v1) SGFree(v1); //nb v1 = NULL;
	if (v2) SGFree(v2); //nb v2 = NULL;
	if (sl) SGFree(sl); //nb sl = NULL;
	save_obj_attrib = obj_attrib;
	obj_attrib.color = (BYTE)color;
	obj_attrib.flg_transp	= TRUE;
	ret = end_tri_biand(&trb, listh);
	obj_attrib = save_obj_attrib;
	return ret;

fat_err:
	save_obj_attrib = obj_attrib;
	obj_attrib.color = (BYTE)color;
	obj_attrib.flg_transp	= TRUE;
	/*nb ret = */end_tri_biand(&trb, listh);
	obj_attrib = save_obj_attrib;
	if (v1) SGFree(v1); //nb v1 = NULL;
	if (v2) SGFree(v2); //nb v2 = NULL;
	if (sl) SGFree(sl); //nb sl = NULL;
	skip_vertex();
	return FALSE;
}

static BOOL write_sloy(lpD_POINT v1, lpD_POINT v2, int num, lpTRI_BIAND trb)
{
	int		i;

	for (i = 0; i < num - 1; i++) {
		if (!put_face_4(&v1[i], &v1[i+1], &v2[i+1], &v2[i],
				TRP_APPROCH | (TRP_APPROCH << 2) | (TRP_APPROCH << 4) |
				(TRP_APPROCH << 6), trb))
			return FALSE;
	}
	return TRUE;
}
