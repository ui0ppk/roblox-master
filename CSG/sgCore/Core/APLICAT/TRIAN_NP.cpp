#include "../sg.h"

BOOL (*trian_pre_brep)  (hOBJ hobj)                                    = NULL;
BOOL (*trian_post_brep) (BOOL ret, hOBJ hobj)                          = NULL;
BOOL (*trian_pre_np)    (lpNPW npw, short num_np)                        = NULL;
BOOL (*trian_post_np)   (BOOL ret, lpNPW npw, short num_np)              = NULL;
BOOL (*trian_pre_face)  (lpNPW npw, short num_np, short numf)              = NULL;
BOOL (*trian_put_face)  (lpNPW npw, short numf, short lv, short *v, short *vi) = NULL;
BOOL (*trian_post_face) (BOOL ret, lpNPW npw, short num_np, short numf)    = NULL;

lpTR_INFO tr_info = NULL;
BOOL      trian_error = FALSE;  

BOOL trian_alloc(lpNPW npw, short num_np, short max_v);
void trian_free(void);
//static BOOL trian_np(lpNPW npw, short num_np, short max_v);
static BOOL trian_face(short num_f);
static void form_tr_info(short numf);
static void beg_vcont(short nc);
static void proect_face(void);
static BOOL put_simple_face(short lv, short i1, short i2, short i3, short i4,
														short flag_vi);
static BOOL triangl(void);
static BOOL tr_test3(short ib, short is, short ie);
static BOOL tr_coeffp(sgFloat x1, sgFloat y1, sgFloat x2, sgFloat y2,
											sgFloat *a, sgFloat *b, sgFloat *c);
static void tr_del_pc(short i);
static BOOL trian_delete_hole(void);
static void serch_max_p(short *nc, short *nt);
static BOOL serch_razrez(short nc, short nt);
static BOOL test_in(short is, short it);
static void razrez(short iv, short nd, short id);


#define TR_INVISIBLE  0
#define TR_VISIBLE    1
#define TR_EPS_GAB    1.e-7

BOOL trian_brep(hOBJ hobj, short max_v)
{
SCAN_CONTROL sc;
short          info[2];
BOOL ret;

	if(trian_pre_brep)
		if(!trian_pre_brep(hobj))return FALSE;
	init_scan(&sc);
	sc.data = &info;
  sc.user_geo_scan = trian_geo_scan;
	info[0] = 0; info[1] = max_v;
	ret = !(o_scan(hobj, &sc) == OSFALSE);
  if(trian_post_brep) ret = trian_post_brep(ret, hobj);
	return ret;
}
#pragma argsused
OSCAN_COD trian_geo_scan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
short i;
short *info;

	if( lpsc->type != OBREP ) return OSFALSE;
	for(i = 1; i <= npwg->nov ; i++)    
		o_hcncrd(lpsc->m, &npwg->v[i], &npwg->v[i]);
	if(!np_cplane(npwg)) return OSFALSE; 
	info = (short*)lpsc->data;
	if(!trian_np(npwg, info[0], info[1])) return OSFALSE;
	info[0]++;
	return OSTRUE;
}

BOOL trian_alloc(lpNPW npw, short num_np, short max_v){

	if ((tr_info = (TR_INFO*)SGMalloc(sizeof(TR_INFO))) == NULL) goto err;
	tr_info->npw    = npw;
	tr_info->num_np = num_np;
	tr_info->max_v  = max_v;
	if((tr_info->mb  = (short*)SGMalloc(sizeof(short)*(npw->noc + 1))) == NULL)        goto err;
	if((tr_info->me  = (short*)SGMalloc(sizeof(short)*(npw->noc + 1))) == NULL)        goto err;
	if((tr_info->m   = (short*)SGMalloc(sizeof(short)*(npw->nov + npw->noe))) == NULL) goto err;
	if((tr_info->mv  = (short*)SGMalloc(sizeof(short)*(npw->nov + npw->noe))) == NULL) goto err;
	if((tr_info->mr  = (short*)SGMalloc(sizeof(short)*(npw->nov + npw->noe))) == NULL) goto err;
	if((tr_info->mrv = (short*)SGMalloc(sizeof(short)*(npw->nov + npw->noe))) == NULL) goto err;
	if((tr_info->mpr = (UCHAR*)SGMalloc(npw->nov + 1)) == NULL)                      goto err;
	if((tr_info->x   = (sgFloat*)SGMalloc(sizeof(sgFloat)*(npw->nov + 1))) == NULL)     goto err;
	if((tr_info->y   = (sgFloat*)SGMalloc(sizeof(sgFloat)*(npw->nov + 1))) == NULL)     goto err;
  return TRUE;
err:
  trian_free();
  return FALSE;
}

