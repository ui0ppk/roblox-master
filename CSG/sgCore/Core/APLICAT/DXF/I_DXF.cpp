#include "../../sg.h"

lpDXF_GROUP dxfg;

static  BOOL load_entities(lpLISTH listh, int last_block, BOOL first);
static  BOOL load_blocks(int last_block);
static  BOOL load_tables(void);

BOOL ACAD_R13 = FALSE;

short ImportDxf(lpBUFFER_DAT bd, lpLISTH listh)
{
	DXF_GROUP dxfgg;
	short			count = (short)listh->num;
	int			  last_block;
	int				len;
	char			code[2];

	dxfg = &dxfgg;		
	dxfg->bd = bd;    
	len = strlen(dxf_binary_tytle) + 1;
	if (load_data(bd, len, dxfg->str) != (WORD)len) return 0;
	if (!strcmp(dxfg->str, dxf_binary_tytle)) {
		dxfg->binary = TRUE;
		if (load_data(bd, sizeof(code), code) != sizeof(code)) return 0;
		if (code[1] == 0) ACAD_R13 = TRUE;
		else              ACAD_R13 = FALSE;
		if (!seek_buf(bd, (int)len)) return 0;
	} else {
		dxfg->binary = FALSE;
		if (!seek_buf(bd, 0L)) return 0;
	}
	init_vdim(&dxfg->layers, sizeof(LAYER));
	last_block = vd_blocks.num_elem;

//	dxfg->num_proc = start_grad(GetIDS(IDS_SG004), bd->len_file);

	if (application_interface && application_interface->GetProgresser()) 
		application_interface->GetProgresser()->Progress(0);
	while (1) {
		if (!get_group(dxfg)) goto end;
		switch (dxfg->gr ) {
			case 0:
				if (!memcmp(dxfg->str,dxf_keys[DXF_SECTION],7)) {
					if (!get_group(dxfg)) goto end;
					switch (dxfg->gr ) {
						case 2:
							if (!memcmp(dxfg->str,dxf_keys[DXF_TABLES],6)) {
								if (!load_tables()) goto end;
								break;
							}
							if (!memcmp(dxfg->str,dxf_keys[DXF_BLOCKS],6)) {
								if (!load_blocks(last_block)) goto end;
								break;
							}
							if (!memcmp(dxfg->str,dxf_keys[DXF_ENTITIES],8)) {
								if (!load_entities(listh, last_block, FALSE)) goto end;
								break;
							}
							break;
					}
					continue;
				}
				if (!memcmp(dxfg->str,dxf_keys[DXF_EOF],3)) goto end;
		}
	}
end:
//	end_grad(dxfg->num_proc , bd->len_file);
	if (application_interface && application_interface->GetProgresser()) 
		application_interface->GetProgresser()->Progress(100);
	free_vdim(&dxfg->layers);
	return ((short)listh->num) - count;
}

#pragma argsused
static  BOOL load_entities(lpLISTH listh, int last_block, BOOL first)
{
	VDIM 	faces;
	BOOL	ret;

	init_vdim(&faces, sizeof(FACE4));
	if (!first) {
		if (!get_group(dxfg)) goto err;
	}
	while (1) {
		if (dxfg->gr )	{
			if (!get_group(dxfg)) goto err;
			continue;  
		}

		if (!memcmp(dxfg->str,dxf_keys[DXF_LINE],4)) {
			if (!dxf_line(listh)) goto err;
			continue;
		}
		if (!memcmp(dxfg->str,dxf_keys[DXF_3DLINE],6)) {
			if (!dxf_line(listh)) goto err;
			continue;
		}
		if (!memcmp(dxfg->str,dxf_keys[DXF_CIRCLE],6)) {
			if (!dxf_circle(listh)) goto err;
			continue;
		}
		if (!memcmp(dxfg->str,dxf_keys[DXF_ARC],3)) {
			if (!dxf_arc(listh)) goto err;
			continue;
		}
		if (!memcmp(dxfg->str,dxf_keys[DXF_TEXT],4)) {
			if (!dxf_text(listh)) goto err;
			continue;
		}
		if (!memcmp(dxfg->str,dxf_keys[DXF_POLYLINE],8)) {
			if (!dxf_poly(listh)) goto err;
			continue;
		}
		if (!memcmp(dxfg->str,dxf_keys[DXF_INSERT],6)) {
			if (!dxf_insert(listh, last_block)) goto err;
			continue;
		}
		if (!memcmp(dxfg->str,dxf_keys[DXF_3DFACE],6)) {
			if (!dxf_3dface(&faces)) goto err;
			continue;
		}
		if (!memcmp(dxfg->str,dxf_keys[DXF_SOLID],5)) {
			if (!dxf_solid(listh, &faces)) goto err;
			continue;
		}
		if (!memcmp(dxfg->str,dxf_keys[DXF_ENDBLK],6)) goto ex;
		if (!memcmp(dxfg->str,dxf_keys[DXF_ENDSEC],6)) goto ex;
		if (!get_group(dxfg)) goto err;
	}
ex:
	ret = TRUE;
	{	
		TRI_BIAND		trb;
		lpFACE4			face;
		BOOL				first;
		int					i, color, layer, flag;
		OBJ_ATTRIB	save_obj_attrib;
		int				rest;

		rest = faces.num_elem;
		while (rest > 0) {
			if ( !begin_tri_biand(&trb) ) goto err;
			first = TRUE;
			if (!begin_rw(&faces, 0)) goto err;
			for	(i = 0; i < faces.num_elem; i++) {
				if ((face = (FACE4*)get_next_elem(&faces)) == NULL) goto err;
				if (face->flag) {
					if (first) {
						color = face->color;
						layer = face->layer;
						first = FALSE;
					}
					if (face->color == color &&
							face->layer == layer) {
						flag = 0;
						flag |= (face->flag_v & 1) ? TRP_DUMMY      : TRP_APPROCH;
						flag |= (face->flag_v & 2) ? TRP_DUMMY << 2 : TRP_APPROCH << 2;
						flag |= (face->flag_v & 4) ? TRP_DUMMY << 4 : TRP_APPROCH << 4;
						flag |= (face->flag_v & 8) ? TRP_DUMMY << 6 : TRP_APPROCH << 6;
						if (!(ret = put_face_4(&face->p1, &face->p2,
																	 &face->p3, &face->p4,
																	 flag,
																	 &trb))) break;
						face->flag = FALSE;
						if (!(--rest)) break;	
					}
				}
			}	// for
			end_rw(&faces);
			save_obj_attrib = obj_attrib;
			obj_attrib.color = (BYTE)color;
			obj_attrib.flg_transp	= TRUE;
			ret = end_tri_biand(&trb, listh);
			obj_attrib = save_obj_attrib;
			if (!ret) break;
		}	// while
	}
ex2:
	free_vdim(&faces);
	return ret;
err:
	ret = FALSE;
	goto ex2;
}


