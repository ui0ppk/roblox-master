#include "../../sg.h"

NP_TYPE_POINT np_point_np(lpD_POINT p, lpNPW np)
{
	short 		edge, vv;
	short 		v1, v2, p1, p2;
	short    	face, fp, fm;
	int			k = 0;
	D_POINT	pp;							//      
	D_PLANE	pl;
	lpD_POINT	v;
	sgFloat 	f;
	NP_TYPE_POINT p_type;

	for (face = 1; face <= np->nof; face++) {
		pp.y = p->y;
		pp.z = p->z;
		v = &np->p[face].v;
		f = v->x*p->x + v->y*p->y + v->z*p->z + np->p[face].d;
		if ( (sg(f,eps_d)) == 0 ) {           //    
			if ( np_point_belog_to_face1(np, p, face, &edge, &vv) != NP_OUT ) return NP_ON;
		}
		if ( fabs(np->p[face].v.x) < eps_n ) continue;	//   

		pp.x = (-p->y * v->y - p->z * v->z - np->p[face].d)/v->x;
		if ( pp.x < p->x ) continue;       //  
		p_type = np_point_belog_to_face1(np, &pp, face, &edge, &vv);
		if ( p_type == NP_OUT ) continue;
		if ( p_type == NP_IN ) { k++; continue; }
/*		if ( np_point_belog_to_face1(np, &pp, face, &edge, &vv) == NP_OUT )
			continue;
		if ( np_point_belog_to_face1(np, &pp, face, &edge, &vv) == NP_IN ) {
			k++;
			continue;
		}
*/
		//   
		fp = np->efc[edge].fp;
		fm = np->efc[edge].fm;
		if (fp < 0)   fp = np_calc_gru(np, edge);
		if (fm < 0)   fm = np_calc_gru(np, edge);
		v1 = np->efr[edge].bv;
		v2 = np->efr[edge].ev;
		dpoint_cplane((&np->v[v1]), (&np->v[v2]), p, &pp);
		pl.v = pp;
		pl.d = -pp.x*p->x - pp.y*p->y - pp.z*p->z;
		p1 = np_pv(np,         edge,  &pl);
		p2 = np_pv(np,(short)(-edge), &pl);
		if ( p1 == p2 ) continue;
		if ( p1+p2 == 1 ) { k++; continue; }		//   ""
		if ( p1 == -p2 )  { // 
			if ( fp > 0 && fm > 0 ) { //   
				if ( fp < face || fm < face )	k++;
			} else { //  
				if (fp > 0 ) {  //   -
					if (np->ident < np->efc[edge].fm ) k++;
				} else          //   +
					if (np->ident < np->efc[edge].fp ) k++;
			}
		}
	}
	if ((k/2)*2 == k) return NP_OUT;
	else              return NP_IN;
}
sgFloat np_point_dist_to_np(lpD_POINT p, lpNPW np, sgFloat f_min,
													 lpD_POINT p_min)
{
	short 			edge, vv;
	short    		face;
	D_POINT		pp;							//     
	D_POINT   pp_min;
	lpD_POINT	v;
//	sgFloat		f_min = 1.e35;  //     
	sgFloat 		f;							//     

	for (face = 1; face <= np->nof; face++) {
		v = &np->p[face].v;
		f = fabs(v->x*p->x + v->y*p->y + v->z*p->z + np->p[face].d);
		if (f >= f_min) continue;
		if ( (sg(f,eps_d)) == 0 ) {           //    
			if ( np_point_belog_to_face1(np, p, face, &edge, &vv) != NP_OUT ) return 0.;
		}
		intersect_3d_pl(&np->p[face].v, np->p[face].d, p, &np->p[face].v,
										&pp);
		if ( np_point_belog_to_face1(np, &pp, face, &edge, &vv) == NP_OUT ) {
			f = np_point_dist_to_face(p, np, face, &pp);
		}
		if (f < f_min) { f_min = f; pp_min = pp; }
	}
	*p_min = pp_min;
	return f_min;
}

sgFloat np_point_dist_to_face(lpD_POINT p, lpNPW np, short face,
														 lpD_POINT p_min)
{
	short 		loop;
	int			v1, v2;
	short 		edge, fedge;
	sgFloat 	f, f_min;
	D_POINT	n, pp_min;				 	//  

	loop = np->f[face].fc;
	edge = np->c[loop].fe;
    v1 = np->efr[(int)abs(edge)].bv;
    v2 = np->efr[(int)abs(edge)].ev;
	dpoint_sub(&np->v[v1], &np->v[v2], &n);
	dnormal_vector(&n);
	f_min = dist_point_to_segment( p, &np->v[v1], &np->v[v2], &pp_min);
	do {
		edge = fedge = np->c[loop].fe;
		v1 = BE(edge,np);  v2 = EE(edge,np);
		do {
			dpoint_sub(&np->v[v1], &np->v[v2], &n);
			dnormal_vector(&n);
			f = dist_point_to_segment( p, &np->v[v1], &np->v[v2], &n);
			if ( f < f_min ) { f_min = f; pp_min = n; }
			edge = SL(edge,np); v1 = v2; v2 = EE(edge,np);
		} while (edge != fedge);
		loop = np->c[loop].nc;
	} while (loop > 0);
	*p_min = pp_min;
	return f_min;
}

