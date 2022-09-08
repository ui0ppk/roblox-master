#ifndef __OBJ_UTILS_HDR__
#define __OBJ_UTILS_HDR__

#include "common_hdr.h"

typedef enum {  D_BAD=0,            
                D_FALSE=0,          
                D_OK=1,             
                D_TRUE=1,          
                D_REFUSAL,          
                D_HOT_KEY,          
                D_HOT_REG,          
                D_NEXT,             
                D_NO_MEMORY,        
                D_DIR_TRUNC,       
                D_BAD_MODE,        
                D_HOTKEY_OVERFLOW,  
                D_HOTREG_OVERFLOW, 
                D_ERR_OPEN,        
                D_ERR_READ,        
                D_ERR_WRITE,       
                D_BAD_FILE,        
                D_CONFLICT_MODE,    
                D_PIN,              
                D_ERR_INT,          
                D_ERR_sgFloat,      
}D_COD;

typedef enum { START_VERSION = 13614,        
                VERSION_1_5 = START_VERSION,  
                VERSION_1_6,                  
                VERSION_1_7,                 
                VERSION_1_8,                  
                VERSION_1_9,                
                VERSION_1_10,                
                VERSION_1_11,                 
                VERSION_1_12,                 
                LAST_DOS_VERSION = VERSION_1_12, 
                WINDOWS_V101,                
                WINDOWS_V102,                 
                WINDOWS_V103,                 
                WINDOWS_V405,                 
                WINDOWS_V408,                
                WINDOWS_V501,                
                WINDOWS_V502,                 
                WINDOWS_V503,                 
                LAST_VERSION                 
}VERSION_MODEL;

typedef enum {CFG_VER_FIRST = 1,          
              CFG_VER_1 = CFG_VER_FIRST,  
              CFG_VER_LAST               
}CFG_VER;

#define   MAXBLOCKNAME      256    

#define   MAXVPORT          4     
#define   NUM_SEGMENTS      36     
#define   MIN_NUM_SEGMENTS  3     
#define   GAB_NO_DEF        1.e35F 

#define C_MAX_USER_COLOR 239   

#define CDEFAULT     240   
#define COBJ         241  
#define CTRANSP      242   
#define CTRANSPBLK   243   
#define CBLINK       244   

typedef enum { M_KREST, M_POINT, M_NULL, M_LIGHT, M_ROMB, M_BPLUS, M_LPLUS,
               M_NUM,
} MARKER_TYPE;

typedef enum {
  APPL_SG,
  APPL_PREPROC,
  APPL_POSTP,
  APPL_WCUTS,
  APPL_LOM,
  APPL_DEVELOP,
  APPL_SHIP,
  APPL_MEBEL,
  APPL_DOM,
  LAST_APPL_NUM
} APPL_NUMBER;  


typedef enum{
  G_CANCEL,
  G_ERR,
  G_DONE,
  G_OK,
  G_PIN,
  G_CMD,
  G_KEY,
  G_NUM,
  G_INT,
  G_VADR,
  G_UNK,
  G_OMIT,
  G_ADDTYPE,
  G_TEXT,
  G_IDENT,
  G_OBJ,
  G_REDEFINE,
  G_ARRAY,
  G_QUIT,
  G_UCCONSTANT,
  G_DEBUG,
  G_GLOBAL,
  G_PROTOBREAK,
} GS_CODE;

typedef enum {
    OLD_OTEXT,
    OLD_OPOINT,
    OLD_OLINE,
    OLD_OCIRCLE,
    OLD_OARC,
    OLD_OPATH,
    OLD_OBREP,
    OLD_OGROUP,
    OLD_OSURF,
    OLD_OCURVONSURF,
    OLD_ODIM,
    OLD_OINSERT,
    OLD_OSPLINE,
    OLD_ODUMMY5,
    OLD_ODUMMY6,
    OLD_OFRAME,
    OLD_LAST_OBJ = OLD_OFRAME,
} OLD_OBJTYPE;


typedef int OBJTYPE;
typedef OBJTYPE * lpOBJTYPE;

typedef SG_PTR_TO_DIGIT METHOD;
typedef METHOD * pMETHOD;

typedef DWORD EL_OBJTYPE; 
typedef EL_OBJTYPE * pEL_OBJTYPE;
#define ENULL 0

typedef enum {
    OTEXT,
    OPOINT,
    OLINE,
    OCIRCLE,
    OARC,
    OPATH,
    OBREP,
    OGROUP,
    ODIM,
    OINSERT,
    OSPLINE,
    OFRAME,
} COREOBJTYPE;

extern EL_OBJTYPE  EPOINT,
                   ELINE,
                   ECIRCLE,
                   EARC,
                   EBREP,
                   ESPLINE;



#define MAX_OBJECTS_TYPES 256     
#define LEN_FILTR  (MAX_OBJECTS_TYPES+7)/8
typedef unsigned char OFILTER[LEN_FILTR];


#define ERR_NO_VMEMORY        1  
#define ERR_NUM_OBJECT        2   
#define ERR_NP_MAX            3  
#define ERR_VERSION           4  
#define ERR_DATABASE_FILE     5   
#define ERR_BAD_PARAMETER     6   
#define ERR_NUM_BLOCK         7   
#define SAVE_EMPTY_BLOCK      8   
#define ERR_NO_HEAP           9   
#define ERR_NUM_FRAME         10  
#define ERR_NUM_PRIMITIVE     11  
#define ERR_MAX_NP            12  

typedef struct {
  WORD size;
  char data[1];
}CSG;


typedef enum { BODY, SURF, FRAME } BREPTYPE;

typedef enum { COMMON, BOX, CYL, SPHERE, CONE, TOR, PRISM, PYRAMID, ELIPSOID,
               SPHERIC_BAND, RECTANGLE, POLYGON, CIRCLE } BREPKIND;

typedef enum {
       ST_DEVICE      = 0x0001,    
       ST_SELECT      = 0x0002,    
       ST_BLINK       = 0x0004,    
       ST_FLAT        = 0x0008,     
       ST_CLOSE       = 0x0010,     
       ST_DIRECT      = 0x0020,     
       ST_CONSTR1     = 0x0040,    
       ST_CONSTR2     = 0x0080,    
       ST_ORIENT      = 0x0100,    
       ST_IDENT       = 0x0200,    
    ST_ON_LINE        = 0x0400,    
       ST_ZUD2        = 0x0800,    
       ST_SIMPLE      = 0x1000,     
       ST_WORK        = 0x2000,    
} OSTATUS;

typedef short ODRAWSTATUS;

#define  ST_TD_FRAME     0x0200     
#define  ST_HIDE         0x0100     
#define  ST_TD_BOX       0x0080     
#define  ST_TD_FULL      0x0040    
#define  ST_TD_BACKFACE  0x0020     

