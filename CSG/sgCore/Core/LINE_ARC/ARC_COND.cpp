#include "../sg.h"


REZ_GEO       rez_geo;

BOOL arc_negativ = FALSE;

static void add_arc_lines(lpGEO_LINE lines, short* num,
                          lpD_POINT v1, lpD_POINT v2);

void init_arc_geo_cond(short     cnum,     
									     CNDTYPE ctype){   
short i;
	gcond.flags[0]    = cnum;
	gcond.ctype[cnum] = ctype;
	gcond.flags[5] = 0;
	for(i = 0; i <= cnum; i++)
    gcond.flags[5] |= gcond.ctype[i];
}

BOOL arc_geo_cond_gcs(lpD_POINT  p,       
									    lpD_POINT  norm,      
									    OBJTYPE    *outtype,   
									    lpGEO_ARC  outgeo){   
BOOL wflag = FALSE;
short  num   = 0;
  return arc_geo_cond(p, norm, TRUE, outtype, outgeo, &num, NULL, &wflag);
}

BOOL arc_geo_cond(lpD_POINT  p,       
									lpD_POINT  norm,    
									BOOL       gcsflag,    
									OBJTYPE    *outtype,  
									lpGEO_ARC  outgeo,   
									short        *numlines,
									lpGEO_LINE lines,     
									BOOL       *newplane) 
      
