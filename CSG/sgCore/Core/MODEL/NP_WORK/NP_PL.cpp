#include "../../sg.h"

#define MAXVER    100
short np_axis;

typedef int (*fptr)(const void*, const void*);
static int sort_function( const short *a, const short *b);
static lpDA_POINT     wv;

void np_def_point(lpD_PLANE p1, lpD_PLANE p2, lpD_POINT c, lpD_POINT pc);

typedef struct {
	short  uk;                         // -   
	short  maxuk;                      //  
	VADR hedge;                      //   
	short  * edge;                  //  
} NP_STACK_EDGE;

NP_STACK_EDGE np_stack={0,0,NULL,NULL};

short  np_get_stack(void);
BOOL np_init_stack(void);
BOOL np_put_stack(short edge);
BOOL np_put_list(short v, short m, short r, lpNP_VERTEX_LIST ver);
short  np_vper(short v1, short v2, sgFloat f1, sgFloat f2, lpNPW np,
						 lpNP_VERTEX_LIST ver1);

BOOL np_pl(lpNPW np1, short face1, lpNP_VERTEX_LIST ver1,lpD_PLANE pl)
{

#define DIST(i) (np1->v[i].x*ka + np1->v[i].y*kb + np1->v[i].z*kc + kd)

	short   loop;
	short   fedge,edge,edget,edgel;
	short   v1,v2,vt;
	short   p1,p2;
	short   label;
	sgFloat 	ka,kb,kc,kd;
	sgFloat 	f1,f2;
	BOOL   	rt = TRUE; //, pr = FALSE;
	D_POINT	pc, c, n;

//	ka = pl->v.x;  kb = pl->v.y;  kc = pl->v.z;	kd = pl->d;

	f1 = dskalar_product(&np1->p[face1].v, &pl->v);	//    ,     
	if ( fabs(f1) > 0.5 ) {
		np_def_point(&np1->p[face1], pl, &c, &pc);	//    
		dvector_product(&c, &np1->p[face1].v, &n);	//    ,     
		dnormal_vector(&n);
		ka = n.x;  kb = n.y;  kc = n.z;
		kd = dcoef_pl(&pc, &n);
	} else {
		ka = pl->v.x;  kb = pl->v.y;  kc = pl->v.z;	kd = pl->d;
	}

//first:
	loop = np1->f[face1].fc;
	do {
		fedge = np1->c[loop].fe;
		v1 = BE(fedge,np1);  edge = fedge;    f1 = DIST(v1);
		do {
			if (fabs(f1) > eps_d) goto mt;
			edge = SL(edge,np1); v1 = BE(edge,np1); f1 = DIST(v1);
		} while (edge != fedge);
 //   :   face1    pl
//    np_handler_err(NP_LOOP_IN_PLANE);
//		return FALSE;
		return TRUE;

/*		if ( pr )	return TRUE;	//   
		else {                  //     
			pr = TRUE;
			f1 = dskalar_product(&np1->p[face1].v, &pl->v);	//    ,     
			if ( fabs(f1) > 0.9 ) {
				np_def_point(&np1->p[face1], pl, &c, &pc);	//    
				dvector_product(&c, &np1->p[face1].v, &n);	//    ,     
				dnormal_vector(&n);
				ka = n.x;  kb = n.y;  kc = n.z;
				kd = dcoef_pl(&pc, &n);
			}
			goto first;
		}
*/
mt: fedge = edge;
		p1 = sg(f1,eps_d);
		do {
//cnt:
			v2 = EE(edge,np1);  f2 = DIST(v2);
      p2 = sg(f2,eps_d);
			if (p1 == p2) {                       //   
				v1 = v2;    f1 = f2;    p1 = p2;
				edge = SL(edge,np1);
				continue;
			}
      if (p1 == -p2) {                         //  
        if ( (vt = np_vper(v1,v2,f1,f2,np1,ver1)) == 0) { rt = FALSE; goto end; }
        if ( !(np_put_list(vt,1,edge,ver1)) ) { rt = FALSE; goto end; }
        v1 = v2;  f1 = f2;  p1 = p2;
        edge = SL(edge,np1);
				continue;
			}

/*			if ( !pr ) {	//   
				pr = TRUE;
				f1 = dskalar_product(&np1->p[face1].v, &pl->v);	//    ,     
				if ( fabs(f1) > 0.9 ) {
					np_def_point(&np1->p[face1], pl, &c, &pc);	//    
					dvector_product(&c, &np1->p[face1].v, &n);	//    ,     
					dnormal_vector(&n);
					ka = n.x;  kb = n.y;  kc = n.z;
					kd = dcoef_pl(&pc, &n);
					goto cnt;
				}
			}
*/
      if ( !(np_init_stack()) ) { rt = FALSE; goto end; }
			do {
        if ( !(np_put_stack(edge)) ) { rt = FALSE; goto end; }
        edge = SL(edge,np1);
        v2 = EE(edge,np1);  f2 = DIST(v2);
        p2 = sg(f2,eps_d);
			} while (p2 == 0);
      if (np_stack.uk == 1) {                     //    
        if (p1 == p2)  label = -4;           // 
        else           label = -1;           // 
        edget = np_get_stack();   vt = EE(edget,np1);
        if ( !(np_put_list(vt,label,edget,ver1)) ) { rt = FALSE; goto end; }
        v1 = v2;  p1 = p2;  f1 = f2;
        edge = SL(edge,np1);
				continue;
			}
			if (p2 == p1) label=2;
			else          label=3;
      edget = np_get_stack();   vt = EE(edget,np1);
      if ( !(np_put_list(vt,label,edget,ver1)) ) { rt = FALSE; goto end; }
      edgel = np_get_stack();
      while ( (edget = np_get_stack()) != 0 ) {
        vt = EE(edgel,np1);
        if ( !(np_put_list(vt,4,edgel,ver1)) ) { rt = FALSE; goto end; }
				edgel = edget;
			}
      vt = EE(edgel,np1);
      if ( !(np_put_list(vt,label,edgel,ver1)) ) { rt = FALSE; goto end; }
      v1 = v2;  f1 = f2;  p1 = p2;
      edge = SL(edge,np1);
    } while (edge != fedge);
    loop = np1->c[loop].nc;
  } while (loop > 0);
end:
	if (np_stack.edge != NULL) np_stack.edge = NULL;
  return rt;
}

