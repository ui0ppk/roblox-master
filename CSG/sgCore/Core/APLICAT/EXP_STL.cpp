#include "../sg.h"

static lpBUFFER_DAT bd;
static BOOL         stl_error = FALSE;

static BOOL stl_pre_brep(hOBJ hobj);
static BOOL stl_post_brep(BOOL ret, hOBJ hobj);
static BOOL stl_put_face(lpNPW npw, short numf, short lv, short *v, short *vi);
static BOOL stl_post_face(BOOL ret, lpNPW npw, short num_np, short numf);
static BOOL put_vertex_in_stl(lpD_POINT v);

BOOL export_stl(char * name, hOBJ hobj)
{
BUFFER_DAT bdw;
BOOL       ret;

	bd = &bdw;
  stl_error = FALSE;
	if(!init_buf(bd, name, BUF_NEW)) return FALSE;
  trian_pre_brep  = stl_pre_brep;
  trian_post_brep = stl_post_brep;
  trian_put_face  = stl_put_face;
  trian_post_face = stl_post_face;
	ret = trian_brep(hobj, 3);
  trian_pre_brep  = NULL;
  trian_post_brep = NULL;
  trian_put_face  = NULL;
  trian_post_face  = NULL;
	close_buf(bd);
	if(!ret) nb_unlink(name);
  if(ret && stl_error)
    put_message(EMPTY_MSG, GetIDS(IDS_SG174), 0);
	return ret;
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
		if(!story_data(bd, strlen(buf), buf)) return FALSE;
		if(i == 2) j = 2;
		if(!story_data(bd, 2, &str[j])) return FALSE;
	}
	return TRUE;
}

#pragma argsused
static BOOL stl_pre_brep(hOBJ hobj)
{
char str[] ={"solid\r\n"};
  if(!story_data(bd, strlen(str), str)) return FALSE;
  return TRUE;
}

#pragma argsused
static BOOL stl_post_brep(BOOL ret, hOBJ hobj)
{
char str[] ={"endsolid\r\n"};
	if(!ret) return FALSE;
	if(!story_data(bd, strlen(str), str)) return FALSE;
	return TRUE;
}

#pragma argsused
static BOOL stl_post_face(BOOL ret, lpNPW npw, short num_np, short numf){
	if(ret) return TRUE;
	if(!trian_error) return FALSE; 
	trian_error = FALSE;
	stl_error = TRUE;
	return TRUE;
}

