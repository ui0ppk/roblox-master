#include "../sg.h"

void tr_vp_to_acs(lpGEO_ARC a, BOOL flag_arc, lpD_POINT vp,
                  lpD_POINT gp, lpD_POINT ap,
									lpGEO_ARC a2d, MATR g_a, MATR a_g)

        
{
sgFloat da;
short kp;
D_POINT na;
D_POINT pc = {0., 0., 0.};
D_POINT wp[2];

	normv(TRUE, dpoint_sub(gcs_to_vcs(&a->n, &na), gcs_to_vcs(&pc, &pc), &na),&na);

	gcs_to_vcs(&a->vc, &pc);     //   -   

	if(fabs(na.z) >= eps_n){   //     
		//  4-  . 
		da = - na.x*pc.x - na.y*pc.y - na.z*pc.z;
		//    vp   
		dpoint_copy(&pc, vp);
 	  pc.z = - (da + na.x*vp->x + na.y*vp->y)/na.z;

    trans_arc_to_acs(a, flag_arc, a2d, g_a, a_g);
		o_hcncrd(g_a, vcs_to_gcs(&pc, gp), ap);
		return;
	}
//       
	dpoint_copy(&na, vp);
	na.z = pc.z + a->r;
	vcs_to_gcs (&na, &wp[0]);
	na.z = pc.z - a->r;
	vcs_to_gcs (&na, &wp[1]);
	na.z = pc.z;
	vcs_to_gcs (&na, gp);

	trans_arc_to_acs(a, flag_arc, a2d, g_a, a_g);
	o_hcncrd(g_a, &wp[0], &wp[0]);
	o_hcncrd(g_a, &wp[1], &wp[1]);

	eq_line(&wp[0], &wp[1], &na.x, &na.y, &na.z);
	kp = intersect_2d_lc(na.x, na.y, na.z, 0., 0., a->r, wp);
	if(kp)
//    
		if(flag_arc)
			test_in_arrey(wp, &kp, OARC, TRUE, a2d);
	if(kp){ //     -  
		 dpoint_copy(ap, wp);
		 o_hcncrd(a_g, ap, gp);
		 return;
	}
//   -    
	o_hcncrd(g_a, gp, ap);
}

void trans_arc_to_acs(lpGEO_ARC a3d, BOOL flag_arc,
										  lpGEO_ARC a2d, MATR g_a, MATR a_g)
//         
{

	get_c_matr((lpGEO_CIRCLE) a3d, g_a, a_g);
	memcpy(a2d, a3d, ((flag_arc) ? sizeof(GEO_ARC) : sizeof(GEO_CIRCLE)));
	a2d->vc.x =	a2d->vc.y = a2d->vc.z =	0.;
	if(!flag_arc) {
		a2d->ab = 0.;
		a2d->angle = 2 * M_PI;
		a2d->vb.x = a2d->ve.x = a2d->r;
		a2d->vb.y = a2d->vb.z = a2d->ve.y = a2d->ve.z = 0.;
	}
	else{
		o_hcncrd(g_a, &a2d->vb, &a2d->vb);
		o_hcncrd(g_a, &a2d->ve, &a2d->ve);
		a2d->vb.z = a2d->ve.z = 0.;
	}
}

//---      
//       
void get_c_matr(lpGEO_CIRCLE circle, MATR g_c, MATR c_g)
{
	D_POINT p;

	dpoint_inv(&circle->vc, &p);
	o_hcunit(g_c);
	o_tdtran(g_c, &p);
	o_hcrot1(g_c, &circle->n);
	o_minv(g_c, c_g);
}

BOOL tr_arc_pl_to_acs(lpGEO_ARC a1, BOOL flag_arc,
                      lpGEO_ARC a12d, MATR g_a, MATR a_g,
                      lpGEO_ARC a2, lpD_POINT c)
//   (a1)      
//  2D (c), 
//     (a1)  (a2)
//      (a1)
// : FALSE -   ;
//						 TRUE  - ;

{
sgFloat  d;
D_POINT p;

	o_hcunit(g_a);
	o_hcrot1(g_a, &a1->n);
	o_hcncrd(g_a, &a2->n, c);
  trans_arc_to_acs(a1, flag_arc, a12d, g_a, a_g);
	o_hcncrd(g_a, &a2->vc, &p);
	c->z = - c->x*p.x - c->y*p.y - c->z*p.z;
	if((d = hypot(c->x, c->y)) < eps_n)
		return FALSE;
	dpoint_scal(c, 1./d, c);
	return TRUE;
}
