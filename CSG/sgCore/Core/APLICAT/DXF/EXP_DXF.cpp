#include "../../sg.h"

short dxf_convert_color(short color);

static OSCAN_COD dxfout_geo_scan(hOBJ hobj, lpSCAN_CONTROL lpsc);
static OSCAN_COD dxfout_pre_scan(hOBJ hobj, lpSCAN_CONTROL lpsc);
static OSCAN_COD dxfout_post_scan(hOBJ hobj, lpSCAN_CONTROL lpsc);
static OSCAN_COD dxfout_pre_scan_brep(hOBJ hobj,   lpSCAN_CONTROL lpsc);
static OSCAN_COD dxfout_pre_scan_insert(hOBJ hobj, lpSCAN_CONTROL lpsc);
static OSCAN_COD tnpf_dxf_geo_scan(hOBJ hobj, lpSCAN_CONTROL lpsc);
static BOOL dxfout_pre_np(lpNPW npw, short num_np);
static BOOL dxfout_post_np(BOOL ret, lpNPW npw, short num_np);
static BOOL dxfout_put_face10(lpNPW npw, short num_f, short lv, short *v, short *e);
static BOOL dxfout_put_face12(lpNPW npw, short num_f, short lv, short *v, short *e);
static BOOL dxfout_pre_face(lpNPW npw, short num_np, short numf);
static BOOL dxfout_post_face(BOOL ret, lpNPW npw, short num_np, short numf);
static BOOL dxf_export_tables(void);

static BOOL put_dxf_obj_header(DXF_KEYS key, short color, short layer);
static void dxf_ins_par(lpMATR m_lg, BOOL orient, lpD_POINT vz, lpD_POINT insp,
												lpD_POINT scale, sgFloat *angle);

static OSCAN_COD  (**dxfout_type_g)(void * geo, lpSCAN_CONTROL lpsc);

static BOOL dxfout_header(void);
static bool export_to_dxf_objects_list(lpLISTH listh, NUM_LIST num_list);
static BOOL create_dxf_blocks_map(lpLISTH listh, NUM_LIST list_zudina,
					lpVDIM vdim, short *num);
static void sg_to_dxf_name(char *sg_2_name, char *dxfname);
static void auto_dxf_name(char *name);
static BOOL compare_names(void * elem, void * user_data);
static BOOL dxf_export_blocks(short num);
static BOOL dxf_export_obj(hOBJ hobj);
static BOOL dxfout_sply_point(lpD_POINT p, BOOL knot, lpMATR m,
															short flag, short flag3D);


#define MAXDXFBLOCKNAME  31  

bool dxf_full_flag    = true;
bool dxf_flat_flag    = false;
bool dxf_blocks_flag  = true;
bool dxf_binary_flag  = false;
short  dxf_acad_version = 10;
lpVDIM vd_dxf_layers = NULL;

char dxf_binary_tytle[] = {"AutoCAD Binary DXF\r\n\032"};
char dxf_auto_name[] = {"SG_BLOCK"};
static short dxf_auto_name_count = 0;

typedef struct {
	short  count;             
	char name[MAXBLOCKNAME]; 
}DXF_BLOCKS_MAP;
typedef DXF_BLOCKS_MAP * lpDXF_BLOCKS_MAP;

typedef struct {
	short      num_np;
	short      max_v;
	BUFFER_DAT bd;
	VDIM       v_map;   
	short      globalnv;
	short      color;
	short      layer;
	int  	     offset;
	BOOL       trian_error;
	int			 	 path;			
	BOOL			 flat;			
	D_POINT		 normal;		
	MATR			 m_gcs_ocs;	
	D_POINT		 last_v;		
	BOOL			 close;			
}DXF_INFO;
typedef DXF_INFO * lpDXF_INFO;

static lpDXF_INFO dxf_info;

bool export_3d_to_dxf(char *name)
{
DXF_INFO di;
bool     ret = false;

	memset(&di, 0, sizeof(DXF_INFO));
	di.trian_error = FALSE;
	di.layer = -1;
	dxf_info = &di;

	if(!init_buf(&dxf_info->bd, name, BUF_NEW)) return false;

	if(dxfout_header()){
		if(dxf_export_tables()){
			ret = (dxf_full_flag) ?
			export_to_dxf_objects_list(&objects,  OBJ_LIST) :
			export_to_dxf_objects_list(&selected, SEL_LIST) ;
		}
	}
	close_buf(&dxf_info->bd);
	if(!ret) nb_unlink(name);
	if(ret && di.trian_error)
		put_message(EMPTY_MSG, GetIDS(IDS_SG001), 0);
	return ret;
}

static BOOL dxfout_header(void)
{
BOOL  ret = FALSE;
	
	const char ttt[] = "SolidGraph export";
  if(dxf_binary_flag) 
    if(!story_data(&dxf_info->bd, strlen(dxf_binary_tytle) + 1,
                    dxf_binary_tytle)) goto end;

	if(!put_dxf_group(&dxf_info->bd, 999, (void*)ttt)) goto end;
	if(dxf_full_flag){} 
	else if(vd_dxf_layers){
		if(vd_dxf_layers->num_elem){
			if(!put_dxf_group(&dxf_info->bd, 0, dxf_keys[DXF_SECTION])) goto end;
			if(!put_dxf_group(&dxf_info->bd, 2, dxf_keys[DXF_HEADER])) goto end;
			if(!put_dxf_group(&dxf_info->bd, 0, dxf_keys[DXF_ENDSEC])) goto end;
		}
	}

	ret = TRUE;
end:
	return ret;
}

