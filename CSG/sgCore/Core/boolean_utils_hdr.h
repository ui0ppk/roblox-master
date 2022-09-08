#ifndef __BOOLEAN_UTILS__HDR__
#define __BOOLEAN_UTILS__HDR__



typedef enum {UNION,               
              INTER,              
              SUB,                 
              LINE_INTER,          
              PENETRATE,          
              INTER_POINT          
} CODE_OPERATION;

typedef enum { B_IN_IN,   
               B_IN_ON,  
               B_ON_IN,   
               B_ON_ON    
} BO_POINT_TYPE;

typedef struct {
  D_POINT       p;          
  D_POINT       p1;         
  D_POINT       p2;         
  BO_POINT_TYPE type;      
  sgFloat        t;          
} BO_INTER_POINT;

typedef BO_INTER_POINT    * lpBO_INTER_POINT;

OSCAN_COD boolean_operation(hOBJ hobj_a, hOBJ hobj_b, CODE_OPERATION, lpLISTH listh);

BOOL bo_line_object(lpD_POINT pb, lpD_POINT pe, hOBJ hobj, lpVDIM point);

BOOL bo_arc_object(lpGEO_ARC arc, lpREGION_3D gab_arc, hOBJ hbody, lpVDIM vdim);
BOOL bo_points(hOBJ hobj_l, hOBJ hobj_b, lpVDIM points, CODE_OPERATION code_operation,
               lpLISTH listh);
BOOL cut(lpLISTH list, NUM_LIST num_list, lpD_PLANE pl, lpLISTH listh_cut);
BOOL check_point_object(lpD_POINT p, hOBJ hobj, lpNP_TYPE_POINT answer);
BOOL check_and_edit_model(hOBJ hobj, hOBJ *hnew_obj, BOOL * pr);

extern BOOL Bo_Put_Message;



typedef enum {
  CND_NODEF = 0x0000,
  CND_START = 0x0001,
  CND_END   = 0x0002,
  CND_NEAR  = 0x0004,
  CND_CENTER= 0x0008,
  CND_LENGTH= 0x0010,
  CND_RADIUS= 0x0020,
  CND_ANGLE = 0x0040,
  CND_VECTOR= 0x0080,
  CND_TANG  = 0x1000,
  CND_APOINT= 0x2000,
} CNDTYPE;  

#define CND_POINT (CND_START|CND_END|CND_NEAR|CND_CENTER|CND_VECTOR|CND_APOINT)
#define CND_SCAL  (CND_LENGTH|CND_RADIUS|CND_ANGLE)

typedef CNDTYPE  * lpCNDTYPE;

#define MAX_GEO_MATR  4

typedef struct {
  short         maxcond; 
  short *    flags;   
  sgFloat * scal;    
  lpCNDTYPE   ctype;   
  lpOBJTYPE   type;   
  lpGEO_ARC   geo;    
  lpD_POINT   p;       
  lpD_PLANE   pl;      
  lpMATR    m[MAX_GEO_MATR];  
  hOBJ        *hobj;   
  void     *user;   
} GEO_COND; 


#define  PL_NODEF   0 
#define  PL_FIXED   1  
#define  PL_CURR    2 
#define  PL_DEF     3  


extern CODE_MSG el_geo_err;
extern MATR     el_g_e, el_e_g;
extern GEO_ARC  el_geo1, el_geo2;

extern GEO_ARC  fr_geo;
extern D_POINT  fr_point;
extern OBJTYPE  fr_type;
extern BOOL     fr_mod;
extern GEO_COND gcond;
extern BOOL     true_geo_flag;

//--ne_to_vp.c
lpD_POINT nearest_to_vp(lpD_POINT vp, lpD_POINT p, short kp, lpD_POINT np);
short       num_nearest_to_vp(lpD_POINT vp, lpD_POINT p, short kp);
lpD_POINT nearest_to_vp_2d(lpD_POINT vp, lpD_POINT p, short kp, lpD_POINT np);
sgFloat    add_distans_to_vp(lpD_POINT vp, lpD_POINT p, short kp);

