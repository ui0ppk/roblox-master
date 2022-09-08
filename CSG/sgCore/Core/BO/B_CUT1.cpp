#include "../sg.h"


//static lpNPW 		np_cut = NULL;

static OSCAN_COD cut_geo_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);
static BOOL  cut_pl(lpNPW np, lpD_PLANE pl, lpNPW np_cut);
static BOOL  cut_up(lpNPW np, lpNP_VERTEX_LIST ver, lpD_PLANE pl, lpNPW np_cut);
static BOOL  cut_zav(lpNPW np_cut);

typedef struct {
	D_PLANE 	pl;  									         			  
	lpNPW 		np_cut;
	lpLISTH listh_cut;
} CUT_DATA;
typedef CUT_DATA * lpCUT_DATA;

#define   EDIT_EDGE_LABLE      0x1000     

#pragma argsused
BOOL cut(lpLISTH list, NUM_LIST num_list, lpD_PLANE pl, lpLISTH listh_cut)
{
//  short           num_proc;
	OBJTYPE				type;
	SCAN_CONTROL  sc;
	OSCAN_COD     cod;
	REGION_3D  	  gab_obj = {{1.e35,1.e35,1.e35},{-1.e35,-1.e35,-1.e35}};
	CUT_DATA      cd={{{0.,0.,-1.},0}/*,{1.e35,1.e35,1.e35},{-1.e35,-1.e35,-1.e35}*/, NULL};
	hOBJ   			  hobj, hnext; //, hobj_c = NULL;
	lpOBJ					obj;
	ODRAWSTATUS	  dstatus;
	lpNPW 				np_cut;


//	VMM_Clear_Ram();
	init_listh(listh_cut);
	if ( (cd.np_cut = creat_np_mem(TNPW,MAXNOV*2,MAXNOV*2,10,10,MAXNOV*2))
			== NULL ) return FALSE;
	np_cut = cd.np_cut;
	cd.pl  = *pl;
	cd.listh_cut = listh_cut;

	init_scan(&sc);
	sc.data          = &cd;
	sc.user_geo_scan = cut_geo_scan;

	hobj = list->hhead;
	while (hobj != NULL) {
//		step_grad (num_proc , j++);
		get_next_item_z(num_list,hobj,&hnext);
		obj = (lpOBJ)hobj;
		dstatus = obj->drawstatus;
		type = obj->type;
		if (dstatus == ST_HIDE) goto met;       				 		
		if ( type != OBREP && type != OGROUP ) goto met;
		if ( !get_object_limits(hobj, &gab_obj.min, &gab_obj.max) ) goto err;
		define_eps(&gab_obj.min, &gab_obj.max);       	 
		if (np_gab_pl(&gab_obj, &cd.pl, eps_d)) goto met;  
		cod = o_scan(hobj,&sc);
		switch ( cod ) {
			case OSSKIP:
				break;
			case OSBREAK:
				cod = OSTRUE;
				goto br;
			case OSTRUE:
			//	tb_clear();
/*				if ( np_cut->nov ) { 						 					 
			//		tb_setcolor(IDXYELLOW);
					if ( !cut_zav(np_cut) ) goto err;	  
					dpoint_minmax(&np_cut->v[1],np_cut->nov,&(np_cut->gab.min),
												&(np_cut->gab.max));
					np_gab_un(&gab_cut, &(np_cut->gab), &gab_cut);

					
					init_listh(&listh_path);
					loop = 1;
					do {
						if((hobj_path = o_alloc(OPATH)) == NULL)	goto err;
						obj_path = (lpOBJ)hobj_path;
						geo_path = (lpGEO_PATH)obj_path->geo_data;
						o_hcunit(geo_path->matr);
						init_listh(&geo_path->listh);
						first_edge = edge = np_cut->c[loop].fe;
						vertex = BE(edge, np_cut);
						vn = np_cut->v[vertex];
						min = vn;
						max = vn;
						do {
							vertex = EE(edge, np_cut);
							vk = np_cut->v[vertex];
							modify_limits_by_point(&vk, &min, &max);
			//			if ( !tb_put_otr(vn,vk) ) goto err;
							if((hobj_line = o_alloc(OLINE)) == NULL) goto err;
							obj_line = (lpOBJ)hobj_line;
							obj_line->hhold = hobj_path;
							geo_line = (lpGEO_LINE)(obj_line->geo_data);
							geo_line->v1 = vn;
							geo_line->v2 = vk;

							attach_item_tail(&geo_path->listh, hobj_line);
							vn = vk;
							edge = SL(edge, np_cut);
						} while ( edge != first_edge && edge != 0);
						geo_path->min = min;
						geo_path->max = max;
						obj_path->status |= ST_FLAT;            
						if ( edge == first_edge ) obj_path->status |= ST_CLOSE;
						if ((cod1 = test_self_cross_path(hobj_path,NULL)) == OSFALSE) goto err;
						if (cod1 == OSTRUE) {
							obj_path = (lpOBJ)hobj_path;
							obj_path->status |= ST_SIMPLE;
						}
						attach_item_tail(&listh_path, hobj_path);
						loop = np_cut->c[loop].nc;
					} while (loop != 0);
					if ( listh_path.num > 0) {  								
						if ( (hobj_c = make_group(&listh_path)) == NULL ) goto err;
						attach_item_tail(listh_cut, hobj_c);
					}
			}
*/
		//	tb_clear();
				goto met;
			case OSFALSE:
			default:
				goto err;
		}
met:hobj = hnext;
	}
//	end_grad(num_proc, i);
//	num_proc = -1;

	cod = OSTRUE;
	goto end;

err:
	cod = OSFALSE;
br:

end:
//	tb_clear();
	np_free_stack();
	free_np_mem(&np_cut);
	return cod;
}
static OSCAN_COD cut_geo_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	register short 	i;
	lpGEO_BREP 		lpgeobrep;
	REGION_3D  		gab = {{1.e35,1.e35,1.e35},{-1.e35,-1.e35,-1.e35}};
	lpD_POINT  		v;//, vn, vk;
	lpNPW					np_cut;
	D_POINT				vn, vk, min, max;
	int						loop, edge, vertex, first_edge;
	OSCAN_COD     cod;
	hOBJ   			  hobj_c;//nb  = NULL;
	hOBJ					hobj_line, hobj_path;
	lpOBJ					obj, obj_line, obj_path;
	lpGEO_LINE  	geo_line;
	lpGEO_PATH  	geo_path;
	LISTH			 	  listh_path = {NULL,NULL,0};	

	if (ctrl_c_press) { 												
		put_message(CTRL_C_PRESS, NULL, 0);
		return OSBREAK;
	}
