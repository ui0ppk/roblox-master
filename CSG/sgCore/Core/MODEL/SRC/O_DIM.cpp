#include "../../sg.h"

struct u_d_d {
	lpGEO_DIM geo;
	BOOL (*dim_line)(lpD_POINT pb, lpD_POINT pe, void* user_data);
	void* user_data;
};

BOOL dim_line_geo(lpD_POINT pb, lpD_POINT pe, void* user_data);

WORD tsym_OemToAnsi(WORD sym);
static void grad_format(UCHAR format, sgFloat value, sgFloat snap, char *txt);
static BOOL dim_ekr_line(sgFloat txg, sgFloat tyg, lpD_POINT pb, lpD_POINT pe,
												 BOOL (*dim_line)(lpD_POINT pb, lpD_POINT pe, void* user_data),
												 void* user_data);
static BOOL dim_ekr_point(sgFloat txg, sgFloat tyg, lpD_POINT p);
static short dim_clip_line_out(sgFloat dx, sgFloat dy,
														 lpD_POINT p1, lpD_POINT p2,
														 lpD_POINT pb, lpD_POINT pe);
static void set_shp_point(sgFloat d, sgFloat sa, sgFloat ca,
													char cx, char cy, lpD_POINT p, lpD_POINT p1);
static BOOL arrows_pre_place(lpARRSHP  shp, lpGEO_DIM geo,
														 lpD_POINT gab,
														 sgFloat *ext, sgFloat *extin, sgFloat *tails, UCHAR *outfl);
static void get_dimarc_ext_points(lpDIMLN_GEO dg, lpD_POINT p);
static void get_dimln_ext(lpDIMLN_GEO dg, sgFloat *lext);
static void get_ln_ext(lpDIMLN_GEO dg, lpD_POINT p1, lpD_POINT p2, sgFloat *lext);
static void get_lider_ext(sgFloat coeff, lpDIMLN_GEO dg, sgFloat *lext);
static BOOL dimtext_pre_place(lpGEO_DIM geo, lpDIMLN_GEO dg,
															lpFONT font, UCHAR *text,
															lpD_POINT arrgab);
BOOL test_dim_to_invert(lpGEO_DIM geo);
void set_dimln_geo(lpGEO_DIM geo, lpDIMLN_GEO dlgeo);

lpDIMINFO   dim_info = NULL;


BOOL draw_geo_dim(lpGEO_DIM geo,
					 BOOL (*dim_line)(lpD_POINT pb, lpD_POINT pe, void* user_data),
					 void* user_data)
{
struct       u_d_d ud;

	ud.geo       = geo;
	ud.dim_line  = dim_line;
	ud.user_data = user_data;

	return draw_dim(geo, dim_line_geo, &ud);
}

BOOL dim_line_geo(lpD_POINT pb, lpD_POINT pe, void* user_data){
struct u_d_d *ud = (struct u_d_d*)user_data;
D_POINT pb1, pe1;
	o_hcncrd(ud->geo->matr, pb, &pb1);
	o_hcncrd(ud->geo->matr, pe, &pe1);
	return ud->dim_line(&pb1, &pe1, ud->user_data);
}

BOOL draw_dim(lpGEO_DIM geo,
							BOOL (*dim_line)(lpD_POINT pb, lpD_POINT pe, void* user_data),
							void* user_data)
{
lpFONT       font;
UCHAR        *text;
BOOL         ret;
ULONG        len;

	if(!geo->dtstyle.font || !geo->dtstyle.text) return FALSE;

	if((font = (FONT*)alloc_and_get_ft_value(geo->dtstyle.font, &len)) == NULL)
		return FALSE;
	if((text = (UCHAR*)alloc_and_get_ft_value(geo->dtstyle.text, &len)) == NULL){
		SGFree(font);
		return FALSE;
	}
	ret = dim_grf_create(geo, font, text);
	if(ret)
		ret = draw_dim1(geo, font, text, dim_line, user_data);
	SGFree(text);
	SGFree(font);
	return ret;
}

BOOL draw_dim1(lpGEO_DIM geo, lpFONT font, UCHAR *text,
							BOOL (*dim_line)(lpD_POINT pb, lpD_POINT pe, void* user_data),
							void* user_data)
 
{
D_POINT     arrgab[4], pext[2];
lpARRSHP    shp = (lpARRSHP)ft_bd->buf;
sgFloat      lexta[2], extin[2], tails[4], coeff, a, cb, ce;
sgFloat      lb[4], le[4];
sgFloat      lext[2] = {0, 0};
UCHAR       out[2], inv;
lpGEO_LINE  diml = dim_info->wlines;
lpGEO_ARC   dima = dim_info->warcs;
short       nln = 0, i, j, nn;
ULONG       len;
DIMLN_GEO   dg;

	if(!shp) return FALSE;

	coeff = get_grf_coeff();
	set_dimln_geo(geo, &dg);

	//    
	if(!arrows_pre_place(shp, geo, arrgab, lexta, extin, tails, out))
		return FALSE;

	//    
	if(!dimtext_pre_place(geo, &dg, font, text, arrgab)) return FALSE;

	//  
	for(i = 0; i < 2; i++){
		if(!(geo->dgstyle.flags&((i == 0) ? EXT1_NONE:EXT2_NONE))){
			diml[nln].v1 = geo->p[i + 2];
			diml[nln].v2 = geo->p[i];
			if(correct_line_tails(&diml[nln].v1, &diml[nln].v2,
														geo->dgstyle.extln_in*coeff,
														geo->dgstyle.extln_out*coeff))
				nln++;
		}
	}


	if((geo->dgstyle.tplace == DTP_LIDER) && geo->numtp > 0 ){
		get_lider_ext(geo->tcoeff, &dg, lext);
	}
	else if(dg.drawfl){
		get_dimln_ext(&dg, lext);
	}
	for(i = 0; i < 2; i++)
		if(lexta[i] > lext[i]) lext[i] = lexta[i];
	lb[0] = -lext[0]; le[0] = -tails[0];
	if(!(geo->dgstyle.flags&DIM_LN_NONE)){
		lb[1] = tails[1]; le[1] = dg.l - tails[2];
		lb[2] = le[2] = dg.l;
	}
	else{
		lb[1] = tails[1]; le[1] = extin[0];
		lb[2] = dg.l - extin[1]; le[2] = dg.l - tails[2];
	}
	lb[3] = dg.l + tails[3]; le[3] = dg.l + lext[1];

	if((geo->dgstyle.tplace == DTP_LIDER) && (geo->numtp != 0)){
		if(geo->numtp > 1){
			if(geo->type == DM_ANG){
				if(!get_arc_parpoint(&geo->p[4], &geo->p[0], &geo->p[1], geo->neg,
														 geo->tcoeff, &diml[nln].v1, &a, &inv)) return FALSE;
			}
			else {
				dpoint_parametr(&geo->p[0], &geo->p[1], geo->tcoeff, &diml[nln].v1);
			}
			diml[nln].v2 = geo->tp[0];
			if(!dpoint_eq(&diml[nln].v1, &diml[nln].v2, eps_d)) nln++;
		}
		diml[nln].v1 = geo->tp[0];
		diml[nln].v2 = geo->tp[1];
		lext[0] = lext[1] = 0.;
		get_ln_ext(&dg, &diml[nln].v1, &diml[nln].v2, lext);
		if(correct_line_tails(&diml[nln].v1, &diml[nln].v2, -lext[0], lext[1]))
		nln++;
	}

	//      
	if(geo->type == DM_ANG)	get_dimarc_ext_points(&dg, pext);
	if(dg.l < eps_d) return FALSE;
	for(i = 0; i < 4; i++){
		if(le[i] - lb[i] < eps_d) continue;
		if(geo->type == DM_ANG){
			if(lb[i] < 0. && le[i] > 0.){
				cb = lb[i]; ce = 0.;
				lb[i] = 0.;
				i--;
			}
			else if(lb[i] < dg.l && le[i] > dg.l){
				cb = lb[i]; ce = dg.l;
				lb[i] = dg.l;
				i--;
			}
			else{
				cb = lb[i]; ce = le[i];
			}
			cb /= dg.l; ce /= dg.l;
			if(cb < 0.) {
				dpoint_parametr(&geo->p[0], &pext[0], -cb, &diml[nln].v1);
				dpoint_parametr(&geo->p[0], &pext[0], -ce, &diml[nln].v2);
				if(!dim_ekr_line(dg.txg, dg.tyg, &diml[nln].v1, &diml[nln].v2, dim_line, user_data))
				 return FALSE;
			}
			else if(ce > 1.){
				dpoint_parametr(&geo->p[1], &pext[1], cb - 1., &diml[nln].v1);
				dpoint_parametr(&geo->p[1], &pext[1], ce - 1., &diml[nln].v2);
				if(!dim_ekr_line(dg.txg, dg.tyg, &diml[nln].v1, &diml[nln].v2, dim_line, user_data))
				 return FALSE;
			}
			else{
				get_arc_parpoint(&dg.pc, &dg.pb, &dg.pe, dg.neg, cb, &dima->vb, &a, &inv);
				get_arc_parpoint(&dg.pc, &dg.pb, &dg.pe, dg.neg, ce, &dima->ve, &a, &inv);
				dima->vc = dg.pc; dima->ab = dg.ab; dima->angle = dg.a; dima->r = dg.r;
				nn = d_arc_aprnum(dima, vi_h_tolerance);
				d_get_point_on_arc(dima, 0., &diml[nln].v1);
				for (j = 0; j <= nn; j++) {
					d_get_point_on_arc(dima,(sgFloat)j/nn, &diml[nln].v2);
					if(!dim_ekr_line(dg.txg, dg.tyg, &diml[nln].v1, &diml[nln].v2, dim_line, user_data))
						return FALSE;
					diml[nln].v1 = diml[nln].v2;
				}
			}
		}
		else{ //  
			cb = lb[i]/dg.l; ce = le[i]/dg.l;
			dpoint_parametr(&geo->p[0], &geo->p[1], cb, &diml[nln].v1);
			dpoint_parametr(&geo->p[0], &geo->p[1], ce, &diml[nln].v2);
			if(!dim_ekr_line(dg.txg, dg.tyg, &diml[nln].v1, &diml[nln].v2, dim_line, user_data))
				 return FALSE;
		}
	}
	//  
	for(i = 0; i < nln; i++)
		if(!dim_ekr_line(dg.txg, dg.tyg, &diml[i].v1, &diml[i].v2, dim_line, user_data))
			return FALSE;

	//  
	if(geo->type == DM_ANG){
		cb = M_PI*((dg.a > 0.) ? 0.5:-0.5);
		a = dg.ab + cb;
		for(i = 0; i < 2; i++){
			//     ?
			if(dim_ekr_point(dg.txg, dg.tyg, &geo->p[i])) goto met_aa;
			if(!load_ft_sym(FTARROW, geo->dgstyle.arrtype[i], shp, &len))
				return FALSE;
			if(len){
				if(!draw_arr_shp(geo->dgstyle.arrlen*coeff, 0., 0., a,
					 &geo->p[i], out[i], shp, dim_line, user_data))
					return FALSE;
			}
met_aa:
				a = dg.ab + dg.a - cb;
		}
	}
	else{
		a = dg.a;
		for(i = 0; i < 2; i++){
		//     ?
			if(dim_ekr_point(dg.txg, dg.tyg, &geo->p[i])) goto met_a;
			if(!load_ft_sym(FTARROW, geo->dgstyle.arrtype[i], shp, &len))
				return FALSE;
			if(len){
				if(!draw_arr_shp(geo->dgstyle.arrlen*coeff, 0., 0., a,
					 &geo->p[i], out[i], shp, dim_line, user_data))
					return FALSE;
			}
met_a:
			a += M_PI;
		}
	}

	if(dg.drawfl){ // 
		faf_flag = 1;
		dg.drawfl = draw_dim_text(dg.value, &geo->dtstyle, text, font,
															dim_line, user_data);
		faf_flag = 0;
		return dg.drawfl;
	}
	return TRUE;
}