short np_vper(short v1, short v2, sgFloat f1, sgFloat f2, lpNPW np,
					  lpNP_VERTEX_LIST ver)
{
	short   nv, i;
  sgFloat r;

  if (np->nov == np->nov_tmp) {
		if ( !(o_expand_np(np,MAXNOV/4,0,0,0)) )   return 0;
		for (i = 0 ; i < ver->uk ; i++) {
			if ( ver->v[i].v > np->nov ) ver->v[i].v += MAXNOV/4;
		}
	}
	nv = np->nov_tmp;
  r = fabs(f1)/(fabs(f2) + fabs(f1));
  np->v[nv].x = np->v[v1].x + (np->v[v2].x - np->v[v1].x)*r;
  np->v[nv].y = np->v[v1].y + (np->v[v2].y - np->v[v1].y)*r;
  np->v[nv].z = np->v[v1].z + (np->v[v2].z - np->v[v1].z)*r;
	np->nov_tmp--;
	return nv;
}

BOOL np_put_list(short v, short m, short r, lpNP_VERTEX_LIST ver)
{
	short uk;

  if (ver->uk == ver->maxuk)              //   
		if ( !(np_ver_alloc(ver)) ) return FALSE;
	uk = ver->uk++;
	ver->v[uk].m = m;
	ver->v[uk].v = v;
	ver->v[uk].r = r;
  return TRUE;
}

short np_defc(lpD_POINT p1, lpD_POINT p2)
{
	D_POINT c;

	dvector_product(p1, p2, &c);
	c.x = fabs(c.x);
	c.y = fabs(c.y);
	c.z = fabs(c.z);
	if (c.x <= c.y)
		if (c.y <= c.z)  return 2;
		else             return 1;
	else
		if (c.x <= c.z)  return 2;
		else             return 0;
}

void np_av(short mt, short *p,short *pr,short *pt, BOOL *le)
{
  if (abs(mt) == 1) {
		*p=-*p;
		return;
	}
	if (abs(mt) == 4) return;
	if (mt == 2) {
		if (*pr <  0) {   /*    */
			*pt=*p;  *p=1;  *pr=1;
			*le = TRUE;
			return;
    } else {         /*     */
			*p=*pt;  *pr=-1;
			*le = FALSE;
			return;
		}
	} else {  /* mt=3 */
		if (*pr < 0) {   /*    */
			*pt=*p;  *p=1;  *pr=1;
			*le = TRUE;
			return;
		} else {       /*     */
			*p=-*pt;  *pr=-1;
			*le = FALSE;
			return;
		}
	}
}

