#ifndef __SPLINE_NURBS_UTILS_HDR__
#define __SPLINE_NURBS_UTILS_HDR__




#define  MAX_POINT_ON_SPLINE       100  
#define  INTERNAL_SPLINE_POINT     15  
                                      
#define  MAX_INTERNAL_SPLINE_POINT 50  
#define  NUMBER_OF_ITER            50  

#define SPL_NO_MEMORY      1 
#define SPL_NO_HEAP        2  
#define SPL_MAX            3 
#define SPL_INTERNAL_ERROR 4 
#define SPL_ENPTY_DATA     5  
#define SPL_BAD_POINT      6  

typedef enum{
  CRE_SPL       = 0x0001,  
  EDIT_SPL      = 0x0002,  
  ADD_SPL       = 0x0010,  
}SPL_WORK;

typedef struct {
  DA_POINT   p;    
  short        num; 
}SNODE;
typedef SNODE  * lpSNODE;

typedef struct {
  sgFloat vertex[4];   
}W_NODE;
typedef W_NODE  * lpW_NODE;

typedef enum{
  SPL_NEW   = 0x0001, 
  SPL_GEO   = 0x0002,  
  SPL_FREE  = 0x0010,  
  SPL_CLOSE = 0x0020, 
  SPL_APPR  = 0x0100,  
  SPL_INT   = 0x0200,  

} SPLY_TYPE_IN;

typedef struct {
  SPLY_TYPE_IN sdtype;       
  short        degree;      
  lpDA_POINT   knots;       
  short        numk;         // knots count
  lpSNODE      derivates;    
  short        numd;        
  lpW_NODE     P;            // (Wx, Wy, Wz, W)/(nump)
  short        nump;         
//  short        *multiply;    
    sgFloat *     u;          
  sgFloat *     U;            
  short      numU;         

  int          allocSizeFor_knots;
  int          allocSizeFor_derivates;
  int          allocSizeFor_P;
  int          allocSizeFor_u;
  int          allocSizeFor_U;

} SPLY_DAT; 
typedef SPLY_DAT * lpSPLY_DAT;

//----------------------------------------------------spl_util.c
BOOL begin_use_sply (lpGEO_SPLINE geo_sply, lpSPLY_DAT sply_dat);
void end_use_sply   (lpGEO_SPLINE geo_sply, lpSPLY_DAT sply_dat);
BOOL init_sply_dat  (char sdtype, short degree, short nurbs_type, lpSPLY_DAT sply_dat);
void free_sply_dat  ( lpSPLY_DAT sply_dat);
BOOL create_geo_sply(lpSPLY_DAT sply_dat, lpGEO_SPLINE geo_sply);
BOOL unpack_geo_sply(lpGEO_SPLINE geo_sply, lpSPLY_DAT sply_dat);
BOOL expand_spl     ( lpSPLY_DAT spline, short num, short degree, short type );

//----------------------------------------------------spl_use.c
void get_point_on_sply   (lpSPLY_DAT sply_dat, sgFloat t,
                          lpD_POINT p, short deriv);
void get_first_sply_point(lpSPLY_DAT sply_dat, sgFloat sigma, BOOL back,
                          lpD_POINT p);
void get_first_sply_segment_point(lpSPLY_DAT sply_dat, sgFloat sigma,
                          short bknot, short eknot, lpD_POINT p);
BOOL get_next_sply_point (lpSPLY_DAT sply_dat, lpD_POINT p);
BOOL get_next_sply_point_tolerance(lpSPLY_DAT sply_dat, lpD_POINT p,
                          BOOL *knot );
BOOL get_next_sply_point_and_knot(lpSPLY_DAT sply_dat,
                          lpD_POINT p, BOOL *knot);