void set_dimln_geo(lpGEO_DIM geo, lpDIMLN_GEO dlgeo)
{
	memset(dlgeo, 0, sizeof(DIMLN_GEO));
	dlgeo->type = geo->type;
	dlgeo->pb = geo->p[0];
	dlgeo->pe = geo->p[1];
	dlgeo->neg = geo->neg;
	if(geo->type == DM_ANG){
		dlgeo->pc = geo->p[4];
		dlgeo->neg = geo->neg;
		get_dimarc_angles(&geo->p[4], &geo->p[0], &geo->p[1], geo->neg,
											&dlgeo->ab, &dlgeo->a, &dlgeo->r);
		dlgeo->value = fabs(dlgeo->a);
		dlgeo->l = dlgeo->value*dlgeo->r;
	}
	else{
		dlgeo->a = v_angle(geo->p[1].x - geo->p[0].x, geo->p[1].y - geo->p[0].y);
		dlgeo->l = dpoint_distance(&geo->p[0], &geo->p[1]);
		dlgeo->value = get_dim_value(geo); //  
	}
}

short d_arc_aprnum(GEO_ARC *geo_arc, sgFloat h)
{
	int			l;
	sgFloat 	alfa;

	if(h > geo_arc->r/2) alfa = M_PI / 3;
	else alfa = 2*asin(sqrt(h*(2*geo_arc->r - h))/geo_arc->r);
	l = (int)(abs(geo_arc->angle/alfa)) + 1;
	if(l < MIN_NUM_SEGMENTS)
		l = MIN_NUM_SEGMENTS;
	return l;
}

void d_get_point_on_arc(GEO_ARC *geo_arc, sgFloat alpha, lpD_POINT p)
{
	sgFloat 	an;

	if (alpha <= 0.) 	*p = geo_arc->vb;
	else if (alpha >= 1.) *p = geo_arc->ve;
	else {
		an = geo_arc->ab + alpha * geo_arc->angle;
		p->x = geo_arc->vc.x + geo_arc->r*cos(an);
		p->y = geo_arc->vc.y + geo_arc->r*sin(an);
		p->z = geo_arc->vc.z;
	}
}

void get_dimarc_angles(lpD_POINT pc, lpD_POINT p1, lpD_POINT p2,
											 UCHAR neg, sgFloat *ab, sgFloat *a, sgFloat *r)
{
sgFloat ae;
	*ab = v_angle(p1->x - pc->x, p1->y - pc->y);
	ae = v_angle(p2->x - pc->x, p2->y - pc->y);
	while (*ab > ae) ae += 2*M_PI;
	*a = ae - *ab;
	if(*a > M_PI + eps_n) //   180
		neg = (BYTE)(1 - neg);
	if(neg)
		*a -= 2*M_PI;
	*r = dpoint_distance(pc, p1);
}


sgFloat get_dimarc_len(lpD_POINT pc, lpD_POINT p1, lpD_POINT p2,	UCHAR neg)
{
sgFloat l, a, r;
	get_dimarc_angles(pc, p1, p2, neg, &l, &a, &r);
	l = fabs(a)*r;
	return l;
}

sgFloat get_signed_angle(lpD_POINT pc, lpD_POINT p1, lpD_POINT p2, short sgn)
{
sgFloat a, ab, ae;
	ab = v_angle(p1->x - pc->x, p1->y - pc->y);
	ae = v_angle(p2->x - pc->x, p2->y - pc->y);
	while (ab > ae) ae += 2*M_PI;
	a = ae - ab;
	if(sg(a, 0.)*sgn < 0)
		a -= 2*M_PI;
	return a;
}

BOOL get_arc_proection(lpD_POINT pc, lpD_POINT pb, lpD_POINT pe, UCHAR neg,
											 lpD_POINT p, lpD_POINT pp, sgFloat *d)
{
sgFloat   l, a, ab, a1, ap, r, d1, d2;
D_POINT  wp, p1, p2;

	get_dimarc_angles(pc, pb, pe, neg, &ab, &a, &r);
	if(fabs(r) < eps_d || fabs(a) < eps_n) return FALSE;
	l = fabs(a)*r;
	a1 = get_signed_angle(pc, pb, p, sg(a, 0.));
	if(fabs(a1) >= 0. && fabs(a1) <= fabs(a)){ //   
		ab += a1;
		pp->x = r*cos(ab); pp->y = r*sin(ab); pp->z = 0.;
		dpoint_add(pc, pp, pp);
		*d = fabs(a1)/fabs(a);
		return TRUE;
	}
	//    
	ap = ab - M_PI*0.5*sg(a, 0.);
	wp.x = l*cos(ap); wp.y = l*sin(ap); wp.z = 0.;
	dpoint_add(pb, &wp, &wp);
	get_point_proection(pb, &wp, p, &p1);
	d1 = get_point_par(pb, &wp, &p1);
	//    
	ap = ab + a + M_PI*0.5*sg(a, 0.);
	wp.x = l*cos(ap); wp.y = l*sin(ap); wp.z = 0.;
	dpoint_add(pe, &wp, &wp);
	get_point_proection(pe, &wp, p, &p2);
	d2 = get_point_par(pe, &wp, &p2);
	if(dpoint_distance(&p1, p) <= dpoint_distance(&p2, p)){
		*pp = p1;
		*d = -d1;
	}
	else{
		*pp = p2;
		*d = 1. + d2;
	}
	return TRUE;
}

static void get_dimarc_ext_points(lpDIMLN_GEO dg, lpD_POINT p)
{
sgFloat   ap;
D_POINT  wp;

	ap = dg->ab - M_PI*0.5*sg(dg->a, 0.);
	wp.x = dg->l*cos(ap); wp.y = dg->l*sin(ap); wp.z = 0.;
	dpoint_add(&dg->pb, &wp, &p[0]);

	ap = dg->ab + dg->a + M_PI*0.5*sg(dg->a, 0.);
	wp.x = dg->l*cos(ap); wp.y = dg->l*sin(ap); wp.z = 0.;
	dpoint_add(&dg->pe, &wp, &p[1]);
}

BOOL get_arc_parpoint(lpD_POINT pc, lpD_POINT pb, lpD_POINT pe, UCHAR neg,
											sgFloat d, lpD_POINT p, sgFloat *ap, UCHAR *inv)
{
sgFloat   l, a, ab, r;
D_POINT  wp;

	get_dimarc_angles(pc, pb, pe, neg, &ab, &a, &r);
	if(fabs(r) < eps_d || fabs(a) < eps_n) return FALSE;
	l = fabs(a)*r;
	if(d >= 0. && d <= 1.){ //   
		ab += a*d;
		p->x = r*cos(ab); p->y = r*sin(ab); p->z = 0.;
		dpoint_add(pc, p, p);
		*ap = ab + M_PI/2*sg(a, 0.);
	}
	else if(d < 0.){//   
		ab = ab - M_PI*0.5*sg(a, 0.);
		wp.x = l*cos(ab); wp.y = l*sin(ab); wp.z = 0.;
		dpoint_add(pb, &wp, &wp);
		dpoint_parametr(pb, &wp, -d, p);
		*ap = ab;
	}
	else{ //   
		ab = ab + a + M_PI*0.5*sg(a, 0.);
		wp.x = l*cos(ab); wp.y = l*sin(ab); wp.z = 0.;
		dpoint_add(pe, &wp, &wp);
		dpoint_parametr(pe, &wp, d - 1, p);
		*ap = ab;
	}
	while (*ap < 0.) *ap += 2*M_PI;
	while (*ap > 2*M_PI) *ap -= 2*M_PI;
	if(*ap > M_PI*0.50001 && *ap < M_PI*1.50001){
		*ap += M_PI;
		*inv = 1;
	}
	else
		*inv = 0;
	return TRUE;
}

static void get_dimln_ext(lpDIMLN_GEO dg, sgFloat *lext)
  
{
sgFloat  ap, amin = 0, amax = 1;
D_POINT pp;
short   i;

	for(i = 0; i < 4; i++){
		if(dg->type == DM_ANG){
			if(!get_arc_proection(&dg->pc, &dg->pb, &dg->pe, dg->neg,
														&dg->gab[i], &pp, &ap)) return;
		}
		else{
			get_point_proection(&dg->pb, &dg->pe, &dg->gab[i], &pp);
			ap = get_point_par(&dg->pb, &dg->pe, &pp);
		}
		if(ap < amin) amin = ap;
		if(ap > amax) amax = ap;
	}
	lext[0] = -dg->l*amin;
	lext[1] = dg->l*(amax - 1.);
}

static void get_ln_ext(lpDIMLN_GEO dg, lpD_POINT p1, lpD_POINT p2, sgFloat *lext)
//    
{
sgFloat  ap, l, amin = 0, amax = 1;
D_POINT pp;
short   i;

	for(i = 0; i < 4; i++){
		get_point_proection(p1, p2, &dg->gab[i], &pp);
		ap = get_point_par(p1, p2, &pp);
		if(ap < amin) amin = ap;
		if(ap > amax) amax = ap;
	}
	l = dpoint_distance(p1, p2);
	lext[0] = -l*amin;
	lext[1] = l*(amax - 1.);
}

