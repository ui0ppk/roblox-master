#ifndef __SMTHNK_UTILS_HDR__
#define __SMTHNK_UTILS_HDR__

#include "common_hdr.h"

typedef enum {
  AP_NULL   = 0x00,
  AP_CONSTR = 0x01, 
  AP_SOPR   = 0x02, 
  AP_COND   = 0x04, 
} AP_TYPE;  
typedef AP_TYPE * lpAP_TYPE;

//------------>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<---------------
typedef struct{
  D_POINT begin;    
  D_POINT end;      
  D_POINT middle;   
}POINT3;
typedef POINT3 * lpPOINT3;

// -------------apr.path.c
BOOL apr_path(lpVDIM vdim, hOBJ hobj);
OSCAN_COD path_pre_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);
//--------------extrud.c
BOOL extrud_mesh(lpLISTH listh_path, short type,
                 lpMATR matr, BOOL bottoms, hOBJ *hrez );
// ------------- pipe.c
BOOL pipe_mesh(lpLISTH listh_cross, lpVDIM vdim_path, BOOL type_path, OSTATUS status1,
               BOOL ZU,
               AP_TYPE bstat, AP_TYPE estat, BOOL bottoms, hOBJ *hrez);
// ------------- pipe_r.c
BOOL real_pipe_mesh(sgFloat R1, sgFloat R2, short num, hOBJ hpath,  bool clos, hOBJ *hrez );
// -------------rev.c
BOOL rev_mesh(hOBJ hobj, sgFloat fi, BOOL zu, int type, lpMATR matr, BOOL closed,
              hOBJ *hrez);
//--------------screw.c
BOOL screw_mesh(lpLISTH listh_path, lpD_POINT f, lpD_POINT s,
        sgFloat step, sgFloat height, short num_v, BOOL bottoms,hOBJ *hrez );
//--------------div_bott.c
BOOL create_contour_appr( lpVDIM list_vdim, hOBJ hpath, BOOL *closed);
BOOL put_bottom         ( lpNPW np, lpVDIM list_vdim );
BOOL sub_division_face  ( lpNPW np, lpVDIM add_vdim );
BOOL add_new_points     ( lpVDIM list_vdim, lpVDIM add_vdim );
void multiply_np        ( lpNPW np, lpMATR matr);
BOOL free_list_vdim     ( lpVDIM list);

extern sgFloat  c_h_tolerance;  
extern IDENT_V id_Density;     
extern sgFloat  Default_Density;


BOOL sew(lpLISTH list, hOBJ *hrez, sgFloat precision);


BOOL equid(hOBJ hobj, sgFloat h, BOOL type_body, hOBJ *hrez);

//===========================MIX=============================
typedef enum { X_SQUARE, X_VOLUME, X_MASS, X_WEIGHT,
               X_STATIC_MOMENT_XY, X_STATIC_MOMENT_YZ, X_STATIC_MOMENT_ZX,
               X_IM_OX, X_IM_OY, X_IM_OZ,
               X_IM_XY, X_IM_YZ, X_IM_ZX,
               X_CIM_XY, X_CIM_YZ, X_CIM_ZX,
               X_MASS_CENTER_X, X_MASS_CENTER_Y, X_MASS_CENTER_Z,
               X_NUM
} X_TYPE;
void Calculate_Group_M_In( lpLISTH group,
                     sgFloat *Density, sgFloat M_to_UNIT,
                     BOOL *flag, sgFloat *value );
void Calculate_M_In( hOBJ hobj, sgFloat Density, sgFloat M_to_UNIT,
                     BOOL *flag, sgFloat *value );

//----------------- aplicat\s_design\faced.c
BOOL faced_mesh(lpLISTH listh_path, hOBJ *hrez );


typedef struct{
  short     n_type;     
  short     n_primitiv; 
  short     n_num;      
  short     *n_point;   
  VDIM    vdim;       
  BOOL    first;
}RIB_DATA;
typedef RIB_DATA * lpRIB_DATA;
extern RIB_DATA rib_data;
BOOL apr_rib( hOBJ hobj, short work_type );
BOOL Skin_Surface_BRP( lpLISTH rib_curves, lpVDIM begin_point,
                       BOOL closed,  BOOL solid, hOBJ *hrez);
//------------------ apr_smot.c
BOOL apr_smooth( hOBJ hobj );




typedef enum {
  CIN_BIG_LOOP = 1001,  
  CIN_NO_CORRECT_OZ,   
  CIN_NO_CORRECT_NZ,    
  CIN_BAD_PATH,         
  CIN_OVERFLOW_MDD,     
  CIN_OUT_RADIUS_BAD,   
} CINEMA_ERR;

// ------------- pipe.c
BOOL create_pipe( lpMESHDD g, hOBJ cross, lpVDIM vdim_path, MATR m_all,
                  BOOL *d_tip, BOOL ZU, AP_TYPE bstat, AP_TYPE estat );
BOOL create_cross( hOBJ hpath, lpVDIM vdim_cross );
BOOL put_closed_path(  lpMESHDD g,
                       lpMNODE previos, lpMNODE current,
                       lpVDIM vdim_path, lpVDIM vdim_cross, lpMATR,
                       AP_TYPE bstat, AP_TYPE estat);
BOOL put_unclosed_path( short j, lpMESHDD g,
                         lpD_POINT previos, lpD_POINT current,
                         lpVDIM vdim_cross );
BOOL create_cross_section( lpMNODE previos, lpMNODE current, lpMNODE next,
                           lpD_POINT v1, lpD_POINT v2, sgFloat *D, sgFloat *R );
BOOL write_cross_section( lpMESHDD g, short mask, lpVDIM vdim_cross,
                          lpD_POINT v1, lpD_POINT v2, sgFloat D, sgFloat R,
                          AP_TYPE bstat, AP_TYPE estat);
// -------------bottoms.c
BOOL put_top1_bottom(lpNPW np, lpMESHDD g );
BOOL put_top_bottom(lpNPW np, lpMESHDD g );
BOOL put_bottom_bottom(lpNPW np, hOBJ cross );
BOOL put_bottom_rev( lpNPW np, lpMESHDD g, BOOL clouser, BOOL i );
BOOL put_bottom_bottom_skin(lpNPW np, lpMESHDD g );
BOOL my_expand_face( lpNPW np, short m );

//--------------cin_err.c
void cinema_handler_err(short cod,...);

extern lpNPW         c_np_top, c_np_bottom;