static  BOOL load_blocks(int last_block)
{
	IBLOCK		blk;
	D_POINT		v;            
	int				flag;
	hOBJ			hobj, hnext;
	D_POINT		min, max;
	MATR			matr;
	BOOL			Bool;

	if (!get_group(dxfg)) goto err;
	while (1) {
		switch (dxfg->gr ) {
			case 0:
				if (!memcmp(dxfg->str,dxf_keys[DXF_BLOCK],5)) {
					
					memset(&blk,0,sizeof(blk));
					memset(&v,0,sizeof(v));
					flag = 0;
					while (1) {
						if (!get_group(dxfg)) goto err;
						switch (dxfg->gr) {
							case 0:
								break;
							case 2:	
								strncpy(blk.name, dxfg->str, MAXBLOCKNAME);
								blk.name[MAXBLOCKNAME - 1] = 0;
								continue;
							case 70:	
								flag = dxfg->f;
								continue;
							case 10:	
								v = dxfg->xyz;
								continue;
							default:
								continue;
						}
						break;
					}
					if ((flag & (4 | 16 | 32)) != 0 && (flag & 64) == 0)
						break;	
					Bool = load_entities(&blk.listh, last_block, TRUE);
					if (blk.listh.num) {							
						
						o_hcunit(matr);
						o_tdtran(matr, dpoint_inv(&v, &v));
						
						hobj = blk.listh.hhead;
						blk.min.x = blk.min.y = blk.min.z =  GAB_NO_DEF;
						blk.max.x = blk.max.y = blk.max.z = -GAB_NO_DEF;
						while (hobj) {
							if (!o_trans(hobj, matr)) goto del;
							if (!get_object_limits(hobj, &min, &max)) goto del;
							union_limits(&min, &max, &blk.min, &blk.max);
							get_next_item(hobj, &hobj);
						}
						if (!add_elem(&vd_blocks,&blk)) goto del;
					}
					if (!Bool && !blk.listh.num) goto err;
					break;
				}
				if (!memcmp(dxfg->str,dxf_keys[DXF_ENDSEC],6)) goto ex;
		}
		if (!get_group(dxfg)) goto err;
	}
del:
	hobj = blk.listh.hhead;
	while (hobj) {
		get_next_item(hobj, &hnext);
		o_free(hobj, NULL);
		hobj = hnext;
	}
	goto err;
ex:
	return TRUE;
err:
	return FALSE;
}

static  BOOL load_tables(void)
{
	LAYER	elem;
	BOOL	tog;

	if (!get_group(dxfg)) goto err;
	while (1) {
		switch (dxfg->gr ) {
			case 0:
				if (!memcmp(dxfg->str,dxf_keys[DXF_TABLE],5)) {
					
					if (!get_group(dxfg)) break;
					if (!memcmp(dxfg->str,dxf_keys[DXF_LAYER],5)) {
						
						tog = FALSE;
						while (1) {
							if (!get_group(dxfg)) break;
							switch (dxfg->gr) {
								case 0:
									if (tog) break;
									continue;
								case 2:
									strncpy(elem.name,dxfg->str,LEN_NAME_LAYER);
									elem.name[LEN_NAME_LAYER] = 0;
									tog = TRUE;
									continue;
								case 62:
									elem.color = dxfg->p;
									continue;
								default:
									continue;
							}
							if (tog) {
								if (!add_elem(&dxfg->layers, &elem)) goto err;
								tog = FALSE;
							}
							if (!memcmp(dxfg->str,dxf_keys[DXF_ENDTAB],5)) break;
						}
					}
				}
				if (!memcmp(dxfg->str,dxf_keys[DXF_ENDSEC],6)) goto ex;
		}
		if (!get_group(dxfg)) goto err;
	}
ex:
	return TRUE;
err:
	return FALSE;
}