static void get_lider_ext(sgFloat coeff, lpDIMLN_GEO dg, sgFloat *lext)
//     
{
sgFloat  amin = 0, amax = 1;
	if(coeff > amax) amax = coeff;
	if(coeff < amin) amin = coeff;
	lext[0] = -dg->l*amin;
	lext[1] = dg->l*(amax - 1.);
}

static BOOL arrows_pre_place(lpARRSHP  shp, lpGEO_DIM geo,
														 lpD_POINT gab,
														 sgFloat *ext, sgFloat *extin, sgFloat *tails, UCHAR *outfl)
  
{
UCHAR     out1, out[2] = {ARR1_OUT, ARR2_OUT};
sgFloat    coeff;
WORD      imin, imax;
ULONG     len;
D_POINT   p0 = {0, 0, 0};
short     i;

	for(i = 0; i < 2; i++){
		imin = 2*i; imax = imin + 1;
		gab[imin] = gab[imax] = p0;
		tails[imin] = tails[imax] = ext[i] = extin[i] = 0.;
		out1 = (UCHAR)i;
		outfl[i] = 0;
		if(!geo->dgstyle.arrtype[i]) continue;
		if(geo->dgstyle.flags&out[i]){
			out1 = (BYTE)(1 - out1);
			outfl[i] = 1;
		}
		//  
		if(!load_ft_sym(FTARROW, geo->dgstyle.arrtype[i], shp, &len))
			return FALSE;
		if(!len){
			shp->ngrid = 100;
			shp->ext_out = shp->ext_in = shp->out = 0;
		}

		coeff = (geo->dgstyle.arrlen*get_grf_coeff()/shp->ngrid);
		if(!shp->out) outfl[i] = 0;
		if(outfl[i])
			ext[i] = (geo->dgstyle.arrext + geo->dgstyle.arrlen)*get_grf_coeff();
		else
			extin[i] = (geo->dgstyle.arrext + geo->dgstyle.arrlen)*get_grf_coeff();

		if(out1){
			coeff = -coeff;
			imin += 1;
			imax -= 1;
		}
		gab[imin].x = shp->xmin*coeff;
		gab[imin].y = shp->ymin*coeff;
		gab[imax].x = shp->xmax*coeff;
		gab[imax].y = shp->ymax*coeff;
		tails[imin] = fabs(shp->ext_out*coeff);
		tails[imax] = fabs(shp->ext_in*coeff);
		if(!outfl[i] && !shp->out)
			extin[i] = tails[imax];
	}
	return TRUE;
}

static BOOL dimtext_pre_place(lpGEO_DIM geo, lpDIMLN_GEO dg,
															lpFONT font, UCHAR *text,
															lpD_POINT arrgab)
//     
{
D_POINT p = {0, 0, 0}, ttrans;
sgFloat  d, a, hz, lz, lsdv, sa, ca;
short   i0, i1;
UCHAR   inv;

	dg->drawfl = TRUE;
	dg->gab[0].x = dg->gab[0].y = dg->gab[0].z =  GAB_NO_DEF;
	dg->gab[1].x = dg->gab[1].y = dg->gab[1].z = -GAB_NO_DEF;

	if(!draw_dim_text(get_dim_value(geo), &geo->dtstyle, text, font,
										text_gab_lsk, dg->gab)) return FALSE;
	if(dg->gab[0].x == GAB_NO_DEF){ //  
		dg->drawfl = FALSE;
		dg->txg = dg->tyg = 0.;
		dg->gab[0] = dg->gab[1] = p;
		return TRUE;
	}
	else{
		dg->drawfl = TRUE;
		dg->txg = dg->gab[1].x - dg->gab[0].x;
		dg->tyg = dg->gab[1].y - dg->gab[0].y;
	}
	hz =   dg->tyg/5;
	lz =   dg->txg/10;
	lsdv = dg->txg/2;

	if(geo->dgstyle.tplace == DTP_MANUAL){ // 
		a = v_angle(geo->tp[1].x - geo->tp[0].x, geo->tp[1].y - geo->tp[0].y);
		if(a > M_PI*0.50001 && a < M_PI*1.50001) a += M_PI;
		ttrans = geo->tp[0];
		hz = 0.;
		goto mplace;
	}
	if(geo->dgstyle.tplace == DTP_LIDER && geo->numtp != 0){ //  
		i0 = 0; i1 = 1;
		if(dpoint_distance(&geo->tp[0], &geo->tp[1]) > dg->txg){
			i0 = 1; i1 = 0;
		}
		a = v_angle(geo->tp[i1].x - geo->tp[i0].x, geo->tp[i1].y - geo->tp[i0].y);
		lsdv = 0.;
		if(a > M_PI*0.50001 && a < M_PI*1.50001){
			lsdv = dg->txg; a += M_PI;
		}
		ttrans = geo->tp[i0];
		goto mplace;
	}

	if (dg->l < eps_d) return FALSE;
	switch(geo->dgstyle.tplace){
		case DTP_CENTER: // 
			d = 0.5;
			break;
		case DTP_ALONG: //
		case DTP_LIDER: // 
			d = geo->tcoeff;
			break;
		case DTP_LEFT: //
			d = -(dg->txg/2 + lz - arrgab[0].x)/dg->l;
			break;
		case DTP_RIGHT: //
			d = 1 + (dg->txg/2 + lz + arrgab[3].x)/dg->l;
			break;
		default:
			return FALSE;
	}
	if(geo->type == DM_ANG){
		if(!get_arc_parpoint(&geo->p[4], &geo->p[0], &geo->p[1], geo->neg,
												 d, &ttrans, &a, &inv)) return FALSE;
	}
	else{
		dpoint_parametr(&geo->p[0], &geo->p[1], d, &ttrans);
		a = v_angle(geo->p[1].x - geo->p[0].x, geo->p[1].y - geo->p[0].y);
		if(a > M_PI*0.50001 && a < M_PI*1.50001) a += M_PI;
	}
	lsdv = dg->txg/2;
mplace:
	sa = sin(a); ca = cos(a);
	ttrans.x -= (lsdv*ca + hz*sa);
	ttrans.y -= (lsdv*sa - hz*ca);
	ttrans.z = a;
	faf_init();
	faf_set(FAF_ROTATE, sin(ttrans.z), cos(ttrans.z));
	faf_set(FAF_MOVE, ttrans.x, ttrans.y);
	for(i1 = 0; i1 < 4; i1++){
		ttrans = dg->gab[i1];
		faf_exe(&ttrans, &dg->gab[i1]);
	}
	return TRUE;
}

BOOL test_dim_to_invert(lpGEO_DIM geo)
     
{
sgFloat a;
	a = v_angle(geo->p[1].x - geo->p[0].x, geo->p[1].y - geo->p[0].y);
	if(a > M_PI*0.50001 && a < M_PI*1.50001) return TRUE;
	return FALSE;
}

void invert_dim_points(lpGEO_DIM geo){
D_POINT p;
	p = geo->p[0]; geo->p[0] = geo->p[1]; geo->p[1] = p;
	p = geo->p[2]; geo->p[2] = geo->p[3]; geo->p[3] = p;
}

void invert_dim(lpGEO_DIM geo){
WORD    flags = 0;
	invert_dim_points(geo);
	if(geo->dgstyle.flags&EXT1_NONE) flags |= EXT2_NONE;
	if(geo->dgstyle.flags&EXT2_NONE) flags |= EXT1_NONE;
	geo->dgstyle.flags &= ~(EXT1_NONE|EXT2_NONE);
	geo->dgstyle.flags |= flags;
	if(!(geo->dgstyle.flags&ARR_AUTO)){
		flags = 0;
		if(geo->dgstyle.flags&ARR1_OUT) flags |= ARR2_OUT;
		if(geo->dgstyle.flags&ARR2_OUT) flags |= ARR1_OUT;
		geo->dgstyle.flags &= ~(ARR1_OUT|ARR2_OUT);
		geo->dgstyle.flags |= flags;
	}
	if(geo->dgstyle.tplace == DTP_ALONG || geo->dgstyle.tplace == DTP_LIDER)
		geo->tcoeff = -(geo->tcoeff - 1.);
}

void  odim_trans(lpGEO_DIM gdim, lpMATR matr)
{
D_POINT   p0 = {0,0,0}, p1={0,0,1}, vold, vnew;
MATR      m;
short     i, numpnt, numtp = 0;


	if(gdim->type == DM_ANG || gdim->type == DM_RAD) numpnt = 5;
	else                     numpnt = 4;
	if((gdim->dgstyle.tplace == DTP_MANUAL) ||
		 ((gdim->dgstyle.tplace == DTP_LIDER) && (gdim->numtp != 0)))
		numtp = 2;

	//1.      
	o_hcncrd(gdim->matr, &p0, &p0);
	o_hcncrd(gdim->matr, &p1, &p1);
	dpoint_sub(&p1, &p0, &vold);

	//1.     
	o_hcmult(gdim->matr, matr);
	for(i = 0; i < numpnt; i++)
		o_hcncrd(gdim->matr, &gdim->p[i], &gdim->p[i]);
	for(i = 0; i < numtp; i++)
		o_hcncrd(gdim->matr, &gdim->tp[i], &gdim->tp[i]);

	//3.    
	o_hcncrd(matr, &p0, &p0);
	o_hcncrd(matr, &p1, &p1);
	dpoint_sub(&p1, &p0, &vnew);
	//4.        
	dnormal_vector(&vold);
	dnormal_vector(&vnew);
	if(dskalar_product(&vold, &vnew) < 0)
		dpoint_inv(&vnew, &vnew);
	//5.       
	o_hcunit(m);
	o_hcrot1(m, &vnew);
	for(i = 0; i < numpnt; i++)
		o_hcncrd(m, &gdim->p[i], &gdim->p[i]);
	for(i = 0; i < numtp; i++)
		o_hcncrd(m, &gdim->tp[i], &gdim->tp[i]);
	vnew.x = vnew.y = 0.;
	vnew.z = -gdim->p[0].z;
	o_tdtran(m, &vnew); //   0
	for(i = 0; i < numpnt; i++)
		gdim->p[i].z = 0.;
	for(i = 0; i < numtp; i++)
		gdim->tp[i].z = 0.;
	//6.     
	o_minv(m, gdim->matr);
	//7.  
	{
	lpFONT       font;
	UCHAR        *text;
	ULONG        len;
	if(!gdim->dtstyle.font || !gdim->dtstyle.text) return;
	if((font = (FONT*)alloc_and_get_ft_value(gdim->dtstyle.font, &len)) == NULL)
		return;
	if((text = (UCHAR*)alloc_and_get_ft_value(gdim->dtstyle.text, &len)) == NULL){
		SGFree(font);
		return;
	}
	dim_grf_create(gdim, font, text);
	SGFree(font);
	SGFree(text);
	}
	//!!
}