typedef struct {
  char  config_name[MAXPATH];
  char  append_path[MAXPATH];
  char  import_path[MAXPATH];
  UCHAR import_num;
  char  export_path[MAXPATH];
  char  macro_path[MAXPATH];
  char  slide_path[MAXPATH];
  F_PLPOINT exp_hp_gab;
  char  version[MAXPATH];
  char  copyright[MAXPATH];
  UCHAR export_2D_num, export_3D_num;
  BOOL  FileLinkSaving;
  char  save_choice_path[MAXPATH];
  char  browse_path[MAXPATH];
  UCHAR WidthThreshold;
  char  proto_path[MAXPATH];
  char  dbvar_path[MAXPATH];
  char  print_path[MAXPATH];
  char  print_doc_name[MAXPATH];
  BOOL  RoundOn;
} STATIC_DATA;
typedef STATIC_DATA * lpSTATIC_DATA;

extern lpSTATIC_DATA static_data;



typedef struct {
  VDIM    vdv;         
  VDIM    vdl;         
  I_LISTH flisth;     
  I_LISTH freelv;     
  I_LISTH freell;      
  sgFloat  z;          
} ZCUT;
typedef ZCUT * lpZCUT;

typedef struct {
  I_LIST      list;   
  UCHAR       status; 
  IDENT_V     idf;    
  I_LISTH     listh;  
} ZCP;
typedef ZCP * lpZCP;

typedef struct {
  I_LIST      list;    
  D_PLPOINT   v;        
  sgFloat      k;        
} ZCV;
typedef ZCV * lpZCV;


#define ZC_NULL    0x00
#define ZC_CLOSE   0x01
#define ZC_PODPOR  0x02
#define ZC_HOLE    0x04
#define ZC_INBOX   0x08 
#define ZC_OUTBOX  0x10 
#define ZC_PREV    0x20 
void zc_init(lpZCUT cut, sgFloat z);
void zc_free(lpZCUT cut);
IDENT_V zcf_create_new_face(lpZCUT cut, UCHAR status);
IDENT_V zcp_create_new_path(lpZCUT cut, IDENT_V idf, UCHAR status);
IDENT_V zcv_add_to_tail(lpZCUT cut, IDENT_V idp, void *v, sgFloat k);
void zcp_close(lpZCUT cut, IDENT_V idp);
UCHAR zcp_status(lpZCUT cut, IDENT_V idp);
void zcp_set_status(lpZCUT cut, IDENT_V idp, UCHAR status);
IDENT_V zcf_get_first_face(lpZCUT cut);
IDENT_V zcf_get_next_face(lpZCUT cut, IDENT_V idf);
IDENT_V zcp_get_first_path_in_face(lpZCUT cut, IDENT_V idf);
IDENT_V zcp_get_next_path(lpZCUT cut, IDENT_V idp);
IDENT_V zcv_get_first_point(lpZCUT cut, IDENT_V idp, void *v, sgFloat* k);
IDENT_V zcv_get_next_point(lpZCUT cut, IDENT_V idv, void *v, sgFloat* k);
IDENT_V zcv_get_prev_point(lpZCUT cut, IDENT_V idv, void *v, sgFloat* k);
void zcv_overwrite_point(lpZCUT cut, IDENT_V idv, void *v, sgFloat k);
IDENT_V zcv_get_point(lpZCUT cut, IDENT_V idv, void *v, sgFloat* k);
int zcp_get_num_point(lpZCUT cut, IDENT_V idp);
void zcv_detach_point(lpZCUT cut, IDENT_V idp, IDENT_V idv);
IDENT_V zcv_insert_point(lpZCUT cut, IDENT_V idp, IDENT_V idv, void *v, sgFloat k);
void zcp_free_path(lpZCUT cut, IDENT_V idp);
void zcp_delete_path(lpZCUT cut, IDENT_V idf, IDENT_V idp);
IDENT_V zcf_equid(lpZCUT cut, lpZCUT equid, IDENT_V idf, sgFloat h, sgFloat h2, BOOL app_arc);

BOOL create_equ_path2(hOBJ hobj, lpD_PLANE lpPlane, sgFloat height1, sgFloat height2,
                     BOOL arc_equd, hOBJ * hobj_equ);

#define     SG_EXT ".sgr"   

#define MAX_SEL_STK 5      

typedef enum { ARC_CENTER, ARC_BEGIN, ARC_END} ARC_INIT_REGIM;

typedef enum {  
  TRAP_SEARCH, WINDOW_SEARCH, CROSS_SEARCH,
  GROUP_CREATE, GROUP_EXPLODE, GROUP_ADD, GROUP_REMOVE,
  SIMPLE_SELECT, AUTODELETE_SELECT,
  NO_BLINK, LOCAL_BLINK, GLOBAL_BLINK, MARK_BLINK,
} SYM_CONST;

typedef enum { 
  GROUPTRANSP,
  AUTOBOX,
  AUTODELETE,
  AUTOCOMMAND,
  AUTOSINGLE,
  AUTODEL_CINEMA,
  DRAG_IMAGE,
  SAVE_ON_UCS,
  LCS_PATH,
  CONTOSNAP,
  SWITCH_NUM,
} I_SWITCH;

typedef enum {
  R_VP0,    
  R_VP1,   
  R_VP2,   
  R_VP3,    
  R_VIEW,  
  R_NUM     
} SG_REGS;  

#define LEN_sgFloat         10

#define UN_MIRROR  0x01 
#define UN_BLOCK   0x02 
#define UN_LOAD    0x04 

typedef enum {
  ECS_CIRCLE,  
  ECS_ARC,      
  ECS_NOT_DEF,  
} ARC_TYPE;
typedef ARC_TYPE * lpARC_TYPE;

typedef enum {W_BACKFACE, W_BPOINT, W_SET_AND_REGEN, W_NUM2};
typedef enum {W_CONSTR, W_SKETCH, W_COND, W_APPROCH, W_DUMMY, W_FULL, W_NUM};



extern UCHAR screen_color;
extern UCHAR curr_vp_box;
typedef enum {  
  VP_SINGLE, VP_NOT_SINGLE, VP_SWITCH,
} VP_SYM_CONST;

typedef enum {
  CS_DEFAULT,
  CS_VIEW,    //---
  CS_USER,    
  CS_GLOBAL,  //---
  NUM_CS_TYPE
} TYPE_CS;   

typedef TYPE_CS * lpTYPE_CS;

extern BOOL bool_dis[];
extern BOOL bool_vis[];

extern short    sg_m_num_meridians;       

extern short   curr_color;                  

extern UCHAR      message_level;  

extern D_POINT    scene_min, scene_max;
extern LISTH      objects; 
extern LISTH      selected; 

extern   int      bool_var[]; 

