#include "../../sg.h"

WORD*       geo_size = NULL;          

OBJ_ATTRIB 	obj_attrib;             
OFILTER     struct_filter;        
                                    
OFILTER     complex_filter;       
                                    
OFILTER     frame_filter;            
//                                
OFILTER     enable_filter;         
lpNPW 			npwg = NULL;   				  
int  				scan_level = 0;  		  
sgFloat      vi_h_tolerance = 0.2; 
                                  
VDIM        vd_blocks;            
VDIM        vd_brep;               
BOOL        scan_flg_np_begin = FALSE; 
BOOL        scan_flg_np_end  = FALSE;  

BOOL        scan_true_vision = FALSE; 

short       num_blk_prev = 0;    
                     
UCHAR 			blink_color = CBLINK;	  

APPL_NUMBER o_appl_number = APPL_SG;	 
int				  o_appl_version = 0; 			 

sgFloat			sgw_unit = 1;      
sgFloat			file_unit = 1;     

// --------  -   ()-- o_npbuf.c
BOOL (*read_user_data_buf) (lpNP_BUF np_buf,lpNPW npw) = NULL;
BOOL (*write_user_data_buf)(lpNP_BUF np_buf,lpNPW npw) = NULL;
WORD (*size_user_data) (lpNPW npw) = NULL;

//    
void (*free_extern_obj_data)(hOBJ hobj) = NULL;
//       
void (*free_extern_obj_blk_data)(hOBJ hobj) = NULL;

// --------  -   ()-- o_npbuf.c
BOOL (*add_user_data_mem)(lpVLD vld,lpNPW npw) 	= NULL;
BOOL (*read_user_data_mem) (lpNPW npw) 					= NULL;
BOOL (*rezet_user_data_mem) (lpNPW npw) 				= NULL;

// --------      --------
BOOL       (*expand_user_data)(lpNPW npw,short nov, short noe, short nof, short noc)
            = NULL;
// ------------------------------------------------------------


