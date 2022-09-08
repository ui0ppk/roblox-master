#include "../sg.h"

BOOL check_loop_area(lpNPW np, short face, sgFloat eps);
//BOOL check_loop_area(lpNPW np, short loop, sgFloat *sq);


/************************************************
 *           --- BOOL	check_model ---
 *	Check vertexes in model with eps precision
 *  and delete bad edges
 ************************************************/
BOOL	check_brep(lpNP_STR_LIST list_str, sgFloat eps)
{
	short     i, face;
	short 		nov, noe, noc;
	NP_STR	str;
	NP_BIT_FACE *bf;

	for ( i = 0; i < list_str->number_all; i++) {
		read_elem(&list_str->vdim, i, &str);
		if (!(read_np(&list_str->bnp, str.hnp, npwg))) return FALSE;

		nov = check_vertex(npwg, eps);
//		if ( nov == npwg->nov ) continue;	// without eq vertexes
		noe = check_edge(npwg);
		noc = check_loop(npwg);

		for ( face = 1; face <= npwg->nof; face++ ) {
			if ( npwg->f[face].fc != 0 )
				if ( check_loop_area( npwg, face, eps_d * eps_d) )	continue;
			bf = (NP_BIT_FACE*)&npwg->f[face].fl;
			bf->met = 0;            // face not on result
		}

		if ( !(np_del_face(npwg)) ) return FALSE;
		if (npwg->nof == 0) {
			str.lab = -1;                     //NP not on result
			return TRUE;
		}

		if ( !(np_del_edge(npwg)) ) return FALSE;
		np_del_vertex(npwg);
		np_del_loop(npwg);

		if ( !(overwrite_np(&list_str->bnp, str.hnp, npwg, TNPW)) )
			return FALSE;
	}
	return TRUE;
}

// check vertexes on NP
// return count of vertexes
short check_vertex(lpNPW np, sgFloat eps)
{
	short  nov;
	short  i, j, edge;
	BOOL lab[MAXNOV];

	memset(&lab, 0, sizeof(lab[0])*MAXNOV);
	nov = np->nov;
	for ( i = 1; i <= np->nov; i++ ) {
		if ( lab[i] ) continue;
		for ( j = i+1; j <= np->nov; j++ ) {
			if ( dpoint_eq(&np->v[i], &np->v[j], eps) ) { 	// vertexes are equals
				nov--;
				lab[j] = TRUE;
				for ( edge = 1; edge <= np->noe; edge++ ) {
					if ( np->efr[edge].bv == j ) np->efr[edge].bv = i;
					if ( np->efr[edge].ev == j ) np->efr[edge].ev = i;
				}
			}
		}
	}
	return nov;
}
// Check edges
// return edges count
short check_edge(lpNPW np)
{
	short noe;
	short edge, edge_n, ledge, jedge;
	short	loop, face;

	noe = np->noe;
	for ( edge = 1; edge <= np->noe; edge++ ) {
		if ( np->efr[edge].bv == np->efr[edge].ev ) {	// begin vertex=end vertex
			noe--;
			if ( np->efc[edge].fp > 0 ) {	// face +
				edge_n = np->efc[edge].ep;
				// find edge
				ledge = jedge = edge; jedge = SL(jedge,np);
				while ( jedge != edge ) {
					ledge = jedge;  jedge = SL(jedge,np);
				}
				if ( ledge > 0 ) np->efc[ ledge].ep  = edge_n;
				else             np->efc[-ledge].em = edge_n;

				face = np->efc[edge].fp;
				loop = np->f[face].fc;
				do {
					if ( np->c[loop].fe == edge ) {
						np->c[loop].fe = edge_n;
						break;
					}
					loop = np->c[loop].nc;
				} while ( loop != 0 );
			}
			if ( np->efc[edge].fm > 0 ) {	// face -
				edge_n = np->efc[edge].em;
				// find edge "-edge"
				ledge = jedge = -edge; jedge = SL(jedge,np);
				while ( jedge != -edge ) {
					ledge = jedge;  jedge = SL(jedge,np);
				}
				if ( ledge > 0 ) np->efc[ ledge].ep = edge_n;
				else             np->efc[-ledge].em = edge_n;

				face = np->efc[edge].fm;
				loop = np->f[face].fc;
				do {
					if ( np->c[loop].fe == -edge ) {
						np->c[loop].fe = edge_n;
						break;
					}
					loop = np->c[loop].nc;
				} while ( loop != 0 );
			}

			memset(&np->efc[edge], 0, sizeof(np->efc[0]));
			memset(&np->efr[edge], 0, sizeof(np->efr[0]));
		}
	}
	return noe;
}


