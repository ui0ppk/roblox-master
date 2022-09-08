#ifndef __ATTRIB_UTILS_HDR__
#define __ATTRIB_UTILS_HDR__


BOOL fc_assign_curr_ident(hOBJ hobj);
void fc_id_mark(hOBJ hobj, UCHAR type);
void fc_id_mark1(hOBJ hobj, UCHAR type);

typedef enum {    
  ATTR_STR,       
  ATTR_sgFloat,    
  ATTR_BOOL,     
  ATTR_TEXT,      
  NUM_ATTR        
} ATTR_TYPE;


#define AT_HEAP      1  
#define AT_ERR_FILE  2   
#define AT_DIFF_TYPE 3   
#define AT_EMPTY_DB  4   


#define MAX_ATTR_STRING         C_MAX_STRING_CONSTANT
#define MAX_ATTR_NAME           C_MAX_IDENT_NAME
#define MAX_ATTR_TEXT           30


#define ATT_FREE           0x00  
#define ATT_USED           0x01  
#define ATT_SYSTEM         0x02  
#define ATT_RGB            0x04  
#define ATT_PATH           0x08  
#define ATT_DEFAULT        0x10  
#define ATT_ENUM           0x20  
#define ATT_MARK           0x40  

typedef struct { 
  I_LIST     list;                     
  IDENT_V    sinfo;                    
  char       name[MAX_ATTR_NAME + 1];  
  char       text[MAX_ATTR_TEXT + 1];  
  UCHAR      type;                     
  UCHAR      status;                  
  UCHAR      size;                     
  short      precision;                
  IDENT_V    curr;                     
  I_LISTH    ilisth;                  
}ATTR;
typedef ATTR * lpATTR;

#define MAX_BOOL_TEXT  MAX_INLINE_VALUE - 2
typedef struct {
  UCHAR b;
  char  tb[MAX_BOOL_TEXT + 1];
} BOOL_VALUE;
typedef BOOL_VALUE * lpBOOL_VALUE;

typedef struct {
  I_LIST      list;    
  IDENT_V     sinfo;   
  IDENT_V     attr;    
  UCHAR       type;    
  A_VALUE     v;      
}ATTR_ITEM;
typedef ATTR_ITEM * lpATTR_ITEM;

typedef struct {
  IDENT_V     attr;
  IDENT_V     item;
}RECORD_ITEM;
typedef RECORD_ITEM * lpRECORD_ITEM;

typedef struct {
  I_LIST      list;        
  IDENT_V     sinfo;        
  short       num;          
  WORD        count;       
  NOT_INLINE  vbd;          
}ATTR_RECORD;
typedef ATTR_RECORD * lpATTR_RECORD;

#define REPOUT_EXT  ".txt"  
#define DBE_EXT     ".exd"  
#define DBF_EXT     ".dbf"  

extern ULONG   attr_size_value[]; 

extern lpVDIM  vd_attr;         
extern lpVDIM  vd_attr_item;   
extern lpVDIM  vd_record;       
extern lpVLD   vld_attr_item;   
extern lpVLD   vld_record;      

extern lpI_LISTH attr_ilisth;          
extern lpI_LISTH free_attr_ilisth;     
extern lpI_LISTH free_attr_item_ilisth; 
extern lpI_LISTH record_ilisth;         
extern lpI_LISTH free_record_ilisth;   

extern IDENT_V   attr_work_irec;   

extern WORD   max_record_len;      
extern ULONG  max_attr_item_len;    
extern ULONG  record_free_space;    
extern ULONG  attr_item_free_space; 

extern UCHAR max_attr_text;
extern UCHAR max_attr_name;

extern void    (*u_attr_exit)(void);

extern UCHAR obj_attr_flag;       

void init_attr_bd(void);
void free_attr_bd(void);
void free_attr_bd_mem(void);
IDENT_V find_attr_by_name(char *name);
BOOL  add_attr(lpATTR attr, lpIDENT_V index);
IDENT_V create_sys_attr(char *name, char *text,
                      UCHAR type, UCHAR size, short prec,
                      WORD addstatus, void  *value);
