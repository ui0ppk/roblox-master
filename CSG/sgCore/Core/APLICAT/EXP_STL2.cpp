#include "../sg.h"

static BOOL stlout_header(void);
static BOOL stlout_tail(void);
static OSCAN_COD stlout_geo_scan(hOBJ hobj, lpSCAN_CONTROL lpsc);
static BOOL stl_put_face(lpNPW npw, short numf, short lv, short *v, short *vi);
static BOOL stl_post_face(BOOL ret, lpNPW npw, short num_np, short numf);
static BOOL put_vertex_in_stl(lpD_POINT v);
static BOOL export_to_stl_objects_list(lpLISTH listh, NUM_LIST num_list);
static BOOL stl_export_obj(hOBJ hobj);

//---------------
//  STL
//---------------

bool stl_full_flag    = true;

typedef struct {
	BUFFER_DAT bd;
	BOOL       trian_error;
}STL_INFO;
typedef STL_INFO * lpSTL_INFO;

static lpSTL_INFO stl_info;

bool export_3d_to_stl(char *name)

{
STL_INFO di;
bool     ret = false;

	di.trian_error = FALSE;
	stl_info = &di;

	if(!init_buf(&stl_info->bd, name, BUF_NEW)) return false;

	if (stlout_header()) 
	{
		ret = (stl_full_flag) ?
		export_to_stl_objects_list(&objects,  OBJ_LIST) :
		export_to_stl_objects_list(&selected, SEL_LIST) ;
		if (ret) 
			ret = stlout_tail()?true:false;
	}

	close_buf(&stl_info->bd);
	if(!ret) 
		nb_unlink(name);
	if(ret && di.trian_error)
		put_message(EMPTY_MSG, GetIDS(IDS_SG001), 0);
	return ret;
}

static BOOL stlout_header(void)

{
	char str[] ={"solid\r\n"};

	if	(!story_data(&stl_info->bd, strlen(str), str)) return FALSE;
	return TRUE;
}

static BOOL stlout_tail(void)
{
	char str[] ={"endsolid\r\n"};

	if (!story_data(&stl_info->bd, strlen(str), str)) return FALSE;
	return TRUE;
}

static BOOL export_to_stl_objects_list(lpLISTH listh, NUM_LIST num_list)

{
short   n = 0;
hOBJ  hobj;
BOOL  ret = FALSE, errflag = FALSE;
	hobj = listh->hhead;
	while (hobj) {
		if (num_list == SEL_LIST) {
			unmark_obj(hobj);
//			change_viobj_color(hobj, COBJ);
			//draw_obj(hobj, CDEFAULT);
		}
		if (!errflag){
			if (!stl_export_obj(hobj)) {
				if (num_list != SEL_LIST) goto end;
				errflag = TRUE;
			}
//			step_grad (num_proc , ++n);
		}
		get_next_item_z(num_list, hobj, &hobj);
	}
	if (errflag) goto end;
	ret = TRUE;
end:
//	end_grad(num_proc, listh->num);
	return ret;
}