static bool export_to_dxf_objects_list(lpLISTH listh, NUM_LIST num_list)
{
short   num, n = 0;
hOBJ  hobj;
bool  ret = false, errflag = false;

	if(!create_dxf_blocks_map(listh, num_list, &dxf_info->v_map, &num)) goto end;
	if(!dxf_export_blocks(num)) goto end;


	if(!put_dxf_group(&dxf_info->bd, 0, dxf_keys[DXF_SECTION])) goto end;
	if(!put_dxf_group(&dxf_info->bd, 2, dxf_keys[DXF_ENTITIES])) goto end;
//  num_proc = start_grad(dxf_keys[DXF_ENTITIES], listh->num);
	if (application_interface && application_interface->GetProgresser())
		application_interface->GetProgresser()->InitProgresser(1);
	hobj = listh->hhead;
	while (hobj) {
    if(num_list == SEL_LIST) {
      unmark_obj(hobj);
//      change_viobj_color(hobj, COBJ);
      //draw_obj(hobj, CDEFAULT);
    }
		if(!errflag){
      if(!dxf_export_obj(hobj)) {
        if(num_list != SEL_LIST) goto end;
        errflag = true;
      }
//			step_grad (num_proc , ++n);
	  if (application_interface && application_interface->GetProgresser() && listh->num>0) 
		  application_interface->GetProgresser()->Progress(100*(++n)/listh->num);
    }
		get_next_item_z(num_list, hobj, &hobj);
	}
  if(errflag) goto end;
	if(!put_dxf_group(&dxf_info->bd, 0, dxf_keys[DXF_ENDSEC])) goto end;
	if(!put_dxf_group(&dxf_info->bd, 0, dxf_keys[DXF_EOF])) goto end;
	ret = true;
end:
//  end_grad(num_proc, listh->num);
	if (application_interface && application_interface->GetProgresser())
		application_interface->GetProgresser()->StopProgresser();;
	free_vdim(&dxf_info->v_map);
	return ret;
}

static BOOL create_dxf_blocks_map(lpLISTH listh, NUM_LIST list_zudina,
																	lpVDIM vdim, short *num)
{
int             i, j;
DXF_BLOCKS_MAP   map;
IBLOCK           blk;

	init_vdim(vdim, sizeof(DXF_BLOCKS_MAP));
	memset(&map, 0, sizeof(map));
 	if(!create_count_blocks(listh, list_zudina, vdim, &map)) goto err;
 	dxf_auto_name_count = 0;
	*num = 0;
	for(i = 0; i < vd_blocks.num_elem; i++) {
		if(!read_elem(vdim,       i, &map)) goto err;
		if(!read_elem(&vd_blocks, i, &blk)) goto err;
		if(map.count <= 0) continue;
		(*num)++;
		sg_to_dxf_name(blk.name, map.name);
		j = 0;
		while(find_elem(vdim, &j, compare_names, map.name)){
			if(j >= i) break;
			auto_dxf_name(map.name);
			j = 0;
		}
		if(!write_elem(vdim, i, &map)) goto err;
	}
	return TRUE;
err:
	free_vdim(vdim);
	return FALSE;
}

static void sg_to_dxf_name(char *sg_2_name, char *dxfname){

static char rus_lat[]= "ABVGDEGZIIKLMNOPRSTUFHCTSSI__EUA";
char *d, *c = dxfname;

	strcpy(dxfname, sg_2_name);
	if(strlen(dxfname) > MAXDXFBLOCKNAME) dxfname[MAXDXFBLOCKNAME] = 0;
	while(*c){
		if((d = strchr(rus_up, *c)) != NULL)
			*c = rus_lat[(short)(d - rus_up)];
		else if((d = strchr(rus_dn, *c)) != NULL)
			*c = rus_lat[(short)(d - rus_dn)];
		else if(!isascii(*c)) *c = '$';
		else if(isalpha(*c)){
			if(islower(*c)) *c = (BYTE)_toupper(*c);
		}
		else if((!isdigit(*c)) && (!strchr("$-_", *c))) *c = '_';
		c++;
	}
}

static void auto_dxf_name(char *name){
char buf[10];

	strcpy(name, dxf_auto_name);

	/*itoa(*/dxf_auto_name_count++/*, buf, 10)*/;
	strcat(name, buf);
}

static BOOL compare_names(void * elem, void * user_data){

lpDXF_BLOCKS_MAP map;

	map = (lpDXF_BLOCKS_MAP)elem;
	if(map->count <= 0) return FALSE;
	if(!map->name[0])   return TRUE;
	return (strcmp(map->name, (char*)user_data)) ? FALSE : TRUE;
}

static BOOL  dxf_export_blocks(short num)

