#include "../sg.h"

static void save_min_var(sgFloat dist, sgFloat x, sgFloat y, sgFloat r,
												 sgFloat* mindist,
												 lpD_POINT cd, short num, lpD_POINT cp, lpD_POINT tp);

static BOOL trans_min_var(sgFloat mindist, lpD_POINT cp, lpD_POINT tp);

BOOL arc_b_e_c(lpD_POINT bp, lpD_POINT ep, lpD_POINT cp,
							 BOOL cend, BOOL cradius, BOOL negativ,
							 lpGEO_ARC arc){
//   2D   ,   
sgFloat    d;
D_POINT   wep;
lpD_POINT aep;

	if(!cradius)  //   
		if((arc->r = dpoint_distance_2d(cp, bp)) < eps_d){
			el_geo_err = ARC_NULL_RAD;
			return FALSE;
		}
	if(!cend){ //    
		if((d = dpoint_distance_2d(cp, ep)) < eps_d){
			el_geo_err = BAD_ARC_POINTS;
			return FALSE;
		}
		aep = &wep;
		dpoint_parametr(cp, ep, arc->r/d, aep);
	}
	else
		aep = ep;
	dpoint_copy(&arc->vc, cp);
	dpoint_copy(&arc->vb, bp);
	arc->ab = v_angle(bp->x - cp->x, bp->y - cp->y);
	//    
	if(dpoint_eq(bp, aep, eps_d)){
		dpoint_copy(&arc->ve, bp);
		arc->angle = 2*M_PI;
	}
	else{
		dpoint_copy(&arc->ve, aep);
		arc->angle = v_angle(aep->x - cp->x, aep->y - cp->y);
		if(arc->angle < arc->ab) arc->angle += 2*M_PI;
		arc->angle -= arc->ab;
		if(negativ)
			arc->angle -= 2*M_PI;
	}
	return TRUE;
}


void circle_c_bt(lpD_POINT cp, lpD_POINT bcond, lpD_POINT vp,
								 OBJTYPE type, lpD_POINT bp){

sgFloat  r, s, c;
D_POINT wp;

	bp->z = 0;
	switch(type){
		case OLINE:
			r = bcond->x*cp->x + bcond->y*cp->y + bcond->z;
			bp->x = cp->x - r*bcond->x; bp->y = cp->y - r*bcond->y;
			break;
		case OARC:
		case OCIRCLE:
			if((r = hypot(c = cp->x - bcond->x, s = cp->y - bcond->y)) < eps_d)
				dpoint_copy(bp, vp);
			else{
				c = bcond->z*(c/r); s = bcond->z*(s/r);
				bp->x = bcond->x + c; bp->y = bcond->y + s;
				wp.x =  bcond->x - c; wp.y  = bcond->y - s; wp.z = 0;
				if(dpoint_distance_2d(vp, bp) > dpoint_distance_2d(vp, &wp))
					dpoint_copy(bp, &wp);
			}
	}
}
BOOL arc_p_p_r(short ib, short ie, sgFloat r, lpD_POINT p, lpD_POINT vp,
               lpOBJTYPE type, BOOL negativ, lpGEO_ARC arc){
 

D_POINT   cp, wp[2];

	if(!type) //  
    return arc_b_e_r(&p[ib], &p[ie], r, negativ, arc);

	if(r < 0.){
		negativ = TRUE;
		r = -r;
	}
  if(!circle_g_g_r(type, p, r, vp, wp, &cp))
		return FALSE;
	arc->r = cp.z;
	cp.z = 0.;
	return arc_b_e_c(&wp[ib], &wp[ie], &cp, TRUE, TRUE, negativ, arc);
}

BOOL arc_b_c_a(lpD_POINT bp, lpD_POINT cp, sgFloat angle, lpGEO_ARC arc){
//   2D   ,    
sgFloat    da;

	if((arc->r = dpoint_distance_2d(cp, bp)) < eps_d){
			el_geo_err = ARC_NULL_RAD;
			return FALSE;
	}
	dpoint_copy(&arc->vc, cp);
	dpoint_copy(&arc->vb, bp);
	arc->ab = v_angle(bp->x - cp->x, bp->y - cp->y);
	da = 2*M_PI;
	if     (angle >  da) arc->angle = da;
	else if(angle < -da) arc->angle = -da;
	else                 arc->angle = angle;
	da = arc->ab + arc->angle;
	arc->ve.x = cp->x + arc->r*cos(da);
	arc->ve.y = cp->y + arc->r*sin(da);
	arc->ve.z = 0.;
	if(fabs(arc->angle) < M_PI/10. && dpoint_distance_2d(bp, &arc->ve) < eps_d){
		el_geo_err = ARC_ANGLE_ZERO;
		return FALSE;
	}
	return TRUE;
}