// find bad contours
BOOL check_loop(lpNPW np)
{
	short	noc;
	short edge, f_edge;
	short	loop, loop1, count;
	short face1, face2, face;

	noc = np->noc;
	for ( loop = 1; loop <= np->noc; loop++ ) {
		if ( np->c[loop].fe == 0 ) continue;

		// find contours in circle
		count = 1;
		edge = np->c[loop].fe;
		f_edge = edge; edge = SL(edge,np);
		while ( f_edge != edge && edge != 0) {
			count++;
			edge = SL(edge,np);
		}
		if ( count == 1 ) {			// delete contour
			np->c[loop].fe = 0;
			for (face = 1; face <= np->nof; face++) {
				if ( np->f[face].fc == loop ) {	// first contour
					np->f[face].fc = np->c[loop].nc;
					break;
				} else {
					loop1 = np->f[face].fc;
					while ( loop1 != 0 ) {
						if ( np->c[loop1].nc == loop ) {
							np->c[loop1].nc = np->c[loop].nc;
							break;
						} else
							loop1 = np->c[loop1].nc;
					}
				}
			}
			continue;
		}

		if ( count == 2 ) {	// bad circe - deletet
			noc--;
			f_edge = SL(edge, np);	// edge and f_edge - 2 edges of controur
			face = EF(edge, np);		// face
			if ( edge > 0 ) {
				np->efc[edge].ep = np->efc[edge].fp = 0;
				face1 = np->efc[edge].fm;
			} else {
				np->efc[-edge].em = np->efc[-edge].fm = 0;
				face1 = np->efc[-edge].fp;
			}
			if ( f_edge > 0 ) {
				np->efc[ f_edge].ep = np->efc[ f_edge].fp = 0;
				face2 = np->efc[f_edge].fm;
			} else {
				np->efc[-f_edge].em = np->efc[-f_edge].fm = 0;
				face2 = np->efc[-f_edge].fp;
			}

			// delete contour
			if ( np->f[face].fc == loop ) {	
				np->f[face].fc = np->c[loop].nc;
			} else {                        
				loop1 = np->f[face].fc;
				while ( loop1 != 0 ) {
					if ( np->c[loop1].nc == loop ) {
						np->c[loop1].nc = np->c[loop].nc;
						break;
					} else
						loop1 = np->c[loop1].nc;
				}
			}
			np->c[loop].fe = 0;

			
		}
	}
	return noc;
}
// check face
//  FALSE - face with 0 square
BOOL check_loop_area(lpNPW np, short face, sgFloat eps)
{
	short     v;
	short     e1, e2, edge_cnt;
	short     contur, contur_cnt;
//	short     ip;
	sgFloat	cs;
	D_POINT c, r, a, b, d;

	c.x = c.y = c.z = 0.;
	contur = np->f[face].fc;
	if (contur <= 0 || contur > np->noc) return FALSE;
    int indxxx = abs(np->c[contur].fe);
    v = np->efr[indxxx].ev;
	if (v <= 0 || v > np->nov) return FALSE;
	r = np->v[v];
	contur_cnt = 0;

	while (contur > 0) {                  // for each contour
		if (++contur_cnt > np->noc) return FALSE;
		e1 = np->c[contur].fe;
		if (e1 == 0 || abs(e1) > np->noe) return FALSE;
		v = BE(e1,np);                             // v - begin of e1
		if (v <= 0 || v > np->nov) return FALSE;
		dpoint_sub(&(np->v[v]), &r, &b);
		e2 = e1;
		edge_cnt = 0;

		while (TRUE) {                       // for each contour
			if (++edge_cnt > np->noe) return FALSE;
			a = b;
			v = EE(e2,np);                            //  v - end of e2
			e2 = SL(e2,np);
			if (e2 == 0 || abs(e2) > np->noe ||
					v  <= 0 ||       v > np->nov ) return FALSE;
			dpoint_sub(&(np->v[v]), &r, &b);
			dvector_product(&a, &b, &d);
			c.x = c.x + d.x;
			c.y = c.y + d.y;
			c.z = c.z + d.z;
			if ( e2 == e1) break;
		}

		contur = np->c[contur].nc;
		if (contur > np->noc || contur < 0) return FALSE;
		if (contur == 0) break;
	}
	cs = sqrt(c.x *c.x + c.y * c.y + c.z*c.z);
	if ( cs < eps ) return FALSE;
	return TRUE;
}