void trian_free(void){
	if(tr_info){
		if(tr_info->mb)  SGFree(tr_info->mb);
		if(tr_info->me)  SGFree(tr_info->me);
		if(tr_info->m)   SGFree(tr_info->m);
		if(tr_info->mv)  SGFree(tr_info->mv);
    if(tr_info->mr)  SGFree(tr_info->mr);
		if(tr_info->mrv) SGFree(tr_info->mrv);
		if(tr_info->mpr) SGFree(tr_info->mpr);
		if(tr_info->x)   SGFree(tr_info->x);
		if(tr_info->y)   SGFree(tr_info->y);
		SGFree(tr_info);
		tr_info = NULL;
	}
}

BOOL trian_np(lpNPW npw, short num_np, short max_v)
{
BOOL ret = FALSE;
short  num_f;

	if(trian_pre_np) if(!trian_pre_np(npw, num_np)) return FALSE;
	if(trian_alloc(npw, num_np, max_v)){
		for(num_f = 1; num_f <= npw->nof; num_f++) {
			if(!trian_face(num_f)) goto met_end;
		}
		ret = TRUE;
	}
met_end:
	trian_free();
	if(trian_post_np) ret = trian_post_np(ret, npw, num_np);
	return ret;
}
#pragma argsused
BOOL fly_trian_face( lpNPW npw, short num_f, short num_np, short max_v)
{
BOOL ret = FALSE;

//	if(!trian_alloc(npw, num_np, max_v)) return ret;
	if(!trian_face(num_f)) goto met_end;
	ret = TRUE;
met_end:
//	trian_free();
	return ret;
}


static BOOL trian_face(short num_f) 
{
BOOL ret = FALSE;

	form_tr_info(num_f);
	if(trian_pre_face)
		if(!trian_pre_face(tr_info->npw, tr_info->num_np, num_f)) goto met_end;

	if(tr_info->num_cf == 1){
		if(tr_info->me[0] < 2) goto met_end;
		if(tr_info->me[0] < tr_info->max_v){
			beg_vcont(0);
			ret = put_simple_face(tr_info->l_vcont, 0, 1, 2, 3, TR_VISIBLE);
			goto met_end;
		}
    proect_face();
		beg_vcont(0);
	}
	else{
    proect_face();
    if(!trian_delete_hole()){
      trian_error = TRUE;  
      goto met_end;
    }
	}
  ret = triangl();
met_end:
  if(trian_post_face)
    ret = trian_post_face(ret, tr_info->npw, tr_info->num_np, num_f);
  return ret;
}

static void form_tr_info(short numf)
{
short    numc, nume, e_first, j = 0;
lpNPW  npw = tr_info->npw;

	tr_info->num_f  = numf;
	tr_info->num_cf = 0;

	numc = npw->f[numf].fc;  

	while(numc){
		nume = npw->c[numc].fe;
		e_first = abs(nume);
		if(nume < 0)
			tr_info->m[j] = npw->efr[-nume].ev;
		else
			tr_info->m[j] = abs(npw->efr[nume].bv);

    tr_info->mr[j] = e_first;
		tr_info->mb[tr_info->num_cf] = j;

    while(TRUE) {
			if(nume < 0){
				tr_info->m[++j] = abs(npw->efr[-nume].bv);
				nume = npw->efc[-nume].em;
			}
			else{
				tr_info->m[++j] = npw->efr[nume].ev;
				nume = npw->efc[nume].ep;
			}
      if(abs(nume) != e_first) tr_info->mr[j] = abs(nume);
      else                     break;
    }

		tr_info->me[tr_info->num_cf] = j - 1;
		(tr_info->num_cf)++;
		numc = npw->c[numc].nc;
	}
}

