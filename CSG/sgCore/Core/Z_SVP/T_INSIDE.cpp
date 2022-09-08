#include "../sg.h"

//    htest   hout
// O      plane
// ,   ,      !
// :  1 - htest  hout
//             -1 - htest  hout
//					 0 -  

short test_inside_path(hOBJ htest, hOBJ hout, lpD_PLANE plane)
{
	int		j, n, num;
	lpMNODE	lpmnode;
	VDIM		vdim;
	MATR		m;
	D_POINT	p, bp, ep;

	if (!apr_path(&vdim, hout)) return 0;
	o_hcunit(m);
	o_hcrot1(m, &plane->v);
	get_endpoint_on_path(htest, &p, (OSTATUS)0);
	o_hcncrd(m, &p, &p);
	num = 0;
	if (!begin_rw(&vdim, 0)) goto err;
	for (j = 0; j < vdim.num_elem; j++) {
		if ( (lpmnode = (lpMNODE)get_next_elem(&vdim)) == NULL) goto err1;
		o_hcncrd(m, &lpmnode->p, &ep);
		if (j) {
			if((n = test_cont_int(&bp, &ep, &p)) < 0) {
				put_message(INTERNAL_ERROR, NULL, 0);
				goto err1;
			}
			num += n;
		}
		dpoint_copy(&bp, &ep);
	}
	end_rw(&vdim);
	free_vdim(&vdim);
	return (num % 2) ? 1 : -1;	// htest / hout
err1:
	end_rw(&vdim);
err:
	free_vdim(&vdim);
	return 0;
}