BOOL arc_b_e_pa(lpD_POINT bp, lpD_POINT ep, lpD_POINT ap, BOOL negativ,
							  lpGEO_ARC arc){
  
sgFloat    a, b, c, a1, b1, c1;
D_POINT   cp;
lpD_POINT p;

	if(eq_pe_line(bp, ep, &a, &b, &c)){
	  p = (dpoint_distance_2d(ep, ap) < dpoint_distance_2d(bp, ap)) ? ep : bp;
		if(eq_line(p, ap, &a1, &b1, &c1)){
			if(p_int_lines(a, b, c, a1, b1, c1, &cp)){
				if(!arc_b_e_c(bp, ep, &cp, TRUE, FALSE, negativ, arc))
					return FALSE;
				return TRUE;
			}
		}
	}
	el_geo_err = BAD_ARC_POINTS;
	return FALSE;
}

BOOL arc_b_e_a(lpD_POINT bp, lpD_POINT ep, sgFloat angle, lpGEO_ARC arc){
    
sgFloat    a, b, c, a1, b1 = M_PI, c1;
D_POINT   cp;

	if(fabs(angle) > 2*M_PI*(1. - eps_n) || fabs(angle) < 2*M_PI*eps_n){
met_err:
	  el_geo_err = BAD_ARC_ANGLE;
	  return FALSE;
	}
	if(eq_pe_line(bp, ep, &a, &b, &c)){
		if(angle < 0.) b1 = -b1;
		a1 = v_angle(ep->x - bp->x, ep->y - bp->y) + (b1 - angle)/2.;
		c1 = dpoint_distance_2d(bp, ep);
		cp.x = bp->x + c1*cos(a1); cp.y = bp->y + c1*sin(a1);  cp.z = 0.;
		if(eq_line(bp, &cp, &a1, &b1, &c1)){
			if(!p_int_lines(a, b, c, a1, b1, c1, &cp))
				goto met_err;
			if(!arc_b_c_a(bp, &cp, angle, arc))
				return FALSE;
			return TRUE;
		}
	}
	el_geo_err = BAD_ARC_POINTS;
	return FALSE;
}

BOOL arc_b_c_h(lpD_POINT bp, lpD_POINT cp, sgFloat h, BOOL negativ,
               lpGEO_ARC arc){
  
sgFloat  r, a, b, c;
D_POINT ep[2];
short     i;
BOOL    large = FALSE;

	if(h < 0.){
		large = TRUE;
		h = -h;
	}
	if((r = dpoint_distance_2d(bp, cp)) < eps_d){
	  el_geo_err = BAD_ARC_POINTS;
		return FALSE;
	}
	arc->r = r;

	if(h < eps_d) //  
	  return arc_b_e_c(bp, bp, cp, TRUE, TRUE, negativ, arc);

	switch(intersect_2d_cc(cp->x, cp->y, r, bp->x, bp->y, h, ep)){
		case 0:
		  el_geo_err = BAD_ARC_HLEGTH;
			return FALSE;
		case 1:
			i = 0;
			break;
		case 2:
	    eq_line(cp, bp, &a, &b, &c);
		  i = (ep[0].x*a + ep[0].y*b + c > 0.) ? 0 : 1;
		  if(negativ) i = 1 - i;
		  if(large)   i = 1 - i;
			break;
	}
	return arc_b_e_c(bp, &ep[i], cp, TRUE, TRUE, negativ, arc);
}

