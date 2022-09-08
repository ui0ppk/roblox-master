#include "../../sg.h"

short o_init_ecs_arc(lpARC_DATA ad,lpGEO_ARC arc, OARC_TYPE flag, sgFloat h)
{
	D_POINT p;
	int			l;
  sgFloat 	alfa;

	if (flag == OECS_NOT_DEF) return 0;
	p.x = - arc->vc.x;
	p.y = - arc->vc.y;
	p.z = - arc->vc.z;
	o_hcunit(ad->gcs_ecs_arc);
	o_tdtran(ad->gcs_ecs_arc, &p);
	o_hcrot1(ad->gcs_ecs_arc, &arc->n);
	o_minv(ad->gcs_ecs_arc, ad->ecs_gcs_arc);
	l = (flag == OECS_ARC) ? sizeof(GEO_ARC) : sizeof(GEO_CIRCLE);
	memcpy(&ad->geo_arc, arc, l);
	if (flag == OECS_CIRCLE) {
		ad->geo_arc.ab    = 0;
		ad->geo_arc.angle = 2 * M_PI;
		p.x = arc->r;
		p.y = p.z = 0;
    o_ecs_to_gcs(ad, &p, &ad->geo_arc.vb);
		ad->geo_arc.ve = ad->geo_arc.vb;
	}
	if ( h > ad->geo_arc.r/2) alfa = M_PI / 3;
	else alfa = 2*asin(sqrt( h*( 2*ad->geo_arc.r-h))/ad->geo_arc.r);
	l = (int)abs((int)(ad->geo_arc.angle / alfa)) + 1;
	if ( l < MIN_NUM_SEGMENTS) l = MIN_NUM_SEGMENTS;
	//if ( l > NUM_SEGMENTS) l = NUM_SEGMENTS;
	return NUM_SEGMENTS;//*/l;
}


void o_ecs_to_gcs(lpARC_DATA ad, lpD_POINT from, lpD_POINT to)
{
	o_hcncrd(ad->ecs_gcs_arc, from, to);
}

void o_get_point_on_arc(lpARC_DATA ad,sgFloat alpha, lpD_POINT p)
{
	sgFloat 	an;
	D_POINT	tmp;

	if (alpha <= 0.) 	*p = ad->geo_arc.vb;
	else {
		if (alpha >= 1.) *p = ad->geo_arc.ve;
		else {
			an = ad->geo_arc.ab + alpha * ad->geo_arc.angle;
			tmp.x = ad->geo_arc.r * cos(an);
			tmp.y = ad->geo_arc.r * sin(an);
			tmp.z = 0;
			o_ecs_to_gcs(ad,&tmp, p);
		}
	}
}
void o_trans_arc(lpGEO_ARC p, lpMATR matr, OARC_TYPE flag)
{
	D_POINT				pn;

	if ( flag != OECS_ARC ) {
		pn = p->vc;
		pn.x += p->r;
	}
	if ( !o_trans_vector(matr,&p->vc, &p->n) ) return;
  if ( !dnormal_vector(&p->n) )              return;

	if ( flag == OECS_ARC ) {
		MATR          gcs_ecs;

		o_hcunit(gcs_ecs);
		o_hcncrd(matr,&p->vb,&p->vb);
		p->r = dpoint_distance(&p->vc, &p->vb);
		o_hcncrd(matr,&p->ve,&p->ve);
		o_tdtran(gcs_ecs,dpoint_inv(&p->vc,&pn));
		o_hcrot1(gcs_ecs,&p->n);
		o_hcncrd(gcs_ecs,&p->vb,&pn);
		p->ab = atan2(pn.y, pn.x);
		if (p->ab < 0.) p->ab += 2 * M_PI;
	} else {
		o_hcncrd(matr,&pn,&pn);
		p->r = dpoint_distance(&p->vc, &pn);
	}
}

