#ifndef __ARR_UTILS_HDR__
#define __ARR_UTILS_HDR__

#define UE_NO_VMEMORY        100  // no virtual memory
#define UE_OPEN_FILE         101  // error file open
#define UE_WRITE_FILE        102  // error write to file
#define UE_READ_FILE         103  // error read file
#define UE_CLOSE_FILE        104  // error close file
#define UE_END_OFF_FILE      105  
#define UE_SEEK_FILE         106  
#define UE_OVERFLOW_VDIM     107  
#define UE_OPEN_ACCES        108
#define UE_OPEN_EINVACC      109
#define UE_OPEN_EMFILE       110
#define UE_OPEN_ENOENT       111

//=== VIRTUAL ADDRESSES ====
typedef VADR                 hD_PLPOINT;
typedef VADR                 hD_PLANE;
typedef VADR                 hREGION_3D;
typedef VADR                 hD_POINT;
typedef VADR                 hLISTH;
typedef VADR                 hLIST;

typedef struct {
  VADR hnext;
  VADR hprev;
}LIST;
typedef LIST   * lpLIST;

typedef struct {
  VADR hhead;
  VADR htail;
  int num;
}LISTH;
typedef LISTH  * lpLISTH;

typedef struct {
  LIST    list;
  WORD    free_mem;
}RAW;
typedef RAW * lpRAW;

typedef struct {
  VADR hpage;
  WORD offset;
}VI_LOCATION;
typedef VI_LOCATION * lpVI_LOCATION;

typedef enum { 
  OBJ_LIST,    
  SEL_LIST,    
  ALL_LIST,    
} NUM_LIST;

#define NUM_VDIM_PAGE 10
// virtual array
typedef struct 
{
  VADR  page[NUM_VDIM_PAGE];    // VIRTUAL PAGES
  VADR  hpage_a;                // extended pointers to virtula pages
  VADR  hpagerw;          // current page  (get_next_elem)
  int   cur_elem;         // number of current page     (get_next_elem)
  char* elem;             // current element            (get_next_elem)
  int   num_elem;         // count of elements
  short num_elem_page;    // count of eletnts on one page
  short size_elem;        // size of one elemetn
}VDIM;
typedef VDIM * lpVDIM;

// virtual linear array (VLD)
typedef struct {
  ULONG       mem;      // array memry
  VI_LOCATION viloc;    // free mmory header
  lpRAW       lprawg;   // for add_vld_data
  LISTH       listh;    // list of pages
}VLD;
typedef VLD * lpVLD;

typedef struct {
  HANDLE handle;      // bufer handle file
  char   *buf;        // storage buffer
  WORD   len_buf;     // buffer length
  WORD   cur_buf;     // current buffer
  UCHAR  flg_modify;  // modification flag
  ULONG  p_beg;       // 
  ULONG  len_file;    
  char   flg_mess_eof;// 1 - message UE_END_OF_FILE
  char   flg_no_read; 
  int cur_line;    
}BUFFER_DAT;
typedef BUFFER_DAT * lpBUFFER_DAT;

// virtual BD
typedef struct{
  VDIM  vdim;     
  VLD   vld;      
  long  ind_free;
  long  free_space;
  ULONG max_len;  
}VIRT_BD;
typedef VIRT_BD * lpVIRT_BD;

typedef long IDENT_V;  
typedef IDENT_V * lpIDENT_V;


typedef struct{     
  IDENT_V    head;  
  IDENT_V    tail;  
  long       num;  
}I_LISTH;
typedef I_LISTH * lpI_LISTH;

typedef struct{     
  IDENT_V    next;  
  IDENT_V    prev;  
}I_LIST;
typedef I_LIST * lpI_LIST;

#define MAX_INLINE_VALUE  10  // max value in vld

// Value state
#define A_NOT_INLINE  (MAX_INLINE_VALUE+1)  // save in vld

typedef struct {          
  ULONG       len;         
  VI_LOCATION loc;         
} NOT_INLINE;
typedef NOT_INLINE * lpNOT_INLINE;

typedef struct{
  UCHAR status;
  union {      
    sgFloat     dv;
    char       cv[MAX_INLINE_VALUE];
    NOT_INLINE ni;
  } value;
} A_VALUE;
typedef A_VALUE  * lpA_VALUE;

typedef struct {
  lpVDIM    vd;       
  lpVLD     vld;
  short     maxline;  
  UCHAR     gmod_flag;
} DTED_TEXT;
typedef DTED_TEXT * lpDTED_TEXT;