static void beg_vcont(short nc)
{

short i = 0;
short j;

  for(j = tr_info->mb[nc]; j <= tr_info->me[nc]; j++){
    tr_info->mv[i] = tr_info->m[j];
    tr_info->mrv[i++] = tr_info->mr[j];
	}
  tr_info->l_vcont = i;
  tr_info->me[nc] *= -1;

}

static void proect_face(void)
{
lpNPW   npw = tr_info->npw;
short     i, iv;
sgFloat  xmin, xmax, ymin, ymax, dxx, dyy;
D_POINT p;
MATR    m;

	o_hcunit(m);
	o_hcrot1(m, &(npw->p[tr_info->num_f].v));

	iv = tr_info->m[0];
	tr_info->mpr[iv] = 0;
	o_hcncrd(m, &(npw->v[iv]), &p);
	xmin = xmax = tr_info->x[iv] = p.x;
	ymin = ymax = tr_info->y[iv] = p.y;

	for(i = 1; i <= tr_info->me[tr_info->num_cf - 1]; i++){
		iv = tr_info->m[i];
		tr_info->mpr[iv] = 0;
		o_hcncrd(m, &(npw->v[iv]), &p);
		tr_info->x[iv] = p.x;
		tr_info->y[iv] = p.y;
		xmin = min(tr_info->x[iv], xmin);
		xmax = max(tr_info->x[iv], xmax);
		ymin = min(tr_info->y[iv], ymin);
		ymax = max(tr_info->y[iv], ymax);
	}

	dxx = (xmax + xmin)/2;
	dyy = (ymax + ymin)/2;

	for(i = 0; i <= tr_info->me[tr_info->num_cf - 1]; i++){
		iv = tr_info->m[i];
		if(tr_info->mpr[iv]) continue;
		tr_info->mpr[iv] = 1;
		tr_info->x[iv] -= dxx;
		tr_info->y[iv] -= dyy;
	}
	tr_info->epsf = max((xmax - xmin)/2, (ymax - ymin)/2)*TR_EPS_GAB;
}

static BOOL put_simple_face(short lv, short i1, short i2, short i3, short i4,
														short flag_vi)
{

short i, v[4], vi[4];

  v[0] = i1; v[1] = i2; v[2] = i3; if(lv == 4) v[3] = i4;

   for(i = 0; i < lv; i++){
    vi[i] = tr_info->mrv[v[i]];
		v[i] = tr_info->mv[v[i]];
  }
	if(flag_vi == TR_INVISIBLE) vi[lv - 1] = TR_INVISIBLE;
	if(trian_put_face)
    return trian_put_face(tr_info->npw, tr_info->num_f, lv, v, vi);
	 return TRUE;
}

static BOOL triangl(void)
{
short ib, is, ie, ie2, kv, nkus;

  for( ib = 0, nkus = 0; tr_info->l_vcont > tr_info->max_v; ib++, nkus++){

    if(nkus > tr_info->l_vcont){
      trian_error = TRUE;  
      return FALSE;
		}


    ib %= tr_info->l_vcont;
    is = (ib + 1) % tr_info->l_vcont;
    ie = (is + 1) % tr_info->l_vcont;
	ie2 = 0;



    if(!tr_test3(ib, is, ie)) continue;

		if(tr_info->max_v == 4){
			
      ie2 = (ie + 1) % tr_info->l_vcont;
      kv = (tr_test3(ib, ie, ie2)) ? 4 : 3;
    }
    else kv = 3;

		if(!put_simple_face(kv, ib, is, ie, ie2, TR_INVISIBLE)) return FALSE;
		nkus = 0;
    tr_info->mrv[ib] = TR_INVISIBLE;
    tr_del_pc(is);
		if(kv == 4)
      tr_del_pc((short)(is % tr_info->l_vcont));
	}
  return put_simple_face(tr_info->l_vcont, 0, 1, 2, 3, TR_VISIBLE);
}

