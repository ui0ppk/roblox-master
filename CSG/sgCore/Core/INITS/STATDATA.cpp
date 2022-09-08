#include "../sg.h"
void InitStaticData(void);

void InitStaticData(void)
{
	memset(static_data, 0, sizeof(STATIC_DATA));
	static_data->exp_hp_gab.x = 297;
	static_data->exp_hp_gab.y = 210;
	static_data->WidthThreshold = 100;		
  static_data->RoundOn        = FALSE;
}