#define  ST_TD_CLEAR     0x003F    
#define  ST_TD_SKETCH    0x0010     
#define  ST_TD_DUMMY     0x0008    
#define  ST_TD_APPROCH   0x0004     
#define  ST_TD_COND      0x0002     
#define  ST_TD_CONSTR    0x0001     


typedef struct {
  int       SpecPointsCount;
  lpD_POINT pSpecPoints;
}DNM_INFO, * pDNM_INFO;

typedef struct {
    LIST                list;
    LIST                listz[2]; 
  OBJTYPE               type;      
  UCHAR               layer;   
  UCHAR               color;    
  UCHAR               ltype;    
  UCHAR               lthickness; 
  IDENT_V               irec;       
  VADR                hhold;     
                           
  ODRAWSTATUS             drawstatus;
  short               status;   
  VADR                hcsg;   
  sgCObject*              extendedClass;
  bool                isAttachedToScene;
  ISceneTreeControl::TREENODEHANDLE   handle_of_tree_node;
  DWORD                               bit_buffer_size;
  void*                               bit_buffer;
  VI_LOCATION             loc[MAXVPORT];  
  char                geo_data[1];   
} OBJ;

typedef struct {
  UCHAR layer;         
  UCHAR color;         
  UCHAR ltype;         
  UCHAR lthickness;    
  BOOL  flg_transp;    
} OBJ_ATTRIB, * lpOBJ_ATTRIB;

typedef enum { OSFALSE, OSTRUE, OSBREAK, OSSKIP, OSCONTAINS } OSCAN_COD;
struct a{
  OBJTYPE     type;            
  LPARAM      lparam;         
  void        *data;           
  lpMATR      m;              
  VI_LOCATION viloc;          
  OSTATUS     status;          
  ODRAWSTATUS drawstatus;     
  UCHAR       color;           
  UCHAR       ltype;           
  UCHAR       lthickness;      
  UCHAR       layer;           
  short       block_level;     
  BOOL        scan_draw;       
  BREPTYPE    breptype;        
  hOBJ        hobj;            
  OSCAN_COD (*user_pre_scan)      (hOBJ hobj, struct a *sc);
  OSCAN_COD (*user_geo_scan)      (hOBJ hobj, struct a *sc);
  OSCAN_COD (*user_post_scan)     (hOBJ hobj, struct a *sc);
};
typedef struct a            SCAN_CONTROL;
typedef SCAN_CONTROL    * npSCAN_CONTROL;
typedef SCAN_CONTROL * lpSCAN_CONTROL;

typedef WORD VNP;

typedef struct {
//  VI_LOCATION viloc;
  int offset;
  short  size;
} NP_LOC, * lpNP_LOC;

typedef struct {
  VDIM dim;
  BUFFER_DAT bd;
//  char name[MAXFILE+MAXEXT];
  char name[MAXPATH];
} NP_BUF, * lpNP_BUF;

// 1. Placement ;
typedef struct {
  D_POINT p;
  D_POINT x;
  D_POINT y;
} GEO_PLACEMENT;

// 2. Point ;
typedef D_POINT GEO_POINT;

// 3. Line;
typedef struct {
  D_POINT v1;
  D_POINT v2;
}GEO_LINE;

// 4. Circle;
typedef struct {
  sgFloat   r;
  D_POINT  n;          
  D_POINT  vc;
 }GEO_CIRCLE;

// 5. Arc;
typedef struct {
  sgFloat   r;
  D_POINT  n;      
  D_POINT  vc;
  D_POINT  vb;
  D_POINT  ve;
  sgFloat   ab;
  sgFloat   angle;
}GEO_ARC;

enum { PATH_COMMON, PATH_RECT, PATH_POLY };
typedef struct {
  MATR      matr;        
  D_POINT   min;         
  D_POINT   max;         
  LISTH     listh;        
  UCHAR     kind;         
}GEO_PATH;

typedef struct {
  short   count;      
  int     num;       
  short   num_np;    
  short/*BREPKIND*/ kind;     
  VADR    hcsg;       
  LISTH   listh;     
 
  LISTH    surf;      
  short    num_surf; 
}LNP;
typedef LNP * lpLNP;

typedef struct {
  MATR      matr;       
  D_POINT   min;        
  D_POINT   max;        
//  NP_DATA   npd;         
  short     type;       
  short     ident_np;   
  int     num;         
  
}GEO_BREP;

typedef struct {
  MATR      matr;        
  D_POINT   min;         
  D_POINT   max;          
  LISTH     listh;        
 }GEO_GROUP;


#define CONSTR_U    0x8000  
#define CONSTR_V    0x4000   
#define COND_U      0x2000  
#define COND_V      0x1000   

typedef struct {  
  D_POINT   p;
  short       num;     
}MNODE;
typedef MNODE  * lpMNODE;

#define  BS_ENABLE   0 
#define  BS_DISABLE  1  
#define  BS_DELETE   2  
#define  BS_HIDE     3 


#define BLK_EXTERN  0x01  
#define BLK_HOLDER  0x02  

typedef struct {
  char    name[MAXBLOCKNAME];  
  short     count;               
  D_POINT min;                 
  D_POINT max;                  
  LISTH   listh;               
  int    new_number;           
  char    stat;                 
  IDENT_V irec;                
  char    type;                 
}IBLOCK;
typedef IBLOCK  * lpIBLOCK;

typedef struct {
  MATR      matr;        
  D_POINT   min;         
  D_POINT   max;         
  int      num;          

}GEO_INSERT;

// 13. OSPLINE
typedef enum {
  SPLY_CUB    = 0x0001,      
  SPLY_POLY   = 0x0002,     
  SPLY_NURBS  = 0x0004,     
  SPLY_CLOSE  = 0x0100,     
  SPLY_APPR   = 0x0200,      
} SPLY_TYPE;

typedef struct {
  short     type;        
  VADR      hpoint;       
  VADR      hderivates;   
  VADR      hvector;      
  short     nump;         
  short     numd;         
  short     numu;         
  short     degree;       
}GEO_SPLINE;


typedef enum {
  NSURF_BEZE    = 0x0001,      //
  NSURF_NURBS   = 0x0002,      //
  NSURF_CLOSEU  = 0x0100,      //
  NSURF_CLOSEV  = 0x0200,      //
  NSURF_APPR    = 0x0400,      //
} NSURF_TYPE;

typedef struct {
//  GEO_BREP    geo;         
  NSURF_TYPE  type;         
  VADR        hpoint;       
  VADR        hparam;       
  VADR        hderivates;   
  short       numpu;        
  short       numpv;        
  short       numd;         
  short       degreeu;      
  short       degreev;      
  short       num_curve;    
  VADR        curve;       
  short       orient;       
}GEO_SURFACE;

typedef struct {
  short     m, n;        
  VDIM      vdim;        
}MESHDD;
typedef MESHDD           * lpMESHDD;

typedef struct {
  MATR    matr;     
  D_POINT min;       
  D_POINT max;       
  VLD     vld;       
  short   num;      
}GEO_FRAME;

