#include "../sg.h"

//      
lpSTATIC_DATA static_data = NULL;	//    

short				sg_m_num_meridians = 24;	// -  

LISTH     objects;          //    .
LISTH     selected;         //    

// ,    
short curr_color   = IDXBLUE;		                        //    
UCHAR vport_bg     = IDXLIGHTGRAY;                      //   

UCHAR screen_color = IDXBLACK;                          //    

//===  
D_POINT   scene_min = { 100, 100, 100}, // .
					scene_max = {-100,-100,-100}; //  ..
UCHAR			message_level = 3;	          //   

MATR			m_ucs_gcs, m_gcs_ucs;	        //  

BOOL super_flag = FALSE;
D_POINT super_normal, super_origin;