static BOOL tr_test3(short ib, short is, short ie)
{
sgFloat a1, b1, c1;
sgFloat a2, b2, c2;
sgFloat a3, b3, c3;
short    jb, js, je, i, j;

  jb = tr_info->mv[ib];
  js = tr_info->mv[is];
	je = tr_info->mv[ie];


	if(!tr_coeffp(tr_info->x[jb], tr_info->y[jb], tr_info->x[je], tr_info->y[je],
								&a1, &b1, &c1)) return FALSE;
	if(a1*tr_info->x[js] + b1*tr_info->y[js] + c1 <= tr_info->epsf) return FALSE;



	if(!tr_coeffp(tr_info->x[jb], tr_info->y[jb], tr_info->x[js], tr_info->y[js],
								&a2, &b2, &c2)) return FALSE;
	if(!tr_coeffp(tr_info->x[js], tr_info->y[js], tr_info->x[je], tr_info->y[je],
								&a3, &b3, &c3)) return FALSE;

	for (i = 0; i < tr_info->l_vcont; i++){
		j = tr_info->mv[i];
		if(j == jb || j == js || j == je) continue;
    if(a1*tr_info->x[j] + b1*tr_info->y[j] + c1 < -tr_info->epsf) continue;
    if(a2*tr_info->x[j] + b2*tr_info->y[j] + c2 >  tr_info->epsf) continue;
    if(a3*tr_info->x[j] + b3*tr_info->y[j] + c3 >  tr_info->epsf) continue;
		
		return FALSE;
	}
	return TRUE;
}

static BOOL tr_coeffp(sgFloat x1, sgFloat y1, sgFloat x2, sgFloat y2,
                      sgFloat *a, sgFloat *b, sgFloat *c){

sgFloat aw, bw, r;
  aw = y2 - y1; bw = x1 - x2; *c = y1*x2 - x1*y2;
  if((r = hypot(aw, bw)) < tr_info->epsf) return FALSE;
  *a = aw/r; *b = bw/r; *c = *c/r;
  return TRUE;
}

static void tr_del_pc(short i){

  tr_info->l_vcont--;

  while(i < tr_info->l_vcont){
    tr_info->mv[i]  = tr_info->mv [i + 1];
    tr_info->mrv[i] = tr_info->mrv[i + 1];
		i++;
	}
}

static BOOL trian_delete_hole(void)
{

short nc, nt, num_hole;

	serch_max_p(&nc, &nt); 
	beg_vcont(nc);
  num_hole = tr_info->num_cf - 1;

	while(num_hole){
		serch_max_p(&nc, &nt); 
    if(!serch_razrez(nc, nt)) return FALSE;
		num_hole--;
	}
  return TRUE;
}

static void serch_max_p(short *nc, short *nt){

short    i, j, iv;
BOOL   flag = FALSE;
sgFloat xmax = 0;

  for(i = 0; i < tr_info->num_cf; i++){
    if(tr_info->me[i] < 0) continue;
    for(j = tr_info->mb[i]; j <= tr_info->me[i]; j++){
      iv = tr_info->m[j];
			if(flag)
        if(tr_info->x[iv] <= xmax) continue;
      xmax = tr_info->x[iv];
			*nc = i;
			*nt = j;
      flag = TRUE;
		}
	}
}

static BOOL serch_razrez(short nc, short nt){

short i, j, flag, vb, ve, vv, vt, in;
sgFloat xmn, xmx, xmxh, a, b, c, a1, b1, c1, db, de;

  for(i = tr_info->mb[nc]; i <= tr_info->me[nc]; i++)
    for(j = 0; j < tr_info->l_vcont; j++)
      if(tr_info->m[i] == tr_info->mv[j]){
        in = (i == tr_info->me[nc]) ? tr_info->mb[nc] : i + 1;
        if(!test_in(j, tr_info->m[in])) continue;
				razrez(j, nc, i);
        return TRUE;
			}

  vt = tr_info->m[nt];
  xmxh = tr_info->x[vt] + tr_info->epsf;
  xmn = tr_info->x[vt] - tr_info->epsf;

  for(i = 0; i < tr_info->l_vcont; i++){
    vv = tr_info->mv[i];
    if(tr_info->x[vv] < xmxh) continue;

    if(!test_in(i, vt)) continue;

    if(!tr_coeffp(tr_info->x[vt], tr_info->y[vt],
                  tr_info->x[vv], tr_info->y[vv], &a, &b, &c)) continue;
    xmx = tr_info->x[vv] + tr_info->epsf;
		flag = 1;
    for(j = 0; j < tr_info->l_vcont; j++){

      vb = tr_info->mv[j];
      ve = tr_info->mv[(j + 1) % tr_info->l_vcont];

      if((tr_info->x[vb] <= xmn) && (tr_info->x[ve] <= xmn)) continue;
      if((tr_info->x[vb] >= xmx) && (tr_info->x[ve] >= xmx)) continue;

      db = a*tr_info->x[vb] + b*tr_info->y[vb] + c;
      de = a*tr_info->x[ve] + b*tr_info->y[ve] + c;
      if(db <= -tr_info->epsf && de <= -tr_info->epsf) continue;
      if(db >= tr_info->epsf && de >= tr_info->epsf)   continue;

			if(vb  == vv){
        if(ve == vb) continue;
        if(fabs(de) >= tr_info->epsf) continue;
        if(tr_info->x[ve] >= xmx)     continue;
        else  goto met;
			}

			if(ve  == vv){
        if(fabs(db) >= tr_info->epsf) continue;
        if(tr_info->x[vb] >= xmx)     continue;
        else goto met;
			}

      if(!tr_coeffp(tr_info->x[vb], tr_info->y[vb],
                    tr_info->x[ve], tr_info->y[ve], &a1, &b1, &c1)) goto met;
      if(fabs(db = a*b1 - a1*b) < eps_n*eps_n) continue;
			de = (b*c1 - b1*c)/db;
      if(de <= xmn || de >= xmx) continue;
met:
			flag = 0;
			break;
		}
		if(flag){
			razrez(i, nc, nt);
      return TRUE;
		}
	}

  return FALSE;
}

