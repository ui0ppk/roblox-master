#include "../sg.h"

/*
		      (hobj2 == NULL)
		       

		: OSTRUE -     
								OSBREAK -      
								OSFALSE -  - 
*/

static sgFloat exec_alpha(lpD_POINT p, lpD_POINT p1, lpD_POINT p2);
static short intersect_ll_on_line(lpGEO_LINE l1, lpGEO_LINE l2);

OSCAN_COD test_self_cross_path(hOBJ hobj, hOBJ hobj2)
{
	MNODE			node;
	lpMNODE		lpnode;
	short			i, j, kp;
	short			count, count1 = 0, step;
	BOOL			closed;
	OSTATUS		status;
	GEO_LINE	line1, line2;
	D_POINT		mn1, mx1, mn2, mx2;
	VDIM			vdim, vdim2;

	if (!hobj2) {	//    
		if (!get_status_path(hobj, &status)) return OSFALSE;
		closed = (status & ST_CLOSE) ? TRUE : FALSE;
		if (!apr_path(&vdim, hobj)) return OSFALSE;
		count = ((int)(vdim.num_elem - 1) * (vdim.num_elem - 2)) / 2;
		if (count < 100) step = 1;
		else             step = count / 100;
//		num_proc = start_grad(GetIDS(IDS_SG047), count);
		if (!read_elem(&vdim, 0, &node)) {
err:
//			end_grad(num_proc , count);
			free_vdim(&vdim);
			return OSFALSE;
err1:
			end_rw(&vdim);
			goto err;
		}
		line1.v1 = node.p;
		for (i = 1; i < vdim.num_elem - 1; i++) {
			if (ctrl_c_press) {
				put_message(CTRL_C_PRESS, NULL, 0);
        goto err;
			}
			if (!begin_rw(&vdim, i)) goto err;
			if ( (lpnode = (lpMNODE)get_next_elem(&vdim)) == NULL ) goto err1;
			line2.v1 = line1.v2 = lpnode->p;
			dpoint_minmax(&line1.v1, 2, &mn1, &mx1);
			for (j = i + 1; j < vdim.num_elem; j++) {
				count1++;
//				if (!(count1 % step)) step_grad(num_proc , count1);
				if ( (lpnode = (lpMNODE)get_next_elem(&vdim)) == NULL ) goto err1;
				line2.v2 = lpnode->p;
				dpoint_minmax(&line2.v1, 2, &mn2, &mx2);
				el_geo_err = EL_OK;
				if (inter_limits_two_obj(&mn1, &mx1, &mn2, &mx2)) {
					if (!intersect_3d_ll(&line1, TRUE, &line2, TRUE, &node.p, &kp)) {
						put_message(EMPTY_MSG,GetIDS(IDS_SG048),0);
						goto err1;
					}
					if (el_geo_err == EL_EQ_LINE) {	//    
						kp = intersect_ll_on_line(&line1, &line2);
						if (kp == -1) goto brk;
					}
					/*if (kp) {
						if (!(j == i + 1 ||
								 (closed && j == vdim.num_elem - 1))) goto brk;
					}*/
					if (kp) {
						if ((j != i + 1)&&
							!(closed && j == vdim.num_elem - 1 && i==1)) 
							goto brk;
					}
				}
				line2.v1 = line2.v2;
			}
			end_rw(&vdim);
			line1.v1 = line1.v2;
		}
//		end_grad(num_proc , count);
		free_vdim(&vdim);
		return OSTRUE;
brk:
		end_rw(&vdim);
//		end_grad(num_proc , count);
		free_vdim(&vdim);
		return OSBREAK;
	} else {	//   
		if (!apr_path(&vdim, hobj)) return OSFALSE;
		if (!apr_path(&vdim2, hobj2)) {
			free_vdim(&vdim);
			return OSFALSE;
		}
		count = (int)(vdim.num_elem - 1) * (vdim2.num_elem - 1);
		if (count < 100) step = 1;
		else             step = count / 100;
//		num_proc = start_grad(GetIDS(IDS_SG047), count);
		if (!begin_rw(&vdim, 0)) goto err2;
		if ( (lpnode = (lpMNODE)get_next_elem(&vdim)) == NULL ) goto err3;
		line1.v1 = lpnode->p;
		for (i = 1; i < vdim.num_elem; i++) {
			if ( (lpnode = (lpMNODE)get_next_elem(&vdim)) == NULL ) goto err3;
			line1.v2 = lpnode->p;
			dpoint_minmax(&line1.v1, 2, &mn1, &mx1);
			if (!begin_rw(&vdim2, 0)) goto err3;
			if ( (lpnode = (lpMNODE)get_next_elem(&vdim2)) == NULL ) goto err4;
			line2.v1 = lpnode->p;
			for (j = 1; j < vdim2.num_elem; j++) {
				count1++;
//				if (!(count1 % step)) step_grad(num_proc , count1);
				if ( (lpnode = (lpMNODE)get_next_elem(&vdim2)) == NULL ) goto err4;
				line2.v2 = lpnode->p;
				dpoint_minmax(&line2.v1, 2, &mn2, &mx2);
				el_geo_err = EL_OK;
				if (inter_limits_two_obj(&mn1, &mx1, &mn2, &mx2)) {
					if (!intersect_3d_ll(&line1, TRUE, &line2, TRUE, &node.p, &kp)) {
						put_message(EMPTY_MSG,GetIDS(IDS_SG048),0);
						goto err4;
					}
					if (el_geo_err == EL_EQ_LINE) {	//    
						kp = intersect_ll_on_line(&line1, &line2);
					}
					if (kp) goto brk2;
				}
				line2.v1 = line2.v2;
			}
			end_rw(&vdim2);
			line1.v1 = line1.v2;
		}
		end_rw(&vdim);
//		end_grad(num_proc , count);
		free_vdim(&vdim);
		free_vdim(&vdim2);
		return OSTRUE;
err2:
//		end_grad(num_proc , count);
		free_vdim(&vdim);
		free_vdim(&vdim2);
		return OSFALSE;
err4:
		end_rw(&vdim2);
err3:
		end_rw(&vdim);
		goto err2;
brk2:
		end_rw(&vdim2);
		end_rw(&vdim);
//		end_grad(num_proc , count);
		free_vdim(&vdim);
		free_vdim(&vdim2);
		return OSBREAK;
	}
}

