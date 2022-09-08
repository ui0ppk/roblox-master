#include "../sg.h"

/*         

	: scs -   	(short sx, short sy)
							 vcs -   			(D_POINT vp)
							 gcs -    			(D_POINT gp)
							 ucs -    			(D_POINT up)
							 cpl -          (D_POINT cp)
*/

//---
lpD_POINT scs_to_vcs (short sx, short sy, lpD_POINT vp)
{
/*	f_tfm(&curr_w3->fwind, sx, sy, &vp->x, &vp->y);
  vp->z = (curr_w3->type == CAM_VIEW) ? 0. : curr_point_vcs.z;*/
	return vp;
}

//---
void vcs_to_scs (lpD_POINT vp, short *sx, short *sy)
{
	//f_tmf(&curr_w3->fwind, vp->x, vp->y, sx, sy);
}

//---
lpD_POINT vcs_to_gcs (lpD_POINT vp, lpD_POINT gp)
{
	//MATR view_port_matr = {-1.0, 0.0, 0.0, 0.0, 
	//						0.0, -1.0, 0.0, 0.0, 
	//						0.0, 0.0, 1.0, 0.0,
	//						0.0, 0.0, 0.0, 1.0};
	//o_hcncrd(/*curr_w3->matri*/view_port_matr, vp, gp);
	assert(0);
	return gp;
}

//---
lpD_POINT gcs_to_vcs (lpD_POINT gp, lpD_POINT vp)
{
	//MATR view_port_matr = {-1.0, 0.0, 0.0, 0.0, 
	//						1.0, -1.0, 0.0, 0.0, 
	//						0.0, 0.0, 1.0, 0.0,
	//						0.0, 0.0, 1.0, 1.0};
	//o_hcncrd(/*curr_w3->matrp*/view_port_matr, gp, vp);
	assert(0);
	return vp;
}

//---
lpD_POINT gcs_to_ucs (lpD_POINT gp, lpD_POINT up)
{
	o_hcncrd(m_gcs_ucs, gp, up);
	return up;
}

//---
lpD_POINT ucs_to_gcs (lpD_POINT up, lpD_POINT gp)
{
	o_hcncrd(m_ucs_gcs, up, gp);
	return gp;
}

//---
lpD_POINT scs_to_gcs (short sx, short sy, lpD_POINT gp)
{
	return vcs_to_gcs(scs_to_vcs(sx,sy,gp),gp);
}

//---
void gcs_to_scs (lpD_POINT gp, short *sx, short *sy)
{
	D_POINT vp;

	vcs_to_scs(gcs_to_vcs(gp,&vp),sx,sy);
}

//---
lpD_POINT scs_to_ucs (short sx, short sy, lpD_POINT up)
{
	return vcs_to_ucs(scs_to_vcs(sx,sy,up),up);
}

//---
void ucs_to_scs (lpD_POINT up, short *sx, short *sy)
{
	D_POINT vp;

	vcs_to_scs(ucs_to_vcs(up,&vp),sx,sy);
}

//---
lpD_POINT vcs_to_ucs (lpD_POINT vp, lpD_POINT up)
{
	return gcs_to_ucs(vcs_to_gcs(vp,up),up);
}

//---
lpD_POINT ucs_to_vcs (lpD_POINT up, lpD_POINT vp)
{
	return gcs_to_vcs(ucs_to_gcs(up,vp),vp);
}

//---
void gcs_to_vp (short vp, lpD_POINT gp, short *sx, short *sy)
{
	//D_POINT p;

	//o_hcncrd(vport[vp].wind3d.matrp, gp, &p);
	//f_tmf(&vport[vp].wind3d.fwind, p.x, p.y, sx, sy);
}

//---
static D_POINT cpl_abc;	//     
extern BOOL super_flag;
extern D_POINT super_normal, super_origin;
#pragma argsused
void cpl_init(TYPE_CS type)
{
	D_POINT pp0, pp1;

	if (super_flag) {
		gcs_to_vcs(&super_origin, &pp0);
		dpoint_add(&super_origin, &super_normal, &pp1);
	} else {
		DA_POINT p1 = {0,0,0};
		D_POINT  p0 = {0,0,0};
		int	axis = get_index_invalid_axis();

		p1[axis] = 1;
		gcs_to_vcs(&pp0, &pp0);
	}
	gcs_to_vcs(&pp1, &pp1);
	dpoint_sub(&pp1, &pp0, &cpl_abc);
}

//---    cpl_init()  !
lpD_POINT vcs_to_cpl(lpD_POINT vp, lpD_POINT cp)
{
	sgFloat d;

	d = dskalar_product(&cpl_abc, &curr_point_vcs);
	cp->x = vp->x;
	cp->y = vp->y;
	cp->z = - (cpl_abc.x * vp->x + cpl_abc.y * vp->y - d) / cpl_abc.z;
	return vcs_to_gcs(cp,cp);
}