typedef struct{
  char   name[MAXFILE16 + MAXEXT16 + 1];     // FONT NAME  (WITHOUT PAH)
  WORD   beg_sym;    // first symbol
  WORD   end_sym;    // last
  WORD   num_sym;    // table size
  UCHAR  num_up;     // size under zero
  UCHAR  num_dn;     // size over zero
  UCHAR  status;     // != 0 - no vertical font
  sgFloat lsym;       // proportions
  UCHAR  table[1];   // table
} FONT; 
typedef FONT *  lpFONT;

#define DRAW_TEXT_VERT  0x01  
#define TEXT_FIXED_STEP 0x02  

typedef struct{
  UCHAR  status;
  sgFloat height; 
  sgFloat scale;  
  sgFloat angle;  
  sgFloat dhor;   
  sgFloat dver;   
} TSTYLE;   
typedef TSTYLE *  lpTSTYLE;

typedef struct {
  I_LIST      list;    
  IDENT_V     sinfo;   
  WORD        count;   
  A_VALUE     v;       
}FT_ITEM;    
typedef FT_ITEM * lpFT_ITEM;

typedef enum {
 FTTEXT,
 FTFONT,
 FTARROW,
 FTSPS,
 FTLTYPE,
 FTHATCH,
 FTNUMTYPES,
}FTTYPE;

#define DRAFTTLEN     15
#define DRAFT_TITLE   "SolidgraphDri"
#define DRAFT_VERSION 1


typedef struct {
  VDIM      vd;
  VLD       vld;
  I_LISTH   ft_listh[FTNUMTYPES];
  I_LISTH   free_listh;
  IDENT_V   def_font;
  IDENT_V   curr_font;
  TSTYLE    curr_style;
  int       free_space;
  ULONG     max_len;
  char      *buf;
  lpIDENT_V idlist[FTNUMTYPES - FTARROW];
  char      draft_title[DRAFTTLEN];
  char      font_path[MAXPATH];
  char      exe_path[MAXPATH];
  char      text_path[MAXPATH];
  char      draft_path[MAXPATH];
}FT_BD;   
typedef FT_BD * lpFT_BD;

typedef struct {
  MATR    matr;     
  D_POINT min;      
  D_POINT max;      
  TSTYLE  style;    
  IDENT_V font_id; 
  IDENT_V text_id;  
}GEO_TEXT;
typedef GEO_TEXT  * lpGEO_TEXT;


#define FONT_EXT    ".shx"
#define TEXT_EXT    ".txt"

typedef struct{
  IDENT_V  font;       // font
  IDENT_V  text;       // text template
  TSTYLE   tstyle;     // text style
  UCHAR    format;     // values format
  sgFloat   snap;       // precision
  sgFloat   tlrcoeff;   // proportions
} DIMTSTYLE;           
typedef DIMTSTYLE *  lpDIMTSTYLE;

#define DTF_LINEAR     0   
#define DTF_ANGLE      1  
#define DTF_ANGMIN     2   
#define DTF_ANGMINSEC  3   
#define DTF_MSK        7

#define DIM_MAX_PREC    6     
#define DIM_AUTO_SUBST  "<>"  


typedef short DM_TYPE;
#define DM_LIN       0
#define DM_ANG       1
#define DM_RAD       2
#define DM_DIAM      3
#define DM_NUM_TYPES 4

#define MAX_SHP_LEN        10000
#define MAX_DRAFT_NAME     8

// ARROW
typedef struct{
  char   name[MAX_DRAFT_NAME + 1];    
  char   ngrid;      // arrow grid size
  char   xmin;       // gabarites
  char   xmax;
  char   ymin;
  char   ymax;
  char   ext_in;     
  char   ext_out;    
  char   out;        
  char   table[1];   
} ARRSHP; 
typedef ARRSHP   * lpARRSHP;

typedef struct{
  char   name[MAX_DRAFT_NAME + 1];     
  char   ngrid;                       
  char   ang;                         
  char   x[6];                         
  char   table[1];                     
} SPSSHP; 
typedef SPSSHP   * lpSPSSHP;

typedef struct{
  char   name[MAX_DRAFT_NAME + 1];    
  char   npar;                        
  sgFloat s[1];                         
} LINESHP; 
typedef LINESHP   * lpLINESHP;

#define MAX_SHTRIH  10
typedef struct {
  UCHAR   n;
  UCHAR   i;
  sgFloat  ls;
  sgFloat  lotr;
  BOOL    flag;
  BOOL    flagnl;
  sgFloat  s[MAX_SHTRIH];
  D_POINT v1;
  D_POINT v2;
} LTYPE_INFO;
typedef LTYPE_INFO  * lpLTYPE_INFO;

typedef struct{
  char   name[MAX_DRAFT_NAME + 1];    
  char   table[1];                    
} HATCHSHP; 
typedef HATCHSHP   * lpHATCHSHP;


typedef struct{
  sgFloat     extln_in;      
  sgFloat     extln_out;     
  WORD       flags;         
  UCHAR      tplace;       
  sgFloat     arrlen;       
  sgFloat     arrext;       
  UCHAR      arrtype[2];   
} DIMGRSTYLE;
typedef DIMGRSTYLE *  lpDIMGRSTYLE; 

#define DI_NUMWL 5
#define DI_NUMWA 1

typedef struct{
  DIMTSTYLE  dts[DM_NUM_TYPES]; 
  DIMGRSTYLE dgs;              
  UCHAR      defarr;           
  DM_TYPE    curr_type;        

  sgFloat     bstep;        
  sgFloat     valcoeff;      
  sgFloat     grfcoeff;      
  sgFloat     lpunkt;        

  sgFloat     hcoeff;       
  sgFloat     hangle;       
  WORD       htype;       
  UCHAR      hunits;       

  short        ldimflg[2];    
  short        adimflg;

  GEO_LINE   wlines[DI_NUMWL];  
  GEO_ARC    warcs [DI_NUMWA];

} DIMINFO;      
typedef DIMINFO *  lpDIMINFO;

#define DIM_LN_NONE     0x0001 
#define EXT1_NONE       0x0002 
#define EXT2_NONE       0x0004 
#define ARR1_OUT        0x0008 
#define ARR2_OUT        0x0010 
#define ARR_AUTO        0x0020 
#define ARR_DEFINE      0x0040 

#define DTP_AUTO        0x0100  

#define DTP_CENTER      0  
#define DTP_LEFT        1  
#define DTP_RIGHT       2  
#define DTP_ALONG       3  
#define DTP_MANUAL      4  
#define DTP_LIDER       5  

#define ARR_MASK        (ARR_AUTO|ARR1_OUT|ARR2_OUT)