BOOL np_ver_alloc(lpNP_VERTEX_LIST ver)
{
	VADR   hv;//nb = NULL;
	lpNP_VERTEX  v;//nb = NULL;

	if (ver->maxuk == 0) {
		if ( (ver->hv = SGMalloc(sizeof(NP_VERTEX)*MAXVER)) == NULL) 	return FALSE;
		ver->v = (lpNP_VERTEX)ver->hv;
		ver->maxuk = MAXVER;
		ver->uk = 0;
		return TRUE;
	}

	ver->maxuk += MAXVER;
	if ( (hv = SGMalloc(sizeof(NP_VERTEX)*ver->maxuk)) == NULL) 		return FALSE;
	v = (lpNP_VERTEX)hv;
	memcpy(v,&(ver->v),sizeof(NP_VERTEX)*(ver->maxuk - MAXVER));
	SGFree (ver->hv);
	ver->hv = hv;
	ver->v  = v;
  return TRUE;
}
void np_free_ver(lpNP_VERTEX_LIST ver)
{
	if (ver->v != NULL)  { ver->v = NULL; }
	if (ver->hv != NULL) { SGFree(ver->hv); ver->hv = NULL; }
  ver->uk = 0;
  ver->maxuk = 0;
}
void np_sort_ver(lpD_POINT v, lpNP_VERTEX_LIST ver, short axis)
{
	np_axis = axis;
  wv = (lpDA_POINT)v;
 	qsort((void*)ver->v,ver->uk,sizeof(NP_VERTEX),(fptr)sort_function);
}
int sort_function( const short *a, const short *b)
{
	 if ( wv[(*(NP_VERTEX*)a).v][np_axis] > wv[(*(NP_VERTEX*)b).v][np_axis]) return 1;
	 return -1;
}
BOOL np_put_stack(short r)
{
	short * edge;//nb = NULL;
	VADR hedge;//nb = NULL;

	if (np_stack.uk == np_stack.maxuk) {                // 
		np_stack.maxuk += 10;
		if ( (hedge = SGMalloc(sizeof(short)*np_stack.maxuk)) == NULL) return FALSE;
		if (np_stack.edge == NULL) np_stack.edge = (short*)np_stack.hedge;
		edge = (short*)hedge;
		memcpy(edge,np_stack.edge,sizeof(short)*(np_stack.maxuk - 10));
		SGFree(np_stack.hedge);
		np_stack.hedge = hedge;
		np_stack.edge  = edge;
	}
	np_stack.edge[np_stack.uk] = r;
	np_stack.uk++;
	return TRUE;
}
BOOL np_init_stack(void)
{
  if (np_stack.maxuk == 0) {                  //  
    if ( (np_stack.hedge = SGMalloc(sizeof(short)*10)) == NULL) return FALSE;
    np_stack.maxuk = 10;
  }
	if ( !np_stack.edge ) np_stack.edge = (short*)np_stack.hedge;
	np_stack.uk = 0;
  return TRUE;
}
short np_get_stack(void)
{
  np_stack.uk--;
  if (np_stack.uk < 0) return 0;
  return np_stack.edge[np_stack.uk];
}
void np_free_stack(void)
{
	if (np_stack.edge != NULL) {
		np_stack.edge = NULL;
	}
	if (np_stack.hedge != NULL) {
		SGFree(np_stack.hedge);
		np_stack.hedge = NULL;
	}
	np_stack.uk = np_stack.maxuk = 0;
}

void np_def_point(lpD_PLANE p1, lpD_PLANE p2, lpD_POINT c, lpD_POINT pc)
{
	sgFloat	*a[2], a1[2], a2[2], b[2];
	short		num;

	a[0] = a1;
	a[1] = a2;
	dvector_product(&p1->v, &p2->v, c);	//     
	dnormal_vector(c);

	if ( fabs(c->x) > fabs(c->y) ) {
		if ( fabs(c->x) > fabs(c->z) )  {
			a1[0] =  p1->v.y;			a2[0] =  p2->v.y;
			a1[1] =  p1->v.z;    	a2[1] =  p2->v.z;
			b[0]	= -p1->d;      	b[1] 	= -p2->d;
			num = 1;
		}	else {
			a1[0] =  p1->v.x;			a2[0] =  p2->v.x;
			a1[1] =  p1->v.y;    	a2[1] =  p2->v.y;
			b[0]	= -p1->d;      	b[1] 	= -p2->d;
			num = 3;
		}
	} else {
		if ( fabs(c->y) > fabs(c->z) )  {
			a1[0] =  p1->v.x;			a2[0] =  p2->v.x;
			a1[1] =  p1->v.z;    	a2[1] =  p2->v.z;
			b[0]	= -p1->d;      	b[1] 	= -p2->d;
			num = 2;
		}	else {
			a1[0] =  p1->v.x;			a2[0] =  p2->v.x;
			a1[1] =  p1->v.y;    	a2[1] =  p2->v.y;
			b[0]	= -p1->d;      	b[1] 	= -p2->d;
			num = 3;
		}
	}

	gauss_classic( a, b, 2 );
	switch ( num ) {
		case 1:
			pc->x = 0;	pc->y = b[0]; 	pc->z = b[1];
			break;
		case 2:
			pc->x = b[0];	pc->y = 0; 	pc->z = b[1];
			break;
		case 3:
			pc->x = b[0];	pc->y = b[1]; 	pc->z = 0;
			break;
	}
}