BOOL dim_grf_create(lpGEO_DIM geo, lpFONT font, UCHAR *text)
      
{
lpARRSHP  shp = (lpARRSHP)ft_bd->buf;
sgFloat    arrmax = 0., ltext = 0., larrfree, ldim, coeff, ab, aa, r;
UCHAR     arrout = 0, defarr;
ULONG     len;
D_POINT   gab[2];
BOOL      inv;

	inv = test_dim_to_invert(geo);
	if(inv)
		invert_dim(geo);
	if(!(geo->dgstyle.flags&ARR_AUTO) &&
		 !(geo->dgstyle.flags&DTP_AUTO)){
	//   
		 return TRUE;
	}
	//   
	gab[0].x = gab[0].y = gab[0].z =  GAB_NO_DEF;
	gab[1].x = gab[1].y = gab[1].z = -GAB_NO_DEF;

	if(!draw_dim_text(get_dim_value(geo), &geo->dtstyle, text, font,
										text_gab_lsk, gab)) return FALSE;
	if(gab[0].x != GAB_NO_DEF){
		ltext = (gab[1].x - gab[0].x)*1.1;
	}

	switch(geo->type){
		case DM_ANG:
			get_dimarc_angles(&geo->p[4], &geo->p[0], &geo->p[1], geo->neg, &ab, &aa, &r);
			ldim = fabs(aa)*r;
			goto met_lin;
		default:
			ldim = dpoint_distance(&geo->p[0], &geo->p[1]);
met_lin:
			if(geo->dgstyle.flags&DTP_AUTO){
				//      
				if(ldim > ltext)
					geo->dgstyle.tplace = DTP_CENTER;
				else{
					if(geo->type == DM_RAD){
						if(dpoint_eq(&geo->p[0], &geo->p[4], eps_d))
							geo->dgstyle.tplace = DTP_RIGHT;
						else
							geo->dgstyle.tplace = DTP_LEFT;
					}
				}
			}
			if(geo->dgstyle.flags&ARR_AUTO){  //  - 
				geo->dgstyle.flags &= ~(ARR_MASK & (~ARR_AUTO));
				//    
				larrfree = ldim;
				if(geo->dgstyle.tplace == DTP_CENTER) larrfree -= ltext;
				if(larrfree < 0.) larrfree = 0.;

				//    
				if(geo->dgstyle.flags&ARR_DEFINE)
					defarr = max(geo->dgstyle.arrtype[0], geo->dgstyle.arrtype[1]);
				else
					defarr = dim_info->defarr;
				if(defarr){
					if(!load_ft_sym(FTARROW, defarr, shp, &len))
						return FALSE;
					if(len){
						coeff = (geo->dgstyle.arrlen*get_grf_coeff()/shp->ngrid);
						arrmax = shp->xmax*coeff;
						arrout = shp->out;
					}
				}
				
				geo->dgstyle.flags |= ARR_DEFINE;
				if(geo->type == DM_RAD){
					if(dpoint_eq(&geo->p[0], &geo->p[4], eps_d))
						geo->dgstyle.arrtype[0] = 0;
					else
						geo->dgstyle.arrtype[1] = 0;
				}
				if((3*arrmax > larrfree) && arrout)
					geo->dgstyle.flags |= (ARR1_OUT|ARR2_OUT);

				break;
			}
	}
	return TRUE;
}


BOOL draw_arr_shp(sgFloat h, sgFloat hin, sgFloat hout, sgFloat a, lpD_POINT p,
									UCHAR out, lpARRSHP arr,
									BOOL (*dim_line)(lpD_POINT pb, lpD_POINT pe, void* user_data),
									void* user_data)
{
short   i = 0;
BOOL    first;
char    c, cx, cy;
D_POINT pb, pe, ps = {0, 0, 0};
sgFloat  d, sa, ca;

	 if(out) a += M_PI;
	 d = h/arr->ngrid;
	 sa = sin(a); ca = cos(a);
	 pb = pe = ps;

	 // 
	 hin -= d*arr->ext_in;
	 if(hin > eps_d){
		 set_shp_point(d, sa, ca, arr->ext_in, 0, p, &pb);
		 pe.x = pb.x + hin*ca;
		 pe.y = pb.y + hin*sa;
		 if(!dim_line(&pb, &pe, user_data)) return FALSE;
	 }
	 hout += d*arr->ext_out;
	 if(hout > eps_d){
		 set_shp_point(d, sa, ca, (char)-arr->ext_out, 0, p, &pb);
		 pe.x = pb.x - hout*ca;
		 pe.y = pb.y - hout*sa;
		 if(!dim_line(&pb, &pe, user_data)) return FALSE;
	 }

	 while(TRUE){
		 //  
		 c = arr->table[i++];
		 switch(c){
			 case 0:
				 break;
			 case 1: //  
				 while(TRUE){
					 cx = arr->table[i++];
					 if(cx > 126 || cx < -126) break;
					 cy = arr->table[i++];
					 set_shp_point(d, sa, ca, cx, cy, p, &pb);
					 cx = arr->table[i++];
					 cy = arr->table[i++];
					 set_shp_point(d, sa, ca, cx, cy, p, &pe);
					 if(!dim_line(&pb, &pe, user_data)) return FALSE;
				 }
				 continue;
			 case 2: // 
			 case 3: // 
			 case 4: //  
				 first = TRUE;
				 while(TRUE){
					 cx = arr->table[i++];
					 if(cx > 126 || cx < -126) break;
					 cy = arr->table[i++];
					 if(first){
						 set_shp_point(d, sa, ca, cx, cy, p, &pb);
						 ps = pb;
						 first = FALSE;
						 continue;
					 }
					 set_shp_point(d, sa, ca, cx, cy, p, &pe);
					 if(!dim_line(&pb, &pe, user_data)) return FALSE;
					 pb = pe;
				 }
				 if(c != 2)
					 if(!dim_line(&pb, &ps, user_data)) return FALSE;
				 continue;
			 case 5: //  
				 cx = arr->table[i++];
				 cy = arr->table[i++];
				 set_shp_point(d, sa, ca, cx, cy, p, &pb);
				 *p = pb;
				 continue;
		 }
		 break;
	 }
	 return TRUE;
}

static void  set_shp_point(sgFloat d, sgFloat sa, sgFloat ca,
													char cx, char cy, lpD_POINT p, lpD_POINT p1)
{
sgFloat dx, dy;

	dx = d*cx; dy = d*cy;
	d  = dx*ca - dy*sa;
	dy = dx*sa + dy*ca;
	dx = d;
	p1->x = p->x + dx;
	p1->y = p->y + dy;
}

static BOOL dim_ekr_point(sgFloat txg, sgFloat tyg, lpD_POINT p)
// ,     
{
D_POINT p1;

	if(txg < eps_d || tyg < eps_d)
		return FALSE;
	faf_inv(p, &p1);
	if(p1.x < eps_d || p1.x > txg - eps_d) return FALSE;
	if(p1.y < eps_d || p1.y > tyg - eps_d) return FALSE;
	return TRUE;
}

static BOOL dim_ekr_line(sgFloat txg, sgFloat tyg, lpD_POINT pb, lpD_POINT pe,
							BOOL (*dim_line)(lpD_POINT pb, lpD_POINT pe, void* user_data),
							void* user_data)
//      
{
D_POINT wpb[2], wpe[2], p1, p2;
short   i, n;

	if(txg < eps_d || tyg < eps_d)
		return dim_line(pb, pe, user_data);
	faf_inv(pb, &p1);
	faf_inv(pe, &p2);
	n = dim_clip_line_out(txg, tyg, &p1, &p2, wpb, wpe);
	for (i = 0; i < n; i++){
		faf_exe(&wpb[i], &wpb[i]);
		faf_exe(&wpe[i], &wpe[i]);
		if(!dim_line(&wpb[i], &wpe[i], user_data)) return FALSE;
	}
	return TRUE;
}

static short dim_clip_line_out(sgFloat dx, sgFloat dy,
														 lpD_POINT p1, lpD_POINT p2,
														 lpD_POINT pb, lpD_POINT pe)
//        (0,0)  (dx,dy)
//     pb, pe.   
{
/*F_WINDOW w;
D_POINT  wp[2];
short    n, i1, i2;
sgFloat   a[2], lx, ly;

	w.xw1 = 0; 	w.xw2 = dx; w.yw1 = 0; w.yw2 = dy;
	wp[0] = *p1; wp[1] = *p2;*/

	/*switch (f_clip_line(&w, &wp[0].x, &wp[0].y, &wp[1].x, &wp[1].y)){
		default: //(W_DRAW)
			return 0;
		case W_EMPTY: *///  
			pb[0] = *p1; pe[0] = *p2;
			return 1;
		/*case W_CLIP:  //  
			lx = p2->x - p1->x;
			ly = p2->y - p1->y;
			if(fabs(lx) > fabs(ly)) {
				if(fabs(lx) < eps_d) return 0;
				a[0] = (wp[0].x - p1->x)/lx;
				a[1] = (wp[1].x - p1->x)/lx;
			}
			else{
				if(fabs(ly) < eps_d) return 0;
				a[0] = (wp[0].y - p1->y)/ly;
				a[1] = (wp[1].y - p1->y)/ly;
			}
			i1 = 0; i2 = 1;
			if(a[0] > a[1]){i1 = 1; i2 = 0;}

			n = 0;
			if(a[i1] >= eps_n){
				pb[n] = *p1;
				pe[n] = wp[i1];
				n++;
			}
			if(1. - a[i2] >= eps_n){
				pb[n] = wp[i2];
				pe[n] = *p2;
				n++;
			}
			return n;
	}*/
}