static BOOL test_in(short is, short it){

sgFloat a1, b1, c1;
sgFloat a2, b2, c2;
sgFloat a3, b3, c3;
sgFloat d1, d2;
short ib, ie;

		ib = ie = is;
		do {
      ib = (ib - 1 + tr_info->l_vcont) % tr_info->l_vcont;
    } while(tr_info->mv[ib] == tr_info->mv[is]);

		do {
      ie = (ie + 1) % tr_info->l_vcont;
    } while(tr_info->mv[ie] == tr_info->mv[is]);

    is = tr_info->mv[is];
    ib = tr_info->mv[ib];
    ie = tr_info->mv[ie];

  if(!tr_coeffp(tr_info->x[ib], tr_info->y[ib], tr_info->x[ie], tr_info->y[ie],
                &a1, &b1, &c1)) return FALSE;
  if(!tr_coeffp(tr_info->x[ib], tr_info->y[ib], tr_info->x[is], tr_info->y[is],
                &a2, &b2, &c2)) return FALSE;
  if(!tr_coeffp(tr_info->x[is], tr_info->y[is], tr_info->x[ie], tr_info->y[ie],
                &a3, &b3, &c3)) return FALSE;
  d1 = a2*tr_info->x[it] + b2*tr_info->y[it] + c2;
  d2 = a3*tr_info->x[it] + b3*tr_info->y[it] + c3;

  if(a1*tr_info->x[is] + b1*tr_info->y[is] + c1 >= 0.){ 
    if(d1 <= -tr_info->epsf && d2 <= -tr_info->epsf) return TRUE;
    else return FALSE;
	}
	else{                                 
    if(d1 >= tr_info->epsf && d2 >= tr_info->epsf) return FALSE;
    else return TRUE;
	}
}

static void razrez(short iv, short nd, short id){

short sdw, i, j;

  sdw = tr_info->me[nd] - tr_info->mb[nd] + 3;
  for(i = tr_info->l_vcont - 1; i > iv; i--){
    tr_info->mv[i + sdw] = tr_info->mv[i];
    tr_info->mrv[i + sdw] = tr_info->mrv[i];
	}


  for(i = iv + 1, j = id; j <= tr_info->me[nd]; i++, j++){
    tr_info->mv[i] = tr_info->m[j];
    tr_info->mrv[i] = tr_info->mr[j];
	}


  for(j = tr_info->mb[nd]; j < id; i++, j++){
    tr_info->mv[i] = tr_info->m[j];
    tr_info->mrv[i] = tr_info->mr[j];
	}


  tr_info->mv[i] = tr_info->m[id];
  tr_info->mrv[i++] = TR_INVISIBLE;
  tr_info->mv[i] = tr_info->mv[iv];
  tr_info->mrv[i] = tr_info->mrv[iv];
  tr_info->mrv[iv] = TR_INVISIBLE;

  tr_info->me[nd] = -tr_info->me[nd];
  tr_info->l_vcont += sdw;
}
