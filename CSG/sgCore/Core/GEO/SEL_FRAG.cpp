#include "../sg.h"

GEO_ARC fr_geo;
D_POINT fr_point;
OBJTYPE fr_type;
BOOL    fr_mod;

BOOL select_fragment(hOBJ hobj, lpSCAN_CONTROL lpsc)
//      
 
{
BOOL      flag;
OBJTYPE   type;
GEO_ARC   geo;
D_POINT   p[2];
short       kp;
// lpOBJ     obj;

  get_scanned_geo(hobj, lpsc, &geo);
  type = lpsc->type;
  set_flag_el(&type, &flag);

  if(!intersect_two_obj((fr_type == OCIRCLE)? OARC : fr_type, &fr_geo,
												(fr_type == OCIRCLE)? FALSE : TRUE,
	                      type, &geo, flag, p, &kp))
		return FALSE;
	if(kp)
		fragment_points(p, kp);
	return TRUE;
}

void fragment_points(lpD_POINT p, short kp){

lpGEO_ARC geo[2] = {&el_geo1, &el_geo2};
OBJTYPE   type[2];
short       i, j;

	for(i = 0; i < kp; i++){
		if((i == 1) &&
			 !test_in_pe_3d(TRUE, &p[i], fr_type, &fr_geo, TRUE))
			 break;
		j = 0;
		switch(break_obj_p(&p[i], fr_type, &fr_geo, geo[0], &type[0],
		                                            geo[1], &type[1])){
			case 2:
				j = (test_in_pe_3d(TRUE, &fr_point, type[0], geo[0], TRUE)) ? 0: 1;
			case 1:
				memcpy(&fr_geo, geo[j], geo_size[type[j]]);
				fr_type = type[j];
				fr_mod = TRUE;
			default:
				break;
		}
	}
}

short delete_fragment(OBJTYPE type, void  *geo,
								    void  *geo1, OBJTYPE *type1,
										void  *geo2, OBJTYPE *type2)
   
{
short       ke = 0;
D_POINT p1, p2;

	switch(fr_type){
		case OCIRCLE:
			return ke;
		case OARC:
			dpoint_copy(&p1, &((lpGEO_ARC)(&fr_geo))->vb);
			dpoint_copy(&p2, &((lpGEO_ARC)(&fr_geo))->ve);
			break;
		case OLINE:
			dpoint_copy(&p1, &((lpGEO_LINE)(&fr_geo))->v1);
			dpoint_copy(&p2, &((lpGEO_LINE)(&fr_geo))->v2);
			break;
	}
	switch(break_obj_p(&p1, type, geo, geo1, type1, geo2, type2)){
		case 2:
			memcpy(geo, geo2, geo_size[*type2]);
			type = *type2;
			ke = 1;
			break;
		case 1:
			memcpy(geo, geo1, geo_size[*type1]);
			type = *type1;
		case 0:
			break;
	}
	if(break_obj_p(&p2, type, geo, &fr_geo, &fr_type, geo2, type2) == 2){
		ke++;
		if(ke == 1){
			memcpy(geo1, geo2, geo_size[*type2]);
			*type1 = *type2;
		}
	}
	return ke;
}

void fragment_points_2(lpD_POINT p){
//     

lpGEO_ARC geo[2] = {&el_geo1, &el_geo2};
OBJTYPE   type[2];
short       i, j;

	if(fr_type == OCIRCLE){
    fragment_points(p, 2);
		return;
	}
	for(i = 0; i < 2; i++){
		if(break_obj_p(&p[i], fr_type, &fr_geo, geo[0], &type[0],
		                                        geo[1], &type[1]) == 2){
			j = test_in_pe_3d(TRUE, &p[1 - i], type[0], geo[0], TRUE) ? 0 : 1;
			memcpy(&fr_geo, geo[j], geo_size[type[j]]);
			fr_type = type[j];
			fr_mod = TRUE;
		}
	}
}
