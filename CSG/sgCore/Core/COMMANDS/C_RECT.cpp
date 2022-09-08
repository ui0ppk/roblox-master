#include "../sg.h"


static void add_rectangle_csg(hOBJ hobj, sgFloat a, sgFloat b);
//--

//++ SVP 19/09/2002
static void add_rectangle_csg(hOBJ hobj, sgFloat a, sgFloat b)
{
	lpOBJ 		obj;
	VADR			hcsg;
	short			size;
	lpCSG			csg;                          
	char*     adr;

	
	size = sizeof(short) + 2 * sizeof(sgFloat);
	hcsg = SGMalloc(size + sizeof(short));
	csg = (lpCSG)hcsg;
	csg->size = size;
	*(short*)csg->data = RECTANGLE;
	adr = csg->data + sizeof(short);
	memcpy(adr, &a, sizeof(sgFloat));
	adr += sizeof(sgFloat);
	memcpy(adr, &b, sizeof(sgFloat));
	obj = (lpOBJ)hobj;
	obj->hcsg = hcsg;
	return;
}

hOBJ create_rectangle(sgFloat height, sgFloat width, D_POINT p0, D_POINT pX, D_POINT pY, UCHAR color)
{
	GEO_PATH		path;
	lpGEO_PATH	lppath;
  hOBJ				hpath, hobj;
  lpOBJ				obj;
  D_POINT			p[4];
	OSCAN_COD		cod;
  MATR				m;
  int					i;

	memset(&path, 0, sizeof(GEO_PATH));
	path.kind = PATH_RECT;
	o_hcunit(path.matr);
  p[0].x 	= p[0].y = p[0].z = 0.0;
  p[1] 		= p[0];
  p[1].y 	= height;
  p[2]		= p[1];
  p[2].x	= width;
  p[3]		= p[2];
  p[3].y	= 0.0;
 	lcs_oxy(&p0, &pX, &pY, m);
  for (i = 0; i < 4; i++)
	  o_hcncrd(m, &p[i], &p[i]);
	if (create_line(&p[0], &p[1], &path.listh) == NULL) goto err;
	if (create_line(&p[1], &p[2], &path.listh) == NULL) goto err;
	if (create_line(&p[2], &p[3], &path.listh) == NULL) goto err;
	if (create_line(&p[3], &p[0], &path.listh) == NULL) goto err;
  if ((hpath = o_alloc(OPATH)) == NULL) goto err;
  obj = (lpOBJ)hpath;
  set_obj_nongeo_par(obj);
  memcpy(obj->geo_data, &path, geo_size[OPATH]);
  hobj = path.listh.hhead;
  while (hobj) {
    obj 				= (lpOBJ)hobj;
	  obj->color 	= color;
    obj->hhold 	= hpath;
    get_next_item(hobj, &hobj);
  }
	if (!get_object_limits(hpath, &path.min, &path.max)) goto err2;
	obj = (lpOBJ)hpath;
	lppath = (lpGEO_PATH)obj->geo_data;
	lppath->min = path.min;
	lppath->max = path.max;
	if (!set_contacts_on_path(hpath)) goto err2;
	if (is_path_on_one_line(hpath))
	{
		obj->status |= ST_ON_LINE;
		obj->status &= ~ST_FLAT;
	}
	else
	{
		if (set_flat_on_path(hpath, NULL))
			obj->status |= ST_FLAT;
		else      
			obj->status &= ~ST_FLAT;
	}
	//if (!set_flat_on_path(hpath, NULL)) goto err2;
	if ((cod = test_self_cross_path(hpath, NULL)) == OSFALSE) goto err2;
	if (cod == OSTRUE) {
		obj = (lpOBJ)hpath;
		obj->status |= ST_SIMPLE;
	}
	add_rectangle_csg(hpath, height, width);
  return hpath;
err2:
	autodelete_obj(hpath);
	return NULL;
err:
	while ((hobj = path.listh.hhead) != NULL) {
		o_free(hobj, &path.listh);
	}
	return NULL;
}
//--