//--geosvp.c
BOOL eq_plane_pl(lpD_POINT p, lpGEO_LINE l, lpD_POINT np, sgFloat *dp);
short eq_plane_ll(lpGEO_LINE l1, lpGEO_LINE l2, lpD_POINT np, sgFloat *dp);
lpD_POINT dpoint_copy(lpD_POINT pnew, lpD_POINT pold);
lpD_POINT dpoint_project(lpD_POINT p, lpD_POINT pp, short axis);
lpD_POINT dpoint_reconstruct(lpD_POINT pp, lpD_POINT p, short axis, sgFloat d);

//--test_wrd.c
BOOL test_wrd_line(lpGEO_LINE l);
BOOL test_wrd_arc(lpGEO_ARC a, BOOL *flag_arc);

//--test_pe.c
BOOL test_in_pe_2d(lpD_POINT p, OBJTYPE type, BOOL flag_pr, void  *geo);
BOOL test_in_pa_2d(lpD_POINT p, lpGEO_ARC a);
short test_in_arrey(lpD_POINT p, short *kp,
                  OBJTYPE type, BOOL flag_pr, void  *geo);
BOOL test_in_pl_3d(BOOL flag_pr, lpD_POINT p, lpGEO_LINE l, BOOL flag_line);
BOOL test_in_pa_3d(BOOL flag_pr, lpD_POINT p, lpGEO_ARC a, BOOL flag_arc);
BOOL test_in_pe_3d(BOOL flag_pr, lpD_POINT p, OBJTYPE type,
                   void  *geo, BOOL flag_el);
lpD_POINT test_cont(lpD_POINT pp, short kp, lpD_POINT vp,
                    OBJTYPE type, void  *geo, BOOL flag_el,
                    lpD_POINT p);

//--t_to_acs.c
void tr_vp_to_acs(lpGEO_ARC a, BOOL flag_arc, lpD_POINT vp,
                  lpD_POINT gp, lpD_POINT ap,
                  lpGEO_ARC a2d, MATR g_a, MATR a_g);
void trans_arc_to_acs(lpGEO_ARC a3d, BOOL flag_arc,
                     lpGEO_ARC a2d, MATR g_a, MATR a_g);
void get_c_matr(lpGEO_CIRCLE circle, MATR g_c, MATR c_g);
BOOL tr_arc_pl_to_acs(lpGEO_ARC a1, BOOL flag_arc,
                      lpGEO_ARC a12d, MATR g_a, MATR a_g,
                      lpGEO_ARC a2, lpD_POINT c);

//--nea_util.c
lpD_POINT near_3d_pl(lpGEO_LINE l, lpD_POINT vp, lpD_POINT p);
lpD_POINT near_3d_pa(lpGEO_ARC a, BOOL flag_arc, lpD_POINT vp, lpD_POINT p);

//--netohobj.c
lpD_POINT near_to_hobj(hOBJ hobj, OBJTYPE type, lpD_POINT vp, lpD_POINT p);
lpD_POINT near_to_geo_obj(OBJTYPE type, void * geo,
                          lpD_POINT vp, lpD_POINT p);

//--q_ofhobj.c
lpD_POINT quadrant_of_hobj(hOBJ hobj, OBJTYPE type, lpD_POINT vp, lpD_POINT p);
lpD_POINT quadrant_of_geo_obj(OBJTYPE type, void * geo,
                              lpD_POINT vp, lpD_POINT p);

//--c_ofhobj.c
lpD_POINT center_of_hobj(hOBJ hobj, OBJTYPE type, lpD_POINT vp, lpD_POINT p);
lpD_POINT center_of_geo_obj(OBJTYPE type, void * geo,
                            lpD_POINT vp, lpD_POINT p);

//--tatohobj.c
lpD_POINT tangent_to_hobj(hOBJ hobj, OBJTYPE type, lpD_POINT vp, lpD_POINT p);
lpD_POINT tangent_to_geo_obj(OBJTYPE type, void * geo,
                             lpD_POINT vp, lpD_POINT p);

