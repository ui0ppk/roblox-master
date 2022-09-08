#include "../sg.h"



static void add_polygon_csg(hOBJ hobj, short NumSide, sgFloat OutRadius);

hOBJ create_line(lpD_POINT v1, lpD_POINT v2, lpLISTH listh)
{
	hOBJ			hobj;
	lpOBJ			obj;
	GEO_LINE 	line;

	if ((hobj = o_alloc(OLINE)) != NULL) {
		obj = (lpOBJ)hobj;
		line.v1 = *v1;
    line.v2 = *v2;
		memcpy(obj->geo_data, &line, geo_size[OLINE]);
		attach_item_tail(listh, hobj);
	}
	return hobj;
}

static void add_polygon_csg(hOBJ hobj, short NumSide, sgFloat OutRadius)
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
	*(short*)csg->data = POLYGON;
	adr = csg->data + sizeof(short);
	memcpy(adr, &NumSide, sizeof(NumSide));
	adr += sizeof(NumSide);
	memcpy(adr, &OutRadius, sizeof(OutRadius));
	obj = (lpOBJ)hobj;
	obj->hcsg = hcsg;
	return;
}

hOBJ create_polygon(short NumSide, sgFloat OutRadius, D_POINT p0, D_POINT pX, D_POINT pY, UCHAR color)
{
	GEO_PATH		path;
	lpGEO_PATH	lppath;
  hOBJ				hpath, hobj;
  lpOBJ				obj;
  D_POINT			p1, p2, vp1, vp2;
	OSCAN_COD		cod;
  MATR				m, matr1;
  int					i;

	memset(&path, 0, sizeof(GEO_PATH));
	path.kind = PATH_RECT;
	o_hcunit(path.matr);
  p1.x 	= OutRadius; p1.y = p1.z = 0.0;
 	lcs_oxy(&p0, &pX, &pY, m);
	o_hcunit(matr1);
	o_tdrot(matr1, 3, 360. / NumSide);
  for (i = 0; i < NumSide; i++) {
		o_hcncrd(matr1, &p1, &p2);
		o_hcncrd(m, &p1, &vp1);
		o_hcncrd(m, &p2, &vp2);
		if ((hobj = create_line(&vp1, &vp2, &path.listh))	== NULL) goto err;
		p1 = p2;
	}
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
	add_polygon_csg(hpath, NumSide, OutRadius);
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