extern MATR       m_ucs_gcs, m_gcs_ucs;     

extern D_POINT    curr_point_vcs;           
extern D_POINT    curr_point;              

extern FINDS      serch_finds;
extern CODE_MSG   el_geo_err;



extern short floating_precision;        
extern short export_floating_precision; 


//-------------------------------------------------- arc_util.c
void     get_curr_ecs_arc (lpGEO_ARC arc, lpARC_TYPE flag);
void     init_ecs_arc     (lpGEO_ARC arc, ARC_TYPE flag);
void     ecs_to_gcs       (lpD_POINT from, lpD_POINT to);
void     gcs_to_ecs       (lpD_POINT from, lpD_POINT to);
void     get_point_on_arc (sgFloat alpha, lpD_POINT p);
CODE_MSG arc_init         (ARC_INIT_REGIM regim, lpD_POINT p, lpGEO_ARC arc);
void     calc_arc_ab      (lpGEO_ARC arc);
void     normal_cpl       (lpD_POINT n);
CODE_MSG circle_init      (lpGEO_CIRCLE circle, lpD_POINT p);
void calc_arc_angle(lpGEO_ARC arc);
//-------------------------------------------------- a&regen.c
BOOL    no_attach_and_regen(hOBJ hobj);
BOOL    attach_and_regen (hOBJ hobj);
//---------------------------------------------------- cs_util.c
void lcs_oxy      (lpD_POINT p0, lpD_POINT px, lpD_POINT py, MATR m);
void lcs_cpl_bp   (MATR m);
void box_fixed_lsk(lpD_POINT p1);
void lcs_axis_z   (lpD_POINT p0, lpD_POINT pz, MATR m);
void lcs_o_px     (lpD_POINT p0, lpD_POINT px, MATR m);
void matr_default_cs_to_gcs(MATR matr);
void matr_gcs_to_default_cs(MATR matr);
short get_index_invalid_axis(void);

//---------------------------------------------------- floatpoi.c
void FloatPointErrors    (void);
void set_matherr_message (void);
//---------------------------------------------------- geo_util.c
lpD_POINT projection_on_line(lpD_POINT p,
                             lpD_POINT l1, lpD_POINT l2,
                             lpD_POINT pp);
BOOL      calc_face4        (lpD_POINT p0, lpD_POINT p1, lpD_POINT p,
                             lpD_POINT p2, lpD_POINT p3);
//---------------------------------------------------- get_i&b.c

//---------------------------------------------------- get_p_on.c
lpD_POINT get_point_on_edge(OBJTYPE type, void *geo,
                            sgFloat parm, lpD_POINT p);
//---------------------------------------------------- off_sel.c
void off_blink_selected(void);
void off_blink_one_obj(hOBJ hobj);
//--------------------------------------------------- pnt_tr.c
lpD_POINT scs_to_vcs (short sx, short sy, lpD_POINT vp);
void      vcs_to_scs (lpD_POINT vp, short *sx, short *sy);
lpD_POINT vcs_to_gcs (lpD_POINT vp, lpD_POINT gp);
lpD_POINT gcs_to_vcs (lpD_POINT gp, lpD_POINT vp);
lpD_POINT gcs_to_ucs (lpD_POINT gp, lpD_POINT up);
lpD_POINT ucs_to_gcs (lpD_POINT up, lpD_POINT gp);
lpD_POINT scs_to_gcs (short sx, short sy, lpD_POINT gp);
void      gcs_to_scs (lpD_POINT gp, short *sx, short *sy);
lpD_POINT scs_to_ucs (short sx, short sy, lpD_POINT up);
void      ucs_to_scs (lpD_POINT up, short *sx, short *sy);
lpD_POINT vcs_to_ucs (lpD_POINT vp, lpD_POINT up);
lpD_POINT ucs_to_vcs (lpD_POINT up, lpD_POINT vp);
void      gcs_to_vp  (short vp, lpD_POINT gp, short *sx, short *sy);
void      cpl_init   (TYPE_CS type);
lpD_POINT vcs_to_cpl (lpD_POINT vp, lpD_POINT cp);
//--------------------------------------------------- save&ext.c
void SG_save_and_exit(CODE_MSG code, char *str);
//--------------------------------------------------- tr_sel.c

//--- del_obj.c
void delete_obj(hOBJ hobj);
void autodelete_obj(hOBJ hobj);

//--- groupopr.c
void detach_all_group_obj(hOBJ hgroup, BOOL undoflag);
void attach_obj_to_group(hOBJ hgroup, hOBJ hobj);

//--- get_olim.c
void union_limits(lpD_POINT mn1, lpD_POINT mx1,
                  lpD_POINT mn2, lpD_POINT mx2);
BOOL get_scene_limits(void);
void modify_limits(lpD_POINT min, lpD_POINT max);

BOOL init_system_step2(char *exe_path, char *cfg_name);
BOOL init_system_step3(void);
//BOOL init_screen(int first);
//-- put_msg.c
void put_message(CODE_MSG code, char *s, UCHAR level);
//-- str_util.c
char* alltrim(char *str);
char* add_ext (char *name, char *def_ext);
char* chg_ext (char *name, char *def_ext);
char* extract_path(char *path, const char *pathname);

//--creatobj.c
hOBJ  create_simple_obj_loc(OBJTYPE type, void  *geo);
hOBJ create_simple_obj(OBJTYPE type, void  *geo);
hOBJ create_simple_obj_to_prot(lpOBJ prot, OBJTYPE type, void  *geo);
void set_obj_nongeo_par(lpOBJ obj);
void copy_obj_nongeo_par(lpOBJ prot, lpOBJ obj);
OBJTYPE get_simple_obj_geo(hOBJ hobj, void  *geo);
void get_simple_obj(hOBJ hobj, lpOBJ nongeo, void  *geo);

//--curr_pnt.c
void set_curr_point(lpD_POINT p);

//--get_p&o.c
//GS_CODE get_obj_and_pnt(lpINDEX_KW keys, lpINDEX_KW menu,
//                        OBJTYPE *type, hOBJ * findhobj);

//-- bl&mark.c
void unmark_obj(hOBJ hobj);
/*void blink_and_mark(SYM_CONST blink, hOBJ hobj);
void blink_blink(void);*/

//---d_to_txt.c
char *sgFloat_to_text(sgFloat d, BOOL delspace, char *txt);
char *sgFloat_to_textf(sgFloat d, short size, short precision, char *txt);
char *dummy_to_text(char *txt);
char *sgFloat_to_textp(sgFloat num, char *buf);
void sgFloat_to_text_for_export(char *buf, sgFloat num);