//     
//		    !
BOOL inter_limits_two_obj(lpD_POINT mn1, lpD_POINT mx1,
													lpD_POINT mn2, lpD_POINT mx2)
{
	if (mx2->x + eps_d < mn1->x || mn2->x - eps_d > mx1->x) return FALSE;
	if (mx2->y + eps_d < mn1->y || mn2->y - eps_d > mx1->y) return FALSE;
	if (mx2->z + eps_d < mn1->z || mn2->z - eps_d > mx1->z) return FALSE;
	return TRUE;
}

/*
	    p   ,
	   p1, p2
*/
static sgFloat exec_alpha(lpD_POINT p, lpD_POINT p1, lpD_POINT p2)
{
	if (fabs(p2->x - p1->x) > eps_d) return (p->x - p1->x) / (p2->x - p1->x);
	if (fabs(p2->y - p1->y) > eps_d) return (p->y - p1->y) / (p2->y - p1->y);
	return (p->z - p1->z) / (p2->z - p1->z);
}

/*
	   ,    

	: -1      
							 0      
							 1    (  )
*/
static short intersect_ll_on_line(lpGEO_LINE l1, lpGEO_LINE l2)
{
	sgFloat al3, al4;

	if      (dpoint_eq(&l2->v1, &l1->v1, eps_d)) al3 = 0.;
	else if (dpoint_eq(&l2->v1, &l1->v2, eps_d)) al3 = 1.;
	else al3 = exec_alpha(&l2->v1, &l1->v1, &l1->v2);
	if      (dpoint_eq(&l2->v2, &l1->v1, eps_d)) al4 = 0.;
	else if (dpoint_eq(&l2->v2, &l1->v2, eps_d)) al4 = 1.;
	else al4 = exec_alpha(&l2->v2, &l1->v1, &l1->v2);
	if ((al3 > 1. && al4 > 1.) ||
			(al3 < 0. && al4 < 0.)) return 0;
	if ((al3 < 0. && al4 == 0.) ||
			(al4 < 0. && al3 == 0.) ||
			(al3 > 1. && al4 == 1.) ||
			(al4 > 1. && al3 == 1.)) return 1;
	return -1;
}