//----------------------------------------------------spl_crea.c
BOOL add_sply_point   (lpSPLY_DAT sply_dat, lpD_POINT point, short num);
BOOL close_sply       (lpSPLY_DAT sply_dat);
BOOL move_sply_point  (lpSPLY_DAT sply_dat, short num, lpD_POINT point);
BOOL delete_sply_point(lpSPLY_DAT sply_dat, short num);
BOOL break_close_sply (lpSPLY_DAT sply_dat, short num);
BOOL insert_sply_point(lpSPLY_DAT sply_dat, short num, lpD_POINT point);
BOOL set_derivate_to_point     (lpSPLY_DAT sply_dat, short num, lpD_POINT derivates);
BOOL fix_free_derivate_to_point(lpSPLY_DAT sply_dat, short num, BOOL fix);
BOOL change_APPR_INT(lpSPLY_DAT sply_dat);
BOOL change_INT_APPR(lpSPLY_DAT sply_dat);
BOOL change_degree  (lpSPLY_DAT sply_dat, short new_p );
BOOL change_weight  (lpSPLY_DAT sply_dat, short p, sgFloat new_w );
BOOL add_break_knots(lpSPLY_DAT sply_dat, short num );
BOOL del_break_knots(lpSPLY_DAT sply_dat, short num );

//-----------------------------------------------------nrb_mem.c
void free_matrix    ( sgFloat **L, short n );
sgFloat ** mem_matrix( short m, short n );

//-----------------------------------------------------nrb_matr.c
void nurbs_matrix_create( lpSPLY_DAT sply_dat, short p_c,
                          sgFloat **A, short gran );
BOOL gauss( sgFloat **A, lpW_NODE y, short u );
void change_AB( sgFloat *i, sgFloat *j );

//-----------------------------------------------------nrb_base.c
sgFloat nurbs_basis_func( short i, short p, sgFloat *U_p, sgFloat u_p );
void   parameter_free  ( short n_c, short p_c, lpDA_POINT Q_tmp,
                         sgFloat *u_c, sgFloat *U_c );
void   parameter_free_APPR( short n_c, short p_c, lpW_NODE P,
                         sgFloat *u_c, sgFloat *U_c );
void   parameter_closed( short n_c, short p_c, lpDA_POINT Q_tmp,
                         sgFloat *u_c, sgFloat *U_c );
void   parameter_deriv ( short n_c, short p_c, lpDA_POINT Q_tmp, lpSNODE deriv,
                         sgFloat *u_c, sgFloat *U_c );
void   parameter_deriv_closed( short n_c, short p_c, lpDA_POINT Q_tmp, lpSNODE deriv,
                         sgFloat *u_c, sgFloat *U_c );
void Calculate_Point   ( lpSPLY_DAT sply_dat, short degree, sgFloat t, lpD_POINT p);
void Calculate_Deriv   ( lpSPLY_DAT sply_dat, short degree, sgFloat t,
                         lpD_POINT p, short deriv);
BOOL Calculate_Weight_Point(lpSPLY_DAT sply_dat, short degree, sgFloat t, lpD_POINT p);
BOOL Calculate_Weight_Deriv(lpSPLY_DAT sply_dat, short degree, sgFloat t,
                         lpD_POINT p, short deriv);
//-----------------------------------------------------nrb_crea.c
void   parameter         (lpSPLY_DAT sply_dat, short condition, BOOL p );
BOOL   calculate_P       (lpSPLY_DAT sply_dat, short condition );

//-----------------------------------------------------spl_len.c
sgFloat spl_length( lpSPLY_DAT sply_dat );
sgFloat spl_length_P( lpSPLY_DAT sply_dat );
//=============================================================>>>>>>>>>>

//-----------------------------------------------------flet_pau.c
typedef sgFloat ( *functionFP ) ( void  *, void  *, sgFloat[] );
typedef sgFloat ( *derivativs)( void  *, void  *, sgFloat[], sgFloat[] );
// same the NEWTON's method of optimiztion function
sgFloat Fletchjer_Paur( functionFP, derivativs, void  *, void  *, sgFloat[], short, short );
//for NURBScurve-to-NURBScurve distance
sgFloat function_3  ( void  *, void  *, sgFloat[] );
sgFloat derivativs_3( void  *, void  *, sgFloat[], sgFloat[] );
//for NURBScurve-to-plane distance
sgFloat function_2  ( void  *, void  *, sgFloat[] );
sgFloat derivativs_2( void  *, void  *, sgFloat[], sgFloat[] );
//for NURBScurve-to-line distance
sgFloat function_1  ( void  *, void  *, sgFloat[] );
sgFloat derivativs_1( void  *, void  *, sgFloat[], sgFloat[] );
//for NURBScurve-to-point distance
sgFloat function_0  ( void  *, void  *, sgFloat[] );
sgFloat derivativs_0( void  *, void  *, sgFloat[], sgFloat[] );
//for NURBSsurface-to-point distance
sgFloat function_0_0  ( void  *, void  *, sgFloat[] );
sgFloat derivativs_0_0( void  *, void  *, sgFloat[], sgFloat[] );
//-----------------------------------------------------spl_intr.c
BOOL intersection_with_spline( hOBJ hspl, hOBJ hcont, short type,
                               lpVDIM data, lpVDIM data1);