//--objt_str.c
char* GetObjTypeCaption(OBJTYPE type, char* str, int len);
char* GetObjDescriptionStr(char* str, int len, hOBJ hobj);


//---------- z&svp\cmd\cre_back.c
bool CreateBakFile(char *pathname);
void RestoreFromBakFile(char *pathname);

//--- s_contac.c
BOOL set_contacts_on_path(hOBJ hobj);
void calc_geo_direction(OBJTYPE type, void * geo, lpD_POINT v);
//--- s_flat.c
BOOL set_flat_on_path(hOBJ hobj, lpD_PLANE calc_plane);
bool is_path_on_one_line(hOBJ hobj);
//short test_spline_on_plane(lpGEO_SPLINE, lpD_PLANE plane);
//--- self_cro.c
OSCAN_COD test_self_cross_path(hOBJ hobj, hOBJ hobj2);
BOOL inter_limits_two_obj(lpD_POINT mn1, lpD_POINT mx1,
                          lpD_POINT mn2, lpD_POINT mx2);
//--- t_orient.c
short test_orient_path(hOBJ hpath, lpD_PLANE plane);
//--- t_inside.c
short test_inside_path(hOBJ htest, hOBJ hout, lpD_PLANE plane);

//-------------------------------------------------- c_equ_p.c
BOOL create_equ_path(hOBJ hobj, lpD_PLANE lpPlane, sgFloat height, BOOL arc_equd,
                     hOBJ * hobj_equ);
//-------------------------------------------------- c_ginfo.c
BOOL calc_curve_length(hOBJ hobj, sgFloat *length);
//-------------------------------------------------- c_noprot.c
GS_CODE c_noprot(void);
//-------------------------------------------------- c_open.c
GS_CODE load_model(char *name, MATR matr, BOOL draw);
BOOL NewCommon(void);
GS_CODE append_from_file(char *name, BOOL property, MATR matr, BOOL draw);

ODRAWSTATUS get_switch_display(void);
ODRAWSTATUS get_regim_edge(void);
BOOL        get_draw_bpoint(void);
//-------------------------------------------------- c_path.c
void    get_endpoint_on_path(hOBJ hobj, lpD_POINT p, OSTATUS direct);
BOOL    get_status_path(hOBJ hpath, OSTATUS *status);
BOOL    get_flat_path(hOBJ hpath);
hOBJ    create_path_by_point(lpD_POINT p, short num);
void    reset_direct_on_path_1(hOBJ hpath);
//-------------------------------------------------- c_pedit.c
short get_primitive_param(hOBJ hobj, void **par, sgFloat *scale);
BOOL CalcPoxyPolygon(hOBJ hobj, sgFloat OutRadius,
                     lpD_POINT p0, lpD_POINT pX, lpD_POINT pY, UCHAR *color);
//-------------------------------------------------- c_pipe.c
BOOL    get_2_points_on_path(hOBJ hpath, lpD_POINT pp);
//-------------------------------------------------- c_poly.c
hOBJ create_line(lpD_POINT v1, lpD_POINT v2, lpLISTH listh);
hOBJ create_polygon(short NumSide, sgFloat OutRadius,
                    D_POINT p0, D_POINT pX, D_POINT pY, UCHAR color);
//-------------------------------------------------- c_rect.c
hOBJ create_rectangle(sgFloat height, sgFloat width, D_POINT p0, D_POINT pX, D_POINT pY, UCHAR color);
//-------------------------------------------------- c_save.c
bool RealSaveModel(char *FileName, BOOL SelFlag,
           const void* userData, unsigned long userDataSize);

//-------------------------------------------------- c_setvar.c
//GS_CODE c_setvardc(void);
//-------------------------------------------------- c_skin.c
BOOL init_get_point_on_path(hOBJ hobj, sgFloat *length);
void term_get_point_on_path(void);
BOOL get_point_on_path(sgFloat t, lpD_POINT p);

//-------------------------------------------------- c_quit.c
void free_all_sg_mem(void);
//-------------------------------------------------- c_ucs.c
void create_ucs_matr(short flag,
           lpD_POINT p0, lpD_POINT p1, lpD_POINT p2, MATR matr);
BOOL place_ucs_to_lcs(hOBJ hobj, MATR matr_ucs_gcs);
BOOL place_ucs_to_lcs_common(hOBJ hobj, MATR matr_lcs_gcs, BOOL lcs_path);
OSCAN_COD ucs_geo_scan(hOBJ hobj, lpSCAN_CONTROL lpsc);
void create_ucs_matr1(short flag, lpD_POINT p0, lpD_POINT p1, lpD_POINT p2,
                      MATR matr, short First, short Second);
short get_primitive_param_2d(hOBJ hobj, void** par);


typedef enum { 
  SCAL_SNAP,
  ANGLE_SNAP,
  NUM_SNAPTYPE
} SNAPTYPE;

typedef enum { 
  RELATIVE_CRD,
  ORTHO_CRD,
  POLAR_CRD,
  NUM_CRD_PAR
} CRD_PAR;

#define SNP_EN   0x01
#define SNP_NPP  0x02


#define MAX_PNT_STK  3 

//---------------------------------------------------- calc_snp.c
void calc_snap(SNAPTYPE type, sgFloat *d);
BOOL calc_grid_snap(lpD_POINT vp, lpD_POINT vpg);
sgFloat floor_crd_par(sgFloat crd, sgFloat step);

extern BOOL       arc_negativ;
extern EL_OBJTYPE last_geo_type;
extern D_POINT    last_geo_p[];

//--arc_cond.c
void init_arc_geo_cond(short cnum, CNDTYPE ctype);
BOOL arc_geo_cond_gcs(lpD_POINT  p,        
                      lpD_POINT  norm,     
                      OBJTYPE    *outtype, 
                      lpGEO_ARC  outgeo);  
BOOL arc_geo_cond(lpD_POINT  p,         
                  lpD_POINT  curr_norm, 
                  BOOL       gcsflag,   
                  OBJTYPE    *outtype,  
                  lpGEO_ARC  outgeo,    
                  short        *numlines, 
                  lpGEO_LINE lines,     
                  BOOL       *newplane);

//--condtstr.c
void trans_cond_to_plane(short cnum, lpD_POINT pp, lpD_POINT pg);
BOOL test_cond_to_plane(short cnum);
BOOL test_points_to_plane(short nump, lpD_POINT p, lpD_PLANE pl);

//--last_dir.c
BOOL set_cont_geo(void  *geo, OBJTYPE type, lpD_POINT vp);
void set_last_geo(OBJTYPE type, void * geo, BOOL direct);
void get_geo_endpoints_and_vectors(OBJTYPE type, void * geo,
                                   lpD_POINT p, lpD_POINT v);
