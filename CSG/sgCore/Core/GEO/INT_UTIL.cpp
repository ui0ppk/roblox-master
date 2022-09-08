#include "../sg.h"

BOOL intersect_3d_po(lpD_POINT po,
                     OBJTYPE type, void  *geo, BOOL flag_el,
										 lpD_POINT p, short *kp);

BOOL intersect_two_obj(OBJTYPE type1, void  *geo1, BOOL flag_1,
                       OBJTYPE type2, void  *geo2, BOOL flag_2,
										   lpD_POINT p, short *kp)

   
{

BOOL ret;

	if(type1 == OPOINT)
      return intersect_3d_po((lpD_POINT)geo1, type2, geo2, flag_2, p, kp);
	if(type2 == OPOINT)
      return intersect_3d_po((lpD_POINT)geo2, type1, geo1, flag_1, p, kp);

	switch (type1) {
		case OLINE:
			switch (type2) {
				case OLINE:
					ret = intersect_3d_ll((lpGEO_LINE)geo1, flag_1,
					                      (lpGEO_LINE)geo2, flag_2,
																p, kp);
					break;
				case OCIRCLE:
					flag_2 = FALSE;
				case OARC:
					ret = intersect_3d_la((lpGEO_LINE)geo1, flag_1,
					                      (lpGEO_ARC)geo2,  flag_2,
																p, kp);
					break;
			}
			break;
		case OCIRCLE:
			flag_1 = FALSE;
		case OARC:
			switch (type2) {
				case OLINE:
					ret = intersect_3d_la((lpGEO_LINE)geo2, flag_2,
					                      (lpGEO_ARC)geo1, flag_1,
																p, kp);
					break;
				case OCIRCLE:
					flag_2 = FALSE;
				case OARC:
          ret = intersect_3d_aa((lpGEO_ARC) geo1, flag_1,
					                      (lpGEO_ARC) geo2, flag_2,
		  							            p, kp);
					break;
			}
			break;
	}
	return ret;
}

BOOL intersect_3d_po(lpD_POINT po,
                     OBJTYPE type, void  *geo, BOOL flag_el,
										 lpD_POINT p, short *kp)

{
	if(!test_in_pe_3d(FALSE, po, type, geo, flag_el))
		*kp = 0;
	else {
		dpoint_copy(p, po);
		*kp = 1;
	}
	return TRUE;
}

BOOL intersect_3d_ll(lpGEO_LINE l1, BOOL flag_line1,
                     lpGEO_LINE l2, BOOL flag_line2,
										 lpD_POINT p, short *kp)

{
short       npl;
sgFloat    d;
D_POINT   n, pp;
lpD_POINT p1, p2;
lpGEO_LINE  pl1 = (lpGEO_LINE)&el_geo1;
lpGEO_LINE  pl2 = (lpGEO_LINE)&el_geo2;

	set_el_geo();
	*kp = 0;
	if(!normv(TRUE, dpoint_sub(&l1->v2, &l1->v1, &n), &n)){
		el_geo_err = EL_LINE_NULL_LEN;  //   
		return FALSE;
	}

//        
	if(dist_p_l(&l1->v1, &n, &l2->v1, &pp) <
		 dist_p_l(&l1->v1, &n, &l2->v2, &pp)){
		 p1 = &l2->v2;
		 p2 = &l2->v1;
	}
	else{
		 p1 = &l2->v1;
		 p2 = &l2->v2;
	}
//  ,    
	if(!eq_plane(&l1->v1, &l1->v2, p1, &n, &d)){
	  if(!test_wrd_line(l2))
			return FALSE;
		el_geo_err = EL_EQ_LINE;  //     
		return TRUE;
	}

	if(fabs(dist_p_pl(p2, &n, d)) >= eps_d)
		goto met_err;   // 

	npl = best_crd_pln(&n);
	dpoint_project(&l1->v1, &pl1->v1, npl);
	dpoint_project(&l1->v2, &pl1->v2, npl);
	dpoint_project(&l2->v1, &pl2->v1, npl);
	dpoint_project(&l2->v2, &pl2->v2, npl);

  *kp = intersect_2d_ll(pl1, pl2, &pp);
	if(*kp){
	  switch(npl){
		  case 1:          // X
			  d = (-n.y*(pp.x) - n.z*(pp.y) - d)/n.x;
			  break;
		  case 2:        // Y
			  d = (-n.x*(pp.x) - n.z*(pp.y) - d)/n.y;
			  break;
		  case 3:        // Z
			  d = (-n.x*(pp.x) - n.y*(pp.y) - d)/n.z;
			break;
		}
	}
	if(flag_line1)
		if(!test_in_pe_2d(&pp, OLINE, TRUE, pl1))
			 *kp = 0;
	if(*kp && flag_line2)
		if(!test_in_pe_2d(&pp, OLINE, TRUE, pl2))
			 *kp = 0;
  if(*kp)
		dpoint_reconstruct(&pp, p, npl, d);
	return TRUE;
met_err:
	el_geo_err = EL_NO_INTER_POINT;
	return TRUE;
}