//--m_ofhobj.c
lpD_POINT middle_of_hobj(hOBJ hobj, OBJTYPE type, lpD_POINT vp, lpD_POINT p);
lpD_POINT middle_of_geo_obj(OBJTYPE type, void * geo,
                            lpD_POINT vp, lpD_POINT p);

//--int_util.c
BOOL intersect_two_obj(OBJTYPE type1, void  *geo1, BOOL flag_1,
                       OBJTYPE type2, void  *geo2, BOOL flag_2,
                       lpD_POINT p, short *kp);
BOOL intersect_3d_po(lpD_POINT po,
           OBJTYPE type, void *geo, BOOL flag_el,
           lpD_POINT p, short *kp);
short intersect_2d_ll(lpGEO_LINE l1, lpGEO_LINE l2, lpD_POINT p);
short intersect_2d_lc(sgFloat a, sgFloat b, sgFloat c,
                    sgFloat xc, sgFloat yc, sgFloat r,
                    lpD_POINT p);
short intersect_2d_cc(sgFloat xc1, sgFloat yc1, sgFloat r1,
                    sgFloat xc2, sgFloat yc2, sgFloat r2,
                    lpD_POINT p);
BOOL intersect_3d_ll(lpGEO_LINE l1, BOOL flag_line1,
                     lpGEO_LINE l2, BOOL flag_line2,
                     lpD_POINT p,   short *kp);
BOOL intersect_3d_la(lpGEO_LINE l, BOOL flag_line,
                     lpGEO_ARC a,  BOOL flag_arc,
                     lpD_POINT p,  short *kp);
BOOL intersect_3d_aa(lpGEO_ARC a1, BOOL fl_arc1,
                     lpGEO_ARC a2, BOOL fl_arc2,
                     lpD_POINT p,  short *kp);

//--tr_g_inf.c
BOOL trans_geo_info(hOBJ hobj, OBJTYPE *type, BOOL *flag_el, void  *geo);
BOOL true_geo_info(hOBJ hobj, OBJTYPE *type, void  *geo);
void set_flag_el(OBJTYPE *type, BOOL *flag_el);

//--setelgeo.c
void set_el_geo(void);

//--per_util.c
short perp_2d_pa(BOOL flag_arc, lpGEO_ARC a, lpD_POINT p, lpD_POINT pp);
BOOL perp_3d_pl(lpGEO_LINE l, lpD_POINT b, lpD_POINT p);
BOOL perp_3d_pa(lpGEO_ARC a, BOOL flag_arc, lpD_POINT b, lpD_POINT vp,
                lpD_POINT p, short *kp, short *np);
BOOL perp_to_obj(OBJTYPE type, void  *geo, BOOL flag_el,
                lpD_POINT b, lpD_POINT vp,
                lpD_POINT p, short *kp, short *np);

//--brea_obj.c
BOOL break_line_p(lpD_POINT p, lpGEO_LINE l, lpGEO_LINE l1, lpGEO_LINE l2);
void break_circle_p(lpD_POINT p, lpGEO_CIRCLE c, lpGEO_ARC a);
BOOL break_arc_p(lpD_POINT p, lpGEO_ARC a, lpGEO_ARC a1, lpGEO_ARC a2);
short break_obj_p(lpD_POINT p, OBJTYPE type, void  *geo,
                 void  *geo1, OBJTYPE *type1,
                 void  *geo2, OBJTYPE *type2);

//--sel_frag.c
BOOL select_fragment(hOBJ hobj, lpSCAN_CONTROL lpsc);
short delete_fragment(OBJTYPE type, void  *geo,
                    void  *geo1, OBJTYPE *type1,
                    void  *geo2, OBJTYPE *type2);
void fragment_points(lpD_POINT p, short kp);
void fragment_points_2(lpD_POINT p);