// Intersect_Spline_By_Plane  --->> into b_point.c
BOOL AutoIntersect_Spline      (hOBJ hspl, short type, lpVDIM data);
BOOL Devide_Spline_By_Point    (hOBJ hspl, short type, lpD_POINT point, hOBJ hspl_l, hOBJ hspl_r);
//-----------------------------------------------------nrb_intr.c
BOOL Spline_By_Point ( lpSPLY_DAT spl, void  *point, sgFloat *t, sgFloat *dist );
BOOL Spline_By_Line( lpSPLY_DAT spl, void  *line,
                     sgFloat *t_sply, sgFloat *t_line,
                     sgFloat *bound, sgFloat *dist );
BOOL Spline_By_Plane ( lpSPLY_DAT spl, void  *point, sgFloat *t, sgFloat *bound, sgFloat *dist );
BOOL Spline_By_Spline( lpSPLY_DAT spl1, void  *spl2,
                       sgFloat *t1,      sgFloat *t2,
                       sgFloat *bound1,  sgFloat *bound2,
                       sgFloat *dist );
//-----------------------------------------------------nrb_geo.c
BOOL Create_Multy_Point( lpSPLY_DAT spline, sgFloat t1, short type );
BOOL Get_Part_Spline_Geo(lpSPLY_DAT spline, sgFloat t1, sgFloat t2,
                         lpGEO_SPLINE geo_spline);
BOOL Get_Part_Spline_Dat(lpSPLY_DAT spline, sgFloat t1, sgFloat t2,
                         lpSPLY_DAT sply_dat);
BOOL Two_Spline_Union_Dat(lpSPLY_DAT sply_dat1, lpSPLY_DAT sply_dat2,
                          lpSPLY_DAT sply_new);
BOOL Increase_Spline_Degree(lpSPLY_DAT sply_dat, short new_degree);
BOOL Reparametrization( short nump, short numu, sgFloat c, sgFloat d,
                        short p, sgFloat *U, lpW_NODE P, sgFloat *u );
BOOL Reorient_Spline  ( lpSPLY_DAT sply_dat );
BOOL Modify_Spline_Str( lpSPLY_DAT sply_dat, sgFloat *U_new, short numu,
                        lpW_NODE P_new, sgFloat *u_new, short nump, short degree );
//-----------------------------------------------------nrb_work.c
BOOL Merge_Control_Vectors( lpVDIM ribs );
void Create_3D_From_4D( lpW_NODE node, sgFloat *point, sgFloat *w);
void Create_4D_From_3D( lpW_NODE node, sgFloat *point, sgFloat w);
sgFloat dpoint_distance_4d(lpW_NODE p1, lpW_NODE p2);
sgFloat dpoint_distance_4dl(lpW_NODE p1, lpW_NODE p2);
BOOL Detect_Multy_Insertion(sgFloat *U, short p, sgFloat t, short nump, short *s,
                            short *k);
//-----------------------------------------------------nrb_knot.c
BOOL Insert_Knot( sgFloat *U, lpW_NODE P, sgFloat *u, short nump, short p, sgFloat t,
                  sgFloat **U_new, lpW_NODE *P_new, sgFloat **u_new, short *nump_new,
                  short num_ins, short beg_ins, short mult);
short Del_Knot  ( sgFloat *U, lpW_NODE P, sgFloat *u, short *nump, short numu, short d, sgFloat t, short r);
BOOL Increase_Composite_Curve_Degree(lpW_NODE P_new1, lpW_NODE *P_new2,
                                     sgFloat *u_new1, sgFloat **u_new2,
                                      short nump_new1, short *nump_new2,
                                      short degree, short add);
//-----------------------------------------------------nrb_arc.c
//create splile semulating arc/circle
BOOL transform_to_NURBS(hOBJ hobj, hOBJ *hobj_new);

//-------------------------------------------------------------------------
extern   SPL_WORK spl_work;
//-------------------------------------------------------------------------

void nurbs_handler_err(short cod,...);