short intersect_2d_ll(lpGEO_LINE l1, lpGEO_LINE l2, lpD_POINT p)

{
sgFloat a1, b1, c1, a2, b2, c2;

  if(!eq_line(&l1->v1, &l1->v2, &a1, &b1, &c1) ||
     !eq_line(&l2->v1, &l2->v2, &a2, &b2, &c2))
		return 0;

  if(p_int_lines(a1, b1, c1, a2, b2, c2, p))
		return 1;
	return 0;
}

short intersect_2d_lc(sgFloat a, sgFloat b, sgFloat c,
                    sgFloat xc, sgFloat yc, sgFloat r,
										lpD_POINT p)

  
{

sgFloat r1, r2, xt, yt;
short kp = 1;

  r1 = a*xc + b*yc + c;
  if( fabs(r1) - r >= eps_d)
		return 0;

	p[0].x = xt = xc - a*r1;
	p[0].y = yt = yc - b*r1;
	p[0].z = 0.;

  if(r - fabs(r1) >= eps_d){
    r2 = sqrt(r*r - r1*r1);
    p[0].x = xt + b*r2;
	  p[0].y = yt - a*r2;
    p[1].x = xt - b*r2;
    p[1].y = yt + a*r2;
		p[1].z = 0.;
		kp = 2;
	}
	return kp;
}

short intersect_2d_cc(sgFloat xc1, sgFloat yc1, sgFloat r1,
                    sgFloat xc2, sgFloat yc2, sgFloat r2,
										lpD_POINT p)
 
{

sgFloat r, dx, dy, xt, yt, s, c;
short kp = 1;

  r = hypot(xc1 - xc2, yc1 - yc2);
  if(r < eps_d){
		if(fabs(r1 - r2) < eps_d)
			el_geo_err = EL_EQ_ARC;
		return 0;
	}
  if(r1 + r2 + eps_d < r)
		return 0;
  min_max_2(r1, r2, &dx, &dy);
  if(dx + r + eps_d < dy)
		return 0;

  s = (yc1 - yc2)/r;
  c = (xc1 - xc2)/r;
  dx = 0.5*((r2*r2 - r1*r1)/r + r);
  p[0].x = xt = xc2 + dx*c;
	p[0].y = yt = yc2 + dx*s;
	p[0].z = 0.;
	if( r2 - fabs(dx) >= eps_d){  //  
		dy = sqrt(r2*r2 - dx*dx);
		dx = dy*s;
		dy = dy*c;
		p[0].x = xt + dx;
		p[0].y = yt - dy;
		p[1].x = xt - dx;
		p[1].y = yt + dy;
		p[0].z = p[1].z = 0.;
		kp = 2;
	}
	return kp;
}