{
int           i;
IBLOCK         blk;
DXF_BLOCKS_MAP map;
hOBJ           hobj;
BOOL           ret = FALSE;
short            n = 0, bflag = 64;
char           buf[80];
D_POINT        wp = {0., 0., 0.};

	if(!dxf_full_flag && !dxf_blocks_flag) return TRUE;
	if(!num) return TRUE;

//	num_proc = start_grad(dxf_keys[DXF_BLOCKS], num);
	if(!put_dxf_group(&dxf_info->bd, 0, dxf_keys[DXF_SECTION])) goto end;
	if(!put_dxf_group(&dxf_info->bd, 2, dxf_keys[DXF_BLOCKS])) goto end;

	for(i = 0; i < vd_blocks.num_elem; i++){
		if(!read_elem(&dxf_info->v_map, i, &map)) goto end;
		if(map.count <= 0) continue;
		if(!read_elem(&vd_blocks, i, &blk)) goto end;

		if(!put_dxf_obj_header(DXF_BLOCK, -1, -1)) goto end;
		if(!put_dxf_group(&dxf_info->bd, 2, map.name)) goto end;
		if(strcmp(map.name, blk.name)){
			strcpy(buf, GetIDS(IDS_SG003));
			strcat(buf, blk.name);
			strcat(buf, "\"");
			if(!put_dxf_group(&dxf_info->bd, 999, buf)) goto end;
		}
		if(!put_vertex_to_dxf(&dxf_info->bd, 10, &wp)) goto end;
		if(!put_dxf_group(&dxf_info->bd, 70, &bflag)) goto end;

		hobj = blk.listh.hhead;
		while (hobj) {
			if(!dxf_export_obj(hobj)) goto end;
			get_next_item(hobj, &hobj);
		}
//		step_grad (num_proc , ++n);
    if(!put_dxf_group(&dxf_info->bd, 0, dxf_keys[DXF_ENDBLK])) goto end;
	}
  if(!put_dxf_group(&dxf_info->bd, 0, dxf_keys[DXF_ENDSEC])) goto end;
	ret = TRUE;
end:
//	end_grad(num_proc , num);
	return ret;
}

static BOOL dxf_export_obj(hOBJ hobj)
{
OSCAN_COD    	code;
SCAN_CONTROL 	sc;
MATR					matr;


	dxfout_type_g = (OSCAN_COD(**)(void * geo, lpSCAN_CONTROL lpsc))GetMethodArray(OMT_DXFOUT);
	init_scan(&sc);
	sc.m = matr;
	matr_gcs_to_default_cs(matr);	
	sc.user_pre_scan  = dxfout_pre_scan;
	sc.user_geo_scan  = dxfout_geo_scan;
	sc.user_post_scan = dxfout_post_scan;
	sc.data = dxf_info;
	if(dxf_acad_version > 10){
		trian_put_face    = dxfout_put_face12;
		trian_pre_np      = dxfout_pre_np;
		trian_post_np     = dxfout_post_np;
	}
	else
		trian_put_face  = dxfout_put_face10;

	trian_pre_face    = dxfout_pre_face;
	trian_post_face   = dxfout_post_face;

	code = o_scan(hobj, &sc);
	trian_put_face  = NULL;
	trian_pre_np    = NULL;
  trian_post_np   = NULL;
  trian_pre_face  = NULL;
  trian_post_face = NULL;
  return (code == OSFALSE) ? FALSE : TRUE;
}

#pragma argsused
static OSCAN_COD dxfout_pre_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	int	    n;
	short   color;
	D_POINT	point;
	D_PLANE	plane;
	OSTATUS	status;
	MATR		matr;

	switch(lpsc->type){
		default:
			break;
		case OINSERT:
			return dxfout_pre_scan_insert(hobj, lpsc);
		case OBREP:
			return dxfout_pre_scan_brep(hobj, lpsc);
		case OPATH:
			dxf_info->path++;
			if (dxf_info->path == 1) {
				if (!get_status_path(hobj, &status)) return OSFALSE;
				if (lpsc->color == CTRANSP) color = curr_color;
				else                        color = lpsc->color;
				if(!put_dxf_obj_header(DXF_POLYLINE, color, lpsc->layer)) return OSFALSE;
				n = 1;
				if(!put_dxf_group(&dxf_info->bd, 66, &n)) return OSFALSE;
				n = 0;
				if (status & ST_CLOSE) {
					n++;
					dxf_info->close = TRUE;
				} else {
					dxf_info->close = FALSE;
				}
				if ((status & ST_FLAT)) {
					if (!set_flat_on_path(hobj, &plane))
						n += 8;
				} else
					n += 8;
				if (n != 0)
					if(!put_dxf_group(&dxf_info->bd, 70, &n)) return OSFALSE;
				memset(&point, 0, sizeof(D_POINT));
				if (n & 8) {	
					dxf_info->flat = FALSE;
					if(!put_vertex_to_dxf(&dxf_info->bd, 10, &point)) return OSFALSE;
				} else {	
					if (lpsc->m) {
						memset(&point, 0, sizeof(D_POINT));
						if (!o_trans_vector(lpsc->m, &point, &plane.v)) return OSFALSE;
						if (!dnormal_vector(&plane.v)) return OSFALSE;
					}
					point.x = point.y = 0; point.z = -1;
					if (dxf_flat_flag && dpoint_eq(&plane.v, &point, eps_d)) {
						dpoint_inv(&plane.v, &plane.v);
					}
					dxf_info->flat = TRUE;
					dxf_info->normal = plane.v;
					ocs_gcs(&plane.v, matr, dxf_info->m_gcs_ocs);
					get_endpoint_on_path(hobj, &point, (OSTATUS)0);
					o_hcncrd(dxf_info->m_gcs_ocs, &point, &point);
					point.x = point.y = 0;
					if(!put_vertex_to_dxf(&dxf_info->bd, 10, &point)) return OSFALSE;
					point.z = 1;
					if (!dpoint_eq(&point, &plane.v, eps_n)) {
						if(!put_vertex_to_dxf(&dxf_info->bd, 210, &plane.v)) return OSFALSE;
					}
				}
			}
			break;
	}
	return OSTRUE;
}