#define  MAX_POINT_ON_SURFACE       150 
#define SURF_NO_MEMORY      1 
#define SURF_INTERNAL_ERROR 4 
#define SURF_EMPTY_DATA     5
#define SURF_BAD_POINT      6 
//----------------------------------------------------------
#define SRF_DERIVATE_MOVE  0
#define SRF_MOVE           1
#define SRF_LINE_MOVE      2
//=======================SURFACE============================
extern MATR surf_matr;

typedef enum{
  SURF_NEW   = 0x0001,  
  SURF_GEO   = 0x0002,  
  SURF_APPR  = 0x0004,  
  SURF_INT   = 0x0008,  

  SURF_SKIN  = 0x0010,  
  SURF_RULED = 0x0020,  
  SURF_ROTATE= 0x0040,  
  SURF_COMMON= 0x0080,  
  SURF_CLOSEU= 0x1000,
  SURF_CLOSEV= 0x2000,
//#define  SPL        0x08  
} SURF_TYPE_IN;

typedef enum{
  SURF_POLEU0 = 0x0001,
  SURF_POLEU1 = 0x0002,
  SURF_POLEV0 = 0x0004,
  SURF_POLEV1 = 0x0008,
}SURF_POLES;

typedef struct {

  SURF_TYPE_IN sdtype;     
    SURF_POLES   poles;      
  short        degree_p;   
  short        degree_q;    
  MESHDD       knots;       
  lpMESHDD     derivates;  
  sgFloat       *u;          
  sgFloat       *v;          
  MESHDD       P;           
  sgFloat       *U;          
  sgFloat       *V;          
  short        num_u;       
  short        num_v;       
    short        orient;      

  int          allocSizeFor_u;
  int          allocSizeFor_v;
  int          allocSizeFor_U;
  int          allocSizeFor_V;
} SURF_DAT;  

typedef SURF_DAT * lpSURF_DAT;

//-------------------------------------->surface.c
hOBJ surface_edit       ( lpGEO_SURFACE geo_surf );
//BOOL surface_end        ( lpVLD vld, lpLNP lnp, lpGEO_SURFACE geo_surf );
hOBJ create_BREP_surface( lpGEO_SURFACE geo_surf, lpMESHDD mdd, bool clear_c_num_np = true );
//-------------------------------------->surf_edi.c
//GS_CODE edit_surf(void);
//GS_CODE edit_surf_knot(void);
//GS_CODE move_surf_knot(void);
//GS_CODE get_surf_knot(INDEX_KW *keys, char *prompt, short *num_u, short *num_v);
//BOOL    draw_surf_tb(void);
//void    clear_surf_tb(void);

//-------------------------------------->memory.c
void free_matrix_point( lpD_POINT *L, short n );
lpD_POINT * mem_matrix_point( short m, short n );
//-------------------------------------->surf_fin.c
BOOL find_surf_knot ( lpD_POINT vp, lpSURF_DAT surf, short *numk_u, short *numk_v);
//-------------------------------------->surf_uti.c
BOOL begin_use_surf ( lpGEO_SURFACE geo_surf, lpSURF_DAT surf_dat);
void end_use_surf   ( lpSURF_DAT surf_dat);
BOOL init_surf_dat  ( char sdtype, short p, short q, short type, lpSURF_DAT surf_dat );
void free_surf_data ( lpSURF_DAT surf);
BOOL create_geo_surf( lpSURF_DAT surf_dat, lpGEO_SURFACE geo_surf);
BOOL unpack_geo_surf( lpGEO_SURFACE geo_surf, lpSURF_DAT surf_dat);
void free_geo_surf  ( lpGEO_SURFACE geo_surf );
BOOL put_surf_ribs  ( lpSURF_DAT surf_dat, short type, lpVDIM ribs, BOOL closer );
void free_vdim_hobj ( lpVDIM rib_curve );
void free_vdim_sply ( lpVDIM ribs );
BOOL Create_MESHDD  ( lpMESHDD mdd, lpLISTH listho );
//-------------------------------------->surf_bas.c
BOOL surf_parameter  ( lpSURF_DAT surf_dat, short conditon, BOOL p );
BOOL calculate_surf_P( lpSURF_DAT surf_dat, short condition);
//-------------------------------------->surf_cre.c
BOOL move_surf_knot         ( lpSURF_DAT surf_dat, short i, short j, lpD_POINT point );
BOOL change_surf_weight_knot( lpSURF_DAT surf_dat, short i, short j, sgFloat W );
BOOL add_break_surf_knot    ( lpSURF_DAT surf_dat, short i, short j );
BOOL move_surf_line         ( lpSURF_DAT surf_dat, short count, short direction, sgFloat D, lpD_POINT W);
BOOL change_surf_weight_on_line ( lpSURF_DAT surf_dat, short count, short direction, sgFloat W);
BOOL add_break_surf_line        ( lpSURF_DAT surf_dat, short direction, short num );
//-------------------------------------->surf_use.c
BOOL knots_on_surface    ( lpSURF_DAT surf_dat);
BOOL get_point_on_surface( lpSURF_DAT surf_dat, sgFloat u, sgFloat v, lpD_POINT p);
BOOL get_deriv_on_surface(lpSURF_DAT surf_dat, sgFloat u, sgFloat v, lpD_POINT d, short deriv );
BOOL Bild_Mesh_On_Surf         ( lpSURF_DAT surf_dat, lpMESHDD mdd, short n, short m);
BOOL Bild_Mesh_On_Surf_Restrict( lpSURF_DAT surf_dat, lpMESHDD mdd, short middle_u,
                                 short middle_v, short ib, short ie, short jb, short je);