void pereor_geo(OBJTYPE type, void * geo);

//--defcndpl.c
BOOL define_cond_plane(short cnum, short *plflag,
                       lpD_POINT norm, lpD_PLANE pl, lpD_POINT vp,
                       short *plstatus);



#define SPL_DERIVATE_MOVE  0
#define SPL_MOVE           1

//----------------------------------------------------spl_edit.c
GS_CODE edit_sply(void);
GS_CODE edit_sply_form(void);
//------------------------------->>>>
GS_CODE edit_sply_knot(void);
GS_CODE move_sply_knot(void);
GS_CODE insert_sply_knot(void);
GS_CODE delete_sply_knot(void);
GS_CODE add_sply_knot(void);
//------------------------------->>>>
GS_CODE edit_sply_derivate(void);
GS_CODE move_sply_derivate(void);
GS_CODE fix_free_sply_derivate(BOOL fix);
//------------------------------->>>>
GS_CODE edit_sply_break(void);
//------------------------------->>>>
//----------------------------------------------------spl_find.c
BOOL find_sply_knot(lpD_POINT vp, lpSPLY_DAT spl, short *numint, short *numknot);

//----------------------------------------------------spl_msg.c
BOOL spl_message(void);


typedef struct {
  lpVDIM    vdim;         
  GEO_ARC   arc;          
  REGION_3D gab;
  BOOL      flag_arc;     
} BO_ARC_DATA, * lpBO_ARC_DATA;

#define SF(r,m)   ((r) > 0 ? m->efc[r].fm : m->efc[-r].fp)

typedef enum { BO_FIRST, BO_NEXT, BO_END} SVTYPE;


typedef enum {
       ST_MAX       = 0x0002,    
       ST_SV        = 0x0001,     
} BO_SV_STATUS;

typedef VADR                hBO_USER;
typedef VADR                hINDEX;

typedef struct {
  float  bnd;                      
  short    ind;                      
} BO_USER;

typedef struct {
  short    ident;                   
  VNP    hnp;                      
} INDEX;

typedef struct {
  WORD mem :  11;             
  WORD flg :  1;               
  WORD ep  :  2;               
  WORD em  :  2;              
} BO_BIT_EDGE;

typedef BO_USER         * lpBO_USER;
typedef INDEX           * lpINDEX;

BOOL b_as(lpNPW np);
BOOL b_del(lpNP_STR_LIST list_str, lpNPW np, lpNP_STR str);
BOOL b_del_sv(lpNP_STR_LIST list_str, lpNP_STR str);
short  b_dive(lpNPW np, short edge, short v);
short  b_diver(lpNPW np, lpNP_VERTEX_LIST ver, short ukt, short r,
             lpD_POINT vn, lpD_POINT vk);
//short  b_diver_ident(lpNPW np,short r,lpD_POINT vn,lpD_POINT vk);
BOOL b_divide_face(lpNPW np, short face);
BOOL b_fa(lpNPW np);
short  b_fa_lab1(lpNPW np);
BOOL b_fa_lab2(lpNPW np);
short  b_face_angle(lpNPW np1, short r, lpNPW np2, short f1, short f2, short pe);
short  b_face_label(short met);
VNP  b_get_hnp(hINDEX hindex, short num, short ident);
BOOL b_lnp(lpNP_STR str1,lpNP_STR str2);
BOOL b_form_model_list(lpLISTH list_brep, hOBJ hobj);
void b_move_gru(lpNPW np);
BOOL b_np_lab(lpNP_STR_LIST list_str, lpNP_STR str, lpNPW np);
BOOL b_ort(lpD_POINT * vn,lpD_POINT * vk,lpD_POINT n1,lpD_POINT n2);
BOOL b_ort_on(lpNPW np, short r, lpD_POINT * vn, lpD_POINT * vk);
short  b_pv(lpNPW np1, short r, lpD_PLANE pl);
short  b_search_edge(lpNPW npw, lpD_POINT bv, lpD_POINT ev, short ident);
BOOL b_sv(lpNPW np);
BOOL b_vr(short uka, short ukb, BOOL lea, BOOL leb);
BOOL b_vr_idn(lpNPW np,lpD_POINT vn,lpD_POINT vk,lpNPW npr,short edge);
BOOL b_vr_line(lpD_POINT vn,lpD_POINT vk);
short  b_vran(lpNPW np, lpNP_VERTEX_LIST sp, short ukt);
short  b_vrr(lpNPW np,lpD_POINT vn,lpD_POINT vk,lpNP_VERTEX_LIST ver,
           short ukt, short face);

BOOL bo_new_user_data(lpNPW np);
void bo_free_user_data(lpNPW np);
BOOL bo_copy_user_data(lpNPW dist, lpNPW source);
BOOL bo_expand_user_data(lpNPW np, short nov, short noe, short nof, short noc);
BOOL bo_read_user_data_buf(lpNP_BUF np_buf,lpNPW npw);
BOOL bo_write_user_data_buf(lpNP_BUF np_buf,lpNPW npw);
WORD bo_size_user_data(lpNPW npw);
BOOL bo_read_user_data_mem(lpNPW npw);
BOOL bo_add_user_data_mem(lpVLD vld,lpNPW npw);

BOOL check_np(lpNPW np1, lpNPW np2, lpNP_STR str1,lpNP_STR str2);
BOOL check_del(lpNP_STR str);
BOOL check_vr(lpNPW np1, lpNPW np2, lpD_POINT vn, lpD_POINT vk,
              short uka, short ukb, BOOL lea, BOOL leb);


extern short key, object_number, ident_c;
extern lpLISTH bo_line_listh;             
extern REGION_3D gab_c;                   
extern short facea, faceb;
extern lpNPW npa, npb;
extern hINDEX hindex_a, hindex_b;
extern int num_index_a, num_index_b;
extern NP_STR_LIST lista, listb;
extern NP_VERTEX_LIST vera, verb;
extern BOOL brept1;                      
extern BOOL check;                        


#define   _CMD_SUPPORT

BOOL  f17                   (LPSTR  buffer, int buflen,  LPSTR password,  int   passlen);

#include "..//resource.h"

//  Prototypes
#define BAD_DW (0xFFFFFFFF)

//////////////////////////////////////////////////////
//  Prototypes for functions - Init.c               //
//////////////////////////////////////////////////////

void  FatalStartupError (int code);

////////////////////////////////////////////////////////
//  Prototypes for functions - Tools.c                //
////////////////////////////////////////////////////////
UINT        Message           (UINT uiBtns, LPSTR lpFormat, ...) ;