#pragma argsused
static OSCAN_COD dxfout_post_scan(hOBJ hobj, lpSCAN_CONTROL lpsc){
	int	n;

	switch (lpsc->type) {
		default:
			break;
		case OBREP:
			if	(dxf_acad_version > 10)	{
				if(!put_dxf_group(&dxf_info->bd, 0, dxf_keys[DXF_SEQEND]))
					return OSFALSE;
			}
			break;
		case OPATH:
			dxf_info->path--;
			if (dxf_info->path == 0) {
				if (!dxf_info->close) {
					if(!put_dxf_obj_header(DXF_VERTEX, lpsc->color, lpsc->layer)) return OSFALSE;
					if(!put_vertex_to_dxf(&dxf_info->bd, 10, &dxf_info->last_v))
						return OSFALSE;
					if (!dxf_info->flat) { 
						n = 32;
						if(!put_dxf_group(&dxf_info->bd, 70, &n)) return OSFALSE;
					}
				}
				if(!put_dxf_group(&dxf_info->bd, 0, dxf_keys[DXF_SEQEND]))
					return OSFALSE;
			}
			break;
	}
	return OSTRUE;
}

#pragma argsused
static OSCAN_COD dxfout_pre_scan_brep(hOBJ hobj, lpSCAN_CONTROL lpsc)
{

	dxf_info->num_np = 0;
	dxf_info->max_v  = 4;
	dxf_info->globalnv = 0;
	dxf_info->color = lpsc->color;
	dxf_info->layer = lpsc->layer;
	if(dxf_acad_version > 10){ 
		short n = 64;
		if(!put_dxf_obj_header(DXF_POLYLINE, lpsc->color, lpsc->layer)) return OSFALSE;
		if(!put_dxf_group(&dxf_info->bd, 70, &n)) return OSFALSE;
		n = 1;
		if(!put_dxf_group(&dxf_info->bd, 66, &n)) return OSFALSE;
  }
	return OSTRUE;
}

static BOOL put_dxf_obj_header(DXF_KEYS key, short color, short layer){
char* lname;
char  deflname[] = {"0"};
LAYER lr;

	lname = deflname;

	if(!put_dxf_group(&dxf_info->bd, 0, dxf_keys[key])) return FALSE;

	if(layer < 0)
		goto defl;
	if(!vd_dxf_layers)
		goto defl;
	if(vd_dxf_layers->num_elem <= layer)
		goto defl;
	if(!read_elem(vd_dxf_layers, layer, &lr)) return FALSE;
	lname = lr.name;
defl:
	if(!put_dxf_group(&dxf_info->bd, 8, lname))  return FALSE;
	if(color >= 0){
		color = dxf_convert_color(color);
		if(!put_dxf_group(&dxf_info->bd, 62, &color)) return FALSE;
	}
  return TRUE;
}

#pragma argsused
static BOOL dxfout_pre_np(lpNPW npw, short num_np){
short i, n = 128+64;
	if(dxf_acad_version > 10){
		for(i = 1; i <= npw->nov ; i++){
			if(!put_dxf_obj_header(DXF_VERTEX, -1, dxf_info->layer))       return FALSE;
			if(!put_dxf_group(&dxf_info->bd, 70, &n)) return FALSE;
			if(!put_vertex_to_dxf(&dxf_info->bd, 10, &npw->v[i])) return FALSE;
		}
  }
  return TRUE;
}

#pragma argsused
static BOOL dxfout_post_np(BOOL ret, lpNPW npw, short num_np){
	if(!ret) return FALSE;
	dxf_info->globalnv += npw->nov;
	return TRUE;
}
#pragma argsused
static BOOL dxfout_pre_face(lpNPW npw, short num_np, short numf){
  dxf_info->offset = get_buf_offset(&dxf_info->bd);
  return TRUE;
}

#pragma argsused
static BOOL dxfout_post_face(BOOL ret, lpNPW npw, short num_np, short numf){
  if(ret) return TRUE;
  if(!trian_error) return FALSE; 
  if(!seek_buf(&dxf_info->bd, dxf_info->offset)) return FALSE;
  trian_error = FALSE;
  dxf_info->trian_error = TRUE;
  return TRUE;
}

#pragma argsused
static BOOL dxfout_put_face10(lpNPW npw, short num_f, short lv, short *v, short *e)
{
short i, n = 0;

	if(!put_dxf_obj_header(DXF_3DFACE, dxf_info->color, dxf_info->layer)) return FALSE;

  for(i = 0; i < lv; i++){
    if(e[i])
      if(!(npw->efr[e[i]].el & ST_TD_DUMMY)) continue;
    n += 1 << i;
  }
  if(lv == 3){
    n |= 4;
    if     (!e[2])                             n |= 8;
		else if((npw->efr[e[2]].el & ST_TD_DUMMY)) n |= 8;
  }
	if(!put_dxf_group(&dxf_info->bd, 70, &n))  return FALSE;
  for(i = 0; i < lv; i++)
		if(!put_vertex_to_dxf(&dxf_info->bd, i + 10, &npw->v[v[i]]))  return FALSE;
  if(lv == 3)
    if(!put_vertex_to_dxf(&dxf_info->bd, 13, &npw->v[v[2]]))  return FALSE;
	return TRUE;
}

