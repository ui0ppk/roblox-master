#include "../sg.h"

//       

static GEO_ARC 	geo_arc;								//  . /
static ARC_TYPE geo_flag = ECS_NOT_DEF;	//    ARC_TYPE
static MATR    	ecs_gcs_arc;						// .   .  
static MATR		 	gcs_ecs_arc;						// .     .

//---    /
void get_curr_ecs_arc(lpGEO_ARC arc, lpARC_TYPE flag)
{

	*flag = geo_flag;
	if (geo_flag == ECS_NOT_DEF) return;
	memcpy(arc, &geo_arc,
					 ((geo_flag) ? sizeof(GEO_ARC) : sizeof(GEO_CIRCLE)));
}
/*---   /

	arc  -    /
	flag -     
*/
void init_ecs_arc(lpGEO_ARC arc, ARC_TYPE flag)
{
	D_POINT p;
	int			l;

	if (flag == ECS_NOT_DEF) return;
	p.x = - arc->vc.x;
	p.y = - arc->vc.y;
	p.z = - arc->vc.z;
	o_hcunit(gcs_ecs_arc);
	o_tdtran(gcs_ecs_arc, &p);
	o_hcrot1(gcs_ecs_arc, &arc->n);
	o_minv(gcs_ecs_arc, ecs_gcs_arc);
	l = (flag == ECS_ARC) ? sizeof(GEO_ARC) : sizeof(GEO_CIRCLE);
	memcpy(&geo_arc, arc, l);
	geo_flag = flag;
	if (flag == ECS_CIRCLE) {
		geo_arc.ab    = 0;
		geo_arc.angle = 2 * M_PI;
		p.x = arc->r;
		p.y = p.z = 0;
		ecs_to_gcs(&p, &geo_arc.vb);
		memcpy(&geo_arc.ve, &geo_arc.vb, sizeof(D_POINT));
	}
}

//---     .  
void ecs_to_gcs(lpD_POINT from, lpD_POINT to)
{
	o_hcncrd(ecs_gcs_arc, from, to);
}

//---      .
void gcs_to_ecs(lpD_POINT from, lpD_POINT to)
{
	o_hcncrd(gcs_ecs_arc, from, to);
}

//---  .  ./
void get_point_on_arc(sgFloat alpha, lpD_POINT p)
{
	sgFloat 	an;
	D_POINT	tmp;

	if (alpha <= 0.)
		memcpy(p, &geo_arc.vb, sizeof(D_POINT));
	else if (alpha >= 1.)
		memcpy(p, &geo_arc.ve, sizeof(D_POINT));
	else {
		an = geo_arc.ab + alpha * geo_arc.angle;
		tmp.x = geo_arc.r * cos(an);
		tmp.y = geo_arc.r * sin(an);
		tmp.z = 0;
		ecs_to_gcs(&tmp, p);
	}
}

//---     ,   
CODE_MSG arc_init(ARC_INIT_REGIM regim, lpD_POINT p, lpGEO_ARC arc)
{
	D_POINT   pp;
	sgFloat		dist;
	CODE_MSG	code;

	switch (regim) {
		case ARC_CENTER:
			memcpy(&arc->vc, p, sizeof(D_POINT));
			break;
		case ARC_BEGIN:
			if ((code = circle_init((lpGEO_CIRCLE)arc, p)) != EMPTY_MSG)
				return code;
			memcpy(&arc->vb, p, sizeof(D_POINT));
			break;
		case ARC_END:
			if (fabs(dskalar_product(&arc->n, dpoint_sub(p, &arc->vc, &pp))
							) > eps_d) return POINT_NOT_ON_CIRCLE_PLANE;
			if (fabs(dist = dpoint_distance(&arc->vc, p)) < eps_d) {
				return ARC_ANGLE_ZERO;
			}
			dpoint_parametr(&arc->vc, p, arc->r / dist, &arc->ve);
			calc_arc_angle(arc);
//			if (dskalar_product(&arc->n,&curr_w3->from) < 0.)
//				arc->angle -= 2 * M_PI;
			break;
	}
	return EMPTY_MSG;
}

/*---      , , ,
			  
*/
void calc_arc_ab(lpGEO_ARC arc)
{
	D_POINT v1;

	init_ecs_arc(arc,ECS_ARC);
	gcs_to_ecs(&arc->vb,&v1);
	arc->ab = atan2(v1.y, v1.x);
	if (arc->ab < 0.) arc->ab += 2 * M_PI;
}

void calc_arc_angle(lpGEO_ARC arc)
{
	D_POINT pp;

	calc_arc_ab(arc);
	gcs_to_ecs(&arc->ve,&pp);
	arc->angle = atan2(pp.y, pp.x);
	if (arc->angle < 0.) arc->angle += 2 * M_PI;
	if (arc->angle < arc->ab) arc->angle += 2 * M_PI;
	arc->angle -= arc->ab;
}

//---       
extern BOOL super_flag;
extern D_POINT super_normal, super_origin;
void normal_cpl(lpD_POINT n)
{
	if (super_flag) {
		*n = super_normal;
	} else {
		DA_POINT p1 = {0,0,0};
		D_POINT  p0 = {0,0,0}, pp0, pp1;
		int	axis = get_index_invalid_axis();

		p1[axis] = 1;
		dpoint_sub(&pp1, &pp0, n);
	}
//	cpl_vport[curr_vp].n = *n;
}

/*---        
	  ..  circle->vc
	p -   
	  EMPTY_MSG,   cicrle->r  circle->n
														    
	  !
*/
CODE_MSG circle_init(lpGEO_CIRCLE circle, lpD_POINT p)
{
	D_POINT pp;

	normal_cpl(&circle->n);
	circle->r = 0;
	if (fabs(dskalar_product(&circle->n, dpoint_sub(p, &circle->vc, &pp))
					) > eps_d) return POINT_NOT_ON_CIRCLE_PLANE;
	circle->r = dpoint_distance(p, &circle->vc);
	if (circle->r < eps_d) return RADIUS_ZERO;
	return EMPTY_MSG;
}