//--geotypes.c
OBJTYPE get_obj_type(EL_OBJTYPE eltype);
EL_OBJTYPE get_el_type(OBJTYPE type);
void set_filter_msk(OFILTER filter, EL_OBJTYPE msk);

//--arc_calc.c
BOOL arc_b_e_c(lpD_POINT bp, lpD_POINT ep, lpD_POINT cp,
               BOOL cend, BOOL cradius, BOOL negativ, lpGEO_ARC arc);
void circle_c_bt(lpD_POINT cp, lpD_POINT bcond, lpD_POINT vp,
                 OBJTYPE type, lpD_POINT bp);
BOOL arc_b_c_h(lpD_POINT bp, lpD_POINT cp, sgFloat h, BOOL negativ,
               lpGEO_ARC arc);
BOOL arc_b_c_a(lpD_POINT bp, lpD_POINT cp, sgFloat angle, lpGEO_ARC arc);
BOOL arc_b_e_pa(lpD_POINT bp, lpD_POINT ep, lpD_POINT ap, BOOL negativ,
                lpGEO_ARC arc);
BOOL arc_b_e_a(lpD_POINT bp, lpD_POINT ep, sgFloat angle, lpGEO_ARC arc);
BOOL arc_p_p_r(short ib, short ie, sgFloat r, lpD_POINT p, lpD_POINT vp,
               lpOBJTYPE type, BOOL negativ, lpGEO_ARC arc);
BOOL arc_b_e_r(lpD_POINT bp, lpD_POINT ep, sgFloat r,
               BOOL negativ, lpGEO_ARC arc);
short intersect_two_2d_obj(lpOBJTYPE type, lpD_POINT g, lpD_POINT p);
BOOL circle_g_g_r(lpOBJTYPE type, lpD_POINT g, sgFloat r, lpD_POINT vp,
                  lpD_POINT tp, lpD_POINT cp);
BOOL arc_b_e_v(lpD_POINT bp, lpD_POINT ep, lpD_POINT vp, BOOL negativ,
               lpGEO_ARC arc);
BOOL arc_b_e_vt(lpD_POINT bp, OBJTYPE type, lpD_POINT te, lpD_POINT sp,
                lpD_POINT vp, BOOL negativ, lpGEO_ARC arc);
BOOL arc_p_p_p(short ib, short ie, short im, lpD_POINT p, lpD_POINT vp,
               lpOBJTYPE type, BOOL negativ, lpGEO_ARC arc);
BOOL circle_b_e_m(lpD_POINT p, lpD_POINT cp);
BOOL circle_c_c_c(lpD_POINT p, lpD_POINT vp, lpD_POINT tp, lpD_POINT cp);
BOOL circle_c_l_l(short ic, short l1, short l2,
                  lpD_POINT p, lpD_POINT vp, lpD_POINT tp, lpD_POINT cp);
BOOL circle_c_c_l(short c1, short c2, short l,
                  lpD_POINT p, lpD_POINT vp, lpD_POINT tp, lpD_POINT cp);
BOOL circle_l_l_l(lpD_POINT l, lpD_POINT vp, lpD_POINT tp, lpD_POINT cp);

//--ln_calc.c
BOOL line_t_t(BOOL vflag, lpD_POINT a, lpD_POINT vp, lpD_POINT tp);
BOOL line_t_p_t(lpD_POINT a, lpD_POINT vp, lpD_POINT tp);
BOOL line_t_v_t(lpD_POINT a, lpD_POINT vp, lpD_POINT tp);
void set_tline_sign(lpD_POINT a, lpD_POINT vp);

//-- t_o_plan.c
BOOL test_geo_to_plane(OBJTYPE type, void * geo, lpD_PLANE pl);
//-- t_cont_i.c
short test_cont_int(lpD_POINT bp, lpD_POINT ep, lpD_POINT vp);

lpD_POINT intersect_two_hobj(hOBJ hobj1, hOBJ hobj2, lpD_POINT vp, lpD_POINT p);



#endif 