BOOL intersect_3d_la(lpGEO_LINE l, BOOL flag_line,
										 lpGEO_ARC a, BOOL flag_arc,
										 lpD_POINT p, short *kp)
  
{
sgFloat   al, bl, cl, da;
short      i;
D_POINT  n;
lpGEO_ARC  a2d = &el_geo1;
lpGEO_LINE l2d = (lpGEO_LINE)&el_geo2;

	set_el_geo();
	*kp = 0;
//    ()
	if(!test_wrd_arc(a, &flag_arc))
		return FALSE;
//     
	if(!normv(TRUE, dpoint_sub(&l->v2, &l->v1, &n), &n)){
		el_geo_err = EL_LINE_NULL_LEN;  //  
		return FALSE;
	}
//  4- .  
	da = - a->n.x*a->vc.x - a->n.y*a->vc.y - a->n.z*a->vc.z;

//      
	switch(intersect_3d_pl(&a->n, da, &l->v1, &n, p)) {
		case -1: //   
			break;
		case 1: //   
			if(test_in_pl_3d(TRUE, p, l, flag_line) &&
				 test_in_pa_3d(FALSE, p, a, flag_arc))
				*kp = 1;
			break;
		case 0: //    
//  2D    
			trans_arc_to_acs(a, flag_arc, a2d, el_g_e, el_e_g);
			o_hcncrd(el_g_e, &l->v1, &l2d->v1);
			o_hcncrd(el_g_e, &l->v2, &l2d->v2);
			eq_line(&l2d->v1, &l2d->v2, &al, &bl, &cl);
//  2D -  
			if((*kp = intersect_2d_lc(al, bl, cl, 0., 0., a->r, p)) != 0){
//    
				if(flag_line)
					test_in_arrey(p, kp, OLINE, TRUE, l2d);
//    
				if(*kp && flag_arc)
					test_in_arrey(p, kp, OARC, TRUE, a2d);
//   
				if(*kp)
					for(i = 0; i < *kp; i++)
						o_hcncrd(el_e_g, &p[i], &p[i]);
			}
			break;
	}
	return TRUE;
}

BOOL intersect_3d_aa(lpGEO_ARC a1, BOOL fl_arc1, lpGEO_ARC a2, BOOL fl_arc2,
										 lpD_POINT p, short *kp)
     
{

D_POINT    c;
short        i, j;
lpGEO_ARC  a12d = &el_geo1;
BOOL       fl_pr2 = FALSE;

	set_el_geo();
	*kp = 0;
//     ()
	if(!test_wrd_arc(a1, &fl_arc1))
		return FALSE;
	if(!test_wrd_arc(a2, &fl_arc2))
		return FALSE;

// ,     
	if(!coll_tst(&a1->n, TRUE, &a2->n, TRUE)){

//   

		if(!tr_arc_pl_to_acs(a1, fl_arc1, a12d, el_g_e, el_e_g, a2, &c))
			goto ret;
//         
		if(!(*kp = intersect_2d_lc(c.x, c.y, c.z, 0., 0., a1->r, p)))
			goto ret;
//     
met:
		if(fl_arc1)
			if(!test_in_arrey(p, kp, OARC, TRUE, a12d))
				goto ret;
//     
//       
		for( i = 0, j = 0; i < *kp; i++){
			o_hcncrd(el_e_g, &p[i], &p[i]);
			if(test_in_pa_3d(fl_pr2, &p[i], a2, fl_arc2))
				dpoint_copy(&p[j++], &p[i]);
		}
		*kp = j;
ret:
		return TRUE;
	}

//    ()   
	trans_arc_to_acs(a1, fl_arc1, a12d, el_g_e, el_e_g);

//        
	o_hcncrd(el_g_e, &a2->vc, &c);
//   
	if(fabs(c.z) >= eps_d)
		goto ret;    //    

//  2D -  
	if(!(*kp = intersect_2d_cc(0., 0., a1->r, c.x, c.y, a2->r, p)))
		goto ret;
	fl_pr2 = TRUE;
	goto met;
}
