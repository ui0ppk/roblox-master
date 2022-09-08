#ifndef __SPACE_MATH_HDR__
#define __SPACE_MATH_HDR__


sgFloat d_radian(sgFloat degree);
sgFloat d_degree(sgFloat radian);
#define d_radian(degree) ((degree / 180.) * M_PI)
#define d_degree(radian) ((radian / M_PI) * 180.)

short       isg(short r);
short       sg(sgFloat r, sgFloat eps);
void      d_minmax(sgFloat * b, short numb,
                         sgFloat * min, sgFloat * max);
void      dpoint_minmax(lpD_POINT b, short numb,
                        lpD_POINT min, lpD_POINT max);
BOOL      dnormal_vector (lpD_POINT v);
BOOL      dnormal_vector1 (lpD_POINT v);
lpF_PLPOINT dpoint_to_plpoint(lpD_POINT p1, lpF_PLPOINT p2);
lpD_POINT dpoint_inv(lpD_POINT p1, lpD_POINT p2);
BOOL      dpoint_eq(lpD_POINT p1, lpD_POINT p2, sgFloat eps);
lpD_POINT dpoint_sub(lpD_POINT p1, lpD_POINT p2, lpD_POINT p3);
lpD_POINT dpoint_add(lpD_POINT p1, lpD_POINT p2, lpD_POINT p3);
lpD_POINT dpoint_parametr(lpD_POINT p1, lpD_POINT p2, sgFloat t, lpD_POINT p3);
sgFloat    dpoint_distance(lpD_POINT p1, lpD_POINT p2);
sgFloat    dskalar_product(lpD_POINT v1, lpD_POINT v2);
lpD_POINT dvector_product(lpD_POINT v1, lpD_POINT v2, lpD_POINT v3);
BOOL      fnormal_vector (lpF_POINT v);
float     fskalar_product(lpF_POINT v1, lpF_POINT v2);
lpF_POINT fvector_product(lpF_POINT v1, lpF_POINT v2, lpF_POINT v3);

void      define_eps(lpD_POINT min,lpD_POINT max);

BOOL gauss_classic( sgFloat **A, sgFloat *y, int u );

BOOL  dpoint_cplane(lpD_POINT p1, lpD_POINT p2, lpD_POINT p3,lpD_POINT pr);
float calc_body_angle(lpD_POINT p1, lpD_POINT p2, lpD_POINT p3,lpD_POINT p4);
void modify_limits_by_point(lpD_POINT p, lpD_POINT min, lpD_POINT max);
sgFloat round(sgFloat din, short precision);

//  line via 2 points (without z)
BOOL eq_line(lpD_POINT p1, lpD_POINT p2, sgFloat *a, sgFloat *b, sgFloat *c);

//  line perpendicular to the middle of segment
BOOL eq_pe_line(lpD_POINT p1, lpD_POINT p2, sgFloat *a, sgFloat *b, sgFloat *c);

// two lines intersection
short p_int_lines(sgFloat a1, sgFloat b1, sgFloat c1,
                sgFloat a2, sgFloat b2, sgFloat c2,
                lpD_POINT p);

BOOL normv(BOOL flag_3d, lpD_POINT v, lpD_POINT n);


sgFloat v_angle(sgFloat x, sgFloat y);
// angle beetween X and vector

void min_max_2(sgFloat x1, sgFloat x2, sgFloat *mn, sgFloat *mx);

sgFloat dist_p_l(lpD_POINT vl, lpD_POINT cl, lpD_POINT p, lpD_POINT pl);
// distance from point to line
// (vl) - point on line, (cl) - line vector
// (3D)
// (pl) - projection of point

short intersect_3d_pl(lpD_POINT np, sgFloat dp, lpD_POINT vl, lpD_POINT cl,
                    lpD_POINT p);
//  intersect plane and line

//  (np, dp)         - plane
//  (vl)             - line point
//  (cl)             - line vector
//  (p)              - intersection point

//  Result:     1 - intersect
//               0 - line on plane
//              -1 - line || plane


BOOL eq_plane(lpD_POINT p1, lpD_POINT p2, lpD_POINT p3,
             lpD_POINT np, sgFloat *dp);
// normal plane eq

sgFloat dist_p_pl(lpD_POINT p, lpD_POINT n, sgFloat d);

sgFloat dcoef_pl(lpD_POINT p, lpD_POINT n);
// PLane D coeff

