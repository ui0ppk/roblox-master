#include "../sg.h"

static void save_min_var_l(sgFloat dist, sgFloat* mindist, lpD_POINT cd,
													 lpD_POINT tp);
static BOOL line_t_t_1(lpD_POINT a, short* sgn, lpD_POINT lv);

BOOL line_t_t(BOOL vflag, lpD_POINT a, lpD_POINT vp, lpD_POINT tp){

	if(((vflag) ? line_t_v_t(a, vp, tp) : line_t_p_t(a, vp, tp))){
		if(!dpoint_eq(&tp[0], &tp[1], eps_d))
			return TRUE;
		el_geo_err = NULL_LINE_LENGTH;
	}
	else
		el_geo_err = TANG_NO_CREATE;
	return FALSE;
}

BOOL line_t_p_t(lpD_POINT a, lpD_POINT vp, lpD_POINT tp){

short      i, sgn[2];
sgFloat   dist, mindist = -1.;
D_POINT  lv[2];

	if(dpoint_distance_2d(&a[1], &a[0]) < eps_d) return FALSE;
	for(sgn[0] = -1; sgn[0] < 2; sgn[0] += 2)
		for(sgn[1] = -1; sgn[1] < 2; sgn[1] += 2){
			if(!line_t_t_1(a, sgn, lv)) continue;
			dist = 0.;
			for(i = 0; i < 2; i++)
				dist += dpoint_distance_2d(&vp[i], &lv[i]);
			save_min_var_l(dist, &mindist, lv, tp);
		}
	return (mindist >= 0.) ? TRUE : FALSE;
}

BOOL line_t_v_t(lpD_POINT a, lpD_POINT vp, lpD_POINT tp){

short      sgn[2];
sgFloat   mindist = -1.;
D_POINT  lv[2], wp[2], cp;

	if(dpoint_distance_2d(&a[1], &a[0]) < eps_d) return FALSE;
	for(sgn[0] = -1; sgn[0] < 2; sgn[0] += 2)
		for(sgn[1] = -1; sgn[1] < 2; sgn[1] += 2){
			if(!line_t_t_1(a, sgn, lv)) continue;
			dpoint_copy(&cp, &a[0]); cp.z = 0;
			dpoint_sub(&lv[0], &cp,    &wp[0]);
			dpoint_sub(&lv[1], &lv[0], &wp[1]);
			if(vp[0].z*(wp[0].x*wp[1].y - wp[0].y*wp[1].x) < 0.) continue;
			save_min_var_l(dpoint_distance_2d(&vp[1], &lv[1]), &mindist, lv, tp);
		}
	return (mindist >= 0.) ? TRUE : FALSE;
}

void set_tline_sign(lpD_POINT a, lpD_POINT vp){

D_POINT  wp[2], cp;

	dpoint_copy(&cp, a); cp.z = 0.;
	dpoint_sub(&vp[0], &cp,    &wp[0]);
	dpoint_sub(&vp[1], &vp[0], &wp[1]);
	vp[0].z = wp[0].x*wp[1].y - wp[0].y*wp[1].x;
}

static void save_min_var_l(sgFloat dist, sgFloat* mindist, lpD_POINT cd,
													lpD_POINT tp){

short i;
	if((*mindist >= 0.) && (dist > *mindist))return;
	*mindist = dist;
	for(i = 0; i < 2; i++)
		dpoint_copy(&tp[i], &cd[i]);
}

static BOOL line_t_t_1(lpD_POINT a, short* sgn, lpD_POINT lv){

short      i;
sgFloat   c, d, t, c1, c2, r[2];
D_POINT  p;

	dpoint_sub(&a[1], &a[0], &p);
	d = p.x*p.x + p.y*p.y;
	for(i = 0; i < 2; i++)
	  r[i] = a[i].z*sgn[i];
	c = r[0] - r[1];
	if(fabs(t = d - c*c) < eps_d*eps_d) t = 0.;
	if(t < 0.) return FALSE;
	t = sqrt(t);
	c1 = c*p.x + t*p.y;
	c2 = c*p.y - t*p.x;
	for(i = 0; i < 2; i++){
		lv[i].x = a[i].x + (r[i]*c1)/d;
		lv[i].y = a[i].y + (r[i]*c2)/d;
		lv[i].z = 0.;
	}
	return TRUE;
}