void del_attr(IDENT_V attr_id);
lpATTR lock_attr(IDENT_V attr_id);
void unlock_attr(void);
void read_attr(IDENT_V attr_id, lpATTR attr);
void write_attr(IDENT_V attr_id, lpATTR attr);
IDENT_V get_next_attr(IDENT_V attr_id);
BOOL retype_attr(IDENT_V attr_id, UCHAR newtype);
BOOL  add_attr_value(IDENT_V attr_id, void  *value, lpIDENT_V item_id);
short compare_attr_item(IDENT_V item1, IDENT_V item2);
short compare_attr_item_and_new_value(IDENT_V item, void  *value);
ULONG get_attr_item_value(IDENT_V item_id, void  *value);
IDENT_V attr_to_var(IDENT_V attr_id, IDENT_V item_id);
void  *alloc_and_get_attr_value(IDENT_V item_id, UCHAR *type, ULONG *len);
char* get_attr_item_string(IDENT_V attr_id, IDENT_V item_id, BOOL format,
                           char *string);
/*NB++*/
BOOL get_attr_item_sgFloat(IDENT_V attr_id, IDENT_V item_id, sgFloat *val);
/*NB--*/

BOOL create_dtt_by_attr(IDENT_V attr_id, IDENT_V item_id, lpDTED_TEXT dtt);
BOOL create_attr_by_dtt(lpDTED_TEXT dtt, IDENT_V attr_id, lpIDENT_V item_id);
ULONG get_attr_curr_value(lpATTR attr, void  *value);
lpATTR_ITEM lock_attr_item(IDENT_V item_id);
void unlock_attr_item(void);
void read_attr_item(IDENT_V item_id, lpATTR_ITEM attr_item);
void write_attr_item(IDENT_V item_id, lpATTR_ITEM attr_item);
IDENT_V get_next_attr_item(IDENT_V item_id);
void free_attr_value(lpATTR_ITEM attr_item);
BOOL add_not_inline_data(lpVLD vld, lpNOT_INLINE ni, ULONG len,
                         void * data);
BOOL story_attr_string(lpBUFFER_DAT bd, char *str);
BOOL place_attr_value(lpATTR_ITEM attr_item, void  *value);

BOOL get_hobj_attr_value(IDENT_V id_attr, hOBJ hobj, void  *value);
IDENT_V get_hobj_attr_item_id(IDENT_V id_attr, hOBJ hobj);
BOOL get_attr_default_value(IDENT_V id_attr, void  *value);
BOOL set_attr_default_value(IDENT_V id_attr, void  *value);
BOOL set_hobj_attr_value(IDENT_V id_attr, hOBJ hobj, void  *value);
void copy_obj_attrib(hOBJ hobjin, hOBJ hobjout);
IDENT_V get_block_irec(int bnum);
IDENT_V get_hobj_irec(hOBJ hobj);
BOOL add_attr_to_hobj(hOBJ hobj, IDENT_V rec_id);
BOOL add_attr_to_block(int bnum, IDENT_V rec_id);
BOOL add_attr_to_objrec(IDENT_V rec_id, lpIDENT_V objrec_id);
void change_hobj_record(hOBJ hobj, IDENT_V irecnew);
void change_block_record(int bnum, IDENT_V irecnew);
void modify_record_count(IDENT_V irec, short inc);
lpATTR_RECORD lock_record(IDENT_V rec_id);
void unlock_record(void);
void read_record(IDENT_V rec_id, lpATTR_RECORD rec);
void write_record(IDENT_V rec_id, lpATTR_RECORD rec);
IDENT_V get_next_record(IDENT_V rec_id);
IDENT_V add_record(lpATTR_RECORD rec, lpRECORD_ITEM ri);
void del_record(IDENT_V rec_id);
BOOL create_work_record(lpVDIM vd, lpIDENT_V rec_id);
lpRECORD_ITEM alloc_and_load_record(IDENT_V rec_id, lpATTR_RECORD rec);
void load_record(IDENT_V rec_id, lpATTR_RECORD rec, lpRECORD_ITEM ri);
WORD get_record_size(lpATTR_RECORD record);
BOOL record_union(lpATTR_RECORD rec1, lpRECORD_ITEM irec1,
                  lpATTR_RECORD rec2, lpRECORD_ITEM irec2,
                  lpATTR_RECORD rec3, lpRECORD_ITEM irec3);
