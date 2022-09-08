#include "../sg.h"

typedef struct {
	short loop;
	int	edge;
	sgFloat min;
} LOOP_DATA;

typedef LOOP_DATA 		* lpLOOP_DATA;

BOOL exp_divide_face(lpNPW np, short face, lpREGION_3D	gab);

static BOOL check_face(lpNPW np, short face, lpREGION_3D	gab, short num_ver);
typedef int (*fptr)(const void*, const void*);
static int sort_function( const short *a, const short *b);
static short check_defc(lpD_POINT n);
static BOOL add_dummy_edge(lpNPW np, short face);
static short search_near_edge(lpNPW np, short loop, lpD_POINT point1,
														lpD_POINT point2);


BOOL check_np_new(lpNPW np, short num_ver)
{
	short			face, loop;
	REGION_3D	gab = {{1.e35,1.e35,1.e35},{-1.e35,-1.e35,-1.e35}};
	BOOL			rt = TRUE;

	for (face = 1; face <= np->nof; face++) {
		loop = np->f[face].fc;
		if ( np->c[loop].nc > 0 ) {  
			if ( !add_dummy_edge(np, face) ) return FALSE;
		}
		if ( !check_face(np, face, &gab, num_ver) ) {      
			if ( !(exp_divide_face(np, face, &gab)) ) return FALSE;
			face--;
		}
	}

//nb end:
	return rt;
}

static BOOL add_dummy_edge(lpNPW np, short face)
{
	lpLOOP_DATA loops;
	short				i, il = 0;				
	short				loop, out_loop, last_loop;
	short				fedge, edge, edge1, edge2, new_edge;
	short				v1, v2, nov;
	short				axis;
	sgFloat			sq;
	lpDA_POINT	w;
	D_POINT			point1, point2;
	BOOL				rt = TRUE;

	if ((loops = (LOOP_DATA*)SGMalloc(sizeof(LOOP_DATA)*(np->noc))) == NULL) return FALSE;

	axis = check_defc(&(np->p[face].v));
	w = (lpDA_POINT)np->v;
	loop = np->f[face].fc;
	last_loop = 0;
	do {                                       
		np_ort_loop(np,face,loop,&sq);
		if (sq  > 0) { 							 
			out_loop = loop;
			loop = np->c[loop].nc;
//			fe = np->c[out_loop].fe;
			
			if (last_loop == 0) {       
				np->f[face].fc = loop;
			} else {                    
				np->c[last_loop].nc = loop;
			}
			continue;
		}
		
		loops[il].loop = loop;
		
		fedge = edge = np->c[loop].fe;
		v1 = EE(edge,np);
		loops[il].min = w[v1][axis];
		loops[il].edge = edge;
		do {
			if ( w[v1][axis] < loops[il].min ) {
				loops[il].min = w[v1][axis];
				loops[il].edge = edge;
			}
			edge = SL(edge,np);  v1 = EE(edge,np);
		} while (edge != fedge);
		last_loop = loop;
		loop = np->c[loop].nc; il++;
	} while (loop != 0);
	
	qsort((void*)loops,il,sizeof(LOOP_DATA),(fptr)sort_function);

	for (i = 0; i < il; i++) {     
		edge1 = loops[i].edge;
		v1 = EE(edge1,np);
		point1 = np->v[v1];
		edge2 = search_near_edge(np, out_loop, &point1, &point2);
		nov = np->nov;
		v2 = np_new_vertex(np, &point2);     
		if ( v2 == 0 ) { rt = FALSE; goto end; }
		if ( v2 > nov ) { 
			if ( !(new_edge = np_divide_edge(np, edge2, v2)) ) { rt = FALSE; goto end; }
			if ( edge2 < 0 ) edge2 = -new_edge;
		}
		if ( !(new_edge = np_new_edge(np,v2,v1)) )  
			{ rt = FALSE; goto end; }
    np->efc[new_edge].fp = np->efc[new_edge].fm = face;
		np_insert_edge(np, new_edge, edge2);
		np_insert_edge(np,(short)(-new_edge), edge1);
//		np->c[out_loop].nc = np->c[loops[i].loop].nc;
//		np->c[loops[i].loop].fe = 0;
	}
	np->f[face].fc = out_loop;
	np->c[out_loop].nc = 0;

end:
	if (loops) { SGFree(loops); /*loops = NULL; */}
	return rt;
}

static short search_near_edge(lpNPW np, short loop, lpD_POINT p,
														lpD_POINT p_min)
{
	short 			v1, v2;
	short 			fedge, edge, edge_min;
	sgFloat 		f, f_min = 1e35;
	lpD_POINT p1, p2;
	D_POINT		pp, pp_min;

	fedge = edge = np->c[loop].fe;
	v1 = BE(edge,np);  p1 = &np->v[v1];
	do {
		v2 = EE(edge,np);   p2 = &np->v[v2];
		f = dist_point_to_segment( p, p1, p2,	&pp);
		if ( f < f_min ) {
			f_min = f;
			pp_min = pp;
			edge_min = edge;
		}
		edge = SL(edge,np);  /*nb v1 = v2;*/    p1 = p2;
	} while (edge != fedge);

	*p_min = pp_min;
	return edge_min;
}
int sort_function( const short *a, const short *b)
{
	 if ( ((lpLOOP_DATA)a)->min > ((lpLOOP_DATA)b)->min) return 1;
	 return -1;
}

static BOOL check_face(lpNPW np, short face, lpREGION_3D	gab, short num_ver)
{
	short loop, v;
	short ver = 0;             
	short edge, fedge;

	gab->min.x = gab->min.y = gab->min.z =  1.e35;
	gab->max.x = gab->max.y = gab->max.z = -1.e35;

	loop = np->f[face].fc;
	do {                                       
		fedge = edge = np->c[loop].fe;
		v = BE(edge,np);
		do {                                       
			if (np->v[v].x < gab->min.x) gab->min.x = np->v[v].x;
			if (np->v[v].y < gab->min.y) gab->min.y = np->v[v].y;
			if (np->v[v].z < gab->min.z) gab->min.z = np->v[v].z;
			if (np->v[v].x > gab->max.x) gab->max.x = np->v[v].x;
			if (np->v[v].y > gab->max.y) gab->max.y = np->v[v].y;
			if (np->v[v].z > gab->max.z) gab->max.z = np->v[v].z;
			ver++;
			edge = SL(edge,np);  v = BE(edge,np);
		} while (edge != fedge);
		loop = np->c[loop].nc;
	} while (loop != 0);

	if (ver < num_ver) 	return TRUE;
	else           			return FALSE;
}

static short check_defc(lpD_POINT n)
{
	D_POINT	c;

	c = *n;
	c.x = fabs(c.x);
	c.y = fabs(c.y);
	c.z = fabs(c.z);
	if (c.x <= c.y)
		if (c.x <= c.z)  return 0;
		else             return 2;
	else
		if (c.y <= c.z)  return 1;
		else             return 2;
}

