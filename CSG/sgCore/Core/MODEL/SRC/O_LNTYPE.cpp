#include "../../sg.h"

BOOL init_ltype_info(lpLTYPE_INFO lti, UCHAR lt, sgFloat lobj, BOOL closefl)
{
lpLINESHP   shp;
ULONG       len;

	shp = (lpLINESHP)ft_bd->buf;
	if(!load_ft_sym(FTLTYPE, (WORD)lt, shp, &len)) return FALSE;
	if(!len)                                       return FALSE;
	return init_ltype_info1(lti, shp, lobj, closefl);
}

BOOL init_ltype_info1(lpLTYPE_INFO lti, lpLINESHP shp, sgFloat lobj, BOOL closefl)
{
short       i;
sgFloat      lstep, l, d;

	if(shp->npar < 2)                              return FALSE;
	lstep = 0;
	for(i = 0; i < shp->npar; i++){
		lti->s[i] = shp->s[i];
		if(fabs(lti->s[i]) < eps_d) lti->s[i] = dim_info->lpunkt;
		lti->s[i] *= get_grf_coeff();
		lstep += fabs(lti->s[i]);
	}
	d = (closefl) ? 0. : lti->s[0];
	l = lobj - d;
	if(l <= eps_d)                                 return FALSE;
	l = floor(l/lstep + 0.5);
	l = lobj/(l*lstep + d);
	if(l < 0.5)                                    return FALSE;
	for(i = 0; i < shp->npar; i++){
		lti->s[i] *= l;
		if(fabs(lti->s[i]) < eps_d)                  return FALSE;
	}
	lti->n = shp->npar;
	lti->i = 0;
	lti->ls = lti->s[0];
	lti->flag = TRUE;
	lti->lotr  = 0.;
	lti->flagnl = FALSE;
	return TRUE;
}

BOOL get_next_shtrih_line(lpLTYPE_INFO lti, lpD_POINT v1, lpD_POINT v2)
{
BOOL ret;

	if(lti->flagnl) {
		lti->flagnl = FALSE;
		return FALSE;
	}

	if(lti->lotr <= 0.){
		lti->lotr = dpoint_distance(v1, v2);
		lti->v1 = *v1;
		lti->v2 = *v2;
	}
met:
	if(lti->ls == 0.){
		lti->i++;
		if(lti->i >= lti->n) lti->i = 0;
		lti->ls = fabs(lti->s[lti->i]);
		lti->flag = (lti->s[lti->i] < 0.) ? FALSE : TRUE;
	}

	if(lti->ls >= lti->lotr){ //   
		lti->ls -= lti->lotr;
		*v1 = lti->v1;
		*v2 = lti->v2;
		if(lti->lotr < eps_d) ret = FALSE;
		else                  ret = (lti->flag) ? TRUE:FALSE;
		lti->lotr = 0.;
		lti->flagnl = ret;
		return ret;
	}
	else {
		if(lti->lotr >= eps_d){
			dpoint_parametr(&lti->v1, &lti->v2, lti->ls/lti->lotr, v2);
			*v1 = lti->v1;
			lti->v1 = *v2;
		}
		lti->lotr -= lti->ls;
		if(lti->lotr < eps_d){
			lti->flagnl = TRUE;
			*v2 = lti->v2;
			lti->ls += lti->lotr;
			lti->lotr = 0.;
			ret = (lti->flag && (lti->ls >= eps_d)) ? TRUE:FALSE;
			lti->ls = 0.;
			lti->flagnl = ret;
			return ret;
		}
		lti->ls = 0.;
		if(lti->flag) return TRUE;
		goto met;
	}
}