{
lpMATR     gcs_pl = gcond.m[0];
lpMATR     pl_gcs = gcond.m[1];
lpMATR     wmatr  = gcond.m[2];
lpD_PLANE  pl     = gcond.pl;
lpD_POINT  vp     = gcond.p;
lpD_POINT  pp     = gcond.p + 3;
lpD_POINT  pg     = gcond.p + 6;
short        cnum   = gcond.flags[0];
CNDTYPE    ctype;
D_POINT    wp, wp1;
short        iln[3], ipn[3], nln = 0, npn = 0, i;
lpGEO_LINE wl;
short 		   oldgeoflag;
sgFloat     w;


#define      geoflag  gcond.flags[1]
#define           ib  gcond.flags[2]
#define           ie  gcond.flags[3]
#define           ic  gcond.flags[4]
#define common_ctype  gcond.flags[5]

	*newplane = FALSE;
	*numlines = 0;
	*outtype  = OPOINT;

	if(!cnum) { // 
		ib = ie = ic = -1;
		//   
		geoflag = PL_NODEF;
		//    
		o_hcunit(wmatr);
		o_tdtran(wmatr, dpoint_inv(p, &wp));
	}

	ctype  = gcond.ctype[cnum];
	if     (ctype&CND_START)  ib = cnum;
	else if(ctype&CND_END)    ie = cnum;
	else if(ctype&CND_POINT)  ic = cnum;


	if(ctype&CND_POINT)
		dpoint_copy(&vp[cnum], p);

	//   - ,   
	if(ctype&CND_SCAL){
		if(geoflag == PL_NODEF)
			goto met_nodef;
		oldgeoflag = geoflag;
		geoflag = PL_FIXED;
		goto met_create;
	}

  if(!define_cond_plane(cnum, &geoflag, norm, pl, vp, &i))
		return FALSE;
	switch(i){
		case 0:
			goto met_create;
		case 2:
			goto met_matr;
		default: // case 1:
      break;
	}
//nb met_pl_to_cond:  //    
	if(geoflag == PL_FIXED)
		return FALSE;
	if((gcond.type[cnum] == OARC) || (gcond.type[cnum] == OCIRCLE)){
	  //      ( )
	  dpoint_copy(&pl->v, &gcond.geo[cnum].n);
	  pl->d = dcoef_pl(&vp[0], &pl->v);
		for(i = 0; i <= cnum; i++)
			if(!test_cond_to_plane(i))
		    goto met_nodef;
		if(cnum < gcond.maxcond - 1) goto met_matr;
			goto met_def;
	}
	geoflag = PL_NODEF;
	if(!cnum)
		return TRUE;
	for(i = 0; i <= cnum; i++)
		if(gcond.type[i] == OLINE) iln[nln++] = i;
			else                     ipn[npn++] = i;

	switch(nln){
		case 0: //  
			if(npn != 3)
				goto met_line;
			if(!eq_plane(&vp[0], &vp[1], &vp[2], &pl->v, &pl->d))
				goto met_nodef;
			goto met_def;
		case 3: //  3,      
			goto met_nodef; // -  
		case 2: //  2
			if(eq_plane_ll((lpGEO_LINE)(&gcond.geo[iln[0]]),
			               (lpGEO_LINE)(&gcond.geo[iln[1]]),
										 &pl->v, &pl->d) != 1)
				goto met_nodef;
			if(npn){
				if(!test_cond_to_plane(ipn[0]))
					goto met_nodef;
			}
			else {  //   ,     - 
	      geoflag = PL_FIXED;
			  goto met_matr;
			}
			goto met_def;
		case 1: //   1
			switch(npn){
				case 1:
					if(!eq_plane_pl(&vp[ipn[0]], (lpGEO_LINE)(&gcond.geo[iln[0]]),
					   &pl->v, &pl->d))
						goto met_line;
					if(gcsflag || (iln[0] == 1)){ // 
	          geoflag = PL_FIXED;
			      goto met_matr;
					}
					goto met_def;
				case 2:
					wl = (lpGEO_LINE)(&gcond.geo[ipn[0]]);
					dpoint_copy(&wl->v1, &vp[ipn[0]]);
					dpoint_copy(&wl->v2, &vp[ipn[1]]);
					if(eq_plane_ll((lpGEO_LINE)(&gcond.geo[iln[0]]), wl,
												 &pl->v, &pl->d) != 1)
						goto met_nodef;
					goto met_def;
			}
	}
met_def:
	geoflag = PL_DEF;
met_matr: 	//       
	memcpy(gcs_pl, wmatr, sizeof(MATR));
	//     
	if(dskalar_product(&pl->v, norm) < 0.){
		dpoint_inv(&pl->v, &pl->v);
		pl->d = -pl->d;
	}
	o_hcrot1(gcs_pl, &pl->v);
	o_minv(gcs_pl, pl_gcs);

	for(i = 0; i < cnum; i++)
		trans_cond_to_plane(i, pp, pg);

	*newplane = TRUE;

met_create: //      
						//   

	trans_cond_to_plane(cnum, pp, pg);
	switch(cnum){
		case 0:   //   
			return TRUE;
		case 1:   //  
			if(!(common_ctype&CND_CENTER)) //   
			  goto met_line;
			//  
			*outtype = OCIRCLE;
			if(gcond.ctype[ib]&CND_TANG) //   
				circle_c_bt(&pp[ic], &pg[ib], &pp[ib], gcond.type[ib], &wp);
			else
				dpoint_copy(&wp, &pp[ib]);
			if(!arc_b_e_c(&wp, &wp, &pp[ic], TRUE, FALSE, FALSE, outgeo))
				return FALSE;
			if(!gcsflag){
			//      
				o_hcncrd(pl_gcs, &wp, &wp);
				add_arc_lines(lines, numlines, &vp[ic], &wp);
				return TRUE;
			}
			if(gcond.ctype[ib]&CND_TANG){  //    - 
				geoflag = PL_FIXED;
				dpoint_copy(&pp[ib], &wp);
			}
			goto met_arc;
		default:  //  
			*outtype = OARC;
			if(common_ctype&CND_CENTER){ //  
				if(common_ctype&CND_END){ //   -  
					if((gcond.ctype[cnum]&CND_CENTER) &&      //  
						 (gcond.ctype[ib]&CND_TANG))            //    
						 circle_c_bt(&pp[ic], &pg[ib], &pp[ib], gcond.type[ib], &wp);
					else
						dpoint_copy(&wp, &pp[ib]);
					if(!arc_b_e_c(&wp, &pp[ie], &pp[ic], FALSE, FALSE,
												arc_negativ, outgeo))
						return FALSE;
					if(gcsflag) goto met_arc;
					add_arc_lines(lines, numlines,
												&vp[(gcond.ctype[cnum]&CND_CENTER)?ie:ic], p);
					return TRUE;
				}
				if(common_ctype&CND_LENGTH){ //   -  
					if(!arc_b_c_h(&pp[ib], &pp[ic], gcond.scal[0], arc_negativ, outgeo)){
					  geoflag = oldgeoflag;
						return FALSE;
					}
					if(gcsflag) goto met_arc;
				  o_hcncrd(pl_gcs, &outgeo->vb, &wp);
				  o_hcncrd(pl_gcs, &outgeo->ve, &wp1);
					add_arc_lines(lines, numlines, &wp, &wp1);
					add_arc_lines(lines, numlines, &vp[ic], p);
					return TRUE;
				}
				if(common_ctype&CND_ANGLE){ //   -  
					if(!arc_b_c_a(&pp[ib], &pp[ic], gcond.scal[0], outgeo)){
					  geoflag = oldgeoflag;
						return FALSE;
					}
					if(gcsflag) goto met_arc;
					o_hcncrd(pl_gcs, &outgeo->vb, &wp);
					add_arc_lines(lines, numlines, &vp[ic], &wp);
					o_hcncrd(pl_gcs, &outgeo->ve, &wp);
					dpoint_parametr(&vp[ic], &wp,
						dpoint_distance(p, &vp[ic])/outgeo->r, &wp1);
					add_arc_lines(lines, numlines, &vp[ic], &wp1);
					return TRUE;
				}
				return FALSE;  //    -    
			}
			else if(common_ctype&CND_NEAR){  //    
				if(!arc_p_p_p(ib, ie, ic, pg, pp,
				              (common_ctype&CND_TANG) ? gcond.type : NULL,
				              arc_negativ, outgeo))
					return FALSE;
				if(gcsflag) goto met_arc;
				return TRUE;
			}
			else if(common_ctype&CND_VECTOR){  //  
				if(common_ctype&CND_TANG){ //  
					if(!arc_b_e_vt(&pp[ib], gcond.type[ie], &pg[ie], &pp[ie], &pp[ic],
						 arc_negativ, outgeo))
					return FALSE;
				}
				else{
					if(!arc_b_e_v(&pp[ib], &pp[ie], &pp[ic], arc_negativ, outgeo))
						return FALSE;
				}
				if(gcsflag) goto met_arc;
				if(ctype&CND_VECTOR)
          add_arc_lines(lines, numlines, &vp[ib], p);
				return TRUE;
			}
			else if(ctype&CND_RADIUS){ //  
        if(!arc_p_p_r(ib, ie, gcond.scal[0], pg, pp,
											(common_ctype&CND_TANG) ? gcond.type : NULL,
											arc_negativ, outgeo)){
					geoflag = oldgeoflag;
					return FALSE;
				}
				if(gcsflag) goto met_arc;
				  o_hcncrd(pl_gcs, &outgeo->vc, &wp);
					dpoint_parametr(&wp, p,
													fabs(gcond.scal[0])/dpoint_distance(&wp, p), p);
				  add_arc_lines(lines, numlines, p, &wp);
				return TRUE;
			}
			else if(ctype&CND_APOINT){ // ,  
				if(!arc_b_e_pa(&pp[ib], &pp[ie], &pp[ic], arc_negativ, outgeo))
					return FALSE;
				return TRUE;
			}
			else if(ctype&CND_ANGLE){ //  
				if(!arc_b_e_a(&pp[ib], &pp[ie], gcond.scal[0], outgeo)){
					geoflag = oldgeoflag;
					return FALSE;
				}
				if(gcsflag) goto met_arc;
				o_hcncrd(pl_gcs, &outgeo->vc, &wp);
				if((w = dpoint_distance(&wp, p)) <= outgeo->r){
				  add_arc_lines(lines, numlines, &vp[ib], &wp);
				  add_arc_lines(lines, numlines, &vp[ie], &wp);
				}
				else{
					if(dpoint_distance(&vp[ib], p) < dpoint_distance(&vp[ie], p)){
						nln = ib; npn = ie;
					}
					else{
						nln = ie; npn = ib;
					}
					dpoint_parametr(&wp, &vp[nln], w/outgeo->r, &wp1);
				  add_arc_lines(lines, numlines, &vp[npn], &wp);
				  add_arc_lines(lines, numlines, &wp1, &wp);
				}
				return TRUE;
			}
	}
met_line:  //       
	if(!gcsflag)
    add_arc_lines(lines, numlines, &vp[0], &vp[1]);
	return TRUE;
met_arc:
	o_hcncrd(pl_gcs, &outgeo->vc, &outgeo->vc);
	o_hcncrd(pl_gcs, &outgeo->vb, &outgeo->vb);
	o_hcncrd(pl_gcs, &outgeo->ve, &outgeo->ve);
	dpoint_copy(&outgeo->n, &pl[0].v);
	return TRUE;
met_nodef:
	el_geo_err = BAD_ARC_PLANE;
	geoflag = PL_NODEF;
	return FALSE;
}

static void add_arc_lines(lpGEO_LINE lines, short* num,
                          lpD_POINT v1, lpD_POINT v2){
//      
	if(lines){
	  dpoint_copy(&lines[*num].v1, v1);
	  dpoint_copy(&lines[(*num)++].v2, v2);
	}
}