sgFloat get_dim_value(lpGEO_DIM geo)
//     
{
sgFloat ab, aa, r;

	switch(geo->type){
		default:
			return dpoint_distance(&geo->p[0], &geo->p[1])*dim_info->valcoeff;
		case DM_ANG:
			get_dimarc_angles(&geo->p[4], &geo->p[0], &geo->p[1], geo->neg, &ab, &aa, &r);
			return fabs(aa);
	}
}

sgFloat get_grf_coeff(void)
//   
{
sgFloat v;

	v = dim_info->grfcoeff;
	if(!v) v = 1;
	return v;
}

BOOL draw_dim_text(sgFloat dimvalue, lpDIMTSTYLE dstyle, UCHAR *dimtext,
									 lpFONT font,
									 BOOL (*sym_line)(lpD_POINT pb, lpD_POINT pe, void* user_data),
									 void* user_data)
{

sgFloat   gdx, gdy, sx, ds, a, htlr, tlrcoeff, height;
D_POINT  p = {0., 0., 0.}, p1, p2;
char     cs, *t, *tg;
char     *str[4];
char     strvalue[C_MAX_STRING_CONSTANT];
char     strsave[30];
BOOL     ret = FALSE;
lpTSTYLE style;

	style = &(dstyle->tstyle);

	get_dimt_str4((char*)dimtext, str);

	t = strstr(str[0], DIM_AUTO_SUBST);
	if(t != NULL){ //    
		cs = *t;
		*t = 0;
		strcpy(strvalue, str[0]);
		//    
		tg = &strvalue[strlen(strvalue)];
		get_dim_value_text(dimvalue, dstyle, tg);
		tg = strchr(tg, '°');
		if(tg){
		 *tg = 0;
		 strcpy(strsave, tg + 1);
		 strcat(strvalue, "%%d");
		 if(strlen(strsave))
			 strcat(strvalue, strsave);
		}
		if(strlen(t + 2)) strcat(strvalue, t + 2);
	}
	else{
		strcpy(strvalue, str[0]);
	}
	//     +-
	if(str[1] != NULL && str[2] != NULL){
		if(*(str[1]) == '+' && *(str[2]) == '-' && !strcmp(str[1] + 1, str[2] + 1)){
			//  
			strcat(strvalue, "%%p");
			strcat(strvalue, str[1] + 1);
			str[1] = str[2] = NULL;
		}
	}
	str[0] = strvalue;
	//  
	//       
	height = style->height*get_grf_coeff();
	gdy = height/font->num_up;
	gdx = gdy*(style->scale/100);
	a = ((90 - style->angle)*M_PI)/180;
	sx  = 1./tan(a);
	ds = (style->dhor*height)/100;

	if(strlen(str[0])){ //   . 
		if(!draw_string(font, (UCHAR*)str[0], 0, gdx, gdy, sx, ds, &p, sym_line,
										user_data)) goto metend;
	}
	p1 = p2 = p;
	tlrcoeff = dstyle->tlrcoeff;
	htlr = height*tlrcoeff;
	if(str[2])if(strlen(str[2])){ //     
		p.y = p.y + height/2 - htlr*1.1;
		if(!draw_string(font, (UCHAR*)str[2], 0, gdx*tlrcoeff, gdy*tlrcoeff, sx,
										ds*tlrcoeff, &p, sym_line, user_data)) goto metend;
		p2 = p;
	}

	if(str[1])if(strlen(str[1])){ //     
		p = p1;
		p.y = p.y + height/2 + htlr*0.1;
		if(!draw_string(font, (UCHAR*)str[1], 0, gdx*tlrcoeff, gdy*tlrcoeff, sx,
										ds*tlrcoeff, &p, sym_line, user_data)) goto metend;
	}
	if(p2.x < p.x) p2.x = p.x;
	p = p1; p.x = p2.x;

	if(str[3])if(strlen(str[3])){ //   . 
		if(!draw_string(font, (UCHAR*)str[3], 0, gdx, gdy, sx, ds, &p, sym_line,
										user_data)) goto metend;
	}
	ret = TRUE;
metend:
	if(t != NULL) *t = cs;
	return ret;
}

char *get_dim_value_text(sgFloat dimvalue, lpDIMTSTYLE dstyle, char *txt)
{
sgFloat num;
char   *t;
UCHAR  format;

	num = (dstyle->snap < eps_n) ? 1 : dstyle->snap;

	format = (BYTE)(dstyle->format&DTF_MSK);
	if(format == DTF_LINEAR)
		sgFloat_output1(floor_crd_par(dimvalue, num),
									 0, DIM_MAX_PREC, TRUE, TRUE, FALSE, txt);
	else
		grad_format(format, dimvalue, num, txt);

	t = strchr(txt, '.');
	if(t) *t = ',';
	return txt;
}

//    
static void grad_format(UCHAR format, sgFloat value, sgFloat snap, char *txt)
{
sgFloat 	grad, min, mind, sec;
char		bufmin[40], bufsec[40];

	value = (value*180.)/M_PI;
	bufmin[0] = bufsec[0] = 0;

	switch(format){
		case DTF_ANGMINSEC:
			grad = floor(value);
			min =  floor(mind = (value - grad) * 60.);
			sec = (mind - min) * 60.;
			sgFloat_output1(floor_crd_par(sec, snap),
										 0, DIM_MAX_PREC, TRUE, TRUE, FALSE, bufsec);
			if(!strncmp(bufsec, "60", 2)){
				min = min + 1.;
				bufsec[0] = 0;
			}
			else{
				strcat(bufsec, "\"");
			}
			goto metmin;
		case DTF_ANGMIN:
			grad = floor(value);
			min = (value - grad) * 60.;

metmin:
			sgFloat_output1(floor_crd_par(min, snap),
										 0, DIM_MAX_PREC, TRUE, TRUE, FALSE, bufmin);
			if(!strncmp(bufmin, "60", 2)){
				grad = grad + 1.;
				bufmin[0] = 0;
			}
			else{
				strcat(bufmin, "\'");
			}
			goto metgrad;
		case DTF_ANGLE:
			grad = value;
metgrad:
			while(grad > 360.) grad = grad - 360.;
			sgFloat_output1(floor_crd_par(grad, snap),
										 0, DIM_MAX_PREC, TRUE, TRUE, FALSE, txt);
			strcat(txt, "°");
			strcat(txt, bufmin);
			strcat(txt, bufsec);
			break;
		default:
			txt[0] = 0;
			break;
	}
	return;
}


void get_dimt_str4(char *dimtext, char **str)
{
BOOL  flag = FALSE;
short i;

	str[0] = str[1] = str[2] = str[3] = NULL;

	if(*dimtext == ML_S_C) {
		flag = TRUE;
		dimtext++;
	}
	i = 0;
	while(*dimtext != ML_S_C && i < 4){
		str[i++] = (char*)dimtext;
		if(!flag) break;
		dimtext += (strlen((char*)dimtext) + 1);
	}
}


ULONG compact_dimt(char *dimtext)
{
char  *str[4], *c;
char  buf[C_MAX_STRING_CONSTANT + 1];
ULONG i;
ULONG n;

	get_dimt_str4(dimtext, str);

	for(i = 3; i >= 0; i--){
		if(str[i]) if(strlen(str[i])){
			n = i + 1;
			break;
		}
	}
	c = buf;
	if(n > 1){
		*c = ML_S_C;
		c++;
	}
	for(i = 0; i < n; i++){
		strcpy(c, ((str[i] == NULL) ? "" : str[i]));
		c += (strlen(c) + 1);
	}
	if(n > 1)	*c = ML_S_C;
//	n = av_text_len(UC_TEXT, buf);
	memcpy(dimtext, buf, n);
	return n;
}

void dim_info_init(void)
{
char       shablon[] = {'%','%','c','<','>',0};
DIMTSTYLE  dts;
short      i;

	dts.font = ft_bd->def_font;
	dts.text = add_ft_value(FTTEXT, &shablon[3], 3);
	dts.tstyle = style_for_init;
	dts.format = DTF_LINEAR;
	dts.snap = 0.1;
	dts.tlrcoeff = 0.6;

	for(i = 0; i <  DM_NUM_TYPES; i++)
		dim_info->dts[i] = dts;

	//   -  
	//  
	dim_info->dts[DM_ANG].format = DTF_ANGMIN;
	dim_info->dts[DM_ANG].snap = 0;
	//  
	dim_info->dts[DM_DIAM].text = add_ft_value(FTTEXT, shablon, 6);
	//  
	shablon[2] = 'R';
	dim_info->dts[DM_RAD].text = add_ft_value(FTTEXT, &shablon[2], 4);

	//------------

	dim_info->dgs.arrlen = 4.;    //  
	dim_info->dgs.arrext = 2.;    //  
	dim_info->dgs.extln_in = 0.;  //     
	dim_info->dgs.extln_out = 5.;     //    
	dim_info->dgs.flags = ARR_AUTO|DTP_AUTO; // . 
	dim_info->dgs.tplace = 0;         // 

	dim_info->defarr = 1;  //    

	dim_info->bstep = 8;   //   

	// 
	dim_info->valcoeff  = 1.;
	dim_info->grfcoeff  = 1.;

	//  
	dim_info->lpunkt    = 0.5;
	dim_info->hcoeff    = 1.;            //  
	dim_info->hangle    = d_radian(45.); //  
	dim_info->htype     = 0;             //  
	dim_info->hunits    = 0;

	//  
	dim_info->ldimflg[0] = LDIM_PAR;
	dim_info->ldimflg[1] = LDIM_NEW;
	dim_info->adimflg    = ADIM_POZITIV;

	return;
}

void free_dim_info(void)
//    
//   
{
	if(dim_info){
		SGFree(dim_info);
		dim_info = NULL;
	}
}

BOOL set_ldim_geo(lpGEO_DIM geo_dim,short move_type, 
				  lpFONT fnt, UCHAR *txt,
				  lpD_POINT f_p,  lpD_POINT s_p, lpD_POINT p1, 
				  const sgCMatrix* plM, const sgCMatrix* invM, 
				  short *errtype);
BOOL set_adim_geo(lpGEO_DIM geo_dim,lpFONT fnt, UCHAR *txt,
				  lpD_POINT f_p,  lpD_POINT s_p, lpD_POINT th_p,lpD_POINT four_p,
				  lpD_POINT p1, 
				  const sgCMatrix* plM, const sgCMatrix* invM,
				  short *errtype);


BOOL set_dim_geo(lpGEO_DIM geo_dim,short move_type, 
				 lpFONT fnt, UCHAR *txt,
				 lpD_POINT f_p,  lpD_POINT s_p, lpD_POINT th_p,lpD_POINT four_p,
				 lpD_POINT p1, short *errtype)
