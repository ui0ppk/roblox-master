#include "../sg.h"

static IDENT_V zcp_equid(lpZCUT equid, IDENT_V idpw, IDENT_V idfeq,
															sgFloat h, BOOL app_arc);
static IDENT_V zcp_equid_line(lpZCUT cut, lpZCUT equid, IDENT_V idface, IDENT_V idp,
																	 sgFloat h, sgFloat h2);
static short get_point_param(lpD_POINT v0, lpD_POINT v1, lpD_POINT vnew);
static BOOL define_k(lpZCUT cut, IDENT_V idp, sgFloat h1, sgFloat h2);
static sgFloat len_vect(lpD_POINT p);
static sgFloat get_max_h_1(lpZCUT cut, IDENT_V idl, sgFloat h);
static sgFloat get_max_h_2(lpZCUT cut, IDENT_V idl, sgFloat h);
static IDENT_V create_tmp_path(lpZCUT cut, lpZCUT equid, IDENT_V idface, IDENT_V idp);
static IDENT_V create_tmp_path_null(lpZCUT cut, lpZCUT equid, IDENT_V idface, IDENT_V idp);

IDENT_V zcf_equid(lpZCUT cut, lpZCUT equid, IDENT_V idf, sgFloat h, sgFloat h2, BOOL app_arc)
//    (  h    h2   )
//   idf  cut
//  app_arc = TRUE,      (  )
//      equid,  
//      .
//       
//  h > 0,         ,
// .,     - .
//    ,
//  0    
{
//	short			count = 0;
	int		num_otr;
	UCHAR   status;
	IDENT_V idfeq, idp, idp_cur, idpeq, idpw;
	lpZCUT 	cut_cur;
	sgFloat	H, hmax_1, hmax_2, hcur;
	BOOL		end, first;

	status = zcp_status(cut, idf);
	idfeq = zcf_create_new_face(equid, status);
	if ( !idfeq ) return 0;
	idp = zcp_get_first_path_in_face(cut, idf);
	while ( idp ) { //     
		if ( h == 0 ) {
			if ( (idpw = create_tmp_path_null(cut, equid, idfeq, idp)) < 0 ) return 0;
			idp = zcp_get_next_path(cut, idp);
			continue;
		}

		num_otr = zcp_get_num_point(cut, idp);
		if ( num_otr == 2 ) {  // 
			/*idpw =*/ zcp_equid_line(cut, equid, idfeq, idp, h, h2);
//			if ( idpw  > 0) count++;  //   
			idp = zcp_get_next_path(cut, idp);
			continue;
		}
		H = h;														//  
		cut_cur = cut;
		idp_cur = idp;
		first = TRUE;
		do {
			idpw = create_tmp_path(cut_cur, equid, idfeq, idp_cur);
			if ( idpw < 0) goto err;
			if ( idpw == 0) { //  
				if ( cut_cur == equid ) {									//   
					zcp_delete_path(equid, idfeq, idp_cur); //   
				}
				idp_cur = 0;
				break;
			}
			if ( first ) {   //   
				first = FALSE;
				if ( !define_k(equid, idpw, h, h2) ) goto err;
			}
			hmax_1 = get_max_h_1(equid, idpw, h); 	//    ( > 0)
			hmax_2 = get_max_h_2(equid, idpw, h); 	//    ( > 0)
			if ( hmax_1 == 0 || hmax_2 == 0 ) { //    
				if ( cut_cur == equid ) { 										//   
					zcp_delete_path(equid, idfeq, idp_cur); //   
					idp_cur = 0;
				}
				break;
			}
			if ( fabs(hmax_1) < fabs(H) || fabs(hmax_2) < fabs(H) ) {
				hcur = min(hmax_1,hmax_2) * sg(h, eps_d);
				H = H - hcur;
				end = FALSE;
			} else {
				hcur = H;
				end = TRUE;
			}

			idpeq = zcp_equid(equid, idpw, idfeq, hcur, app_arc);
			zcp_delete_path(equid, idfeq, idpw); 				//   
			if ( cut_cur == equid ) {										//   
				zcp_delete_path(equid, idfeq, idp_cur); 	//   
			}
//			if ( idpeq == 0 ) break;		//  
			cut_cur = equid;
			idp_cur = idpeq;
		} while ( !end  && idpeq != 0);

		if ( idp_cur ) {								//  
			idpw = create_tmp_path(equid, equid, idfeq, idp_cur);
			if ( idpw  < 0) goto err;
			zcp_delete_path(equid, idfeq, idp_cur);
 //			if ( idpw  > 0) count++;  //   
		}
		idp = zcp_get_next_path(cut, idp);
	}

	return idfeq;
err:
	zcp_delete_path(equid, idfeq, idp_cur); //   
	return 0;
}
static IDENT_V zcp_equid(lpZCUT equid, IDENT_V idpw, IDENT_V idfeq, sgFloat h, BOOL app_arc)
//  app_arc = TRUE,      (  )
//      h   idpw  equid.
//       ,     .
//       ,  idfeq
//    ,
//  0    
{
	IDENT_V   idvbeg, idv0, idv1, idv2, idpeq;
	D_POINT   v0, v1, v2, ve1, vb, c01, c12;
	D_POINT   vp, vs1, vs2, vs3, vs4;
	BOOL      flag = TRUE;
	int      num_otr;
	short       min_otr = 3, fl1, fl2; //, unclose = 1;
	sgFloat		k0, k1, k2;
	UCHAR     status, status1;

	v0.z = v1.z = v2.z = ve1.z = vp.z = vb.z = 0.;
	vs1.z = vs2.z = vs3.z = vs4.z = 0.;
	status = status1 = zcp_status(equid, idpw);
	if ( status & ZC_CLOSE ) status1 -= ZC_CLOSE;
	if ( !(idpeq = zcp_create_new_path(equid, idfeq, status1)) ) return 0;

	num_otr = zcp_get_num_point(equid, idpw); // - unclose;
	if ( num_otr < min_otr ) goto empty;

	v0.z = v1.z = v2.z = 0;
	idv0 = zcv_get_first_point(equid, idpw, &v0, &k0);
	idv1 = zcv_get_next_point(equid, idv0, &v1, &k1);
	eq_line(&v0, &v1, &c01.x, &c01.y, &c01.z);
	vp = c01; vp.z = 0.;
	dpoint_scal(&vp, h*k0, &vp);
	dpoint_add(&v0, &vp, &vs1);
	if ( !(status & ZC_CLOSE) ) {
		if ( !zcv_add_to_tail(equid, idpeq, &vs1, k0) ) goto empty;
	}
	idv2 = idvbeg = zcv_get_next_point(equid, idv1, &v2, &k2);

	do {
//		eq_line(&v0, &v1, &c01.x, &c01.y, &c01.z);
		vp = c01; vp.z = 0.;
		dpoint_scal(&vp, h*k1, &vp);
		dpoint_add(&v0, &vp, &vs1);
		dpoint_add(&v1, &vp, &vs2);

		eq_line(&v1, &v2, &c12.x, &c12.y, &c12.z);
		p_int_lines(c01.x, c01.y, c01.z - h*k1, c12.x, c12.y, c12.z - h*k1, &ve1);

		//    
		vp = c12; vp.z = 0.;
		dpoint_scal(&vp, h*k1, &vp);
		dpoint_add(&v1, &vp, &vs3);
		dpoint_add(&v2, &vp, &vs4);

		fl1 = get_point_param(&vs1, &vs2, &ve1);
		fl2 = get_point_param(&vs3, &vs4, &ve1);
		if ( !app_arc ) { 				//   
			if ( !zcv_add_to_tail(equid, idpeq, &ve1, k1) ) goto empty;
		} else {
			if ( !flag ) {  //    
	//			if ( fl1 >= 0 )
					if ( !zcv_add_to_tail(equid, idpeq, &vs1, k1) ) goto empty;
			}
			if ( !(fl1 == 1 && fl2 == -1) ) {		//  
				if ( !zcv_add_to_tail(equid, idpeq, &ve1, k1) ) goto empty;
				flag = TRUE;
			}	else {                        //  
				flag = FALSE;
				dpoint_sub(&ve1, &v1, &vp);
				dnormal_vector1(&vp);     								//  
				dpoint_scal(&vp, fabs(h*k1), &vp);
				dpoint_add(&v1, &vp, &vb);							//   
				if ( dpoint_eq(&vb, &ve1, c_h_tolerance) ) { //   
					if ( !zcv_add_to_tail(equid, idpeq, &ve1, k1) ) goto empty;
					flag = TRUE;
				} else {
					if ( !zcv_add_to_tail(equid, idpeq, &vs2, k1) ) goto empty;
					if ( !zcv_add_to_tail(equid, idpeq, &vb, k1) ) goto empty;
				}
			}
		}

		/*idv0 = idv1; */idv1 = idv2;
		v0 = v1; v1 = v2;
		c01 = c12;
		vs1 = vs3; vs2 = vs4; k1 = k2;
		idv2 = zcv_get_next_point(equid, idv1, &v2, &k2);
		v2.z = 0;
	} while ( idv2 != idvbeg && idv2 != 0 );

	if ( !flag ) {  //    
			if ( !zcv_add_to_tail(equid, idpeq, &vs1, k1) ) goto empty;
	}

	vp = c01; vp.z = 0.;
	dpoint_scal(&vp, h*k1, &vp);
	dpoint_add(&v1, &vp, &vs2);

	if ( status & ZC_CLOSE) zcp_close(equid, idpeq);
	else {
		if ( !zcv_add_to_tail(equid, idpeq, &vs2, k2) ) goto empty;
	}

//end:
	return idpeq;
empty:
	if ( idpeq )
		zcp_free_path(equid, idpeq);
	return 0;
}
static sgFloat get_max_h_2(lpZCUT cut, IDENT_V idl, sgFloat h)
//           
// (  ) 
{
	IDENT_V   idvbeg, idv0, idv1, idv2;
	D_POINT   v0, v1, v2, v3;
	D_POINT   w1, w2, w3, c, c1, c2, p, p1;
	D_POINT		n = {0.,0.,1.};			//  
	sgFloat		hmin = MAXsgFloat, hcur, cs;
	sgFloat		k0, k1, k2;

	if ( h < 0 )dpoint_inv(&n, &n);

	v0.z = v1.z = v2.z = v3.z = 0;
	idv0 = zcv_get_first_point(cut, idl, &v0, &k0);
	idv1= idvbeg = zcv_get_next_point(cut, idv0, &v1, &k1);
	idv2 = zcv_get_next_point(cut, idv1, &v2, &k2);
	dpoint_sub(&v0,&v1, &w1);
	dnormal_vector1(&w1);     								//  
	if ( fabs(k0) < eps_d ) k0 = eps_d;
	if ( fabs(k1) < eps_d ) k1 = eps_d;
	if ( fabs(k2) < eps_d ) k2 = eps_d;

	do {
		dpoint_sub(&v2,&v1, &w2);
		dnormal_vector1(&w2);     									//  
		dpoint_add(&w1, &w2, &w3);                //  
		dvector_product(&w1, &w2, &c);
		dnormal_vector1(&c);
		n.z = 1;
		if ( h * k1 < 0 ) n.z = - n.z;
		cs = dskalar_product(&c, &n);
		if ( cs < 0 ) {      //  
			dpoint_add(&v1, &w3, &v3);
			eq_line(&v1, &v3, &c1.x, &c1.y, &c1.z);   //   

			//  
			dvector_product(&w1, &n, &c);
			dpoint_add(&v0, &c, &v3);
			eq_line(&v0, &v3, &c2.x, &c2.y, &c2.z);

			if ( p_int_lines( c1.x, c1.y, c1.z, c2.x, c2.y, c2.z,	&p) )		{ //  
				hcur = dist_p_l(&v0, &w1, &p, &p1) / k0; // k0 -      v0
				if ( hcur < hmin ) hmin = hcur; 		//        p
			}
			//  
			dvector_product(&w2, &n, &c);
			dpoint_add(&v2, &c, &v3);
			eq_line(&v2, &v3, &c2.x, &c2.y, &c2.z);

			if ( p_int_lines( c1.x, c1.y, c1.z, c2.x, c2.y, c2.z,	&p) )		{ //  
				hcur = dist_p_l(&v2, &w2, &p, &p1) / k2;   // k2 -      v2
				if ( hcur < hmin ) hmin = hcur; 		//        p
			}
		}

		v0 = v1; v1 = v2; k0 = k1; k1 = k2;
		dpoint_inv(&w2, &w1);
		idv2 = zcv_get_next_point(cut, idv2, &v2, &k2);
		if ( fabs(k2) < eps_d ) k2 = eps_d;
		v2.z = 0;
	} while( idv2 != idvbeg && idv2 != 0);

	return hmin;
//met:
//	return 0;
}