#pragma argsused
static BOOL dxfout_put_face12(lpNPW npw, short num_f, short lv, short *v, short *e)
{
short     i, nv, n = 128;
D_POINT p = {0., 0., 0.};

	if(!put_dxf_obj_header(DXF_VERTEX, -1, dxf_info->layer))   return FALSE;
  if(!put_dxf_group(&dxf_info->bd, 70, &n))                  return FALSE;
  if(!put_vertex_to_dxf(&dxf_info->bd, 10, &p))              return FALSE;
  for(i = 0; i < lv; i++){
    nv = dxf_info->globalnv + v[i];
    if(e[i]){
      if(npw->efr[e[i]].el & ST_TD_DUMMY) nv = -nv;
    }
    else nv = -nv;
    if(!put_dxf_group(&dxf_info->bd, 71 + i, &nv))           return FALSE;
	}
	return TRUE;
}

#pragma argsused
static OSCAN_COD dxfout_pre_scan_insert(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
lpOBJ           obj;
DXF_BLOCKS_MAP  map;
int            num;
MATR            matr;
BOOL            invert;
D_POINT         vz, insp, scale;
sgFloat          angle;
short             color = lpsc->color;

	obj = (lpOBJ)hobj;
	num = ((lpGEO_INSERT)obj->geo_data)->num;
	memcpy(matr,((lpGEO_INSERT)obj->geo_data)->matr,sizeof(matr));
	invert = ((obj->status&ST_ORIENT) != 0);

	if(!read_elem(&dxf_info->v_map, num, &map))    return OSFALSE;
  if(color == CTRANSP || color == CTRANSPBLK) color = -1;
  if(!put_dxf_obj_header(DXF_INSERT, color, lpsc->layer))     return OSFALSE;
	if(!put_dxf_group(&dxf_info->bd, 2, map.name)) return OSFALSE;
	dxf_ins_par(matr, invert, &vz, &insp, &scale, &angle);
	if(!put_vertex_to_dxf(&dxf_info->bd, 10, &insp)) return OSFALSE;
	if(!put_dxf_group(&dxf_info->bd, 41, &scale.x))  return OSFALSE;
	if(!put_dxf_group(&dxf_info->bd, 42, &scale.y))  return OSFALSE;
	if(!put_dxf_group(&dxf_info->bd, 43, &scale.z))  return OSFALSE;
	if(!put_dxf_group(&dxf_info->bd, 50, &angle))    return OSFALSE;
	if(!put_vertex_to_dxf(&dxf_info->bd, 210, &vz))  return OSFALSE;
	return OSSKIP;
}

#pragma argsused
static void dxf_ins_par(lpMATR m_lg, BOOL orient, lpD_POINT vz, lpD_POINT insp,
												lpD_POINT scale, sgFloat *angle)

{
D_POINT l0 = {0., 0., 0.}, lz = {0., 0., 1.}, lx = {1., 0., 0.}, w0, w1;
MATR m_lo, m_go;


	o_hcncrd(m_lg, &l0, &w0);
	o_hcncrd(m_lg, &lz, &w1);
	dpoint_sub(&w1, &w0, vz);
	dnormal_vector(vz);   
	ocs_gcs(vz, m_lo, m_go);
	memcpy(m_lo, m_lg, sizeof(MATR));
	o_hcmult(m_lo, m_go);  

	o_hcncrd(m_lo, &l0, insp); 
	o_hcncrd(m_lo, &lx, &w1);
	dpoint_sub(&w1, insp, &w0);
	scale->x = scale->y = scale->z = dpoint_distance(insp, &w1);
	*angle = (v_angle(w0.x, w0.y)*180)/M_PI;
}


static OSCAN_COD dxfout_geo_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
lpOBJ      obj;
OSCAN_COD  ret;

	if(lpsc->type == OBREP){
		if( npwg->type != TNPF) return trian_geo_scan(hobj, lpsc);
    else                    return tnpf_dxf_geo_scan(hobj, lpsc);
	}

	obj = (lpOBJ)hobj;
	ret = dxfout_type_g[lpsc->type](obj->geo_data, lpsc);
	return ret;
}

#pragma argsused
static OSCAN_COD tnpf_dxf_geo_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
short i, bv, ev, n = 128;
D_POINT p = {0., 0., 0.};

	for(i = 1; i <= npwg->nov ; i++)     
		o_hcncrd(lpsc->m, &npwg->v[i], &npwg->v[i]);

  if(dxf_acad_version > 10){
    if(!dxfout_pre_np(npwg, 0))                     return OSFALSE;
    for(i = 1; i <= npwg->noe ; i++){
      bv = abs(npwg->efr[i].bv) + dxf_info->globalnv;
      ev = abs(npwg->efr[i].ev) + dxf_info->globalnv;
      if(npwg->efr[i].el & ST_TD_DUMMY) continue;
      if(!put_dxf_obj_header(DXF_VERTEX, -1, lpsc->layer))       return OSFALSE;
      if(!put_dxf_group(&dxf_info->bd, 70, &n))     return OSFALSE;
      if(!put_vertex_to_dxf(&dxf_info->bd, 10, &p)) return OSFALSE;
      if(!put_dxf_group(&dxf_info->bd, 71 , &bv))   return OSFALSE;
      if(!put_dxf_group(&dxf_info->bd, 72 , &ev))   return OSFALSE;
    }
    if(!dxfout_post_np(TRUE, npwg, 0))              return OSFALSE;
  }
  else { 
    for(i = 1; i <= npwg->noe ; i++){
      if(npwg->efr[i].el & ST_TD_DUMMY) continue;
      bv = abs(npwg->efr[i].bv);
      ev = abs(npwg->efr[i].ev);
      if(!put_dxf_obj_header(DXF_LINE, dxf_info->color, lpsc->layer))      return OSFALSE;
			if(!put_vertex_to_dxf(&dxf_info->bd, 10, &npwg->v[bv])) return OSFALSE;
			if(!put_vertex_to_dxf(&dxf_info->bd, 11, &npwg->v[ev])) return OSFALSE;
    }
	}
	return OSTRUE;
}