BOOL arc_b_e_r(lpD_POINT bp, lpD_POINT ep, sgFloat r, BOOL negativ,
               lpGEO_ARC arc){
 
sgFloat  d;
D_POINT cp, c;
short     left;
BOOL    large = FALSE;

	if(r < 0.){
		large = TRUE;
		r = -r;
	}
	d = dpoint_distance_2d(bp, ep)/2;
	if(r - d < 0.){
		r = d;
		d = 0.;
	}
	else
	  d = sqrt(r*r - d*d);
	if(!eq_line(bp, ep, &c.x, &c.y, &c.z)){
	  el_geo_err = BAD_ARC_POINTS;
		return FALSE;
	}
	c.z = 0.;
	left = ((large)?-1:1)*((negativ)?-1:1);
  dpoint_parametr(bp, ep, 0.5, &cp);
	dpoint_scal(&c, d, &c);
	if(left == -1)
		dpoint_inv(&c, &c);
	dpoint_add(&cp, &c, &cp);
	arc->r = r;
	return arc_b_e_c(bp, ep, &cp, TRUE, TRUE, negativ, arc);
}

short intersect_two_2d_obj(lpOBJTYPE type, lpD_POINT g, lpD_POINT p)
{
short il[2], ic[2], nl = 0, nc = 0, i;

	for(i = 0; i < 2; i++)
		if(type[i] == OLINE) il[nl++] = i;
		else                 ic[nc++] = i;

	switch (nl) {
		case 2:
			return (p_int_lines(g[0].x, g[0].y, g[0].z,
													g[1].x, g[1].y, g[1].z, p)) ? 1 : 0;
		case 1:
			return intersect_2d_lc(g[il[0]].x, g[il[0]].y, g[il[0]].z,
														 g[ic[0]].x, g[ic[0]].y, g[ic[0]].z, p);
		default: //0
			return intersect_2d_cc(g[0].x, g[0].y, g[0].z, g[1].x, g[1].y, g[1].z,
														 p);
	}
}

BOOL circle_g_g_r(lpOBJTYPE type, lpD_POINT g, sgFloat r, lpD_POINT vp,
									lpD_POINT tp, lpD_POINT cp){

D_POINT cd[2], wp[2], wc[2];
sgFloat  x, y, si, co, dist, dd;
sgFloat  mindist = -1.;
short     i, i1, i2, j, kp, sgn, sg[2];

	if((r = fabs(r))< eps_d){
		el_geo_err = ARC_NULL_RAD;
		return FALSE;
	}
	for(i = 0; i < 2; i ++)
		dpoint_copy(&wp[i], &g[i]);
	for(i = -1; i < 2; i += 2){
		sg[0] = i; wp[0].z = g[0].z + i*r;
		if(type[0] != OLINE) wp[0].z = fabs(wp[0].z);
		for(i1 = -1; i1 < 2; i1 += 2){
			sg[1] = i1; wp[1].z = g[1].z + i1*r;
			if(type[1] != OLINE) wp[1].z = fabs(wp[1].z);
			if(!(kp = intersect_two_2d_obj(type, wp, wc))) continue;
			for(i2 = 0; i2 < kp; i2++){
				x = wc[i2].x;  y = wc[i2].y;
				dist = 0.;
				for(j = 0; j < 2; j++){
					if(type[j] != OLINE){
						co = g[j].x - x; si = g[j].y - y;
						if((dd = hypot(si, co)) < eps_d)goto met_cont;
						sgn = ((dd < g[j].z) && (r < g[j].z)) ? -1: 1;
						cd[j].x = x + sgn*r*(co/dd); cd[j].y = y + sgn*r*(si/dd);
					}
					else{
						cd[j].x = x + sg[j]*g[j].x*r; cd[j].y = y + sg[j]*g[j].y*r;
					}
					cd[j].z = 0.;
					dist += dpoint_distance_2d(&vp[j], &cd[j]);
				}
				save_min_var(dist, x, y, r, &mindist, cd, 2, cp, tp);
met_cont:;
			}
		}
	}
	if(mindist < 0.){
		el_geo_err = TANG_NO_CREATE;
		return FALSE;
	}
	return TRUE;
}

