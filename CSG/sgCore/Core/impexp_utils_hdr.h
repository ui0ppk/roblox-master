#ifndef __IMP_EXP_UTILS_HDR__
#define __IMP_EXP_UTILS_HDR__



#define MAX_REZ_OTR  MAXSHORT  
#define REZ_CONTINUE_TEST   FALSE//((rez_stat_flag)?TRUE:(RubberContinueTest()))

#define AE_NO_VMEMORY         300 
#define AE_BAD_FILE           301 
#define AE_BAD_STRING         302 
#define AE_ERR_LOAD_STRING    303 
#define AE_ERR_DATA           304 

#define PltUnitMM 0.02488       

void rfcvt(short cvtType, unsigned char *p, sgFloat *cd);

typedef enum {                                           //NB: 18/X-98
  DEV_HP, DEV_PIC, DEV_FRG, DEV_DXF, DEV_WRL, DEV_InteAr, DEV_WMF,
  DEV_BMP, //NB 22/I-01
  DEV_KTG = 300, DEV_GEMMA, DEV_STL,
} DEV_TYPE;

//=================== 2D Export====================
/*BOOL export_vi(DEV_TYPE dev, char * name, lpVPORT vp,
              lpF_PLPOINT vp1, lpF_PLPOINT vp2,
              lpF_PLPOINT devp1, lpF_PLPOINT devp2, BOOL mono, BOOL opt);*/
//=================== 3D Export====================
BOOL o_export_3d(DEV_TYPE dev,char * name,lpLISTH listh,
                NUM_LIST list_zudina );
BOOL  o_export_ktg(lpLISTH listh, NUM_LIST list_zudina, char * name);

//=================== VRML Export =================
BOOL export_3d_to_vrml(char *name);

//=================== STL-Export===================
extern bool stl_full_flag;
bool export_3d_to_stl(char *name);


typedef struct {
  lpNPW  npw;         
  short    num_np;      
  short    num_f;       
  short    max_v;       
  short    num_cf;      

  short    *mb;
  short    *me;
  short    *m;
  short    *mv;
  short    *mr;
  short    *mrv;
  short    l_vcont;
  UCHAR  *mpr;
  sgFloat *x;
  sgFloat *y;
  sgFloat epsf;
}TR_INFO;
typedef TR_INFO * lpTR_INFO;


BOOL trian_brep(hOBJ hobj, short max_v);
BOOL trian_np(lpNPW npw, short num_np, short max_v);
OSCAN_COD trian_geo_scan(hOBJ hobj, lpSCAN_CONTROL lpsc);

BOOL  check_brep(lpNP_STR_LIST list_str, sgFloat eps);
short check_vertex(lpNPW np, sgFloat eps);
short check_edge(lpNPW np);
BOOL check_loop(lpNPW np);
BOOL check_loop_area(lpNPW np, short face, sgFloat eps);

extern BOOL (*trian_pre_brep)  (hOBJ hobj);
extern BOOL (*trian_post_brep) (BOOL ret, hOBJ hobj);
extern BOOL (*trian_pre_np)    (lpNPW npw, short num_np);
extern BOOL (*trian_post_np)   (BOOL ret, lpNPW npw, short num_np);
extern BOOL (*trian_pre_face)  (lpNPW npw, short num_np, short numf);
extern BOOL (*trian_put_face)  (lpNPW npw, short numf, short lv, short *v, short *vi);
extern BOOL (*trian_post_face) (BOOL ret, lpNPW npw, short num_np, short numf);

extern lpTR_INFO tr_info;
extern BOOL      trian_error;

typedef union {
  GEO_ARC     arc;
  GEO_CIRCLE  circle;
  GEO_LINE    line;
}GEO_SIMPLE;
typedef GEO_SIMPLE * lpGEO_SIMPLE;


int  import(DEV_TYPE dev, char * name, lpLISTH listh, BOOL frame, lpMATR m, bool solid_checking);
//int  ImportPic(lpBUFFER_DAT bd, lpLISTH list, lpVLD vld, BOOL frame);
int  ImportHp (lpBUFFER_DAT bd, lpLISTH list, lpVLD vld, BOOL frame);
//int  ImportFrg(lpBUFFER_DAT bd, lpLISTH list, lpVLD vld, BOOL frame);
BOOL  cr_add_obj(OBJTYPE type, short cur_color, lpGEO_SIMPLE geo,
                 lpLISTH list, lpVLD vld, BOOL frame);