typedef struct {
  DM_TYPE     type;
  DIMTSTYLE   dtstyle;
  DIMGRSTYLE  dgstyle;
  MATR        matr;    
  D_POINT     p[5];
  sgFloat      tcoeff;  
  UCHAR       numtp;   
  D_POINT     tp[2];   
  UCHAR       neg;     
} GEO_DIM;
typedef GEO_DIM   * lpGEO_DIM;


typedef struct {
  DM_TYPE     type;
  D_POINT     pb;
  D_POINT     pe;
  D_POINT     pc;
  sgFloat      l;
  sgFloat      r;
  sgFloat      ab;
  sgFloat      a;
  sgFloat      value;
  sgFloat      txg;
  sgFloat      tyg;
  BOOL        drawfl;
  D_POINT     gab[4];
  UCHAR       neg;
} DIMLN_GEO;
typedef DIMLN_GEO   * lpDIMLN_GEO;

typedef struct _rdm_ui{
  UCHAR      *text;
  lpFONT     font;
  lpGEO_DIM  geo;
}RDM_UI;
typedef RDM_UI   * lpRDM_UI;

typedef struct _rtt_ui{
  UCHAR      *text;
  lpFONT     font;
  lpTSTYLE   style;
}RTT_UI;
typedef RTT_UI   * lpRTT_UI;


#define LDIM_VERT   1
#define LDIM_HOR    2
#define LDIM_PAR    3
#define LDIM_ANG    4
#define LDIM_FREE   5
#define LDIM_FREE2  6
#define DDIM_LINEAR 7
#define DDIM_CENTER 8

#define LDIM_NEW      1
#define LDIM_BASE     2
#define LDIM_CONT     3
#define LDIM_ALONG    4
#define DIM_TP1       5
#define DIM_TP2       6
#define LDIM_ALONGC   7
#define LDIM_LIDER    8
#define LDIM_LIDERC   9
#define DIM_LIDER1   10
#define DIM_LIDER2   11
#define RDIM_ARC     12
#define RDIM_POINTS  13
#define LDIM_NEW3D   14
#define GEODIM_EDIT  15

#define ADIM_POZITIV 0  
#define ADIM_NEGATIV 1

void  init_draft_info(/*const char* applpath, const char* applname*/);
GS_CODE get_text_geo_par(lpGEO_TEXT gtext);
IDENT_V set_font(char *name, short *errcode);
lpFONT load_font(char *name, ULONG *lenfont, char *comment, short commlen,
                 short *errcode);
BOOL draw_geo_text(lpGEO_TEXT geo,
           BOOL (*sym_line)(lpD_POINT pb, lpD_POINT pe, void  *user_data),
           void  *user_data);
BOOL draw_text(lpFONT font, lpTSTYLE style, UCHAR *text,
           BOOL (*sym_line)(lpD_POINT pb, lpD_POINT pe, void  *user_data),
           void  *user_data);

BOOL draw_string(lpFONT font, UCHAR *str,
                 UCHAR status, sgFloat gdx, sgFloat gdy, sgFloat sx, sgFloat dh,
                 lpD_POINT p,
                 BOOL(*sym_line)(lpD_POINT pb, lpD_POINT pe, void  *user_data),
                 void  *user_data);
BOOL text_gab_lsk(lpD_POINT pb, lpD_POINT pe, void * gab);
BOOL draw_shp1(lpSPSSHP shp, BOOL vstatus, sgFloat gdx, sgFloat gdy, sgFloat sx,
               lpD_POINT p,
               BOOL(*sym_line)(lpD_POINT pb, lpD_POINT pe, void  *user_data),
               void  *user_data);
void ft_free_mem(void);
void ft_clear(void);
BOOL load_ft_value(IDENT_V item_id, void  *value, ULONG *len);
void  *alloc_and_get_ft_value(IDENT_V item_id, ULONG *len);
IDENT_V add_ft_value(FTTYPE fttype, void  *value, ULONG len);
ULONG ft_item_value(lpFT_ITEM ft_item, void  *value);
void delete_ft_item(FTTYPE fttype, IDENT_V id);
BOOL save_draft_info(short *errcode);
BOOL load_draft_info(short *errcode);
BOOL reload_draft_item(FTTYPE fttype, void  *shp, ULONG len);
BOOL load_ft_sym(FTTYPE fttype, WORD sym, void  *shp, ULONG *len);

sgFloat get_grf_coeff(void);
sgFloat get_dim_value(lpGEO_DIM geo);

void  odim_trans(lpGEO_DIM gdim, lpMATR matr);
BOOL dim_grf_create(lpGEO_DIM geo, lpFONT font, UCHAR *text);
sgFloat get_signed_angle(lpD_POINT pc, lpD_POINT p1, lpD_POINT p2, short sgn);
void get_dimarc_angles(lpD_POINT pc, lpD_POINT p1, lpD_POINT p2,
                      UCHAR neg, sgFloat *ab, sgFloat *a, sgFloat *r);
BOOL get_arc_proection(lpD_POINT pc, lpD_POINT pb, lpD_POINT pe, UCHAR neg,
                       lpD_POINT p, lpD_POINT pp, sgFloat *d);
BOOL get_arc_parpoint(lpD_POINT pc, lpD_POINT pb, lpD_POINT pe, UCHAR neg,
                      sgFloat d, lpD_POINT p, sgFloat *ap, UCHAR *inv);
short d_arc_aprnum(GEO_ARC *geo_arc, sgFloat h);
void d_get_point_on_arc(GEO_ARC *geo_arc, sgFloat alpha, lpD_POINT p);
sgFloat get_dimarc_len(lpD_POINT pc, lpD_POINT p1, lpD_POINT p2, UCHAR neg);

BOOL draw_geo_dim(lpGEO_DIM geo,
           BOOL (*dim_line)(lpD_POINT pb, lpD_POINT pe, void  *user_data),
           void  *user_data);

BOOL draw_dim(lpGEO_DIM geo,
              BOOL (*dim_line)(lpD_POINT pb, lpD_POINT pe, void  *user_data),
              void  *user_data);

BOOL draw_dim1(lpGEO_DIM geo, lpFONT font, UCHAR *text,
               BOOL (*dim_line)(lpD_POINT pb, lpD_POINT pe, void  *user_data),
               void  *user_data);


BOOL draw_arr_shp(sgFloat h, sgFloat hin, sgFloat hout, sgFloat a, lpD_POINT p,
                  UCHAR out, lpARRSHP arr,
                  BOOL (*dim_line)(lpD_POINT pb, lpD_POINT pe, void  *user_data),
                  void  *user_data);

BOOL draw_dim_text(sgFloat dimvalue, lpDIMTSTYLE dstyle, UCHAR *dimtext,
                   lpFONT font,
                   BOOL (*sym_line)(lpD_POINT pb, lpD_POINT pe, void  *user_data),
                   void  *user_data);

char *get_dim_value_text(sgFloat dimvalue, lpDIMTSTYLE dstyle, char *txt);

