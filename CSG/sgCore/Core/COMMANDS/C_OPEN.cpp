#include "../sg.h"

BOOL bool_var[] = {              
	TRUE,	  //GROUPTRANSP	
	FALSE,	//AUTOBOX
	FALSE,	//AUTODELETE
	TRUE,		//AUTOCOMMAND
	FALSE,  //AUTOSINGLE
	FALSE,	//AUTODEL_CINEMA
	FALSE,	//DRAG_IMAGE
	FALSE,  //SAVE_ON_UCS
  TRUE,		//LCS_PATH
	FALSE,  //CONTOSNAP
};

BOOL bool_dis[] = {             
	FALSE,	
	TRUE,		
	TRUE,		
};
BOOL bool_vis[] = {             
	TRUE,		//	ST_TD_CONSTR                  
	TRUE,		//  ST_TD_SKETCH                    
	TRUE,		//	ST_TD_COND                     
	FALSE,	//	ST_TD_APPROCH                   
	FALSE,	//  ST_TD_DUMMY                     
	FALSE,	//	ST_TD_FULL                     
};


BOOL NewCommon(void)
{
	hOBJ          hobj, hnext;

	
	hobj = objects.hhead;
	while (hobj) 
	{
		get_next_item(hobj, &hnext);
		o_free(hobj, &objects);
		hobj = hnext;
	}
	init_listh(&objects);
	init_listh(&selected);

	D_POINT min = {-100, -100, -100}, max = {100, 100, 100};

	if (!objects.hhead) 
	{
		scene_min.x = scene_min.y = scene_min.z =  GAB_NO_DEF;
		scene_max.x = scene_max.y = scene_max.z = -GAB_NO_DEF;
		modify_limits(&min, &max);
	}

	free_blocks_list();
	free_np_brep_list();

	init_vdim(&vd_blocks, sizeof(IBLOCK));
	init_vdim(&vd_brep,   sizeof(LNP));
	
	return TRUE;
}




GS_CODE load_model(char *name, MATR matr, BOOL draw)
{
	GS_CODE     gs_code = G_OK;
	int					count;//nb  = 0;	
	char 				tcount[128], text[15];
	hOBJ    		htail, hobj;
	BOOL    		code;//, regen;
	D_POINT 		p_min, p_max;
	WORD			  i;
	int					num_inter_block, num_extern_block, num_obj;
	sgFloat      dd;
	short 			num_inter, num_extern;
	IBLOCK  		blk;
	
	int cur_ind=0;
	htail = objects.htail;
	num_obj			= (short)objects.num;
	num_inter_block = num_extern_block = 0;
	for (i = 0; i < vd_blocks.num_elem; i++) {
		if (!read_elem(&vd_blocks, i, &blk)) break;
		if (blk.stat) continue;
		if ((blk.type&BLK_HOLDER)) continue;	
		if ((blk.type&BLK_EXTERN)) num_extern_block++;
		else                       num_inter_block++;
	}
	if (!objects.hhead) {
		scene_min.x = scene_min.y = scene_min.z =  GAB_NO_DEF;
		scene_max.x = scene_max.y = scene_max.z = -GAB_NO_DEF;
	}

	code = SGLoadModel(name, &p_min, &p_max, matr, &objects);
	if (!code) put_message(MSG_SHU_025,name,0);
//nb 	code = TRUE;
	count = (short)objects.num - num_obj;
	if ( !count ) goto ex;
	dd = 100 * eps_d;
	if (fabs(p_min.x - p_max.x) < eps_d) {	p_min.x -= dd; p_max.x += dd;	}
	if (fabs(p_min.y - p_max.y) < eps_d) {	p_min.y -= dd; p_max.y += dd;	}
	if (fabs(p_min.z - p_max.z) < eps_d) {	p_min.z -= dd; p_max.z += dd;	}
	modify_limits(&p_min, &p_max);

	if ( htail ) get_next_item(htail, &htail);
	else {
		htail = objects.hhead;
    SetModifyFlag(FALSE);
	}
	hobj = htail;
	cur_ind=0;
	while (hobj) {
		fc_assign_curr_ident(hobj);
		//undo_to_creat(hobj);
		get_next_item(hobj, &hobj);
		cur_ind++;
	}
	
	/*if (draw && htail)
		vi_regen_list(&objects, get_regim_edge() | get_switch_display(),
												htail);*/
	gs_code = G_OK;
	goto ex;
//nb err:
//nb	gs_code = G_CANCEL;
ex:
	memset(tcount, 0, sizeof(tcount));
	strncpy(tcount, get_message(FROM_FILE)->str, sizeof(tcount) - 1);
	strncat(tcount, name, sizeof(tcount) - strlen(tcount) - 1);
	strncat(tcount, get_message(LOAD_OBJECTS)->str,
					sizeof(tcount) - strlen(tcount) - 1);
//	strncat(tcount, itoa(count, text, 10),
//					sizeof(tcount) - strlen(tcount) - 1);
	num_inter = num_extern = 0;
	for (i = 0; i < vd_blocks.num_elem; i++) {
		if (!read_elem(&vd_blocks, i, &blk)) break;
		if (blk.stat) continue;
		if ((blk.type&BLK_HOLDER)) continue;	
		if ((blk.type&BLK_EXTERN)) num_extern++;
		else                       num_inter++;
	}
	num_inter -= num_inter_block;
	num_extern -= num_extern_block;
	if (num_inter) {
		strncat(tcount, get_message(NUM_BLOCKS)->str,
						sizeof(tcount) - strlen(tcount) - 1);
		//strncat(tcount, itoa(num_inter, text, 10),
		//				sizeof(tcount) - strlen(tcount) - 1);
	}
	if (num_extern) {
		strncat(tcount, get_message(MSG_SHU_019)->str,
						sizeof(tcount) - strlen(tcount) - 1);
		//strncat(tcount, itoa(num_extern, text, 10),
		//				sizeof(tcount) - strlen(tcount) - 1);
	}
	put_message(EMPTY_MSG, tcount, 2);
	return gs_code;
}