BOOL arc_b_e_v(lpD_POINT bp, lpD_POINT ep, lpD_POINT vp,
							 BOOL negativ, lpGEO_ARC arc){

sgFloat  a, b, c, a1, b1, c1, c2;
D_POINT cp;

	if(eq_pe_line(bp, ep, &a, &b, &c))
		if(eq_line(bp, vp, &a1, &b1, &c1)){
			c2 = a1*bp->y - b1*bp->x;
			if(p_int_lines(a, b, c, b1, -a1, c2, &cp)){
				if(!arc_b_e_c(bp, ep, &cp, TRUE, FALSE, FALSE, arc))
					return FALSE;
				//    
				a = a1*ep->x + b1*ep->y + c1;
				if(negativ)
					a = -a;
				if(a < 0.)
					arc->angle -= 2*M_PI;
				return TRUE;
			}
		}
	el_geo_err = BAD_ARC_POINTS;
	return FALSE;
}

BOOL arc_b_e_vt(lpD_POINT bp, OBJTYPE type, lpD_POINT te, lpD_POINT sp,
								lpD_POINT vp, BOOL negativ, lpGEO_ARC arc){


sgFloat  r, d, rad, s, c, y[2];
short     i, sign;
short     num = 0;
D_POINT cp = {0., 0., 0.};
D_POINT we, ws, ep[2];

	if((r = hypot(vp->x, vp->y)) < eps_d){
		el_geo_err = BAD_ARC_POINTS;
		return FALSE;
	}
	faf_init();
	faf_set(FAF_ROTATE, -vp->y/r, vp->x/r);
	faf_point(sp, &ws);  //   
	if(type == OLINE)    //  
		faf_line(te, &we);
	else{
		faf_point(te, &we);
		we.z = 0.;
		r = (we.x*we.x + we.y*we.y - te->z*te->z)/2.;
	}
	for(i = -1; i < 2; i += 2){
		sign = 1;
		switch(type){
			case OLINE:
				if(fabs(d = we.y + i) < eps_n)continue;
				y[num] = -we.z/d;
				c = we.x;
				s = we.y;
				if(we.y*y[num] + we.z > 0.)
					sign = -1;
				break;
			case OARC:
			case OCIRCLE:
				if(fabs(d = we.y + i*te->z) < eps_d)continue;
				y[num] = r/d;
				if((d = hypot(we.x, we.y - y[num])) < eps_d)continue;
				c = we.x/d;
				s = (we.y - y[num])/d;
				if((d < te->z) && (fabs(y[num]) < te->z))
					sign = -1;
				break;
		}
		if((rad = fabs(y[num])) < eps_d)continue;
		ep[num].x = sign*rad*c;
		ep[num].y = y[num] + sign*rad*s;
		ep[num++].z = 0.;
	}
	switch(--num){
		case -1: //     
			el_geo_err = TANG_NO_CREATE;
			return FALSE;
		case 1:
			if(dpoint_distance_2d(&ws, &ep[0]) < dpoint_distance_2d(&ws, &ep[1]))
				num = 0;
		default:
			break;
	}
	cp.y = y[num];
	faf_inv(&cp, &cp);
	faf_inv(&ep[num], &ep[num]);
	arc->r = fabs(y[num]);
	if(!arc_b_e_c(bp, &ep[num], &cp, TRUE, TRUE, FALSE, arc))
		return FALSE;
	//    
	if(y[num] < 0.) negativ = !negativ;
	if(negativ)
		arc->angle -= 2*M_PI;
	return TRUE;
}


BOOL arc_p_p_p(short ib, short ie, short im, lpD_POINT p, lpD_POINT vp,
               lpOBJTYPE type, BOOL negativ, lpGEO_ARC arc){

sgFloat    a, b, c;
D_POINT   cp, wp[3];
lpD_POINT ap;
short       ic[3] = {-1, -1, -1}, il[3] = {-1, -1, -1}, nc = 0, nl = 0, i;
BOOL      rez;

	if(!type){ //  
		if(!circle_b_e_m(p, &cp))
			return FALSE;
		ap = p;
	}
	else{ //   
		for(i = 0; i < 3; i++)
			if(type[i] == OLINE) il[nl++] = i;
			else                 ic[nc++] = i;
		switch(nl){
			case 0:
		    rez = circle_c_c_c(p, vp, wp, &cp);
				break;
			case 1:
				rez = circle_c_c_l(ic[0], ic[1], il[0], p, vp, wp, &cp);
				break;
			case 2:
		    rez = circle_c_l_l(ic[0], il[0], il[1], p, vp, wp, &cp);
				break;
			case 3:
		    rez = circle_l_l_l(p, vp, wp, &cp);
				break;
		}
		if(!rez) return FALSE;
		ap = wp;
	}
	arc->r = cp.z;
	cp.z = 0.;
	if(!arc_b_e_c(&ap[ib], &ap[ie], &cp, TRUE, TRUE, FALSE, arc))
		return FALSE;
	//    
	if(eq_line(&ap[ib], &ap[ie], &a, &b, &c)){
		if(a*ap[im].x + b*ap[im].y + c > 0.)
			negativ = !negativ;
		if(negativ)
			arc->angle -= 2*M_PI;
		return TRUE;
	}
	if(negativ){
		el_geo_err = ARC_ANGLE_ZERO;
		return FALSE;
	}
	return TRUE;
}

