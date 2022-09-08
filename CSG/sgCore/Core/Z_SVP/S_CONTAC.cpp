#include "../sg.h"

/*
		  :
			-    ,  ;
			-   
*/

typedef struct {
	hOBJ 		hobj_first, hobj_prev;
	UCHAR		d_first, 		d_prev;
	GEO_OBJ	prev_geo;
	OBJ			prev_obj;
	D_POINT	first_v, 		prev_v, first_vb;
} CONTACT_DATA;
typedef CONTACT_DATA * lpCONTACT_DATA;

static OSCAN_COD contact_geo_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);
static void set_contact_in_node(lpCONTACT_DATA data, hOBJ hobj,
																UCHAR direct, lpD_POINT curr_v);
static OSTATUS get_mask_of_status(UCHAR direct);
static lpD_POINT get_ptr_to_begin(OBJTYPE type, void * geo, UCHAR direct);

BOOL set_contacts_on_path(hOBJ hobj)
{
	SCAN_CONTROL 	sc;
	CONTACT_DATA	data = {NULL, NULL, 0, 0};
	lpOBJ					obj;

	init_scan(&sc);
	sc.user_geo_scan  = contact_geo_scan;
	sc.data 	        = &data;
	if (o_scan(hobj,&sc) == OSFALSE) return FALSE;
	//   
	if (dpoint_eq(&data.first_vb,
			get_ptr_to_begin(data.prev_obj.type, &data.prev_geo, 1),
			eps_d)) {
		//  
		obj = (lpOBJ)hobj;
		obj->status |= ST_CLOSE;
		set_contact_in_node(&data, data.hobj_first, data.d_first, &data.first_v);
	}
	return TRUE;
}

static OSCAN_COD contact_geo_scan(hOBJ hobj,lpSCAN_CONTROL lpsc)
{
	lpCONTACT_DATA 	data = (lpCONTACT_DATA)lpsc->data;
	lpOBJ						obj;
	UCHAR						direct;
	GEO_OBJ					curr_geo;
	OBJ							curr_obj;
	D_POINT					curr_v[2];

	direct = (UCHAR)((lpsc->status & ST_DIRECT) ? 1 : 0);
	//   
	get_simple_obj(hobj, &curr_obj, &curr_geo);
	if (direct) pereor_geo(curr_obj.type, &curr_geo);
	calc_geo_direction(curr_obj.type, &curr_geo, curr_v);
	if (!data->hobj_first) {
		//       
		data->hobj_first = hobj;
		data->d_first    = direct;
		data->first_v    = curr_v[0];
		data->first_vb   = *get_ptr_to_begin(curr_obj.type, &curr_geo, 0);
		//    
		obj = (lpOBJ)hobj;
		obj->status &= (UCHAR)(~get_mask_of_status((UCHAR)(!(data->d_first))));
	}
	if (data->hobj_prev)
		// A   :   
		set_contact_in_node(data, hobj, direct, curr_v);

	//      
	data->hobj_prev = hobj;
	data->d_prev    = (UCHAR)(lpsc->status & ST_DIRECT);
	data->prev_obj  = curr_obj;
	data->prev_geo  = curr_geo;
	data->prev_v		= curr_v[1];
	return OSTRUE;
}

// A   :   
static void set_contact_in_node(lpCONTACT_DATA data, hOBJ hobj,
																UCHAR direct, lpD_POINT curr_v)
{
	lpOBJ	obj;

	switch (coll_tst(curr_v, TRUE, &data->prev_v, TRUE)) {
		case 1: 	//  
			obj = (lpOBJ)data->hobj_prev;
			obj->status |= (UCHAR)(get_mask_of_status((UCHAR)(!(data->d_prev))));
			obj = (lpOBJ)hobj;
			obj->status |= get_mask_of_status(direct);
			break;
		case 0:		//   
		case -1:	//    -   1
			obj = (lpOBJ)data->hobj_prev;
			obj->status &= (UCHAR)(~get_mask_of_status((UCHAR)(!(data->d_prev))));
			obj = (lpOBJ)hobj;
			obj->status &= ~get_mask_of_status(direct);
			break;
	}
}

//       .
static OSTATUS get_mask_of_status(UCHAR direct)
{
	OSTATUS	status[2] = { ST_CONSTR1 , ST_CONSTR2 };

	return status[direct];
}

//        
void calc_geo_direction(OBJTYPE type, void * geo, lpD_POINT v)
{
  D_POINT       p[2];
	short        		i;

  get_geo_endpoints_and_vectors(type, geo, p, v);
  for (i = 0; i < 2; i++)
    normv(TRUE, &v[i], &v[i]);

}

//    /   
static lpD_POINT get_ptr_to_begin(OBJTYPE type, void * geo, UCHAR direct)
{
	lpGEO_LINE 	line;
	lpGEO_ARC		arc;
	lpD_POINT		p;

	switch (type) {
		case OLINE:
			line = (lpGEO_LINE)geo;
			p = (direct) ? &line->v2 : &line->v1;
			break;
		case OARC:
			arc = (lpGEO_ARC)geo;
			p = (direct) ? &arc->ve : &arc->vb;
			break;
		default:
			put_message(INTERNAL_ERROR,"S_CONTAC.C",0);
			p = (lpD_POINT)geo;
			break;
	}
	return p;
}
