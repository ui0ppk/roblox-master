#include "../sg.h"

void InitSysAttrs(void);

IDENT_V id_Density;  				
IDENT_V id_Material; 					
IDENT_V id_TextureScaleU; 		
IDENT_V id_TextureScaleV; 		
IDENT_V	id_TextureShiftU; 		
IDENT_V id_TextureShiftV; 		
IDENT_V	id_TextureAngle; 			
IDENT_V	id_Smooth; 						
IDENT_V	id_MixColor; 					
IDENT_V	id_UVType; 						
IDENT_V	id_TextureMult; 			
IDENT_V	id_LookNum = 0; 			

IDENT_V	id_ProtoInfo; 				 

IDENT_V id_Mass;						
IDENT_V id_XCenterMass;				
IDENT_V id_YCenterMass;				
IDENT_V id_ZCenterMass;				
IDENT_V id_Layer = 0;					

sgFloat	Default_Density = 1000.;	

IDENT_V	id_Name; 			
IDENT_V	id_TypeID;

static  IDENT_V RegSysAttr(char *Name, char* text, UCHAR Type, UCHAR Size, short Prec, WORD AddStatus,...)
{
	va_list argptr;
	sgFloat  num;
	void*   adr;
	IDENT_V id;

    
	va_start(argptr, AddStatus);
	switch(Type)
	{
    case ATTR_STR:
    case ATTR_TEXT:
		  adr = va_arg(argptr, void*);
		  break;
	case ATTR_sgFloat:
	case ATTR_BOOL:
		  num = va_arg(argptr, sgFloat);
		  adr = (void*)&num;
		  break;
    default:
		  adr = NULL;
	}
    id = create_sys_attr(Name, text, Type, Size, Prec, AddStatus, adr);
	va_end(argptr);
  
    return id;
}



void InitSysAttrs(void)
{

/*  
	ATTR_STR        //  
	ATTR_sgFloat     // 
	ATTR_BOOL       // 
	ATTR_TEXT       // 
*/

/* 
	ATT_SYSTEM      // 
	ATT_RGB         //   GOLORREF
	ATT_PATH        //    
	ATT_ENUM        // 
*/


/*	id_Density = RegSysAttr("$Density", "Density",
									ATTR_sgFloat, 10, 3, ATT_SYSTEM|ATT_ENUM, Default_Density);
*/
	id_Material = RegSysAttr("$1M", "1M",
									ATTR_sgFloat, 10, 3, ATT_SYSTEM|ATT_ENUM, -1.);

	id_TextureScaleU = RegSysAttr("$2M", "2M",
									ATTR_sgFloat, 10, 3, ATT_SYSTEM|ATT_ENUM, 1.);

	id_TextureScaleV = RegSysAttr("$3M", "3M",
									ATTR_sgFloat, 10, 3, ATT_SYSTEM|ATT_ENUM, 1.);

	id_TextureShiftU = RegSysAttr("$4M", "4M",
									ATTR_sgFloat, 10, 3, ATT_SYSTEM|ATT_ENUM, 0.);

	id_TextureShiftV = RegSysAttr("$5M", "5M",
									ATTR_sgFloat, 10, 3, ATT_SYSTEM|ATT_ENUM, 0.);

	id_TextureAngle = RegSysAttr("$6M", "6M",
									ATTR_sgFloat, 10, 3, ATT_SYSTEM|ATT_ENUM, 0.);

	id_Smooth = RegSysAttr("$7M", "7M",
									ATTR_sgFloat, 10, 3, ATT_SYSTEM|ATT_ENUM, 1.);

	id_MixColor = RegSysAttr("$8M", "8M",
									ATTR_sgFloat, 10, 3, ATT_SYSTEM|ATT_ENUM, 3.);

	id_UVType = RegSysAttr("$9M", "9M",
									ATTR_sgFloat, 10, 3, ATT_SYSTEM|ATT_ENUM, 1.);

	id_TextureMult = RegSysAttr("$10M", "10M",
									ATTR_sgFloat, 10, 3, ATT_SYSTEM|ATT_ENUM, 1.);

	/*id_LookNum = RegSysAttr("$LookNum", "LookNum",
									ATTR_sgFloat, 10, 3, ATT_SYSTEM|ATT_ENUM, 0.);

	id_ProtoInfo = RegSysAttr("$ProtoInfo", "ProtoInfo",
									ATTR_TEXT, 30, 255, ATT_SYSTEM, NULL);

	id_Mass = RegSysAttr("$Mass", "Mass",
									ATTR_sgFloat, 10, 3, ATT_SYSTEM, 0.);

	id_XCenterMass = RegSysAttr("$XCMass", "XCMass",
									ATTR_sgFloat, 10, 3, ATT_SYSTEM, 0.);

	id_YCenterMass = RegSysAttr("$YCMass", "YCMass",
									ATTR_sgFloat, 10, 3, ATT_SYSTEM, 0.);

	id_ZCenterMass = RegSysAttr("$ZCMass", "ZCMass",
									ATTR_sgFloat, 10, 3, ATT_SYSTEM, 0.);*/

	id_Name = RegSysAttr("$Name", "Name", ATTR_STR, SG_OBJ_NAME_MAX_LEN, 3, ATT_SYSTEM, 0);
	id_TypeID = RegSysAttr("$TypeID", "TypeID",ATTR_STR, 39, 3, ATT_SYSTEM, 0);
}