BOOL  get_nums_from_string(char *line, sgFloat *values, short *num);
short   ImportMesh(lpBUFFER_DAT bd, lpLISTH listh);
short   ImportDxf(lpBUFFER_DAT bd, lpLISTH listh);
short   ImportStl(lpBUFFER_DAT bd, lpLISTH listh, lpMATR m, bool solid_checking);




void a_handler_err(short cod,...);


BOOL check_model(hOBJ hobj, BOOL * pr,
                 BOOL (*np_put_otr1)(lpD_POINT p1, lpD_POINT p2));


#define FIRST_VER_HEAD  171   
#define VER_HEAD_1_1    171   //
#define VER_HEAD_1_2    172   //
#define VER_HEAD_1_3    173   // NB 24/XI-02
#define VER_HEAD_1_4    174   // Z 14/XII-02
#define LAST_VER_HEAD   174   // 

#define TITLE_FILE     "SGScene"  
#define OLD_TITLE_FILE "S-G "   

#define MAXCOMMENT 80

#define FPO_CURNAM   0x01
#define FPO_COMMENT  0x02
#define FPO_PIC      0x04
#define FPO_COLORPIC 0x08  // NB 24/XI-02
#define FPO_EH       0x10  // NB 24/XI-02

#define VER_HEAD_1_2_PROPS (FPO_CURNAM|FPO_COMMENT|FPO_PIC)
#define VER_HEAD_1_3_PROPS (VER_HEAD_1_2_PROPS|FPO_EH|FPO_COLORPIC)
//--------------------------------------

#define MIN_PASS_LEN 4
#define MAX_PASS_LEN 12
#define MAX_USERNAME 32
#define MAX_MACHINENAME 32
#define OLD_MAX_APPNAME 20
#define MAX_APPNAME 80

#define EH_SHRINK   0x01                   
#define EH_ENCODE   0x08                    
#define EH_TUTORIAL 0x10                   



#define BIT_SIZEX     104
#define BIT_SIZEY     88
#define BIT_SIZE_BUF  ((BIT_SIZEX*BIT_SIZEY)/8)


#define COLORPIC_BIT_SIZEX    (BIT_SIZEX)
#define COLORPIC_BIT_SIZEY    (BIT_SIZEY)
#define COLORPIC_PALETTE_SIZE (4*256)
#define COLORPIC_PIC_SIZE (COLORPIC_BIT_SIZEX * COLORPIC_BIT_SIZEY)
#define COLORPIC_SIZE_BUF (COLORPIC_PALETTE_SIZE + COLORPIC_PIC_SIZE)



#define FPTYPE_MISSINGPIC  0
#define FPTYPE_MONOPIC     1
#define FPTYPE_COLORPIC    2
//===========================================================================

bool  sg_save_model(char *name, BOOL sel_flag, int *count, 
            MATR matr, MATR matr_inv,
            const void* userData, unsigned long userDataSize);

BOOL  SGLoadHeader(lpBUFFER_DAT bd, lpD_POINT p_min, lpD_POINT p_max,
                   UCHAR* Header_Version);
BOOL  SGLoadModel(char* name, lpD_POINT p_min, lpD_POINT p_max,
                  MATR matr, lpLISTH listh);

//----------- dist_brp.c
sgFloat dist_point_to_hobj(lpD_POINT p, hOBJ hobj, lpD_POINT p_min);



#define LEN_NAME_LAYER 31
typedef struct {
  char name[LEN_NAME_LAYER+1];
  short color;
} LAYER;
typedef LAYER * lpLAYER;

typedef struct {
  int           gr;       
  int           i;        
  int           p;       
  int           f;       
  D_POINT       xyz;
  sgFloat        a;
  sgFloat        v;
  char          str[256]; 
  lpBUFFER_DAT  bd;
  MATR          ocs_gcs, gcs_ocs;
  VDIM          layers;
  int           num_proc;
  BOOL          binary;
}DXF_GROUP;
typedef DXF_GROUP * lpDXF_GROUP;