//   GCOND    p1  GEO_DIM
{
	/*if(gcond.flags[1] == GEODIM_EDIT){
		switch(((lpRDM_UI)gcond.user)->geo->type){
			case DM_LIN:
			default: // 
				return edit_ldim_geo(p1, errtype);
			case DM_ANG:
				return edit_adim_geo(p1, errtype);
		}
	}*/
	SG_VECTOR plN;
	sgFloat    plD;
	sgSpaceMath::PlaneFromPoints(*((SG_POINT*)f_p),*((SG_POINT*)s_p),*((SG_POINT*)p1),plN,plD);
	sgCMatrix planeMatr;
	planeMatr.VectorToZAxe(plN);
	D_POINT aaa,bbb,ccc,ddd,eee;
	aaa=*f_p;
	bbb=*s_p;
	if (th_p)
		ccc=*th_p;
	if (four_p)
		ddd = *four_p;
	eee = *p1;
	planeMatr.ApplyMatrixToPoint(*(SG_POINT*)(&aaa));
	planeMatr.ApplyMatrixToPoint(*(SG_POINT*)(&bbb));
	if (th_p)
		planeMatr.ApplyMatrixToPoint(*(SG_POINT*)(&ccc));
	if (four_p)
		planeMatr.ApplyMatrixToPoint(*(SG_POINT*)(&ddd));
	planeMatr.ApplyMatrixToPoint(*(SG_POINT*)(&eee));
	sgCMatrix inverseMatr(planeMatr);
	SG_VECTOR transV = {0.0, 0.0, -aaa.z};
	inverseMatr.Translate(transV);
	inverseMatr.Inverse();
	aaa.z = bbb.z = ccc.z =ddd.z =eee.z =0.0;
	switch(geo_dim->type){
		case DM_LIN:
		case DM_RAD:
		case DM_DIAM:
			return set_ldim_geo(geo_dim,move_type,fnt,txt, &aaa, &bbb, &eee, &planeMatr,&inverseMatr,errtype);
		case DM_ANG:
			return set_adim_geo(geo_dim,fnt,txt, &aaa, &bbb, &ccc, &ddd,&eee,&planeMatr,&inverseMatr,errtype);
		default:
			return FALSE;
	}
}


void gcs_to_dimpln(lpMATR m, lpMATR mi, lpD_POINT gp, lpD_POINT dp,
				   const sgCMatrix* plM, const sgCMatrix* invM)
{
	D_POINT v0 = {0, 0, 0};
	D_POINT v1 = {0, 0, 1};
	sgFloat  d;

	o_hcncrd(m, &v0, &v0);
	o_hcncrd(m, &v1, &v1);
	/*gcs_to_vcs(&v0, &v0);*/plM->ApplyMatrixToPoint(*(SG_POINT*)(&v0));
	/*gcs_to_vcs(&v1, &v1);*/plM->ApplyMatrixToPoint(*(SG_POINT*)(&v1));
	dpoint_sub(&v1, &v0, &v1);
	if(fabs(v1.z) < eps_n){
		o_hcncrd(mi, gp, dp);
	}
	else{
		dnormal_vector(&v1);
		d = dcoef_pl(&v0, &v1);
		/*gcs_to_vcs(gp, dp);*/ *dp=*gp;plM->ApplyMatrixToPoint(*(SG_POINT*)(dp));
		dp->z = (-d - v1.x*dp->x - v1.y*dp->y)/v1.z;
		/*vcs_to_gcs(dp, dp);*/ invM->ApplyMatrixToPoint(*(SG_POINT*)(dp));
		o_hcncrd(mi, dp, dp);
	}
}


void pozitiv_in_vcs(lpD_POINT v,const sgCMatrix* plM, const sgCMatrix* invM)
{
	D_POINT v0 = {0, 0, 0};
	D_POINT p, p0;

	/*gcs_to_vcs(v, &p);*/ p=*v;plM->ApplyMatrixToPoint(*(SG_POINT*)(&p));
	/*gcs_to_vcs(&v0, &p0);*/ p0=v0;plM->ApplyMatrixToPoint(*(SG_POINT*)(&p0));
	dpoint_sub(&p, &p0, &p);
	if(p.z < 0.)
		dpoint_inv(v, v);
}


void set_dim_matr(lpGEO_DIM geo, lpD_POINT v, lpD_POINT p, short n, lpD_POINT p1,
				  const sgCMatrix* plM, const sgCMatrix* invM)
{
	
	memcpy(geo->matr,const_cast<sgCMatrix*>(invM)->GetData(),sizeof(MATR));
}


BOOL set_ldim_geo(lpGEO_DIM geo_dim,short move_type, 
				  lpFONT fnt, UCHAR *txt,
				  lpD_POINT f_p,  lpD_POINT s_p, lpD_POINT p1, 
				  const sgCMatrix* plM, const sgCMatrix* invM,
				  short *errtype)