typedef struct {
  A_VALUE     v;       
  UCHAR       mark;   
}DTED_STR;

#define  ML_S_C           1      
#define  TED_MAX_TEXT_LEN 65000L 

typedef enum { BUF_NEW, BUF_OLD, BUF_NEWINMEM } BUFTYPE;
BOOL      init_buf(lpBUFFER_DAT bd, char* name, BUFTYPE regim);
void      turn_story_data_progress(BOOL on);
BOOL      story_data(lpBUFFER_DAT bd, ULONG len, void* data);
ULONG     load_data(lpBUFFER_DAT bd, ULONG len, void* data);
WORD      load_str(lpBUFFER_DAT bd, WORD len, char* data);
BOOL      flush_buf(lpBUFFER_DAT bd);
BOOL      seek_buf(lpBUFFER_DAT bd,int offset);
int       get_buf_offset(lpBUFFER_DAT bd);
BOOL      close_buf(lpBUFFER_DAT bd);

// virtual array
BOOL      init_vdim(lpVDIM dim, short size_elem);
BOOL      add_elem(lpVDIM dim,void* buf);
BOOL      write_elem(lpVDIM dim, int num, void* data);
BOOL      read_elem(lpVDIM dim, int num, void* buf);
BOOL      find_elem(lpVDIM dim,int *num_first, BOOL (*func)
          (void* elem, void* user_data), void* user_data);
void*     get_elem(lpVDIM dim, int num);
BOOL      begin_rw(lpVDIM dim, int num);
void*     get_next_elem(lpVDIM dim);
void      end_rw(lpVDIM dim);
void      free_vdim(lpVDIM dim);
void      delete_elem_list(lpVDIM dim,int number);

typedef   int (*VDFCMP)(const void* elem1, const void* elem2);
BOOL      sort_vdim(lpVDIM vdim, VDFCMP fcmp);
int       bsearch_vdim(const void *key,lpVDIM vdim, VDFCMP func);

// virtual linear array
void      init_vld(lpVLD vld);
void      begin_read_vld(lpVI_LOCATION lploc);
void      get_read_loc(lpVI_LOCATION lploc);
void      set_read_loc(lpVI_LOCATION lploc);
void      end_read_vld(void);
void      read_vld_data(ULONG len, void* p);
BOOL      load_vld_data(lpVI_LOCATION loc1, ULONG len, void* p);
void      free_vld_data(lpVLD lpvld);
BOOL      add_vpage(lpVLD lpvld);
BOOL      add_vld_data(lpVLD lpvlp,ULONG len, void* p);
void      lock_vld_data(lpVLD lpvld);
void      unlock_vld_data(lpVLD lpvld);
void      rezet_vld_data(lpVI_LOCATION lploc,ULONG len, void* p);
void      begin_rezet_vld(lpVI_LOCATION lploc);
void      rezet_vld_data_f(ULONG len, void* p);
void      end_rezet_vld(void);

// virtual bd
void init_vbd(lpVIRT_BD vbd);
BOOL free_vbd(lpVIRT_BD vbd);
IDENT_V vbd_add_str(lpVIRT_BD vbd, char* str);
BOOL  vbd_get_str(lpVIRT_BD vbd, IDENT_V ident, char* str);
BOOL vbd_write_data(lpVIRT_BD vbd, IDENT_V ident, void* data);
IDENT_V vbd_add_data(lpVIRT_BD vbd, ULONG len, void* data);
BOOL  vbd_del_data(lpVIRT_BD vbd, IDENT_V ident);
BOOL  vbd_load_data(lpVIRT_BD vbd, IDENT_V ident, void* data);
BOOL  vbd_get_len(lpVIRT_BD vbd, IDENT_V ident, ULONG *len);
BOOL vbd_sborka_musora(lpVIRT_BD vbd);
BOOL add_data_to_avalue(lpVLD vld, lpA_VALUE av, ULONG len, void* value);
BOOL add_not_inline_data(lpVLD vld, lpNOT_INLINE ni, ULONG len, void* data);
BOOL load_data_from_avalue(lpA_VALUE av, ULONG *len, void* value);
BOOL vbd_copy(lpVIRT_BD OldVbd, lpVIRT_BD NewVbd);