typedef struct {
  D_POINT p1, p2, p3, p4; 
  int     flag_v;         
  int     color;          
  int     layer;         
  BOOL    flag;          
} FACE4;
typedef FACE4 * lpFACE4;

//--------------------------------- get_grou.c
BOOL  get_group(lpDXF_GROUP dxfg);
//--------------------------------- ocs_gcs.c
void ocs_gcs(lpD_POINT N, MATR A, MATR iA);
//--------------------------------- dxf_extd
BOOL dxf_extrude(OBJTYPE type, short color, lpGEO_SIMPLE geo,
                 lpLISTH listh, BOOL flag_h, lpD_POINT v,
                 sgFloat height, BOOL closed);
//------------------------------------------------------ put_face.c
BOOL put_face_4(lpD_POINT p1, lpD_POINT p2, lpD_POINT p3, lpD_POINT p4,
                int flag_v, lpTRI_BIAND trb);
//--------------------------------- dxf_util.c
short dxf_color(short color);
BOOL dxf_cmp_func(void *elem, void *data);
BOOL skip_vertex(void);
void dxf_err(CODE_MSG code);

BOOL  dxf_text(lpLISTH listh);
BOOL  dxf_line(lpLISTH listh);
BOOL  dxf_arc(lpLISTH listh);
BOOL  dxf_circle(lpLISTH listh);                    //-- dxf_circ.c
BOOL  dxf_3dface(lpVDIM vdim);                      //-- dxf_face.c
BOOL  dxf_poly(lpLISTH listh);
BOOL  dxf_insert(lpLISTH listh, int last_block);    //-- dxf_ins.c
BOOL  dxf_solid(lpLISTH listh, lpVDIM faces);       //-- dxf_sol.c
BOOL  poly_path(sgFloat height, short flag, short type, short color,//-- pol_path.c
                lpLISTH listh2, BOOL flag_h, lpD_POINT v, lpD_POINT p10);
//------------------------------------------------------ pol_mesh.c
BOOL poly_mesh(short m, short flag, short color, lpLISTH listh);
BOOL poly_mesh64(short color, lpLISTH listh);         //-- pol_m64.c

extern lpDXF_GROUP dxfg;


//=================== DXF-Export===================
bool export_3d_to_dxf(char *name);
BOOL put_vertex_to_dxf(lpBUFFER_DAT bd, int gr_num, lpD_POINT p);
BOOL put_dxf_group(lpBUFFER_DAT bd, int GrNum, void *par);

typedef enum {

  DXF_SECTION,
  DXF_ENDSEC,
  DXF_HEADER,
  DXF_TABLES,
  DXF_TABLE,
  DXF_ENDTAB,
  DXF_LTYPE,
  DXF_LAYER,
  DXF_STYLE,
  DXF_VIEW,
  DXF_BLOCKS,
  DXF_ENTITIES,
  DXF_EOF,

  DXF_LINE,
  DXF_POINT,
  DXF_CIRCLE,
  DXF_ARC,
  DXF_TRACE,
  DXF_SOLID,
  DXF_TEXT,
  DXF_SHAPE,
  DXF_BLOCK,
  DXF_ENDBLK,
  DXF_INSERT,
  DXF_ATTDEF,
  DXF_ATTRIB,
  DXF_POLYLINE,
  DXF_VERTEX,
  DXF_SEQEND,
  DXF_3DLINE,
  DXF_3DFACE,

} DXF_KEYS;

extern char *dxf_keys[];


extern short  export_floating_precision;
extern bool dxf_binary_flag;
extern bool dxf_full_flag, dxf_flat_flag;
extern short  dxf_acad_version;
extern char dxf_binary_tytle[];
extern lpVDIM vd_dxf_layers;

void   auto_name(const char * prefix, char * name, short maxlen);


//-------------------------------------------------- sel_bl_1.c
BOOL select_block_from_list(char *title,
                            char *name, BOOL fnew);

//-------------------------------------------------- dis_up_b.c
BOOL  disable_upper_blocks(int num);



#endif