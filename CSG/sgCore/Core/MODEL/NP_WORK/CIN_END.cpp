#include "../../sg.h"

BOOL cinema_end(lpNP_STR_LIST c_list_str, hOBJ *hobj)
{
	LISTH	listh;
	int		number;

	init_listh(&listh);
	number = np_end_of_put(c_list_str, NP_ALL, (float)c_smooth_angle, &listh);
	*hobj = make_group(&listh, TRUE);
	if ( number ) return TRUE;
	else					return FALSE;
}

hOBJ make_group(lpLISTH listh, BOOL one)
{
	hOBJ        hobj, hobj1;
	lpOBJ       obj;//nb      = NULL;
	lpGEO_GROUP group;
	D_POINT			min, max;

//nb	hobj = NULL;
	if (listh->num > 1 || !one ) {
		if ( ( hobj = o_alloc(OGROUP)) == NULL) goto err;

		//     
		hobj1 = listh->hhead;
		while (hobj1) {
			obj = (lpOBJ)hobj1;
			obj->hhold = hobj;
			get_next_item(hobj1,&hobj1);
		}

		obj = (lpOBJ)hobj;
		group = (lpGEO_GROUP)obj->geo_data;
		o_hcunit(group->matr);
		group->listh = *listh;
		if (!get_object_limits(hobj, &min, &max)) goto err;
		group->min = min;
		group->max = max;
	} else {
		hobj = listh->hhead;
	}
	return hobj;
err:
	while ((hobj1 = listh->hhead) != NULL) {
		o_free(hobj1, listh);
	}
	if (hobj) o_free(hobj, NULL);
	return NULL;
}
