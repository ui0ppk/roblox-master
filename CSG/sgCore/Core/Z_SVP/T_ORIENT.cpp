#include "../sg.h"

//    hpath    plane
// : 1 -  
//            -1 -  
//					0 -  
//				  -2 -  

short test_orient_path(hOBJ hpath, lpD_PLANE plane)
{
	sgFloat		s;
	MATR			m;
	OSTATUS		status;
	VDIM			vdim;
	lpMNODE		lpmnode;
	int			j;
	D_POINT		bp, ep;

	if (!get_status_path(hpath, &status)) return -2;
	if (!(status & ST_CLOSE)) return 0;
	memset(&bp, 0, sizeof(bp));
	s = 0;
	o_hcunit(m);
	o_hcrot1(m, &plane->v);
	if (!apr_path(&vdim, hpath)) return -2;
	if (!begin_rw(&vdim, 0)) goto err;
	for (j = 0; j < vdim.num_elem; j++) {
		if ( (lpmnode = (lpMNODE)get_next_elem(&vdim)) == NULL) goto err1;
		o_hcncrd(m, &lpmnode->p, &ep);
		if (j) s += (bp.x * ep.y - ep.x * bp.y);
		dpoint_copy(&bp, &ep);
	}
	end_rw(&vdim);
	free_vdim(&vdim);
	return sg(s / 2, eps_n * eps_n);
err1:
	end_rw(&vdim);
err:
	free_vdim(&vdim);
	return -2;
}