int         Error             (LPSTR msg);
UINT        atox              (LPSTR str);

long        GetCurrentDateTime(void);

int         GetShortDateStr(int day, int mon, int year, LPSTR datestr);
int         GetShortTimeStr(int hour, int min, int sec, LPSTR timestr);

LONG        GetFileNameSize   (LPSTR name);
DWORD       CS                (LPSTR name, LONG len);

char*       GetIDS      (unsigned id);
char*       GetOemIDS   (unsigned id);

//todofile.c
LONG        nb_filelength(HANDLE fin);
int         nb_access         (const char *filename, int amode);
DWORD       nb_attr           (const char *filename);
HANDLE      nb_open           (const char *name, int access, WORD mode);
BOOL        nb_close          (HANDLE);
int         nb_unlink         (const char *name);
int         nb_rename         (const char *oldname, const char *newname);
DWORD       nb_write          (HANDLE h, LPCVOID buf, DWORD size);
DWORD       nb_read           (HANDLE h, LPCVOID buf, DWORD size);
DWORD       nb_lseek          (HANDLE handle, long offset, int fromwhere);

LONG        GetFileNameSize   (LPSTR fname);
BOOL        FileIsExist       (LPSTR fname);


typedef struct _P3D {
  sgFloat  x;
  sgFloat  y;
  sgFloat  z;
} P3D, *LPP3D;


typedef enum {
  SG_L_NP_SOFT,    
  SG_L_NP_SHARP,   
  SG_L_NP_PLANE   
}SG_L_NP_TYPE;


#define SETBIT(value, bit)  (value |=  (WORD)(1 << bit))
#define CLEARBIT(value, bit)  (value &= ~(WORD)(1 << bit))
#define TESTBIT(value, bit)  (value &   (WORD)(1 << bit))
#define SETMASK(value, mask)  (value |=  (unsigned int)(mask))
#define CLEARMASK(value, mask)  (value &= ~(unsigned int)(mask))
#define TESTMASK(value, mask)  (value &   (unsigned int)(mask))

#define   HM_TEXTURE    0 
#define   HM_LAMP       1 
#define   HM_LINEOBJ    2 

#define   MM_FLAT       0 
#define   MM_BKTITLE    1
#define   MM_ASLINES    2 
#define   MM_ALIAS      3 
#define   MM_LIGHT      4 
#define   MM_INDIVID    5 
#define   MEF_HIDEOBJ       0 
#define   MEF_TEXPLANE      1 
#define   MEF_TEXCIL        2 
#define   MEF_TEXSPHERE     3 
#define   MEF_TEXSMOOTH     4 
#define   MEF_TEXREPEAT     5 
#define   MFIL_TEXDECAL     0 //
#define   MFIL_TEXBLEND     1 //
#define   MFIL_TEXMODULATE  2 //

typedef enum {COL_VPORT_BG = 0, COL_SCREEN_COLOR, COL_CURR_VP_BOX, COL_C_RUBBER,
              COL_C_CURSOR, COL_GRID_COLOR, COL_C_BASE_POINT,
              COL_AXIS_BRIGHT, COL_AXIS_DULL,
              COL_BLINK_COLOR1, COL_BLINK_COLOR2,
              COL_COL_OPTION_TOTAL} OPTIONS_COLORS;


#define  C_REVOLVE              1005
#define  C_SCREW                1006
//#define  C_...                1016
#define  C_ACS                  1018
//#define  C_...                1019
#define  C_DC_ACS               1020
//#define  C_CONFIG               1022
//#define  C_...                1023
//#define  C_...                1024
#define  C_SKIN                 1025
#define  C_SEW                  1026
//#define  C_...                1028
#define  C_UCS                  1029
#define  C_PROBA                1036
//#define  C_RENEW                1039
#define  C_SKIN_B               1052
#define  C_BG_COLOR             1056
#define  C_MPRINT               1057
#define  C_HTPRINT              1058
#define  C_VISUAL               1059
#define  C_RULED                1061
//#define  C_HATCH                1062
#define  C_DEFORM               1063
//#define  C_LOOK                 1071


#define   C_PAN                25
#define   C_UNDO               27
#define   C_REDO               28
//#define   C_MACRO              37
#define   C_SETNAME            38
//#define   C_BPOINT             40

//#define   C_CRDPAR             44
#define   C_GRID               46
#define   C_RELATIVE           47
#define   C_ABSOLUTE           48
#define   C_POLAR              60
#define   C_RECTANGULAR        61
#define   C_ORTHO              70
  
#define   C_PACS               71
#define   C_PVCS               72
#define   C_PUCS               73
#define   C_PGCS               74

#define   C_VIEWPAR            84

#define   C_DEFARRAYS          91

#define   C_PUTREPORT          92

#define   C_TEDIT              94

#define   C_FONT               150
#define   C_THEIGHT            153
#define   C_TSCALE             154
#define   C_TANGLE             155
#define   C_TDCHAR             156
#define   C_TDSTRING           157
#define   C_TVERTICAL          158
#define   C_TFIXED             159

#define   C_DBEXP              160

#define   C_GETMPAR            161
#define   C_LOCAL              163

#define   C_DIMPAR             176

#define   C_LTYPE              177
#define   C_LTHICKNESS         178

#define   C_PROTOOBJ           202
#define   C_DBCONSTSET         204

#define   C_SELECT             228

#define   C_AUTOPAN            229
#define   C_AUTOZOOM           230

#define DSE_LAST_ERROR    0
#define DSE_NO_MEM        1
#define DSE_EMPTY_TABLE   2

#define DELETE_ACTION               114
#define GET_ITEM_INFO_ACTION        115
#define SET_ITEM_INFO_ACTION        116
#define GET_LIST_ACTION             117
#define GET_TREE_ACTION             118
#define GET_PARENT_WND_ACTION       119
#define SET_CHANGE_ACTION           120
#define SELECT_ACTION               121
#define EXPAND_ACTION               122
#define EXPAND_FULL_ACTION          123
#define DELETE_CHILDREN_ACTION      124
#define GET_DATA_ACTION             125
#define SET_DATA_ACTION             126
#define GET_TEXT_ACTION             127
#define SET_TEXT_ACTION             128
#define MOVE_UP_ACTION              129
#define MOVE_DOWN_ACTION            130
#define MOVE_LEFT_ACTION            131
#define MOVE_RIGHT_ACTION           132
#define BEGIN_UPDATE_ACTION         133
#define END_UPDATE_ACTION           134
#define CLEAR_ACTION                135
#define GET_SEL_ACTION              136
#define GET_HANDLER_ACTION          137
#define SET_HANDLER_ACTION          138
#define GET_CHANGE_ACTION           139
#define SET_MENUID_ACTION           140
#define GET_MENUID_ACTION           141
#define GET_EXPAND_ACTION           142
#define SET_SUBITEMS_ACTION         143

