#include "../sg.h"

static BOOL cr_path_obj(lpLISTH listh,lpLISTH list_path);

void rfcvt(short cvtType, unsigned char *p, sgFloat *cd)
{
	WORD e;
	unsigned char * c = (unsigned char*)cd;

	switch (cvtType) {
		case 2:		// C sgFloat -> Pascal real
			e = ((c[6] >> 4) & 0xf) | (c[7] << 4);
			if (e > 0) e -= 894;
			p[0] = (BYTE)e;
			p[5] = (BYTE)(((c[6] & 0xf) << 3) | (c[5] >> 5) | (c[7] & 0x80));
			p[4] = (BYTE)((c[5] << 3) | (c[4] >> 5));
			p[3] = (BYTE)((c[4] << 3) | (c[3] >> 5));
			p[2] = (BYTE)((c[3] << 3) | (c[2] >> 5));
			p[1] = (BYTE)((c[2] << 3) | (c[1] >> 5));
			break;
		case 3:		// Pascal real -> C sgFloat
			e = p[0] & 0xff;
			if (e > 0) e += 894;
			c[7] = (BYTE)((p[5] & 0x80) | ((e >> 4) & 0x7f));
			c[6] = (BYTE)(((p[5] & 0x7f) >> 3) | ((e & 0xf) << 4));
			c[5] = (BYTE)(((p[5] & 0x7) << 5) | (p[4] >> 3));
			c[4] = (BYTE)(((p[4] & 0x7) << 5) | (p[3] >> 3));
			c[3] = (BYTE)(((p[3] & 0x7) << 5) | (p[2] >> 3));
			c[2] = (BYTE)(((p[2] & 0x7) << 5) | (p[1] >> 3));
			c[1] = (BYTE)(((p[1] & 0x7) << 5));
			c[0] = 0;
	}
}
static BOOL cr_path_obj(lpLISTH listh,lpLISTH list_path)
{
	hOBJ hpath, hobj;
	lpOBJ obj;
	lpGEO_PATH path;
	D_POINT	mn, mx;

	if ( (hpath = o_alloc(OPATH)) == NULL ) return FALSE;

	obj = (lpOBJ)hpath;
	path = (lpGEO_PATH)obj->geo_data;
	o_hcunit(path->matr);
	path->listh = *list_path;
	hobj = list_path->hhead;
	while ( hobj ) {
		obj = (lpOBJ)hobj;
		obj->hhold = hpath;
		get_next_item(hobj,&hobj);
	}
	if (!set_contacts_on_path(hpath)) goto err;
	obj = (lpOBJ)hpath;
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
	//if (!set_flat_on_path(hpath, NULL)) goto err;
  if (test_self_cross_path(hpath,NULL) == OSTRUE) {
		obj = (lpOBJ)hpath;
    obj->status |= ST_SIMPLE;
  }
	if (get_object_limits(hpath,&mn,&mx)) {
		obj = (lpOBJ)hpath;
		path = (lpGEO_PATH)obj->geo_data;
		path->min = mn;
		path->max = mx;
	}
	attach_item_tail(listh,hpath);
	return TRUE;
err:
	o_free(hpath, NULL);
	return FALSE;
}