#pragma argsused
OSCAN_COD dxfout_point(void * geo, lpSCAN_CONTROL lpsc)
{
D_POINT p;

	if(lpsc->m) o_hcncrd(lpsc->m, (lpD_POINT)geo, &p);
	else        memcpy(&p, (lpD_POINT)geo, sizeof(p));
	if(!put_dxf_obj_header(DXF_POINT, lpsc->color, lpsc->layer)) return OSFALSE;
	if(!put_vertex_to_dxf(&dxf_info->bd, 10, &p))   return OSFALSE;
	return OSTRUE;
}

#pragma argsused
OSCAN_COD dxfout_line(void * geo, lpSCAN_CONTROL lpsc)
{
	D_POINT p1, p2, p;
	int			n;

	if(lpsc->m) {
		o_hcncrd(lpsc->m, &((lpGEO_LINE)geo)->v1, &p1);
		o_hcncrd(lpsc->m, &((lpGEO_LINE)geo)->v2, &p2);
	} else {
		memcpy(&p1, &((lpGEO_LINE)geo)->v1, sizeof(p1));
		memcpy(&p2, &((lpGEO_LINE)geo)->v2, sizeof(p2));
	}
	if (dxf_info->path > 0) {	
		if (lpsc->status & ST_DIRECT) {
			p  = p1;
			p1 = p2;
			p2 = p;
		}
		if(!put_dxf_obj_header(DXF_VERTEX, lpsc->color, lpsc->layer)) return OSFALSE;
		if (dxf_info->flat) { 
			o_hcncrd(dxf_info->m_gcs_ocs, &p1, &p1);
			o_hcncrd(dxf_info->m_gcs_ocs, &p2, &p2);
		}
		dxf_info->last_v = p2;
		if(!put_vertex_to_dxf(&dxf_info->bd, 10, &p1)) return OSFALSE;
		if (!dxf_info->flat) { 
			n = 32;
			if(!put_dxf_group(&dxf_info->bd, 70, &n)) return OSFALSE;
		}
	} else {	
		if(!put_dxf_obj_header(DXF_LINE, lpsc->color, lpsc->layer)) return OSFALSE;
		if(!put_vertex_to_dxf(&dxf_info->bd, 10, &p1)) return OSFALSE;
		if(!put_vertex_to_dxf(&dxf_info->bd, 11, &p2)) return OSFALSE;
	}
	return OSTRUE;
}

#pragma argsused
OSCAN_COD dxfout_circle(void * g, lpSCAN_CONTROL lpsc)
{
GEO_CIRCLE  geo;
MATR        m_og, m_go;
D_POINT			AxisZ = {0., 0., 1.}, pp;

	memcpy(&geo, g, sizeof(geo));
 	if(lpsc->m ) o_trans_arc((lpGEO_ARC)&geo, lpsc->m, OECS_CIRCLE);
	if (dxf_info->flat) {	
		if (dpoint_eq(&geo.n, dpoint_inv(&AxisZ, &pp), eps_d)) {
			
			dpoint_inv(&geo.n, &geo.n);
		}
	}
	ocs_gcs(&geo.n, m_og, m_go);
	o_hcncrd(m_go, &geo.vc, &geo.vc);

	if(!put_dxf_obj_header(DXF_CIRCLE, lpsc->color, lpsc->layer))     return OSFALSE;
	if(!put_vertex_to_dxf(&dxf_info->bd, 10, &geo.vc)) return OSFALSE;
	if(!put_dxf_group(&dxf_info->bd, 40, &geo.r))      return OSFALSE;
	if (!dpoint_eq(&geo.n, &AxisZ, eps_d)) {
		if(!put_vertex_to_dxf(&dxf_info->bd, 210, &geo.n)) return OSFALSE;
	}

	return OSTRUE;
}