#define GET_TOOLBAR_INFO            301
#define GET_BUTTON_INFO             302
#define DRAW_KTBUTTON               303
#define GET_BUTTON_STATE            304
#define TB_RUN_TRACK                305
#define TB_RUN_FTRACK               306
#define TB_HIDE_ME                  307
#define TB_LOCK_ME                  308
#define TB_PLACE_ME                 309
#define TB_HINT_ME                  310
#define TB_RESET_DRAW_STATUS        311



#define HK_A   0x0100
#define HK_C   0x0200
#define HK_S   0x0400
#define HK_AC  (HK_A|HK_C)
#define HK_AS  (HK_A|HK_S)
#define HK_CS  (HK_C|HK_S)
#define HK_ACS (HK_A|HK_C|HK_S)

#define IT_GETSTATUS 0
#define IT_GETSTYLE  1
#define IT_GETTEXT   2
#define IT_GETIMAGE  3
#define IT_GETHINT   4
#define IT_DRAWIMAGE 5

#define AGROUP        DWORD
#define MAX_ACTGROUP       32

#define  KTBUTTON     KMENU_ITEM
#define pKTBUTTON    pKMENU_ITEM


#define ADD_DOCK_SIZE    22
#define MAX_DOCK_RECTS   30
#define KDOCK_PANS_COUNT 5

#define FLOAT_DV (2)//*GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYSMCAPTION))
#define FLOAT_DH (2)//*GetSystemMetrics(SM_CXSIZEFRAME))

#define STKEYWORD  0
#define STCOMMAND  1
#define STPROMPT   2
#define STKEYIMG   3
#define STACTIMG   4
#define STFUNCTION 5
#define STVIEWERS  6
#define STACTHKEY  7

void InitsBeforeMain(void);

void InitSysAttrs(void);

extern METHOD

   OMT_COMMON_FILTERS,         

   OMT_ENDPOINT_SNAP,           
   OMT_MIDDLE_SNAP,             
   OMT_PERP_SNAP,               
   OMT_NEAR_SNAP,
   OMT_TANG_SNAP,
   OMT_QUADRANT_SNAP,
   OMT_CENTER_SNAP,

   OMT_FREE_GEO,               
   OMT_OBJTYPE_CAPTION,         
   OMT_OBJ_DESCRIPT,           
   OMT_SCAN,                   
   OMT_VIOBRNP,                 
   OMT_VIREGEN,                
   OMT_INVERT,                  
   OMT_LIMITS,                  
   OMT_TRANS,                   
   OMT_VIFIND,                  
   OMT_TRANSTONURBS,            
   OMT_SPLINTER,                
   OMT_APR_PATH,                
   OMT_APR_RIB,                
   OMT_APR_SMOT,                
   OMT_PIPE_R,                  
   OMT_SAVE,                    
   OMT_LOAD,                   
   OMT_LOAD_DOS,                
   OMT_ZB_SCAN,                
   OMT_DXFOUT,                 
   OMT_VRML,                   
   OMT_GETLCS,                  
   OMT_LAST
   ;


OSCAN_COD  IdleHOBJScan(hOBJ hobj, lpSCAN_CONTROL lpsc);  
OSCAN_COD  IdleGEOScan(void* geo, lpSCAN_CONTROL lpsc);   
OSCAN_COD  IdleLPOBJScan(lpOBJ obj, lpSCAN_CONTROL lpsc); 



BOOL  free_geo_simple_obj( lpOBJ obj);
BOOL  free_geo_text      ( lpOBJ obj);
BOOL  free_geo_dim       ( lpOBJ obj);
BOOL  free_geo_brep      ( lpOBJ obj);
BOOL  free_geo_spline    ( lpOBJ obj);
BOOL  free_geo_frame     ( lpOBJ obj);


void IdleObjDecriptStr(hOBJ hobj, char *str, int len);
void ot_brep_str      (hOBJ hobj, char *str, int len);
void ot_insert_str    (hOBJ hobj, char *str, int len);
void ot_path_str      (hOBJ hobj, char *str, int len);


OSCAN_COD  scan_simple_obj( hOBJ hobj, lpSCAN_CONTROL lpsc );
OSCAN_COD  scan_path      ( hOBJ hobj, lpSCAN_CONTROL lpsc );
OSCAN_COD  scan_brep      ( hOBJ hobj, lpSCAN_CONTROL lpsc );
OSCAN_COD  scan_bgroup    ( hOBJ hobj, lpSCAN_CONTROL lpsc );
OSCAN_COD  scan_insert    ( hOBJ hobj, lpSCAN_CONTROL lpsc );
OSCAN_COD  scan_frame     ( hOBJ hobj, lpSCAN_CONTROL lpsc );


OSCAN_COD  invert_obj          (hOBJ hobj, lpSCAN_CONTROL lpsc);
OSCAN_COD  invert_arc          (hOBJ hobj, lpSCAN_CONTROL lpsc);


OSCAN_COD  get_text_limits     (void * geo, lpSCAN_CONTROL lpsc);
OSCAN_COD  get_dim_limits      (void * geo, lpSCAN_CONTROL lpsc);
OSCAN_COD  get_point_limits    (void * geo, lpSCAN_CONTROL lpsc);
OSCAN_COD  get_line_limits     (void * geo, lpSCAN_CONTROL lpsc);
OSCAN_COD  get_circle_limits   (void * geo, lpSCAN_CONTROL lpsc);
OSCAN_COD  get_arc_limits      (void * geo, lpSCAN_CONTROL lpsc);
OSCAN_COD  get_brep_limits     (void * geo, lpSCAN_CONTROL lpsc);
OSCAN_COD  get_spline_limits   (void * geo, lpSCAN_CONTROL lpsc);
OSCAN_COD  get_frame_limits    (void * geo, lpSCAN_CONTROL lpsc);
OSCAN_COD  get_idle_limits     (void * geo, lpSCAN_CONTROL lpsc);


void   trans_point     (void * geo, lpMATR matr);
void   trans_line      (void * geo, lpMATR matr);
void   trans_circle    (void * geo, lpMATR matr);
void   trans_arc       (void * geo, lpMATR matr);
void   trans_complex   (void * geo, lpMATR matr);
void   trans_spline    (void * geo, lpMATR matr);
void   trans_dim       (void * geo, lpMATR matr);


