#include "../sg.h"

lpD_POINT nearest_to_vp(lpD_POINT vp, lpD_POINT p, short kp, lpD_POINT np)

{
	short nnn = num_nearest_to_vp(vp, p, kp);
	return dpoint_copy(np, &p[nnn]);
}

short num_nearest_to_vp(lpD_POINT vp, lpD_POINT p, short kp)

{
D_POINT pp;
sgFloat  r, r1;
short     i = 0;

  gcs_to_vcs (&p[i], &pp);
	r = dpoint_distance_2d(&pp, vp);

	while(--kp){
    gcs_to_vcs (&p[kp], &pp);
		r1 = dpoint_distance_2d(&pp, vp);
		if(r1 < r){
			r = r1;
			i = kp;
		}
	}
	return i;
}

lpD_POINT nearest_to_vp_2d(lpD_POINT vp, lpD_POINT p, short kp,
                             lpD_POINT np)

{
sgFloat  r;

  kp--;
	if(kp == 1){
		 r = dpoint_distance_2d(&p[0], vp);
		 if(r < dpoint_distance_2d(&p[1], vp))
			 kp = 0;
	}
	return dpoint_copy(np, &p[kp]);
}

sgFloat add_distans_to_vp(lpD_POINT vp, lpD_POINT p, short kp)

{
D_POINT pp;
sgFloat  r = 0.;

	while(kp--){
    gcs_to_vcs (&p[kp], &pp);
		r += dpoint_distance_2d(&pp, &vp[kp]);
	}
	return r;
}
