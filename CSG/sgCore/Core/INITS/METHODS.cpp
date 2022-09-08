#include "../sg.h"


METHOD
   OMT_COMMON_FILTERS,          //    

   OMT_ENDPOINT_SNAP,           //   
   OMT_MIDDLE_SNAP,             // SNP_EN  -    
   OMT_PERP_SNAP,               // SNP_NPP -    
   OMT_NEAR_SNAP,
   OMT_TANG_SNAP,
   OMT_QUADRANT_SNAP,
   OMT_CENTER_SNAP,


   OMT_FREE_GEO,                //    
   OMT_OBJTYPE_CAPTION,         //    
   OMT_OBJ_DESCRIPT,            //   
   OMT_SCAN,                    //  
   OMT_VIOBRNP,                 //   
   OMT_VIREGEN,                 //  
   OMT_INVERT,                  //     
   OMT_LIMITS,                  //  
   OMT_TRANS,                   //   
   OMT_VIFIND,                  //     
   OMT_TRANSTONURBS,            //   NURBS
   OMT_SPLINTER,                //   
   OMT_APR_PATH,                //  ( apr_path)
   OMT_APR_RIB,                 //  ( apr_rib)
   OMT_APR_SMOT,                //  ( apr_smot)
   OMT_PIPE_R,                  //    pipe_r
   OMT_SAVE,                    //  
   OMT_LOAD,                    //  
   OMT_LOAD_DOS,                // 
   OMT_ZB_SCAN,                 //    
   OMT_DXFOUT,                  //   DXF
   OMT_VRML,                    //   VRML
   OMT_GETLCS,                  //       
   OMT_LAST
   ;

void RegCommonMethods(void)
{

  OMT_COMMON_FILTERS  = RegCommonMethod(0);

  OMT_ENDPOINT_SNAP   = RegCommonMethod(0);
  OMT_MIDDLE_SNAP     = RegCommonMethod(0);
  OMT_PERP_SNAP       = RegCommonMethod(0);
  OMT_NEAR_SNAP       = RegCommonMethod(0);
  OMT_TANG_SNAP       = RegCommonMethod(0);
  OMT_QUADRANT_SNAP   = RegCommonMethod(0);
  OMT_CENTER_SNAP     = RegCommonMethod(0);

  OMT_FREE_GEO        = RegCommonMethod((SG_PTR_TO_DIGIT)free_geo_simple_obj);
  OMT_OBJTYPE_CAPTION = RegCommonMethod(UNKNOWN_OBJ_MSG);
  OMT_OBJ_DESCRIPT    = RegCommonMethod((SG_PTR_TO_DIGIT)IdleObjDecriptStr);
  OMT_SCAN            = RegCommonMethod((SG_PTR_TO_DIGIT)scan_simple_obj);
 // OMT_VIOBRNP         = RegCommonMethod((SG_PTR_TO_DIGIT)creat_vi_idle);
//  OMT_VIREGEN         = RegCommonMethod((SG_PTR_TO_DIGIT)creat_vi_idle);  //      VIOBRNP
  OMT_INVERT          = RegCommonMethod((SG_PTR_TO_DIGIT)IdleHOBJScan);
  OMT_LIMITS          = RegCommonMethod((SG_PTR_TO_DIGIT)IdleGEOScan);
  OMT_TRANS           = RegCommonMethod(0L);
//  OMT_VIFIND          = RegCommonMethod((SG_PTR_TO_DIGIT)simple_true);
  OMT_TRANSTONURBS    = RegCommonMethod((SG_PTR_TO_DIGIT)IdleLPOBJScan);
  OMT_SPLINTER        = RegCommonMethod((SG_PTR_TO_DIGIT)IdleHOBJScan);
  OMT_APR_PATH        = RegCommonMethod((SG_PTR_TO_DIGIT)IdleLPOBJScan);
  OMT_APR_RIB         = RegCommonMethod((SG_PTR_TO_DIGIT)IdleLPOBJScan);
  OMT_APR_SMOT        = RegCommonMethod((SG_PTR_TO_DIGIT)IdleLPOBJScan);
  OMT_PIPE_R          = RegCommonMethod((SG_PTR_TO_DIGIT)IdleLPOBJScan);
  OMT_SAVE            = RegCommonMethod((SG_PTR_TO_DIGIT)simple_save);
  OMT_LOAD            = RegCommonMethod(0L);
  OMT_LOAD_DOS        = RegCommonMethod(0L);
//  OMT_ZB_SCAN         = RegCommonMethod((SG_PTR_TO_DIGIT)zb_idle);
  OMT_DXFOUT          = RegCommonMethod((SG_PTR_TO_DIGIT)IdleGEOScan);
  OMT_VRML            = RegCommonMethod((SG_PTR_TO_DIGIT)IdleHOBJScan);
  OMT_GETLCS          = RegCommonMethod((SG_PTR_TO_DIGIT)lcs_idle);
}