//  step_grad (num_proc , i);
	if ( lpsc->type != OBREP) return OSSKIP;
	if ( lpsc->breptype != BODY && lpsc->breptype != SURF) return OSSKIP;

	np_cut = ((lpCUT_DATA)(lpsc->data))->np_cut;
	if ( scan_flg_np_begin ) {        
//		tb_setcolor(IDXYELLOW);
//		breptype = lpsc->breptype;
		obj = (lpOBJ)hobj;
		lpgeobrep = (lpGEO_BREP)obj->geo_data;
		gab.min = lpgeobrep->min;
		gab.max = lpgeobrep->max;
	
		f3d_gabar_project1(lpgeobrep->matr,&gab.min,&gab.max,&gab.min,&gab.max);
		
		if (np_gab_pl(&gab, &((lpCUT_DATA)(lpsc->data))->pl, eps_d)) return OSSKIP;  
		np_init(np_cut);
	}
	npwg->type = TNPW;
	v = npwg->v;
	for (i = 1; i <= npwg->nov ; i++) {       
		if ( !o_hcncrd(lpsc->m,&v[i],&v[i])) return OSFALSE;
	}
	dpoint_minmax(&npwg->v[1], npwg->nov, &npwg->gab.min,&npwg->gab.max);
	if ( !np_gab_pl(&npwg->gab, &((lpCUT_DATA)lpsc->data)->pl, eps_d)) {
							
		if ( !np_cplane(npwg) ) return OSFALSE;   
		if ( cut_pl(npwg, &((lpCUT_DATA)lpsc->data)->pl,
								np_cut) == FALSE ) return OSFALSE;
	}

	if ( scan_flg_np_end ) {        
		if ( np_cut->nov ) { 						 					
			if ( !cut_zav(np_cut) ) return OSFALSE;	  

			
			init_listh(&listh_path);
			loop = 1;
			do {
			 if((hobj_path = o_alloc(OPATH)) == NULL)	return OSFALSE;
			 obj_path = (lpOBJ)hobj_path;
			 geo_path = (lpGEO_PATH)obj_path->geo_data;
			 o_hcunit(geo_path->matr);
			 init_listh(&geo_path->listh);
			 first_edge = edge = np_cut->c[loop].fe;
			 vertex = BE(edge, np_cut);
			 vn = np_cut->v[vertex];
			 min = vn;
			 max = vn;
			 do {
				 vertex = EE(edge, np_cut);
				 vk = np_cut->v[vertex];
				 if (!dpoint_eq(&vn, &vk, eps_d)) {
					 modify_limits_by_point(&vk, &min, &max);
			//			if ( !tb_put_otr(vn,vk) ) goto err;
					 if((hobj_line = o_alloc(OLINE)) == NULL) return OSFALSE;
					 obj_line = (lpOBJ)hobj_line;
					 obj_line->hhold = hobj_path;
					 geo_line = (lpGEO_LINE)(obj_line->geo_data);
					 geo_line->v1 = vn;
					 geo_line->v2 = vk;
			
					 attach_item_tail(&geo_path->listh, hobj_line);
					 vn = vk;
				 }
				 edge = SL(edge, np_cut);
			 } while ( edge != first_edge && edge != 0);
			 if (geo_path->listh.num == 0) {
				 o_free(hobj_path, NULL);
			 } else {
				 geo_path->min = min;
				 geo_path->max = max;
				 obj_path->status |= ST_FLAT;            
				 if ( edge == first_edge ) obj_path->status |= ST_CLOSE; 
				 if ((cod = test_self_cross_path(hobj_path,NULL)) == OSFALSE) return OSFALSE;
				 if (cod == OSTRUE) {
					 obj_path = (lpOBJ)hobj_path;
					 obj_path->status |= ST_SIMPLE;
				 }
				 attach_item_tail(&listh_path, hobj_path);
			 }
			 loop = np_cut->c[loop].nc;
			} while (loop != 0);
			if ( listh_path.num > 0) {  								
			 if ( (hobj_c = make_group(&listh_path, TRUE)) == NULL ) return OSFALSE;
			 attach_item_tail(((lpCUT_DATA)(lpsc->data))->listh_cut, hobj_c);
			}
			//	tb_clear();
		}
  }
	return OSTRUE;
}
static BOOL  cut_pl(lpNPW np, lpD_PLANE pl, lpNPW np_cut)
{
	short            face;
	NP_VERTEX_LIST ver = {0,0,NULL,NULL};
	BOOL           rt;//nb  = FALSE;

	if ( !(rt = np_ver_alloc(&ver)) ) goto end;

	for (face = 1 ; face <= np->nof ; face++) {
		if (fabs(np->p[face].v.x - pl->v.x) <= eps_n &&
			fabs(np->p[face].v.y - pl->v.y) <= eps_n &&
			fabs(np->p[face].v.z - pl->v.z) <= eps_n ||
			fabs(np->p[face].v.x + pl->v.x) <= eps_n &&
			fabs(np->p[face].v.y + pl->v.y) <= eps_n &&
			fabs(np->p[face].v.z + pl->v.z) <= eps_n ) continue;
		ver.uk = 0;
		np->nov_tmp = np->maxnov;
		if ( !(rt=np_pl(np, face, &ver, pl)) ) goto end;
		if (ver.uk == 0) continue;     										 
		np_axis = np_defc( &np->p[face].v, &pl->v);
		np_sort_ver(np->v,&ver,np_axis);
		if ( !(rt=cut_up(np, &ver, pl, np_cut)) ) goto end;  
	}

	rt = TRUE;
end:
	np_free_ver(&ver);
	return rt;
}
/************************************************
 *            --- BOOL cut_up ---
 * 					
 ************************************************/
