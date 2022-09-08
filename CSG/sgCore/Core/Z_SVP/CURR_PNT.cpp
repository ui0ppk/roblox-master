#include "../sg.h"

//     

D_POINT curr_point ={0., 0., 0.}, curr_point_vcs;

void set_curr_point(lpD_POINT p){ //   

	memcpy(&curr_point, p, sizeof(D_POINT));
	gcs_to_vcs(&curr_point, &curr_point_vcs);
}