void get_dimt_str4(char *dimtext, char **str);
ULONG compact_dimt(char *dimtext);
BOOL test_dim_to_invert(lpGEO_DIM geo);
void invert_dim_points(lpGEO_DIM geo);
void invert_dim(lpGEO_DIM geo);

void dim_info_init(void);
void free_dim_info(void);

BOOL set_dim_geo(lpGEO_DIM geo_dim,short move_type, lpFONT fnt, UCHAR *txt,
          lpD_POINT f_p,  lpD_POINT s_p, lpD_POINT th_p,lpD_POINT four_p,lpD_POINT p1, short *errtype);

BOOL init_ltype_info(lpLTYPE_INFO lti, UCHAR lt, sgFloat lobj, BOOL closefl);
BOOL init_ltype_info1(lpLTYPE_INFO lti, lpLINESHP shp, sgFloat lobj, BOOL closefl);
BOOL get_next_shtrih_line(lpLTYPE_INFO lti, lpD_POINT v1, lpD_POINT v2);


extern  lpFT_BD     ft_bd;            
extern  TSTYLE      style_for_init;    
extern  lpDIMINFO   dim_info;          
extern  UCHAR       faf_flag;


typedef struct {
  short bv;                /* first vertex  */
  short ev;                /* second vertex  */
  short el;                /* edge label:
                          *  xxxx xxxx xxxx xcba
                          *    "a" smooth edge
                          *    "b" dirty edge
                          *    "c" non visible edge
                          *    "x" not use
                          */
}EDGE_FRAME;

typedef struct {
  short fp;                /* + face    */
  short ep;                /* + edge    */
  short fm;                /* - face   */
  short em;                /* - edge   */
}EDGE_FACE;

#define ST_FACE_PLANE 0x0001  
#define ST_FACE_CUT   0x0002   
typedef struct {
  short fc;      /* first contour of face */
  short fl;      /* face label:
                *  xxxx xxxx xxxx xxba
                */
}FACE;

typedef struct {
  short fe;                /* first edge */
  short nc;                /* next contour */
}CYCLE;

typedef struct {
    float r;
    float g;
    float b;
}VERTINFO;

typedef float GRU;

#define MAXNOV 250  
#define MAXNOE 400  
#define MAXNOF 202  
#define MAXNOC 222 

typedef enum { TNPB, TNPF, TNPW, TNPS} NPTYPE;

typedef struct {

  short  type;  
  short   ident;  
  short  surf;          // carrier 
  short  or_surf;       // 

  short   nov;    /* vertexes count */
  short   maxnov; 
  short   noe;    /* edges count */
  short   maxnoe; 
  short   maxgru; 
  short   nof;    /* faces count */
  short   maxnof; 
  short   noc;    /* contours count */
  short   maxnoc;

  /* verteses */
  D_POINT * v;

  VERTINFO * vertInfo;

  /* edges */
  EDGE_FRAME * efr;

  EDGE_FACE * efc;

  /* faces */
  FACE  *f;

  /* cycles */
  CYCLE  *c;

  GRU  * gru;

  D_PLANE  *p;

  void * user;
  REGION_3D  gab;        
  short   nov_tmp;
} NPW;

typedef enum {
  OECS_CIRCLE,    
  OECS_ARC,    
  OECS_NOT_DEF, 
} OARC_TYPE;
typedef OARC_TYPE * lpOARC_TYPE;

typedef struct {
  GEO_ARC   geo_arc;                
  MATR      ecs_gcs_arc;            
  MATR      gcs_ecs_arc;            
}ARC_DATA;
typedef ARC_DATA          * lpARC_DATA;

typedef OBJ              * lpOBJ;
typedef GEO_PLACEMENT    * lpGEO_PLACEMENT;
typedef GEO_POINT        * lpGEO_POINT;
typedef GEO_LINE         * lpGEO_LINE;
typedef GEO_CIRCLE       * lpGEO_CIRCLE;
typedef GEO_ARC          * lpGEO_ARC;
typedef GEO_PATH         * lpGEO_PATH;
typedef GEO_BREP         * lpGEO_BREP;
typedef GEO_GROUP        * lpGEO_GROUP;
typedef GEO_INSERT       * lpGEO_INSERT;
typedef GEO_SPLINE       * lpGEO_SPLINE;
typedef GEO_SURFACE      * lpGEO_SURFACE;
typedef GEO_TEXT         * lpGEO_TEXT;
typedef GEO_FRAME        * lpGEO_FRAME;

typedef CSG              * lpCSG;

typedef NPW                   NP;
typedef NP               * lpNP;
typedef NPW              * lpNPW;
typedef EDGE_FRAME       * lpEDGE_FRAME;
typedef EDGE_FACE        * lpEDGE_FACE;
typedef FACE             * lpFACE;
typedef CYCLE            * lpCYCLE;
typedef GRU              * lpGRU;
typedef VERTINFO         * lpVERTINFO;

typedef  struct
{
  sgCBRep* br;
  short    ii;
} TMP_STRUCT_FOR_BREP_PIECES_SETTING;
void         Set_i_BRepPiece(TMP_STRUCT_FOR_BREP_PIECES_SETTING*,lpNPW);

lpNPW        GetNPWFromBRepPiece(sgCBRepPiece*);
void         SetBRepPieceMinTriangleNumber(sgCBRepPiece*, int);
void         SetBRepPieceMaxTriangleNumber(sgCBRepPiece*, int);

#define MAX_SIMPLE_GEO_SIZE  1
typedef union {
  GEO_PLACEMENT placement;
  GEO_POINT     point;
  GEO_LINE      line;
  GEO_CIRCLE    circle;
  GEO_ARC       arc;
  GEO_SPLINE    spline;
  GEO_TEXT      text;
  GEO_DIM       dim;
  BYTE          buf[MAX_SIMPLE_GEO_SIZE];
}GEO_OBJ;
typedef GEO_OBJ * lpGEO_OBJ;


hOBJ      o_alloc(OBJTYPE type);
void      handler_err(short cod, ...);
void      o_free(hOBJ hobj,lpLISTH listh);


BOOL o_save_one_block(lpBUFFER_DAT bd, lpIBLOCK blk);
BOOL o_save_obj(lpBUFFER_DAT bd, hOBJ hobj);
BOOL o_save_list(lpBUFFER_DAT bd,lpLISTH listh, NUM_LIST list_zudina,
                  int * num_obj, MATR matr, MATR matr_inv);


void  appl_init(void);
VERSION_MODEL get_version_model(lpBUFFER_DAT bd);
BOOL LoadModelVersion(lpBUFFER_DAT bd, VERSION_MODEL *VerMod);
BOOL load_one_block(lpBUFFER_DAT bd);
BOOL o_load_obj(lpBUFFER_DAT bd,hOBJ * hobj);
BOOL o_load_list(lpBUFFER_DAT bd,lpLISTH listh, NUM_LIST list_zudina,
                  short *num_obj, UCHAR Header_Version, void* pProp);