static BOOL  cut_up(lpNPW np, lpNP_VERTEX_LIST ver, lpD_PLANE pl, lpNPW np_cut)
{
	short        p = -1, pr = -1, pt = -1;
	short        p1, p2, face1, face2;
	short        ukt = 0;
	short        w1, w2;
	short        edge, r;
	float      gru;
	D_POINT    vvn, vvk;
	lpD_POINT  vn, vk;
	BOOL       le = FALSE;                     
																		
	while (ukt < ver->uk) {
		np_av(ver->v[ukt].m,&p,&pr,&pt,&le);
		ukt++;
		if (p == 1) {															
			r = b_vran(np, ver, ukt);					 
			if ( le ) {                                		
				face1 = EF( r,np);
				face2 = EF(-r,np);
				if (face2 < 0)   face2 = np_calc_gru(np, (short)(abs(r)));
				p1 = b_pv(np,         r,  pl);
				p2 = b_pv(np,(short)(-r), pl);
				if (p1 == p2)  continue;               	 
				if (p1 == 0 || p2 == 0) {                     
                    if ( face2 == 0) gru = np->gru[(int)abs(r)];
					else
						if (face1 > 0 && face2 > 0)
							if ( (gru=np_gru(np,r,&np->p[face1].v,&np->p[face2].v)) == 0 )
								return FALSE;


					if (gru > M_PI) continue;
				}
			}
			
			face1 = EF(r,np);
			vvn = np->v[ver->v[ukt-1].v];
			vvk = np->v[ver->v[ukt].v];
			vn = &vvn;        vk = &vvk;
			if ( !b_ort(&vn,&vk,&(np->p[face1].v),&pl->v) ) return FALSE;
			if ( (w1 = np_new_vertex(np_cut, vn)) == 0) return FALSE;
			if ( (w2 = np_new_vertex(np_cut, vk)) == 0) return FALSE;
			if ( w1 != w2 ) {
				if ( !(edge=np_new_edge(np_cut,w1,w2)) ) return FALSE;
				if (edge < 0) continue;
				np_cut->efc[edge].fp = 1;
      }
		}
	}
	return TRUE;
}
 