short best_crd_pln(lpD_POINT n);
short coll_tst(lpD_POINT v1, BOOL norm1, lpD_POINT v2, BOOL norm2);
sgFloat det3_3(sgFloat *a, sgFloat *b, sgFloat *c);

short qwadr_eq(sgFloat a, sgFloat b, sgFloat c, sgFloat* x);
// a*x*x + b*x +c = 0

sgFloat dpoint_distance_2d(lpD_POINT p1, lpD_POINT p2);
lpD_POINT dpoint_scal(lpD_POINT p1, sgFloat r, lpD_POINT p2);
sgFloat dist_point_to_segment(lpD_POINT p, lpD_POINT v1, lpD_POINT v2,
                          lpD_POINT p_min);


sgFloat floor_crd_par(sgFloat crd, sgFloat step);

//--fast2daf.c
#define FAF_MOVE    0
#define FAF_ROTATE  1

void faf_init(void);
void faf_set(short type, sgFloat x, sgFloat y);
void faf_point(lpD_POINT p1, lpD_POINT p2);
void faf_line(lpD_POINT p1, lpD_POINT p2);
void faf_inv(lpD_POINT p1, lpD_POINT p2);
void faf_exe(lpD_POINT p1, lpD_POINT p2);


sgFloat get_point_par(lpD_POINT pb, lpD_POINT pe, lpD_POINT p);
sgFloat get_point_par_3d(lpD_POINT pb, lpD_POINT pe, lpD_POINT p);


BOOL get_point_proection(lpD_POINT pb, lpD_POINT pe, lpD_POINT p,
                         lpD_POINT pp);

BOOL  correct_line_tails(lpD_POINT p1, lpD_POINT p2, sgFloat h1, sgFloat h2);


// poly from Borlnd
sgFloat z_poly(sgFloat x, int degree, sgFloat* coeffs);


bool Is2DPointInside2DPolygon(const SG_POINT *polygon,int N, const SG_POINT& p);

bool CalcPointOf3PlanesInter(lpD_PLANE pPln1, lpD_PLANE pPln2, lpD_PLANE pPln3, lpD_POINT pPnt);


typedef struct {
  D_POINT        p;
  D_POINT        v;
}D_AXIS, *pD_AXIS;


bool CalcAxisOfPlanesInter(lpD_PLANE pPln1, lpD_PLANE pPln2, pD_AXIS pAx);




BOOL   o_trans_vector(lpMATR t,lpD_POINT base, lpD_POINT vector);
BOOL   o_hcncrd(lpMATR t,lpD_POINT v,lpD_POINT vp);
BOOL   o_hcncrdf(lpMATR t,lpD_POINT v,lpF_POINT vp);
BOOL   o_hcncrd2(lpMATR t,lpD_POINT v,lpF_PLPOINT vp);
BOOL   o_move_xyz(lpMATR matr, lpD_POINT a,lpD_POINT p);
BOOL   o_rotate_xyz(lpMATR matr,lpD_POINT p,lpD_POINT a,
                              sgFloat alfa);
BOOL   o_get_param(lpMATR matr,lpD_POINT p,
                           sgFloat  *az1,sgFloat  *ax,sgFloat  *az2,
                           sgFloat  *sx,sgFloat  *sy,sgFloat  *sz);
BOOL   o_rot(lpD_POINT a,lpD_POINT c, sgFloat  fi,lpD_POINT b);
BOOL   o_minv(lpMATR a,lpMATR ai);
BOOL   o_hcrot1(lpMATR t,lpD_POINT p);
BOOL   o_hcrot_angle(lpD_POINT p,sgFloat * axp,sgFloat * azp);
void   o_hcmult(lpMATR a, lpMATR b);
void   o_tdrot(lpMATR t, int naxes, sgFloat alpha);
void   o_tdscal(lpMATR t, int naxis, sgFloat scale);
void   o_scale( lpMATR t,sgFloat scale);
void   o_tdtran(lpMATR t,lpD_POINT v);
void   o_hcunit(lpMATR matr);
BOOL   o_hcprsp(lpMATR matr,sgFloat h);


#define   CAM_AXONOM  0 
#define   CAM_VIEW    1 

typedef enum {  
  VCS_BOX,  
  GCS_BOX,  
  DEFAULT_SCALE,
  } SCALE_TYPE;

void   f3d_gabar_project1(lpMATR matr,lpD_POINT min1, lpD_POINT max1,
                                               lpD_POINT min2, lpD_POINT max2);

void get_box_vertex(lpD_POINT p1, lpD_POINT p2, short num, lpD_POINT v);


#endif