BOOL Bild_Array_On_Surf_Restrict( lpSURF_DAT surf_dat, lpD_POINT *array,
                                 short middle_u, short middle_v, short ib, short ie, short jb, short je);
BOOL Create_Ribs         ( lpLISTH list, lpVDIM ribs );
BOOL Reorient_Ribs       ( lpLISTH list, lpVDIM ribs, lpD_POINT spin );
BOOL Elevete_Rib_Degree  ( lpVDIM ribs, short degree, lpVDIM rib_curv );

BOOL point_on_surface    ( lpSURF_DAT surf, lpMESHDD mdd, short n_u,
                           short n_v, short constr_u, short constr_v );
//-------------- nrb_inter.c
BOOL Surface_By_Point( lpSURF_DAT srf, void *point, sgFloat *tu, sgFloat *tv, sgFloat *dist );
//-------------- spl_prim.c
hOBJ Create_Cone_Surf  ( sgFloat r1_OX, sgFloat r1_OY, sgFloat r2_OX, sgFloat r2_OY, sgFloat h );
hOBJ Create_Sphere_Surf( sgFloat r_OX, sgFloat r_OY, sgFloat r_OZ );
hOBJ Create_Spheric_Band_Surf( sgFloat r, sgFloat min_c, sgFloat max_c );
hOBJ Create_Torus_Surf( sgFloat ro_OX, sgFloat ro_OY, sgFloat rn_OX, sgFloat rn_OY);
void Create_Full_Surface_of_Revolution( lpSPLY_DAT spline, lpSURF_DAT surface, sgFloat coeff);
hOBJ Create_Surface_of_Revolution( lpSPLY_DAT spline, sgFloat Fi, short type, BOOL closed );
hOBJ Create_Extruded_Surface( lpSPLY_DAT spline, lpMATR matr, BOOL closed );
BOOL Create_Surf_By_Mesh( lpMESHDD mdd, short degree_u, short degree_v, hOBJ *hrez);

//-------------- skinning.c
BOOL Skin_Surface      ( lpLISTH list, lpD_POINT spin, short n_section, short q, short p,
                         BOOL closed_u, BOOL closed_v, BOOL solid, hOBJ *hrez);
BOOL detect_number     ( lpVDIM rib_curves, sgFloat *u_compat, short n_section,
                         short degree,  short *n_point, BOOL closed  );
//--------------- coons.c
BOOL Coons_Surface     ( lpVDIM u1, lpVDIM v1, lpVDIM u2, lpVDIM v2, short n_u,
                         short n_v, short constr_u, short constr_v, hOBJ *hrez);
//----------------- ruled.c
BOOL Ruled_Surface     ( lpLISTH list, lpD_POINT spin, short q, short p, BOOL closed,
                         BOOL solid, hOBJ *hrez);
//BOOL Draw_Tangent_Arrow(hOBJ hobj, lpW_NODE origin);
//BOOL Get_Near_Node_Path( hOBJ hobj, lpW_NODE p );



#endif