#pragma argsused
OSCAN_COD dxfout_arc(void * g, lpSCAN_CONTROL lpsc)
{
	GEO_ARC  	geo;
	MATR     	m_og, m_go;
	sgFloat   	a1, a2, tang;
	D_POINT		p, AxisZ = {0., 0., 1.};
	int				n;

	memcpy(&geo, g, sizeof(geo));
	if(lpsc->m) o_trans_arc(&geo, lpsc->m, OECS_ARC);
	if (dxf_info->path > 0) {
		if (lpsc->status & ST_DIRECT) {
			
			p      = geo.vb;
			geo.vb = geo.ve;
			geo.ve = p;
			geo.angle = -geo.angle;
			calc_arc_ab(&geo);
		}
		if (dxf_info->flat) {	
			if (dskalar_product(&geo.n, &dxf_info->normal) < - eps_n) {
				
				dpoint_inv(&geo.n, &geo.n);
				geo.angle = -geo.angle;
				calc_arc_ab(&geo);
			}
			if(!put_dxf_obj_header(DXF_VERTEX, lpsc->color, lpsc->layer)) return OSFALSE;
			if (dxf_info->flat) { 
				o_hcncrd(dxf_info->m_gcs_ocs, &geo.vb, &geo.vb);
				o_hcncrd(dxf_info->m_gcs_ocs, &geo.ve, &geo.ve);
			}
			dxf_info->last_v = geo.ve;
			if(!put_vertex_to_dxf(&dxf_info->bd, 10, &geo.vb)) return OSFALSE;
			if (geo.angle < 0) tang = - tan(-geo.angle / 4);
			else               tang =   tan( geo.angle / 4);
			if(!put_dxf_group(&dxf_info->bd, 42, &tang))   return OSFALSE;
		} else {	
			if(!put_dxf_obj_header(DXF_VERTEX, lpsc->color, lpsc->layer)) return OSFALSE;
			dxf_info->last_v = geo.ve;
			if(!put_vertex_to_dxf(&dxf_info->bd, 10, &geo.vb)) return OSFALSE;
			n = 32;
			if(!put_dxf_group(&dxf_info->bd, 70, &n)) return OSFALSE;
		}
	} else {
		if (dxf_info->flat) {	
			if (dpoint_eq(&geo.n, dpoint_inv(&AxisZ, &p), eps_d)) {
				
				dpoint_inv(&geo.n, &geo.n);
				geo.angle = -geo.angle;
				calc_arc_ab(&geo);
			}
		}
		ocs_gcs(&geo.n, m_og, m_go);
		o_hcncrd(m_go, &geo.vc, &geo.vc);
		o_hcncrd(m_go, &geo.vb, &geo.vb);
		o_hcncrd(m_go, &geo.ve, &geo.ve);
		a1 = (v_angle(geo.vb.x - geo.vc.x, geo.vb.y - geo.vc.y)*180)/M_PI;
		a2 = (v_angle(geo.ve.x - geo.vc.x, geo.ve.y - geo.vc.y)*180)/M_PI;
		if(geo.angle < 0) { geo.angle = a1; a1 = a2; a2 = geo.angle;}
		if(!put_dxf_obj_header(DXF_ARC, lpsc->color, lpsc->layer))      return OSFALSE;
		if(!put_vertex_to_dxf(&dxf_info->bd, 10, &geo.vc)) return OSFALSE;
		if(!put_dxf_group(&dxf_info->bd, 40, &geo.r))      return OSFALSE;
		if(!put_dxf_group(&dxf_info->bd, 50, &a1))         return OSFALSE;
		if(!put_dxf_group(&dxf_info->bd, 51, &a2))         return OSFALSE;
		if (!dpoint_eq(&geo.n, &AxisZ, eps_d)) {
			if(!put_vertex_to_dxf(&dxf_info->bd, 210, &geo.n)) return OSFALSE;
		}
	}
	return OSTRUE;
}

#pragma argsused
OSCAN_COD dxfout_text(void * g, lpSCAN_CONTROL lpsc)
{
lpFONT       font;
UCHAR        *text, *stext;
BOOL         mline = FALSE, fline = TRUE;
ULONG        len;
GEO_TEXT 	   geo;
MATR     	   m_og, m_go;
sgFloat   	   a1, a2, height, xscale, gdy, dt, xbeg;
D_POINT		   p, pn = {0, 0, 1}, px = {1, 0, 0}, ps =  {0, 0, 0};


	memcpy(&geo, g, sizeof(geo));
	if(lpsc->m)
		o_hcmult(geo.matr, lpsc->m);
	if(!geo.font_id || !geo.text_id	) return OSFALSE;
	if(0 ==(font = (FONT*)alloc_and_get_ft_value(geo.font_id, &len))) return OSFALSE;
	if(0 ==(stext = (UCHAR*)alloc_and_get_ft_value(geo.text_id, &len))){
		SGFree(font);
		return OSFALSE;
	}
	text = stext;
	height = fabs(geo.style.height*get_grf_coeff());
	if(font->num_up)
		gdy = height/font->num_up;
	else
		gdy = height;
	xscale = fabs(geo.style.scale/100);
	a2 = geo.style.angle;
	dt = gdy*(font->num_up + font->num_dn) + (geo.style.dver*height)/100;

	SGFree(font);
	if(*text == ML_S_C) {
		mline = TRUE;
		text++;
	}
	xbeg = ps.x;
	while(*text != ML_S_C){
		p = ps;
		if(fline){
			o_hcncrd(geo.matr, &pn, &pn);
			o_hcncrd(geo.matr, &px, &px);
			o_hcncrd(geo.matr, &p, &p);
			dpoint_sub(&pn, &p, &pn);
			ocs_gcs(&pn, m_og, m_go);
			o_hcncrd(m_go, &p, &p);
			o_hcncrd(m_go, &px, &px);
			a1 = (v_angle(px.x - p.x, px.y - p.y)*180)/M_PI;
			o_hcmult(geo.matr, m_go);
			fline = FALSE;
		}
		else {
			o_hcncrd(geo.matr, &p, &p);
		}
		if(!put_dxf_obj_header(DXF_TEXT, lpsc->color, lpsc->layer))  goto err;
		if(!put_vertex_to_dxf(&dxf_info->bd, 10, &p))   goto err;
		if(!put_dxf_group(&dxf_info->bd, 1, text))      goto err;
		if(!put_dxf_group(&dxf_info->bd, 40, &height))  goto err;
		if(!put_dxf_group(&dxf_info->bd, 41, &xscale))  goto err;
		if(!put_dxf_group(&dxf_info->bd, 50, &a1))      goto err;
		if(!put_dxf_group(&dxf_info->bd, 51, &a2))      goto err;
		if(!put_vertex_to_dxf(&dxf_info->bd, 210, &pn)) goto err;
		
		ps.y -= dt;
		ps.x = xbeg;
		if(!mline) break;
		text += (strlen((char*)text) + 1);
	}
	if (stext) SGFree(stext);
	return OSTRUE;
err:
	if (stext) SGFree(stext);
	return OSFALSE;
}

