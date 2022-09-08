#include "../../sg.h"

static OSCAN_COD invert_pre_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);

static OSCAN_COD  (**invert_type_g)(hOBJ obj, lpSCAN_CONTROL lpsc );

BOOL o_invert(hOBJ hobj)
{
	SCAN_CONTROL sc;
  invert_type_g = (OSCAN_COD (**)(hOBJ obj, lpSCAN_CONTROL lpsc))GetMethodArray(OMT_INVERT);
	init_scan(&sc);
  sc.user_pre_scan = invert_pre_scan;
  if ( o_scan(hobj,&sc) == OSFALSE ) return FALSE;
	return TRUE;
}

#pragma argsused
static OSCAN_COD invert_pre_scan(hOBJ hobj,lpSCAN_CONTROL lpsc)
{
	if ( !invert_type_g[lpsc->type]) {
		handler_err(ERR_NUM_OBJECT);
		return OSFALSE;
	}
  return invert_type_g[lpsc->type](hobj,lpsc);
}

#pragma argsused
OSCAN_COD  invert_arc(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	lpOBJ obj;
	obj = (lpOBJ)hobj;
	((lpGEO_ARC)obj->geo_data)->angle = -((lpGEO_ARC)obj->geo_data)->angle;
  return OSTRUE;
}
#pragma argsused
OSCAN_COD  invert_obj(hOBJ hobj, lpSCAN_CONTROL lpsc)
{
	lpOBJ obj;

	obj = (lpOBJ)hobj;
	obj->status ^= ST_ORIENT;
	if ( lpsc->type == OINSERT ) return OSSKIP;
  return OSTRUE;
}