//   GCOND    p1  GEO_DIM
{
	D_POINT    p[4], v = {0, 0, 0};
	sgFloat     a, b, c, c1, d, d1, z, r;
	lpGEO_DIM  geo;
	short        i;
	BOOL       inv;
	lpFONT   font = fnt;
	UCHAR    *text = txt;
	
	geo = geo_dim;

	/*D_POINT ppp[3];
	ppp[0] = p[0] =*f_p; 
	ppp[1] = p[1] =*s_p;
	ppp[2] = p[2] =*p1;
	gcond.p = ppp;*/



	short one_more_flag = move_type;//LDIM_FREE;//dim_info->ldimflg[0];//was gcond.flags[0] RA
	
	*errtype = 0;

	short  flag_was_gcond_flag_1 = LDIM_NEW;

	if (geo->type==DM_RAD || geo->type==DM_DIAM)
		flag_was_gcond_flag_1 = RDIM_POINTS;

	if((flag_was_gcond_flag_1 == LDIM_ALONGC) ||  //  
		(flag_was_gcond_flag_1 == LDIM_LIDERC)){   //   
			geo->tcoeff = p1->x;
			geo->dgstyle.tplace = (UCHAR)((flag_was_gcond_flag_1 == LDIM_ALONGC) ? DTP_ALONG:DTP_LIDER);
			geo->dgstyle.flags &= (~DTP_AUTO);
			geo->numtp = 0;
			return dim_grf_create(geo, font, text);
		}
		if((flag_was_gcond_flag_1 == LDIM_ALONG) ||  //  
			(flag_was_gcond_flag_1 == LDIM_LIDER)){   //   
				gcs_to_dimpln(gcond.m[0], gcond.m[1], p1, &p[0],plM,invM);
				if(!get_point_proection(&geo->p[0], &geo->p[1], &p[0], &p[1]))
					return FALSE;
				geo->tcoeff = get_point_par(&geo->p[0], &geo->p[1], &p[1]);
				geo->dgstyle.tplace = (UCHAR)((flag_was_gcond_flag_1 == LDIM_ALONG) ? DTP_ALONG:DTP_LIDER);
				geo->dgstyle.flags &= (~DTP_AUTO);
				geo->numtp = 0;
				if(!dim_grf_create(geo, font, text)) return FALSE;
				//set_echo_par(1, &geo->tcoeff);
				return TRUE;
			}
			if(flag_was_gcond_flag_1 == DIM_LIDER1){ //    
				dpoint_parametr(&geo->p[0], &geo->p[1], geo->tcoeff, &geo->tp[0]);
				o_hcncrd(gcond.m[1], p1, &geo->tp[1]);
				geo->tp[1].z = 0.;
				geo->numtp = 1;
				geo->dgstyle.tplace = DTP_LIDER;
				if(!dim_grf_create(geo, font, text)) return FALSE;
				return TRUE;
			}
			if(flag_was_gcond_flag_1 == DIM_LIDER2){ //    
				if(geo->numtp == 1)
					geo->tp[0] = geo->tp[1];
				o_hcncrd(gcond.m[1], p1, &geo->tp[1]);
				geo->tp[1].z = 0.;
				geo->numtp = 2;
				if(dpoint_eq(&geo->tp[0], &geo->tp[1], eps_d)){
					geo->tp[1] = geo->tp[0];
					dpoint_parametr(&geo->p[0], &geo->p[1], geo->tcoeff, &geo->tp[0]);
					geo->numtp = 1;
				}
				return dim_grf_create(geo, font, text);
			}
			if(flag_was_gcond_flag_1 == DIM_TP1){ //   
				o_hcncrd(gcond.m[1], p1, &geo->tp[0]);
				dpoint_sub(&geo->p[1], &geo->p[0], &p[1]);
				dpoint_add(&geo->tp[0], &p[1], &geo->tp[1]);
				geo->tp[0].z = geo->tp[1].z = 0.;
				geo->dgstyle.tplace = DTP_MANUAL;
				geo->dgstyle.flags &= (~DTP_AUTO);
				return dim_grf_create(geo, font, text);
			}

			if(flag_was_gcond_flag_1 == DIM_TP2){ //   
				o_hcncrd(gcond.m[1], p1, &geo->tp[1]);
				geo->tp[1].z = 0.;
				if(dpoint_eq(&geo->tp[1], &geo->tp[0], eps_d)){
					dpoint_sub(&geo->p[1], &geo->p[0], &p[1]);
					dpoint_add(&geo->tp[0], &p[1], &geo->tp[1]);
					geo->tp[1].z = 0.;
				}
				return dim_grf_create(geo, font, text);
			}

			z = f_p->z;
			if(flag_was_gcond_flag_1 == LDIM_CONT){ //   
				memcpy(geo->matr, gcond.m[0], sizeof(MATR));
				o_hcncrd(gcond.m[1], p1, &p[0]);
				if(!get_point_proection(f_p, s_p, &p[0], &p[1]))
					return FALSE;
				d = get_point_par(f_p, s_p, &p[1]);
				if(d > -eps_n && d < 1. + eps_n){
					*errtype = 1; //  
					return FALSE;
				}
				if (d < 0.) {
					geo->p[0] = *f_p;
					geo->p[1] = p[1];
					geo->p[2] = gcond.p[2];
					geo->p[3] = p[0];
				}
				else{
					geo->p[0] = *s_p;
					geo->p[1] = p[1];
					geo->p[2] = gcond.p[3];
					geo->p[3] = p[0];
				}
				geo->p[3].z = geo->p[2].z = geo->p[1].z = geo->p[0].z = z;
				geo->dgstyle = dim_info->dgs;
				inv = test_dim_to_invert(geo);
				if(inv)
					invert_dim_points(geo);
				geo->dgstyle = dim_info->dgs;
				geo->dgstyle.flags |= ((inv) ? EXT2_NONE : EXT1_NONE);
				if(geo->dgstyle.flags & DTP_AUTO)
					geo->dgstyle.tplace = (UCHAR)(((inv) ? DTP_LEFT:DTP_RIGHT));
				return dim_grf_create(geo, font, text);
			}

			if(flag_was_gcond_flag_1 == LDIM_BASE){ //   
				memcpy(geo->matr, gcond.m[0], sizeof(MATR));
				o_hcncrd(gcond.m[1], p1, &p[0]);

				if(!get_point_proection(f_p, s_p, &p[0], &p[1]))
					return FALSE;
				d  = get_point_par(f_p, s_p, &p[1]);
				if(fabs(d) < eps_n || fabs(d - 1.) < eps_n)
					return FALSE;


				if(d < 0.) { //  
					geo->p[0] = *s_p;
					geo->p[1] = p[1];
					geo->p[2] = gcond.p[3];
					geo->p[3] = p[0];
					p[2] = gcond.p[0];
					p[3] = gcond.p[2];
				}
				else { //   
					geo->p[0] = *f_p;
					geo->p[1] = p[1];
					geo->p[2] = gcond.p[2];
					geo->p[3] = p[0];
					p[2] = gcond.p[1];
					p[3] = gcond.p[3];
				}
				// 
				d1 = dim_info->bstep*get_grf_coeff();
				if(!eq_line(&geo->p[0], &geo->p[1], &a, &b, &c))
					return FALSE;
				for(i = 0; i < 2; i++){
					p[0].x = geo->p[0].x + d1*a;
					p[0].y = geo->p[0].y + d1*b;
					p[1].x = geo->p[1].x + d1*a;
					p[1].y = geo->p[1].y + d1*b;
					d1 = -d1;

					// 
					if(d > 0. && d < 1.){
						if(sg(a*p[1].x + b*p[1].y + c, eps_d)*
							sg(a*geo->p[3].x + b*geo->p[3].y + c, eps_d) < 0) continue;
					}
					else{
						c = -(a*p[0].x + b*p[0].y);
						if(sg(a*p[2].x + b*p[2].y + c, eps_d)*
							sg(a*p[3].x + b*p[3].y + c, eps_d) < 0) continue;
					}
					//  
					geo->p[0] = p[0];
					geo->p[1] = p[1];
					geo->p[3].z = geo->p[2].z = geo->p[1].z = geo->p[0].z = z;
					if(test_dim_to_invert(geo))
						invert_dim_points(geo);
					geo->dgstyle = dim_info->dgs;
					return dim_grf_create(geo, font, text);
				}
				*errtype = 2;
				return FALSE;
			}
			if(geo->type == DM_RAD || geo->type == DM_DIAM){// ()
				if(flag_was_gcond_flag_1 == RDIM_ARC){// ()  
					v = *s_p;
					set_dim_matr(geo, &v, p, 2, p1,plM,invM);
					r = gcond.p[2].x;
				}
				if(flag_was_gcond_flag_1 == RDIM_POINTS){// ()   
					//RA - - normal_cpl(&v);
					p[0] = *f_p;//RA
					p[1] = *s_p;//RA
					p[2] = *p1;//RA
					set_dim_matr(geo, &v, p, 2, p1,plM,invM);
					r = dpoint_distance_2d(&p[0], &p[1]);
					if(r < eps_d)
						return FALSE;
				}
				a = dpoint_distance_2d(&p[0], &p[2]);
				if(a >= eps_d && geo->type!=SG_DT_RAD){//   
					dpoint_sub(&p[2], &p[0], &v);
					b = v_angle(v.x, v.y) + M_PI/2;
					geo->p[2].x = p[0].x + r*cos(b);
					geo->p[2].y = p[0].y + r*sin(b);
					geo->p[2].z = 0;
					dpoint_parametr(&p[0], &geo->p[2], -1., &geo->p[3]);
					dpoint_add(&geo->p[2], &v, &geo->p[0]);
					dpoint_add(&geo->p[3], &v, &geo->p[1]);
					geo->p[3].z = geo->p[2].z = geo->p[1].z = geo->p[0].z = 0.;
					if(dpoint_eq(&geo->p[1], &geo->p[0], eps_d))
						return FALSE;
					if(test_dim_to_invert(geo))
						invert_dim_points(geo);
					//geo->dgstyle = dim_info->dgs;
					return dim_grf_create(geo, font, text);
				}
				if(a < eps_d){
					p[2] = p[0]; p[2].x += r; a = r;
				}
				dpoint_parametr(&p[0], &p[2], r/a, &geo->p[1]);
				if(geo->type == DM_RAD)
					geo->p[0] = geo->p[4] = p[0];
				else
					dpoint_parametr(&p[0], &geo->p[1], -1., &geo->p[0]);
				geo->p[1].z = geo->p[0].z = geo->p[4].z = 0.;
				geo->p[3] = geo->p[1]; geo->p[2] = geo->p[0];
				if(test_dim_to_invert(geo))
					invert_dim_points(geo);
				//geo->dgstyle = dim_info->dgs;
				geo->dgstyle.flags &= ~(EXT1_NONE|EXT2_NONE);
				if(dpoint_eq(&geo->p[1], &geo->p[0], eps_d))
					return FALSE;
				return dim_grf_create(geo, font, text);
			}


			//  
	/*		if(flag_was_gcond_flag_1 == LDIM_NEW3D){
				if(!eq_plane(f_p, s_p, p1, &v, &d)) return FALSE;
			}
			else
			{
				//normal_cpl(&v);
				if(!eq_plane(f_p, s_p, p1, &v, &d)) return FALSE;
			}*/
			set_dim_matr(geo, &v, p, /*gcond.maxcond*/2, p1,plM,invM);

			p[0] = *f_p;
			p[1] = *s_p;
			p[2] = *p1;
			switch(one_more_flag){ //  
		case LDIM_VERT:
			geo->p[2] = p[0];
			geo->p[3] = p[1];
			geo->p[0] = geo->p[1] = p[2];
			geo->p[0].y = geo->p[2].y;
			geo->p[1].y = geo->p[3].y;
			break;
		case LDIM_HOR:
			geo->p[2] = p[0];
			geo->p[3] = p[1];
			geo->p[0] = geo->p[1] = p[2];
			geo->p[0].x = geo->p[2].x;
			geo->p[1].x = geo->p[3].x;
			break;
		case LDIM_PAR:
			geo->p[2] = p[0];
			geo->p[3] = p[1];
			geo->p[0] = geo->p[1] = p[2];
			if(!eq_line(&geo->p[2], &geo->p[3], &a, &b, &c))
				return FALSE;
			d = a*p[2].x + b*p[2].y + c;
			geo->p[0].x = geo->p[2].x + d*a;
			geo->p[0].y = geo->p[2].y + d*b;
			geo->p[1].x = geo->p[3].x + d*a;
			geo->p[1].y = geo->p[3].y + d*b;
			break;
		case LDIM_ANG:
			geo->p[2] = p[0];
			geo->p[3] = p[1];
			geo->p[0] = geo->p[1] = p[2];
			dpoint_sub(&geo->p[3], &geo->p[2], &p[2]);
			dpoint_add(&geo->p[0], &p[2], &geo->p[1]);
			break;
		case LDIM_FREE:
			geo->p[2] = p[0];
			geo->p[3] = p[1];
			geo->p[0] = geo->p[1] = p[2];
			if(!eq_line(&geo->p[2], &geo->p[0], &a, &b, &c))
				return FALSE;
			c = -(geo->p[3].x*a + geo->p[3].y*b);
			c1 = -(geo->p[0].x*(-b) + geo->p[0].y*a);
			if(!p_int_lines(a, b, c,  -b, a, c1,  &geo->p[1]))
				return FALSE;
			break;
		default:
			return FALSE;
			}
			geo->p[3].z = geo->p[2].z = geo->p[1].z = geo->p[0].z = 0.;
			if(dpoint_eq(&geo->p[1], &geo->p[0], eps_d))
				return FALSE;
			if(test_dim_to_invert(geo))
				invert_dim_points(geo);
			//geo->dgstyle = dim_info->dgs;
			return dim_grf_create(geo, font, text);
}

BOOL set_adim_geo(lpGEO_DIM geo_dim,lpFONT fnt, UCHAR *txt,
				  lpD_POINT f_p,  lpD_POINT s_p, lpD_POINT th_p,lpD_POINT four_p,
				  lpD_POINT p1, 
				  const sgCMatrix* plM, const sgCMatrix* invM,
				  short *errtype)