BOOL circle_b_e_m(lpD_POINT p, lpD_POINT cp){

sgFloat  a, b, c, a1, b1, c1;

	if(eq_pe_line(&p[0], &p[1], &a, &b, &c))
		if(eq_pe_line(&p[1], &p[2], &a1, &b1, &c1))
			if(p_int_lines(a, b, c, a1, b1, c1, cp)){
				if((cp->z = dpoint_distance_2d(cp, &p[0])) < eps_d){
			    el_geo_err = ARC_NULL_RAD;
			    return FALSE;
		    }
				return TRUE;
			}
	el_geo_err = BAD_ARC_POINTS;
	return FALSE;
}

BOOL circle_c_c_c(lpD_POINT p, lpD_POINT vp, lpD_POINT tp, lpD_POINT cp){
D_POINT cd[3], wp[6];
sgFloat  rc[2], cs, a1, b1, c1, a2, b2, c2, si, co, x, y, r, dist, dd, q3;
sgFloat  q1 = 1.;
sgFloat  q2 = 0.;
sgFloat  mindist = -1.;
short     i, i1, i2, j, k, sgn, place;

	faf_init();
	//     0
	faf_set(FAF_MOVE, -p[0].x, -p[0].y);
	for(i = 0; i < 3; i++){
		faf_point(&p[i], &wp[i]);     //  
		faf_point(&vp[i], &wp[i+3]);  //  
	}
	b1 = ((wp[0].z + wp[1].z)*(wp[0].z - wp[1].z))/2;
	b2 = ((wp[0].z + wp[2].z)*(wp[0].z - wp[2].z))/2;
	// ,       X
	if((r = hypot(wp[1].x, wp[1].y)) < eps_d){
		if((r = hypot(wp[2].x, wp[2].y)) < eps_d)goto met_err;
		faf_set(FAF_ROTATE, -wp[2].y/r, wp[2].x/r);
		faf_point(&wp[2], &wp[2]);    //  3 
		place = 0;
	}
	else{
		faf_set(FAF_ROTATE, -wp[1].y/r, wp[1].x/r);
		for(i = 1; i < 3; i++)
			faf_point(&wp[i], &wp[i]);    //  2  3 
		if(fabs(wp[2].y) < eps_d){
			b1 = wp[1].x*wp[1].x/2 + b1;
			b2 = wp[2].x*wp[2].x/2 + b2;
			a1 = wp[1].x*b2 - wp[2].x*b1;
			place = 2;
		}
		else{
			b1 = wp[1].x/2 + b1/wp[1].x;
			b2 = wp[2].y/2 + (wp[2].x*wp[2].x/2 + b2 - b1*wp[2].x)/wp[2].y;
			place = 1;
		}
	}

	for(i = 0; i < 3; i++)
		faf_point(&wp[i+3], &wp[i+3]);  //   

	for(i = -1; i < 2; i += 2){
		cs = i*wp[0].z;
		for(i1 = -1; i1 < 2; i1 += 2){
			c1 = cs - i1*wp[1].z;
			switch(place){
				case 0: //   
					c1 = - cs - i1*wp[1].z;
					if((r = c1/2) < eps_d) continue;
					break;
				case 1: //  
					a1 = c1/wp[1].x;
				default:
					break;
			}
			for(i2 = -1; i2 < 2; i2 += 2){
				c2 = cs - i2*wp[2].z;
				switch(place){
					case 0: //   
						x = wp[2].x/2 + (r*c2 + b2)/wp[2].x;
						goto met2;
					case 1: //  
						a2 = (c2 - a1*wp[2].x)/wp[2].y;
						q1 = a1*a1 + a2*a2 - 1;
						q2 = 2*(a1*b1 +a2*b2 - i*wp[0].z);
						q3 = b1*b1 + b2*b2 - wp[0].z*wp[0].z;
						break;
					case 2: //      
						if(fabs(a2 = wp[2].x*c1 - wp[1].x*c2) < eps_d) continue;
						if((r = a1/a2) < eps_d)continue;
						x = (b2*c1 - b1*c2)/a2;
met2:      	q3 = r + cs; q3 = x*x - q3*q3;
						break;
				}
				if(!(k = qwadr_eq(q1, q2, q3, rc)))continue;
				while(--k >= 0){
					switch(place){
						case 0: //   
						case 2: //     
							y = rc[k];
							break;
						case 1: //  
							if((r = rc[k]) < eps_d)continue;
							x = a1*r + b1;
							y = a2*r + b2;
							break;
					}
					dist = 0.;
					for(j = 0; j < 3; j++){
						co = wp[j].x - x;
						si = wp[j].y - y;
						if((dd = hypot(si, co)) < eps_d)goto met_cont;
						sgn = ((dd < wp[j].z) && (r < wp[j].z)) ? -1: 1;
						cd[j].x = x + sgn*r*(co/dd);
						cd[j].y = y + sgn*r*(si/dd);
						cd[j].z = 0.;
						dist += dpoint_distance_2d(&wp[j + 3], &cd[j]);
					}
					save_min_var(dist, x, y, r, &mindist, cd, 3, cp, tp);
met_cont:;
				}
			}
		}
	}
  if(trans_min_var(mindist, cp, tp)) return TRUE;
met_err:
	el_geo_err = TANG_NO_CREATE;
	return FALSE;
}