GS_CODE append_from_file(char *name, BOOL property, MATR matr, BOOL draw)
{
	GS_CODE 	ret;
	hOBJ			hobj, hlast;
//	lpOBJ			obj;
	BOOL			/*hide,*/ code;
	LISTH			listh;
	MATR      m;
	int      num;
	D_POINT		p_min, p_max;
	int				i;
	IBLOCK    blk;

	//undo_to_load();
	hlast = objects.htail;

	if (!draw && static_data->FileLinkSaving) 
	{
		
		num = -1;
		for (i = 0; i < vd_blocks.num_elem; i++) 
		{
			if (!read_elem(&vd_blocks, i, &blk)) return G_CANCEL;
			if (blk.stat) continue;
			if ( !(blk.type&BLK_EXTERN) )
				continue;	
			//if (!strcmp(strlwr(name), strlwr(blk.name))) 
if (!strcmp(name, blk.name)) 
			{
				num = i;
				break;
			}
		}
		if (num == -1) 
		{
			
			init_listh(&listh);
			//blk_level++;
			code = SGLoadModel(name, &p_min, &p_max,
					 /*((property) ? pFileProp : NULL),*/ matr, &listh);
			//blk_level--;
			if (!code) {
				ret = G_CANCEL;
				goto met_ret;
			}
			num = -1;
			o_hcunit(m);
			if (!create_block(&listh, name, m, BLK_EXTERN, &num)) {
				while ((hobj = listh.hhead) != NULL) {
					o_free(hobj, &listh);
				}
				ret = G_CANCEL;
				goto met_ret;
			}
			//undo_to_block(num, m);
		}
		
		o_hcunit(m);
		hobj = create_insert_by_number(num, m);
		if (!hobj) ret = G_CANCEL;
		else {
			attach_item_tail(&objects, hobj);
			//undo_to_creat(hobj);
			ret = G_OK;
		}
	} else {
		
		ret = load_model(name, matr, draw);
	}
	if (ret != G_OK) goto met_ret;
	if (property) {
    ;
	}
	if (hlast) get_next_item(hlast, &hlast);
	else       hlast = objects.hhead;
	
	init_listh(&selected);


	return ret;
met_ret:
	//undo_creat_post_cancel();
	return ret;
}




//---
ODRAWSTATUS get_switch_display(void)
{
	ODRAWSTATUS st = 0;

	if (bool_dis[W_BACKFACE]) st |= ST_TD_BACKFACE;
	return st;
}

//---
ODRAWSTATUS get_regim_edge(void)
{
	ODRAWSTATUS st = 0;

	if (bool_vis[W_FULL]) st |= ST_TD_FULL;
	if (bool_vis[W_APPROCH]) st |= ST_TD_APPROCH;
	if (bool_vis[W_COND]) st |= ST_TD_COND;
	if (bool_vis[W_CONSTR]) st |= ST_TD_CONSTR;
	if (bool_vis[W_DUMMY]) st |= ST_TD_DUMMY;
	if (bool_vis[W_SKETCH]) st |= ST_TD_SKETCH;
	return st;
}

//---
BOOL get_draw_bpoint(void)
{
	return bool_dis[W_BPOINT];
}