static sgFloat get_max_h_1(lpZCUT cut, IDENT_V idl, sgFloat h)
//     
{
	IDENT_V   idvbeg, idv0, idv1, idv2, idv3;
	D_POINT   v0, v1, v2, v3;
	D_POINT   w1, w2, w3, c, p, p1;
	D_POINT		n = {0.,0.,1.};			//  
	sgFloat		hmin = MAXsgFloat, hcur, cs;
	sgFloat 		a1, b1, c1, a2, b2, c2;
	sgFloat		k0, k1, k2;
	UCHAR     status;

	if ( h < 0 )dpoint_inv(&n, &n);

	v0.z = v1.z = v2.z = v3.z = 0;
	idv0 = zcv_get_first_point(cut, idl, &v0, &k0);
	idv1 = zcv_get_next_point(cut, idv0, &v1, &k1);

	status = zcp_status(cut, idl);
	if ( !(status & ZC_CLOSE) ) { //   
		dpoint_sub(&v1,&v0, &w2);
		dnormal_vector1(&w2);     								//  
		dvector_product(&n, &w2, &w3);
		v2 = v1; idv2 = idv1;
		v1 = v0; /*idv1 = idv0; */
	} else {
		idv2 = zcv_get_next_point(cut, idv1, &v2, &k2);
		dpoint_sub(&v0,&v1, &w1);
		dpoint_sub(&v2,&v1, &w2);
		dnormal_vector1(&w1);     								//  
		dnormal_vector1(&w2);     								//  
		dpoint_add(&w1, &w2, &w3);              //   
		dvector_product(&w1, &w2, &c);
		dnormal_vector1(&c);     								//  
		cs = dskalar_product(&c, &n);
/*		if ( fabs(cs) <= eps_d ) { 							//     
			dvector_product(&w1, &n, &w3);
		}
*/
		if ( cs >= eps_d ) dpoint_inv(&w3, &w3);      //  
	}
	eq_line(&v1, dpoint_add(&v1, &w3, &c), &a1, &b1, &c1);		 //   

	idv3 = idvbeg = zcv_get_next_point(cut, idv2, &v3, &k2);
//	if ( idv3 == idv2 )		return 0;

	do {
		dpoint_inv(&w2, &w1);
		dpoint_sub(&v3,&v2, &w2);
		dnormal_vector1(&w2);     									//  
		dpoint_add(&w1, &w2, &w3);                //   

		dvector_product(&w1, &w2, &c);
		dnormal_vector1(&c);     // 
		cs = dskalar_product(&c, &n);
		if (cs > 0.) dpoint_inv(&w3, &w3);        //  

		eq_line(&v2, dpoint_add(&v2, &w3, &c), &a2, &b2, &c2);	 //   

		if ( p_int_lines( a1, b1, c1, a2, b2, c2,	&p) )		{ //  
			dpoint_inv(&w1, &w1);
			hcur = dist_p_l(&v1, &w1, &p, &p1);
			if ( hcur < hmin ) { 		//        p
				dpoint_sub(&p,&p1, &c);
				dnormal_vector1(&c);     									//  
				dvector_product(&w1, &c, &p1);
				cs = dskalar_product(&p1, &n);
				if ( cs > 1 - eps_n	)		//   
					hmin = hcur;
			}
		}

		a1 = a2; 	b1 = b2;	c1 = c2; k1 = k2;
		v1 = v2;	v2 = v3;  idv2 = idv3;

		idv3 = zcv_get_next_point(cut, idv2, &v3, &k2);
    v3.z = 0;
	} while( idv3 != idvbeg && idv3 != 0);

	if ( idv3 == 0 ) { //   ,   
		dvector_product(&n, &w2, &w3);
		eq_line(&v2, dpoint_add(&v2, &w3, &c), &a2, &b2, &c2);	 //   

		if ( p_int_lines( a1, b1, c1, a2, b2, c2,	&p) )		{ //  
			dpoint_inv(&w1, &w1);
			hcur = dist_p_l(&v1, &w1, &p, &p1);
			if ( hcur < hmin ) hmin = hcur;
		}
	}

	return hmin;
//met:
//	return 0;
}
static IDENT_V create_tmp_path(lpZCUT cut, lpZCUT equid, IDENT_V idface, IDENT_V idp)
//    idp   cut,       
//      equid,  idface
// : >0 -  
//						  0 -  
//             -1 -   
{
	short			count;
	IDENT_V idvbeg, idv0, idv1, idv2, idv_b;
	D_POINT v0, v1, v2, c01; //, ve1, c12;
	int    num_otr, num_count;
	short     min_otr = 1, unclose = 1;
	IDENT_V idpw;
	UCHAR   status;
	sgFloat	k0, k1, k2;

	status = zcp_status(cut, idp);
	count = 0;

	if ( !(idpw = zcp_create_new_path(equid, idface, status))) return -1;
	//     -    idp
	v0.z = v1.z = v2.z = 0;
	idv0 = idvbeg = zcv_get_first_point(cut, idp, &v0, &k0);
	idv1 = zcv_get_next_point(cut, idv0, &v1, &k1);
	if ( status & ZC_CLOSE ) {
		/*idv2 = */ zcv_get_prev_point(cut, idv0, &v2, &k2);
		if ( !dpoint_eq(&v0, &v2, eps_d) ) {
			if ( eq_line(&v0, &v2, &c01.x, &c01.y, &c01.z) ) {
				if ( !zcv_add_to_tail(equid, idpw, &v0, k0) ) goto err;
				count++;
			}
		}
	} else {
		if ( !zcv_add_to_tail(equid, idpw, &v0, k0) ) goto err;
		count++;

	}
	while ( idv1 != idvbeg && idv1 != 0 ) { //  ,   
		if ( !dpoint_eq(&v0, &v1, eps_d) ) {
			if ( eq_line(&v0, &v1, &c01.x, &c01.y, &c01.z) ) {
				if ( !zcv_add_to_tail(equid, idpw, &v1, k1) ) goto err;
				count++;
				v0 = v1;
			}
		}
		idv1 = zcv_get_next_point(cut, idv1, &v1, &k1);
		v1.z = 0;
	}

	if ( count < 3 ) goto empty;
	if ( status & ZC_CLOSE ) {
		 zcp_close(equid, idpw);
		 unclose = 0;
		 min_otr  = 3;
	}

	num_otr = zcp_get_num_point(equid, idpw) - unclose;
	if ( num_otr < min_otr ) goto empty;

	idv0 = idv_b = zcv_get_first_point(equid, idpw, &v0, &k0);
	idv1 = zcv_get_next_point(equid, idv0, &v1, &k1);
	idv2 = idvbeg = zcv_get_next_point(equid, idv1, &v2, &k2);
	eq_line(&v0, &v1, &c01.x, &c01.y, &c01.z);
//	eq_line(&v1, &v2, &c12.x, &c12.y, &c12.z);

	count = 0;
	num_count = num_otr;
	while ( count <= num_count + 1 ) {
		count++;
//		if ( !dpoint_eq(&c01, &c12, eps_n) &&
//				 !dpoint_eq(&c01, dpoint_inv(&c12, &ve1), eps_n) ) {
		if ( fabs(c01.x*v2.x + c01.y*v2.y + c01.z) > eps_d ) {
			v0 = v1; 			v1 = v2; 			//c01 = c12;
			idv0 = idv1;  idv1 = idv2;
		} else {  //  
			zcv_detach_point(equid, idpw, idv1);
			num_otr--;
			if ( !dpoint_eq(&v0, &v2, eps_d) ) {
				if ( eq_line(&v0, &v2, &c01.x, &c01.y, &c01.z) ) {
					v1 = v2;			//   ,  
					idv1 = idv2;
					goto next;
				}
			}

			if ( idv0 != idv_b || status & ZC_CLOSE ) { // v0       
				idv1 = idv0; v1 = v0; k1 = k0;         						// v2   v0
				idv0 = zcv_get_prev_point(equid, idv1, &v0, &k0);
				zcv_detach_point(equid, idpw, idv2);
				idv2 = idv1;
			} else { // v0   
				idv1 = zcv_get_next_point(equid, idv0, &v1, &k1);
				idv2 = idv1;
			}
			num_otr--;
//			if ( idv2 == idvbeg || idv2 == 0 ) break;
//			eq_line(&v0, &v1, &c01.x, &c01.y, &c01.z);
		}
next:
		idv2 = zcv_get_next_point(equid, idv2, &v2, &k2);
		if ( idv2 == idvbeg || idv2 == 0 ) break;
		eq_line(&v0, &v1, &c01.x, &c01.y, &c01.z);
//		eq_line(&v1, &v2, &c12.x, &c12.y, &c12.z);
	}

	if ( num_otr < min_otr ) goto empty;

	return idpw;

err:
	zcp_delete_path(equid, idface, idpw); //   
	return -1;
empty:
	zcp_delete_path(equid, idface, idpw); //   
	return 0;
}
static IDENT_V create_tmp_path_null(lpZCUT cut, lpZCUT equid, IDENT_V idface, IDENT_V idp)
//    0 !!!!!
//    idp   cut,       
//      equid,  idface
//    0 -      
{
//	short			count;
	IDENT_V idvbeg, idv0, idv1; //, idv2;
	D_POINT v0, v1, v2, c01; //, ve1, c12;
//	int    num_otr;
//	short     min_otr = 1, unclose = 1;
	IDENT_V idpw;
	UCHAR   status;
	sgFloat	k0, k1, k2;

	status = zcp_status(cut, idp);
//	count = 0;

	if ( !(idpw = zcp_create_new_path(equid, idface, status))) return 0;
	//     -    idp
	v0.z = v1.z = v2.z = 0;
	idv0 = idvbeg = zcv_get_first_point(cut, idp, &v0, &k0);
	idv1 = zcv_get_next_point(cut, idv0, &v1, &k1);
	if ( status & ZC_CLOSE ) {
//		idv2 = zcv_get_prev_point(cut, idv0, &v2);
		zcv_get_prev_point(cut, idv0, &v2, &k2);
		if ( !dpoint_eq(&v0, &v2, eps_d) ) {
			if ( eq_line(&v0, &v2, &c01.x, &c01.y, &c01.z) ) {
				if ( !zcv_add_to_tail(equid, idpw, &v0, k0) ) goto empty;
//				count++;
			}
		}
	} else {
		if ( !zcv_add_to_tail(equid, idpw, &v0, k0) ) goto empty;
//		count++;

	}
	while ( idv1 != idvbeg && idv1 != 0 ) { //  ,   
		if ( !dpoint_eq(&v0, &v1, eps_d) ) {
			if ( eq_line(&v0, &v1, &c01.x, &c01.y, &c01.z) ) {
				if ( !zcv_add_to_tail(equid, idpw, &v1, k1) ) goto empty;
 //				count++;
				v0 = v1;
			}
		}
		idv1 = zcv_get_next_point(cut, idv1, &v1, &k1);
		v1.z = 0;
	}

	if ( status & ZC_CLOSE ) zcp_close(equid, idpw);

	return idpw;

empty:
	zcp_delete_path(equid, idface, idpw); //   
	return 0;
}
static short get_point_param(lpD_POINT v0, lpD_POINT v1, lpD_POINT vnew)
// : -1,  vnew  v0
//							1,  vnew   v0
//							0,  vnew   v0  v1,   v0  v1
{
	D_POINT vect1, vect2;
	sgFloat  lv;

	if ( dpoint_eq(v0, vnew, eps_d) ) return 0; 	//   v0
	if ( dpoint_eq(v1, vnew, eps_d) ) return 0; 	//   v1
	dpoint_sub(v1, v0, &vect1);
	dpoint_sub(vnew, v0, &vect2);
	lv = len_vect(&vect2); 													//  vect2
	if(coll_tst(&vect1, FALSE, &vect2, FALSE) != 1) return -1;
	else if(lv > len_vect(&vect1) + eps_d) return 1;
	else return 0;
}