BOOL circle_c_c_l(short c1, short c2, short l,
                  lpD_POINT p, lpD_POINT vp, lpD_POINT tp, lpD_POINT cp){
  
D_POINT cd[3], wp[6];
sgFloat  rc[2], cs, a1, b1, a2, b2, si, co, x, y, r, dist, dd, q3;
sgFloat  q1 = 1.;
sgFloat  q2 = 0.;
sgFloat  mindist = -1.;
short     i, i1, i2, j, k, sgn, place;

	faf_init();
	//     0
	faf_set(FAF_MOVE, -p[c1].x, -p[c1].y);
	for(i = 0; i < 3; i++){
		if(i == l) faf_line (&p[i], &wp[i]);     //  
		else       faf_point(&p[i], &wp[i]);     //  
		faf_point(&vp[i], &wp[i+3]);  //  
	}
	b1 = ((wp[c1].z + wp[c2].z)*(wp[c1].z - wp[c2].z))/2;
	// ,       X
	if((r = hypot(wp[c2].x, wp[c2].y)) < eps_d){ //  
		//     X
		faf_set(FAF_ROTATE, wp[l].x, wp[l].y);
		faf_line(&wp[l], &wp[l]);
		place = 0;
	}
	else{
		faf_set(FAF_ROTATE, -wp[c2].y/r, wp[c2].x/r);
		faf_point(&wp[c2], &wp[c2]);    //  2 
		faf_line (&wp[l], &wp[l]);      //  
		b1 = wp[c2].x/2 + b1/wp[c2].x;
		if(fabs(wp[l].y) < eps_n){
			b2 = -wp[l].z/wp[l].x;
			place = 2;
		}
		else{
			b2 = -(wp[l].z + wp[l].x*b1)/wp[l].y;
			place = 1;
		}
	}

	for(i = 0; i < 3; i++)
		faf_point(&wp[i+3], &wp[i+3]);  //   

	for(i = -1; i < 2; i += 2){
		cs = i*wp[c1].z;
		for(i1 = -1; i1 < 2; i1 += 2){
			switch(place){
				case 0: //   
					if((r = -(cs + i1*wp[c2].z)/2) < eps_d) continue;
					break;
				case 1:
				case 2:
					a1 = (cs - i1*wp[c2].z)/wp[c2].x;
					break;
			}
			for(i2 = -1; i2 < 2; i2 += 2){
				switch(place){
					case 0: //   
						y = (i2*r - wp[l].z)/wp[l].y;
						q3 = r + i*wp[c1].z; q3 = y*y - q3*q3;
						break;
					case 1: //  
						a2 = (i2 - a1*wp[l].x)/wp[l].y;
						q1 = a1*a1 + a2*a2 - 1;
						q2 = 2*(a1*b1 +a2*b2 - i*wp[c1].z);
						q3 = b1*b1 + b2*b2 - wp[c1].z*wp[c1].z;
						break;
					case 2: //    X
						a2 = i2/wp[l].x;
						if(fabs(r = a2 - a1) < eps_n)continue;
						if((r = (b1 - b2)/r) < eps_d)continue;
						x = ((a1 + a2)*r + b1 + b2)/2;
						q3 = r + i*wp[c1].z; q3 = x*x - q3*q3;
						break;
				}
				if(!(k = qwadr_eq(q1, q2, q3, rc)))continue;
				while(--k >= 0){
					switch(place){
						case 0:
							x = rc[k];
							break;
						case 2:
							y = rc[k];
							break;
						case 1:
							if((r = rc[k]) < eps_d)continue;
							x = a1*r + b1;
							y = a2*r + b2;
							break;
					}
					dist = 0.;
					for(j = 0; j < 3; j++){
						if(j != l){
							co = wp[j].x - x; si = wp[j].y - y;
							if((dd = hypot(si, co)) < eps_d)goto met_cont;
							sgn = ((dd < wp[j].z) && (r < wp[j].z)) ? -1: 1;
							cd[j].x = x + sgn*r*(co/dd); cd[j].y = y + sgn*r*(si/dd);
						}
						else{
							cd[j].x = x - i2*wp[l].x*r; cd[j].y = y - i2*wp[l].y*r;
						}
						cd[j].z = 0.;
						dist += dpoint_distance_2d(&wp[j + 3], &cd[j]);
					}
					save_min_var(dist, x, y, r, &mindist, cd, 3, cp, tp);
met_cont:;
				}
			}
		}
	}
	if(trans_min_var(mindist, cp, tp)) return TRUE;
//nb met_err:
	el_geo_err = TANG_NO_CREATE;
	return FALSE;
}