BOOL irec_inter(IDENT_V rec1, IDENT_V rec2, lpIDENT_V rec3);

BOOL record_inter(lpATTR_RECORD rec1, lpRECORD_ITEM irec1,
                  lpATTR_RECORD rec2, lpRECORD_ITEM irec2,
                  lpATTR_RECORD rec3, lpRECORD_ITEM irec3);
BOOL irec_sub(IDENT_V rec1, IDENT_V rec2, lpIDENT_V rec3);

BOOL irec_subst(IDENT_V rec1, IDENT_V rec2, lpIDENT_V rec3);

BOOL record_sub(lpATTR_RECORD rec1, lpRECORD_ITEM irec1,
                lpATTR_RECORD rec2, lpRECORD_ITEM irec2,
                lpATTR_RECORD rec3, lpRECORD_ITEM irec3);

BOOL delete_attr_in_all_records(lpRECORD_ITEM dri, WORD numattr);

BOOL replace_attr_item_in_all_records(IDENT_V item_id, IDENT_V newid);

void attr_exit(void);
void attr_handler_err(short cod,...);
void test_attr_value_to_oem(ATTR_TYPE type, char *value, ULONG len);


#define MAX_GRM_STC  3
#define MAX_GP_STC   10

typedef int GS_MASK;

#define FL_NULL           0x00000000L
#define FL_PIN            0x00000001L
#define FL_sgFloat         0x00000002L
#define FL_INT            0x00000004L
#define FL_VADR           0x00000008L
#define FL_UNK            0x00000010L
#define FL_STANDARD_OMIT  0x00000020L
#define FL_ENTER_OMIT     0x00000040L
#define FL_END_CMD_OMIT   0x00000080L
#define FL_EOS_OMIT       0x00000100L
#define FL_CMD            0x00000200L
#define FL_KEY            0x00000400L
#define FL_OUT            0x00000800L
#define FL_END_CMD        0x00001000L
#define FL_TEXT           0x00002000L
#define FL_ADDTYPE        0x00004000L
#define FL_IDENT          0x00008000L
#define FL_POZ            0x00010000L
#define FL_CONVENT        0x00020000L
#define FL_ERR            0x00040000L
#define FL_ARRAY          0x00080000L
#define FL_CAM            0x00100000L
#define FL_EMPTY          0x00200000L
#define FL_ADR            0x00400000L
#define FL_VOID           0x00800000L
#define FL_ANYTYPE        0x01000000L
#define FL_TIDENT         0x02000000L
#define FL_UCCONSTANT     0x04000000L
#define FL_LABEL          0x08000000L
#define FL_LABELNM        0x10000000L
#define FL_BLK            0x20000000L
#define FL_GLOBAL         0x40000000L

#define FL_NUM    (FL_INT|FL_sgFloat)
#define FL_CALC   (FL_NUM|FL_VADR|FL_TEXT|FL_ADDTYPE|FL_ANYTYPE)
#define FL_CAMPIN (FL_PIN|FL_CAM)
#define FL_OMIT   (FL_STANDARD_OMIT|FL_ENTER_OMIT|FL_END_CMD_OMIT|FL_EOS_OMIT)
#define FL_ETEXT  (FL_TEXT|FL_EMPTY)
#define FL_NAME   (FL_TEXT|FL_TIDENT)
#define FL_ALTERKEY  (FL_IDENT|FL_CALC|FL_UNK|FL_UCCONSTANT|FL_LABELNM|FL_GLOBAL)

typedef int CODE_MSG;

typedef struct {
  CODE_MSG  code; 
  char      *str; 
} SG_MESSAGE;     
typedef SG_MESSAGE * lpSG_MESSAGE;

//typedef short CODE_PROMPT;
typedef int CODE_PROMPT;

typedef struct {
  CODE_PROMPT code; 
  char        *str; 
} SG_PROMPT; 
typedef SG_PROMPT * lpSG_PROMPT;

extern void * SG_PROMPT_code;
extern char * SG_PROMPT_char;

