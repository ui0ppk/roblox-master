#include "../sg.h"

short test_cont_int(lpD_POINT bp, lpD_POINT ep, lpD_POINT vp){

sgFloat min, max, y;

	min_max_2(bp->x, ep->x, &min, &max);
	if((min == max) || (vp->x < min) || (vp->x >= max)) return 0;
	if((max - min) < eps_d){
		min_max_2(bp->y, ep->y, &min, &max);
		if(vp->y < min) return 1;
		if(vp->y > max) return 0;
		return -1;
	}
	y = (ep->y - bp->y)*((vp->x - bp->x)/(ep->x - bp->x)) + bp->y;
	if(vp->y < y - eps_d) return 1;
	if(vp->y > y + eps_d) return 0;
	return -1;
}