BOOL o_load_list_dos(lpBUFFER_DAT bd,lpLISTH listh, NUM_LIST list_zudina,
                     short *num_obj,VERSION_MODEL verm, void* pProp);
void test_to_oem(char* str, short len);

BOOL o_copy_obj(hOBJ hobjin, hOBJ * hobjout, char * path);


BOOL o_trans(hOBJ hobj, MATR matr);
void trans_geo(OBJTYPE type, void * geo, lpMATR matr);
void get_scanned_geo(hOBJ hobj, lpSCAN_CONTROL lpsc, void * geo);


BOOL o_invert(hOBJ hobj);


BOOL get_object_limits_lsk(hOBJ hobj, lpD_POINT min, lpD_POINT max, lpMATR m);
BOOL get_object_limits(hOBJ hobj, lpD_POINT mn, lpD_POINT mx);
BOOL get_object_limits_lsk_draw(hOBJ hobj, lpD_POINT min, lpD_POINT max,
                                lpMATR m, BOOL draw);


BOOL create_block(lpLISTH listh, char * name, lpMATR m, char type,
                  int* number);
hOBJ create_insert_by_name(char * name, lpMATR m);
hOBJ create_insert_by_number(int number, lpMATR m);
void free_blocks_list_num(int num);
void free_blocks_list(void);
void free_block(lpIBLOCK blk);
BOOL find_block(char * name, int *num);
BOOL mark_block_delete(int number);
BOOL set_block_status(int number, char stat);
void free_block_num(int num);
BOOL create_count_blocks( lpLISTH listh, NUM_LIST list_zudina, lpVDIM vdim,
                          void * elem);


BOOL      story_np(lpBUFFER_DAT bd);
BOOL      load_np(lpBUFFER_DAT bd);
BOOL      o_expand_np(lpNPW np, short nov, short noe, short nof, short noc);
BOOL      put_np_brep(lpLNP lnp, int *num);
//BOOL      get_np_brep(lpNP_DATA npd, lpLISTH listh, short *num_np);
BOOL      free_np_brep(int num);
BOOL      free_np_brep_list_num(int num);
void      free_np_brep_list(void);

lpNPW     creat_np_mem(NPTYPE type,short nov,short noe, short noc, short nof, short nogru);
void      free_np_mem(lpNPW * np);
void      free_np_mem_tmp(void);
BOOL      create_np_mem_tmp(void);
BOOL      defrag_np_mem_tmp(void);
void      get_first_np_loc( lpLISTH listh, lpVI_LOCATION viloc);
BOOL      read_np_mem(lpNPW npw);
BOOL      add_np_mem(lpVLD vld,lpNPW npw, NPTYPE type);
void      rezet_np_mem(lpVI_LOCATION lploc,lpNPW npw, NPTYPE type);


short       otype_num(OBJTYPE type);
void      add_type_list(OFILTER ofiltr,short num, ...);
void      add_type(OFILTER ofiltr,OBJTYPE type);
BOOL      check_type(OFILTER ofiltr,OBJTYPE type);


BOOL      begin_rw_np(lpNP_BUF np_buf);
BOOL      end_rw_np(lpNP_BUF np_buf);
VNP       write_np(lpNP_BUF np_buf,lpNPW npw,NPTYPE type);
BOOL      read_np(lpNP_BUF np_buf,VNP vnp,lpNPW npw);
BOOL      overwrite_np(lpNP_BUF np_buf,VNP vnp, lpNPW npw, NPTYPE type);


short  o_init_ecs_arc(lpARC_DATA ad,lpGEO_ARC arc, OARC_TYPE flag, sgFloat h);
void o_ecs_to_gcs(lpARC_DATA ad, lpD_POINT from, lpD_POINT to);
void o_get_point_on_arc(lpARC_DATA ad,sgFloat alpha, lpD_POINT p);
void o_trans_arc(lpGEO_ARC p, lpMATR matr, OARC_TYPE flag);


void      init_scan(lpSCAN_CONTROL lpsc);
OSCAN_COD o_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);


void frame_read_begin(lpGEO_FRAME geo);
BOOL frame_read_next(OBJTYPE* type, short* color, UCHAR* ltype, UCHAR* lthick,
                     lpGEO_OBJ  geo);
BOOL explode_frame(hOBJ hobj, lpLISTH listh);


BOOL      load_csg(lpBUFFER_DAT bd,hCSG * hcsg);
BOOL      save_csg(lpBUFFER_DAT bd,hCSG hcsg);
void      free_csg(hCSG hcsg);


BOOL    GeoObjectsInit(void);
void    GeoObjectsFreeMem(void);
SG_PTR_TO_DIGIT   GetObjMethod(OBJTYPE Type, METHOD iMethod);
SG_PTR_TO_DIGIT*  GetMethodArray(METHOD iMethod);
void    SetTmpObjMethod(OBJTYPE Type, METHOD iMethod,  SG_PTR_TO_DIGIT MethodData);
METHOD  RegCommonMethod(SG_PTR_TO_DIGIT IdleMethod);
OBJTYPE RegObjType(char* ObjName, int ObjGeoSize, int OldType, pEL_OBJTYPE pElObjType, BOOL bInGeoObj);
OBJTYPE RegCoreObjType(COREOBJTYPE Type, char* ObjName, int ObjGeoSize, int OldType,
                       pEL_OBJTYPE pElObjType, BOOL bInGeoObj);
BOOL    RegObjMethod(OBJTYPE Type, METHOD iMethod, SG_PTR_TO_DIGIT MethodData);
void    RegCoreObjects(void);
void    RegCommonMethods(void);
void    InitCoreObjMetods(void);
OBJTYPE FindObjTypeByName(char* TypeName);
OBJTYPE FindObjTypeByOldType(int OldType);


BOOL o_lcs(hOBJ hobj, MATR matr_lcs_gcs, BOOL *left);

void SetModifyFlag(BOOL bFlag);
BOOL GetModifyFlag(void);


extern BOOL (*expand_user_data)(lpNPW npw,short nov, short noe,
                                short nof, short noc);
extern BOOL (*read_user_data_buf)(lpNP_BUF np_buf,lpNPW npw);
extern BOOL (*write_user_data_buf)(lpNP_BUF np_buf,lpNPW npw);
extern BOOL (*add_user_data_mem)(lpVLD vld,lpNPW npw);
extern BOOL (*read_user_data_mem) (lpNPW npw);
extern BOOL (*rezet_user_data_mem) (lpNPW npw);
extern WORD (*size_user_data)(lpNPW npw);

extern void (*free_extern_obj_data)(hOBJ hobj);
extern void (*free_extern_obj_blk_data)(hOBJ hobj);

extern lpNPW        npwg;
extern OFILTER      enable_filter;

extern OFILTER      struct_filter;
extern OFILTER      complex_filter;
extern OFILTER      frame_filter;
#define FF_IS_STRUCT       0x00000001       
#define FF_HAS_MATRIX      0x00000002       
#define FF_FRAME_ITEM      0x00000004      