static sgFloat len_vect(lpD_POINT p){
	return hypot(p->x, p->y);
}
//    idp   
// h1 -    , h2 -    
//   -   
static BOOL define_k(lpZCUT cut, IDENT_V idp, sgFloat h1, sgFloat h2)
{
	IDENT_V 	idvb, idv; //, idv1;
	D_POINT 	vn, vk; //, p;
//	IDENT_V 	idpw;
//	UCHAR   	status;
	sgFloat 		L, l, k;

//	status = zcp_status(cut, idp);

	//   
	vn.z = vk.z = 0.;
	L = 0;
	idv = idvb = zcv_get_first_point(cut, idp, &vn, &k);
	do {
		idv = zcv_get_next_point(cut, idv, &vk, &k);
		if (idv == 0) break;	//   
		L += dpoint_distance(&vn, &vk);
    vn = vk;
	} while (idv != idvb);

//	if ( !(idpw = zcp_create_new_path(cut, idface, status))) return 0;
	//     -    idp
	idv = idvb = zcv_get_first_point(cut, idp, &vn, &k);
	vn.z = vk.z = 0.;
	l = 0;       //   
	k = 1;
	zcv_overwrite_point(cut, idv, &vn, k);
//	if ( !zcv_add_to_tail(cut, idpw, &p) ) goto empty;
	do {
		idv = zcv_get_next_point(cut, idv, &vk, &k);
		if (idv == 0) break;	//   
		l += dpoint_distance(&vn, &vk);
		k = 1 - l/L*(1 - h2/h1);
//		if ( !zcv_add_to_tail(cut, idpw, &vk) ) goto empty;
		zcv_overwrite_point(cut, idv, &vk, k);
//		idv = idv1;
		vn = vk;
	} while (idv != idvb);

//	if ( status & ZC_CLOSE ) zcp_close(cut, idpw);

	return TRUE;

//empty:
//	zcp_delete_path(cut, idface, idpw); //   
//	return FALSE;
}
static IDENT_V zcp_equid_line(lpZCUT cut, lpZCUT equid, IDENT_V idface, IDENT_V idp,
																	 sgFloat h, sgFloat h2)
{
	short			count;
	sgFloat  k;
	IDENT_V idv0; //, idv1;
	D_POINT v0, v1, p1, p2, pn, pc;
	IDENT_V idpw;
	UCHAR   status;

	count = 0;

	status = zcp_status(cut, idp);
	if ( !(idpw = zcp_create_new_path(equid, idface, status))) return -1;
	v0.z = v1.z = 0;
	idv0 = zcv_get_first_point(cut, idp, &v0, &k);
	zcv_get_next_point(cut, idv0, &v1, &k);

	if ( dpoint_eq(&v0, &v1, eps_d) ) goto empty;

	pn = v0;
	pn.z = 1;
	dpoint_sub(&v1, &v0, &p1);
	dpoint_sub(&pn, &v0, &p2);
	dvector_product(&p2, &p1, &pn);
	dnormal_vector(&pn);		// ,  

	dpoint_scal(&pn, h, &pc);
	dpoint_add(&v0, &pc, &p1);
	dpoint_scal(&pn, h2, &pc);
	dpoint_add(&v1, &pc, &p2);

	if ( !zcv_add_to_tail(equid, idpw, &p1, h) ) { count = -1; goto empty; }
	if ( !zcv_add_to_tail(equid, idpw, &p2, h2) ) { count = -1; goto empty; }

	return idpw;

empty:
	zcp_delete_path(equid, idface, idpw); //   
	return count;
}