// lists fucntions
void  copy_data(VADR dist,VADR source,WORD size);
void  init_listh(lpLISTH listh);
void  attach_item_head(lpLISTH listh,VADR hitem);
void  attach_item_tail(lpLISTH listh,VADR hitem);
void  get_next_item(VADR hitem,VADR * hnext);
void  get_prev_item(VADR hitem,VADR * hprev);
void  detach_item(lpLISTH listh,VADR hitem);
void  free_item_list(lpLISTH listh);
void  free_tail_list(lpLISTH listh,VADR hpage);
void  free_tail_list_cur(lpLISTH listh,VADR hpage);
WORD  free_intermediate_list(lpLISTH listh, VADR hpage_beg, VADR hpage_end);

void  attach_item_head_z(NUM_LIST num, lpLISTH listh, VADR hitem);
void  attach_item_tail_z(NUM_LIST num, lpLISTH listh, VADR hitem);
void  get_next_item_z(NUM_LIST num, VADR hitem, VADR * hnext);
void  get_prev_item_z(NUM_LIST num, VADR hitem, VADR * hprev);
void  detach_item_z(NUM_LIST num, lpLISTH listh, VADR hitem);

void il_init_listh(lpI_LISTH ilisth);
BOOL il_attach_item_tail(lpVDIM vd, lpI_LISTH ilisth, IDENT_V item);
BOOL il_attach_item_head(lpVDIM vd, lpI_LISTH ilisth, IDENT_V item);
BOOL il_attach_item_befo(lpVDIM vd, lpI_LISTH ilisth, IDENT_V bitem,
                         IDENT_V item);
BOOL il_close(lpVDIM vd, lpI_LISTH ilisth);
BOOL il_get_next_item(lpVDIM vd, IDENT_V item, lpIDENT_V next);
BOOL il_get_prev_item(lpVDIM vd, IDENT_V item, lpIDENT_V prev);
BOOL il_detach_item(lpVDIM vd, lpI_LISTH ilisth, IDENT_V item);
BOOL il_add_elem(lpVDIM vd, lpI_LISTH filisth, lpI_LISTH ilisth,
                 void *value, IDENT_V bitem, lpIDENT_V item);
BOOL il_attach_list_befo(lpVDIM vd, lpI_LISTH ilisth, IDENT_V bitem,
                         lpI_LISTH ilisthins);
BOOL il_break(lpVDIM vd, lpI_LISTH ilisth, IDENT_V item);
BOOL GetIndexByID(lpI_LISTH pIListh, lpVDIM pVd, IDENT_V id, long *index);
BOOL GetIDByIndex(lpI_LISTH pIListh, lpVDIM pVd, long index, IDENT_V* id);


BOOL ted_text_init(lpDTED_TEXT dtt, short max_line);
void ted_text_free(lpDTED_TEXT dtt);
void static_text_init(lpDTED_TEXT dtt, short max_line);
void static_text_free(lpDTED_TEXT dtt);
BOOL ted_text_add(lpDTED_TEXT dtt, char *str);
void ted_load_str(void  *ted, IDENT_V id, char *str, WORD *len);
IDENT_V ted_get_next_str(void *ted, IDENT_V id, char *str);
void ted_get_text_size(void *ted, ULONG *numchar, int *numstr);
BOOL create_dtt_by_file(lpBUFFER_DAT bd, lpDTED_TEXT dtt, BOOL dosflag, BOOL *trimflag);
BOOL create_file_by_dtt(lpBUFFER_DAT bd, lpDTED_TEXT dtt, BOOL dosflag);
void ted_exit(void);

extern void (*u_ted_exit)  (void);
//--------------------------------------------------------

void u_handler_err(short cod,...);

extern int  buf_level;


#define C_MAX_STRING_CONSTANT  256  

#define C_MAX_IDENT_NAME    12  


#define RCDERR_OPEN              1
#define RCDERR_BADFILE           2
#define RCDERR_CODEFILE          3
#define RCDERR_NOMEM             4
#define RCDERR_BOOKMARK          5
#define RCDERR_INTERNAL          6
#define RCDERR_REED              7
#define RCDERR_NOCURRPOS         8
#define RCDERR_BADFIELDNUM       9
#define RCDERR_UNKFIELDVALUE     10
#define RCDERR_BADVALUE          11
#define RCDERR_UNKFIELDTYPE      12
#define RCDERR_BADFIELDNAME      13
#define RCDERR_CLOSE             14
#define RCDERR_CALCULATION       15
#define RCDERR_SYNTAX            16
#define RCDERR_BADFIELD          17
#define RCDERR_WRITE             18


typedef struct {
  char  name[11];
  char  type;
  UCHAR size;
  UCHAR prec;
}RCDFIELD;
typedef RCDFIELD * lpRCDFIELD;

typedef int RCDBOOKMARK;
typedef RCDBOOKMARK * lpRCDBOOKMARK;