static BOOL stl_export_obj(hOBJ hobj)
{
	OSCAN_COD    code;
	SCAN_CONTROL sc;

	init_scan(&sc);
	sc.user_geo_scan = stlout_geo_scan;
	sc.data = stl_info;
	code = o_scan(hobj, &sc);
	return (code == OSFALSE) ? FALSE : TRUE;

/*	lpOBJ 	obj = static_cast<lpOBJ>(hobj);

	if (obj->extendedClass==NULL)
		return TRUE;
	if (obj->extendedClass->GetType()!=SG_OT_3D)
		return TRUE;
	sgC3DObject*  obj3D = reinterpret_cast<sgC3DObject*>(obj->extendedClass);

	const SG_ALL_TRIANGLES*  objTr = obj3D->GetTriangles();
	
	if (objTr==NULL)
		return TRUE;

	SG_VECTOR  tmpV1, tmpV2;
	D_POINT    tmpPnt;

	char str1[] ={"  facet normal   "};
	char str2[] ={"    outer loop\r\n"};
	char str3[] ={"      vertex   "};
	char str4[] ={"    endloop\r\n"};
	char str5[] ={"  endfacet\r\n"};
	lpBUFFER_DAT bd = &stl_info->bd;

	for(int i = 0; i < 3*objTr->nTr; i += 3)
	{
		tmpV2 = sgSpaceMath::VectorsAdd(objTr->allNormals[i],objTr->allNormals[i+1]);
		tmpV1 = sgSpaceMath::VectorsAdd(tmpV2,objTr->allNormals[i+2]);
		sgSpaceMath::NormalVector(tmpV1);

		memcpy(&tmpPnt,&tmpV1,sizeof(D_POINT));

		if(!story_data(bd, strlen(str1), str1)) return FALSE;
		if(!put_vertex_in_stl(&tmpPnt))	return FALSE;
		if(!story_data(bd, strlen(str2), str2)) return FALSE;
		for(int ttt = 0; ttt < 3; ttt++){
			if(!story_data(bd, strlen(str3), str3)) return FALSE;
			memcpy(&tmpPnt,&(objTr->allVertex[i+ttt]),sizeof(D_POINT));
			if(!put_vertex_in_stl(&tmpPnt))
				return FALSE;
		}
		if(!story_data(bd, strlen(str4), str4)) return FALSE;
		if(!story_data(bd, strlen(str5), str5)) return FALSE;
	}


	return TRUE;*/
}

#pragma argsused
static OSCAN_COD stlout_geo_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	BOOL		ret;
	short		i,	num_np;

	if ( lpsc->type != OBREP) return OSSKIP;
	if ( scan_flg_np_begin ) num_np = 0; 

	if ( lpsc->breptype != BODY && lpsc->breptype != SURF) return OSSKIP;
	trian_pre_brep  = NULL;
	trian_post_brep = NULL;
	trian_put_face  = stl_put_face;
	trian_post_face = stl_post_face;
	if (lpsc->m) {
		for (i = 1; i <= npwg->nov; i++) {
			o_hcncrd(lpsc->m, &npwg->v[i], &npwg->v[i]);
		}
	}
	num_np = 0;
	if(!np_cplane(npwg)) return OSFALSE; 
	ret = trian_np(npwg, num_np++, 3);
//	ret = trian_brep(hobj, 3);
	trian_pre_brep  = NULL;
	trian_post_brep = NULL;
	trian_put_face  = NULL;
	trian_post_face = NULL;
	return (ret) ? OSTRUE : OSFALSE;
}

#pragma argsused
static BOOL stl_post_face(BOOL ret, lpNPW npw, short num_np, short numf){
	if(ret) return TRUE;
	if(!trian_error) return FALSE; 
	trian_error = FALSE;
	stl_info->trian_error = TRUE;
	return TRUE;
}

#pragma argsused
static BOOL stl_put_face(lpNPW npw, short numf, short lv, short *v, short *vi)
{
char str1[] ={"  facet normal   "};
char str2[] ={"    outer loop\r\n"};
char str3[] ={"      vertex   "};
char str4[] ={"    endloop\r\n"};
char str5[] ={"  endfacet\r\n"};
short  i;
lpBUFFER_DAT bd = &stl_info->bd;

	if(!story_data(bd, strlen(str1), str1)) return FALSE;
	if(!put_vertex_in_stl(&npw->p[numf].v))
		return FALSE;
	if(!story_data(bd, strlen(str2), str2)) return FALSE;
	for(i = 0; i < 3; i++){
		if(!story_data(bd, strlen(str3), str3)) return FALSE;
		if(!put_vertex_in_stl(&npw->v[v[i]]))
			return FALSE;
	}
	if(!story_data(bd, strlen(str4), str4)) return FALSE;
	if(!story_data(bd, strlen(str5), str5)) return FALSE;
	return TRUE;
}

static BOOL put_vertex_in_stl(lpD_POINT v){
char   buf[MAX_sgFloat_STRING], str[] = {"  \r\n"};
sgFloat *d;
short    i, j = 0;

	d = (sgFloat*)v;
	for(i = 0; i < 3; i++){
		sgFloat_to_text_for_export(buf, d[i]);
		if(!story_data(&stl_info->bd, strlen(buf), buf)) return FALSE;
		if(i == 2) j = 2;
		if(!story_data(&stl_info->bd, 2, &str[j])) return FALSE;
	}
	return TRUE;
}

