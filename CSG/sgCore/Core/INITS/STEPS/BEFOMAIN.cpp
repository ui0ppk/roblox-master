#include "../../sg.h"
void InitDbgMem(void);

void alloc_attr_bd(void);     // Model\Src\O_Attrib.c
void ft_alloc_mem(void);      // Model\Src\O_Text.c
//void uc_alloc_mem(void);      // O_Util\U_Calc.c
//void init_camera_array(void); // Vimage\Cam_Util.c
//void tb_init(void);           // Vimage\Vi_Tbuf.c
//void o_startup_vimage(void);  // Vimage\Vi_Util.c
//void InitMenuBD(void);        // Z_MDI\KMenuMem.c
void o_startup_src(void);
void InitStaticData(void);


//        Main
//       
//   #pragma startup        
// 

void InitsBeforeMain(void)
{
  InitDbgMem();
  alloc_attr_bd();     // Model\Src\O_Attrib.c
  ft_alloc_mem();      // Model\Src\O_Text.c
//  uc_alloc_mem();      // O_Util\U_Calc.c
//  gs_alloc_mem();      // Syntax\Gs_Init.c
  //init_camera_array(); // Vimage\Cam_Util.c
//  tb_init();           // Vimage\Vi_Tbuf.c
//  o_startup_vimage();  // Vimage\Vi_Util.c
//  InitMenuBD();        // Z_MDI\KMenuMem.c
//  o_startup_grad();    // O_Util\U_Grad.c

//  static_data
	static_data = (STATIC_DATA*)SGMalloc(sizeof(STATIC_DATA));
  if(!static_data)
    FatalStartupError(1);
  InitStaticData();
  o_startup_src();

  if(pExtStartupInits)
    pExtStartupInits();
}

void o_startup_src(void)
// 
// !!!    WinMain  InitsBeforeMain()
{
	memset(&obj_attrib, 0, sizeof(obj_attrib));
  obj_attrib.color      = (BYTE)curr_color;
	obj_attrib.flg_transp = bool_var[GROUPTRANSP];
	init_listh(&objects);
	init_listh(&selected);
	init_vdim(&vd_blocks, sizeof(IBLOCK));
  init_vdim(&vd_brep,   sizeof(LNP));
	o_hcunit(m_ucs_gcs);
	o_hcunit(m_gcs_ucs);

//  u_attr_exit      = user_attr_exit;

	//pFileProp =  ScenePropCreate();
//Z  ScenePropCreate   ,      --- NB+ 18/II-02

/*	for(i = 0; i < MAXVPORT; i++){
		memset(&vport[i], 0, sizeof(VPORT));
		vport[i].vi.max.x = vport[i].vi.max.y = -GAB_NO_DEF;
		vport[i].vi.min.x = vport[i].vi.min.y =  GAB_NO_DEF;
	}*/
}

void FatalStartupError(int code)
//      Main.    
{
char Buf[256];

  switch(code){
    case 1:
      strcpy(Buf, "Fatal startup error:\r\nUnable to allocate memory\r\n");
    default:
      strcpy(Buf, "");
  }
  strcat(Buf, "Program reset");
//  MessageBox(NULL, Buf, GetSystemName(), MB_OK|MB_ICONERROR);
	exit(code);
}

