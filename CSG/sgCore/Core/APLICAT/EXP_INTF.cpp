#include "../sg.h"

static void  exp_def_pl(lpD_PLANE pl, lpREGION_3D	g);
static BOOL  exp_cut_face_pl(lpNPW np, short face, lpD_PLANE pl,
																 lpNP_VERTEX_LIST ver);
static BOOL  exp_up(lpNPW np, short face, lpNP_VERTEX_LIST ver);


BOOL exp_divide_face(lpNPW np, short face, lpREGION_3D	gab)
{
	NP_VERTEX_LIST ver = {0,0,NULL,NULL};
	D_PLANE        pl;
	BOOL           rt = TRUE;

	if ( !np_ver_alloc(&ver) ) return FALSE;

	exp_def_pl(&pl, gab);     
	if ( !exp_cut_face_pl(np,face,&pl,&ver) ) { rt = FALSE; goto end; }
	if ( !np_fa(np,face) )  rt = FALSE;
end:
	np_free_ver(&ver);
	np_free_stack();
	return rt;
}

static void  exp_def_pl(lpD_PLANE pl, lpREGION_3D	g)
{
	sgFloat    dx, dy, dz;

	pl->v.x = pl->v.y = pl->v.z = 0;

	dx = fabs(g->max.x - g->min.x);
	dy = fabs(g->max.y - g->min.y);
	dz = fabs(g->max.z - g->min.z);
	if (dx > dy) {
		if (dx > dz) { pl->v.x = 1; pl->d = - (g->min.x + dx/2); }
		else         { pl->v.z = 1; pl->d = - (g->min.z + dz/2); }
	} else {
		if (dy > dz) { pl->v.y = 1; pl->d = - (g->min.y + dy/2); }
		else         { pl->v.z = 1; pl->d = - (g->min.z + dz/2); }
	}
}

static BOOL  exp_cut_face_pl(lpNPW np, short face, lpD_PLANE pl, lpNP_VERTEX_LIST ver)
{
	ver->uk = 0;
	np->nov_tmp = np->maxnov;
	if ( !np_pl(np, face, ver, pl) )  return FALSE;
	if (ver->uk == 0) 		return TRUE;	 

	np_axis = np_defc(&np->p[face].v,&pl->v);
	np_sort_ver(np->v, ver, np_axis);
	if ( !exp_up(np,face,ver) ) return FALSE;
	return TRUE;
}

static BOOL  exp_up(lpNPW np, short face, lpNP_VERTEX_LIST ver)
{
	short        p = -1, pr = -1, pt = -1;
	short        ukt = 0;
	short        loop1, loop2;
	short        w1, w2;
	short        edge, last_edge1, last_edge2;
	D_POINT    v1, v2;
	BOOL       le = FALSE;                    
																		 
	while (ukt < ver->uk) {
		np_av(ver->v[ukt].m,&p,&pr,&pt,&le); 
		ukt++;
		if (p == 1) {          // insert edge
			if (ver->v[ukt-1].v > np->nov) {
				v1 = np->v[ver->v[ukt-1].v];
				if ( (w1 = np_new_vertex(np, &v1)) == 0)    return FALSE;
			} else w1 = ver->v[ukt-1].v;
			if (ver->v[ukt-1].m == 1)
				if ((edge=np_divide_edge(np, ver->v[ukt-1].r, w1)) == 0) return FALSE;
			if (ver->v[ukt].v > np->nov) {
				v2 = np->v[ver->v[ukt].v];
				if ( (w2 = np_new_vertex(np, &v2)) == 0)    return FALSE;
			} else w2 = ver->v[ukt].v;
			if (ver->v[ukt].m == 1)
				if ((edge=np_divide_edge(np,ver->v[ukt].r,w2)) == 0) return FALSE;
			if ( !(edge = np_new_edge(np,w1,w2)) ) return FALSE;
			if ( edge < 0 ) continue;
			np->efc[edge].fp = np->efc[edge].fm = face;
			loop1 = np_def_loop(np, face,          edge, &last_edge1);
			loop2 = np_def_loop(np, face, (short)(-edge), &last_edge2);
			np_insert_edge(np,          edge, last_edge1);
			np_insert_edge(np, (short)(-edge), last_edge2);
			if (loop1 == 0 || loop2 == 0) continue;
			if ( !np_an_loops(np, face, loop1, loop2, edge) ) return FALSE;
		}
	}
	return TRUE;
}