static BOOL  cut_zav(lpNPW np_cut)
{
	short  i, ii;
	short  loop1, loop2;
	short  first_edge, edge;
	short  v;
	BOOL rt = TRUE;

	ii = 0;                      					  
	loop2 = 0;                     					
	while ( 1 ) {
		for (i=1 ; i <= np_cut->noe ; i++) {   
//			if (np_cut->efc[i].ep == 0) break;
			if (np_cut->efr[i].el & EDIT_EDGE_LABLE) continue;
			break;
		}
		if (i > np_cut->noe) break;                    
		ii++;
		if ( !(loop1=np_new_loop(np_cut, i)) ) return FALSE; 
		if ( loop2 != 0 ) np_cut->c[loop2].nc = loop1;   	 
		loop2 = loop1;

		first_edge = edge = i;
		np_cut->efr[edge].el |= EDIT_EDGE_LABLE;
		while ( 1 ) {                    						
			v = np_cut->efr[edge].ev;
			for (i=1 ; i <= np_cut->noe ; i++) {
				if (np_cut->efr[i].bv == v) {
					if ( np_cut->efc[i].ep ) continue;  				 
					np_cut->efc[edge].ep = i;
					np_cut->efr[i].el |= EDIT_EDGE_LABLE;
					edge = i;
					ii++;
					break;
				}
			}
			if (i > np_cut->noe) {
//				last_edge = edge;
				edge = first_edge;
				while ( 1 ) {                    			
					v = np_cut->efr[edge].bv;
					for (i=1 ; i <= np_cut->noe ; i++) {
						if (np_cut->efr[i].ev == v) {
							if (np_cut->efr[i].el & EDIT_EDGE_LABLE) continue;
							np_cut->efc[i].ep = edge;
							edge = i;
							np_cut->efr[i].el |= EDIT_EDGE_LABLE;
							ii++;
							break;
						}
					}
					if (i > np_cut->noe) {  									
						np_cut->c[loop1].fe = edge;
						goto met;
					}
				}
			}

			if (np_cut->efr[edge].ev == np_cut->efr[first_edge].bv) {
				np_cut->efc[edge].ep = first_edge;
				break;         															
			}
		}
met: if (ii >= np_cut->noe) break;
	}
	for (i=1; i<=np_cut->noe; i++) {
		if ( !(np_cut->efr[i].el & EDIT_EDGE_LABLE) ) continue;
		np_cut->efr[i].el &= (~EDIT_EDGE_LABLE);
	}
	return rt;
}
/*
void cut_ort_loop(lpNPW np)
{
	short 		loop;	//, lc;
	int			to, from, nxt;
	sgFloat 	sq;

//	lc = 0;                             
	loop = np->f[1].fc;
	do {                                       
		np_ort_loop(np,1,loop,&sq);
		if (sq < 0) {      											
			to = np->c[loop].fe;
			from = SL(to,np);
			do {
				if (from > 0) {
					nxt = np->efc[abs(from)].ep;
					np->efc[abs(from)].em = -to;
				} else {
					nxt = np->efc[abs(from)].em;
					np->efc[abs(from)].ep = -to;
				}
				to   = from;
				from = nxt;
			} while (to != np->c[loop].fe);
			np->c[loop].fe = - np->c[loop].fe;
		}
//		lc = loop;
		loop = np->c[loop].nc;
	} while (loop != 0);
}
*/