{
	D_POINT    p[5], wp, v1, v2 = {0, 0, 0};
	sgFloat     a1, b1, c1, a2, b2, c2, d1, d2, r, r1, r2, r3, ab, a;
	lpGEO_DIM  geo;
	short        i;
	UCHAR      inv = 0;
	BOOL       invp;

	lpFONT   font = fnt;
	UCHAR    *text = txt;

	geo = geo_dim;

	short  flag_was_gcond_flag_1 = LDIM_NEW;

	short  flag_was_gcond_flag_0 = (geo_dim->neg)?ADIM_NEGATIV:ADIM_POZITIV;

	*errtype = 0;

	if((flag_was_gcond_flag_1 == LDIM_ALONGC) ||  //  
		(flag_was_gcond_flag_1 == LDIM_LIDERC)){   //   
			geo->tcoeff = p1->x;
			geo->dgstyle.tplace = (UCHAR)((flag_was_gcond_flag_1 == LDIM_ALONGC) ? DTP_ALONG:DTP_LIDER);
			geo->dgstyle.flags &= (~DTP_AUTO);
			geo->numtp = 0;
			return dim_grf_create(geo, font, text);
		}
		if((flag_was_gcond_flag_1 == LDIM_ALONG) ||  //  
			(flag_was_gcond_flag_1 == LDIM_LIDER)){   //   
				gcs_to_dimpln(gcond.m[0], gcond.m[1], p1, &p[0],plM,invM);
				if(!get_arc_proection(&geo->p[4], &geo->p[0], &geo->p[1], geo->neg,
					&p[0], &p[1], &geo->tcoeff)) return FALSE;
				geo->dgstyle.tplace = (UCHAR)((gcond.flags[1] == LDIM_ALONG) ? DTP_ALONG:DTP_LIDER);
				geo->dgstyle.flags &= (~DTP_AUTO);
				geo->numtp = 0;
				if(!dim_grf_create(geo, font, text)) return FALSE;
				//set_echo_par(1, &geo->tcoeff);
				return TRUE;
			}
			if(flag_was_gcond_flag_1 == DIM_LIDER1){ //    
				if(!get_arc_parpoint(&geo->p[4], &geo->p[0], &geo->p[1], geo->neg,
					geo->tcoeff, &geo->tp[0], &a, &inv))
					return FALSE;
				o_hcncrd(gcond.m[1], p1, &geo->tp[1]);
				geo->tp[1].z = 0.;
				geo->numtp = 1;
				geo->dgstyle.tplace = DTP_LIDER;
				if(!dim_grf_create(geo, font, text)) return FALSE;
				return TRUE;
			}
			if(flag_was_gcond_flag_1 == DIM_LIDER2){ //    
				if(geo->numtp == 1)
					geo->tp[0] = geo->tp[1];
				o_hcncrd(gcond.m[1], p1, &geo->tp[1]);
				geo->tp[1].z = 0.;
				geo->numtp = 2;
				if(dpoint_eq(&geo->tp[0], &geo->tp[1], eps_d)){
					geo->tp[1] = geo->tp[0];
					if(!get_arc_parpoint(&geo->p[4], &geo->p[0], &geo->p[1], geo->neg,
						geo->tcoeff, &geo->tp[0], &a, &inv)) return FALSE;
					geo->numtp = 1;
				}
				return dim_grf_create(geo, font, text);
			}
			if(flag_was_gcond_flag_1 == DIM_TP1){ //   
				o_hcncrd(gcond.m[1], p1, &geo->tp[0]);
				if(!get_arc_proection(&geo->p[4], &geo->p[0], &geo->p[1], geo->neg,
					&geo->tp[0], &p[0], &a1)) return FALSE;
				if(!get_arc_parpoint(&geo->p[4], &geo->p[0], &geo->p[1], geo->neg,
					a1, &p[1], &a2, &inv)) return FALSE;
				r = dpoint_distance(&geo->p[0], &geo->p[1]);
				p[1].x = r*cos(a2); p[1].y = r*sin(a2); p[1].z = 0.;
				dpoint_add(&geo->tp[0], &p[1], &geo->tp[1]);
				geo->tp[0].z = geo->tp[1].z = 0.;
				geo->dgstyle.tplace = DTP_MANUAL;
				geo->dgstyle.flags &= (~DTP_AUTO);
				return dim_grf_create(geo, font, text);
			}

			if(flag_was_gcond_flag_1 == DIM_TP2){ //   
				o_hcncrd(gcond.m[1], p1, &geo->tp[1]);
				geo->tp[1].z = 0.;
				if(dpoint_eq(&geo->tp[1], &geo->tp[0], eps_d)){
					if(!get_arc_proection(&geo->p[4], &geo->p[0], &geo->p[1], geo->neg,
						&geo->tp[0], &p[0], &a1)) return FALSE;
					if(!get_arc_parpoint(&geo->p[4], &geo->p[0], &geo->p[1], geo->neg,
						a1, &p[1], &a2, &inv)) return FALSE;
					r = dpoint_distance(&geo->p[0], &geo->p[1]);
					p[1].x = r*cos(a2); p[1].y = r*sin(a2); p[1].z = 0.;
					dpoint_add(&geo->tp[0], &p[1], &geo->tp[1]);
					geo->tp[1].z = 0.;
				}
				return dim_grf_create(geo, font, text);
			}

			if(flag_was_gcond_flag_1 == LDIM_BASE ||
				flag_was_gcond_flag_1 == LDIM_CONT) { //      
					memcpy(geo->matr, gcond.m[0], sizeof(MATR));
					o_hcncrd(gcond.m[1], p1, &p[0]);
					geo->p[4] = gcond.p[4];

					get_dimarc_angles(&gcond.p[4], &gcond.p[0], &gcond.p[1],
						(UCHAR)gcond.flags[3], &ab, &a, &r);
					r1 = dpoint_distance(&geo->p[4], &p[0]);
					if(fabs(r1) < eps_d)
						return FALSE;
					if(gcond.flags[1] == LDIM_BASE){
						if(flag_was_gcond_flag_0){//  -   
							ab = ab + a;
							a = -a;
							geo->p[0] = gcond.p[1];
							geo->p[2] = gcond.p[3];
							geo->p[3] = p[0];
							p[1] = gcond.p[2];
						}
						else{  //  
							geo->p[0] = gcond.p[0];
							geo->p[2] = gcond.p[2];
							geo->p[3] = p[0];
							p[1] = gcond.p[3];
						}
						a1 = get_signed_angle(&geo->p[4], &geo->p[0], &p[0], sg(a, 0.));
						d1 = dim_info->bstep*get_grf_coeff();
						for(i = 0; i < 2; i++){
							r2 = r + d1;
							if(r2 < eps_d) continue;
							d1 = -d1;
							// 
							if(fabs(a1) > 0. && fabs(a1) < fabs(a)){
								if(r < max(r1, r2) && r > min(r1, r2)) continue;
							}
							else{
								r3 = dpoint_distance(&geo->p[4], &p[1]);
								if(r2 < max(r, r3) && r2 > min(r, r3)) continue;
							}
							//  
							geo->p[0].x = r2*cos(ab); geo->p[0].y = r2*sin(ab); geo->p[0].z = 0.;
							ab += a1;
							geo->p[1].x = r2*cos(ab); geo->p[1].y = r2*sin(ab); geo->p[1].z = 0.;
							dpoint_add(&geo->p[4], &geo->p[0], &geo->p[0]);
							dpoint_add(&geo->p[4], &geo->p[1], &geo->p[1]);
							geo->p[3].z = geo->p[2].z = geo->p[1].z = geo->p[0].z = geo->p[4].z;
							geo->neg = (UCHAR)((fabs(a1) > M_PI) ? 1: 0);
							if(test_dim_to_invert(geo))
								invert_dim_points(geo);
							geo->dgstyle = dim_info->dgs;
							return dim_grf_create(geo, font, text);
						}
						*errtype = 2;
						return FALSE;
					}
					else{  // 
						a1 = get_signed_angle(&geo->p[4], &gcond.p[0], &p[0], sg(a, 0.));
						if(fabs(a1) > -eps_n && fabs(a1) < fabs(a) + eps_n){
							*errtype = 1;  //  
							return FALSE;
						}

						if(flag_was_gcond_flag_0){//   
							a1 -= 2*M_PI;
							geo->p[0] = gcond.p[0];
							geo->p[2] = gcond.p[2];
							geo->p[3] = p[0];
						}
						else{  //  
							a1 -= a; ab += a;
							geo->p[0] = gcond.p[1];
							geo->p[2] = gcond.p[3];
							geo->p[3] = p[0];
						}

						ab += a1;
						geo->p[1].x = r*cos(ab); geo->p[1].y = r*sin(ab); geo->p[1].z = 0.;
						dpoint_add(&geo->p[4], &geo->p[1], &geo->p[1]);
						geo->p[3].z = geo->p[2].z = geo->p[1].z = geo->p[0].z = geo->p[4].z;
						geo->neg = (UCHAR)((fabs(a1) > M_PI) ? 1 : 0);
						invp = test_dim_to_invert(geo);
						if(invp)
							invert_dim_points(geo);
						geo->dgstyle = dim_info->dgs;
						geo->dgstyle.flags |= ((invp) ? EXT2_NONE : EXT1_NONE);
						if(geo->dgstyle.flags & DTP_AUTO)
							geo->dgstyle.tplace = (UCHAR)(((invp) ? DTP_LEFT:DTP_RIGHT));
						return dim_grf_create(geo, font, text);
					}
				}
				//  
				/*if(flag_was_gcond_flag_1 == LDIM_NEW3D){
					if(!eq_plane_ll((lpGEO_LINE)&gcond.p[0], (lpGEO_LINE)&gcond.p[2], &v2, &d2))
						return FALSE;
				}
				else
					normal_cpl(&v2);
				set_dim_matr(geo, &v2, p, gcond.maxcond, p1,plM,invM);
				for(i = 0; i <= gcond.maxcond; i++)
					p[i].z = 0.;*/

				set_dim_matr(geo, &v2, p, /*gcond.maxcond*/4, p1,plM,invM);

				p[0] = *f_p;
				p[1] = *s_p;
				p[2] = *th_p;
				p[3] = *four_p;
				p[4] = *p1;

				//   
				if(!eq_line(&p[0], &p[1], &a1, &b1, &c1))
					return FALSE;
				if(!eq_line(&p[2], &p[3], &a2, &b2, &c2))
					return FALSE;
				if(!p_int_lines(a1, b1, c1,  a2, b2, c2,  &geo->p[4]))
					return FALSE;

				//      
				r = dpoint_distance(&p[4], &geo->p[4]);
				if(fabs(r) < eps_d)
					return FALSE;
				d1 = -(p[4].x*a1 + p[4].y*b1);
				d2 = -(p[4].x*a2 + p[4].y*b2);
				if(!p_int_lines(a1, b1, c1,  a2, b2, d2,  &wp))
					wp = p[1];
				if(!normv(FALSE, dpoint_sub(&wp, &geo->p[4], &wp), &v1))
					return FALSE;
				if(!p_int_lines(a1, b1, d1,  a2, b2, c2,  &wp))
					wp = p[3];
				if(!normv(FALSE, dpoint_sub(&wp, &geo->p[4], &wp), &v2))
					return FALSE;
				geo->p[0] = geo->p[1] = geo->p[4];
				dpoint_add(&geo->p[4], dpoint_scal(&v1, r, &wp), &geo->p[0]);
				dpoint_add(&geo->p[4], dpoint_scal(&v2, r, &wp), &geo->p[1]);

				d1 = get_point_par(&p[0], &p[1], &geo->p[0]);
				geo->p[2] = (d1 > 0.5) ? p[1] : p[0];
				d1 = get_point_par(&p[2], &p[3], &geo->p[1]);
				geo->p[3] = (d1 > 0.5) ? p[3] : p[2];
				//geo->neg = (UCHAR)dim_info->adimflg;
				if(dpoint_eq(&geo->p[1], &geo->p[0], eps_d))
					return FALSE;
				if(test_dim_to_invert(geo))
					invert_dim_points(geo);
				//geo->dgstyle = dim_info->dgs;
				return dim_grf_create(geo, font, text);
}