extern WORD*        geo_size;
extern OBJ_ATTRIB   obj_attrib;     
extern int          scan_level;
extern VDIM         vd_blocks;     
extern VDIM         vd_brep;       
extern sgFloat       vi_h_tolerance; 
                                    
extern BOOL         scan_flg_np_begin; 
extern BOOL         scan_flg_np_end;   
extern BOOL         scan_true_vision;  
extern short        num_blk_prev;    
                    
extern UCHAR        blink_color;    

extern  APPL_NUMBER o_appl_number;    
extern  int         o_appl_version;    

extern  BOOL        bOemModelFlag;    

extern  VERSION_MODEL ver_load;
extern  APPL_NUMBER   appl_number_load;
extern  short         appl_version_load;
extern  hOBJ          hhold_load;
extern  char          flg_list_load;

extern  sgFloat      sgw_unit;     
extern  sgFloat      file_unit;    

extern  OBJTYPE     ALL_OBJECTS;      
extern  OBJTYPE     NULL_OBJECTS;     
extern  int         GeoObjTypesCount; 
                                     
extern  char**      ppGeoObjNames;    
extern  pEL_OBJTYPE pElTypes;        

/*
   BE(r,m) - first vertes of edge
   EE(r,m) - end vertex of edge
   SL(r,m) - edge after r
   EF(r,m) - face with r
             m - lpNPW
*/

#define BE(r,m)   ((r) > 0 ? m->efr[r].bv : m->efr[-(r)].ev)
#define EE(r,m)   ((r) > 0 ? m->efr[r].ev : m->efr[-(r)].bv)
#define SL(r,m)   ((r) > 0 ? m->efc[r].ep : m->efc[-(r)].em)
#define EF(r,m)   ((r) > 0 ? m->efc[r].fp : m->efc[-(r)].fm)

#define NP_BAD_MODEL        1001  
#define NP_BAD_PLANE        1002  
#define NP_GRU              1003  
#define NP_NO_IDENT         1004  
#define NP_DEL_EDGE         1005  //
#define NP_DEL_FACE         1006  //
#define NP_LOOP_IN_PLANE    1007  //
#define NP_AN_CNT           1008  //
#define NP_CNT_BREAK        1009  
#define NP_NO_CORRECT       1010  
#define NP_NO_EDGE          1011  
#define NP_MESH4            1012 
#define NP_NO_HEAP          1013  
#define NP_DIV_FACE         1014  

typedef enum { NP_WRITE_MEM, NP_WRITE_STR} NP_WR_TARGET;
typedef enum { NP_FIRST, NP_ALL, NP_CANCEL} REGIMTYPE;
typedef enum { NP_IN, NP_OUT, NP_ON} NP_TYPE_POINT;

typedef VADR                 hNP_STR;
typedef VADR                 hNP_STR_LIST;
typedef VADR                 hNP_VERTEX;
typedef VADR                 hNP_VERTEX_LIST;
typedef VADR                 hNP_USER_GRU;

typedef struct{
  short ident;                 
  VNP hnp;                  
  short ort;                   
  short lab;                   
  short len;                   
  int b;                    
  REGION_3D gab;                                            
} NP_STR;

typedef struct{
  VDIM    vdim;             
  VDIM    nb;               
  NP_BUF  bnp;
  int     number_np;
  int     number_all;        
  char    flag;              
} NP_STR_LIST;

typedef struct {
  short m;                      
  short v;                      
  short r;                       
} NP_VERTEX;

typedef struct {
  short uk;                          
  short maxuk;                     
  VADR hv;                        
  NP_VERTEX * v;              
} NP_VERTEX_LIST;

typedef struct {
  WORD mem : 14;     
  WORD flg :  1;      
  WORD met :  1;      
} NP_BIT_FACE;           

typedef struct {
  short cm;                    // number of "-" contour
  short cp;                    // number of "+" contour
  sgFloat mm;                 // label of "-" contour
  sgFloat mp;                 //label of "+" contour
  short mem;
} NP_LOOPS;

typedef struct {    
  float  bnd;                                    
  short    ind;                            
} NP_USER_GRU;

typedef struct {             
  short left;                   
  short rigth;                   
  short first;                   
  short last;                   
  sgFloat x[4];               
  sgFloat y[4];
  sgFloat z[4];
} MESH_TYPE;

#define TRP_CONSTR  0x01
#define TRP_APPROCH 0x02
#define TRP_DUMMY   0x03

typedef struct {
  short     v[3];     // indexes of vertexes
  UCHAR   constr;   
}NPTRI;
typedef NPTRI * lpNPTRI;

typedef struct {
  D_POINT v[3];     // indexes of vertexes
  D_POINT color[3];
  UCHAR   constr;   
}NPTRP;
typedef NPTRP * lpNPTRP;

typedef struct {
  VDIM        coor;       // vertexes array
  VDIM        vdtri;      // trianlges array
  NP_STR_LIST list_str;   // Strategies
  short         *indv;      // coordinates
  short         num_indv;   // count
  short         ident;
}TRI_BIAND;
typedef TRI_BIAND * lpTRI_BIAND;

typedef NP_STR          * lpNP_STR;
typedef NP_STR_LIST     * lpNP_STR_LIST;
typedef NP_VERTEX       * lpNP_VERTEX;
typedef NP_VERTEX_LIST  * lpNP_VERTEX_LIST;
typedef NP_BIT_FACE     * lpNP_BIT_FACE;
typedef NP_LOOPS        * lpNP_LOOPS;
typedef NP_USER_GRU     * lpNP_USER_GRU;
typedef NP_TYPE_POINT   * lpNP_TYPE_POINT;


void np_gab_init(lpREGION_3D gab);                 
BOOL np_gab_inter(lpREGION_3D m1, lpREGION_3D m2);             
void np_gab_un(lpREGION_3D g1, lpREGION_3D g2, lpREGION_3D g3); 
short  np_gab_pl(lpREGION_3D gab, lpD_PLANE pl, sgFloat eps);
                                

BOOL np_approch_to_constr(lpNPW np, float angle);
BOOL np_constr_to_approch(lpNPW np, float angle);
BOOL np_cplane(lpNPW np);              
BOOL np_cplane_face(lpNPW np, short face);
void np_define_edge_label(lpNPW np, float angle);
BOOL np_del_sv( lpNP_STR_LIST  list_str, short *num_np , lpNPW np );
void np_init(lpNP np);                 
void np_inv(lpNPW np, REGIMTYPE regim);  
short  np_max(lpNPW np);                   
NP_TYPE_POINT np_point_np(lpD_POINT p, lpNPW np); 
sgFloat np_point_dist_to_face(lpD_POINT p, lpNPW np, short face,
                             lpD_POINT p_min); 