OSCAN_COD text_save   (lpBUFFER_DAT bd, lpOBJ obj);
OSCAN_COD dim_save    (lpBUFFER_DAT bd, lpOBJ obj);
OSCAN_COD simple_save (lpBUFFER_DAT bd, lpOBJ obj);
OSCAN_COD path_save   (lpBUFFER_DAT bd, lpOBJ obj);
OSCAN_COD brep_save   (lpBUFFER_DAT bd, lpOBJ obj);
OSCAN_COD bgroup_save (lpBUFFER_DAT bd, lpOBJ obj);
OSCAN_COD insert_save (lpBUFFER_DAT bd, lpOBJ obj);
OSCAN_COD spline_save (lpBUFFER_DAT bd, lpOBJ obj);
OSCAN_COD frame_save  (lpBUFFER_DAT bd, lpOBJ obj);


OSCAN_COD text_load   (lpBUFFER_DAT bd, hOBJ hobj);
OSCAN_COD dim_load    (lpBUFFER_DAT bd, hOBJ hobj);
OSCAN_COD simple_load (lpBUFFER_DAT bd, hOBJ hobj);
OSCAN_COD spline_load (lpBUFFER_DAT bd, hOBJ hobj);
OSCAN_COD frame_load  (lpBUFFER_DAT bd, hOBJ hobj);
OSCAN_COD path_load   (lpBUFFER_DAT bd, hOBJ hobj);
OSCAN_COD brep_load   (lpBUFFER_DAT bd, hOBJ hobj);
OSCAN_COD group_load  (lpBUFFER_DAT bd, hOBJ hobj);
OSCAN_COD insert_load (lpBUFFER_DAT bd, hOBJ hobj);



OSCAN_COD  dos_text_load   ( lpBUFFER_DAT bd, hOBJ hobj);
OSCAN_COD  dos_simple_load ( lpBUFFER_DAT bd, hOBJ hobj);
OSCAN_COD  dos_spline_load ( lpBUFFER_DAT bd, hOBJ hobj);
OSCAN_COD  dos_frame_load  ( lpBUFFER_DAT bd, hOBJ hobj);
OSCAN_COD  dos_path_load   ( lpBUFFER_DAT bd, hOBJ hobj);
OSCAN_COD  dos_brep_load   ( lpBUFFER_DAT bd, hOBJ hobj);
OSCAN_COD  dos_group_load  ( lpBUFFER_DAT bd, hOBJ hobj);
OSCAN_COD  dos_insert_load ( lpBUFFER_DAT bd, hOBJ hobj);




OSCAN_COD  dxfout_point  (void * geo, lpSCAN_CONTROL lpsc);
OSCAN_COD  dxfout_line   (void * geo, lpSCAN_CONTROL lpsc);
OSCAN_COD  dxfout_circle (void * geo, lpSCAN_CONTROL lpsc);
OSCAN_COD  dxfout_arc    (void * geo, lpSCAN_CONTROL lpsc);
OSCAN_COD  dxfout_spline (void * geo, lpSCAN_CONTROL lpsc);
OSCAN_COD  dxfout_frame  (void * geo, lpSCAN_CONTROL lpsc);
OSCAN_COD  dxfout_text   (void * geo, lpSCAN_CONTROL lpsc);


OSCAN_COD vrmlout_brep(hOBJ hobj, lpSCAN_CONTROL lpsc);


BOOL lcs_complex         (hOBJ hobj, MATR matr_lcs_gcs);
BOOL lcs_dim             (hOBJ hobj, MATR matr_lcs_gcs);
BOOL lcs_circle_and_arc  (hOBJ hobj, MATR matr_lcs_gcs);
BOOL lcs_point           (hOBJ hobj, MATR matr_lcs_gcs);
BOOL lcs_line            (hOBJ hobj, MATR matr_lcs_gcs);
BOOL lcs_spline_and_path (hOBJ hobj, MATR matr_lcs_gcs);
BOOL lcs_idle            (hOBJ hobj, MATR matr_lcs_gcs);


extern IDENT_V id_Density,
               id_Material,
               id_TextureScaleU,
               id_TextureScaleV,
               id_TextureShiftU,
               id_TextureShiftV,
               id_TextureAngle,
               id_Smooth,
               id_MixColor,
               id_UVType,
               id_TextureMult,
               id_LookNum,
               id_ProtoInfo,
               id_Mass,
               id_XCenterMass,
               id_YCenterMass,
               id_ZCenterMass,
               id_Layer,
         id_Name,
         id_TypeID;

extern int  nMEM_TRACE_LEVEL;            
extern BOOL bINHOUSE_VERSION;         

extern BOOL (*pExtStartupInits)(void);
extern void (*pExtFreeAllMem)(void);
extern void (*pExtSyntaxInit)(void);
extern void (*pExtActionsInit)(void);
extern void (*pExtRegObjects)(void);
extern void (*pExtRegCommonMethods)(void);
extern void (*pExtInitObjMetods)(void);
extern void (*pExtConfigPars)(void);
extern void (*pExtSysAttrs)(void);

extern BOOL (*pExtBeforeRunCmdLoop)(void);

extern BOOL (*pExtBeforeNew)(void);
extern BOOL (*pExtAfterNew)(void);

extern BOOL (*pExtReLoadCSG)(hOBJ hobj, hCSG *hcsg);

#define   IDXBLACK           0
#define   IDXBLUE            1
#define   IDXGREEN           2
#define   IDXCYAN            3
#define   IDXRED             4
#define   IDXMAGENTA         5
#define   IDXBROWN           6
#define   IDXLIGHTGRAY       7
#define   IDXDARKGRAY        8
#define   IDXLIGHTBLUE       9
#define   IDXLIGHTGREEN     10
#define   IDXLIGHTCYAN      11
#define   IDXLIGHTRED       12
#define   IDXLIGHTMAGENTA   13
#define   IDXYELLOW         14
#define   IDXWHITE          15


extern unsigned long TotalMallocMemSize;

void StopInControlMalloc(void);
void StopInNullSizeMalloc(void);
void StopInBadPointerFree(void);
void StopInBadPointerRealloc(void);
void *SGMalloc(size_t size);
void *SGMallocN(size_t size);
void SGFree(void* address);
void *SGRealloc(void *block, size_t size);
void *SGReallocN(void *block, size_t size);
void InitDbgMem(void);
void FreeDbgMem(void);

void vReport(void);
void FatalInternalError(int code);


void*  SoloTriangulateBrep(hOBJ  hobj);
void*  SoloBrepEdges(hOBJ  hobj);




#endif