lpSG_MESSAGE get_message(CODE_MSG code);
lpSG_PROMPT get_prompt(CODE_PROMPT code);


typedef int INDEX_KW; 
typedef INDEX_KW * lpINDEX_KW;
typedef INDEX_KW CMD_CODE;
typedef INDEX_KW KEY_CODE;

#define KW_UNKNOWN MAXINT 

#define  MAX_LINE  C_MAX_STRING_CONSTANT+1


#define    GET_COMMAND  0x01 
#define JOIN_PIN_AND_CHGVP  0x02  
#define       IGNORE_CHGVP  0x04  
#define   GET_COMMAND_DONE  0x08  
#define           ONLY_PIN  0x10  
#define  REZ_IN_CAM_ENABLE  0x20  
#define  ENTER_OMIT_ENABLE  0x40 
#define  FULL_SCREEN_PINPNT 0x80 

#define    OPEN_PAR   TRUE     
#define   CLOSE_PAR  FALSE     

typedef enum {  
  CANCEL_ERR = 0,   
  CANCEL_QUIT,      
  CANCEL_DBG_BEGIN, 
  CANCEL_DBG_SYS,   
} CANCEL_MODE;

#define   COMMA               ','
#define   SPACE               ' ' 
#define   TAB                '\t'
#define   EOS                '\0' 
#define   END_CMD             ';' 

#define   T_CMD              '\'' 
#define   NEW_INPUT           ':' 

#define   MBLK_OPEN           '{' 
#define   MBLK_CLOSE          '}' 

#define DELIM {EOS, SPACE, COMMA, TAB, END_CMD}

#define NUM_REZ_POINT  2
#define NUM_REZ_MATR   1
#define MAX_NUM_ECHO   3
typedef struct {
  short      flag;
  D_POINT  p[NUM_REZ_POINT];      
  MATR     m[NUM_REZ_MATR];       
  short      numetxt;               
  short      numepar;              
  char*    echotxt[MAX_NUM_ECHO]; 
  sgFloat   echopar[MAX_NUM_ECHO]; 
} REZ_GEO;

extern REZ_GEO       rez_geo;

#define VI_NUM_VIT               200  
#define VI_NUM_TBT               201 
#define VI_OVERFLOW_LEVEL_BLOCK  202  

#define DEFAULT_DETAIL_LEVEL  1 
#define MAX_LEVEL_BLOCK 20  

typedef VADR                 hVPORT;
typedef enum {O_OK,
              O_BAD,
              O_NOT_FIND,
              O_EMPTY,
} O_COD;

typedef struct {
  hOBJ        hobjparent; 
  hOBJ        hobjchild;  
  VI_LOCATION viloc;      
  short         num_edge;   
  F_PLPOINT   p1;         
  F_PLPOINT   p2;        
  OBJTYPE     type;       
  GEO_OBJ     geo;        
  BOOL        true_geo;   
  UCHAR       usebmatr;   
  MATR        bmatr;      
}FINDS;
typedef FINDS * lpFINDS;

typedef enum { VIT_END, VIT_OBJ, VIT_PART, VIT_SET, VIT_SETP, VIT_POINT,
               VIT_LINE, VIT_STARTBLOCK, VIT_ENDBLOCK, VIT_JMP,
               VIT_MARK, VIT_COLOR, VIT_THICKNESS, VIT_NUM } VITYPE;

typedef struct {
  VLD         vld;      
  F_PLPOINT   min;      
  F_PLPOINT   max;      
}VI_HEAD;

#define TB_NO_STATUS            0x00
#define TB_NOT_DRAW_IN_CAM_VIEW 0x01
#define TB_NOT_XOR              0x02


#define VP_NOT_DEFINE 0x00
#define VP_REGEN      0x01  
#define VP_REDRAW     0x02  
#define VP_ZOOM       0x04  
#define VP_ON         0x08  
#define VP_STATIC     0x10  

typedef VI_HEAD          * lpVI_HEAD;

#define  MAX_SP       100   
#define  MAX_NOV_TMP  250   
#define  MAX_NOE_TMP  500   

typedef enum { VIS_ERR, VIS_OK, VIS_NULL} VIS_COD;

extern BREPTYPE breptype;



#endif