BOOL circle_c_l_l(short ic, short l1, short l2,
									lpD_POINT p, lpD_POINT vp, lpD_POINT tp, lpD_POINT cp){
 
D_POINT cd[3], wp[6];
sgFloat  rc[2], a1, b1, a2, b2, si, co, x, y, r, dist, dd, q3;
sgFloat  q1 = 1.;
sgFloat  q2 = 0.;
sgFloat  mindist = -1.;
short     i, i1, i2, j, k, place;

	faf_init();
	//     0
	faf_set(FAF_MOVE, -p[ic].x, -p[ic].y);
	for(i = 0; i < 3; i++){
		if(i == ic) faf_point(&p[i], &wp[i]);     //  
		else        faf_line (&p[i], &wp[i]);     //  
		faf_point(&vp[i], &wp[i+3]);  //  
	}
	// ,       Y
	faf_set(FAF_ROTATE, wp[l1].x, wp[l1].y);
	for(i = 0; i < 3; i++)
		if(i != ic) faf_line(&wp[i], &wp[i]);    //  
	b1 = -wp[l1].z/wp[l1].y;
	if(fabs(wp[l2].x) < eps_n){
		b2 = -wp[l2].z/wp[l2].y;
		place = 0;
	}
	else{
		b2 = -(b1*wp[l2].y + wp[l2].z)/wp[l2].x;
		place = 1;
	}

	for(i = 0; i < 3; i++)
		faf_point(&wp[i+3], &wp[i+3]);  //   

	for(i = -1; i < 2; i += 2){
		for(i1 = -1; i1 < 2; i1 += 2){
			a1 = i1/wp[l1].y;
			for(i2 = -1; i2 < 2; i2 += 2){
				switch(place){
					case 0: //  
						a2 = i2/wp[l2].y;
						if(fabs(r = a2 - a1) < eps_n)continue;
						if((r = (b1 - b2)/r) < eps_d)continue;
						y = ((a1 + a2)*r + b1 + b2)/2;
						q3 = r + i*wp[ic].z; q3 = y*y - q3*q3;
						break;
					case 1: //  
						a2 = (i2 - wp[l2].y*a1)/wp[l2].x;
						q1 = a1*a1 + a2*a2 - 1;
						q2 = 2*(a1*b1 +a2*b2 - i*wp[ic].z);
						q3 = b1*b1 + b2*b2 - wp[ic].z*wp[ic].z;
						break;
				}
				if(!(k = qwadr_eq(q1, q2, q3, rc)))continue;
				while(--k >= 0){
					switch(place){
						case 0:
							x = rc[k];
							break;
						case 1: //  
							if((r = rc[k]) < eps_d)continue;
							x = a2*r + b2;
							y = a1*r + b1;
							break;
					}
					co = wp[ic].x - x; si = wp[ic].y - y;
					if((dd = hypot(si, co)) < eps_d)goto met_cont;
					j = ((dd < wp[ic].z) && (r < wp[ic].z)) ? -1: 1;
					cd[ic].x = x + j*r*(co/dd);   cd[ic].y = y + j*r*(si/dd);
					cd[l1].x = x - i1*wp[l1].x*r; cd[l1].y = y - i1*wp[l1].y*r;
					cd[l2].x = x - i2*wp[l2].x*r; cd[l2].y = y - i2*wp[l2].y*r;
					dist = 0.;
					for(j = 0; j < 3; j++){
						cd[j].z = 0.;
						dist += dpoint_distance_2d(&wp[j + 3], &cd[j]);
					}
					save_min_var(dist, x, y, r, &mindist, cd, 3, cp, tp);
met_cont:;
				}
			}
		}
	}
	if(trans_min_var(mindist, cp, tp)) return TRUE;
//nb met_err:
	el_geo_err = TANG_NO_CREATE;
	return FALSE;
}


