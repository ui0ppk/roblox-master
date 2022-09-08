#include "../sg.h"

EL_OBJTYPE last_geo_type = ENULL;
D_POINT    last_geo_p[4];


BOOL set_cont_geo(void * geo, OBJTYPE type, lpD_POINT vp)
//     
{
lpGEO_SPLINE  geo_sply;
D_POINT       p[2];
lpD_POINT     lp;
lpGEO_LINE    line;
lpGEO_ARC     arc;
SPLY_DAT      spl;
BOOL          direct;

	line      = (lpGEO_LINE)geo;
	arc       = (lpGEO_ARC)geo;
	geo_sply  = (lpGEO_SPLINE)geo;

	switch(type){
		case OLINE:
			lp = &line->v1;
			break;
		case OARC:
			lp = &arc->vb;
			break;
		case OSPLINE:
			if(!begin_use_sply(geo_sply, &spl)){
//				o_find_post_processor(&serch_finds);
				return FALSE;
			}
			dpoint_copy(&p[0], (lpD_POINT)&spl.knots[0]);
			dpoint_copy(&p[1], (lpD_POINT)&spl.knots[spl.numk - 1]);
			lp = p;
			break;
	}

	//   - 
	direct = (!num_nearest_to_vp(vp, lp, 2)) ? TRUE : FALSE;

	if(type == OSPLINE){
		set_last_geo(type, &spl, direct);
		end_use_sply(geo_sply, &spl);
	}
	else
		set_last_geo(type, geo, direct);
//	o_find_post_processor(&serch_finds);
	return TRUE;
}

void set_last_geo(OBJTYPE type, void * geo, BOOL direct){
//     ( )
D_POINT    p[2];
D_POINT    v[2];
short        ib = 0, ie = 1;

	get_geo_endpoints_and_vectors(type, geo, p, v);
	dpoint_add(&p[1], &v[1], &v[1]);
	dpoint_sub(&p[0], &v[0], &v[0]);
	if(direct) { ib = 1; ie = 0; }

	last_geo_p[0] = p[ie]; last_geo_p[1] = v[ie];
	last_geo_p[2] = p[ib]; last_geo_p[3] = v[ib];

	last_geo_type = get_el_type(type);
}

void get_geo_endpoints_and_vectors(OBJTYPE type, void * geo,
																	 lpD_POINT p, lpD_POINT v)
{
D_POINT       wp;
lpGEO_ARC     arc;
lpGEO_LINE    line;
lpGEO_CIRCLE  circle;
lpSPLY_DAT    spl;
short           i, k;

	switch (type) {
		case OARC:
			arc = (lpGEO_ARC)geo;
			p[0] = arc->vb; p[1] = arc->ve;
			for (i = 0; i < 2; i++) {
				dvector_product(&arc->n, dpoint_sub(&p[i], &arc->vc, &wp), &v[i]);
				if(arc->angle < 0.)
					dpoint_inv(&v[i], &v[i]);
			}
			break;
		case OCIRCLE:
			circle = (lpGEO_CIRCLE)geo;
			init_ecs_arc((lpGEO_ARC)circle, ECS_CIRCLE);
			get_point_on_arc(0., &p[0]);
			dvector_product(&circle->n, dpoint_sub(&p[0], &circle->vc, &wp), &v[0]);
			p[1] = p[0];
			v[1] = v[0];
			break;
		case OLINE:
			line = (lpGEO_LINE)geo;
			dpoint_sub(&line->v2, &line->v1, v);
			p[0] = line->v1; p[1] = line->v2;
			v[1] = v[0];
			break;
		case OSPLINE:
			spl = (lpSPLY_DAT)geo;

			for(i = 0, k = 0; i < 2; i++, k += (spl->numk -1)){
				dpoint_copy(&p[i], (lpD_POINT)&spl->knots[k]);
				get_point_on_sply( spl, i, &v[i], 1);
			}
			break;
		default:
			put_message(INTERNAL_ERROR,GetIDS(IDS_SG038),0);
			break;
	}
}

void pereor_geo(OBJTYPE type, void * geo){
//  
D_POINT p;
lpGEO_ARC  arc;
lpGEO_LINE line;

	switch(type){
		case OARC:
			arc = (lpGEO_ARC)geo;
			dpoint_copy(&p, &arc->ve);
			dpoint_copy(&arc->ve, &arc->vb);
			dpoint_copy(&arc->vb, &p);
			arc->ab += arc->angle;
			arc->angle = -arc->angle;
			break;
		case OLINE:
			line = (lpGEO_LINE)geo;
			dpoint_copy(&p, &line->v2);
			dpoint_copy(&line->v2, &line->v1);
			dpoint_copy(&line->v1, &p);
			break;
	}
}