#pragma argsused
OSCAN_COD dxfout_spline(void * geo, lpSCAN_CONTROL lpsc)
{
	D_POINT       p1, p = {0., 0., 0.};
	lpGEO_SPLINE  g = (lpGEO_SPLINE)geo;
	SPLY_DAT      sply_dat;
	BOOL          knot = TRUE;
	short           n = 1, flag3D;
	OSCAN_COD     ret = OSFALSE;

	if(!begin_use_sply(g, &sply_dat)) return OSFALSE;

	flag3D = (lpsc->status & ST_FLAT) ? 0 : 32;	

	if(!put_dxf_obj_header(DXF_POLYLINE, lpsc->color, lpsc->layer)) goto end;
	if(!put_dxf_group(&dxf_info->bd, 66, &n))          goto end;
	if(!put_vertex_to_dxf(&dxf_info->bd, 10, &p))      goto end;
	n = (lpsc->status & ST_FLAT) ? 0 : 8;	
	if(sply_dat.sdtype & SPL_CLOSE) n += 1 ;
	if(n != 0 && !put_dxf_group(&dxf_info->bd, 70, &n)) goto end;
	get_first_sply_point(&sply_dat, c_h_tolerance, FALSE, &p);
	p1 = p;
//	while (get_next_sply_point(&sply_dat, &p)){
	while (get_next_sply_point_tolerance(&sply_dat, &p, &knot)) {
		if (!dxfout_sply_point(&p1, FALSE, lpsc->m, 0, flag3D)) goto end;
		p1 = p;
	}
	if (!(sply_dat.sdtype & SPL_CLOSE))
		if(!dxfout_sply_point(&p1, FALSE, lpsc->m, 0, flag3D)) goto end;

	if(!put_dxf_group(&dxf_info->bd, 0, dxf_keys[DXF_SEQEND])) goto end;

	ret = OSTRUE;
end:
	end_use_sply(g, &sply_dat);
	return ret;
}

static BOOL dxfout_sply_point(lpD_POINT p, BOOL knot, lpMATR m,
															short flag, short flag3D)
{
	short n;

	if(m) o_hcncrd(m, p, p);
	if(flag && knot){
		if(!put_dxf_obj_header(DXF_VERTEX, -1, dxf_info->layer))        return FALSE;
		if(!put_vertex_to_dxf(&dxf_info->bd, 10, p))   return FALSE;
		n =  flag3D | 16;
		if(n != 0 && !put_dxf_group(&dxf_info->bd, 70, &n)) return FALSE;
	}
	if(!put_dxf_obj_header(DXF_VERTEX, -1, dxf_info->layer))          return FALSE;
	if(!put_vertex_to_dxf(&dxf_info->bd, 10, p))     return FALSE;
	n =  flag3D | flag;
	if(n != 0 && !put_dxf_group(&dxf_info->bd, 70, &n)) return FALSE;
	return TRUE;
}

OSCAN_COD dxfout_frame(void * g1, lpSCAN_CONTROL lpsc)
{
GEO_OBJ     geo;
OBJTYPE     type;
short         color;
UCHAR       ltype, lthick;

	frame_read_begin((lpGEO_FRAME)g1);
	while(frame_read_next(&type, &color, &ltype, &lthick, &geo)){
    if(!dxfout_type_g[type](&geo, lpsc)) return OSFALSE;
  }
  return OSTRUE;
}

static BOOL  dxf_export_tables(void)

{
int            i;
LAYER          layer;
BOOL           ret = FALSE;
short          inum;
	
	const char ttt[] = "CONTINUOUS";

	if(!vd_dxf_layers)
		return TRUE;
	if(!vd_dxf_layers->num_elem)
		return TRUE;

	if(!put_dxf_group(&dxf_info->bd, 0, dxf_keys[DXF_SECTION])) goto end;
	if(!put_dxf_group(&dxf_info->bd, 2, dxf_keys[DXF_TABLES])) goto end;
	if(!put_dxf_group(&dxf_info->bd, 0, dxf_keys[DXF_TABLE])) goto end;
	if(!put_dxf_group(&dxf_info->bd, 2, dxf_keys[DXF_LAYER])) goto end;
	inum = (short)vd_dxf_layers->num_elem;
	if(!put_dxf_group(&dxf_info->bd, 70, &inum)) goto end;

	for(i = 0; i < vd_dxf_layers->num_elem; i++){
		if(!read_elem(vd_dxf_layers, i, &layer)) goto end;
		if(!put_dxf_group(&dxf_info->bd, 0, dxf_keys[DXF_LAYER])) goto end;
		if(!put_dxf_group(&dxf_info->bd, 2, layer.name)) goto end;
		inum = 7;
		if(!put_dxf_group(&dxf_info->bd, 62, &inum)) goto end;
		if(!put_dxf_group(&dxf_info->bd, 6, (void*)ttt)) goto end;
		inum = 0;
		if(!put_dxf_group(&dxf_info->bd, 70, &inum)) goto end;
	}
	if(!put_dxf_group(&dxf_info->bd, 0, dxf_keys[DXF_ENDTAB])) goto end;
	if(!put_dxf_group(&dxf_info->bd, 0, dxf_keys[DXF_ENDSEC])) goto end;
	ret = TRUE;
end:
	return ret;
}