BOOL circle_l_l_l(lpD_POINT l, lpD_POINT vp, lpD_POINT tp, lpD_POINT cp){

D_POINT cd[3];
sgFloat  a[3], b[3], c[3], d[3], dt, rd, x, y, r, dist;
sgFloat  mindist = -1.;
short     sgn[3], i;

	for(i = 0; i < 3; i++){
		a[i] = l[i].x; b[i] = l[i].y; d[i] = -l[i].z;
	}
	rd = det3_3(a, b, d);
	for(sgn[0] = -1; sgn[0] < 2; sgn[0] += 2){
		c[0] = sgn[0];
		for(sgn[1] = -1; sgn[1] < 2; sgn[1] += 2){
			c[1] = sgn[1];
			for(sgn[2] = -1; sgn[2] < 2; sgn[2] += 2){
				c[2] = sgn[2];
				if(fabs(dt = det3_3(a, b, c)) < eps_n)continue;
				if((r = rd/dt) < eps_d) continue;
				x = det3_3(d, b, c)/dt;
				y = det3_3(a, d, c)/dt;
				dist = 0.;

				for(i = 0; i < 3; i++){
					cd[i].x = x + sgn[i]*l[i].x*r;
					cd[i].y = y + sgn[i]*l[i].y*r;
					cd[i].z = 0.;
					dist += dpoint_distance_2d(&vp[i], &cd[i]);
				}
				save_min_var(dist, x, y, r, &mindist, cd, 3, cp, tp);
			}
		}
	}
	if(mindist >= 0.)
		 return TRUE;
	el_geo_err = TANG_NO_CREATE;
	return FALSE;
}

static void save_min_var(sgFloat dist, sgFloat x, sgFloat y, sgFloat r,
                         sgFloat* mindist,
												 lpD_POINT cd, short num, lpD_POINT cp, lpD_POINT tp){
//   
short i;
	if((*mindist >= 0.) && (dist > *mindist))return;
	*mindist = dist;
	cp->x = x; cp->y = y; cp->z = r;
	for(i = 0; i < num; i++)
	  dpoint_copy(&tp[i], &cd[i]);
}
static BOOL trans_min_var(sgFloat mindist, lpD_POINT cp, lpD_POINT tp){
//    
short i;
	if(mindist < 0.) return FALSE;
	faf_inv(cp, cp);
	for(i = 0; i < 3; i++)
		faf_inv(&tp[i], &tp[i]);
	return TRUE;
}