typedef struct {
  RCDBOOKMARK bmk;
  int offset;
}RCDRECINFO;
typedef RCDRECINFO * lpRCDRECINFO;


typedef struct {
  BUFFER_DAT        bd;        // file
  VDIM              vd;        // Shifts array
  short             fldnum;    // fields count
  lpRCDFIELD        fld;       // fields description
  WORD              recsize;   // record size
  char*             record;    // buffer for current record
  int               boffset;   // begin position
  int               recnum;    // records count
  int               realnum;   // count of non-delete records
  int               irec;      // cur rec
  RCDBOOKMARK       bmk;       // current bookmark
  char*             filter;    // filter
  char              newrec;    // new record
  char              nomatch;   // bad seach
  short             err;        // error code
} RECORDSET;
typedef RECORDSET * lpRECORDSET;


BOOL rcd_create(lpRECORDSET rcd, char* name, lpRCDFIELD fld, short numfld);
BOOL rcd_open(lpRECORDSET rcd, char* name, char *filter);
BOOL rcd_open_by_id(lpRECORDSET rcd, char* name,
                    char* fldnm, sgFloat* id, short numid);
BOOL rcd_close(lpRECORDSET rcd);
RCDBOOKMARK rcd_bookmark(lpRECORDSET rcd);
BOOL rcd_move(lpRECORDSET rcd, int rnum, RCDBOOKMARK bmk);
BOOL rcd_move_first(lpRECORDSET rcd);
BOOL rcd_move_last(lpRECORDSET rcd);
BOOL rcd_move_next(lpRECORDSET rcd);
BOOL rcd_move_previous(lpRECORDSET rcd);
BOOL rcd_BOF(lpRECORDSET rcd);
BOOL rcd_EOF(lpRECORDSET rcd);
BOOL rcd_find_first(lpRECORDSET rcd, char *str);
BOOL rcd_find_last(lpRECORDSET rcd, char *str);
BOOL rcd_find_next(lpRECORDSET rcd, char *str);
BOOL rcd_find_previous(lpRECORDSET rcd, char *str);
BOOL rcd_nomatch(lpRECORDSET rcd);
BOOL rcd_sort(lpRECORDSET rcd, char *str);
BOOL rcd_filter(lpRECORDSET rcd, char *str);
BOOL rcd_getvalue(lpRECORDSET rcd, char *fieldname, void* value);
void * rcd_alloc_and_getvalue(lpRECORDSET rcd, char *fieldname);
BOOL rcd_getvalue_bynum(lpRECORDSET rcd, short fieldnum, void* value);
void * rcd_alloc_and_getvalue_bynum(lpRECORDSET rcd, short fieldnum);
BOOL rcd_getfieldnum(lpRECORDSET rcd, char *fieldname, short* fieldnum);
short rcd_fields_count(lpRECORDSET rcd);
int rcd_records_count(lpRECORDSET rcd);
BOOL rcd_field_info(lpRECORDSET rcd, short fieldnum, lpRCDFIELD fieldinfo);
BOOL rcd_settvalue(lpRECORDSET rcd, char *fieldname, void* value);
BOOL rcd_setvalue_bynum(lpRECORDSET rcd, short fieldnum, void* value);
void rcd_addnew(lpRECORDSET rcd);
BOOL rcd_update(lpRECORDSET rcd);
BOOL rcd_delete(lpRECORDSET rcd);
void rcd_put_message(lpRECORDSET rcd);
void GetCurrentDateForDbf(char* sd);



typedef struct{
  long        lread;
  char        buf[C_MAX_STRING_CONSTANT + 2];
  short       index;
  BOOL        fprogon;
  BUFFER_DAT  bd;
}SB_INFO;
typedef SB_INFO * lpSB_INFO;

BOOL get_sb_sgFloat(lpSB_INFO si, BOOL poz, sgFloat *num);
BOOL get_sb_int(lpSB_INFO si, BOOL poz, short *num);
BOOL get_sb_long(lpSB_INFO si, BOOL poz, int *num);
BOOL get_sb_text(lpSB_INFO si, BOOL empty);
BOOL init_scan_buf(lpSB_INFO si, char* name);
BOOL close_scan_buf(lpSB_INFO si);

#define ctrl_c_press     FALSE

extern REGION bnd;            

extern BOOL (*d_autopan) (short *xn, short *yn, short xold, short yold,
              unsigned char kbs);
extern BOOL (*d_autopan_check) (unsigned char kbs);

void  rez_type(BOOL (*r_user)(short x, short y));



#endif