void InitCoreObjMetods(void)
{
SG_PTR_TO_DIGIT* pMArray;

// OMT_COMMON_FILTERS

  pMArray = GetMethodArray(OMT_COMMON_FILTERS);
  pMArray[OLINE]   =  FF_FRAME_ITEM;
  pMArray[OARC]    =  FF_FRAME_ITEM;
  pMArray[OSPLINE] =  FF_FRAME_ITEM;
  pMArray[OCIRCLE] =  FF_FRAME_ITEM;
  pMArray[OPATH]   =  FF_IS_STRUCT|FF_HAS_MATRIX;
  pMArray[OINSERT] =  FF_IS_STRUCT|FF_HAS_MATRIX;
  pMArray[OGROUP]  =  FF_IS_STRUCT|FF_HAS_MATRIX;
  pMArray[OBREP]   =  FF_HAS_MATRIX;
  pMArray[OTEXT]   =  FF_HAS_MATRIX;

// OMT_ENDPOINT_SNAP
//	{K_ENDPOINT, EPOINT|ELINE|EBREP|EARC|ESPLINE, EPOINT},
  pMArray = GetMethodArray(OMT_ENDPOINT_SNAP);
  pMArray[OPOINT]   = SNP_EN|SNP_NPP;
	pMArray[OLINE]    = SNP_EN;
	pMArray[OARC]     = SNP_EN;
	pMArray[OSPLINE]  = SNP_EN;
	pMArray[OBREP]    = SNP_EN;

// OMT_MIDDLE_SNAP
//	{K_MIDDLE,   EPOINT|ELINE|EBREP|EARC,         EPOINT|ELINE|EARC},
  pMArray = GetMethodArray(OMT_MIDDLE_SNAP);
  pMArray[OPOINT]   = SNP_EN|SNP_NPP;
	pMArray[OLINE]    = SNP_EN|SNP_NPP;
	pMArray[OARC]     = SNP_EN|SNP_NPP;
	pMArray[OBREP]    = SNP_EN;

// OMT_PERP_SNAP
//	{K_PERP,     EPOINT|ELINE|EBREP|EARC|ECIRCLE, EPOINT|ELINE|ECIRCLE|EARC},
  pMArray = GetMethodArray(OMT_PERP_SNAP);
  pMArray[OPOINT]   = SNP_EN|SNP_NPP;
	pMArray[OLINE]    = SNP_EN|SNP_NPP;
	pMArray[OARC]     = SNP_EN|SNP_NPP;
	pMArray[OCIRCLE]  = SNP_EN|SNP_NPP;
	pMArray[OBREP]    = SNP_EN;

// OMT_NEAR_SNAP
//	{K_NEAR,     EPOINT|ELINE|EBREP|EARC|ECIRCLE, EPOINT},
  pMArray = GetMethodArray(OMT_NEAR_SNAP);
  pMArray[OPOINT]   = SNP_EN|SNP_NPP;
	pMArray[OLINE]    = SNP_EN;
	pMArray[OARC]     = SNP_EN;
	pMArray[OCIRCLE]  = SNP_EN;
	pMArray[OBREP]    = SNP_EN;

// OMT_TANG_SNAP
//	{K_TANG,     EARC|ECIRCLE,                    ENULL},
  pMArray = GetMethodArray(OMT_TANG_SNAP);
	pMArray[OARC]     = SNP_EN;
	pMArray[OCIRCLE]  = SNP_EN;

// OMT_QUADRANT_SNAP
//	{K_QUADRANT, EARC|ECIRCLE,                    ENULL},
  pMArray = GetMethodArray(OMT_QUADRANT_SNAP);
	pMArray[OARC]     = SNP_EN;
	pMArray[OCIRCLE]  = SNP_EN;

// OMT_CENTER_SNAP
//	{K_CCENTER,  EARC|ECIRCLE|EBREP|ELINE,       EARC|ECIRCLE|EBREP};
  pMArray = GetMethodArray(OMT_CENTER_SNAP);
	pMArray[OLINE]    = SNP_EN;
	pMArray[OARC]     = SNP_EN|SNP_NPP;
	pMArray[OCIRCLE]  = SNP_EN|SNP_NPP;
	pMArray[OBREP]    = SNP_EN|SNP_NPP;
	pMArray[OPATH]    = SNP_EN|SNP_NPP;



//OMT_FREE_GEO            model\o_free.c
  pMArray = GetMethodArray(OMT_FREE_GEO);
  pMArray[OTEXT]   = (SG_PTR_TO_DIGIT)free_geo_text;
	pMArray[ODIM]    = (SG_PTR_TO_DIGIT)free_geo_dim;
	pMArray[OBREP]   = (SG_PTR_TO_DIGIT)free_geo_brep;
	pMArray[OSPLINE] = (SG_PTR_TO_DIGIT)free_geo_spline;
	pMArray[OFRAME]  = (SG_PTR_TO_DIGIT)free_geo_frame;

// OMT_OBJTYPE_CAPTION
  pMArray = GetMethodArray(OMT_OBJTYPE_CAPTION);
  pMArray[OTEXT]   = OTEXT_MSG;
	pMArray[ODIM]    = ODIM_MSG;
	pMArray[OPOINT]  = OPOINT_MSG;
	pMArray[OLINE]   = OLINE_MSG;
	pMArray[OCIRCLE] = OCIRCLE_MSG;
	pMArray[OARC]    = OARC_MSG;
	pMArray[OPATH]   = OPATH_MSG;
	pMArray[OGROUP]  = OGROUP_MSG;
	pMArray[OBREP]   = BREP_MSG;
	pMArray[OINSERT] = OINSERT_MSG;
	pMArray[OSPLINE] = OSPLINE_MSG;
	pMArray[OFRAME]  = OFRAME_MSG;

// OMT_OBJ_DESCRIPT         cmd\objt_str.c
  pMArray = GetMethodArray(OMT_OBJ_DESCRIPT);
  pMArray[OBREP]   = (SG_PTR_TO_DIGIT)ot_brep_str;
	pMArray[OINSERT] = (SG_PTR_TO_DIGIT)ot_insert_str;
	pMArray[OPATH]   = (SG_PTR_TO_DIGIT)ot_path_str;
	pMArray[OSPLINE] = (SG_PTR_TO_DIGIT)ot_path_str;

// OMT_SCAN                 model\o_scan.c
  pMArray = GetMethodArray(OMT_SCAN);
	pMArray[OPATH]   = (SG_PTR_TO_DIGIT)scan_path;
	pMArray[OBREP]   = (SG_PTR_TO_DIGIT)scan_brep;
	pMArray[OGROUP]  = (SG_PTR_TO_DIGIT)scan_bgroup;
	pMArray[OINSERT] = (SG_PTR_TO_DIGIT)scan_insert;
	pMArray[OFRAME]  = (SG_PTR_TO_DIGIT)scan_frame;


// OMT_VIOBRNP             vimage\vi_obrnp.c
  pMArray = GetMethodArray(OMT_VIOBRNP);
//	pMArray[OTEXT]      = (DWORD)creat_vi_text;
//	pMArray[ODIM]       = (DWORD)creat_vi_dim;
//	pMArray[OPOINT]			= (DWORD)creat_vi_point;
//	pMArray[OLINE]			=	(DWORD)creat_vi_line;
//	pMArray[OCIRCLE]		=	(DWORD)creat_vi_circle;
//	pMArray[OARC]				=	(DWORD)creat_vi_arc;
//	pMArray[OBREP]			=	(DWORD)creat_vi_brep;
//	pMArray[OSPLINE]		=	(DWORD)creat_vi_spline;
//	pMArray[OFRAME]			=	(DWORD)creat_vi_frame;

// OMT_VIREGEN            vimage\vi_regen.c
  //pMArray = GetMethodArray(OMT_VIREGEN); //RA
//	pMArray[OTEXT]      = (SG_PTR_TO_DIGIT)creat_vi_text;
//	pMArray[ODIM]       = (SG_PTR_TO_DIGIT)creat_vi_dim;
	//pMArray[OPOINT]			= (SG_PTR_TO_DIGIT)creat_vireg_point;//RA
	//pMArray[OLINE]			=	(SG_PTR_TO_DIGIT)creat_vireg_line;//RA
//	pMArray[OCIRCLE]		=	(SG_PTR_TO_DIGIT)creat_vi_circle;
//	pMArray[OARC]				=	(SG_PTR_TO_DIGIT)creat_vi_arc;
//	pMArray[OBREP]			=	(SG_PTR_TO_DIGIT)creat_vi_brep;
//	pMArray[OSPLINE]		=	(SG_PTR_TO_DIGIT)creat_vi_spline;
//	pMArray[OFRAME]			=	(SG_PTR_TO_DIGIT)creat_vi_frame;

// OMT_INVERT           model\o_invert.c
  pMArray = GetMethodArray(OMT_INVERT);
	pMArray[OARC]		= (SG_PTR_TO_DIGIT)invert_arc;
	pMArray[OBREP]	= (SG_PTR_TO_DIGIT)invert_obj;
	pMArray[OINSERT]= (SG_PTR_TO_DIGIT)invert_obj;
	pMArray[OFRAME]	= (SG_PTR_TO_DIGIT)invert_obj;

// OMT_LIMITS           model\o_limits.c
  pMArray = GetMethodArray(OMT_LIMITS);
  pMArray[OTEXT]  = (SG_PTR_TO_DIGIT)get_text_limits;
	pMArray[ODIM]   = (SG_PTR_TO_DIGIT)get_dim_limits;
	pMArray[OPOINT] = (SG_PTR_TO_DIGIT)get_point_limits;
	pMArray[OLINE]  = (SG_PTR_TO_DIGIT)get_line_limits;
	pMArray[OCIRCLE]= (SG_PTR_TO_DIGIT)get_circle_limits;
	pMArray[OARC]		= (SG_PTR_TO_DIGIT)get_arc_limits;
	pMArray[OBREP]	= (SG_PTR_TO_DIGIT)get_brep_limits;
	pMArray[OSPLINE]= (SG_PTR_TO_DIGIT)get_spline_limits;
	pMArray[OFRAME]	= (SG_PTR_TO_DIGIT)get_frame_limits;

// OMT_TRANS           model\o_trans.c
  pMArray = GetMethodArray(OMT_TRANS);
	pMArray[OTEXT]    = (SG_PTR_TO_DIGIT)trans_complex;
	pMArray[ODIM]     = (SG_PTR_TO_DIGIT)trans_dim;
	pMArray[OPOINT]	  = (SG_PTR_TO_DIGIT)trans_point;
  pMArray[OLINE]		= (SG_PTR_TO_DIGIT)trans_line;
	pMArray[OCIRCLE]	= (SG_PTR_TO_DIGIT)trans_circle;
  pMArray[OARC]		  = (SG_PTR_TO_DIGIT)trans_arc;
	pMArray[OPATH]		= (SG_PTR_TO_DIGIT)trans_complex;
  pMArray[OBREP]		= (SG_PTR_TO_DIGIT)trans_complex;
  pMArray[OGROUP]	  = (SG_PTR_TO_DIGIT)trans_complex;
	pMArray[OINSERT]	= (SG_PTR_TO_DIGIT)trans_complex;
  pMArray[OSPLINE]	= (SG_PTR_TO_DIGIT)trans_spline;
  pMArray[OFRAME]	  = (SG_PTR_TO_DIGIT)trans_complex;

// OMT_VIFIND       vimage\vi_find.c
  //pMArray = GetMethodArray(OMT_VIFIND);//RA
//	pMArray[OSPLINE]	= (DWORD)spline_true;
//	pMArray[OBREP]		= (DWORD)brep_true;
//	pMArray[OFRAME]		= (DWORD)frame_true;

// OMT_SAVE         model\o_save.c
  pMArray = GetMethodArray(OMT_SAVE);
	pMArray[OTEXT]    = (SG_PTR_TO_DIGIT)text_save;
	pMArray[ODIM]     = (SG_PTR_TO_DIGIT)dim_save;
	pMArray[OPOINT]		= (SG_PTR_TO_DIGIT)simple_save;
	pMArray[OLINE]		= (SG_PTR_TO_DIGIT)simple_save;
	pMArray[OCIRCLE]	= (SG_PTR_TO_DIGIT)simple_save;
	pMArray[OARC]			= (SG_PTR_TO_DIGIT)simple_save;
	pMArray[OPATH]		= (SG_PTR_TO_DIGIT)path_save;
	pMArray[OBREP]		= (SG_PTR_TO_DIGIT)brep_save;
	pMArray[OGROUP]		= (SG_PTR_TO_DIGIT)bgroup_save;
	pMArray[OINSERT]	= (SG_PTR_TO_DIGIT)insert_save;
	pMArray[OSPLINE]	= (SG_PTR_TO_DIGIT)spline_save;
	pMArray[OFRAME]		= (SG_PTR_TO_DIGIT)frame_save;


// OMT_LOAD         model\o_load.c
  pMArray = GetMethodArray(OMT_LOAD);
	pMArray[OTEXT]    = (SG_PTR_TO_DIGIT)text_load;
	pMArray[ODIM]     = (SG_PTR_TO_DIGIT)dim_load;
	pMArray[OPOINT]		= (SG_PTR_TO_DIGIT)simple_load;
	pMArray[OLINE]		= (SG_PTR_TO_DIGIT)simple_load;
	pMArray[OCIRCLE]	= (SG_PTR_TO_DIGIT)simple_load;
	pMArray[OARC]			= (SG_PTR_TO_DIGIT)simple_load;
	pMArray[OPATH]		= (SG_PTR_TO_DIGIT)path_load;
	pMArray[OBREP]		= (SG_PTR_TO_DIGIT)brep_load;
	pMArray[OGROUP]		= (SG_PTR_TO_DIGIT)group_load;
	pMArray[OINSERT]	= (SG_PTR_TO_DIGIT)insert_load;
	pMArray[OSPLINE]	= (SG_PTR_TO_DIGIT)spline_load;
	pMArray[OFRAME]		= (SG_PTR_TO_DIGIT)frame_load;


// OMT_LOAD_DOS       model\o_load_d.c
  pMArray = GetMethodArray(OMT_LOAD_DOS);
	pMArray[OTEXT]    = (SG_PTR_TO_DIGIT)dos_text_load;
	pMArray[OPOINT]		= (SG_PTR_TO_DIGIT)dos_simple_load;
	pMArray[OLINE]		= (SG_PTR_TO_DIGIT)dos_simple_load;
	pMArray[OCIRCLE]	= (SG_PTR_TO_DIGIT)dos_simple_load;
	pMArray[OARC]			= (SG_PTR_TO_DIGIT)dos_simple_load;
	pMArray[OPATH]		= (SG_PTR_TO_DIGIT)dos_path_load;
	pMArray[OBREP]		= (SG_PTR_TO_DIGIT)dos_brep_load;
	pMArray[OGROUP]		= (SG_PTR_TO_DIGIT)dos_group_load;
	pMArray[OINSERT]	= (SG_PTR_TO_DIGIT)dos_insert_load;
	pMArray[OSPLINE]	= (SG_PTR_TO_DIGIT)dos_spline_load;
	pMArray[OFRAME]		= (SG_PTR_TO_DIGIT)dos_frame_load;

// OMT_ZB_SCAN       aplicat\zb_scan.c
/*  pMArray = GetMethodArray(OMT_ZB_SCAN);
  pMArray[OTEXT]      = (SG_PTR_TO_DIGIT)zb_text;
	pMArray[ODIM]       = (SG_PTR_TO_DIGIT)zb_dim;
	pMArray[OLINE]			=	(SG_PTR_TO_DIGIT)zb_line;
	pMArray[OCIRCLE]		=	(SG_PTR_TO_DIGIT)zb_circle;
	pMArray[OARC]				=	(SG_PTR_TO_DIGIT)zb_arc;
	pMArray[OBREP]			=	(SG_PTR_TO_DIGIT)zb_brep;
	pMArray[OSPLINE]		=	(SG_PTR_TO_DIGIT)zb_spline;
	pMArray[OFRAME]			=	(SG_PTR_TO_DIGIT)zb_frame;*/  //RA

// OMT_DXFOUT       dxf\exp_dxf.c
  pMArray = GetMethodArray(OMT_DXFOUT);
	pMArray[OPOINT]	 = (SG_PTR_TO_DIGIT)dxfout_point;
	pMArray[OLINE]	 = (SG_PTR_TO_DIGIT)dxfout_line;
	pMArray[OCIRCLE] = (SG_PTR_TO_DIGIT)dxfout_circle;
	pMArray[OARC]		 = (SG_PTR_TO_DIGIT)dxfout_arc;
	pMArray[OSPLINE] = (SG_PTR_TO_DIGIT)dxfout_spline;
	pMArray[OFRAME]	 = (SG_PTR_TO_DIGIT)dxfout_frame;
	pMArray[OTEXT]	 = (SG_PTR_TO_DIGIT)dxfout_text;

// OMT_VRML      vrmlout\exp_vrml.c
  pMArray = GetMethodArray(OMT_VRML);
//	pMArray[OBREP]	= (SG_PTR_TO_DIGIT)vrmlout_brep;

// OMT_GETLCS    model\o_lcs.c
  pMArray = GetMethodArray(OMT_GETLCS);
	pMArray[OTEXT]    = (SG_PTR_TO_DIGIT)lcs_complex;
	pMArray[ODIM]     = (SG_PTR_TO_DIGIT)lcs_dim;
	pMArray[OPOINT]	  = (SG_PTR_TO_DIGIT)lcs_point;
  pMArray[OLINE]		= (SG_PTR_TO_DIGIT)lcs_line;
	pMArray[OCIRCLE]	= (SG_PTR_TO_DIGIT)lcs_circle_and_arc;
  pMArray[OARC]		  = (SG_PTR_TO_DIGIT)lcs_circle_and_arc;
	pMArray[OPATH]		= (SG_PTR_TO_DIGIT)lcs_spline_and_path;
  pMArray[OBREP]		= (SG_PTR_TO_DIGIT)lcs_complex;
  pMArray[OGROUP]	  = (SG_PTR_TO_DIGIT)lcs_complex;
	pMArray[OINSERT]	= (SG_PTR_TO_DIGIT)lcs_complex;
  pMArray[OSPLINE]	= (SG_PTR_TO_DIGIT)lcs_spline_and_path;
  pMArray[OFRAME]	  = (SG_PTR_TO_DIGIT)lcs_complex;


// OMT_TRANSTONURBS     nurbs\nrb_arc.c
// OMT_SPLINTER         nurbs\spl_inter.c
// OMT_APR_PATH         s_design\apr_path.c
// OMT_APR_RIB,         s_design\apr_rib.c
// OMT_APR_SMOT         s_design\apr_smot.c
// OMT_PIPE_R           s_design\pipe_r.c

}

#pragma argsused
OSCAN_COD  IdleHOBJScan(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
  return OSTRUE;
}
#pragma argsused
OSCAN_COD IdleGEOScan(void* geo, lpSCAN_CONTROL lpsc)
{
	return OSTRUE;
}

#pragma argsused
OSCAN_COD IdleLPOBJScan(lpOBJ obj, lpSCAN_CONTROL lpsc)
{
	return OSTRUE;
}