sgFloat np_point_dist_to_np(lpD_POINT p, lpNPW np, sgFloat f_min,
                           lpD_POINT p_min); 

short  np_new_edge(lpNPW np, short v1, short v2);    
short  np_new_face(lpNPW np, short cnt);           
short  np_new_loop(lpNPW np, short first_edge);     
short  np_new_vertex(lpNPW np, lpD_POINT v);     
BOOL np_face(lpNPW np, sgFloat r, short n, sgFloat h,short *num_np);

BOOL np_del_edge(lpNPW np);             
BOOL np_del_face(lpNPW np);              
BOOL np_del_loop(lpNPW np);              
BOOL np_del_vertex(lpNPW np);           

void np_free_np_tmp(void);
BOOL np_read_np_tmp(lpNPW npw);
BOOL np_write_np_tmp(lpNPW npw);

BOOL np_new_user_data(lpNPW np);
void np_free_user_data(lpNPW np);
BOOL np_add_user_data_mem(lpVLD vld,lpNPW npw);
BOOL np_read_user_data_mem(lpNPW npw);

BOOL  np_an_loops(lpNPW np, short face, short cnt1, short cnt2, short edge);
void  np_av(short mt, short *p,short *pr,short *pt, BOOL *le);
short   np_calc_gru(lpNPW np, short e);
short   np_def_loop(lpNPW np, short face, short edge, short *edgel);
BOOL  np_def_angle(lpNPW np, short v3, short r1, short r2, short face);
short   np_divide_edge(lpNPW np, short edge, short v);
short   np_edge_exist(lpNPW np, short v1, short v2);
BOOL  np_fa(lpNPW np, short face);
BOOL  np_fa_one(lpNPW np, short face, lpNP_LOOPS * ll, short * loops,
                BOOL msg);
void  np_fa_ef(lpNPW np, short face_old);
float np_gru1(lpNPW np, short r, lpD_POINT f1, lpD_POINT f2);
float np_gru(lpNPW np, short r, lpD_POINT f1, lpD_POINT f2);
BOOL  np_include_ll(lpNPW np,short face, short loop1, short loop2);
short   np_include_vl(lpNPW np, short v, short loop, short k1, short k2);
void  np_insert_edge(lpNPW np, short edge, short edgel);
BOOL  np_ort_loop(lpNPW np, short face, short loop, sgFloat *sq);
BOOL  np_point_belog_to_face(lpNPW np, lpD_POINT v, short face);
short   np_pv(lpNPW np1, short r, lpD_PLANE pl);
NP_TYPE_POINT np_point_belog_to_face1(lpNPW np, lpD_POINT v, short face,
                                      short* edge, short* vv);
lpNP_LOOPS np_realloc_loops(lpNP_LOOPS l, short * loops);
short   np_search_edge(lpNPW npw, lpD_POINT bv, lpD_POINT ev, short ident);
BOOL  np_vertex_normal(lpNP_STR_LIST list_str, lpNPW np, short edge, lpD_POINT p,
                       BOOL (*user_last)(lpNPW np, short edge), BOOL brk);
BOOL RA_np_vertex_normal(sgCBRep* brep, short piece, short edge, lpD_POINT p,
             BOOL (*user_last)(lpNPW np, short edge), BOOL brk);


short  np_defc(lpD_POINT p1, lpD_POINT p2);
void np_free_stack(void);
void np_free_ver(lpNP_VERTEX_LIST ver);
BOOL np_pl(lpNPW np1, short face1, lpNP_VERTEX_LIST ver1,lpD_PLANE pl);
void np_sort_ver(lpD_POINT v, lpNP_VERTEX_LIST ver, short axis);
BOOL np_ver_alloc(lpNP_VERTEX_LIST ver);

short  np_24_init(short kup, short kvp, lpVDIM vdimp,
                    short* ident_freep, BOOL pr);
BOOL np_m24_next(lpNPW npw);
BOOL np_m23_next(lpNPW npw);
BOOL np_mesh3p(lpNP_STR_LIST list, short *ident, lpMESHDD mdd,sgFloat angle);
BOOL np_mesh4p(lpNP_STR_LIST list, short *ident, lpMESHDD mdd,sgFloat angle);
BOOL np_create_brep(lpMESHDD mdd, lpLISTH listh);
BOOL np_mesh3v(lpNP_STR_LIST list, lpNPTRP trp, short *ident, lpVDIM coor, sgFloat angle,
              lpVDIM vdtri);
BOOL  begin_tri_biand(lpTRI_BIAND trb);
BOOL  put_tri(lpTRI_BIAND trb, lpNPTRP trp, bool intellect_adding=false);
BOOL  end_tri_biand(lpTRI_BIAND trb, lpLISTH listho, lpNPTRP trp = NULL);


//-------------- cin_end.cpp
BOOL  cinema_end(lpNP_STR_LIST c_list_str, hOBJ *hobj);
hOBJ  make_group(lpLISTH listh, BOOL one);

//-------------- surf_end.cpp
BOOL surface_end( lpVLD vld, lpLNP lnp, lpGEO_SURFACE geo_surf );

//-------------- div.cpp
BOOL sub_division( lpNPW np_big );

short np_end_of_put(lpNP_STR_LIST list, REGIMTYPE regim, float angle, lpLISTH);
int  np_end_of_put_1(lpNP_STR_LIST list, REGIMTYPE regim, float angle,
                     lpLISTH list_brep, sgFloat precision);
BOOL np_get_str(lpNP_STR_LIST list,short ident,lpNP_STR str1,short *index);
void np_zero_list(lpNP_STR_LIST list);
BOOL np_init_list(lpNP_STR_LIST list);
BOOL np_put(lpNPW hnp,lpNP_STR_LIST list);
BOOL np_str_form(BOOL (*user_gab_inter)(lpREGION_3D g1,lpREGION_3D g2),
                 lpNP_STR_LIST list1,lpNP_STR_LIST list2);
void np_str_free(lpNP_STR_LIST list);

void np_handler_err(short cod,...);


extern sgFloat       c_smooth_angle; // in degrees
extern NP_STR_LIST  c_list_str;
extern short        c_num_np;

extern short          np_axis;

extern short          flg_exist;


hOBJ create_primitive_obj(BREPKIND kind, sgFloat  *par);
short  create_primitive_np(BREPKIND kind,lpLISTH listh, sgFloat  *par);
BOOL o_create_box(sgFloat  *par);
short  create_box_np(lpLISTH listh, sgFloat  *par);
short  create_cone_np(lpLISTH listh, sgFloat  *par, BOOL constr);
short  create_cyl_np(lpLISTH listh, sgFloat  *par, BOOL constr);
short  create_sph_np(lpLISTH listh, sgFloat  *par);
short  create_tor_np(lpLISTH listh, sgFloat  *par);

short create_elipsoid_np(lpLISTH listh, sgFloat  *par);
short create_spheric_band_np(lpLISTH listh, sgFloat  *par);




#endif