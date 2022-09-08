
#include "../../sg.h"
#include "Delone.h"

#define  TRILIBRARY

#pragma warning(disable:4311)
#pragma warning(disable:4312)


#define TRIPERBLOCK 4092           /* Number of triangles allocated at once. */
#define SUBSEGPERBLOCK 508       /* Number of subsegments allocated at once. */
#define VERTEXPERBLOCK 4092         /* Number of vertices allocated at once. */
#define VIRUSPERBLOCK 1020   /* Number of virus triangles allocated at once. */
/* Number of encroached subsegments allocated at once. */
#define BADSUBSEGPERBLOCK 252
/* Number of skinny triangles allocated at once. */
#define BADTRIPERBLOCK 4092
/* Number of flipped triangles allocated at once. */
#define FLIPSTACKERPERBLOCK 252
/* Number of splay tree nodes allocated at once. */
#define SPLAYNODEPERBLOCK 508

#define INPUTVERTEX 0
#define SEGMENTVERTEX 1
#define FREEVERTEX 2
#define DEADVERTEX -32768
#define UNDEADVERTEX -32767

#define SAMPLEFACTOR 11


#define SAMPLERATE 10

#define SQUAREROOTTWO 1.4142135623730950
#define ONETHIRD 0.33333333333333


struct triangulateio 
{
	sgFloat *pointlist;                                               /* In / out */
	sgFloat *pointattributelist;                                      /* In / out */
	int *pointmarkerlist;                                          /* In / out */
	int numberofpoints;                                            /* In / out */
	int numberofpointattributes;                                   /* In / out */

	int *trianglelist;                                             /* In / out */
	sgFloat *triangleattributelist;                                   /* In / out */
	sgFloat *trianglearealist;                                         /* In only */
	int *neighborlist;                                             /* Out only */
	int numberoftriangles;                                         /* In / out */
	int numberofcorners;                                           /* In / out */
	int numberoftriangleattributes;                                /* In / out */

	int *segmentlist;                                              /* In / out */
	int *segmentmarkerlist;                                        /* In / out */
	int numberofsegments;                                          /* In / out */

	sgFloat *holelist;                        /* In / pointer to array copied out */
	int numberofholes;                                      /* In / copied out */

	sgFloat *regionlist;                      /* In / pointer to array copied out */
	int numberofregions;                                    /* In / copied out */

	int *edgelist;                                                 /* Out only */
	int *edgemarkerlist;            /* Not used with Voronoi diagram; out only */
	sgFloat *normlist;                /* Used only with Voronoi diagram; out only */
	int numberofedges;                                             /* Out only */
};

void triangulate(char *, struct triangulateio *, struct triangulateio *,
struct triangulateio *);

void reportTr(struct triangulateio *io,
			  int markers,
			  int reporttriangles,
			  int reportneighbors,
			  int reportsegments,
			  int reportedges,
			  int reportnorms);



enum wordtype {POINTER, FLOATINGPOINT};
enum locateresult {INTRIANGLE, ONEDGE, ONVERTEX, OUTSIDE};
enum insertvertexresult {SUCCESSFULVERTEX, ENCROACHINGVERTEX, VIOLATINGVERTEX,
                         DUPLICATEVERTEX};
enum finddirectionresult {WITHIN, LEFTCOLLINEAR, RIGHTCOLLINEAR};

typedef sgFloat **triangle;            /* Really:  typedef triangle *triangle   */

struct otri {
  triangle *tri;
  int orient;                                         /* Ranges from 0 to 2. */
};


typedef sgFloat **subseg;                  /* Really:  typedef subseg *subseg   */

struct osub {
  subseg *ss;
  int ssorient;                                       /* Ranges from 0 to 1. */
};


typedef sgFloat *vertex;

struct badsubseg {
  subseg encsubseg;                             /* An encroached subsegment. */
  vertex subsegorg, subsegdest;                         /* Its two vertices. */
};

struct badtriang {
  triangle poortri;                       /* A skinny or too-large triangle. */
  sgFloat key;                             /* cos^2 of smallest (apical) angle. */
  vertex triangorg, triangdest, triangapex;           /* Its three vertices. */
  struct badtriang *nexttriang;             /* Pointer to next bad triangle. */
};

struct flipstacker {
  triangle flippedtri;                       /* A recently flipped triangle. */
  struct flipstacker *prevflip;               /* Previous flip in the stack. */
};

struct event {
  sgFloat xkey, ykey;                              /* Coordinates of the event. */
  int *eventptr;      /* Can be a vertex or the location of a circle event. */
  int heapposition;              /* Marks this event's position in the heap. */
};

struct splaynode {
  struct otri keyedge;                     /* Lprev of an edge on the front. */
  vertex keydest;           /* Used to verify that splay node is still live. */
  struct splaynode *lchild, *rchild;              /* Children in splay tree. */
};

struct memorypool {
  int **firstblock, **nowblock;
  int *nextitem;
  int *deaditemstack;
  int **pathblock;
  int *pathitem;
  enum wordtype itemwordtype;
  int alignbytes;
  int itembytes, itemwords;
  int itemsperblock;
  int itemsfirstblock;
  long items, maxitems;
  int unallocateditems;
  int pathitemsleft;
};


/* Global constants.                                                         */

sgFloat splitter;       /* Used to split sgFloat factors for exact multiplication. */
sgFloat epsilon;                             /* Floating-point machine epsilon. */
sgFloat resulterrbound;
sgFloat ccwerrboundA, ccwerrboundB, ccwerrboundC;
sgFloat iccerrboundA, iccerrboundB, iccerrboundC;
sgFloat o3derrboundA, o3derrboundB, o3derrboundC;

/* Random number seed is not constant, but I've made it global anyway.       */

unsigned long randomseed;                     /* Current random number seed. */

struct mesh {
  struct memorypool triangles;
  struct memorypool subsegs;
  struct memorypool vertices;
  struct memorypool viri;
  struct memorypool badsubsegs;
  struct memorypool badtriangles;
  struct memorypool flipstackers;
  struct memorypool splaynodes;

/* Variables that maintain the bad triangle queues.  The queues are          */
/*   ordered from 63 (highest priority) to 0 (lowest priority).              */

  struct badtriang *queuefront[64];
  struct badtriang *queuetail[64];
  int nextnonemptyq[64];
  int firstnonemptyq;

/* Variable that maintains the stack of recently flipped triangles.          */

  struct flipstacker *lastflip;

/* Other variables. */

  sgFloat xmin, xmax, ymin, ymax;                            /* x and y bounds. */
  sgFloat xminextreme;      /* Nonexistent x value used as a flag in sweepline. */
  int invertices;                               /* Number of input vertices. */
  int inelements;                              /* Number of input triangles. */
  int insegments;                               /* Number of input segments. */
  int holes;                                       /* Number of input holes. */
  int regions;                                   /* Number of input regions. */
  int undeads;    /* Number of input vertices that don't appear in the mesh. */
  long edges;                                     /* Number of output edges. */
  int mesh_dim;                                /* Dimension (ought to be 2). */
  int nextras;                           /* Number of attributes per vertex. */
  int eextras;                         /* Number of attributes per triangle. */
  long hullsize;                          /* Number of edges in convex hull. */
  int steinerleft;                 /* Number of Steiner points not yet used. */
  int vertexmarkindex;         /* Index to find boundary marker of a vertex. */
  int vertex2triindex;     /* Index to find a triangle adjacent to a vertex. */
  int highorderindex;  /* Index to find extra nodes for high-order elements. */
  int elemattribindex;            /* Index to find attributes of a triangle. */
  int areaboundindex;             /* Index to find area bound of a triangle. */
  int checksegments;         /* Are there segments in the triangulation yet? */
  int checkquality;                  /* Has quality triangulation begun yet? */
  int readnodefile;                           /* Has a .node file been read? */
  long samples;              /* Number of random samples for point location. */

  long incirclecount;                 /* Number of incircle tests performed. */
  long counterclockcount;     /* Number of counterclockwise tests performed. */
  long orient3dcount;           /* Number of 3D orientation tests performed. */
  long hyperbolacount;      /* Number of right-of-hyperbola tests performed. */
  long circumcentercount;  /* Number of circumcenter calculations performed. */
  long circletopcount;       /* Number of circle top calculations performed. */

/* Triangular bounding box vertices.                                         */

  vertex infvertex1, infvertex2, infvertex3;

/* Pointer to the `triangle' that occupies all of "outer space."             */

  triangle *dummytri;
  triangle *dummytribase;    /* Keep base address so we can free() it later. */

/* Pointer to the omnipresent subsegment.  Referenced by any triangle or     */
/*   subsegment that isn't really connected to a subsegment at that          */
/*   location.                                                               */

  subseg *dummysub;
  subseg *dummysubbase;      /* Keep base address so we can free() it later. */

/* Pointer to a recently visited triangle.  Improves point location if       */
/*   proximate vertices are inserted sequentially.                           */

  struct otri recenttri;

};                                                  /* End of `struct mesh'. */


/* Data structure for command line switches and file names.  This structure  */
/*   is used (instead of global variables) to allow reentrancy.              */

struct behavior {

  int poly, refine, quality, vararea, fixedarea, usertest;
  int regionattrib, convex, weighted, jettison;
  int firstnumber;
  int edgesout, voronoi, neighbors, geomview;
  int nobound, nopolywritten, nonodewritten, noelewritten, noiterationnum;
  int noholes, noexact, nolenses;
  int incremental, sweepline, dwyer;
  int splitseg;
  int docheck;
  int quiet, verbose;
  int usesegments;
  int order;
  int nobisect;
  int steiner;
  sgFloat minangle, goodangle, offconstant;
  sgFloat maxarea;
};                                              /* End of `struct behavior'. */

int plus1mod3[3] = {1, 2, 0};
int minus1mod3[3] = {2, 0, 1};


#define decode(ptr, otri)                                                     \
  (otri).orient = (int) ((unsigned long) (ptr) & (unsigned long) 3l);         \
  (otri).tri = (triangle *)                                                   \
                  ((unsigned long) (ptr) ^ (unsigned long) (otri).orient)


#define encode(otri)                                                          \
  (triangle) ((unsigned long) (otri).tri | (unsigned long) (otri).orient)


#define sym(otri1, otri2)                                                     \
  ptr = (otri1).tri[(otri1).orient];                                          \
  decode(ptr, otri2);

#define symself(otri)                                                         \
  ptr = (otri).tri[(otri).orient];                                            \
  decode(ptr, otri);

/* lnext() finds the next edge (counterclockwise) of a triangle.             */

#define lnext(otri1, otri2)                                                   \
  (otri2).tri = (otri1).tri;                                                  \
  (otri2).orient = plus1mod3[(otri1).orient]

#define lnextself(otri)                                                       \
  (otri).orient = plus1mod3[(otri).orient]

/* lprev() finds the previous edge (clockwise) of a triangle.                */

#define lprev(otri1, otri2)                                                   \
  (otri2).tri = (otri1).tri;                                                  \
  (otri2).orient = minus1mod3[(otri1).orient]

#define lprevself(otri)                                                       \
  (otri).orient = minus1mod3[(otri).orient]

/* onext() spins counterclockwise around a vertex; that is, it finds the     */
/*   next edge with the same origin in the counterclockwise direction.  This */
/*   edge is part of a different triangle.                                   */

#define onext(otri1, otri2)                                                   \
  lprev(otri1, otri2);                                                        \
  symself(otri2);

#define onextself(otri)                                                       \
  lprevself(otri);                                                            \
  symself(otri);

/* oprev() spins clockwise around a vertex; that is, it finds the next edge  */
/*   with the same origin in the clockwise direction.  This edge is part of  */
/*   a different triangle.                                                   */

#define oprev(otri1, otri2)                                                   \
  sym(otri1, otri2);                                                          \
  lnextself(otri2);

#define oprevself(otri)                                                       \
  symself(otri);                                                              \
  lnextself(otri);

#define dnext(otri1, otri2)                                                   \
  sym(otri1, otri2);                                                          \
  lprevself(otri2);

#define dnextself(otri)                                                       \
  symself(otri);                                                              \
  lprevself(otri);


#define dprev(otri1, otri2)                                                   \
  lnext(otri1, otri2);                                                        \
  symself(otri2);

#define dprevself(otri)                                                       \
  lnextself(otri);                                                            \
  symself(otri);

#define rnext(otri1, otri2)                                                   \
  sym(otri1, otri2);                                                          \
  lnextself(otri2);                                                           \
  symself(otri2);

#define rnextself(otri)                                                       \
  symself(otri);                                                              \
  lnextself(otri);                                                            \
  symself(otri);

#define rprev(otri1, otri2)                                                   \
  sym(otri1, otri2);                                                          \
  lprevself(otri2);                                                           \
  symself(otri2);

#define rprevself(otri)                                                       \
  symself(otri);                                                              \
  lprevself(otri);                                                            \
  symself(otri);

/* These primitives determine or set the origin, destination, or apex of a   */
/* triangle.                                                                 */

#define org(otri, vertexptr)                                                  \
  vertexptr = (vertex) (otri).tri[plus1mod3[(otri).orient] + 3]

#define dest(otri, vertexptr)                                                 \
  vertexptr = (vertex) (otri).tri[minus1mod3[(otri).orient] + 3]

#define apex(otri, vertexptr)                                                 \
  vertexptr = (vertex) (otri).tri[(otri).orient + 3]

#define setorg(otri, vertexptr)                                               \
  (otri).tri[plus1mod3[(otri).orient] + 3] = (triangle) vertexptr

#define setdest(otri, vertexptr)                                              \
  (otri).tri[minus1mod3[(otri).orient] + 3] = (triangle) vertexptr

#define setapex(otri, vertexptr)                                              \
  (otri).tri[(otri).orient + 3] = (triangle) vertexptr

/* Bond two triangles together.                                              */

#define bond(otri1, otri2)                                                    \
  (otri1).tri[(otri1).orient] = encode(otri2);                                \
  (otri2).tri[(otri2).orient] = encode(otri1)

#define dissolve(otri)                                                        \
  (otri).tri[(otri).orient] = (triangle) m->dummytri

#define otricopy(otri1, otri2)                                                \
  (otri2).tri = (otri1).tri;                                                  \
  (otri2).orient = (otri1).orient

#define otriequal(otri1, otri2)                                               \
  (((otri1).tri == (otri2).tri) &&                                            \
   ((otri1).orient == (otri2).orient))

#define infect(otri)                                                          \
  (otri).tri[6] = (triangle)                                                  \
                    ((unsigned long) (otri).tri[6] | (unsigned long) 2l)

#define uninfect(otri)                                                        \
  (otri).tri[6] = (triangle)                                                  \
                    ((unsigned long) (otri).tri[6] & ~ (unsigned long) 2l)

#define infected(otri)                                                        \
  (((unsigned long) (otri).tri[6] & (unsigned long) 2l) != 0l)

#define elemattribute(otri, attnum)                                           \
  ((sgFloat *) (otri).tri)[m->elemattribindex + (attnum)]

#define setelemattribute(otri, attnum, value)                                 \
  ((sgFloat *) (otri).tri)[m->elemattribindex + (attnum)] = value

/* Check or set a triangle's maximum area bound.                             */

#define areabound(otri)  ((sgFloat *) (otri).tri)[m->areaboundindex]

#define setareabound(otri, value)                                             \
  ((sgFloat *) (otri).tri)[m->areaboundindex] = value

#define deadtri(tria)  ((tria)[1] == (triangle) NULL)

#define killtri(tria)                                                         \
  (tria)[1] = (triangle) NULL;                                                \
  (tria)[3] = (triangle) NULL

#define sdecode(sptr, osub)                                                   \
  (osub).ssorient = (int) ((unsigned long) (sptr) & (unsigned long) 1l);      \
  (osub).ss = (subseg *)                                                      \
              ((unsigned long) (sptr) & ~ (unsigned long) 3l)

#define sencode(osub)                                                         \
  (subseg) ((unsigned long) (osub).ss | (unsigned long) (osub).ssorient)

#define ssym(osub1, osub2)                                                    \
  (osub2).ss = (osub1).ss;                                                    \
  (osub2).ssorient = 1 - (osub1).ssorient

#define ssymself(osub)                                                        \
  (osub).ssorient = 1 - (osub).ssorient

#define spivot(osub1, osub2)                                                  \
  sptr = (osub1).ss[(osub1).ssorient];                                        \
  sdecode(sptr, osub2)

#define spivotself(osub)                                                      \
  sptr = (osub).ss[(osub).ssorient];                                          \
  sdecode(sptr, osub)

#define snext(osub1, osub2)                                                   \
  sptr = (osub1).ss[1 - (osub1).ssorient];                                    \
  sdecode(sptr, osub2)

#define snextself(osub)                                                       \
  sptr = (osub).ss[1 - (osub).ssorient];                                      \
  sdecode(sptr, osub)

#define sorg(osub, vertexptr)                                                 \
  vertexptr = (vertex) (osub).ss[2 + (osub).ssorient]

#define sdest(osub, vertexptr)                                                \
  vertexptr = (vertex) (osub).ss[3 - (osub).ssorient]

#define setsorg(osub, vertexptr)                                              \
  (osub).ss[2 + (osub).ssorient] = (subseg) vertexptr

#define setsdest(osub, vertexptr)                                             \
  (osub).ss[3 - (osub).ssorient] = (subseg) vertexptr

#define mark(osub)  (* (int *) ((osub).ss + 6))

#define setmark(osub, value)                                                  \
  * (int *) ((osub).ss + 6) = value

#define sbond(osub1, osub2)                                                   \
  (osub1).ss[(osub1).ssorient] = sencode(osub2);                              \
  (osub2).ss[(osub2).ssorient] = sencode(osub1)

#define sdissolve(osub)                                                       \
  (osub).ss[(osub).ssorient] = (subseg) m->dummysub

#define subsegcopy(osub1, osub2)                                              \
  (osub2).ss = (osub1).ss;                                                    \
  (osub2).ssorient = (osub1).ssorient

#define subsegequal(osub1, osub2)                                             \
  (((osub1).ss == (osub2).ss) &&                                              \
   ((osub1).ssorient == (osub2).ssorient))

#define deadsubseg(sub)  ((sub)[1] == (subseg) NULL)

#define killsubseg(sub)                                                       \
  (sub)[1] = (subseg) NULL;                                                   \
  (sub)[2] = (subseg) NULL

#define tspivot(otri, osub)                                                   \
  sptr = (subseg) (otri).tri[6 + (otri).orient];                              \
  sdecode(sptr, osub)

#define stpivot(osub, otri)                                                   \
  ptr = (triangle) (osub).ss[4 + (osub).ssorient];                            \
  decode(ptr, otri)

#define tsbond(otri, osub)                                                    \
  (otri).tri[6 + (otri).orient] = (triangle) sencode(osub);                   \
  (osub).ss[4 + (osub).ssorient] = (subseg) encode(otri)

#define tsdissolve(otri)                                                      \
  (otri).tri[6 + (otri).orient] = (triangle) m->dummysub

#define stdissolve(osub)                                                      \
  (osub).ss[4 + (osub).ssorient] = (subseg) m->dummytri

#define vertexmark(vx)  ((int *) (vx))[m->vertexmarkindex]

#define setvertexmark(vx, value)                                              \
  ((int *) (vx))[m->vertexmarkindex] = value

#define vertextype(vx)  ((int *) (vx))[m->vertexmarkindex + 1]

#define setvertextype(vx, value)                                              \
  ((int *) (vx))[m->vertexmarkindex + 1] = value

#define vertex2tri(vx)  ((triangle *) (vx))[m->vertex2triindex]

#define setvertex2tri(vx, value)                                              \
  ((triangle *) (vx))[m->vertex2triindex] = value

int triunsuitable(vertex triorg, vertex tridest, vertex triapex, sgFloat area)
{
  sgFloat dxoa, dxda, dxod;
  sgFloat dyoa, dyda, dyod;
  sgFloat oalen, dalen, odlen;
  sgFloat maxlen;

  dxoa = triorg[0] - triapex[0];
  dyoa = triorg[1] - triapex[1];
  dxda = tridest[0] - triapex[0];
  dyda = tridest[1] - triapex[1];
  dxod = triorg[0] - tridest[0];
  dyod = triorg[1] - tridest[1];
  /* Find the squares of the lengths of the triangle's three edges. */
  oalen = dxoa * dxoa + dyoa * dyoa;
  dalen = dxda * dxda + dyda * dyda;
  odlen = dxod * dxod + dyod * dyod;
  /* Find the square of the length of the longest edge. */
  maxlen = (dalen > oalen) ? dalen : oalen;
  maxlen = (odlen > maxlen) ? odlen : maxlen;

  if (maxlen > 0.05 * (triorg[0] * triorg[0] + triorg[1] * triorg[1]) + 0.02) {
    return 1;
  } else {
    return 0;
  }
}

int *trimalloc(int size)
{
  int *memptr;

  memptr = (int*)malloc((unsigned int) size);
  if (memptr == (int *) NULL) {
    //  TRACE("Error:  Out of memory.\n");
    exit(1);
  }
  return(memptr);
}

void trifree(int *memptr)
{
  free(memptr);
}

void internalerror()
{
  assert(0);
}

void parsecommandline(int argc, char **argv, struct behavior *b)
{

#define STARTINDEX 0

	int i, j, k;
	char workstring[256];

	b->poly = b->refine = b->quality = 0;
	b->vararea = b->fixedarea = b->usertest = 0;
	b->regionattrib = b->convex = b->weighted = b->jettison = 0;
	b->firstnumber = 1;
	b->edgesout = b->voronoi = b->neighbors = b->geomview = 0;
	b->nobound = b->nopolywritten = b->nonodewritten = b->noelewritten = 0;
	b->noiterationnum = 0;
	b->noholes = b->noexact = 0;
	b->incremental = b->sweepline = 0;
	b->dwyer = 1;
	b->splitseg = 0;
	b->docheck = 0;
	b->nobisect = 0;
	b->nolenses = 0;
	b->steiner = -1;
	b->order = 1;
	b->minangle = 0.0;
	b->maxarea = -1.0;
	b->quiet = b->verbose = 0;

	for (i = STARTINDEX; i < argc; i++) {

			for (j = STARTINDEX; argv[i][j] != '\0'; j++) {
				if (argv[i][j] == 'p') {
					b->poly = 1;
				}
#ifndef CDT_ONLY
				if (argv[i][j] == 'r') {
					b->refine = 1;
				}
				if (argv[i][j] == 'q') {
					b->quality = 1;
					if (((argv[i][j + 1] >= '0') && (argv[i][j + 1] <= '9')) ||
						(argv[i][j + 1] == '.')) {
							k = 0;
							while (((argv[i][j + 1] >= '0') && (argv[i][j + 1] <= '9')) ||
								(argv[i][j + 1] == '.')) {
									j++;
									workstring[k] = argv[i][j];
									k++;
								}
								workstring[k] = '\0';
								b->minangle = (sgFloat) strtod(workstring, (char **) NULL);
						} else {
							b->minangle = 20.0;
						}
				}
				if (argv[i][j] == 'a') {
					b->quality = 1;
					if (((argv[i][j + 1] >= '0') && (argv[i][j + 1] <= '9')) ||
						(argv[i][j + 1] == '.')) {
							b->fixedarea = 1;
							k = 0;
							while (((argv[i][j + 1] >= '0') && (argv[i][j + 1] <= '9')) ||
								(argv[i][j + 1] == '.')) {
									j++;
									workstring[k] = argv[i][j];
									k++;
								}
								workstring[k] = '\0';
								b->maxarea = (sgFloat) strtod(workstring, (char **) NULL);
								if (b->maxarea <= 0.0) {
									//  TRACE("Error:  Maximum area must be greater than zero.\n");
									exit(1);
								}
						} else {
							b->vararea = 1;
						}
				}
				if (argv[i][j] == 'u') {
					b->quality = 1;
					b->usertest = 1;
				}
#endif /* not CDT_ONLY */
				if (argv[i][j] == 'A') {
					b->regionattrib = 1;
				}
				if (argv[i][j] == 'c') {
					b->convex = 1;
				}
				if (argv[i][j] == 'w') {
					b->weighted = 1;
				}
				if (argv[i][j] == 'W') {
					b->weighted = 2;
				}
				if (argv[i][j] == 'j') {
					b->jettison = 1;
				}
				if (argv[i][j] == 'z') {
					b->firstnumber = 0;
				}
				if (argv[i][j] == 'e') {
					b->edgesout = 1;
				}
				if (argv[i][j] == 'v') {
					b->voronoi = 1;
				}
				if (argv[i][j] == 'n') {
					b->neighbors = 1;
				}
				if (argv[i][j] == 'g') {
					b->geomview = 1;
				}
				if (argv[i][j] == 'B') {
					b->nobound = 1;
				}
				if (argv[i][j] == 'P') {
					b->nopolywritten = 1;
				}
				if (argv[i][j] == 'N') {
					b->nonodewritten = 1;
				}
				if (argv[i][j] == 'E') {
					b->noelewritten = 1;
				}

				if (argv[i][j] == 'O') {
					b->noholes = 1;
				}
				if (argv[i][j] == 'X') {
					b->noexact = 1;
				}
				if (argv[i][j] == 'o') {
					if (argv[i][j + 1] == '2') {
						j++;
						b->order = 2;
					}
				}
#ifndef CDT_ONLY
				if (argv[i][j] == 'Y') {
					b->nobisect++;
				}
				if (argv[i][j] == 'S') {
					b->steiner = 0;
					while ((argv[i][j + 1] >= '0') && (argv[i][j + 1] <= '9')) {
						j++;
						b->steiner = b->steiner * 10 + (int) (argv[i][j] - '0');
					}
				}
#endif /* not CDT_ONLY */
#ifndef REDUCED
				if (argv[i][j] == 'i') {
					b->incremental = 1;
				}
				if (argv[i][j] == 'F') {
					b->sweepline = 1;
				}
#endif /* not REDUCED */
				if (argv[i][j] == 'l') {
					b->dwyer = 0;
				}
#ifndef REDUCED
#ifndef CDT_ONLY
				if (argv[i][j] == 's') {
					b->splitseg = 1;
				}
				if (argv[i][j] == 'L') {
					b->nolenses = 1;
				}
#endif /* not CDT_ONLY */
				if (argv[i][j] == 'C') {
					b->docheck = 1;
				}
#endif /* not REDUCED */
				if (argv[i][j] == 'Q') {
					b->quiet = 1;
				}
				if (argv[i][j] == 'V') {
					b->verbose++;
				}
			}
	}

	b->usesegments = b->poly || b->refine || b->quality || b->convex;
	b->goodangle = cos(b->minangle * M_PI / 180.0);
	if (b->goodangle == 1.0) {
		b->offconstant = 0.0;
	} else {
		b->offconstant = 0.475 * sqrt((1.0 + b->goodangle) / (1.0 - b->goodangle));
	}
	b->goodangle *= b->goodangle;
	if (b->refine && b->noiterationnum) {
		//  TRACE(
//			"Error:  You cannot use the -I switch when refining a triangulation.\n");
		exit(1);
	}
	/* Be careful not to allocate space for element area constraints that */
	/*   will never be assigned any value (other than the default -1.0).  */
	if (!b->refine && !b->poly) {
		b->vararea = 0;
	}
	/* Be careful not to add an extra attribute to each element unless the */
	/*   input supports it (PSLG in, but not refining a preexisting mesh). */
	if (b->refine || !b->poly) {
		b->regionattrib = 0;
	}
	/* Regular/weighted triangulations are incompatible with PSLGs */
	/*   and meshing.                                              */
	if (b->weighted && (b->poly || b->quality)) {
		b->weighted = 0;
		if (!b->quiet) {
			//  TRACE("Warning:  weighted triangulations (-w, -W) are incompatible\n");
			//  TRACE("  with PSLGs (-p) and meshing (-q, -a, -u).  Weights ignored.\n"
//				);
		}
	}
	if (b->jettison && b->nonodewritten && !b->quiet) {
		//  TRACE("Warning:  -j and -N switches are somewhat incompatible.\n");
		//  TRACE("  If any vertices are jettisoned, you will need the output\n");
		//  TRACE("  .node file to reconstruct the new node indices.");
	}

}


void poolrestart(struct memorypool *pool)
{
  unsigned long alignptr;

  pool->items = 0;
  pool->maxitems = 0;

  /* Set the currently active block. */
  pool->nowblock = pool->firstblock;
  /* Find the first item in the pool.  Increment by the size of (int *). */
  alignptr = (unsigned long) (pool->nowblock + 1);
  /* Align the item on an `alignbytes'-byte boundary. */
  pool->nextitem = (int *)
    (alignptr + (unsigned long) pool->alignbytes -
     (alignptr % (unsigned long) pool->alignbytes));
  /* There are lots of unallocated items left in this block. */
  pool->unallocateditems = pool->itemsfirstblock;
  /* The stack of deallocated items is empty. */
  pool->deaditemstack = (int *) NULL;
}

void poolinit(struct memorypool *pool, int bytecount, int itemcount,
              int firstitemcount, enum wordtype wtype, int alignment)
{
  int wordsize;

  /* Initialize values in the pool. */
  pool->itemwordtype = wtype;
  wordsize = (pool->itemwordtype == POINTER) ? sizeof(int *) : sizeof(sgFloat);
 
  if (alignment > wordsize) {
    pool->alignbytes = alignment;
  } else {
    pool->alignbytes = wordsize;
  }
  if (sizeof(int *) > pool->alignbytes) {
    pool->alignbytes = sizeof(int *);
  }
  pool->itemwords = ((bytecount + pool->alignbytes - 1) / pool->alignbytes)
                  * (pool->alignbytes / wordsize);
  pool->itembytes = pool->itemwords * wordsize;
  pool->itemsperblock = itemcount;
  if (firstitemcount == 0) {
    pool->itemsfirstblock = itemcount;
  } else {
    pool->itemsfirstblock = firstitemcount;
  }
 pool->firstblock = (int **)
    trimalloc(pool->itemsfirstblock * pool->itembytes + (int) sizeof(int *) +
              pool->alignbytes);
  /* Set the next block pointer to NULL. */
  *(pool->firstblock) = (int *) NULL;
  poolrestart(pool);
}

void pooldeinit(struct memorypool *pool)
{
  while (pool->firstblock != (int **) NULL) {
    pool->nowblock = (int **) *(pool->firstblock);
    trifree((int *) pool->firstblock);
    pool->firstblock = pool->nowblock;
  }
}

int *poolalloc(struct memorypool *pool)
{
  int *newitem;
  int **newblock;
  unsigned long alignptr;

  /* First check the linked list of dead items.  If the list is not   */
  /*   empty, allocate an item from the list rather than a fresh one. */
  if (pool->deaditemstack != (int *) NULL) {
    newitem = pool->deaditemstack;               /* Take first item in list. */
    pool->deaditemstack = * (int **) pool->deaditemstack;
  } else {
    /* Check if there are any free items left in the current block. */
    if (pool->unallocateditems == 0) {
      /* Check if another block must be allocated. */
      if (*(pool->nowblock) == (int *) NULL) {
        /* Allocate a new block of items, pointed to by the previous block. */
        newblock = (int **) trimalloc(pool->itemsperblock * pool->itembytes +
                                       (int) sizeof(int *) +
                                       pool->alignbytes);
        *(pool->nowblock) = (int *) newblock;
        /* The next block pointer is NULL. */
        *newblock = (int *) NULL;
      }

      /* Move to the new block. */
      pool->nowblock = (int **) *(pool->nowblock);
      /* Find the first item in the block.    */
      /*   Increment by the size of (int *). */
      alignptr = (unsigned long) (pool->nowblock + 1);
      /* Align the item on an `alignbytes'-byte boundary. */
      pool->nextitem = (int *)
        (alignptr + (unsigned long) pool->alignbytes -
         (alignptr % (unsigned long) pool->alignbytes));
      /* There are lots of unallocated items left in this block. */
      pool->unallocateditems = pool->itemsperblock;
    }

    /* Allocate a new item. */
    newitem = pool->nextitem;
    /* Advance `nextitem' pointer to next free item in block. */
    if (pool->itemwordtype == POINTER) {
      pool->nextitem = (int *) ((int **) pool->nextitem + pool->itemwords);
    } else {
      pool->nextitem = (int *) ((sgFloat *) pool->nextitem + pool->itemwords);
    }
    pool->unallocateditems--;
    pool->maxitems++;
  }
  pool->items++;
  return newitem;
}

void pooldealloc(struct memorypool *pool, int *dyingitem)
{
  /* Push freshly killed item onto stack. */
  *((int **) dyingitem) = pool->deaditemstack;
  pool->deaditemstack = dyingitem;
  pool->items--;
}

void traversalinit(struct memorypool *pool)
{
  unsigned long alignptr;

  /* Begin the traversal in the first block. */
  pool->pathblock = pool->firstblock;
  /* Find the first item in the block.  Increment by the size of (int *). */
  alignptr = (unsigned long) (pool->pathblock + 1);
  /* Align with item on an `alignbytes'-byte boundary. */
  pool->pathitem = (int *)
    (alignptr + (unsigned long) pool->alignbytes -
     (alignptr % (unsigned long) pool->alignbytes));
  /* Set the number of items left in the current block. */
  pool->pathitemsleft = pool->itemsfirstblock;
}

int *traverse(struct memorypool *pool)
{
  int *newitem;
  unsigned long alignptr;

  /* Stop upon exhausting the list of items. */
  if (pool->pathitem == pool->nextitem) {
    return (int *) NULL;
  }

  /* Check whether any untraversed items remain in the current block. */
  if (pool->pathitemsleft == 0) {
    /* Find the next block. */
    pool->pathblock = (int **) *(pool->pathblock);
    /* Find the first item in the block.  Increment by the size of (int *). */
    alignptr = (unsigned long) (pool->pathblock + 1);
    /* Align with item on an `alignbytes'-byte boundary. */
    pool->pathitem = (int *)
      (alignptr + (unsigned long) pool->alignbytes -
       (alignptr % (unsigned long) pool->alignbytes));
    /* Set the number of items left in the current block. */
    pool->pathitemsleft = pool->itemsperblock;
  }

  newitem = pool->pathitem;
  /* Find the next item in the block. */
  if (pool->itemwordtype == POINTER) {
    pool->pathitem = (int *) ((int **) pool->pathitem + pool->itemwords);
  } else {
    pool->pathitem = (int *) ((sgFloat *) pool->pathitem + pool->itemwords);
  }
  pool->pathitemsleft--;
  return newitem;
}

void dummyinit(struct mesh *m, struct behavior *b, int trianglewords,
               int subsegwords)
{
  unsigned long alignptr;

  /* Set up `dummytri', the `triangle' that occupies "outer space." */
  m->dummytribase = (triangle *)
                    trimalloc(trianglewords * (int) sizeof(triangle) +
                              m->triangles.alignbytes);
  /* Align `dummytri' on a `triangles.alignbytes'-byte boundary. */
  alignptr = (unsigned long) m->dummytribase;
  m->dummytri = (triangle *)
    (alignptr + (unsigned long) m->triangles.alignbytes -
     (alignptr % (unsigned long) m->triangles.alignbytes));
  /* Initialize the three adjoining triangles to be "outer space."  These  */
  /*   will eventually be changed by various bonding operations, but their */
  /*   values don't really matter, as long as they can legally be          */
  /*   dereferenced.                                                       */
  m->dummytri[0] = (triangle) m->dummytri;
  m->dummytri[1] = (triangle) m->dummytri;
  m->dummytri[2] = (triangle) m->dummytri;
  /* Three NULL vertices. */
  m->dummytri[3] = (triangle) NULL;
  m->dummytri[4] = (triangle) NULL;
  m->dummytri[5] = (triangle) NULL;

  if (b->usesegments) {
    m->dummysubbase = (subseg *) trimalloc(subsegwords * (int) sizeof(subseg) +
                                           m->subsegs.alignbytes);
    /* Align `dummysub' on a `subsegs.alignbytes'-byte boundary. */
    alignptr = (unsigned long) m->dummysubbase;
    m->dummysub = (subseg *)
      (alignptr + (unsigned long) m->subsegs.alignbytes -
       (alignptr % (unsigned long) m->subsegs.alignbytes));
    m->dummysub[0] = (subseg) m->dummysub;
    m->dummysub[1] = (subseg) m->dummysub;
    /* Two NULL vertices. */
    m->dummysub[2] = (subseg) NULL;
    m->dummysub[3] = (subseg) NULL;
    /* Initialize the two adjoining triangles to be "outer space." */
    m->dummysub[4] = (subseg) m->dummytri;
    m->dummysub[5] = (subseg) m->dummytri;
    /* Set the boundary marker to zero. */
    * (int *) (m->dummysub + 6) = 0;

    /* Initialize the three adjoining subsegments of `dummytri' to be */
    /*   the omnipresent subsegment.                                  */
    m->dummytri[6] = (triangle) m->dummysub;
    m->dummytri[7] = (triangle) m->dummysub;
    m->dummytri[8] = (triangle) m->dummysub;
  }
}

void initializevertexpool(struct mesh *m, struct behavior *b)
{
  int vertexsize;

 m->vertexmarkindex = ((m->mesh_dim + m->nextras) * sizeof(sgFloat) +
                        sizeof(int) - 1) /
                       sizeof(int);
  vertexsize = (m->vertexmarkindex + 2) * sizeof(int);
  if (b->poly) {
    /* The index within each vertex at which a triangle pointer is found.  */
    /*   Ensure the pointer is aligned to a sizeof(triangle)-byte address. */
    m->vertex2triindex = (vertexsize + sizeof(triangle) - 1) /
                         sizeof(triangle);
    vertexsize = (m->vertex2triindex + 1) * sizeof(triangle);
  }

  /* Initialize the pool of vertices. */
  poolinit(&m->vertices, vertexsize, VERTEXPERBLOCK,
           m->invertices > VERTEXPERBLOCK ? m->invertices : VERTEXPERBLOCK,
           (sizeof(sgFloat) >= sizeof(triangle)) ? FLOATINGPOINT : POINTER, 0);
}

void initializetrisubpools(struct mesh *m, struct behavior *b)
{
  int trisize;

 m->highorderindex = 6 + (b->usesegments * 3);
  /* The number of bytes occupied by a triangle. */
  trisize = ((b->order + 1) * (b->order + 2) / 2 + (m->highorderindex - 3)) *
            sizeof(triangle);
  /* The index within each triangle at which its attributes are found, */
  /*   where the index is measured in sgFloats.                           */
  m->elemattribindex = (trisize + sizeof(sgFloat) - 1) / sizeof(sgFloat);
  /* The index within each triangle at which the maximum area constraint  */
  /*   is found, where the index is measured in sgFloats.  Note that if the  */
  /*   `regionattrib' flag is set, an additional attribute will be added. */
  m->areaboundindex = m->elemattribindex + m->eextras + b->regionattrib;
  /* If triangle attributes or an area bound are needed, increase the number */
  /*   of bytes occupied by a triangle.                                      */
  if (b->vararea) {
    trisize = (m->areaboundindex + 1) * sizeof(sgFloat);
  } else if (m->eextras + b->regionattrib > 0) {
    trisize = m->areaboundindex * sizeof(sgFloat);
  }
  /* If a Voronoi diagram or triangle neighbor graph is requested, make    */
  /*   sure there's room to store an integer index in each triangle.  This */
  /*   integer index can occupy the same space as the subsegment pointers  */
  /*   or attributes or area constraint or extra nodes.                    */
  if ((b->voronoi || b->neighbors) &&
      (trisize < 6 * sizeof(triangle) + sizeof(int))) {
    trisize = 6 * sizeof(triangle) + sizeof(int);
  }

  /* Having determined the memory size of a triangle, initialize the pool. */
  poolinit(&m->triangles, trisize, TRIPERBLOCK,
           (2 * m->invertices - 2) > TRIPERBLOCK ? (2 * m->invertices - 2) :
           TRIPERBLOCK, POINTER, 4);

  if (b->usesegments) {
    /* Initialize the pool of subsegments.  Take into account all six */
    /*   pointers and one boundary marker.                            */
    poolinit(&m->subsegs, 6 * sizeof(triangle) + sizeof(int), SUBSEGPERBLOCK,
             SUBSEGPERBLOCK, POINTER, 4);

    /* Initialize the "outer space" triangle and omnipresent subsegment. */
    dummyinit(m, b, m->triangles.itemwords, m->subsegs.itemwords);
  } else {
    /* Initialize the "outer space" triangle. */
    dummyinit(m, b, m->triangles.itemwords, 0);
  }
}

void triangledealloc(struct mesh *m, triangle *dyingtriangle)
{
  /* Mark the triangle as dead.  This makes it possible to detect dead */
  /*   triangles when traversing the list of all triangles.            */
  killtri(dyingtriangle);
  pooldealloc(&m->triangles, (int *) dyingtriangle);
}

triangle *triangletraverse(struct mesh *m)
{
  triangle *newtriangle;

  do {
    newtriangle = (triangle *) traverse(&m->triangles);
    if (newtriangle == (triangle *) NULL) {
      return (triangle *) NULL;
    }
  } while (deadtri(newtriangle));                         /* Skip dead ones. */
  return newtriangle;
}

void subsegdealloc(struct mesh *m, subseg *dyingsubseg)
{
  /* Mark the subsegment as dead.  This makes it possible to detect dead */
  /*   subsegments when traversing the list of all subsegments.          */
  killsubseg(dyingsubseg);
  pooldealloc(&m->subsegs, (int *) dyingsubseg);
}

subseg *subsegtraverse(struct mesh *m)
{
  subseg *newsubseg;

  do {
    newsubseg = (subseg *) traverse(&m->subsegs);
    if (newsubseg == (subseg *) NULL) {
      return (subseg *) NULL;
    }
  } while (deadsubseg(newsubseg));                        /* Skip dead ones. */
  return newsubseg;
}

void vertexdealloc(struct mesh *m, vertex dyingvertex)
{
  /* Mark the vertex as dead.  This makes it possible to detect dead */
  /*   vertices when traversing the list of all vertices.            */
  setvertextype(dyingvertex, DEADVERTEX);
  pooldealloc(&m->vertices, (int *) dyingvertex);
}

vertex vertextraverse(struct mesh *m)
{
  vertex newvertex;

  do {
    newvertex = (vertex) traverse(&m->vertices);
    if (newvertex == (vertex) NULL) {
      return (vertex) NULL;
    }
  } while (vertextype(newvertex) == DEADVERTEX);          /* Skip dead ones. */
  return newvertex;
}

#ifndef CDT_ONLY

void badsubsegdealloc(struct mesh *m, struct badsubseg *dyingseg)
{
  /* Set subsegment's origin to NULL.  This makes it possible to detect dead */
  /*   subsegments when traversing the list of all encroached subsegments.   */
  dyingseg->subsegorg = (vertex) NULL;
  pooldealloc(&m->badsubsegs, (int *) dyingseg);
}

#endif /* not CDT_ONLY */

#ifndef CDT_ONLY

struct badsubseg *badsubsegtraverse(struct mesh *m)
{
  struct badsubseg *newseg;

  do {
    newseg = (struct badsubseg *) traverse(&m->badsubsegs);
    if (newseg == (struct badsubseg *) NULL) {
      return (struct badsubseg *) NULL;
    }
  } while (newseg->subsegorg == (vertex) NULL);           /* Skip dead ones. */
  return newseg;
}

#endif /* not CDT_ONLY */

vertex getvertex(struct mesh *m, struct behavior *b, int number)
{
  int **getblock;
  vertex foundvertex;
  unsigned long alignptr;
  int current;

  getblock = m->vertices.firstblock;
  current = b->firstnumber;

  /* Find the right block. */
  if (current + m->vertices.itemsfirstblock <= number) {
    getblock = (int **) *getblock;
    current += m->vertices.itemsfirstblock;
    while (current + m->vertices.itemsperblock <= number) {
      getblock = (int **) *getblock;
      current += m->vertices.itemsperblock;
    }
  }

  /* Now find the right vertex. */
  alignptr = (unsigned long) (getblock + 1);
  foundvertex = (vertex) (alignptr + (unsigned long) m->vertices.alignbytes -
                          (alignptr % (unsigned long) m->vertices.alignbytes));
  while (current < number) {
    foundvertex += m->vertices.itemwords;
    current++;
  }
  return foundvertex;
}

void triangledeinit(struct mesh *m, struct behavior *b)
{
  pooldeinit(&m->triangles);
  trifree((int *) m->dummytribase);
  if (b->usesegments) {
    pooldeinit(&m->subsegs);
    trifree((int *) m->dummysubbase);
  }
  pooldeinit(&m->vertices);
#ifndef CDT_ONLY
  if (b->quality) {
    pooldeinit(&m->badsubsegs);
    if ((b->minangle > 0.0) || b->vararea || b->fixedarea || b->usertest) {
      pooldeinit(&m->badtriangles);
      pooldeinit(&m->flipstackers);
    }
  }
#endif /* not CDT_ONLY */
}

void maketriangle(struct mesh *m, struct behavior *b, struct otri *newotri)
{
  int i;

  newotri->tri = (triangle *) poolalloc(&m->triangles);
  /* Initialize the three adjoining triangles to be "outer space". */
  newotri->tri[0] = (triangle) m->dummytri;
  newotri->tri[1] = (triangle) m->dummytri;
  newotri->tri[2] = (triangle) m->dummytri;
  /* Three NULL vertices. */
  newotri->tri[3] = (triangle) NULL;
  newotri->tri[4] = (triangle) NULL;
  newotri->tri[5] = (triangle) NULL;
  if (b->usesegments) {
    /* Initialize the three adjoining subsegments to be the omnipresent */
    /*   subsegment.                                                    */
    newotri->tri[6] = (triangle) m->dummysub;
    newotri->tri[7] = (triangle) m->dummysub;
    newotri->tri[8] = (triangle) m->dummysub;
  }
  for (i = 0; i < m->eextras; i++) {
    setelemattribute(*newotri, i, 0.0);
  }
  if (b->vararea) {
    setareabound(*newotri, -1.0);
  }

  newotri->orient = 0;
}

void makesubseg(struct mesh *m, struct osub *newsubseg)
{
  newsubseg->ss = (subseg *) poolalloc(&m->subsegs);
  /* Initialize the two adjoining subsegments to be the omnipresent */
  /*   subsegment.                                                  */
  newsubseg->ss[0] = (subseg) m->dummysub;
  newsubseg->ss[1] = (subseg) m->dummysub;
  /* Two NULL vertices. */
  newsubseg->ss[2] = (subseg) NULL;
  newsubseg->ss[3] = (subseg) NULL;
  /* Initialize the two adjoining triangles to be "outer space." */
  newsubseg->ss[4] = (subseg) m->dummytri;
  newsubseg->ss[5] = (subseg) m->dummytri;
  /* Set the boundary marker to zero. */
  setmark(*newsubseg, 0);

  newsubseg->ssorient = 0;
}

#define Absolute(a)  ((a) >= 0.0 ? (a) : -(a))

#define Fast_Two_Sum_Tail(a, b, x, y) \
  bvirt = x - a; \
  y = b - bvirt

#define Fast_Two_Sum(a, b, x, y) \
  x = (sgFloat) (a + b); \
  Fast_Two_Sum_Tail(a, b, x, y)

#define Two_Sum_Tail(a, b, x, y) \
  bvirt = (sgFloat) (x - a); \
  avirt = x - bvirt; \
  bround = b - bvirt; \
  around = a - avirt; \
  y = around + bround

#define Two_Sum(a, b, x, y) \
  x = (sgFloat) (a + b); \
  Two_Sum_Tail(a, b, x, y)

#define Two_Diff_Tail(a, b, x, y) \
  bvirt = (sgFloat) (a - x); \
  avirt = x + bvirt; \
  bround = bvirt - b; \
  around = a - avirt; \
  y = around + bround

#define Two_Diff(a, b, x, y) \
  x = (sgFloat) (a - b); \
  Two_Diff_Tail(a, b, x, y)

#define Split(a, ahi, alo) \
  c = (sgFloat) (splitter * a); \
  abig = (sgFloat) (c - a); \
  ahi = c - abig; \
  alo = a - ahi

#define Two_Product_Tail(a, b, x, y) \
  Split(a, ahi, alo); \
  Split(b, bhi, blo); \
  err1 = x - (ahi * bhi); \
  err2 = err1 - (alo * bhi); \
  err3 = err2 - (ahi * blo); \
  y = (alo * blo) - err3

#define Two_Product(a, b, x, y) \
  x = (sgFloat) (a * b); \
  Two_Product_Tail(a, b, x, y)

#define Two_Product_Presplit(a, b, bhi, blo, x, y) \
  x = (sgFloat) (a * b); \
  Split(a, ahi, alo); \
  err1 = x - (ahi * bhi); \
  err2 = err1 - (alo * bhi); \
  err3 = err2 - (ahi * blo); \
  y = (alo * blo) - err3

#define Square_Tail(a, x, y) \
  Split(a, ahi, alo); \
  err1 = x - (ahi * ahi); \
  err3 = err1 - ((ahi + ahi) * alo); \
  y = (alo * alo) - err3

#define Square(a, x, y) \
  x = (sgFloat) (a * a); \
  Square_Tail(a, x, y)

#define Two_One_Sum(a1, a0, b, x2, x1, x0) \
  Two_Sum(a0, b , _i, x0); \
  Two_Sum(a1, _i, x2, x1)

#define Two_One_Diff(a1, a0, b, x2, x1, x0) \
  Two_Diff(a0, b , _i, x0); \
  Two_Sum( a1, _i, x2, x1)

#define Two_Two_Sum(a1, a0, b1, b0, x3, x2, x1, x0) \
  Two_One_Sum(a1, a0, b0, _j, _0, x0); \
  Two_One_Sum(_j, _0, b1, x3, x2, x1)

#define Two_Two_Diff(a1, a0, b1, b0, x3, x2, x1, x0) \
  Two_One_Diff(a1, a0, b0, _j, _0, x0); \
  Two_One_Diff(_j, _0, b1, x3, x2, x1)

#define Two_One_Product(a1, a0, b, x3, x2, x1, x0) \
  Split(b, bhi, blo); \
  Two_Product_Presplit(a0, b, bhi, blo, _i, x0); \
  Two_Product_Presplit(a1, b, bhi, blo, _j, _0); \
  Two_Sum(_i, _0, _k, x1); \
  Fast_Two_Sum(_j, _k, x3, x2)


void exactinit()
{
  sgFloat half;
  sgFloat check, lastcheck;
  int every_other;

#ifdef CPU86
  _control87(_PC_53, _MCW_PC); /* Set FPU control word for sgFloat precision. */
#endif /* CPU86 */

  every_other = 1;
  half = 0.5;
  epsilon = 1.0;
  splitter = 1.0;
  check = 1.0;
  
  do {
    lastcheck = check;
    epsilon *= half;
    if (every_other) {
      splitter *= 2.0;
    }
    every_other = !every_other;
    check = 1.0 + epsilon;
  } while ((check != 1.0) && (check != lastcheck));
  splitter += 1.0;
  /* Error bounds for orientation and incircle tests. */
  resulterrbound = (3.0 + 8.0 * epsilon) * epsilon;
  ccwerrboundA = (3.0 + 16.0 * epsilon) * epsilon;
  ccwerrboundB = (2.0 + 12.0 * epsilon) * epsilon;
  ccwerrboundC = (9.0 + 64.0 * epsilon) * epsilon * epsilon;
  iccerrboundA = (10.0 + 96.0 * epsilon) * epsilon;
  iccerrboundB = (4.0 + 48.0 * epsilon) * epsilon;
  iccerrboundC = (44.0 + 576.0 * epsilon) * epsilon * epsilon;
  o3derrboundA = (7.0 + 56.0 * epsilon) * epsilon;
  o3derrboundB = (3.0 + 28.0 * epsilon) * epsilon;
  o3derrboundC = (26.0 + 288.0 * epsilon) * epsilon * epsilon;
}

int fast_expansion_sum_zeroelim(int elen, sgFloat *e, int flen, sgFloat *f, sgFloat *h)
{
  sgFloat Q;
   sgFloat Qnew;
   sgFloat hh;
   sgFloat bvirt;
  sgFloat avirt, bround, around;
  int eindex, findex, hindex;
  sgFloat enow, fnow;

  enow = e[0];
  fnow = f[0];
  eindex = findex = 0;
  if ((fnow > enow) == (fnow > -enow)) {
    Q = enow;
    enow = e[++eindex];
  } else {
    Q = fnow;
    fnow = f[++findex];
  }
  hindex = 0;
  if ((eindex < elen) && (findex < flen)) {
    if ((fnow > enow) == (fnow > -enow)) {
      Fast_Two_Sum(enow, Q, Qnew, hh);
      enow = e[++eindex];
    } else {
      Fast_Two_Sum(fnow, Q, Qnew, hh);
      fnow = f[++findex];
    }
    Q = Qnew;
    if (hh != 0.0) {
      h[hindex++] = hh;
    }
    while ((eindex < elen) && (findex < flen)) {
      if ((fnow > enow) == (fnow > -enow)) {
        Two_Sum(Q, enow, Qnew, hh);
        enow = e[++eindex];
      } else {
        Two_Sum(Q, fnow, Qnew, hh);
        fnow = f[++findex];
      }
      Q = Qnew;
      if (hh != 0.0) {
        h[hindex++] = hh;
      }
    }
  }
  while (eindex < elen) {
    Two_Sum(Q, enow, Qnew, hh);
    enow = e[++eindex];
    Q = Qnew;
    if (hh != 0.0) {
      h[hindex++] = hh;
    }
  }
  while (findex < flen) {
    Two_Sum(Q, fnow, Qnew, hh);
    fnow = f[++findex];
    Q = Qnew;
    if (hh != 0.0) {
      h[hindex++] = hh;
    }
  }
  if ((Q != 0.0) || (hindex == 0)) {
    h[hindex++] = Q;
  }
  return hindex;
}

int scale_expansion_zeroelim(int elen, sgFloat *e, sgFloat b, sgFloat *h)
{
   sgFloat Q, sum;
  sgFloat hh;
   sgFloat product1;
  sgFloat product0;
  int eindex, hindex;
  sgFloat enow;
   sgFloat bvirt;
  sgFloat avirt, bround, around;
   sgFloat c;
   sgFloat abig;
  sgFloat ahi, alo, bhi, blo;
  sgFloat err1, err2, err3;

  Split(b, bhi, blo);
  Two_Product_Presplit(e[0], b, bhi, blo, Q, hh);
  hindex = 0;
  if (hh != 0) {
    h[hindex++] = hh;
  }
  for (eindex = 1; eindex < elen; eindex++) {
    enow = e[eindex];
    Two_Product_Presplit(enow, b, bhi, blo, product1, product0);
    Two_Sum(Q, product0, sum, hh);
    if (hh != 0) {
      h[hindex++] = hh;
    }
    Fast_Two_Sum(product1, sum, Q, hh);
    if (hh != 0) {
      h[hindex++] = hh;
    }
  }
  if ((Q != 0.0) || (hindex == 0)) {
    h[hindex++] = Q;
  }
  return hindex;
}

sgFloat estimate(int elen, sgFloat *e)
{
  sgFloat Q;
  int eindex;

  Q = e[0];
  for (eindex = 1; eindex < elen; eindex++) {
    Q += e[eindex];
  }
  return Q;
}

sgFloat counterclockwiseadapt(vertex pa, vertex pb, vertex pc, sgFloat detsum)
{
   sgFloat acx, acy, bcx, bcy;
  sgFloat acxtail, acytail, bcxtail, bcytail;
   sgFloat detleft, detright;
  sgFloat detlefttail, detrighttail;
  sgFloat det, errbound;
  sgFloat B[4], C1[8], C2[12], D[16];
   sgFloat B3;
  int C1length, C2length, Dlength;
  sgFloat u[4];
   sgFloat u3;
   sgFloat s1, t1;
  sgFloat s0, t0;

   sgFloat bvirt;
  sgFloat avirt, bround, around;
   sgFloat c;
   sgFloat abig;
  sgFloat ahi, alo, bhi, blo;
  sgFloat err1, err2, err3;
   sgFloat _i, _j;
  sgFloat _0;

  acx = (sgFloat) (pa[0] - pc[0]);
  bcx = (sgFloat) (pb[0] - pc[0]);
  acy = (sgFloat) (pa[1] - pc[1]);
  bcy = (sgFloat) (pb[1] - pc[1]);

  Two_Product(acx, bcy, detleft, detlefttail);
  Two_Product(acy, bcx, detright, detrighttail);

  Two_Two_Diff(detleft, detlefttail, detright, detrighttail,
               B3, B[2], B[1], B[0]);
  B[3] = B3;

  det = estimate(4, B);
  errbound = ccwerrboundB * detsum;
  if ((det >= errbound) || (-det >= errbound)) {
    return det;
  }

  Two_Diff_Tail(pa[0], pc[0], acx, acxtail);
  Two_Diff_Tail(pb[0], pc[0], bcx, bcxtail);
  Two_Diff_Tail(pa[1], pc[1], acy, acytail);
  Two_Diff_Tail(pb[1], pc[1], bcy, bcytail);

  if ((acxtail == 0.0) && (acytail == 0.0)
      && (bcxtail == 0.0) && (bcytail == 0.0)) {
    return det;
  }

  errbound = ccwerrboundC * detsum + resulterrbound * Absolute(det);
  det += (acx * bcytail + bcy * acxtail)
       - (acy * bcxtail + bcx * acytail);
  if ((det >= errbound) || (-det >= errbound)) {
    return det;
  }

  Two_Product(acxtail, bcy, s1, s0);
  Two_Product(acytail, bcx, t1, t0);
  Two_Two_Diff(s1, s0, t1, t0, u3, u[2], u[1], u[0]);
  u[3] = u3;
  C1length = fast_expansion_sum_zeroelim(4, B, 4, u, C1);

  Two_Product(acx, bcytail, s1, s0);
  Two_Product(acy, bcxtail, t1, t0);
  Two_Two_Diff(s1, s0, t1, t0, u3, u[2], u[1], u[0]);
  u[3] = u3;
  C2length = fast_expansion_sum_zeroelim(C1length, C1, 4, u, C2);

  Two_Product(acxtail, bcytail, s1, s0);
  Two_Product(acytail, bcxtail, t1, t0);
  Two_Two_Diff(s1, s0, t1, t0, u3, u[2], u[1], u[0]);
  u[3] = u3;
  Dlength = fast_expansion_sum_zeroelim(C2length, C2, 4, u, D);

  return(D[Dlength - 1]);
}

sgFloat counterclockwise(struct mesh *m, struct behavior *b,
                      vertex pa, vertex pb, vertex pc)
{
  sgFloat detleft, detright, det;
  sgFloat detsum, errbound;

  m->counterclockcount++;

  detleft = (pa[0] - pc[0]) * (pb[1] - pc[1]);
  detright = (pa[1] - pc[1]) * (pb[0] - pc[0]);
  det = detleft - detright;

  if (b->noexact) {
    return det;
  }

  if (detleft > 0.0) {
    if (detright <= 0.0) {
      return det;
    } else {
      detsum = detleft + detright;
    }
  } else if (detleft < 0.0) {
    if (detright >= 0.0) {
      return det;
    } else {
      detsum = -detleft - detright;
    }
  } else {
    return det;
  }

  errbound = ccwerrboundA * detsum;
  if ((det >= errbound) || (-det >= errbound)) {
    return det;
  }

  return counterclockwiseadapt(pa, pb, pc, detsum);
}

sgFloat incircleadapt(vertex pa, vertex pb, vertex pc, vertex pd, sgFloat permanent)
{
   sgFloat adx, bdx, cdx, ady, bdy, cdy;
  sgFloat det, errbound;

   sgFloat bdxcdy1, cdxbdy1, cdxady1, adxcdy1, adxbdy1, bdxady1;
  sgFloat bdxcdy0, cdxbdy0, cdxady0, adxcdy0, adxbdy0, bdxady0;
  sgFloat bc[4], ca[4], ab[4];
   sgFloat bc3, ca3, ab3;
  sgFloat axbc[8], axxbc[16], aybc[8], ayybc[16], adet[32];
  int axbclen, axxbclen, aybclen, ayybclen, alen;
  sgFloat bxca[8], bxxca[16], byca[8], byyca[16], bdet[32];
  int bxcalen, bxxcalen, bycalen, byycalen, blen;
  sgFloat cxab[8], cxxab[16], cyab[8], cyyab[16], cdet[32];
  int cxablen, cxxablen, cyablen, cyyablen, clen;
  sgFloat abdet[64];
  int ablen;
  sgFloat fin1[1152], fin2[1152];
  sgFloat *finnow, *finother, *finswap;
  int finlength;

  sgFloat adxtail, bdxtail, cdxtail, adytail, bdytail, cdytail;
   sgFloat adxadx1, adyady1, bdxbdx1, bdybdy1, cdxcdx1, cdycdy1;
  sgFloat adxadx0, adyady0, bdxbdx0, bdybdy0, cdxcdx0, cdycdy0;
  sgFloat aa[4], bb[4], cc[4];
   sgFloat aa3, bb3, cc3;
   sgFloat ti1, tj1;
  sgFloat ti0, tj0;
  sgFloat u[4], v[4];
   sgFloat u3, v3;
  sgFloat temp8[8], temp16a[16], temp16b[16], temp16c[16];
  sgFloat temp32a[32], temp32b[32], temp48[48], temp64[64];
  int temp8len, temp16alen, temp16blen, temp16clen;
  int temp32alen, temp32blen, temp48len, temp64len;
  sgFloat axtbb[8], axtcc[8], aytbb[8], aytcc[8];
  int axtbblen, axtcclen, aytbblen, aytcclen;
  sgFloat bxtaa[8], bxtcc[8], bytaa[8], bytcc[8];
  int bxtaalen, bxtcclen, bytaalen, bytcclen;
  sgFloat cxtaa[8], cxtbb[8], cytaa[8], cytbb[8];
  int cxtaalen, cxtbblen, cytaalen, cytbblen;
  sgFloat axtbc[8], aytbc[8], bxtca[8], bytca[8], cxtab[8], cytab[8];
  int axtbclen, aytbclen, bxtcalen, bytcalen, cxtablen, cytablen;
  sgFloat axtbct[16], aytbct[16], bxtcat[16], bytcat[16], cxtabt[16], cytabt[16];
  int axtbctlen, aytbctlen, bxtcatlen, bytcatlen, cxtabtlen, cytabtlen;
  sgFloat axtbctt[8], aytbctt[8], bxtcatt[8];
  sgFloat bytcatt[8], cxtabtt[8], cytabtt[8];
  int axtbcttlen, aytbcttlen, bxtcattlen, bytcattlen, cxtabttlen, cytabttlen;
  sgFloat abt[8], bct[8], cat[8];
  int abtlen, bctlen, catlen;
  sgFloat abtt[4], bctt[4], catt[4];
  int abttlen, bcttlen, cattlen;
   sgFloat abtt3, bctt3, catt3;
  sgFloat negate;

   sgFloat bvirt;
  sgFloat avirt, bround, around;
   sgFloat c;
   sgFloat abig;
  sgFloat ahi, alo, bhi, blo;
  sgFloat err1, err2, err3;
   sgFloat _i, _j;
  sgFloat _0;

  adx = (sgFloat) (pa[0] - pd[0]);
  bdx = (sgFloat) (pb[0] - pd[0]);
  cdx = (sgFloat) (pc[0] - pd[0]);
  ady = (sgFloat) (pa[1] - pd[1]);
  bdy = (sgFloat) (pb[1] - pd[1]);
  cdy = (sgFloat) (pc[1] - pd[1]);

  Two_Product(bdx, cdy, bdxcdy1, bdxcdy0);
  Two_Product(cdx, bdy, cdxbdy1, cdxbdy0);
  Two_Two_Diff(bdxcdy1, bdxcdy0, cdxbdy1, cdxbdy0, bc3, bc[2], bc[1], bc[0]);
  bc[3] = bc3;
  axbclen = scale_expansion_zeroelim(4, bc, adx, axbc);
  axxbclen = scale_expansion_zeroelim(axbclen, axbc, adx, axxbc);
  aybclen = scale_expansion_zeroelim(4, bc, ady, aybc);
  ayybclen = scale_expansion_zeroelim(aybclen, aybc, ady, ayybc);
  alen = fast_expansion_sum_zeroelim(axxbclen, axxbc, ayybclen, ayybc, adet);

  Two_Product(cdx, ady, cdxady1, cdxady0);
  Two_Product(adx, cdy, adxcdy1, adxcdy0);
  Two_Two_Diff(cdxady1, cdxady0, adxcdy1, adxcdy0, ca3, ca[2], ca[1], ca[0]);
  ca[3] = ca3;
  bxcalen = scale_expansion_zeroelim(4, ca, bdx, bxca);
  bxxcalen = scale_expansion_zeroelim(bxcalen, bxca, bdx, bxxca);
  bycalen = scale_expansion_zeroelim(4, ca, bdy, byca);
  byycalen = scale_expansion_zeroelim(bycalen, byca, bdy, byyca);
  blen = fast_expansion_sum_zeroelim(bxxcalen, bxxca, byycalen, byyca, bdet);

  Two_Product(adx, bdy, adxbdy1, adxbdy0);
  Two_Product(bdx, ady, bdxady1, bdxady0);
  Two_Two_Diff(adxbdy1, adxbdy0, bdxady1, bdxady0, ab3, ab[2], ab[1], ab[0]);
  ab[3] = ab3;
  cxablen = scale_expansion_zeroelim(4, ab, cdx, cxab);
  cxxablen = scale_expansion_zeroelim(cxablen, cxab, cdx, cxxab);
  cyablen = scale_expansion_zeroelim(4, ab, cdy, cyab);
  cyyablen = scale_expansion_zeroelim(cyablen, cyab, cdy, cyyab);
  clen = fast_expansion_sum_zeroelim(cxxablen, cxxab, cyyablen, cyyab, cdet);

  ablen = fast_expansion_sum_zeroelim(alen, adet, blen, bdet, abdet);
  finlength = fast_expansion_sum_zeroelim(ablen, abdet, clen, cdet, fin1);

  det = estimate(finlength, fin1);
  errbound = iccerrboundB * permanent;
  if ((det >= errbound) || (-det >= errbound)) {
    return det;
  }

  Two_Diff_Tail(pa[0], pd[0], adx, adxtail);
  Two_Diff_Tail(pa[1], pd[1], ady, adytail);
  Two_Diff_Tail(pb[0], pd[0], bdx, bdxtail);
  Two_Diff_Tail(pb[1], pd[1], bdy, bdytail);
  Two_Diff_Tail(pc[0], pd[0], cdx, cdxtail);
  Two_Diff_Tail(pc[1], pd[1], cdy, cdytail);
  if ((adxtail == 0.0) && (bdxtail == 0.0) && (cdxtail == 0.0)
      && (adytail == 0.0) && (bdytail == 0.0) && (cdytail == 0.0)) {
    return det;
  }

  errbound = iccerrboundC * permanent + resulterrbound * Absolute(det);
  det += ((adx * adx + ady * ady) * ((bdx * cdytail + cdy * bdxtail)
                                     - (bdy * cdxtail + cdx * bdytail))
          + 2.0 * (adx * adxtail + ady * adytail) * (bdx * cdy - bdy * cdx))
       + ((bdx * bdx + bdy * bdy) * ((cdx * adytail + ady * cdxtail)
                                     - (cdy * adxtail + adx * cdytail))
          + 2.0 * (bdx * bdxtail + bdy * bdytail) * (cdx * ady - cdy * adx))
       + ((cdx * cdx + cdy * cdy) * ((adx * bdytail + bdy * adxtail)
                                     - (ady * bdxtail + bdx * adytail))
          + 2.0 * (cdx * cdxtail + cdy * cdytail) * (adx * bdy - ady * bdx));
  if ((det >= errbound) || (-det >= errbound)) {
    return det;
  }

  finnow = fin1;
  finother = fin2;

  if ((bdxtail != 0.0) || (bdytail != 0.0)
      || (cdxtail != 0.0) || (cdytail != 0.0)) {
    Square(adx, adxadx1, adxadx0);
    Square(ady, adyady1, adyady0);
    Two_Two_Sum(adxadx1, adxadx0, adyady1, adyady0, aa3, aa[2], aa[1], aa[0]);
    aa[3] = aa3;
  }
  if ((cdxtail != 0.0) || (cdytail != 0.0)
      || (adxtail != 0.0) || (adytail != 0.0)) {
    Square(bdx, bdxbdx1, bdxbdx0);
    Square(bdy, bdybdy1, bdybdy0);
    Two_Two_Sum(bdxbdx1, bdxbdx0, bdybdy1, bdybdy0, bb3, bb[2], bb[1], bb[0]);
    bb[3] = bb3;
  }
  if ((adxtail != 0.0) || (adytail != 0.0)
      || (bdxtail != 0.0) || (bdytail != 0.0)) {
    Square(cdx, cdxcdx1, cdxcdx0);
    Square(cdy, cdycdy1, cdycdy0);
    Two_Two_Sum(cdxcdx1, cdxcdx0, cdycdy1, cdycdy0, cc3, cc[2], cc[1], cc[0]);
    cc[3] = cc3;
  }

  if (adxtail != 0.0) {
    axtbclen = scale_expansion_zeroelim(4, bc, adxtail, axtbc);
    temp16alen = scale_expansion_zeroelim(axtbclen, axtbc, 2.0 * adx,
                                          temp16a);

    axtcclen = scale_expansion_zeroelim(4, cc, adxtail, axtcc);
    temp16blen = scale_expansion_zeroelim(axtcclen, axtcc, bdy, temp16b);

    axtbblen = scale_expansion_zeroelim(4, bb, adxtail, axtbb);
    temp16clen = scale_expansion_zeroelim(axtbblen, axtbb, -cdy, temp16c);

    temp32alen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                            temp16blen, temp16b, temp32a);
    temp48len = fast_expansion_sum_zeroelim(temp16clen, temp16c,
                                            temp32alen, temp32a, temp48);
    finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                            temp48, finother);
    finswap = finnow; finnow = finother; finother = finswap;
  }
  if (adytail != 0.0) {
    aytbclen = scale_expansion_zeroelim(4, bc, adytail, aytbc);
    temp16alen = scale_expansion_zeroelim(aytbclen, aytbc, 2.0 * ady,
                                          temp16a);

    aytbblen = scale_expansion_zeroelim(4, bb, adytail, aytbb);
    temp16blen = scale_expansion_zeroelim(aytbblen, aytbb, cdx, temp16b);

    aytcclen = scale_expansion_zeroelim(4, cc, adytail, aytcc);
    temp16clen = scale_expansion_zeroelim(aytcclen, aytcc, -bdx, temp16c);

    temp32alen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                            temp16blen, temp16b, temp32a);
    temp48len = fast_expansion_sum_zeroelim(temp16clen, temp16c,
                                            temp32alen, temp32a, temp48);
    finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                            temp48, finother);
    finswap = finnow; finnow = finother; finother = finswap;
  }
  if (bdxtail != 0.0) {
    bxtcalen = scale_expansion_zeroelim(4, ca, bdxtail, bxtca);
    temp16alen = scale_expansion_zeroelim(bxtcalen, bxtca, 2.0 * bdx,
                                          temp16a);

    bxtaalen = scale_expansion_zeroelim(4, aa, bdxtail, bxtaa);
    temp16blen = scale_expansion_zeroelim(bxtaalen, bxtaa, cdy, temp16b);

    bxtcclen = scale_expansion_zeroelim(4, cc, bdxtail, bxtcc);
    temp16clen = scale_expansion_zeroelim(bxtcclen, bxtcc, -ady, temp16c);

    temp32alen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                            temp16blen, temp16b, temp32a);
    temp48len = fast_expansion_sum_zeroelim(temp16clen, temp16c,
                                            temp32alen, temp32a, temp48);
    finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                            temp48, finother);
    finswap = finnow; finnow = finother; finother = finswap;
  }
  if (bdytail != 0.0) {
    bytcalen = scale_expansion_zeroelim(4, ca, bdytail, bytca);
    temp16alen = scale_expansion_zeroelim(bytcalen, bytca, 2.0 * bdy,
                                          temp16a);

    bytcclen = scale_expansion_zeroelim(4, cc, bdytail, bytcc);
    temp16blen = scale_expansion_zeroelim(bytcclen, bytcc, adx, temp16b);

    bytaalen = scale_expansion_zeroelim(4, aa, bdytail, bytaa);
    temp16clen = scale_expansion_zeroelim(bytaalen, bytaa, -cdx, temp16c);

    temp32alen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                            temp16blen, temp16b, temp32a);
    temp48len = fast_expansion_sum_zeroelim(temp16clen, temp16c,
                                            temp32alen, temp32a, temp48);
    finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                            temp48, finother);
    finswap = finnow; finnow = finother; finother = finswap;
  }
  if (cdxtail != 0.0) {
    cxtablen = scale_expansion_zeroelim(4, ab, cdxtail, cxtab);
    temp16alen = scale_expansion_zeroelim(cxtablen, cxtab, 2.0 * cdx,
                                          temp16a);

    cxtbblen = scale_expansion_zeroelim(4, bb, cdxtail, cxtbb);
    temp16blen = scale_expansion_zeroelim(cxtbblen, cxtbb, ady, temp16b);

    cxtaalen = scale_expansion_zeroelim(4, aa, cdxtail, cxtaa);
    temp16clen = scale_expansion_zeroelim(cxtaalen, cxtaa, -bdy, temp16c);

    temp32alen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                            temp16blen, temp16b, temp32a);
    temp48len = fast_expansion_sum_zeroelim(temp16clen, temp16c,
                                            temp32alen, temp32a, temp48);
    finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                            temp48, finother);
    finswap = finnow; finnow = finother; finother = finswap;
  }
  if (cdytail != 0.0) {
    cytablen = scale_expansion_zeroelim(4, ab, cdytail, cytab);
    temp16alen = scale_expansion_zeroelim(cytablen, cytab, 2.0 * cdy,
                                          temp16a);

    cytaalen = scale_expansion_zeroelim(4, aa, cdytail, cytaa);
    temp16blen = scale_expansion_zeroelim(cytaalen, cytaa, bdx, temp16b);

    cytbblen = scale_expansion_zeroelim(4, bb, cdytail, cytbb);
    temp16clen = scale_expansion_zeroelim(cytbblen, cytbb, -adx, temp16c);

    temp32alen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                            temp16blen, temp16b, temp32a);
    temp48len = fast_expansion_sum_zeroelim(temp16clen, temp16c,
                                            temp32alen, temp32a, temp48);
    finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                            temp48, finother);
    finswap = finnow; finnow = finother; finother = finswap;
  }

  if ((adxtail != 0.0) || (adytail != 0.0)) {
    if ((bdxtail != 0.0) || (bdytail != 0.0)
        || (cdxtail != 0.0) || (cdytail != 0.0)) {
      Two_Product(bdxtail, cdy, ti1, ti0);
      Two_Product(bdx, cdytail, tj1, tj0);
      Two_Two_Sum(ti1, ti0, tj1, tj0, u3, u[2], u[1], u[0]);
      u[3] = u3;
      negate = -bdy;
      Two_Product(cdxtail, negate, ti1, ti0);
      negate = -bdytail;
      Two_Product(cdx, negate, tj1, tj0);
      Two_Two_Sum(ti1, ti0, tj1, tj0, v3, v[2], v[1], v[0]);
      v[3] = v3;
      bctlen = fast_expansion_sum_zeroelim(4, u, 4, v, bct);

      Two_Product(bdxtail, cdytail, ti1, ti0);
      Two_Product(cdxtail, bdytail, tj1, tj0);
      Two_Two_Diff(ti1, ti0, tj1, tj0, bctt3, bctt[2], bctt[1], bctt[0]);
      bctt[3] = bctt3;
      bcttlen = 4;
    } else {
      bct[0] = 0.0;
      bctlen = 1;
      bctt[0] = 0.0;
      bcttlen = 1;
    }

    if (adxtail != 0.0) {
      temp16alen = scale_expansion_zeroelim(axtbclen, axtbc, adxtail, temp16a);
      axtbctlen = scale_expansion_zeroelim(bctlen, bct, adxtail, axtbct);
      temp32alen = scale_expansion_zeroelim(axtbctlen, axtbct, 2.0 * adx,
                                            temp32a);
      temp48len = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                              temp32alen, temp32a, temp48);
      finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                              temp48, finother);
      finswap = finnow; finnow = finother; finother = finswap;
      if (bdytail != 0.0) {
        temp8len = scale_expansion_zeroelim(4, cc, adxtail, temp8);
        temp16alen = scale_expansion_zeroelim(temp8len, temp8, bdytail,
                                              temp16a);
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp16alen,
                                                temp16a, finother);
        finswap = finnow; finnow = finother; finother = finswap;
      }
      if (cdytail != 0.0) {
        temp8len = scale_expansion_zeroelim(4, bb, -adxtail, temp8);
        temp16alen = scale_expansion_zeroelim(temp8len, temp8, cdytail,
                                              temp16a);
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp16alen,
                                                temp16a, finother);
        finswap = finnow; finnow = finother; finother = finswap;
      }

      temp32alen = scale_expansion_zeroelim(axtbctlen, axtbct, adxtail,
                                            temp32a);
      axtbcttlen = scale_expansion_zeroelim(bcttlen, bctt, adxtail, axtbctt);
      temp16alen = scale_expansion_zeroelim(axtbcttlen, axtbctt, 2.0 * adx,
                                            temp16a);
      temp16blen = scale_expansion_zeroelim(axtbcttlen, axtbctt, adxtail,
                                            temp16b);
      temp32blen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                              temp16blen, temp16b, temp32b);
      temp64len = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                              temp32blen, temp32b, temp64);
      finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp64len,
                                              temp64, finother);
      finswap = finnow; finnow = finother; finother = finswap;
    }
    if (adytail != 0.0) {
      temp16alen = scale_expansion_zeroelim(aytbclen, aytbc, adytail, temp16a);
      aytbctlen = scale_expansion_zeroelim(bctlen, bct, adytail, aytbct);
      temp32alen = scale_expansion_zeroelim(aytbctlen, aytbct, 2.0 * ady,
                                            temp32a);
      temp48len = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                              temp32alen, temp32a, temp48);
      finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                              temp48, finother);
      finswap = finnow; finnow = finother; finother = finswap;


      temp32alen = scale_expansion_zeroelim(aytbctlen, aytbct, adytail,
                                            temp32a);
      aytbcttlen = scale_expansion_zeroelim(bcttlen, bctt, adytail, aytbctt);
      temp16alen = scale_expansion_zeroelim(aytbcttlen, aytbctt, 2.0 * ady,
                                            temp16a);
      temp16blen = scale_expansion_zeroelim(aytbcttlen, aytbctt, adytail,
                                            temp16b);
      temp32blen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                              temp16blen, temp16b, temp32b);
      temp64len = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                              temp32blen, temp32b, temp64);
      finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp64len,
                                              temp64, finother);
      finswap = finnow; finnow = finother; finother = finswap;
    }
  }
  if ((bdxtail != 0.0) || (bdytail != 0.0)) {
    if ((cdxtail != 0.0) || (cdytail != 0.0)
        || (adxtail != 0.0) || (adytail != 0.0)) {
      Two_Product(cdxtail, ady, ti1, ti0);
      Two_Product(cdx, adytail, tj1, tj0);
      Two_Two_Sum(ti1, ti0, tj1, tj0, u3, u[2], u[1], u[0]);
      u[3] = u3;
      negate = -cdy;
      Two_Product(adxtail, negate, ti1, ti0);
      negate = -cdytail;
      Two_Product(adx, negate, tj1, tj0);
      Two_Two_Sum(ti1, ti0, tj1, tj0, v3, v[2], v[1], v[0]);
      v[3] = v3;
      catlen = fast_expansion_sum_zeroelim(4, u, 4, v, cat);

      Two_Product(cdxtail, adytail, ti1, ti0);
      Two_Product(adxtail, cdytail, tj1, tj0);
      Two_Two_Diff(ti1, ti0, tj1, tj0, catt3, catt[2], catt[1], catt[0]);
      catt[3] = catt3;
      cattlen = 4;
    } else {
      cat[0] = 0.0;
      catlen = 1;
      catt[0] = 0.0;
      cattlen = 1;
    }

    if (bdxtail != 0.0) {
      temp16alen = scale_expansion_zeroelim(bxtcalen, bxtca, bdxtail, temp16a);
      bxtcatlen = scale_expansion_zeroelim(catlen, cat, bdxtail, bxtcat);
      temp32alen = scale_expansion_zeroelim(bxtcatlen, bxtcat, 2.0 * bdx,
                                            temp32a);
      temp48len = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                              temp32alen, temp32a, temp48);
      finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                              temp48, finother);
      finswap = finnow; finnow = finother; finother = finswap;
      if (cdytail != 0.0) {
        temp8len = scale_expansion_zeroelim(4, aa, bdxtail, temp8);
        temp16alen = scale_expansion_zeroelim(temp8len, temp8, cdytail,
                                              temp16a);
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp16alen,
                                                temp16a, finother);
        finswap = finnow; finnow = finother; finother = finswap;
      }
      if (adytail != 0.0) {
        temp8len = scale_expansion_zeroelim(4, cc, -bdxtail, temp8);
        temp16alen = scale_expansion_zeroelim(temp8len, temp8, adytail,
                                              temp16a);
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp16alen,
                                                temp16a, finother);
        finswap = finnow; finnow = finother; finother = finswap;
      }

      temp32alen = scale_expansion_zeroelim(bxtcatlen, bxtcat, bdxtail,
                                            temp32a);
      bxtcattlen = scale_expansion_zeroelim(cattlen, catt, bdxtail, bxtcatt);
      temp16alen = scale_expansion_zeroelim(bxtcattlen, bxtcatt, 2.0 * bdx,
                                            temp16a);
      temp16blen = scale_expansion_zeroelim(bxtcattlen, bxtcatt, bdxtail,
                                            temp16b);
      temp32blen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                              temp16blen, temp16b, temp32b);
      temp64len = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                              temp32blen, temp32b, temp64);
      finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp64len,
                                              temp64, finother);
      finswap = finnow; finnow = finother; finother = finswap;
    }
    if (bdytail != 0.0) {
      temp16alen = scale_expansion_zeroelim(bytcalen, bytca, bdytail, temp16a);
      bytcatlen = scale_expansion_zeroelim(catlen, cat, bdytail, bytcat);
      temp32alen = scale_expansion_zeroelim(bytcatlen, bytcat, 2.0 * bdy,
                                            temp32a);
      temp48len = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                              temp32alen, temp32a, temp48);
      finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                              temp48, finother);
      finswap = finnow; finnow = finother; finother = finswap;


      temp32alen = scale_expansion_zeroelim(bytcatlen, bytcat, bdytail,
                                            temp32a);
      bytcattlen = scale_expansion_zeroelim(cattlen, catt, bdytail, bytcatt);
      temp16alen = scale_expansion_zeroelim(bytcattlen, bytcatt, 2.0 * bdy,
                                            temp16a);
      temp16blen = scale_expansion_zeroelim(bytcattlen, bytcatt, bdytail,
                                            temp16b);
      temp32blen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                              temp16blen, temp16b, temp32b);
      temp64len = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                              temp32blen, temp32b, temp64);
      finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp64len,
                                              temp64, finother);
      finswap = finnow; finnow = finother; finother = finswap;
    }
  }
  if ((cdxtail != 0.0) || (cdytail != 0.0)) {
    if ((adxtail != 0.0) || (adytail != 0.0)
        || (bdxtail != 0.0) || (bdytail != 0.0)) {
      Two_Product(adxtail, bdy, ti1, ti0);
      Two_Product(adx, bdytail, tj1, tj0);
      Two_Two_Sum(ti1, ti0, tj1, tj0, u3, u[2], u[1], u[0]);
      u[3] = u3;
      negate = -ady;
      Two_Product(bdxtail, negate, ti1, ti0);
      negate = -adytail;
      Two_Product(bdx, negate, tj1, tj0);
      Two_Two_Sum(ti1, ti0, tj1, tj0, v3, v[2], v[1], v[0]);
      v[3] = v3;
      abtlen = fast_expansion_sum_zeroelim(4, u, 4, v, abt);

      Two_Product(adxtail, bdytail, ti1, ti0);
      Two_Product(bdxtail, adytail, tj1, tj0);
      Two_Two_Diff(ti1, ti0, tj1, tj0, abtt3, abtt[2], abtt[1], abtt[0]);
      abtt[3] = abtt3;
      abttlen = 4;
    } else {
      abt[0] = 0.0;
      abtlen = 1;
      abtt[0] = 0.0;
      abttlen = 1;
    }

    if (cdxtail != 0.0) {
      temp16alen = scale_expansion_zeroelim(cxtablen, cxtab, cdxtail, temp16a);
      cxtabtlen = scale_expansion_zeroelim(abtlen, abt, cdxtail, cxtabt);
      temp32alen = scale_expansion_zeroelim(cxtabtlen, cxtabt, 2.0 * cdx,
                                            temp32a);
      temp48len = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                              temp32alen, temp32a, temp48);
      finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                              temp48, finother);
      finswap = finnow; finnow = finother; finother = finswap;
      if (adytail != 0.0) {
        temp8len = scale_expansion_zeroelim(4, bb, cdxtail, temp8);
        temp16alen = scale_expansion_zeroelim(temp8len, temp8, adytail,
                                              temp16a);
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp16alen,
                                                temp16a, finother);
        finswap = finnow; finnow = finother; finother = finswap;
      }
      if (bdytail != 0.0) {
        temp8len = scale_expansion_zeroelim(4, aa, -cdxtail, temp8);
        temp16alen = scale_expansion_zeroelim(temp8len, temp8, bdytail,
                                              temp16a);
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp16alen,
                                                temp16a, finother);
        finswap = finnow; finnow = finother; finother = finswap;
      }

      temp32alen = scale_expansion_zeroelim(cxtabtlen, cxtabt, cdxtail,
                                            temp32a);
      cxtabttlen = scale_expansion_zeroelim(abttlen, abtt, cdxtail, cxtabtt);
      temp16alen = scale_expansion_zeroelim(cxtabttlen, cxtabtt, 2.0 * cdx,
                                            temp16a);
      temp16blen = scale_expansion_zeroelim(cxtabttlen, cxtabtt, cdxtail,
                                            temp16b);
      temp32blen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                              temp16blen, temp16b, temp32b);
      temp64len = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                              temp32blen, temp32b, temp64);
      finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp64len,
                                              temp64, finother);
      finswap = finnow; finnow = finother; finother = finswap;
    }
    if (cdytail != 0.0) {
      temp16alen = scale_expansion_zeroelim(cytablen, cytab, cdytail, temp16a);
      cytabtlen = scale_expansion_zeroelim(abtlen, abt, cdytail, cytabt);
      temp32alen = scale_expansion_zeroelim(cytabtlen, cytabt, 2.0 * cdy,
                                            temp32a);
      temp48len = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                              temp32alen, temp32a, temp48);
      finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                              temp48, finother);
      finswap = finnow; finnow = finother; finother = finswap;


      temp32alen = scale_expansion_zeroelim(cytabtlen, cytabt, cdytail,
                                            temp32a);
      cytabttlen = scale_expansion_zeroelim(abttlen, abtt, cdytail, cytabtt);
      temp16alen = scale_expansion_zeroelim(cytabttlen, cytabtt, 2.0 * cdy,
                                            temp16a);
      temp16blen = scale_expansion_zeroelim(cytabttlen, cytabtt, cdytail,
                                            temp16b);
      temp32blen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                              temp16blen, temp16b, temp32b);
      temp64len = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                              temp32blen, temp32b, temp64);
      finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp64len,
                                              temp64, finother);
      finswap = finnow; finnow = finother; finother = finswap;
    }
  }

  return finnow[finlength - 1];
}

sgFloat incircle(struct mesh *m, struct behavior *b,
              vertex pa, vertex pb, vertex pc, vertex pd)
{
  sgFloat adx, bdx, cdx, ady, bdy, cdy;
  sgFloat bdxcdy, cdxbdy, cdxady, adxcdy, adxbdy, bdxady;
  sgFloat alift, blift, clift;
  sgFloat det;
  sgFloat permanent, errbound;

  m->incirclecount++;

  adx = pa[0] - pd[0];
  bdx = pb[0] - pd[0];
  cdx = pc[0] - pd[0];
  ady = pa[1] - pd[1];
  bdy = pb[1] - pd[1];
  cdy = pc[1] - pd[1];

  bdxcdy = bdx * cdy;
  cdxbdy = cdx * bdy;
  alift = adx * adx + ady * ady;

  cdxady = cdx * ady;
  adxcdy = adx * cdy;
  blift = bdx * bdx + bdy * bdy;

  adxbdy = adx * bdy;
  bdxady = bdx * ady;
  clift = cdx * cdx + cdy * cdy;

  det = alift * (bdxcdy - cdxbdy)
      + blift * (cdxady - adxcdy)
      + clift * (adxbdy - bdxady);

  if (b->noexact) {
    return det;
  }

  permanent = (Absolute(bdxcdy) + Absolute(cdxbdy)) * alift
            + (Absolute(cdxady) + Absolute(adxcdy)) * blift
            + (Absolute(adxbdy) + Absolute(bdxady)) * clift;
  errbound = iccerrboundA * permanent;
  if ((det > errbound) || (-det > errbound)) {
    return det;
  }

  return incircleadapt(pa, pb, pc, pd, permanent);
}

sgFloat orient3dadapt(vertex pa, vertex pb, vertex pc, vertex pd,
                   sgFloat aheight, sgFloat bheight, sgFloat cheight, sgFloat dheight,
                   sgFloat permanent)
{
   sgFloat adx, bdx, cdx, ady, bdy, cdy, adheight, bdheight, cdheight;
  sgFloat det, errbound;

   sgFloat bdxcdy1, cdxbdy1, cdxady1, adxcdy1, adxbdy1, bdxady1;
  sgFloat bdxcdy0, cdxbdy0, cdxady0, adxcdy0, adxbdy0, bdxady0;
  sgFloat bc[4], ca[4], ab[4];
   sgFloat bc3, ca3, ab3;
  sgFloat adet[8], bdet[8], cdet[8];
  int alen, blen, clen;
  sgFloat abdet[16];
  int ablen;
  sgFloat *finnow, *finother, *finswap;
  sgFloat fin1[192], fin2[192];
  int finlength;

  sgFloat adxtail, bdxtail, cdxtail;
  sgFloat adytail, bdytail, cdytail;
  sgFloat adheighttail, bdheighttail, cdheighttail;
   sgFloat at_blarge, at_clarge;
   sgFloat bt_clarge, bt_alarge;
   sgFloat ct_alarge, ct_blarge;
  sgFloat at_b[4], at_c[4], bt_c[4], bt_a[4], ct_a[4], ct_b[4];
  int at_blen, at_clen, bt_clen, bt_alen, ct_alen, ct_blen;
   sgFloat bdxt_cdy1, cdxt_bdy1, cdxt_ady1;
   sgFloat adxt_cdy1, adxt_bdy1, bdxt_ady1;
  sgFloat bdxt_cdy0, cdxt_bdy0, cdxt_ady0;
  sgFloat adxt_cdy0, adxt_bdy0, bdxt_ady0;
   sgFloat bdyt_cdx1, cdyt_bdx1, cdyt_adx1;
   sgFloat adyt_cdx1, adyt_bdx1, bdyt_adx1;
  sgFloat bdyt_cdx0, cdyt_bdx0, cdyt_adx0;
  sgFloat adyt_cdx0, adyt_bdx0, bdyt_adx0;
  sgFloat bct[8], cat[8], abt[8];
  int bctlen, catlen, abtlen;
   sgFloat bdxt_cdyt1, cdxt_bdyt1, cdxt_adyt1;
   sgFloat adxt_cdyt1, adxt_bdyt1, bdxt_adyt1;
  sgFloat bdxt_cdyt0, cdxt_bdyt0, cdxt_adyt0;
  sgFloat adxt_cdyt0, adxt_bdyt0, bdxt_adyt0;
  sgFloat u[4], v[12], w[16];
   sgFloat u3;
  int vlength, wlength;
  sgFloat negate;

   sgFloat bvirt;
  sgFloat avirt, bround, around;
   sgFloat c;
   sgFloat abig;
  sgFloat ahi, alo, bhi, blo;
  sgFloat err1, err2, err3;
   sgFloat _i, _j, _k;
  sgFloat _0;

  adx = (sgFloat) (pa[0] - pd[0]);
  bdx = (sgFloat) (pb[0] - pd[0]);
  cdx = (sgFloat) (pc[0] - pd[0]);
  ady = (sgFloat) (pa[1] - pd[1]);
  bdy = (sgFloat) (pb[1] - pd[1]);
  cdy = (sgFloat) (pc[1] - pd[1]);
  adheight = (sgFloat) (aheight - dheight);
  bdheight = (sgFloat) (bheight - dheight);
  cdheight = (sgFloat) (cheight - dheight);

  Two_Product(bdx, cdy, bdxcdy1, bdxcdy0);
  Two_Product(cdx, bdy, cdxbdy1, cdxbdy0);
  Two_Two_Diff(bdxcdy1, bdxcdy0, cdxbdy1, cdxbdy0, bc3, bc[2], bc[1], bc[0]);
  bc[3] = bc3;
  alen = scale_expansion_zeroelim(4, bc, adheight, adet);

  Two_Product(cdx, ady, cdxady1, cdxady0);
  Two_Product(adx, cdy, adxcdy1, adxcdy0);
  Two_Two_Diff(cdxady1, cdxady0, adxcdy1, adxcdy0, ca3, ca[2], ca[1], ca[0]);
  ca[3] = ca3;
  blen = scale_expansion_zeroelim(4, ca, bdheight, bdet);

  Two_Product(adx, bdy, adxbdy1, adxbdy0);
  Two_Product(bdx, ady, bdxady1, bdxady0);
  Two_Two_Diff(adxbdy1, adxbdy0, bdxady1, bdxady0, ab3, ab[2], ab[1], ab[0]);
  ab[3] = ab3;
  clen = scale_expansion_zeroelim(4, ab, cdheight, cdet);

  ablen = fast_expansion_sum_zeroelim(alen, adet, blen, bdet, abdet);
  finlength = fast_expansion_sum_zeroelim(ablen, abdet, clen, cdet, fin1);

  det = estimate(finlength, fin1);
  errbound = o3derrboundB * permanent;
  if ((det >= errbound) || (-det >= errbound)) {
    return det;
  }

  Two_Diff_Tail(pa[0], pd[0], adx, adxtail);
  Two_Diff_Tail(pb[0], pd[0], bdx, bdxtail);
  Two_Diff_Tail(pc[0], pd[0], cdx, cdxtail);
  Two_Diff_Tail(pa[1], pd[1], ady, adytail);
  Two_Diff_Tail(pb[1], pd[1], bdy, bdytail);
  Two_Diff_Tail(pc[1], pd[1], cdy, cdytail);
  Two_Diff_Tail(aheight, dheight, adheight, adheighttail);
  Two_Diff_Tail(bheight, dheight, bdheight, bdheighttail);
  Two_Diff_Tail(cheight, dheight, cdheight, cdheighttail);

  if ((adxtail == 0.0) && (bdxtail == 0.0) && (cdxtail == 0.0) &&
      (adytail == 0.0) && (bdytail == 0.0) && (cdytail == 0.0) &&
      (adheighttail == 0.0) &&
      (bdheighttail == 0.0) &&
      (cdheighttail == 0.0)) {
    return det;
  }

  errbound = o3derrboundC * permanent + resulterrbound * Absolute(det);
  det += (adheight * ((bdx * cdytail + cdy * bdxtail) -
                      (bdy * cdxtail + cdx * bdytail)) +
          adheighttail * (bdx * cdy - bdy * cdx)) +
         (bdheight * ((cdx * adytail + ady * cdxtail) -
                      (cdy * adxtail + adx * cdytail)) +
          bdheighttail * (cdx * ady - cdy * adx)) +
         (cdheight * ((adx * bdytail + bdy * adxtail) -
                      (ady * bdxtail + bdx * adytail)) +
          cdheighttail * (adx * bdy - ady * bdx));
  if ((det >= errbound) || (-det >= errbound)) {
    return det;
  }

  finnow = fin1;
  finother = fin2;

  if (adxtail == 0.0) {
    if (adytail == 0.0) {
      at_b[0] = 0.0;
      at_blen = 1;
      at_c[0] = 0.0;
      at_clen = 1;
    } else {
      negate = -adytail;
      Two_Product(negate, bdx, at_blarge, at_b[0]);
      at_b[1] = at_blarge;
      at_blen = 2;
      Two_Product(adytail, cdx, at_clarge, at_c[0]);
      at_c[1] = at_clarge;
      at_clen = 2;
    }
  } else {
    if (adytail == 0.0) {
      Two_Product(adxtail, bdy, at_blarge, at_b[0]);
      at_b[1] = at_blarge;
      at_blen = 2;
      negate = -adxtail;
      Two_Product(negate, cdy, at_clarge, at_c[0]);
      at_c[1] = at_clarge;
      at_clen = 2;
    } else {
      Two_Product(adxtail, bdy, adxt_bdy1, adxt_bdy0);
      Two_Product(adytail, bdx, adyt_bdx1, adyt_bdx0);
      Two_Two_Diff(adxt_bdy1, adxt_bdy0, adyt_bdx1, adyt_bdx0,
                   at_blarge, at_b[2], at_b[1], at_b[0]);
      at_b[3] = at_blarge;
      at_blen = 4;
      Two_Product(adytail, cdx, adyt_cdx1, adyt_cdx0);
      Two_Product(adxtail, cdy, adxt_cdy1, adxt_cdy0);
      Two_Two_Diff(adyt_cdx1, adyt_cdx0, adxt_cdy1, adxt_cdy0,
                   at_clarge, at_c[2], at_c[1], at_c[0]);
      at_c[3] = at_clarge;
      at_clen = 4;
    }
  }
  if (bdxtail == 0.0) {
    if (bdytail == 0.0) {
      bt_c[0] = 0.0;
      bt_clen = 1;
      bt_a[0] = 0.0;
      bt_alen = 1;
    } else {
      negate = -bdytail;
      Two_Product(negate, cdx, bt_clarge, bt_c[0]);
      bt_c[1] = bt_clarge;
      bt_clen = 2;
      Two_Product(bdytail, adx, bt_alarge, bt_a[0]);
      bt_a[1] = bt_alarge;
      bt_alen = 2;
    }
  } else {
    if (bdytail == 0.0) {
      Two_Product(bdxtail, cdy, bt_clarge, bt_c[0]);
      bt_c[1] = bt_clarge;
      bt_clen = 2;
      negate = -bdxtail;
      Two_Product(negate, ady, bt_alarge, bt_a[0]);
      bt_a[1] = bt_alarge;
      bt_alen = 2;
    } else {
      Two_Product(bdxtail, cdy, bdxt_cdy1, bdxt_cdy0);
      Two_Product(bdytail, cdx, bdyt_cdx1, bdyt_cdx0);
      Two_Two_Diff(bdxt_cdy1, bdxt_cdy0, bdyt_cdx1, bdyt_cdx0,
                   bt_clarge, bt_c[2], bt_c[1], bt_c[0]);
      bt_c[3] = bt_clarge;
      bt_clen = 4;
      Two_Product(bdytail, adx, bdyt_adx1, bdyt_adx0);
      Two_Product(bdxtail, ady, bdxt_ady1, bdxt_ady0);
      Two_Two_Diff(bdyt_adx1, bdyt_adx0, bdxt_ady1, bdxt_ady0,
                  bt_alarge, bt_a[2], bt_a[1], bt_a[0]);
      bt_a[3] = bt_alarge;
      bt_alen = 4;
    }
  }
  if (cdxtail == 0.0) {
    if (cdytail == 0.0) {
      ct_a[0] = 0.0;
      ct_alen = 1;
      ct_b[0] = 0.0;
      ct_blen = 1;
    } else {
      negate = -cdytail;
      Two_Product(negate, adx, ct_alarge, ct_a[0]);
      ct_a[1] = ct_alarge;
      ct_alen = 2;
      Two_Product(cdytail, bdx, ct_blarge, ct_b[0]);
      ct_b[1] = ct_blarge;
      ct_blen = 2;
    }
  } else {
    if (cdytail == 0.0) {
      Two_Product(cdxtail, ady, ct_alarge, ct_a[0]);
      ct_a[1] = ct_alarge;
      ct_alen = 2;
      negate = -cdxtail;
      Two_Product(negate, bdy, ct_blarge, ct_b[0]);
      ct_b[1] = ct_blarge;
      ct_blen = 2;
    } else {
      Two_Product(cdxtail, ady, cdxt_ady1, cdxt_ady0);
      Two_Product(cdytail, adx, cdyt_adx1, cdyt_adx0);
      Two_Two_Diff(cdxt_ady1, cdxt_ady0, cdyt_adx1, cdyt_adx0,
                   ct_alarge, ct_a[2], ct_a[1], ct_a[0]);
      ct_a[3] = ct_alarge;
      ct_alen = 4;
      Two_Product(cdytail, bdx, cdyt_bdx1, cdyt_bdx0);
      Two_Product(cdxtail, bdy, cdxt_bdy1, cdxt_bdy0);
      Two_Two_Diff(cdyt_bdx1, cdyt_bdx0, cdxt_bdy1, cdxt_bdy0,
                   ct_blarge, ct_b[2], ct_b[1], ct_b[0]);
      ct_b[3] = ct_blarge;
      ct_blen = 4;
    }
  }

  bctlen = fast_expansion_sum_zeroelim(bt_clen, bt_c, ct_blen, ct_b, bct);
  wlength = scale_expansion_zeroelim(bctlen, bct, adheight, w);
  finlength = fast_expansion_sum_zeroelim(finlength, finnow, wlength, w,
                                          finother);
  finswap = finnow; finnow = finother; finother = finswap;

  catlen = fast_expansion_sum_zeroelim(ct_alen, ct_a, at_clen, at_c, cat);
  wlength = scale_expansion_zeroelim(catlen, cat, bdheight, w);
  finlength = fast_expansion_sum_zeroelim(finlength, finnow, wlength, w,
                                          finother);
  finswap = finnow; finnow = finother; finother = finswap;

  abtlen = fast_expansion_sum_zeroelim(at_blen, at_b, bt_alen, bt_a, abt);
  wlength = scale_expansion_zeroelim(abtlen, abt, cdheight, w);
  finlength = fast_expansion_sum_zeroelim(finlength, finnow, wlength, w,
                                          finother);
  finswap = finnow; finnow = finother; finother = finswap;

  if (adheighttail != 0.0) {
    vlength = scale_expansion_zeroelim(4, bc, adheighttail, v);
    finlength = fast_expansion_sum_zeroelim(finlength, finnow, vlength, v,
                                            finother);
    finswap = finnow; finnow = finother; finother = finswap;
  }
  if (bdheighttail != 0.0) {
    vlength = scale_expansion_zeroelim(4, ca, bdheighttail, v);
    finlength = fast_expansion_sum_zeroelim(finlength, finnow, vlength, v,
                                            finother);
    finswap = finnow; finnow = finother; finother = finswap;
  }
  if (cdheighttail != 0.0) {
    vlength = scale_expansion_zeroelim(4, ab, cdheighttail, v);
    finlength = fast_expansion_sum_zeroelim(finlength, finnow, vlength, v,
                                            finother);
    finswap = finnow; finnow = finother; finother = finswap;
  }

  if (adxtail != 0.0) {
    if (bdytail != 0.0) {
      Two_Product(adxtail, bdytail, adxt_bdyt1, adxt_bdyt0);
      Two_One_Product(adxt_bdyt1, adxt_bdyt0, cdheight, u3, u[2], u[1], u[0]);
      u[3] = u3;
      finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                              finother);
      finswap = finnow; finnow = finother; finother = finswap;
      if (cdheighttail != 0.0) {
        Two_One_Product(adxt_bdyt1, adxt_bdyt0, cdheighttail,
                        u3, u[2], u[1], u[0]);
        u[3] = u3;
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                                finother);
        finswap = finnow; finnow = finother; finother = finswap;
      }
    }
    if (cdytail != 0.0) {
      negate = -adxtail;
      Two_Product(negate, cdytail, adxt_cdyt1, adxt_cdyt0);
      Two_One_Product(adxt_cdyt1, adxt_cdyt0, bdheight, u3, u[2], u[1], u[0]);
      u[3] = u3;
      finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                              finother);
      finswap = finnow; finnow = finother; finother = finswap;
      if (bdheighttail != 0.0) {
        Two_One_Product(adxt_cdyt1, adxt_cdyt0, bdheighttail,
                        u3, u[2], u[1], u[0]);
        u[3] = u3;
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                                finother);
        finswap = finnow; finnow = finother; finother = finswap;
      }
    }
  }
  if (bdxtail != 0.0) {
    if (cdytail != 0.0) {
      Two_Product(bdxtail, cdytail, bdxt_cdyt1, bdxt_cdyt0);
      Two_One_Product(bdxt_cdyt1, bdxt_cdyt0, adheight, u3, u[2], u[1], u[0]);
      u[3] = u3;
      finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                              finother);
      finswap = finnow; finnow = finother; finother = finswap;
      if (adheighttail != 0.0) {
        Two_One_Product(bdxt_cdyt1, bdxt_cdyt0, adheighttail,
                        u3, u[2], u[1], u[0]);
        u[3] = u3;
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                                finother);
        finswap = finnow; finnow = finother; finother = finswap;
      }
    }
    if (adytail != 0.0) {
      negate = -bdxtail;
      Two_Product(negate, adytail, bdxt_adyt1, bdxt_adyt0);
      Two_One_Product(bdxt_adyt1, bdxt_adyt0, cdheight, u3, u[2], u[1], u[0]);
      u[3] = u3;
      finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                              finother);
      finswap = finnow; finnow = finother; finother = finswap;
      if (cdheighttail != 0.0) {
        Two_One_Product(bdxt_adyt1, bdxt_adyt0, cdheighttail,
                        u3, u[2], u[1], u[0]);
        u[3] = u3;
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                                finother);
        finswap = finnow; finnow = finother; finother = finswap;
      }
    }
  }
  if (cdxtail != 0.0) {
    if (adytail != 0.0) {
      Two_Product(cdxtail, adytail, cdxt_adyt1, cdxt_adyt0);
      Two_One_Product(cdxt_adyt1, cdxt_adyt0, bdheight, u3, u[2], u[1], u[0]);
      u[3] = u3;
      finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                              finother);
      finswap = finnow; finnow = finother; finother = finswap;
      if (bdheighttail != 0.0) {
        Two_One_Product(cdxt_adyt1, cdxt_adyt0, bdheighttail,
                        u3, u[2], u[1], u[0]);
        u[3] = u3;
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                                finother);
        finswap = finnow; finnow = finother; finother = finswap;
      }
    }
    if (bdytail != 0.0) {
      negate = -cdxtail;
      Two_Product(negate, bdytail, cdxt_bdyt1, cdxt_bdyt0);
      Two_One_Product(cdxt_bdyt1, cdxt_bdyt0, adheight, u3, u[2], u[1], u[0]);
      u[3] = u3;
      finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                              finother);
      finswap = finnow; finnow = finother; finother = finswap;
      if (adheighttail != 0.0) {
        Two_One_Product(cdxt_bdyt1, cdxt_bdyt0, adheighttail,
                        u3, u[2], u[1], u[0]);
        u[3] = u3;
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                                finother);
        finswap = finnow; finnow = finother; finother = finswap;
      }
    }
  }

  if (adheighttail != 0.0) {
    wlength = scale_expansion_zeroelim(bctlen, bct, adheighttail, w);
    finlength = fast_expansion_sum_zeroelim(finlength, finnow, wlength, w,
                                            finother);
    finswap = finnow; finnow = finother; finother = finswap;
  }
  if (bdheighttail != 0.0) {
    wlength = scale_expansion_zeroelim(catlen, cat, bdheighttail, w);
    finlength = fast_expansion_sum_zeroelim(finlength, finnow, wlength, w,
                                            finother);
    finswap = finnow; finnow = finother; finother = finswap;
  }
  if (cdheighttail != 0.0) {
    wlength = scale_expansion_zeroelim(abtlen, abt, cdheighttail, w);
    finlength = fast_expansion_sum_zeroelim(finlength, finnow, wlength, w,
                                            finother);
    finswap = finnow; finnow = finother; finother = finswap;
  }

  return finnow[finlength - 1];
}

sgFloat orient3d(struct mesh *m, struct behavior *b,
              vertex pa, vertex pb, vertex pc, vertex pd,
              sgFloat aheight, sgFloat bheight, sgFloat cheight, sgFloat dheight)
{
  sgFloat adx, bdx, cdx, ady, bdy, cdy, adheight, bdheight, cdheight;
  sgFloat bdxcdy, cdxbdy, cdxady, adxcdy, adxbdy, bdxady;
  sgFloat det;
  sgFloat permanent, errbound;

  m->orient3dcount++;

  adx = pa[0] - pd[0];
  bdx = pb[0] - pd[0];
  cdx = pc[0] - pd[0];
  ady = pa[1] - pd[1];
  bdy = pb[1] - pd[1];
  cdy = pc[1] - pd[1];
  adheight = aheight - dheight;
  bdheight = bheight - dheight;
  cdheight = cheight - dheight;

  bdxcdy = bdx * cdy;
  cdxbdy = cdx * bdy;

  cdxady = cdx * ady;
  adxcdy = adx * cdy;

  adxbdy = adx * bdy;
  bdxady = bdx * ady;

  det = adheight * (bdxcdy - cdxbdy) 
      + bdheight * (cdxady - adxcdy)
      + cdheight * (adxbdy - bdxady);

  if (b->noexact) {
    return det;
  }

  permanent = (Absolute(bdxcdy) + Absolute(cdxbdy)) * Absolute(adheight)
            + (Absolute(cdxady) + Absolute(adxcdy)) * Absolute(bdheight)
            + (Absolute(adxbdy) + Absolute(bdxady)) * Absolute(cdheight);
  errbound = o3derrboundA * permanent;
  if ((det > errbound) || (-det > errbound)) {
    return det;
  }

  return orient3dadapt(pa, pb, pc, pd, aheight, bheight, cheight, dheight,
                       permanent);
}

sgFloat nonregular(struct mesh *m, struct behavior *b,
                vertex pa, vertex pb, vertex pc, vertex pd)
{
  if (b->weighted == 0) {
    return incircle(m, b, pa, pb, pc, pd);
  } else if (b->weighted == 1) {
    return orient3d(m, b, pa, pb, pc, pd,
                    pa[0] * pa[0] + pa[1] * pa[1] - pa[2],
                    pb[0] * pb[0] + pb[1] * pb[1] - pb[2],
                    pc[0] * pc[0] + pc[1] * pc[1] - pc[2],
                    pd[0] * pd[0] + pd[1] * pd[1] - pd[2]);
  } else {
    return orient3d(m, b, pa, pb, pc, pd, pa[2], pb[2], pc[2], pd[2]);
  }
}

void findcircumcenter(struct mesh *m, struct behavior *b,
                      vertex torg, vertex tdest, vertex tapex,
                      vertex circumcenter, sgFloat *xi, sgFloat *eta, sgFloat *minedge,
                      int offcenter)
{
  sgFloat xdo, ydo, xao, yao;
  sgFloat dodist, aodist, dadist;
  sgFloat denominator;
  sgFloat dx, dy, dxoff, dyoff;

  m->circumcentercount++;

  /* Compute the circumcenter of the triangle. */
  xdo = tdest[0] - torg[0];
  ydo = tdest[1] - torg[1];
  xao = tapex[0] - torg[0];
  yao = tapex[1] - torg[1];
  dodist = xdo * xdo + ydo * ydo;
  aodist = xao * xao + yao * yao;
  dadist = (tdest[0] - tapex[0]) * (tdest[0] - tapex[0]) +
           (tdest[1] - tapex[1]) * (tdest[1] - tapex[1]);
  if (b->noexact) {
    denominator = 0.5 / (xdo * yao - xao * ydo);
  } else {
    /* Use the counterclockwise() routine to ensure a positive (and */
    /*   reasonably accurate) result, avoiding any possibility of   */
    /*   division by zero.                                          */
    denominator = 0.5 / counterclockwise(m, b, tdest, tapex, torg);
    /* Don't count the above as an orientation test. */
    m->counterclockcount--;
  }
  dx = (yao * dodist - ydo * aodist) * denominator;
  dy = (xdo * aodist - xao * dodist) * denominator;

  if ((dodist < aodist) && (dodist < dadist)) {
    *minedge = dodist;
    if (offcenter && (b->offconstant > 0.0)) {
      /* Find the position of the off-center, as described by Alper Ungor. */
      dxoff = 0.5 * xdo - b->offconstant * ydo;
      dyoff = 0.5 * ydo + b->offconstant * xdo;
      /* If the off-center is closer to the origin than the */
      /*   circumcenter, use the off-center instead.        */
      if (dxoff * dxoff + dyoff * dyoff < dx * dx + dy * dy) {
        dx = dxoff;
        dy = dyoff;
      }
    }
  } else if (aodist < dadist) {
    *minedge = aodist;
    if (offcenter && (b->offconstant > 0.0)) {
      dxoff = 0.5 * xao + b->offconstant * yao;
      dyoff = 0.5 * yao - b->offconstant * xao;
      /* If the off-center is closer to the origin than the */
      /*   circumcenter, use the off-center instead.        */
      if (dxoff * dxoff + dyoff * dyoff < dx * dx + dy * dy) {
        dx = dxoff;
        dy = dyoff;
      }
    }
  } else {
    *minedge = dadist;
    if (offcenter && (b->offconstant > 0.0)) {
      dxoff = 0.5 * (tapex[0] - tdest[0]) -
              b->offconstant * (tapex[1] - tdest[1]);
      dyoff = 0.5 * (tapex[1] - tdest[1]) +
              b->offconstant * (tapex[0] - tdest[0]);
      /* If the off-center is closer to the destination than the */
      /*   circumcenter, use the off-center instead.             */
      if (dxoff * dxoff + dyoff * dyoff <
          (dx - xdo) * (dx - xdo) + (dy - ydo) * (dy - ydo)) {
        dx = xdo + dxoff;
        dy = ydo + dyoff;
      }
    }
  }

  circumcenter[0] = torg[0] + dx;
  circumcenter[1] = torg[1] + dy;

  *xi = (yao * dx - xao * dy) * (2.0 * denominator);
  *eta = (xdo * dy - ydo * dx) * (2.0 * denominator);
}

void triangleinit(struct mesh *m)
{
  m->vertices.maxitems = m->triangles.maxitems = m->subsegs.maxitems =
    m->viri.maxitems = m->badsubsegs.maxitems = m->badtriangles.maxitems =
    m->flipstackers.maxitems = m->splaynodes.maxitems = 0l;
  m->vertices.itembytes = m->triangles.itembytes = m->subsegs.itembytes =
    m->viri.itembytes = m->badsubsegs.itembytes = m->badtriangles.itembytes =
    m->flipstackers.itembytes = m->splaynodes.itembytes = 0;
  m->recenttri.tri = (triangle *) NULL; /* No triangle has been visited yet. */
  m->undeads = 0;                       /* No eliminated input vertices yet. */
  m->samples = 1;         /* Point location should take at least one sample. */
  m->checksegments = 0;   /* There are no segments in the triangulation yet. */
  m->checkquality = 0;     /* The quality triangulation stage has not begun. */
  m->incirclecount = m->counterclockcount = m->orient3dcount = 0;
  m->hyperbolacount = m->circletopcount = m->circumcentercount = 0;
  randomseed = 1;

  exactinit();                     /* Initialize exact arithmetic constants. */
}

unsigned long randomnation(unsigned int choices)
{
  randomseed = (randomseed * 1366l + 150889l) % 714025l;
  return randomseed / (714025l / choices + 1);
}

#ifndef REDUCED

void checkmesh(struct mesh *m, struct behavior *b)
{
  struct otri triangleloop;
  struct otri oppotri, oppooppotri;
  vertex triorg, tridest, triapex;
  vertex oppoorg, oppodest;
  int horrors;
  int saveexact;
  triangle ptr;                         /* Temporary variable used by sym(). */

  /* Temporarily turn on exact arithmetic if it's off. */
  saveexact = b->noexact;
  b->noexact = 0;
  if (!b->quiet) {
    //  TRACE("  Checking consistency of mesh...\n");
  }
  horrors = 0;
  /* Run through the list of triangles, checking each one. */
  traversalinit(&m->triangles);
  triangleloop.tri = triangletraverse(m);
  while (triangleloop.tri != (triangle *) NULL) {
    /* Check all three edges of the triangle. */
    for (triangleloop.orient = 0; triangleloop.orient < 3;
         triangleloop.orient++) {
      org(triangleloop, triorg);
      dest(triangleloop, tridest);
      if (triangleloop.orient == 0) {       /* Only test for inversion once. */
        /* Test if the triangle is flat or inverted. */
        apex(triangleloop, triapex);
        if (counterclockwise(m, b, triorg, tridest, triapex) <= 0.0) {
          //  TRACE("  !! !! Inverted ");
          //printtriangle(m, b, &triangleloop);
          horrors++;
        }
      }
      /* Find the neighboring triangle on this edge. */
      sym(triangleloop, oppotri);
      if (oppotri.tri != m->dummytri) {
        /* Check that the triangle's neighbor knows it's a neighbor. */
        sym(oppotri, oppooppotri);
        if ((triangleloop.tri != oppooppotri.tri)
            || (triangleloop.orient != oppooppotri.orient)) {
          //  TRACE("  !! !! Asymmetric triangle-triangle bond:\n");
          if (triangleloop.tri == oppooppotri.tri) {
            //  TRACE("   (Right triangle, wrong orientation)\n");
          }
          //  TRACE("    First ");
//          printtriangle(m, b, &triangleloop);
          //  TRACE("    Second (nonreciprocating) ");
//          printtriangle(m, b, &oppotri);
          horrors++;
        }
        /* Check that both triangles agree on the identities */
        /*   of their shared vertices.                       */
        org(oppotri, oppoorg);
        dest(oppotri, oppodest);
        if ((triorg != oppodest) || (tridest != oppoorg)) {
          //  TRACE("  !! !! Mismatched edge coordinates between two triangles:\n"
//                 );
          //  TRACE("    First mismatched ");
         // printtriangle(m, b, &triangleloop);
          //  TRACE("    Second mismatched ");
          //printtriangle(m, b, &oppotri);
          horrors++;
        }
      }
    }
    triangleloop.tri = triangletraverse(m);
  }
  if (horrors == 0) {
    if (!b->quiet) {
      //  TRACE("  In my studied opinion, the mesh appears to be consistent.\n");
    }
  } else if (horrors == 1) {
    //  TRACE("  !! !! !! !! Precisely one festering wound discovered.\n");
  } else {
    //  TRACE("  !! !! !! !! %d abominations witnessed.\n", horrors);
  }
  /* Restore the status of exact arithmetic. */
  b->noexact = saveexact;
}

#endif /* not REDUCED */

#ifndef REDUCED

void checkdelaunay(struct mesh *m, struct behavior *b)
{
  struct otri triangleloop;
  struct otri oppotri;
  struct osub opposubseg;
  vertex triorg, tridest, triapex;
  vertex oppoapex;
  int shouldbedelaunay;
  int horrors;
  int saveexact;
  triangle ptr;                         /* Temporary variable used by sym(). */
  subseg sptr;                      /* Temporary variable used by tspivot(). */

  /* Temporarily turn on exact arithmetic if it's off. */
  saveexact = b->noexact;
  b->noexact = 0;
  if (!b->quiet) {
    //  TRACE("  Checking Delaunay property of mesh...\n");
  }
  horrors = 0;
  /* Run through the list of triangles, checking each one. */
  traversalinit(&m->triangles);
  triangleloop.tri = triangletraverse(m);
  while (triangleloop.tri != (triangle *) NULL) {
    /* Check all three edges of the triangle. */
    for (triangleloop.orient = 0; triangleloop.orient < 3;
         triangleloop.orient++) {
      org(triangleloop, triorg);
      dest(triangleloop, tridest);
      apex(triangleloop, triapex);
      sym(triangleloop, oppotri);
      apex(oppotri, oppoapex);
      /* Only test that the edge is locally Delaunay if there is an   */
      /*   adjoining triangle whose pointer is larger (to ensure that */
      /*   each pair isn't tested twice).                             */
      shouldbedelaunay = (oppotri.tri != m->dummytri) &&
            !deadtri(oppotri.tri) && (triangleloop.tri < oppotri.tri) &&
            (triorg != m->infvertex1) && (triorg != m->infvertex2) &&
            (triorg != m->infvertex3) &&
            (tridest != m->infvertex1) && (tridest != m->infvertex2) &&
            (tridest != m->infvertex3) &&
            (triapex != m->infvertex1) && (triapex != m->infvertex2) &&
            (triapex != m->infvertex3) &&
            (oppoapex != m->infvertex1) && (oppoapex != m->infvertex2) &&
            (oppoapex != m->infvertex3);
      if (m->checksegments && shouldbedelaunay) {
        /* If a subsegment separates the triangles, then the edge is */
        /*   constrained, so no local Delaunay test should be done.  */
        tspivot(triangleloop, opposubseg);
        if (opposubseg.ss != m->dummysub){
          shouldbedelaunay = 0;
        }
      }
      if (shouldbedelaunay) {
        if (nonregular(m, b, triorg, tridest, triapex, oppoapex) > 0.0) {
          if (!b->weighted) {
            //  TRACE("  !! !! Non-Delaunay pair of triangles:\n");
            //  TRACE("    First non-Delaunay ");
//            printtriangle(m, b, &triangleloop);
            //  TRACE("    Second non-Delaunay ");
          } else {
            //  TRACE("  !! !! Non-regular pair of triangles:\n");
            //  TRACE("    First non-regular ");
//            printtriangle(m, b, &triangleloop);
            //  TRACE("    Second non-regular ");
          }
 //         printtriangle(m, b, &oppotri);
          horrors++;
        }
      }
    }
    triangleloop.tri = triangletraverse(m);
  }
  if (horrors == 0) {
    if (!b->quiet) {
      //  TRACE(
//  "  By virtue of my perceptive intelligence, I declare the mesh Delaunay.\n");
    }
  } else if (horrors == 1) {
    //  TRACE(
      //   "  !! !! !! !! Precisely one terrifying transgression identified.\n");
  } else {
    //  TRACE("  !! !! !! !! %d obscenities viewed with horror.\n", horrors);
  }
  /* Restore the status of exact arithmetic. */
  b->noexact = saveexact;
}

#endif /* not REDUCED */

#ifndef CDT_ONLY

void enqueuebadtriang(struct mesh *m, struct behavior *b,
                      struct badtriang *badtri)
{
  int queuenumber;
  int i;

  if (b->verbose > 2) {
    //  TRACE("  Queueing bad triangle:\n");
    //  TRACE("    (%.12g, %.12g) (%.12g, %.12g) (%.12g, %.12g)\n",
      //     badtri->triangorg[0], badtri->triangorg[1],
       //    badtri->triangdest[0], badtri->triangdest[1],
       //    badtri->triangapex[0], badtri->triangapex[1]);
  }
  /* Determine the appropriate queue to put the bad triangle into. */
  if (badtri->key > 0.6) {
    queuenumber = (int) (160.0 * (badtri->key - 0.6));
    if (queuenumber > 63) {
      queuenumber = 63;
    }
  } else {
    /* It's not a bad angle; put the triangle in the lowest-priority queue. */
    queuenumber = 0;
  }

  /* Are we inserting into an empty queue? */
  if (m->queuefront[queuenumber] == (struct badtriang *) NULL) {
    /* Yes, we are inserting into an empty queue.     */
    /*   Will this become the highest-priority queue? */
    if (queuenumber > m->firstnonemptyq) {
      /* Yes, this is the highest-priority queue. */
      m->nextnonemptyq[queuenumber] = m->firstnonemptyq;
      m->firstnonemptyq = queuenumber;
    } else {
      /* No, this is not the highest-priority queue. */
      /*   Find the queue with next higher priority. */
      i = queuenumber + 1;
      while (m->queuefront[i] == (struct badtriang *) NULL) {
        i++;
      }
      /* Mark the newly nonempty queue as following a higher-priority queue. */
      m->nextnonemptyq[queuenumber] = m->nextnonemptyq[i];
      m->nextnonemptyq[i] = queuenumber;
    }
    /* Put the bad triangle at the beginning of the (empty) queue. */
    m->queuefront[queuenumber] = badtri;
  } else {
    /* Add the bad triangle to the end of an already nonempty queue. */
    m->queuetail[queuenumber]->nexttriang = badtri;
  }
  /* Maintain a pointer to the last triangle of the queue. */
  m->queuetail[queuenumber] = badtri;
  /* Newly enqueued bad triangle has no successor in the queue. */
  badtri->nexttriang = (struct badtriang *) NULL;
}

#endif /* not CDT_ONLY */

#ifndef CDT_ONLY

void enqueuebadtri(struct mesh *m, struct behavior *b, struct otri *enqtri,
                   sgFloat angle, vertex enqapex, vertex enqorg, vertex enqdest)
{
  struct badtriang *newbad;

  /* Allocate space for the bad triangle. */
  newbad = (struct badtriang *) poolalloc(&m->badtriangles);
  newbad->poortri = encode(*enqtri);
  newbad->key = angle;
  newbad->triangapex = enqapex;
  newbad->triangorg = enqorg;
  newbad->triangdest = enqdest;
  enqueuebadtriang(m, b, newbad);
}

#endif /* not CDT_ONLY */

#ifndef CDT_ONLY

struct badtriang *dequeuebadtriang(struct mesh *m)
{
  struct badtriang *result;

  /* If no queues are nonempty, return NULL. */
  if (m->firstnonemptyq < 0) {
    return (struct badtriang *) NULL;
  }
  /* Find the first triangle of the highest-priority queue. */
  result = m->queuefront[m->firstnonemptyq];
  /* Remove the triangle from the queue. */
  m->queuefront[m->firstnonemptyq] = result->nexttriang;
  /* If this queue is now empty, note the new highest-priority */
  /*   nonempty queue.                                         */
  if (result == m->queuetail[m->firstnonemptyq]) {
    m->firstnonemptyq = m->nextnonemptyq[m->firstnonemptyq];
  }
  return result;
}

#endif /* not CDT_ONLY */

#ifndef CDT_ONLY

int under60degrees(struct osub *sub1, struct osub *sub2) {
  vertex segmentapex, v1, v2;
  sgFloat dotprod;

  sorg(*sub1, segmentapex);
  sdest(*sub1, v1);
  sdest(*sub2, v2);
  dotprod = (v2[0] - segmentapex[0]) * (v1[0] - segmentapex[0]) +
            (v2[1] - segmentapex[1]) * (v1[1] - segmentapex[1]);
  return (dotprod > 0.0) &&
         (4.0 * dotprod * dotprod >
          ((v1[0] - segmentapex[0]) * (v1[0] - segmentapex[0]) +
           (v1[1] - segmentapex[1]) * (v1[1] - segmentapex[1])) *
          ((v2[0] - segmentapex[0]) * (v2[0] - segmentapex[0]) +
           (v2[1] - segmentapex[1]) * (v2[1] - segmentapex[1])));
}

#endif /* not CDT_ONLY */

#ifndef CDT_ONLY

int clockwiseseg(struct mesh *m, struct osub *thissub, struct osub *nextsub) {
  struct otri neighbortri;
  triangle ptr;           /* Temporary variable used by sym() and stpivot(). */
  subseg sptr;                      /* Temporary variable used by tspivot(). */

  stpivot(*thissub, neighbortri);
  if (neighbortri.tri == m->dummytri) {
    return 0;
  } else {
    lnextself(neighbortri);
    tspivot(neighbortri, *nextsub);
    while (nextsub->ss == m->dummysub) {
      symself(neighbortri);
      lnextself(neighbortri);
      tspivot(neighbortri, *nextsub);
    }
    ssymself(*nextsub);
    return under60degrees(thissub, nextsub);
  }
}

#endif /* not CDT_ONLY */

#ifndef CDT_ONLY

int counterclockwiseseg(struct mesh *m, struct osub *thissub,
                        struct osub *nextsub) {
  struct otri neighbortri;
  struct osub subsym;
  triangle ptr;           /* Temporary variable used by sym() and stpivot(). */
  subseg sptr;                      /* Temporary variable used by tspivot(). */

  ssym(*thissub, subsym);
  stpivot(subsym, neighbortri);
  if (neighbortri.tri == m->dummytri) {
    return 0;
  } else {
    lprevself(neighbortri);
    tspivot(neighbortri, *nextsub);
    while (nextsub->ss == m->dummysub) {
      symself(neighbortri);
      lprevself(neighbortri);
      tspivot(neighbortri, *nextsub);
    }
    return under60degrees(thissub, nextsub);
  }
}

#endif /* not CDT_ONLY */

#ifndef CDT_ONLY

int splitpermitted(struct mesh *m, struct osub *testsubseg, sgFloat iradius) {
  struct osub cwsubseg, ccwsubseg, cwsubseg2, ccwsubseg2;
  struct osub testsym;
  struct osub startsubseg, nowsubseg;
  vertex suborg, dest1, dest2;
  sgFloat nearestpoweroffour, seglength, prevseglength, edgelength;
  int cwsmall, ccwsmall, cwsmall2, ccwsmall2;
  int orgcluster, destcluster;
  int toosmall;

 sorg(*testsubseg, suborg);
  sdest(*testsubseg, dest1);
  seglength = (dest1[0] - suborg[0]) * (dest1[0] - suborg[0]) +
              (dest1[1] - suborg[1]) * (dest1[1] - suborg[1]);
  nearestpoweroffour = 1.0;
  while (seglength > 2.0 * nearestpoweroffour) {
    nearestpoweroffour *= 4.0;
  }
  while (seglength < 0.5 * nearestpoweroffour) {
    nearestpoweroffour *= 0.25;
  }
  /* If the segment's length is not a power of two, the segment */
  /*   is eligible for splitting.                               */
  if ((nearestpoweroffour > 1.001 * seglength) ||
      (nearestpoweroffour < 0.999 * seglength)) {
    return 1;
  }

  /* Is `testsubseg' part of a subsegment cluster at its origin? */
  cwsmall = clockwiseseg(m, testsubseg, &cwsubseg);
  ccwsmall = cwsmall ? 0 : counterclockwiseseg(m, testsubseg, &ccwsubseg);
  orgcluster = cwsmall || ccwsmall;

  /* Is `testsubseg' part of a subsegment cluster at its destination? */
  ssym(*testsubseg, testsym);
  cwsmall2 = clockwiseseg(m, &testsym, &cwsubseg2);
  ccwsmall2 = cwsmall2 ? 0 : counterclockwiseseg(m, &testsym, &ccwsubseg2);
  destcluster = cwsmall2 || ccwsmall2;

  if (orgcluster == destcluster) {
    /* `testsubseg' is part of two clusters or none, */
    /*   and thus should be split.                   */
    return 1;
  } else if (orgcluster) {
    /* `testsubseg' is part of a cluster at its origin. */
    subsegcopy(*testsubseg, startsubseg);
  } else {
    /* `testsubseg' is part of a cluster at its destination; switch to */
    /*   the symmetric case, so we can use the same code to handle it. */
    subsegcopy(testsym, startsubseg);
    subsegcopy(cwsubseg2, cwsubseg);
    subsegcopy(ccwsubseg2, ccwsubseg);
    cwsmall = cwsmall2;
    ccwsmall = ccwsmall2;
  }

  toosmall = 0;
  if (cwsmall) {
    /* Check the subsegment(s) clockwise from `testsubseg'. */
    subsegcopy(startsubseg, nowsubseg);
    sorg(nowsubseg, suborg);
    sdest(nowsubseg, dest1);
    prevseglength = nearestpoweroffour;
    do {
      /* Is the next subsegment shorter than `startsubseg'? */
      sdest(cwsubseg, dest2);
      seglength = (dest2[0] - suborg[0]) * (dest2[0] - suborg[0]) +
                  (dest2[1] - suborg[1]) * (dest2[1] - suborg[1]);
      if (nearestpoweroffour > 1.001 * seglength) {
        /* It's shorter; it's safe to split `startsubseg'. */
        return 1;
      }
      /* If the current and previous subsegments are split to a length  */
      /*   half that of `startsubseg' (which is a likely consequence if */
      /*   `startsubseg' is split), what will be (the square of) the    */
      /*   length of the free edge between the splitting vertices?      */
      edgelength = 0.5 * nearestpoweroffour *
                   (1 - (((dest1[0] - suborg[0]) * (dest2[0] - suborg[0]) +
                          (dest1[1] - suborg[1]) * (dest2[1] - suborg[1])) /
                         sqrt(prevseglength * seglength)));
      if (edgelength < iradius) {
        /* If this cluster is split, the new edge dest1-dest2 will be     */
        /*   smaller than the insertion radius of the encroaching vertex. */
        /*   Hence, we'd prefer to avoid splitting it if possible.        */
        toosmall = 1;
      }
      if (cwsubseg.ss == startsubseg.ss) {
        /* We've gone all the way around the vertex.  Split the cluster */
        /*   if no edges will be too short.                             */
        return !toosmall;
      }

      /* Find the next subsegment clockwise around the vertex. */
      subsegcopy(cwsubseg, nowsubseg);
      dest1 = dest2;
      prevseglength = seglength;
      cwsmall = clockwiseseg(m, &nowsubseg, &cwsubseg);
    } while (cwsmall);

    /* Prepare to start searching counterclockwise from */
    /*   the starting subsegment.                       */
    ccwsmall = counterclockwiseseg(m, &startsubseg, &ccwsubseg);
  }

  if (ccwsmall) {
    /* Check the subsegment(s) counterclockwise from `testsubseg'. */
    subsegcopy(startsubseg, nowsubseg);
    sorg(nowsubseg, suborg);
    sdest(nowsubseg, dest1);
    prevseglength = nearestpoweroffour;
    do {
      /* Is the next subsegment shorter than `startsubseg'? */
      sdest(ccwsubseg, dest2);
      seglength = (dest2[0] - suborg[0]) * (dest2[0] - suborg[0]) +
                  (dest2[1] - suborg[1]) * (dest2[1] - suborg[1]);
      if (nearestpoweroffour > 1.001 * seglength) {
        /* It's shorter; it's safe to split `startsubseg'. */
        return 1;
      }
      /*   half that of `startsubseg' (which is a likely consequence if */
      /*   `startsubseg' is split), what will be (the square of) the    */
      /*   length of the free edge between the splitting vertices?      */
      edgelength = 0.5 * nearestpoweroffour *
                   (1 - (((dest1[0] - suborg[0]) * (dest2[0] - suborg[0]) +
                          (dest1[1] - suborg[1]) * (dest2[1] - suborg[1])) /
                         sqrt(prevseglength * seglength)));
      if (edgelength < iradius) {
        /* If this cluster is split, the new edge dest1-dest2 will be     */
        /*   smaller than the insertion radius of the encroaching vertex. */
        /*   Hence, we'd prefer to avoid splitting it if possible.        */
        toosmall = 1;
      }
      if (ccwsubseg.ss == startsubseg.ss) {
        /* We've gone all the way around the vertex.  Split the cluster */
        /*   if no edges will be too short.                             */
        return !toosmall;
      }

      /* Find the next subsegment counterclockwise around the vertex. */
      subsegcopy(ccwsubseg, nowsubseg);
      dest1 = dest2;
      prevseglength = seglength;
      ccwsmall = counterclockwiseseg(m, &nowsubseg, &ccwsubseg);
    } while (ccwsmall);
  }

  /* We've found every subsegment in the cluster.  Split the cluster */
  /*   if no edges will be too short.                                */
  return !toosmall;
}

#endif /* not CDT_ONLY */

#ifndef CDT_ONLY

int checkseg4encroach(struct mesh *m, struct behavior *b,
                      struct osub *testsubseg, sgFloat iradius)
{
  struct otri neighbortri;
  struct osub testsym;
  struct badsubseg *encroachedseg;
  sgFloat dotproduct;
  int encroached;
  int sides;
  int enq;
  vertex eorg, edest, eapex;
  triangle ptr;                     /* Temporary variable used by stpivot(). */

  encroached = 0;
  sides = 0;

  sorg(*testsubseg, eorg);
  sdest(*testsubseg, edest);
  /* Check one neighbor of the subsegment. */
  stpivot(*testsubseg, neighbortri);
  /* Does the neighbor exist, or is this a boundary edge? */
  if (neighbortri.tri != m->dummytri) {
    sides++;
    /* Find a vertex opposite this subsegment. */
    apex(neighbortri, eapex);
    /* Check whether the apex is in the diametral lens of the subsegment */
    /*   (or the diametral circle, if `nolenses' is set).  A dot product */
    /*   of two sides of the triangle is used to check whether the angle */
    /*   at the apex is greater than 120 degrees (for lenses; 90 degrees */
    /*   for diametral circles).                                         */
    dotproduct = (eorg[0] - eapex[0]) * (edest[0] - eapex[0]) +
                 (eorg[1] - eapex[1]) * (edest[1] - eapex[1]);
    if (dotproduct < 0.0) {
      if (b->nolenses ||
          (dotproduct * dotproduct >=
           0.25 * ((eorg[0] - eapex[0]) * (eorg[0] - eapex[0]) +
                   (eorg[1] - eapex[1]) * (eorg[1] - eapex[1])) *
                  ((edest[0] - eapex[0]) * (edest[0] - eapex[0]) +
                   (edest[1] - eapex[1]) * (edest[1] - eapex[1])))) {
        encroached = 1;
      }
    }
  }
  /* Check the other neighbor of the subsegment. */
  ssym(*testsubseg, testsym);
  stpivot(testsym, neighbortri);
  /* Does the neighbor exist, or is this a boundary edge? */
  if (neighbortri.tri != m->dummytri) {
    sides++;
    /* Find the other vertex opposite this subsegment. */
    apex(neighbortri, eapex);
    /* Check whether the apex is in the diametral lens of the subsegment */
    /*   (or the diametral circle, if `nolenses' is set).                */
    dotproduct = (eorg[0] - eapex[0]) * (edest[0] - eapex[0]) +
                 (eorg[1] - eapex[1]) * (edest[1] - eapex[1]);
    if (dotproduct < 0.0) {
      if (b->nolenses ||
          (dotproduct * dotproduct >=
           0.25 * ((eorg[0] - eapex[0]) * (eorg[0] - eapex[0]) +
                   (eorg[1] - eapex[1]) * (eorg[1] - eapex[1])) *
                  ((edest[0] - eapex[0]) * (edest[0] - eapex[0]) +
                   (edest[1] - eapex[1]) * (edest[1] - eapex[1])))) {
        encroached += 2;
      }
    }
  }

  if (encroached && (!b->nobisect || ((b->nobisect == 1) && (sides == 2)))) {
    /* Decide whether `testsubseg' should be split. */
    if (iradius > 0.0) {
      /* The encroaching vertex is a triangle circumcenter, which will be   */
      /*   rejected.  Hence, `testsubseg' probably should be split, unless  */
      /*   it is part of a subsegment cluster which, according to the rules */
      /*   described in my paper "Mesh Generation for Domains with Small    */
      /*   Angles," should not be split.                                    */
      enq = splitpermitted(m, testsubseg, iradius);
    } else {
      /* The encroaching vertex is an input vertex or was inserted in a */
      /*   subsegment, so the encroached subsegment must be split.      */
      enq = 1;
    }
    if (enq) {
      if (b->verbose > 2) {
        //  TRACE(
        //  "  Queueing encroached subsegment (%.12g, %.12g) (%.12g, %.12g).\n",
        //  eorg[0], eorg[1], edest[0], edest[1]);
      }
      /* Add the subsegment to the list of encroached subsegments. */
      /*   Be sure to get the orientation right.                   */
      encroachedseg = (struct badsubseg *) poolalloc(&m->badsubsegs);
      if (encroached == 1) {
        encroachedseg->encsubseg = sencode(*testsubseg);
        encroachedseg->subsegorg = eorg;
        encroachedseg->subsegdest = edest;
      } else {
        encroachedseg->encsubseg = sencode(testsym);
        encroachedseg->subsegorg = edest;
        encroachedseg->subsegdest = eorg;
      }
    }
  }

  return encroached;
}

#endif /* not CDT_ONLY */

#ifndef CDT_ONLY

void testtriangle(struct mesh *m, struct behavior *b, struct otri *testtri)
{
  struct otri sametesttri;
  struct osub subseg1, subseg2;
  vertex torg, tdest, tapex;
  vertex anglevertex;
  sgFloat dxod, dyod, dxda, dyda, dxao, dyao;
  sgFloat dxod2, dyod2, dxda2, dyda2, dxao2, dyao2;
  sgFloat apexlen, orglen, destlen;
  sgFloat angle;
  sgFloat area;
  subseg sptr;                      /* Temporary variable used by tspivot(). */

  org(*testtri, torg);
  dest(*testtri, tdest);
  apex(*testtri, tapex);
  dxod = torg[0] - tdest[0];
  dyod = torg[1] - tdest[1];
  dxda = tdest[0] - tapex[0];
  dyda = tdest[1] - tapex[1];
  dxao = tapex[0] - torg[0];
  dyao = tapex[1] - torg[1];
  dxod2 = dxod * dxod;
  dyod2 = dyod * dyod;
  dxda2 = dxda * dxda;
  dyda2 = dyda * dyda;
  dxao2 = dxao * dxao;
  dyao2 = dyao * dyao;
  /* Find the lengths of the triangle's three edges. */
  apexlen = dxod2 + dyod2;
  orglen = dxda2 + dyda2;
  destlen = dxao2 + dyao2;
  if ((apexlen < orglen) && (apexlen < destlen)) {
    /* The edge opposite the apex is shortest. */
    /* Find the square of the cosine of the angle at the apex. */
    angle = dxda * dxao + dyda * dyao;
    angle = angle * angle / (orglen * destlen);
    anglevertex = tapex;
    lnext(*testtri, sametesttri);
    tspivot(sametesttri, subseg1);
    lnextself(sametesttri);
    tspivot(sametesttri, subseg2);
  } else if (orglen < destlen) {
    /* The edge opposite the origin is shortest. */
    /* Find the square of the cosine of the angle at the origin. */
    angle = dxod * dxao + dyod * dyao;
    angle = angle * angle / (apexlen * destlen);
    anglevertex = torg;
    tspivot(*testtri, subseg1);
    lprev(*testtri, sametesttri);
    tspivot(sametesttri, subseg2);
  } else {
    /* The edge opposite the destination is shortest. */
    /* Find the square of the cosine of the angle at the destination. */
    angle = dxod * dxda + dyod * dyda;
    angle = angle * angle / (apexlen * orglen);
    anglevertex = tdest;
    tspivot(*testtri, subseg1);
    lnext(*testtri, sametesttri);
    tspivot(sametesttri, subseg2);
  }

  /* Check if both edges that form the angle are segments. */
  if ((subseg1.ss != m->dummysub) && (subseg2.ss != m->dummysub)) {
    /* The angle is a segment intersection.  Don't add this bad triangle to */
    /*   the list; there's nothing that can be done about a small angle     */
    /*   between two segments.                                              */
    angle = 0.0;
  }

  /* Check whether the angle is smaller than permitted. */
  if (angle > b->goodangle) {
    /* Add this triangle to the list of bad triangles. */
    enqueuebadtri(m, b, testtri, angle, tapex, torg, tdest);
    return;
  }

  if (b->vararea || b->fixedarea || b->usertest) {
    /* Check whether the area is larger than permitted. */
    area = 0.5 * (dxod * dyda - dyod * dxda);
    if (b->fixedarea && (area > b->maxarea)) {
      /* Add this triangle to the list of bad triangles. */
      enqueuebadtri(m, b, testtri, angle, tapex, torg, tdest);
      return;
    }

    /* Nonpositive area constraints are treated as unconstrained. */
    if ((b->vararea) && (area > areabound(*testtri)) &&
        (areabound(*testtri) > 0.0)) {
      /* Add this triangle to the list of bad triangles. */
      enqueuebadtri(m, b, testtri, angle, tapex, torg, tdest);
      return;
    }

    if (b->usertest) {
      /* Check whether the user thinks this triangle is too large. */
      if (triunsuitable(torg, tdest, tapex, area)) {
        enqueuebadtri(m, b, testtri, angle, tapex, torg, tdest);
        return;
      }
    }
  }
}

#endif /* not CDT_ONLY */


void makevertexmap(struct mesh *m, struct behavior *b)
{
  struct otri triangleloop;
  vertex triorg;

  if (b->verbose) {
    //  TRACE("    Constructing mapping from vertices to triangles.\n");
  }
  traversalinit(&m->triangles);
  triangleloop.tri = triangletraverse(m);
  while (triangleloop.tri != (triangle *) NULL) {
    /* Check all three vertices of the triangle. */
    for (triangleloop.orient = 0; triangleloop.orient < 3;
         triangleloop.orient++) {
      org(triangleloop, triorg);
      setvertex2tri(triorg, encode(triangleloop));
    }
    triangleloop.tri = triangletraverse(m);
  }
}

enum locateresult preciselocate(struct mesh *m, struct behavior *b,
                                vertex searchpoint, struct otri *searchtri,
                                int stopatsubsegment)
{
  struct otri backtracktri;
  struct osub checkedge;
  vertex forg, fdest, fapex;
  sgFloat orgorient, destorient;
  int moveleft;
  triangle ptr;                         /* Temporary variable used by sym(). */
  subseg sptr;                      /* Temporary variable used by tspivot(). */

  if (b->verbose > 2) {
    //  TRACE("  Searching for point (%.12g, %.12g).\n",
//           searchpoint[0], searchpoint[1]);
  }
  /* Where are we? */
  org(*searchtri, forg);
  dest(*searchtri, fdest);
  apex(*searchtri, fapex);
  while (1) {
    if (b->verbose > 2) {
      //  TRACE("    At (%.12g, %.12g) (%.12g, %.12g) (%.12g, %.12g)\n",
//             forg[0], forg[1], fdest[0], fdest[1], fapex[0], fapex[1]);
    }
    /* Check whether the apex is the point we seek. */
    if ((fapex[0] == searchpoint[0]) && (fapex[1] == searchpoint[1])) {
      lprevself(*searchtri);
      return ONVERTEX;
    }
    /* Does the point lie on the other side of the line defined by the */
    /*   triangle edge opposite the triangle's destination?            */
    destorient = counterclockwise(m, b, forg, fapex, searchpoint);
    /* Does the point lie on the other side of the line defined by the */
    /*   triangle edge opposite the triangle's origin?                 */
    orgorient = counterclockwise(m, b, fapex, fdest, searchpoint);
    if (destorient > 0.0) {
      if (orgorient > 0.0) {
        /* Move left if the inner product of (fapex - searchpoint) and  */
        /*   (fdest - forg) is positive.  This is equivalent to drawing */
        /*   a line perpendicular to the line (forg, fdest) and passing */
        /*   through `fapex', and determining which side of this line   */
        /*   `searchpoint' falls on.                                    */
        moveleft = (fapex[0] - searchpoint[0]) * (fdest[0] - forg[0]) +
                   (fapex[1] - searchpoint[1]) * (fdest[1] - forg[1]) > 0.0;
      } else {
        moveleft = 1;
      }
    } else {
      if (orgorient > 0.0) {
        moveleft = 0;
      } else {
        /* The point we seek must be on the boundary of or inside this */
        /*   triangle.                                                 */
        if (destorient == 0.0) {
          lprevself(*searchtri);
          return ONEDGE;
        }
        if (orgorient == 0.0) {
          lnextself(*searchtri);
          return ONEDGE;
        }
        return INTRIANGLE;
      }
    }

    /* Move to another triangle.  Leave a trace `backtracktri' in case */
    /*   floating-point roundoff or some such bogey causes us to walk  */
    /*   off a boundary of the triangulation.                          */
    if (moveleft) {
      lprev(*searchtri, backtracktri);
      fdest = fapex;
    } else {
      lnext(*searchtri, backtracktri);
      forg = fapex;
    }
    sym(backtracktri, *searchtri);

    if (m->checksegments && stopatsubsegment) {
      /* Check for walking through a subsegment. */
      tspivot(backtracktri, checkedge);
      if (checkedge.ss != m->dummysub) {
        /* Go back to the last triangle. */
        otricopy(backtracktri, *searchtri);
        return OUTSIDE;
      }
    }
    /* Check for walking right out of the triangulation. */
    if (searchtri->tri == m->dummytri) {
      /* Go back to the last triangle. */
      otricopy(backtracktri, *searchtri);
      return OUTSIDE;
    }

    apex(*searchtri, fapex);
  }
}

enum locateresult locate(struct mesh *m, struct behavior *b,
                         vertex searchpoint, struct otri *searchtri)
{
  int **sampleblock;
  triangle *firsttri;
  struct otri sampletri;
  vertex torg, tdest;
  unsigned long alignptr;
  sgFloat searchdist, dist;
  sgFloat ahead;
  long sampleblocks, samplesperblock, samplenum;
  long triblocks;
  long i, j;
  triangle ptr;                         /* Temporary variable used by sym(). */

  if (b->verbose > 2) {
    //  TRACE("  Randomly sampling for a triangle near point (%.12g, %.12g).\n",
//           searchpoint[0], searchpoint[1]);
  }
  /* Record the distance from the suggested starting triangle to the */
  /*   point we seek.                                                */
  org(*searchtri, torg);
  searchdist = (searchpoint[0] - torg[0]) * (searchpoint[0] - torg[0]) +
               (searchpoint[1] - torg[1]) * (searchpoint[1] - torg[1]);
  if (b->verbose > 2) {
    //  TRACE("    Boundary triangle has origin (%.12g, %.12g).\n",
//           torg[0], torg[1]);
  }

  /* If a recently encountered triangle has been recorded and has not been */
  /*   deallocated, test it as a good starting point.                      */
  if (m->recenttri.tri != (triangle *) NULL) {
    if (!deadtri(m->recenttri.tri)) {
      org(m->recenttri, torg);
      if ((torg[0] == searchpoint[0]) && (torg[1] == searchpoint[1])) {
        otricopy(m->recenttri, *searchtri);
        return ONVERTEX;
      }
      dist = (searchpoint[0] - torg[0]) * (searchpoint[0] - torg[0]) +
             (searchpoint[1] - torg[1]) * (searchpoint[1] - torg[1]);
      if (dist < searchdist) {
        otricopy(m->recenttri, *searchtri);
        searchdist = dist;
        if (b->verbose > 2) {
          //  TRACE("    Choosing recent triangle with origin (%.12g, %.12g).\n",
//                 torg[0], torg[1]);
        }
      }
    }
  }

  /* The number of random samples taken is proportional to the cube root of */
  /*   the number of triangles in the mesh.  The next bit of code assumes   */
  /*   that the number of triangles increases monotonically.                */
  while (SAMPLEFACTOR * m->samples * m->samples * m->samples <
         m->triangles.items) {
    m->samples++;
  }
  triblocks = (m->triangles.maxitems + TRIPERBLOCK - 1) / TRIPERBLOCK;
  samplesperblock = (m->samples + triblocks - 1) / triblocks;
  sampleblocks = m->samples / samplesperblock;
  sampleblock = m->triangles.firstblock;
  sampletri.orient = 0;
  for (i = 0; i < sampleblocks; i++) {
    alignptr = (unsigned long) (sampleblock + 1);
    firsttri = (triangle *) (alignptr + (unsigned long) m->triangles.alignbytes
                      - (alignptr % (unsigned long) m->triangles.alignbytes));
    for (j = 0; j < samplesperblock; j++) {
      if (i == triblocks - 1) {
        samplenum = randomnation((unsigned int)
                                 (m->triangles.maxitems - (i * TRIPERBLOCK)));
      } else {
        samplenum = randomnation(TRIPERBLOCK);
      }
      sampletri.tri = (triangle *)
                      (firsttri + (samplenum * m->triangles.itemwords));
      if (!deadtri(sampletri.tri)) {
        org(sampletri, torg);
        dist = (searchpoint[0] - torg[0]) * (searchpoint[0] - torg[0]) +
               (searchpoint[1] - torg[1]) * (searchpoint[1] - torg[1]);
        if (dist < searchdist) {
          otricopy(sampletri, *searchtri);
          searchdist = dist;
          if (b->verbose > 2) {
            //  TRACE("    Choosing triangle with origin (%.12g, %.12g).\n",
//                   torg[0], torg[1]);
          }
        }
      }
    }
    sampleblock = (int **) *sampleblock;
  }

  /* Where are we? */
  org(*searchtri, torg);
  dest(*searchtri, tdest);
  /* Check the starting triangle's vertices. */
  if ((torg[0] == searchpoint[0]) && (torg[1] == searchpoint[1])) {
    return ONVERTEX;
  }
  if ((tdest[0] == searchpoint[0]) && (tdest[1] == searchpoint[1])) {
    lnextself(*searchtri);
    return ONVERTEX;
  }
  /* Orient `searchtri' to fit the preconditions of calling preciselocate(). */
  ahead = counterclockwise(m, b, torg, tdest, searchpoint);
  if (ahead < 0.0) {
    /* Turn around so that `searchpoint' is to the left of the */
    /*   edge specified by `searchtri'.                        */
    symself(*searchtri);
  } else if (ahead == 0.0) {
    /* Check if `searchpoint' is between `torg' and `tdest'. */
    if (((torg[0] < searchpoint[0]) == (searchpoint[0] < tdest[0])) &&
        ((torg[1] < searchpoint[1]) == (searchpoint[1] < tdest[1]))) {
      return ONEDGE;
    }
  }
  return preciselocate(m, b, searchpoint, searchtri, 0);
}

void insertsubseg(struct mesh *m, struct behavior *b, struct otri *tri,
                  int subsegmark)
{
  struct otri oppotri;
  struct osub newsubseg;
  vertex triorg, tridest;
  triangle ptr;                         /* Temporary variable used by sym(). */
  subseg sptr;                      /* Temporary variable used by tspivot(). */

  org(*tri, triorg);
  dest(*tri, tridest);
  /* Mark vertices if possible. */
  if (vertexmark(triorg) == 0) {
    setvertexmark(triorg, subsegmark);
  }
  if (vertexmark(tridest) == 0) {
    setvertexmark(tridest, subsegmark);
  }
  /* Check if there's already a subsegment here. */
  tspivot(*tri, newsubseg);
  if (newsubseg.ss == m->dummysub) {
    /* Make new subsegment and initialize its vertices. */
    makesubseg(m, &newsubseg);
    setsorg(newsubseg, tridest);
    setsdest(newsubseg, triorg);
    /* Bond new subsegment to the two triangles it is sandwiched between. */
    /*   Note that the facing triangle `oppotri' might be equal to        */
    /*   `dummytri' (outer space), but the new subsegment is bonded to it */
    /*   all the same.                                                    */
    tsbond(*tri, newsubseg);
    sym(*tri, oppotri);
    ssymself(newsubseg);
    tsbond(oppotri, newsubseg);
    setmark(newsubseg, subsegmark);
    if (b->verbose > 2) {
      //  TRACE("  Inserting new ");
     // printsubseg(m, b, &newsubseg);
    }
  } else {
    if (mark(newsubseg) == 0) {
      setmark(newsubseg, subsegmark);
    }
  }
}

void flip(struct mesh *m, struct behavior *b, struct otri *flipedge)
{
  struct otri botleft, botright;
  struct otri topleft, topright;
  struct otri top;
  struct otri botlcasing, botrcasing;
  struct otri toplcasing, toprcasing;
  struct osub botlsubseg, botrsubseg;
  struct osub toplsubseg, toprsubseg;
  vertex leftvertex, rightvertex, botvertex;
  vertex farvertex;
  triangle ptr;                         /* Temporary variable used by sym(). */
  subseg sptr;                      /* Temporary variable used by tspivot(). */

  /* Identify the vertices of the quadrilateral. */
  org(*flipedge, rightvertex);
  dest(*flipedge, leftvertex);
  apex(*flipedge, botvertex);
  sym(*flipedge, top);
#ifdef SELF_CHECK
  if (top.tri == m->dummytri) {
    //  TRACE("Internal error in flip():  Attempt to flip on boundary.\n");
    lnextself(*flipedge);
    return;
  }
  if (m->checksegments) {
    tspivot(*flipedge, toplsubseg);
    if (toplsubseg.ss != m->dummysub) {
      //  TRACE("Internal error in flip():  Attempt to flip a segment.\n");
      lnextself(*flipedge);
      return;
    }
  }
#endif /* SELF_CHECK */
  apex(top, farvertex);

  /* Identify the casing of the quadrilateral. */
  lprev(top, topleft);
  sym(topleft, toplcasing);
  lnext(top, topright);
  sym(topright, toprcasing);
  lnext(*flipedge, botleft);
  sym(botleft, botlcasing);
  lprev(*flipedge, botright);
  sym(botright, botrcasing);
  /* Rotate the quadrilateral one-quarter turn counterclockwise. */
  bond(topleft, botlcasing);
  bond(botleft, botrcasing);
  bond(botright, toprcasing);
  bond(topright, toplcasing);

  if (m->checksegments) {
    /* Check for subsegments and rebond them to the quadrilateral. */
    tspivot(topleft, toplsubseg);
    tspivot(botleft, botlsubseg);
    tspivot(botright, botrsubseg);
    tspivot(topright, toprsubseg);
    if (toplsubseg.ss == m->dummysub) {
      tsdissolve(topright);
    } else {
      tsbond(topright, toplsubseg);
    }
    if (botlsubseg.ss == m->dummysub) {
      tsdissolve(topleft);
    } else {
      tsbond(topleft, botlsubseg);
    }
    if (botrsubseg.ss == m->dummysub) {
      tsdissolve(botleft);
    } else {
      tsbond(botleft, botrsubseg);
    }
    if (toprsubseg.ss == m->dummysub) {
      tsdissolve(botright);
    } else {
      tsbond(botright, toprsubseg);
    }
  }

  /* New vertex assignments for the rotated quadrilateral. */
  setorg(*flipedge, farvertex);
  setdest(*flipedge, botvertex);
  setapex(*flipedge, rightvertex);
  setorg(top, botvertex);
  setdest(top, farvertex);
  setapex(top, leftvertex);
  if (b->verbose > 2) {
    //  TRACE("  Edge flip results in left ");
//    printtriangle(m, b, &top);
    //  TRACE("  and right ");
//    printtriangle(m, b, flipedge);
  }
}

void unflip(struct mesh *m, struct behavior *b, struct otri *flipedge)
{
  struct otri botleft, botright;
  struct otri topleft, topright;
  struct otri top;
  struct otri botlcasing, botrcasing;
  struct otri toplcasing, toprcasing;
  struct osub botlsubseg, botrsubseg;
  struct osub toplsubseg, toprsubseg;
  vertex leftvertex, rightvertex, botvertex;
  vertex farvertex;
  triangle ptr;                         /* Temporary variable used by sym(). */
  subseg sptr;                      /* Temporary variable used by tspivot(). */

  /* Identify the vertices of the quadrilateral. */
  org(*flipedge, rightvertex);
  dest(*flipedge, leftvertex);
  apex(*flipedge, botvertex);
  sym(*flipedge, top);
#ifdef SELF_CHECK
  if (top.tri == m->dummytri) {
    //  TRACE("Internal error in unflip():  Attempt to flip on boundary.\n");
    lnextself(*flipedge);
    return;
  }
  if (m->checksegments) {
    tspivot(*flipedge, toplsubseg);
    if (toplsubseg.ss != m->dummysub) {
      //  TRACE("Internal error in unflip():  Attempt to flip a subsegment.\n");
      lnextself(*flipedge);
      return;
    }
  }
#endif /* SELF_CHECK */
  apex(top, farvertex);

  /* Identify the casing of the quadrilateral. */
  lprev(top, topleft);
  sym(topleft, toplcasing);
  lnext(top, topright);
  sym(topright, toprcasing);
  lnext(*flipedge, botleft);
  sym(botleft, botlcasing);
  lprev(*flipedge, botright);
  sym(botright, botrcasing);
  /* Rotate the quadrilateral one-quarter turn clockwise. */
  bond(topleft, toprcasing);
  bond(botleft, toplcasing);
  bond(botright, botlcasing);
  bond(topright, botrcasing);

  if (m->checksegments) {
    /* Check for subsegments and rebond them to the quadrilateral. */
    tspivot(topleft, toplsubseg);
    tspivot(botleft, botlsubseg);
    tspivot(botright, botrsubseg);
    tspivot(topright, toprsubseg);
    if (toplsubseg.ss == m->dummysub) {
      tsdissolve(botleft);
    } else {
      tsbond(botleft, toplsubseg);
    }
    if (botlsubseg.ss == m->dummysub) {
      tsdissolve(botright);
    } else {
      tsbond(botright, botlsubseg);
    }
    if (botrsubseg.ss == m->dummysub) {
      tsdissolve(topright);
    } else {
      tsbond(topright, botrsubseg);
    }
    if (toprsubseg.ss == m->dummysub) {
      tsdissolve(topleft);
    } else {
      tsbond(topleft, toprsubseg);
    }
  }

  /* New vertex assignments for the rotated quadrilateral. */
  setorg(*flipedge, botvertex);
  setdest(*flipedge, farvertex);
  setapex(*flipedge, leftvertex);
  setorg(top, farvertex);
  setdest(top, botvertex);
  setapex(top, rightvertex);
  if (b->verbose > 2) {
    //  TRACE("  Edge unflip results in left ");
//    printtriangle(m, b, flipedge);
    //  TRACE("  and right ");
 //   printtriangle(m, b, &top);
  }
}

enum insertvertexresult insertvertex(struct mesh *m, struct behavior *b,
                                     vertex newvertex, struct otri *searchtri,
                                     struct osub *splitseg,
                                     int segmentflaws, int triflaws,
                                     sgFloat iradius)
{
  struct otri horiz;
  struct otri top;
  struct otri botleft, botright;
  struct otri topleft, topright;
  struct otri newbotleft, newbotright;
  struct otri newtopright;
  struct otri botlcasing, botrcasing;
  struct otri toplcasing, toprcasing;
  struct otri testtri;
  struct osub botlsubseg, botrsubseg;
  struct osub toplsubseg, toprsubseg;
  struct osub brokensubseg;
  struct osub checksubseg;
  struct osub rightsubseg;
  struct osub newsubseg;
  struct badsubseg *encroached;
  struct flipstacker *newflip;
  vertex first;
  vertex leftvertex, rightvertex, botvertex, topvertex, farvertex;
  sgFloat attrib;
  sgFloat area;
  enum insertvertexresult success;
  enum locateresult intersect;
  int doflip;
  int mirrorflag;
  int enq;
  int i;
  triangle ptr;                         /* Temporary variable used by sym(). */
  subseg sptr;         /* Temporary variable used by spivot() and tspivot(). */

  if (b->verbose > 1) {
    //  TRACE("  Inserting (%.12g, %.12g).\n", newvertex[0], newvertex[1]);
  }

  if (splitseg == (struct osub *) NULL) {
    /* Find the location of the vertex to be inserted.  Check if a good */
    /*   starting triangle has already been provided by the caller.     */
    if (searchtri->tri == m->dummytri) {
      /* Find a boundary triangle. */
      horiz.tri = m->dummytri;
      horiz.orient = 0;
      symself(horiz);
      /* Search for a triangle containing `newvertex'. */
      intersect = locate(m, b, newvertex, &horiz);
    } else {
      /* Start searching from the triangle provided by the caller. */
      otricopy(*searchtri, horiz);
      intersect = preciselocate(m, b, newvertex, &horiz, 1);
    }
  } else {
    /* The calling routine provides the subsegment in which */
    /*   the vertex is inserted.                             */
    otricopy(*searchtri, horiz);
    intersect = ONEDGE;
  }
  if (intersect == ONVERTEX) {
    /* There's already a vertex there.  Return in `searchtri' a triangle */
    /*   whose origin is the existing vertex.                            */
    otricopy(horiz, *searchtri);
    otricopy(horiz, m->recenttri);
    return DUPLICATEVERTEX;
  }
  if ((intersect == ONEDGE) || (intersect == OUTSIDE)) {
    /* The vertex falls on an edge or boundary. */
    if (m->checksegments && (splitseg == (struct osub *) NULL)) {
      /* Check whether the vertex falls on a subsegment. */
      tspivot(horiz, brokensubseg);
      if (brokensubseg.ss != m->dummysub) {
        /* The vertex falls on a subsegment, and hence will not be inserted. */
        if (segmentflaws) {
          if (b->nobisect == 2) {
            enq = 0;
#ifndef CDT_ONLY
          } else if (iradius > 0.0) {
            enq = splitpermitted(m, &brokensubseg, iradius);
#endif /* not CDT_ONLY */
          } else {
            enq = 1;
          }
          if (enq && (b->nobisect == 1)) {
            /* This subsegment may be split only if it is an */
            /*   internal boundary.                          */
            sym(horiz, testtri);
            enq = testtri.tri != m->dummytri;
          }
          if (enq) {
            /* Add the subsegment to the list of encroached subsegments. */
            encroached = (struct badsubseg *) poolalloc(&m->badsubsegs);
            encroached->encsubseg = sencode(brokensubseg);
            sorg(brokensubseg, encroached->subsegorg);
            sdest(brokensubseg, encroached->subsegdest);
            if (b->verbose > 2) {
              //  TRACE(
       //   "  Queueing encroached subsegment (%.12g, %.12g) (%.12g, %.12g).\n",
        //             encroached->subsegorg[0], encroached->subsegorg[1],
        //             encroached->subsegdest[0], encroached->subsegdest[1]);
            }
          }
        }
        /* Return a handle whose primary edge contains the vertex, */
        /*   which has not been inserted.                          */
        otricopy(horiz, *searchtri);
        otricopy(horiz, m->recenttri);
        return VIOLATINGVERTEX;
      }
    }

    /* Insert the vertex on an edge, dividing one triangle into two (if */
    /*   the edge lies on a boundary) or two triangles into four.       */
    lprev(horiz, botright);
    sym(botright, botrcasing);
    sym(horiz, topright);
    /* Is there a second triangle?  (Or does this edge lie on a boundary?) */
    mirrorflag = topright.tri != m->dummytri;
    if (mirrorflag) {
      lnextself(topright);
      sym(topright, toprcasing);
      maketriangle(m, b, &newtopright);
    } else {
      /* Splitting a boundary edge increases the number of boundary edges. */
      m->hullsize++;
    }
    maketriangle(m, b, &newbotright);

    /* Set the vertices of changed and new triangles. */
    org(horiz, rightvertex);
    dest(horiz, leftvertex);
    apex(horiz, botvertex);
    setorg(newbotright, botvertex);
    setdest(newbotright, rightvertex);
    setapex(newbotright, newvertex);
    setorg(horiz, newvertex);
    for (i = 0; i < m->eextras; i++) {
      /* Set the element attributes of a new triangle. */
      setelemattribute(newbotright, i, elemattribute(botright, i));
    }
    if (b->vararea) {
      /* Set the area constraint of a new triangle. */
      setareabound(newbotright, areabound(botright));
    }
    if (mirrorflag) {
      dest(topright, topvertex);
      setorg(newtopright, rightvertex);
      setdest(newtopright, topvertex);
      setapex(newtopright, newvertex);
      setorg(topright, newvertex);
      for (i = 0; i < m->eextras; i++) {
        /* Set the element attributes of another new triangle. */
        setelemattribute(newtopright, i, elemattribute(topright, i));
      }
      if (b->vararea) {
        /* Set the area constraint of another new triangle. */
        setareabound(newtopright, areabound(topright));
      }
    }

    /* There may be subsegments that need to be bonded */
    /*   to the new triangle(s).                       */
    if (m->checksegments) {
      tspivot(botright, botrsubseg);
      if (botrsubseg.ss != m->dummysub) {
        tsdissolve(botright);
        tsbond(newbotright, botrsubseg);
      }
      if (mirrorflag) {
        tspivot(topright, toprsubseg);
        if (toprsubseg.ss != m->dummysub) {
          tsdissolve(topright);
          tsbond(newtopright, toprsubseg);
        }
      }
    }

    /* Bond the new triangle(s) to the surrounding triangles. */
    bond(newbotright, botrcasing);
    lprevself(newbotright);
    bond(newbotright, botright);
    lprevself(newbotright);
    if (mirrorflag) {
      bond(newtopright, toprcasing);
      lnextself(newtopright);
      bond(newtopright, topright);
      lnextself(newtopright);
      bond(newtopright, newbotright);
    }

    if (splitseg != (struct osub *) NULL) {
      /* Split the subsegment into two. */
      setsdest(*splitseg, newvertex);
      ssymself(*splitseg);
      spivot(*splitseg, rightsubseg);
      insertsubseg(m, b, &newbotright, mark(*splitseg));
      tspivot(newbotright, newsubseg);
      sbond(*splitseg, newsubseg);
      ssymself(newsubseg);
      sbond(newsubseg, rightsubseg);
      ssymself(*splitseg);
      /* Transfer the subsegment's boundary marker to the vertex */
      /*   if required.                                          */
      if (vertexmark(newvertex) == 0) {
        setvertexmark(newvertex, mark(*splitseg));
      }
    }

    if (m->checkquality) {
      poolrestart(&m->flipstackers);
      m->lastflip = (struct flipstacker *) poolalloc(&m->flipstackers);
      m->lastflip->flippedtri = encode(horiz);
      m->lastflip->prevflip = (struct flipstacker *) &insertvertex;
    }

#ifdef SELF_CHECK
    if (counterclockwise(m, b, rightvertex, leftvertex, botvertex) < 0.0) {
      //  TRACE("Internal error in insertvertex():\n");
      //  TRACE(
            "  Clockwise triangle prior to edge vertex insertion (bottom).\n");
    }
    if (mirrorflag) {
      if (counterclockwise(m, b, leftvertex, rightvertex, topvertex) < 0.0) {
        //  TRACE("Internal error in insertvertex():\n");
        //  TRACE("  Clockwise triangle prior to edge vertex insertion (top).\n");
      }
      if (counterclockwise(m, b, rightvertex, topvertex, newvertex) < 0.0) {
        //  TRACE("Internal error in insertvertex():\n");
        //  TRACE(
            "  Clockwise triangle after edge vertex insertion (top right).\n");
      }
      if (counterclockwise(m, b, topvertex, leftvertex, newvertex) < 0.0) {
        //  TRACE("Internal error in insertvertex():\n");
        //  TRACE(
            "  Clockwise triangle after edge vertex insertion (top left).\n");
      }
    }
    if (counterclockwise(m, b, leftvertex, botvertex, newvertex) < 0.0) {
      //  TRACE("Internal error in insertvertex():\n");
      //  TRACE(
          "  Clockwise triangle after edge vertex insertion (bottom left).\n");
    }
    if (counterclockwise(m, b, botvertex, rightvertex, newvertex) < 0.0) {
      //  TRACE("Internal error in insertvertex():\n");
      //  TRACE(
        "  Clockwise triangle after edge vertex insertion (bottom right).\n");
    }
#endif /* SELF_CHECK */
    if (b->verbose > 2) {
      //  TRACE("  Updating bottom left ");
//      printtriangle(m, b, &botright);
      if (mirrorflag) {
        //  TRACE("  Updating top left ");
     //   printtriangle(m, b, &topright);
        //  TRACE("  Creating top right ");
     //   printtriangle(m, b, &newtopright);
      }
      //  TRACE("  Creating bottom right ");
    //  printtriangle(m, b, &newbotright);
    }

    /* Position `horiz' on the first edge to check for */
    /*   the Delaunay property.                        */
    lnextself(horiz);
  } else {
    /* Insert the vertex in a triangle, splitting it into three. */
    lnext(horiz, botleft);
    lprev(horiz, botright);
    sym(botleft, botlcasing);
    sym(botright, botrcasing);
    maketriangle(m, b, &newbotleft);
    maketriangle(m, b, &newbotright);

    /* Set the vertices of changed and new triangles. */
    org(horiz, rightvertex);
    dest(horiz, leftvertex);
    apex(horiz, botvertex);
    setorg(newbotleft, leftvertex);
    setdest(newbotleft, botvertex);
    setapex(newbotleft, newvertex);
    setorg(newbotright, botvertex);
    setdest(newbotright, rightvertex);
    setapex(newbotright, newvertex);
    setapex(horiz, newvertex);
    for (i = 0; i < m->eextras; i++) {
      /* Set the element attributes of the new triangles. */
      attrib = elemattribute(horiz, i);
      setelemattribute(newbotleft, i, attrib);
      setelemattribute(newbotright, i, attrib);
    }
    if (b->vararea) {
      /* Set the area constraint of the new triangles. */
      area = areabound(horiz);
      setareabound(newbotleft, area);
      setareabound(newbotright, area);
    }

    /* There may be subsegments that need to be bonded */
    /*   to the new triangles.                         */
    if (m->checksegments) {
      tspivot(botleft, botlsubseg);
      if (botlsubseg.ss != m->dummysub) {
        tsdissolve(botleft);
        tsbond(newbotleft, botlsubseg);
      }
      tspivot(botright, botrsubseg);
      if (botrsubseg.ss != m->dummysub) {
        tsdissolve(botright);
        tsbond(newbotright, botrsubseg);
      }
    }

    /* Bond the new triangles to the surrounding triangles. */
    bond(newbotleft, botlcasing);
    bond(newbotright, botrcasing);
    lnextself(newbotleft);
    lprevself(newbotright);
    bond(newbotleft, newbotright);
    lnextself(newbotleft);
    bond(botleft, newbotleft);
    lprevself(newbotright);
    bond(botright, newbotright);

    if (m->checkquality) {
      poolrestart(&m->flipstackers);
      m->lastflip = (struct flipstacker *) poolalloc(&m->flipstackers);
      m->lastflip->flippedtri = encode(horiz);
      m->lastflip->prevflip = (struct flipstacker *) NULL;
    }

#ifdef SELF_CHECK
    if (counterclockwise(m, b, rightvertex, leftvertex, botvertex) < 0.0) {
      //  TRACE("Internal error in insertvertex():\n");
      //  TRACE("  Clockwise triangle prior to vertex insertion.\n");
    }
    if (counterclockwise(m, b, rightvertex, leftvertex, newvertex) < 0.0) {
      //  TRACE("Internal error in insertvertex():\n");
      //  TRACE("  Clockwise triangle after vertex insertion (top).\n");
    }
    if (counterclockwise(m, b, leftvertex, botvertex, newvertex) < 0.0) {
      //  TRACE("Internal error in insertvertex():\n");
      //  TRACE("  Clockwise triangle after vertex insertion (left).\n");
    }
    if (counterclockwise(m, b, botvertex, rightvertex, newvertex) < 0.0) {
      //  TRACE("Internal error in insertvertex():\n");
      //  TRACE("  Clockwise triangle after vertex insertion (right).\n");
    }
#endif /* SELF_CHECK */
    if (b->verbose > 2) {
      //  TRACE("  Updating top ");
     // printtriangle(m, b, &horiz);
      //  TRACE("  Creating left ");
     // printtriangle(m, b, &newbotleft);
      //  TRACE("  Creating right ");
    //  printtriangle(m, b, &newbotright);
    }
  }

  /* The insertion is successful by default, unless an encroached */
  /*   subsegment is found.                                       */
  success = SUCCESSFULVERTEX;
  /* Circle around the newly inserted vertex, checking each edge opposite */
  /*   it for the Delaunay property.  Non-Delaunay edges are flipped.     */
  /*   `horiz' is always the edge being checked.  `first' marks where to  */
  /*   stop circling.                                                     */
  org(horiz, first);
  rightvertex = first;
  dest(horiz, leftvertex);
  /* Circle until finished. */
  while (1) {
    /* By default, the edge will be flipped. */
    doflip = 1;

    if (m->checksegments) {
      /* Check for a subsegment, which cannot be flipped. */
      tspivot(horiz, checksubseg);
      if (checksubseg.ss != m->dummysub) {
        /* The edge is a subsegment and cannot be flipped. */
        doflip = 0;
#ifndef CDT_ONLY
        if (segmentflaws) {
          /* Does the new vertex encroach upon this subsegment? */
          if (checkseg4encroach(m, b, &checksubseg, iradius)) {
            success = ENCROACHINGVERTEX;
          }
        }
#endif /* not CDT_ONLY */
      }
    }

    if (doflip) {
      /* Check if the edge is a boundary edge. */
      sym(horiz, top);
      if (top.tri == m->dummytri) {
        /* The edge is a boundary edge and cannot be flipped. */
        doflip = 0;
      } else {
        /* Find the vertex on the other side of the edge. */
        apex(top, farvertex);
        /* In the incremental Delaunay triangulation algorithm, any of      */
        /*   `leftvertex', `rightvertex', and `farvertex' could be vertices */
        /*   of the triangular bounding box.  These vertices must be        */
        /*   treated as if they are infinitely distant, even though their   */
        /*   "coordinates" are not.                                         */
        if ((leftvertex == m->infvertex1) || (leftvertex == m->infvertex2) ||
            (leftvertex == m->infvertex3)) {
          /* `leftvertex' is infinitely distant.  Check the convexity of  */
          /*   the boundary of the triangulation.  'farvertex' might be   */
          /*   infinite as well, but trust me, this same condition should */
          /*   be applied.                                                */
          doflip = counterclockwise(m, b, newvertex, rightvertex, farvertex)
                   > 0.0;
        } else if ((rightvertex == m->infvertex1) ||
                   (rightvertex == m->infvertex2) ||
                   (rightvertex == m->infvertex3)) {
          /* `rightvertex' is infinitely distant.  Check the convexity of */
          /*   the boundary of the triangulation.  'farvertex' might be   */
          /*   infinite as well, but trust me, this same condition should */
          /*   be applied.                                                */
          doflip = counterclockwise(m, b, farvertex, leftvertex, newvertex)
                   > 0.0;
        } else if ((farvertex == m->infvertex1) ||
                   (farvertex == m->infvertex2) ||
                   (farvertex == m->infvertex3)) {
          /* `farvertex' is infinitely distant and cannot be inside */
          /*   the circumcircle of the triangle `horiz'.            */
          doflip = 0;
        } else {
          /* Test whether the edge is locally Delaunay. */
          doflip = incircle(m, b, leftvertex, newvertex, rightvertex,
                            farvertex) > 0.0;
        }
        if (doflip) {
          /* We made it!  Flip the edge `horiz' by rotating its containing */
          /*   quadrilateral (the two triangles adjacent to `horiz').      */
          /* Identify the casing of the quadrilateral. */
          lprev(top, topleft);
          sym(topleft, toplcasing);
          lnext(top, topright);
          sym(topright, toprcasing);
          lnext(horiz, botleft);
          sym(botleft, botlcasing);
          lprev(horiz, botright);
          sym(botright, botrcasing);
          /* Rotate the quadrilateral one-quarter turn counterclockwise. */
          bond(topleft, botlcasing);
          bond(botleft, botrcasing);
          bond(botright, toprcasing);
          bond(topright, toplcasing);
          if (m->checksegments) {
            /* Check for subsegments and rebond them to the quadrilateral. */
            tspivot(topleft, toplsubseg);
            tspivot(botleft, botlsubseg);
            tspivot(botright, botrsubseg);
            tspivot(topright, toprsubseg);
            if (toplsubseg.ss == m->dummysub) {
              tsdissolve(topright);
            } else {
              tsbond(topright, toplsubseg);
            }
            if (botlsubseg.ss == m->dummysub) {
              tsdissolve(topleft);
            } else {
              tsbond(topleft, botlsubseg);
            }
            if (botrsubseg.ss == m->dummysub) {
              tsdissolve(botleft);
            } else {
              tsbond(botleft, botrsubseg);
            }
            if (toprsubseg.ss == m->dummysub) {
              tsdissolve(botright);
            } else {
              tsbond(botright, toprsubseg);
            }
          }
          /* New vertex assignments for the rotated quadrilateral. */
          setorg(horiz, farvertex);
          setdest(horiz, newvertex);
          setapex(horiz, rightvertex);
          setorg(top, newvertex);
          setdest(top, farvertex);
          setapex(top, leftvertex);
          for (i = 0; i < m->eextras; i++) {
            /* Take the average of the two triangles' attributes. */
            attrib = 0.5 * (elemattribute(top, i) + elemattribute(horiz, i));
            setelemattribute(top, i, attrib);
            setelemattribute(horiz, i, attrib);
          }
          if (b->vararea) {
            if ((areabound(top) <= 0.0) || (areabound(horiz) <= 0.0)) {
              area = -1.0;
            } else {
              /* Take the average of the two triangles' area constraints.    */
              /*   This prevents small area constraints from migrating a     */
              /*   long, long way from their original location due to flips. */
              area = 0.5 * (areabound(top) + areabound(horiz));
            }
            setareabound(top, area);
            setareabound(horiz, area);
          }

          if (m->checkquality) {
            newflip = (struct flipstacker *) poolalloc(&m->flipstackers);
            newflip->flippedtri = encode(horiz);
            newflip->prevflip = m->lastflip;
            m->lastflip = newflip;
          }

#ifdef SELF_CHECK
          if (newvertex != (vertex) NULL) {
            if (counterclockwise(m, b, leftvertex, newvertex, rightvertex) <
                0.0) {
              //  TRACE("Internal error in insertvertex():\n");
              //  TRACE("  Clockwise triangle prior to edge flip (bottom).\n");
            }
            /* The following test has been removed because constrainededge() */
            /*   sometimes generates inverted triangles that insertvertex()  */
            /*   removes.                                                    */
/*
            if (counterclockwise(m, b, rightvertex, farvertex, leftvertex) <
                0.0) {
              //  TRACE("Internal error in insertvertex():\n");
              //  TRACE("  Clockwise triangle prior to edge flip (top).\n");
            }
*/
            if (counterclockwise(m, b, farvertex, leftvertex, newvertex) <
                0.0) {
              //  TRACE("Internal error in insertvertex():\n");
              //  TRACE("  Clockwise triangle after edge flip (left).\n");
            }
            if (counterclockwise(m, b, newvertex, rightvertex, farvertex) <
                0.0) {
              //  TRACE("Internal error in insertvertex():\n");
              //  TRACE("  Clockwise triangle after edge flip (right).\n");
            }
          }
#endif /* SELF_CHECK */
          if (b->verbose > 2) {
            //  TRACE("  Edge flip results in left ");
            lnextself(topleft);
          //  printtriangle(m, b, &topleft);
            //  TRACE("  and right ");
          //  printtriangle(m, b, &horiz);
          }
          /* On the next iterations, consider the two edges that were  */
          /*   exposed (this is, are now visible to the newly inserted */
          /*   vertex) by the edge flip.                               */
          lprevself(horiz);
          leftvertex = farvertex;
        }
      }
    }
    if (!doflip) {
      /* The handle `horiz' is accepted as locally Delaunay. */
#ifndef CDT_ONLY
      if (triflaws) {
        /* Check the triangle `horiz' for quality. */
        testtriangle(m, b, &horiz);
      }
#endif /* not CDT_ONLY */
      /* Look for the next edge around the newly inserted vertex. */
      lnextself(horiz);
      sym(horiz, testtri);
      /* Check for finishing a complete revolution about the new vertex, or */
      /*   falling outside  of the triangulation.  The latter will happen   */
      /*   when a vertex is inserted at a boundary.                         */
      if ((leftvertex == first) || (testtri.tri == m->dummytri)) {
        /* We're done.  Return a triangle whose origin is the new vertex. */
        lnext(horiz, *searchtri);
        lnext(horiz, m->recenttri);
        return success;
      }
      /* Finish finding the next edge around the newly inserted vertex. */
      lnext(testtri, horiz);
      rightvertex = leftvertex;
      dest(horiz, leftvertex);
    }
  }
}

void triangulatepolygon(struct mesh *m, struct behavior *b,
                        struct otri *firstedge, struct otri *lastedge,
                        int edgecount, int doflip, int triflaws)
{
  struct otri testtri;
  struct otri besttri;
  struct otri tempedge;
  vertex leftbasevertex, rightbasevertex;
  vertex testvertex;
  vertex bestvertex;
  int bestnumber;
  int i;
  triangle ptr;   /* Temporary variable used by sym(), onext(), and oprev(). */

  /* Identify the base vertices. */
  apex(*lastedge, leftbasevertex);
  dest(*firstedge, rightbasevertex);
  if (b->verbose > 2) {
    //  TRACE("  Triangulating interior polygon at edge\n");
    //  TRACE("    (%.12g, %.12g) (%.12g, %.12g)\n", leftbasevertex[0],
//           leftbasevertex[1], rightbasevertex[0], rightbasevertex[1]);
  }
  /* Find the best vertex to connect the base to. */
  onext(*firstedge, besttri);
  dest(besttri, bestvertex);
  otricopy(besttri, testtri);
  bestnumber = 1;
  for (i = 2; i <= edgecount - 2; i++) {
    onextself(testtri);
    dest(testtri, testvertex);
    /* Is this a better vertex? */
    if (incircle(m, b, leftbasevertex, rightbasevertex, bestvertex,
                 testvertex) > 0.0) {
      otricopy(testtri, besttri);
      bestvertex = testvertex;
      bestnumber = i;
    }
  }
  if (b->verbose > 2) {
    //  TRACE("    Connecting edge to (%.12g, %.12g)\n", bestvertex[0],
//           bestvertex[1]);
  }
  if (bestnumber > 1) {
    /* Recursively triangulate the smaller polygon on the right. */
    oprev(besttri, tempedge);
    triangulatepolygon(m, b, firstedge, &tempedge, bestnumber + 1, 1,
                       triflaws);
  }
  if (bestnumber < edgecount - 2) {
    /* Recursively triangulate the smaller polygon on the left. */
    sym(besttri, tempedge);
    triangulatepolygon(m, b, &besttri, lastedge, edgecount - bestnumber, 1,
                       triflaws);
    /* Find `besttri' again; it may have been lost to edge flips. */
    sym(tempedge, besttri);
  }
  if (doflip) {
    /* Do one final edge flip. */
    flip(m, b, &besttri);
#ifndef CDT_ONLY
    if (triflaws) {
      /* Check the quality of the newly committed triangle. */
      sym(besttri, testtri);
      testtriangle(m, b, &testtri);
    }
#endif /* not CDT_ONLY */
  }
  /* Return the base triangle. */
  otricopy(besttri, *lastedge);
}

#ifndef CDT_ONLY

void deletevertex(struct mesh *m, struct behavior *b, struct otri *deltri)
{
  struct otri countingtri;
  struct otri firstedge, lastedge;
  struct otri deltriright;
  struct otri lefttri, righttri;
  struct otri leftcasing, rightcasing;
  struct osub leftsubseg, rightsubseg;
  vertex delvertex;
  vertex neworg;
  int edgecount;
  triangle ptr;   /* Temporary variable used by sym(), onext(), and oprev(). */
  subseg sptr;                      /* Temporary variable used by tspivot(). */

  org(*deltri, delvertex);
  if (b->verbose > 1) {
    //  TRACE("  Deleting (%.12g, %.12g).\n", delvertex[0], delvertex[1]);
  }
  vertexdealloc(m, delvertex);

  /* Count the degree of the vertex being deleted. */
  onext(*deltri, countingtri);
  edgecount = 1;
  while (!otriequal(*deltri, countingtri)) {
#ifdef SELF_CHECK
    if (countingtri.tri == m->dummytri) {
      //  TRACE("Internal error in deletevertex():\n");
      //  TRACE("  Attempt to delete boundary vertex.\n");
      internalerror();
    }
#endif /* SELF_CHECK */
    edgecount++;
    onextself(countingtri);
  }

#ifdef SELF_CHECK
  if (edgecount < 3) {
    //  TRACE("Internal error in deletevertex():\n  Vertex has degree %d.\n",
           edgecount);
    internalerror();
  }
#endif /* SELF_CHECK */
  if (edgecount > 3) {
    /* Triangulate the polygon defined by the union of all triangles */
    /*   adjacent to the vertex being deleted.  Check the quality of */
    /*   the resulting triangles.                                    */
    onext(*deltri, firstedge);
    oprev(*deltri, lastedge);
    triangulatepolygon(m, b, &firstedge, &lastedge, edgecount, 0,
                       !b->nobisect);
  }
  /* Splice out two triangles. */
  lprev(*deltri, deltriright);
  dnext(*deltri, lefttri);
  sym(lefttri, leftcasing);
  oprev(deltriright, righttri);
  sym(righttri, rightcasing);
  bond(*deltri, leftcasing);
  bond(deltriright, rightcasing);
  tspivot(lefttri, leftsubseg);
  if (leftsubseg.ss != m->dummysub) {
    tsbond(*deltri, leftsubseg);
  }
  tspivot(righttri, rightsubseg);
  if (rightsubseg.ss != m->dummysub) {
    tsbond(deltriright, rightsubseg);
  }

  /* Set the new origin of `deltri' and check its quality. */
  org(lefttri, neworg);
  setorg(*deltri, neworg);
  if (!b->nobisect) {
    testtriangle(m, b, deltri);
  }

  /* Delete the two spliced-out triangles. */
  triangledealloc(m, lefttri.tri);
  triangledealloc(m, righttri.tri);
}

#endif /* not CDT_ONLY */

#ifndef CDT_ONLY

void undovertex(struct mesh *m, struct behavior *b)
{
  struct otri fliptri;
  struct otri botleft, botright, topright;
  struct otri botlcasing, botrcasing, toprcasing;
  struct otri gluetri;
  struct osub botlsubseg, botrsubseg, toprsubseg;
  vertex botvertex, rightvertex;
  triangle ptr;                         /* Temporary variable used by sym(). */
  subseg sptr;                      /* Temporary variable used by tspivot(). */

  /* Walk through the list of transformations (flips and a vertex insertion) */
  /*   in the reverse of the order in which they were done, and undo them.   */
  while (m->lastflip != (struct flipstacker *) NULL) {
    /* Find a triangle involved in the last unreversed transformation. */
    decode(m->lastflip->flippedtri, fliptri);

    /* We are reversing one of three transformations:  a trisection of one */
    /*   triangle into three (by inserting a vertex in the triangle), a    */
    /*   bisection of two triangles into four (by inserting a vertex in an */
    /*   edge), or an edge flip.                                           */
    if (m->lastflip->prevflip == (struct flipstacker *) NULL) {
      /* Restore a triangle that was split into three triangles, */
      /*   so it is again one triangle.                          */
      dprev(fliptri, botleft);
      lnextself(botleft);
      onext(fliptri, botright);
      lprevself(botright);
      sym(botleft, botlcasing);
      sym(botright, botrcasing);
      dest(botleft, botvertex);

      setapex(fliptri, botvertex);
      lnextself(fliptri);
      bond(fliptri, botlcasing);
      tspivot(botleft, botlsubseg);
      tsbond(fliptri, botlsubseg);
      lnextself(fliptri);
      bond(fliptri, botrcasing);
      tspivot(botright, botrsubseg);
      tsbond(fliptri, botrsubseg);

      /* Delete the two spliced-out triangles. */
      triangledealloc(m, botleft.tri);
      triangledealloc(m, botright.tri);
    } else if (m->lastflip->prevflip == (struct flipstacker *) &insertvertex) {
      /* Restore two triangles that were split into four triangles, */
      /*   so they are again two triangles.                         */
      lprev(fliptri, gluetri);
      sym(gluetri, botright);
      lnextself(botright);
      sym(botright, botrcasing);
      dest(botright, rightvertex);

      setorg(fliptri, rightvertex);
      bond(gluetri, botrcasing);
      tspivot(botright, botrsubseg);
      tsbond(gluetri, botrsubseg);

      /* Delete the spliced-out triangle. */
      triangledealloc(m, botright.tri);

      sym(fliptri, gluetri);
      if (gluetri.tri != m->dummytri) {
        lnextself(gluetri);
        dnext(gluetri, topright);
        sym(topright, toprcasing);

        setorg(gluetri, rightvertex);
        bond(gluetri, toprcasing);
        tspivot(topright, toprsubseg);
        tsbond(gluetri, toprsubseg);

        /* Delete the spliced-out triangle. */
        triangledealloc(m, topright.tri);
      }

      /* This is the end of the list, sneakily encoded. */
      m->lastflip->prevflip = (struct flipstacker *) NULL;
    } else {
      /* Undo an edge flip. */
      unflip(m, b, &fliptri);
    }

    /* Go on and process the next transformation. */
    m->lastflip = m->lastflip->prevflip;
  }
}

#endif /* not CDT_ONLY */

void vertexsort(vertex *sortarray, int arraysize)
{
  int left, right;
  int pivot;
  sgFloat pivotx, pivoty;
  vertex temp;

  if (arraysize == 2) {
    /* Recursive base case. */
    if ((sortarray[0][0] > sortarray[1][0]) ||
        ((sortarray[0][0] == sortarray[1][0]) &&
         (sortarray[0][1] > sortarray[1][1]))) {
      temp = sortarray[1];
      sortarray[1] = sortarray[0];
      sortarray[0] = temp;
    }
    return;
  }
  /* Choose a random pivot to split the array. */
  pivot = (int) randomnation((unsigned int) arraysize);
  pivotx = sortarray[pivot][0];
  pivoty = sortarray[pivot][1];
  /* Split the array. */
  left = -1;
  right = arraysize;
  while (left < right) {
    /* Search for a vertex whose x-coordinate is too large for the left. */
    do {
      left++;
    } while ((left <= right) && ((sortarray[left][0] < pivotx) ||
                                 ((sortarray[left][0] == pivotx) &&
                                  (sortarray[left][1] < pivoty))));
    /* Search for a vertex whose x-coordinate is too small for the right. */
    do {
      right--;
    } while ((left <= right) && ((sortarray[right][0] > pivotx) ||
                                 ((sortarray[right][0] == pivotx) &&
                                  (sortarray[right][1] > pivoty))));
    if (left < right) {
      /* Swap the left and right vertices. */
      temp = sortarray[left];
      sortarray[left] = sortarray[right];
      sortarray[right] = temp;
    }
  }
  if (left > 1) {
    /* Recursively sort the left subset. */
    vertexsort(sortarray, left);
  }
  if (right < arraysize - 2) {
    /* Recursively sort the right subset. */
    vertexsort(&sortarray[right + 1], arraysize - right - 1);
  }
}

void vertexmedian(vertex *sortarray, int arraysize, int median, int axis)
{
  int left, right;
  int pivot;
  sgFloat pivot1, pivot2;
  vertex temp;

  if (arraysize == 2) {
    /* Recursive base case. */
    if ((sortarray[0][axis] > sortarray[1][axis]) ||
        ((sortarray[0][axis] == sortarray[1][axis]) &&
         (sortarray[0][1 - axis] > sortarray[1][1 - axis]))) {
      temp = sortarray[1];
      sortarray[1] = sortarray[0];
      sortarray[0] = temp;
    }
    return;
  }
  /* Choose a random pivot to split the array. */
  pivot = (int) randomnation((unsigned int) arraysize);
  pivot1 = sortarray[pivot][axis];
  pivot2 = sortarray[pivot][1 - axis];
  /* Split the array. */
  left = -1;
  right = arraysize;
  while (left < right) {
    /* Search for a vertex whose x-coordinate is too large for the left. */
    do {
      left++;
    } while ((left <= right) && ((sortarray[left][axis] < pivot1) ||
                                 ((sortarray[left][axis] == pivot1) &&
                                  (sortarray[left][1 - axis] < pivot2))));
    /* Search for a vertex whose x-coordinate is too small for the right. */
    do {
      right--;
    } while ((left <= right) && ((sortarray[right][axis] > pivot1) ||
                                 ((sortarray[right][axis] == pivot1) &&
                                  (sortarray[right][1 - axis] > pivot2))));
    if (left < right) {
      /* Swap the left and right vertices. */
      temp = sortarray[left];
      sortarray[left] = sortarray[right];
      sortarray[right] = temp;
    }
  }
  /* Unlike in vertexsort(), at most one of the following */
  /*   conditionals is true.                             */
  if (left > median) {
    /* Recursively shuffle the left subset. */
    vertexmedian(sortarray, left, median, axis);
  }
  if (right < median - 1) {
    /* Recursively shuffle the right subset. */
    vertexmedian(&sortarray[right + 1], arraysize - right - 1,
                 median - right - 1, axis);
  }
}

void alternateaxes(vertex *sortarray, int arraysize, int axis)
{
  int divider;

  divider = arraysize >> 1;
  if (arraysize <= 3) {
    /* Recursive base case:  subsets of two or three vertices will be    */
    /*   handled specially, and should always be sorted by x-coordinate. */
    axis = 0;
  }
  /* Partition with a horizontal or vertical cut. */
  vertexmedian(sortarray, arraysize, divider, axis);
  /* Recursively partition the subsets with a cross cut. */
  if (arraysize - divider >= 2) {
    if (divider >= 2) {
      alternateaxes(sortarray, divider, 1 - axis);
    }
    alternateaxes(&sortarray[divider], arraysize - divider, 1 - axis);
  }
}

void mergehulls(struct mesh *m, struct behavior *b, struct otri *farleft,
                struct otri *innerleft, struct otri *innerright,
                struct otri *farright, int axis)
{
  struct otri leftcand, rightcand;
  struct otri baseedge;
  struct otri nextedge;
  struct otri sidecasing, topcasing, outercasing;
  struct otri checkedge;
  vertex innerleftdest;
  vertex innerrightorg;
  vertex innerleftapex, innerrightapex;
  vertex farleftpt, farrightpt;
  vertex farleftapex, farrightapex;
  vertex lowerleft, lowerright;
  vertex upperleft, upperright;
  vertex nextapex;
  vertex checkvertex;
  int changemade;
  int badedge;
  int leftfinished, rightfinished;
  triangle ptr;                         /* Temporary variable used by sym(). */

  dest(*innerleft, innerleftdest);
  apex(*innerleft, innerleftapex);
  org(*innerright, innerrightorg);
  apex(*innerright, innerrightapex);
  /* Special treatment for horizontal cuts. */
  if (b->dwyer && (axis == 1)) {
    org(*farleft, farleftpt);
    apex(*farleft, farleftapex);
    dest(*farright, farrightpt);
    apex(*farright, farrightapex);
    /* The pointers to the extremal vertices are shifted to point to the */
    /*   topmost and bottommost vertex of each hull, rather than the     */
    /*   leftmost and rightmost vertices.                                */
    while (farleftapex[1] < farleftpt[1]) {
      lnextself(*farleft);
      symself(*farleft);
      farleftpt = farleftapex;
      apex(*farleft, farleftapex);
    }
    sym(*innerleft, checkedge);
    apex(checkedge, checkvertex);
    while (checkvertex[1] > innerleftdest[1]) {
      lnext(checkedge, *innerleft);
      innerleftapex = innerleftdest;
      innerleftdest = checkvertex;
      sym(*innerleft, checkedge);
      apex(checkedge, checkvertex);
    }
    while (innerrightapex[1] < innerrightorg[1]) {
      lnextself(*innerright);
      symself(*innerright);
      innerrightorg = innerrightapex;
      apex(*innerright, innerrightapex);
    }
    sym(*farright, checkedge);
    apex(checkedge, checkvertex);
    while (checkvertex[1] > farrightpt[1]) {
      lnext(checkedge, *farright);
      farrightapex = farrightpt;
      farrightpt = checkvertex;
      sym(*farright, checkedge);
      apex(checkedge, checkvertex);
    }
  }
  /* Find a line tangent to and below both hulls. */
  do {
    changemade = 0;
    /* Make innerleftdest the "bottommost" vertex of the left hull. */
    if (counterclockwise(m, b, innerleftdest, innerleftapex, innerrightorg) >
        0.0) {
      lprevself(*innerleft);
      symself(*innerleft);
      innerleftdest = innerleftapex;
      apex(*innerleft, innerleftapex);
      changemade = 1;
    }
    /* Make innerrightorg the "bottommost" vertex of the right hull. */
    if (counterclockwise(m, b, innerrightapex, innerrightorg, innerleftdest) >
        0.0) {
      lnextself(*innerright);
      symself(*innerright);
      innerrightorg = innerrightapex;
      apex(*innerright, innerrightapex);
      changemade = 1;
    }
  } while (changemade);
  /* Find the two candidates to be the next "gear tooth." */
  sym(*innerleft, leftcand);
  sym(*innerright, rightcand);
  /* Create the bottom new bounding triangle. */
  maketriangle(m, b, &baseedge);
  /* Connect it to the bounding boxes of the left and right triangulations. */
  bond(baseedge, *innerleft);
  lnextself(baseedge);
  bond(baseedge, *innerright);
  lnextself(baseedge);
  setorg(baseedge, innerrightorg);
  setdest(baseedge, innerleftdest);
  /* Apex is intentionally left NULL. */
  if (b->verbose > 2) {
    //  TRACE("  Creating base bounding ");
  //  printtriangle(m, b, &baseedge);
  }
  /* Fix the extreme triangles if necessary. */
  org(*farleft, farleftpt);
  if (innerleftdest == farleftpt) {
    lnext(baseedge, *farleft);
  }
  dest(*farright, farrightpt);
  if (innerrightorg == farrightpt) {
    lprev(baseedge, *farright);
  }
  /* The vertices of the current knitting edge. */
  lowerleft = innerleftdest;
  lowerright = innerrightorg;
  /* The candidate vertices for knitting. */
  apex(leftcand, upperleft);
  apex(rightcand, upperright);
  /* Walk up the gap between the two triangulations, knitting them together. */
  while (1) {
    /* Have we reached the top?  (This isn't quite the right question,       */
    /*   because even though the left triangulation might seem finished now, */
    /*   moving up on the right triangulation might reveal a new vertex of   */
    /*   the left triangulation.  And vice-versa.)                           */
    leftfinished = counterclockwise(m, b, upperleft, lowerleft, lowerright) <=
                   0.0;
    rightfinished = counterclockwise(m, b, upperright, lowerleft, lowerright)
                 <= 0.0;
    if (leftfinished && rightfinished) {
      /* Create the top new bounding triangle. */
      maketriangle(m, b, &nextedge);
      setorg(nextedge, lowerleft);
      setdest(nextedge, lowerright);
      /* Apex is intentionally left NULL. */
      /* Connect it to the bounding boxes of the two triangulations. */
      bond(nextedge, baseedge);
      lnextself(nextedge);
      bond(nextedge, rightcand);
      lnextself(nextedge);
      bond(nextedge, leftcand);
      if (b->verbose > 2) {
        //  TRACE("  Creating top bounding ");
//        printtriangle(m, b, &nextedge);
      }
      /* Special treatment for horizontal cuts. */
      if (b->dwyer && (axis == 1)) {
        org(*farleft, farleftpt);
        apex(*farleft, farleftapex);
        dest(*farright, farrightpt);
        apex(*farright, farrightapex);
        sym(*farleft, checkedge);
        apex(checkedge, checkvertex);
        /* The pointers to the extremal vertices are restored to the  */
        /*   leftmost and rightmost vertices (rather than topmost and */
        /*   bottommost).                                             */
        while (checkvertex[0] < farleftpt[0]) {
          lprev(checkedge, *farleft);
          farleftapex = farleftpt;
          farleftpt = checkvertex;
          sym(*farleft, checkedge);
          apex(checkedge, checkvertex);
        }
        while (farrightapex[0] > farrightpt[0]) {
          lprevself(*farright);
          symself(*farright);
          farrightpt = farrightapex;
          apex(*farright, farrightapex);
        }
      }
      return;
    }
    /* Consider eliminating edges from the left triangulation. */
    if (!leftfinished) {
      /* What vertex would be exposed if an edge were deleted? */
      lprev(leftcand, nextedge);
      symself(nextedge);
      apex(nextedge, nextapex);
      /* If nextapex is NULL, then no vertex would be exposed; the */
      /*   triangulation would have been eaten right through.      */
      if (nextapex != (vertex) NULL) {
        /* Check whether the edge is Delaunay. */
        badedge = incircle(m, b, lowerleft, lowerright, upperleft, nextapex) >
                  0.0;
        while (badedge) {
          /* Eliminate the edge with an edge flip.  As a result, the    */
          /*   left triangulation will have one more boundary triangle. */
          lnextself(nextedge);
          sym(nextedge, topcasing);
          lnextself(nextedge);
          sym(nextedge, sidecasing);
          bond(nextedge, topcasing);
          bond(leftcand, sidecasing);
          lnextself(leftcand);
          sym(leftcand, outercasing);
          lprevself(nextedge);
          bond(nextedge, outercasing);
          /* Correct the vertices to reflect the edge flip. */
          setorg(leftcand, lowerleft);
          setdest(leftcand, NULL);
          setapex(leftcand, nextapex);
          setorg(nextedge, NULL);
          setdest(nextedge, upperleft);
          setapex(nextedge, nextapex);
          /* Consider the newly exposed vertex. */
          upperleft = nextapex;
          /* What vertex would be exposed if another edge were deleted? */
          otricopy(sidecasing, nextedge);
          apex(nextedge, nextapex);
          if (nextapex != (vertex) NULL) {
            /* Check whether the edge is Delaunay. */
            badedge = incircle(m, b, lowerleft, lowerright, upperleft,
                               nextapex) > 0.0;
          } else {
            /* Avoid eating right through the triangulation. */
            badedge = 0;
          }
        }
      }
    }
    /* Consider eliminating edges from the right triangulation. */
    if (!rightfinished) {
      /* What vertex would be exposed if an edge were deleted? */
      lnext(rightcand, nextedge);
      symself(nextedge);
      apex(nextedge, nextapex);
      /* If nextapex is NULL, then no vertex would be exposed; the */
      /*   triangulation would have been eaten right through.      */
      if (nextapex != (vertex) NULL) {
        /* Check whether the edge is Delaunay. */
        badedge = incircle(m, b, lowerleft, lowerright, upperright, nextapex) >
                  0.0;
        while (badedge) {
          /* Eliminate the edge with an edge flip.  As a result, the     */
          /*   right triangulation will have one more boundary triangle. */
          lprevself(nextedge);
          sym(nextedge, topcasing);
          lprevself(nextedge);
          sym(nextedge, sidecasing);
          bond(nextedge, topcasing);
          bond(rightcand, sidecasing);
          lprevself(rightcand);
          sym(rightcand, outercasing);
          lnextself(nextedge);
          bond(nextedge, outercasing);
          /* Correct the vertices to reflect the edge flip. */
          setorg(rightcand, NULL);
          setdest(rightcand, lowerright);
          setapex(rightcand, nextapex);
          setorg(nextedge, upperright);
          setdest(nextedge, NULL);
          setapex(nextedge, nextapex);
          /* Consider the newly exposed vertex. */
          upperright = nextapex;
          /* What vertex would be exposed if another edge were deleted? */
          otricopy(sidecasing, nextedge);
          apex(nextedge, nextapex);
          if (nextapex != (vertex) NULL) {
            /* Check whether the edge is Delaunay. */
            badedge = incircle(m, b, lowerleft, lowerright, upperright,
                               nextapex) > 0.0;
          } else {
            /* Avoid eating right through the triangulation. */
            badedge = 0;
          }
        }
      }
    }
    if (leftfinished || (!rightfinished &&
           (incircle(m, b, upperleft, lowerleft, lowerright, upperright) >
            0.0))) {
      /* Knit the triangulations, adding an edge from `lowerleft' */
      /*   to `upperright'.                                       */
      bond(baseedge, rightcand);
      lprev(rightcand, baseedge);
      setdest(baseedge, lowerleft);
      lowerright = upperright;
      sym(baseedge, rightcand);
      apex(rightcand, upperright);
    } else {
      /* Knit the triangulations, adding an edge from `upperleft' */
      /*   to `lowerright'.                                       */
      bond(baseedge, leftcand);
      lnext(leftcand, baseedge);
      setorg(baseedge, lowerright);
      lowerleft = upperleft;
      sym(baseedge, leftcand);
      apex(leftcand, upperleft);
    }
    if (b->verbose > 2) {
      //  TRACE("  Connecting ");
//      printtriangle(m, b, &baseedge);
    }
  }
}

void divconqrecurse(struct mesh *m, struct behavior *b, vertex *sortarray,
                    int vertices, int axis,
                    struct otri *farleft, struct otri *farright)
{
  struct otri midtri, tri1, tri2, tri3;
  struct otri innerleft, innerright;
  sgFloat area;
  int divider;

  if (b->verbose > 2) {
    //  TRACE("  Triangulating %d vertices.\n", vertices);
  }
  if (vertices == 2) {
    /* The triangulation of two vertices is an edge.  An edge is */
    /*   represented by two bounding triangles.                  */
    maketriangle(m, b, farleft);
    setorg(*farleft, sortarray[0]);
    setdest(*farleft, sortarray[1]);
    /* The apex is intentionally left NULL. */
    maketriangle(m, b, farright);
    setorg(*farright, sortarray[1]);
    setdest(*farright, sortarray[0]);
    /* The apex is intentionally left NULL. */
    bond(*farleft, *farright);
    lprevself(*farleft);
    lnextself(*farright);
    bond(*farleft, *farright);
    lprevself(*farleft);
    lnextself(*farright);
    bond(*farleft, *farright);
    if (b->verbose > 2) {
      //  TRACE("  Creating ");
//      printtriangle(m, b, farleft);
      //  TRACE("  Creating ");
 //     printtriangle(m, b, farright);
    }
    /* Ensure that the origin of `farleft' is sortarray[0]. */
    lprev(*farright, *farleft);
    return;
  } else if (vertices == 3) {
    /* The triangulation of three vertices is either a triangle (with */
    /*   three bounding triangles) or two edges (with four bounding   */
    /*   triangles).  In either case, four triangles are created.     */
    maketriangle(m, b, &midtri);
    maketriangle(m, b, &tri1);
    maketriangle(m, b, &tri2);
    maketriangle(m, b, &tri3);
    area = counterclockwise(m, b, sortarray[0], sortarray[1], sortarray[2]);
    if (area == 0.0) {
      /* Three collinear vertices; the triangulation is two edges. */
      setorg(midtri, sortarray[0]);
      setdest(midtri, sortarray[1]);
      setorg(tri1, sortarray[1]);
      setdest(tri1, sortarray[0]);
      setorg(tri2, sortarray[2]);
      setdest(tri2, sortarray[1]);
      setorg(tri3, sortarray[1]);
      setdest(tri3, sortarray[2]);
      /* All apices are intentionally left NULL. */
      bond(midtri, tri1);
      bond(tri2, tri3);
      lnextself(midtri);
      lprevself(tri1);
      lnextself(tri2);
      lprevself(tri3);
      bond(midtri, tri3);
      bond(tri1, tri2);
      lnextself(midtri);
      lprevself(tri1);
      lnextself(tri2);
      lprevself(tri3);
      bond(midtri, tri1);
      bond(tri2, tri3);
      /* Ensure that the origin of `farleft' is sortarray[0]. */
      otricopy(tri1, *farleft);
      /* Ensure that the destination of `farright' is sortarray[2]. */
      otricopy(tri2, *farright);
    } else {
      /* The three vertices are not collinear; the triangulation is one */
      /*   triangle, namely `midtri'.                                   */
      setorg(midtri, sortarray[0]);
      setdest(tri1, sortarray[0]);
      setorg(tri3, sortarray[0]);
      /* Apices of tri1, tri2, and tri3 are left NULL. */
      if (area > 0.0) {
        /* The vertices are in counterclockwise order. */
        setdest(midtri, sortarray[1]);
        setorg(tri1, sortarray[1]);
        setdest(tri2, sortarray[1]);
        setapex(midtri, sortarray[2]);
        setorg(tri2, sortarray[2]);
        setdest(tri3, sortarray[2]);
      } else {
        /* The vertices are in clockwise order. */
        setdest(midtri, sortarray[2]);
        setorg(tri1, sortarray[2]);
        setdest(tri2, sortarray[2]);
        setapex(midtri, sortarray[1]);
        setorg(tri2, sortarray[1]);
        setdest(tri3, sortarray[1]);
      }
      /* The topology does not depend on how the vertices are ordered. */
      bond(midtri, tri1);
      lnextself(midtri);
      bond(midtri, tri2);
      lnextself(midtri);
      bond(midtri, tri3);
      lprevself(tri1);
      lnextself(tri2);
      bond(tri1, tri2);
      lprevself(tri1);
      lprevself(tri3);
      bond(tri1, tri3);
      lnextself(tri2);
      lprevself(tri3);
      bond(tri2, tri3);
      /* Ensure that the origin of `farleft' is sortarray[0]. */
      otricopy(tri1, *farleft);
      /* Ensure that the destination of `farright' is sortarray[2]. */
      if (area > 0.0) {
        otricopy(tri2, *farright);
      } else {
        lnext(*farleft, *farright);
      }
    }
    if (b->verbose > 2) {
      //  TRACE("  Creating ");
//      printtriangle(m, b, &midtri);
      //  TRACE("  Creating ");
  //    printtriangle(m, b, &tri1);
      //  TRACE("  Creating ");
  //    printtriangle(m, b, &tri2);
      //  TRACE("  Creating ");
  //    printtriangle(m, b, &tri3);
    }
    return;
  } else {
    /* Split the vertices in half. */
    divider = vertices >> 1;
    /* Recursively triangulate each half. */
    divconqrecurse(m, b, sortarray, divider, 1 - axis, farleft, &innerleft);
    divconqrecurse(m, b, &sortarray[divider], vertices - divider, 1 - axis,
                   &innerright, farright);
    if (b->verbose > 1) {
      //  TRACE("  Joining triangulations with %d and %d vertices.\n", divider,
      //       vertices - divider);
    }
    /* Merge the two triangulations into one. */
    mergehulls(m, b, farleft, &innerleft, &innerright, farright, axis);
  }
}

long removeghosts(struct mesh *m, struct behavior *b, struct otri *startghost)
{
  struct otri searchedge;
  struct otri dissolveedge;
  struct otri deadtriangle;
  vertex markorg;
  long hullsize;
  triangle ptr;                         /* Temporary variable used by sym(). */

  if (b->verbose) {
    //  TRACE("  Removing ghost triangles.\n");
  }
  /* Find an edge on the convex hull to start point location from. */
  lprev(*startghost, searchedge);
  symself(searchedge);
  m->dummytri[0] = encode(searchedge);
  /* Remove the bounding box and count the convex hull edges. */
  otricopy(*startghost, dissolveedge);
  hullsize = 0;
  do {
    hullsize++;
    lnext(dissolveedge, deadtriangle);
    lprevself(dissolveedge);
    symself(dissolveedge);
    /* If no PSLG is involved, set the boundary markers of all the vertices */
    /*   on the convex hull.  If a PSLG is used, this step is done later.   */
    if (!b->poly) {
      /* Watch out for the case where all the input vertices are collinear. */
      if (dissolveedge.tri != m->dummytri) {
        org(dissolveedge, markorg);
        if (vertexmark(markorg) == 0) {
          setvertexmark(markorg, 1);
        }
      }
    }
    /* Remove a bounding triangle from a convex hull triangle. */
    dissolve(dissolveedge);
    /* Find the next bounding triangle. */
    sym(deadtriangle, dissolveedge);
    /* Delete the bounding triangle. */
    triangledealloc(m, deadtriangle.tri);
  } while (!otriequal(dissolveedge, *startghost));
  return hullsize;
}

long divconqdelaunay(struct mesh *m, struct behavior *b)
{
  vertex *sortarray;
  struct otri hullleft, hullright;
  int divider;
  int i, j;

  if (b->verbose) {
    //  TRACE("  Sorting vertices.\n");
  }

  /* Allocate an array of pointers to vertices for sorting. */
  sortarray = (vertex *) trimalloc(m->invertices * (int) sizeof(vertex));
  traversalinit(&m->vertices);
  for (i = 0; i < m->invertices; i++) {
    sortarray[i] = vertextraverse(m);
  }
  /* Sort the vertices. */
  vertexsort(sortarray, m->invertices);
  /* Discard duplicate vertices, which can really mess up the algorithm. */
  i = 0;
  for (j = 1; j < m->invertices; j++) {
    if ((sortarray[i][0] == sortarray[j][0])
        && (sortarray[i][1] == sortarray[j][1])) {
      if (!b->quiet) {
        //  TRACE(
//"Warning:  A duplicate vertex at (%.12g, %.12g) appeared and was ignored.\n",
 //              sortarray[j][0], sortarray[j][1]);
      }
      setvertextype(sortarray[j], UNDEADVERTEX);
      m->undeads++;
    } else {
      i++;
      sortarray[i] = sortarray[j];
    }
  }
  i++;
  if (b->dwyer) {
    /* Re-sort the array of vertices to accommodate alternating cuts. */
    divider = i >> 1;
    if (i - divider >= 2) {
      if (divider >= 2) {
        alternateaxes(sortarray, divider, 1);
      }
      alternateaxes(&sortarray[divider], i - divider, 1);
    }
  }

  if (b->verbose) {
    //  TRACE("  Forming triangulation.\n");
  }

  /* Form the Delaunay triangulation. */
  divconqrecurse(m, b, sortarray, i, 0, &hullleft, &hullright);
  trifree((int *) sortarray);

  return removeghosts(m, b, &hullleft);
}

#ifndef REDUCED

void boundingbox(struct mesh *m, struct behavior *b)
{
  struct otri inftri;          /* Handle for the triangular bounding box. */
  sgFloat width;

  if (b->verbose) {
    //  TRACE("  Creating triangular bounding box.\n");
  }
  /* Find the width (or height, whichever is larger) of the triangulation. */
  width = m->xmax - m->xmin;
  if (m->ymax - m->ymin > width) {
    width = m->ymax - m->ymin;
  }
  if (width == 0.0) {
    width = 1.0;
  }
  /* Create the vertices of the bounding box. */
  m->infvertex1 = (vertex) trimalloc(m->vertices.itembytes);
  m->infvertex2 = (vertex) trimalloc(m->vertices.itembytes);
  m->infvertex3 = (vertex) trimalloc(m->vertices.itembytes);
  m->infvertex1[0] = m->xmin - 50.0 * width;
  m->infvertex1[1] = m->ymin - 40.0 * width;
  m->infvertex2[0] = m->xmax + 50.0 * width;
  m->infvertex2[1] = m->ymin - 40.0 * width;
  m->infvertex3[0] = 0.5 * (m->xmin + m->xmax);
  m->infvertex3[1] = m->ymax + 60.0 * width;

  /* Create the bounding box. */
  maketriangle(m, b, &inftri);
  setorg(inftri, m->infvertex1);
  setdest(inftri, m->infvertex2);
  setapex(inftri, m->infvertex3);
  /* Link dummytri to the bounding box so we can always find an */
  /*   edge to begin searching (point location) from.           */
  m->dummytri[0] = (triangle) inftri.tri;
  if (b->verbose > 2) {
    //  TRACE("  Creating ");
//    printtriangle(m, b, &inftri);
  }
}

#endif /* not REDUCED */

#ifndef REDUCED

long removebox(struct mesh *m, struct behavior *b)
{
  struct otri deadtriangle;
  struct otri searchedge;
  struct otri checkedge;
  struct otri nextedge, finaledge, dissolveedge;
  vertex markorg;
  long hullsize;
  triangle ptr;                         /* Temporary variable used by sym(). */

  if (b->verbose) {
    //  TRACE("  Removing triangular bounding box.\n");
  }
  /* Find a boundary triangle. */
  nextedge.tri = m->dummytri;
  nextedge.orient = 0;
  symself(nextedge);
  /* Mark a place to stop. */
  lprev(nextedge, finaledge);
  lnextself(nextedge);
  symself(nextedge);
  /* Find a triangle (on the boundary of the vertex set) that isn't */
  /*   a bounding box triangle.                                     */
  lprev(nextedge, searchedge);
  symself(searchedge);
  /* Check whether nextedge is another boundary triangle */
  /*   adjacent to the first one.                        */
  lnext(nextedge, checkedge);
  symself(checkedge);
  if (checkedge.tri == m->dummytri) {
    /* Go on to the next triangle.  There are only three boundary   */
    /*   triangles, and this next triangle cannot be the third one, */
    /*   so it's safe to stop here.                                 */
    lprevself(searchedge);
    symself(searchedge);
  }
  /* Find a new boundary edge to search from, as the current search */
  /*   edge lies on a bounding box triangle and will be deleted.    */
  m->dummytri[0] = encode(searchedge);
  hullsize = -2l;
  while (!otriequal(nextedge, finaledge)) {
    hullsize++;
    lprev(nextedge, dissolveedge);
    symself(dissolveedge);
    /* If not using a PSLG, the vertices should be marked now. */
    /*   (If using a PSLG, markhull() will do the job.)        */
    if (!b->poly) {
      /* Be careful!  One must check for the case where all the input     */
      /*   vertices are collinear, and thus all the triangles are part of */
      /*   the bounding box.  Otherwise, the setvertexmark() call below   */
      /*   will cause a bad pointer reference.                            */
      if (dissolveedge.tri != m->dummytri) {
        org(dissolveedge, markorg);
        if (vertexmark(markorg) == 0) {
          setvertexmark(markorg, 1);
        }
      }
    }
    /* Disconnect the bounding box triangle from the mesh triangle. */
    dissolve(dissolveedge);
    lnext(nextedge, deadtriangle);
    sym(deadtriangle, nextedge);
    /* Get rid of the bounding box triangle. */
    triangledealloc(m, deadtriangle.tri);
    /* Do we need to turn the corner? */
    if (nextedge.tri == m->dummytri) {
      /* Turn the corner. */
      otricopy(dissolveedge, nextedge);
    }
  }
  triangledealloc(m, finaledge.tri);

  trifree((int *) m->infvertex1);  /* Deallocate the bounding box vertices. */
  trifree((int *) m->infvertex2);
  trifree((int *) m->infvertex3);

  return hullsize;
}

#endif /* not REDUCED */

#ifndef REDUCED

long incrementaldelaunay(struct mesh *m, struct behavior *b)
{
  struct otri starttri;
  vertex vertexloop;

  /* Create a triangular bounding box. */
  boundingbox(m, b);
  if (b->verbose) {
    //  TRACE("  Incrementally inserting vertices.\n");
  }
  traversalinit(&m->vertices);
  vertexloop = vertextraverse(m);
  while (vertexloop != (vertex) NULL) {
    starttri.tri = m->dummytri;
    if (insertvertex(m, b, vertexloop, &starttri, (struct osub *) NULL, 0, 0,
                     0.0) == DUPLICATEVERTEX) {
      if (!b->quiet) {
        //  TRACE(
//"Warning:  A duplicate vertex at (%.12g, %.12g) appeared and was ignored.\n",
 //              vertexloop[0], vertexloop[1]);
      }
      setvertextype(vertexloop, UNDEADVERTEX);
      m->undeads++;
    }
    vertexloop = vertextraverse(m);
  }
  /* Remove the bounding box. */
  return removebox(m, b);
}

#endif /* not REDUCED */

#ifndef REDUCED

void eventheapinsert(struct event **heap, int heapsize, struct event *newevent)
{
  sgFloat eventx, eventy;
  int eventnum;
  int parent;
  int notdone;

  eventx = newevent->xkey;
  eventy = newevent->ykey;
  eventnum = heapsize;
  notdone = eventnum > 0;
  while (notdone) {
    parent = (eventnum - 1) >> 1;
    if ((heap[parent]->ykey < eventy) ||
        ((heap[parent]->ykey == eventy)
         && (heap[parent]->xkey <= eventx))) {
      notdone = 0;
    } else {
      heap[eventnum] = heap[parent];
      heap[eventnum]->heapposition = eventnum;

      eventnum = parent;
      notdone = eventnum > 0;
    }
  }
  heap[eventnum] = newevent;
  newevent->heapposition = eventnum;
}

#endif /* not REDUCED */

#ifndef REDUCED

void eventheapify(struct event **heap, int heapsize, int eventnum)
{
  struct event *thisevent;
  sgFloat eventx, eventy;
  int leftchild, rightchild;
  int smallest;
  int notdone;

  thisevent = heap[eventnum];
  eventx = thisevent->xkey;
  eventy = thisevent->ykey;
  leftchild = 2 * eventnum + 1;
  notdone = leftchild < heapsize;
  while (notdone) {
    if ((heap[leftchild]->ykey < eventy) ||
        ((heap[leftchild]->ykey == eventy)
         && (heap[leftchild]->xkey < eventx))) {
      smallest = leftchild;
    } else {
      smallest = eventnum;
    }
    rightchild = leftchild + 1;
    if (rightchild < heapsize) {
      if ((heap[rightchild]->ykey < heap[smallest]->ykey) ||
          ((heap[rightchild]->ykey == heap[smallest]->ykey)
           && (heap[rightchild]->xkey < heap[smallest]->xkey))) {
        smallest = rightchild;
      }
    }
    if (smallest == eventnum) {
      notdone = 0;
    } else {
      heap[eventnum] = heap[smallest];
      heap[eventnum]->heapposition = eventnum;
      heap[smallest] = thisevent;
      thisevent->heapposition = smallest;

      eventnum = smallest;
      leftchild = 2 * eventnum + 1;
      notdone = leftchild < heapsize;
    }
  }
}

#endif /* not REDUCED */

#ifndef REDUCED

void eventheapdelete(struct event **heap, int heapsize, int eventnum)
{
  struct event *moveevent;
  sgFloat eventx, eventy;
  int parent;
  int notdone;

  moveevent = heap[heapsize - 1];
  if (eventnum > 0) {
    eventx = moveevent->xkey;
    eventy = moveevent->ykey;
    do {
      parent = (eventnum - 1) >> 1;
      if ((heap[parent]->ykey < eventy) ||
          ((heap[parent]->ykey == eventy)
           && (heap[parent]->xkey <= eventx))) {
        notdone = 0;
      } else {
        heap[eventnum] = heap[parent];
        heap[eventnum]->heapposition = eventnum;

        eventnum = parent;
        notdone = eventnum > 0;
      }
    } while (notdone);
  }
  heap[eventnum] = moveevent;
  moveevent->heapposition = eventnum;
  eventheapify(heap, heapsize - 1, eventnum);
}

#endif /* not REDUCED */

#ifndef REDUCED

void createeventheap(struct mesh *m, struct event ***eventheap,
                     struct event **events, struct event **freeevents)
{
  vertex thisvertex;
  int maxevents;
  int i;

  maxevents = (3 * m->invertices) / 2;
  *eventheap = (struct event **)
               trimalloc(maxevents * (int) sizeof(struct event *));
  *events = (struct event *) trimalloc(maxevents * (int) sizeof(struct event));
  traversalinit(&m->vertices);
  for (i = 0; i < m->invertices; i++) {
    thisvertex = vertextraverse(m);
    (*events)[i].eventptr = (int *) thisvertex;
    (*events)[i].xkey = thisvertex[0];
    (*events)[i].ykey = thisvertex[1];
    eventheapinsert(*eventheap, i, *events + i);
  }
  *freeevents = (struct event *) NULL;
  for (i = maxevents - 1; i >= m->invertices; i--) {
    (*events)[i].eventptr = (int *) *freeevents;
    *freeevents = *events + i;
  }
}

#endif /* not REDUCED */

#ifndef REDUCED

int rightofhyperbola(struct mesh *m, struct otri *fronttri, vertex newsite)
{
  vertex leftvertex, rightvertex;
  sgFloat dxa, dya, dxb, dyb;

  m->hyperbolacount++;

  dest(*fronttri, leftvertex);
  apex(*fronttri, rightvertex);
  if ((leftvertex[1] < rightvertex[1]) ||
      ((leftvertex[1] == rightvertex[1]) &&
       (leftvertex[0] < rightvertex[0]))) {
    if (newsite[0] >= rightvertex[0]) {
      return 1;
    }
  } else {
    if (newsite[0] <= leftvertex[0]) {
      return 0;
    }
  }
  dxa = leftvertex[0] - newsite[0];
  dya = leftvertex[1] - newsite[1];
  dxb = rightvertex[0] - newsite[0];
  dyb = rightvertex[1] - newsite[1];
  return dya * (dxb * dxb + dyb * dyb) > dyb * (dxa * dxa + dya * dya);
}

#endif /* not REDUCED */

#ifndef REDUCED

sgFloat circletop(struct mesh *m, vertex pa, vertex pb, vertex pc, sgFloat ccwabc)
{
  sgFloat xac, yac, xbc, ybc, xab, yab;
  sgFloat aclen2, bclen2, ablen2;

  m->circletopcount++;

  xac = pa[0] - pc[0];
  yac = pa[1] - pc[1];
  xbc = pb[0] - pc[0];
  ybc = pb[1] - pc[1];
  xab = pa[0] - pb[0];
  yab = pa[1] - pb[1];
  aclen2 = xac * xac + yac * yac;
  bclen2 = xbc * xbc + ybc * ybc;
  ablen2 = xab * xab + yab * yab;
  return pc[1] + (xac * bclen2 - xbc * aclen2 + sqrt(aclen2 * bclen2 * ablen2))
               / (2.0 * ccwabc);
}

#endif /* not REDUCED */

#ifndef REDUCED

void check4deadevent(struct otri *checktri, struct event **freeevents,
                     struct event **eventheap, int *heapsize)
{
  struct event *deadevent;
  vertex eventvertex;
  int eventnum;

  org(*checktri, eventvertex);
  if (eventvertex != (vertex) NULL) {
    deadevent = (struct event *) eventvertex;
    eventnum = deadevent->heapposition;
    deadevent->eventptr = (int *) *freeevents;
    *freeevents = deadevent;
    eventheapdelete(eventheap, *heapsize, eventnum);
    (*heapsize)--;
    setorg(*checktri, NULL);
  }
}

#endif /* not REDUCED */

#ifndef REDUCED

struct splaynode *splay(struct mesh *m, struct splaynode *splaytree,
                        vertex searchpoint, struct otri *searchtri)
{
  struct splaynode *child, *grandchild;
  struct splaynode *lefttree, *righttree;
  struct splaynode *leftright;
  vertex checkvertex;
  int rightofroot, rightofchild;

  if (splaytree == (struct splaynode *) NULL) {
    return (struct splaynode *) NULL;
  }
  dest(splaytree->keyedge, checkvertex);
  if (checkvertex == splaytree->keydest) {
    rightofroot = rightofhyperbola(m, &splaytree->keyedge, searchpoint);
    if (rightofroot) {
      otricopy(splaytree->keyedge, *searchtri);
      child = splaytree->rchild;
    } else {
      child = splaytree->lchild;
    }
    if (child == (struct splaynode *) NULL) {
      return splaytree;
    }
    dest(child->keyedge, checkvertex);
    if (checkvertex != child->keydest) {
      child = splay(m, child, searchpoint, searchtri);
      if (child == (struct splaynode *) NULL) {
        if (rightofroot) {
          splaytree->rchild = (struct splaynode *) NULL;
        } else {
          splaytree->lchild = (struct splaynode *) NULL;
        }
        return splaytree;
      }
    }
    rightofchild = rightofhyperbola(m, &child->keyedge, searchpoint);
    if (rightofchild) {
      otricopy(child->keyedge, *searchtri);
      grandchild = splay(m, child->rchild, searchpoint, searchtri);
      child->rchild = grandchild;
    } else {
      grandchild = splay(m, child->lchild, searchpoint, searchtri);
      child->lchild = grandchild;
    }
    if (grandchild == (struct splaynode *) NULL) {
      if (rightofroot) {
        splaytree->rchild = child->lchild;
        child->lchild = splaytree;
      } else {
        splaytree->lchild = child->rchild;
        child->rchild = splaytree;
      }
      return child;
    }
    if (rightofchild) {
      if (rightofroot) {
        splaytree->rchild = child->lchild;
        child->lchild = splaytree;
      } else {
        splaytree->lchild = grandchild->rchild;
        grandchild->rchild = splaytree;
      }
      child->rchild = grandchild->lchild;
      grandchild->lchild = child;
    } else {
      if (rightofroot) {
        splaytree->rchild = grandchild->lchild;
        grandchild->lchild = splaytree;
      } else {
        splaytree->lchild = child->rchild;
        child->rchild = splaytree;
      }
      child->lchild = grandchild->rchild;
      grandchild->rchild = child;
    }
    return grandchild;
  } else {
    lefttree = splay(m, splaytree->lchild, searchpoint, searchtri);
    righttree = splay(m, splaytree->rchild, searchpoint, searchtri);

    pooldealloc(&m->splaynodes, (int *) splaytree);
    if (lefttree == (struct splaynode *) NULL) {
      return righttree;
    } else if (righttree == (struct splaynode *) NULL) {
      return lefttree;
    } else if (lefttree->rchild == (struct splaynode *) NULL) {
      lefttree->rchild = righttree->lchild;
      righttree->lchild = lefttree;
      return righttree;
    } else if (righttree->lchild == (struct splaynode *) NULL) {
      righttree->lchild = lefttree->rchild;
      lefttree->rchild = righttree;
      return lefttree;
    } else {
/*      //  TRACE("Holy Toledo!!!\n"); */
      leftright = lefttree->rchild;
      while (leftright->rchild != (struct splaynode *) NULL) {
        leftright = leftright->rchild;
      }
      leftright->rchild = righttree;
      return lefttree;
    }
  }
}

#endif /* not REDUCED */

#ifndef REDUCED

struct splaynode *splayinsert(struct mesh *m, struct splaynode *splayroot,
                              struct otri *newkey, vertex searchpoint)
{
  struct splaynode *newsplaynode;

  newsplaynode = (struct splaynode *) poolalloc(&m->splaynodes);
  otricopy(*newkey, newsplaynode->keyedge);
  dest(*newkey, newsplaynode->keydest);
  if (splayroot == (struct splaynode *) NULL) {
    newsplaynode->lchild = (struct splaynode *) NULL;
    newsplaynode->rchild = (struct splaynode *) NULL;
  } else if (rightofhyperbola(m, &splayroot->keyedge, searchpoint)) {
    newsplaynode->lchild = splayroot;
    newsplaynode->rchild = splayroot->rchild;
    splayroot->rchild = (struct splaynode *) NULL;
  } else {
    newsplaynode->lchild = splayroot->lchild;
    newsplaynode->rchild = splayroot;
    splayroot->lchild = (struct splaynode *) NULL;
  }
  return newsplaynode;
}

#endif /* not REDUCED */

#ifndef REDUCED

struct splaynode *circletopinsert(struct mesh *m, struct behavior *b,
                                  struct splaynode *splayroot,
                                  struct otri *newkey,
                                  vertex pa, vertex pb, vertex pc, sgFloat topy)
{
  sgFloat ccwabc;
  sgFloat xac, yac, xbc, ybc;
  sgFloat aclen2, bclen2;
  sgFloat searchpoint[2];
  struct otri dummytri;

  ccwabc = counterclockwise(m, b, pa, pb, pc);
  xac = pa[0] - pc[0];
  yac = pa[1] - pc[1];
  xbc = pb[0] - pc[0];
  ybc = pb[1] - pc[1];
  aclen2 = xac * xac + yac * yac;
  bclen2 = xbc * xbc + ybc * ybc;
  searchpoint[0] = pc[0] - (yac * bclen2 - ybc * aclen2) / (2.0 * ccwabc);
  searchpoint[1] = topy;
  return splayinsert(m, splay(m, splayroot, (vertex) searchpoint, &dummytri),
                     newkey, (vertex) searchpoint);
}

#endif /* not REDUCED */

#ifndef REDUCED

struct splaynode *frontlocate(struct mesh *m, struct splaynode *splayroot,
                              struct otri *bottommost, vertex searchvertex,
                              struct otri *searchtri, int *farright)
{
  int farrightflag;
  triangle ptr;                       /* Temporary variable used by onext(). */

  otricopy(*bottommost, *searchtri);
  splayroot = splay(m, splayroot, searchvertex, searchtri);

  farrightflag = 0;
  while (!farrightflag && rightofhyperbola(m, searchtri, searchvertex)) {
    onextself(*searchtri);
    farrightflag = otriequal(*searchtri, *bottommost);
  }
  *farright = farrightflag;
  return splayroot;
}

#endif /* not REDUCED */

#ifndef REDUCED

long sweeplinedelaunay(struct mesh *m, struct behavior *b)
{
  struct event **eventheap;
  struct event *events;
  struct event *freeevents;
  struct event *nextevent;
  struct event *newevent;
  struct splaynode *splayroot;
  struct otri bottommost;
  struct otri searchtri;
  struct otri fliptri;
  struct otri lefttri, righttri, farlefttri, farrighttri;
  struct otri inserttri;
  vertex firstvertex, secondvertex;
  vertex nextvertex, lastvertex;
  vertex connectvertex;
  vertex leftvertex, midvertex, rightvertex;
  sgFloat lefttest, righttest;
  int heapsize;
  int check4events, farrightflag;
  triangle ptr;   /* Temporary variable used by sym(), onext(), and oprev(). */

  poolinit(&m->splaynodes, sizeof(struct splaynode), SPLAYNODEPERBLOCK,
           SPLAYNODEPERBLOCK, POINTER, 0);
  splayroot = (struct splaynode *) NULL;

  if (b->verbose) {
    //  TRACE("  Placing vertices in event heap.\n");
  }
  createeventheap(m, &eventheap, &events, &freeevents);
  heapsize = m->invertices;

  if (b->verbose) {
    //  TRACE("  Forming triangulation.\n");
  }
  maketriangle(m, b, &lefttri);
  maketriangle(m, b, &righttri);
  bond(lefttri, righttri);
  lnextself(lefttri);
  lprevself(righttri);
  bond(lefttri, righttri);
  lnextself(lefttri);
  lprevself(righttri);
  bond(lefttri, righttri);
  firstvertex = (vertex) eventheap[0]->eventptr;
  eventheap[0]->eventptr = (int *) freeevents;
  freeevents = eventheap[0];
  eventheapdelete(eventheap, heapsize, 0);
  heapsize--;
  do {
    if (heapsize == 0) {
      //  TRACE("Error:  Input vertices are all identical.\n");
      exit(1);
    }
    secondvertex = (vertex) eventheap[0]->eventptr;
    eventheap[0]->eventptr = (int *) freeevents;
    freeevents = eventheap[0];
    eventheapdelete(eventheap, heapsize, 0);
    heapsize--;
    if ((firstvertex[0] == secondvertex[0]) &&
        (firstvertex[1] == secondvertex[1])) {
      if (!b->quiet) {
        //  TRACE(
//"Warning:  A duplicate vertex at (%.12g, %.12g) appeared and was ignored.\n",
  //             secondvertex[0], secondvertex[1]);
      }
      setvertextype(secondvertex, UNDEADVERTEX);
      m->undeads++;
    }
  } while ((firstvertex[0] == secondvertex[0]) &&
           (firstvertex[1] == secondvertex[1]));
  setorg(lefttri, firstvertex);
  setdest(lefttri, secondvertex);
  setorg(righttri, secondvertex);
  setdest(righttri, firstvertex);
  lprev(lefttri, bottommost);
  lastvertex = secondvertex;
  while (heapsize > 0) {
    nextevent = eventheap[0];
    eventheapdelete(eventheap, heapsize, 0);
    heapsize--;
    check4events = 1;
    if (nextevent->xkey < m->xmin) {
      decode(nextevent->eventptr, fliptri);
      oprev(fliptri, farlefttri);
      check4deadevent(&farlefttri, &freeevents, eventheap, &heapsize);
      onext(fliptri, farrighttri);
      check4deadevent(&farrighttri, &freeevents, eventheap, &heapsize);

      if (otriequal(farlefttri, bottommost)) {
        lprev(fliptri, bottommost);
      }
      flip(m, b, &fliptri);
      setapex(fliptri, NULL);
      lprev(fliptri, lefttri);
      lnext(fliptri, righttri);
      sym(lefttri, farlefttri);

      if (randomnation(SAMPLERATE) == 0) {
        symself(fliptri);
        dest(fliptri, leftvertex);
        apex(fliptri, midvertex);
        org(fliptri, rightvertex);
        splayroot = circletopinsert(m, b, splayroot, &lefttri, leftvertex,
                                    midvertex, rightvertex, nextevent->ykey);
      }
    } else {
      nextvertex = (vertex) nextevent->eventptr;
      if ((nextvertex[0] == lastvertex[0]) &&
          (nextvertex[1] == lastvertex[1])) {
        if (!b->quiet) {
          //  TRACE(
//"Warning:  A duplicate vertex at (%.12g, %.12g) appeared and was ignored.\n",
  //               nextvertex[0], nextvertex[1]);
        }
        setvertextype(nextvertex, UNDEADVERTEX);
        m->undeads++;
        check4events = 0;
      } else {
        lastvertex = nextvertex;

        splayroot = frontlocate(m, splayroot, &bottommost, nextvertex,
                                &searchtri, &farrightflag);
/*
        otricopy(bottommost, searchtri);
        farrightflag = 0;
        while (!farrightflag && rightofhyperbola(m, &searchtri, nextvertex)) {
          onextself(searchtri);
          farrightflag = otriequal(searchtri, bottommost);
        }
*/

        check4deadevent(&searchtri, &freeevents, eventheap, &heapsize);

        otricopy(searchtri, farrighttri);
        sym(searchtri, farlefttri);
        maketriangle(m, b, &lefttri);
        maketriangle(m, b, &righttri);
        dest(farrighttri, connectvertex);
        setorg(lefttri, connectvertex);
        setdest(lefttri, nextvertex);
        setorg(righttri, nextvertex);
        setdest(righttri, connectvertex);
        bond(lefttri, righttri);
        lnextself(lefttri);
        lprevself(righttri);
        bond(lefttri, righttri);
        lnextself(lefttri);
        lprevself(righttri);
        bond(lefttri, farlefttri);
        bond(righttri, farrighttri);
        if (!farrightflag && otriequal(farrighttri, bottommost)) {
          otricopy(lefttri, bottommost);
        }

        if (randomnation(SAMPLERATE) == 0) {
          splayroot = splayinsert(m, splayroot, &lefttri, nextvertex);
        } else if (randomnation(SAMPLERATE) == 0) {
          lnext(righttri, inserttri);
          splayroot = splayinsert(m, splayroot, &inserttri, nextvertex);
        }
      }
    }
    nextevent->eventptr = (int *) freeevents;
    freeevents = nextevent;

    if (check4events) {
      apex(farlefttri, leftvertex);
      dest(lefttri, midvertex);
      apex(lefttri, rightvertex);
      lefttest = counterclockwise(m, b, leftvertex, midvertex, rightvertex);
      if (lefttest > 0.0) {
        newevent = freeevents;
        freeevents = (struct event *) freeevents->eventptr;
        newevent->xkey = m->xminextreme;
        newevent->ykey = circletop(m, leftvertex, midvertex, rightvertex,
                                   lefttest);
        newevent->eventptr = (int *) encode(lefttri);
        eventheapinsert(eventheap, heapsize, newevent);
        heapsize++;
        setorg(lefttri, newevent);
      }
      apex(righttri, leftvertex);
      org(righttri, midvertex);
      apex(farrighttri, rightvertex);
      righttest = counterclockwise(m, b, leftvertex, midvertex, rightvertex);
      if (righttest > 0.0) {
        newevent = freeevents;
        freeevents = (struct event *) freeevents->eventptr;
        newevent->xkey = m->xminextreme;
        newevent->ykey = circletop(m, leftvertex, midvertex, rightvertex,
                                   righttest);
        newevent->eventptr = (int *) encode(farrighttri);
        eventheapinsert(eventheap, heapsize, newevent);
        heapsize++;
        setorg(farrighttri, newevent);
      }
    }
  }

  pooldeinit(&m->splaynodes);
  lprevself(bottommost);
  return removeghosts(m, b, &bottommost);
}

#endif /* not REDUCED */

long delaunay(struct mesh *m, struct behavior *b)
{
  long hulledges;

  m->eextras = 0;
  initializetrisubpools(m, b);

#ifdef REDUCED
  if (!b->quiet) {
    //  TRACE(
      "Constructing Delaunay triangulation by divide-and-conquer method.\n");
  }
  hulledges = divconqdelaunay(m, b);
#else /* not REDUCED */
  if (!b->quiet) {
    //  TRACE("Constructing Delaunay triangulation ");
    if (b->incremental) {
      //  TRACE("by incremental method.\n");
    } else if (b->sweepline) {
      //  TRACE("by sweepline method.\n");
    } else {
      //  TRACE("by divide-and-conquer method.\n");
    }
  }
  if (b->incremental) {
    hulledges = incrementaldelaunay(m, b);
  } else if (b->sweepline) {
    hulledges = sweeplinedelaunay(m, b);
  } else {
    hulledges = divconqdelaunay(m, b);
  }
#endif /* not REDUCED */

  if (m->triangles.items == 0) {
    /* The input vertices were all collinear, so there are no triangles. */
    return 0l;
  } else {
    return hulledges;
  }
}

#ifndef CDT_ONLY

#ifdef TRILIBRARY
int reconstruct(struct mesh *m, struct behavior *b, int *trianglelist,
                sgFloat *triangleattriblist, sgFloat *trianglearealist,
                int elements, int corners, int attribs,
                int *segmentlist,int *segmentmarkerlist, int numberofsegments)

#else /* not TRILIBRARY */

long reconstruct(struct mesh *m, struct behavior *b, char *elefilename,
                 char *areafilename, char *polyfilename, FILE *polyfile)

#endif /* not TRILIBRARY */

{
#ifdef TRILIBRARY
  int vertexindex;
  int attribindex;
#else /* not TRILIBRARY */
  FILE *elefile;
  FILE *areafile;
  char inputline[INPUTLINESIZE];
  char *stringptr;
  int areaelements;
#endif /* not TRILIBRARY */
  struct otri triangleloop;
  struct otri triangleleft;
  struct otri checktri;
  struct otri checkleft;
  struct otri checkneighbor;
  struct osub subsegloop;
  triangle *vertexarray;
  triangle *prevlink;
  triangle nexttri;
  vertex tdest, tapex;
  vertex checkdest, checkapex;
  vertex shorg;
  vertex killvertex;
  sgFloat area;
  int corner[3];
  int end[2];
  int killvertexindex;
  int incorners;
  int segmentmarkers;
  int boundmarker;
  int aroundvertex;
  long hullsize;
  int notfound;
  long elementnumber, segmentnumber;
  int i, j;
  triangle ptr;                         /* Temporary variable used by sym(). */

  m->inelements = elements;
  incorners = corners;
  if (incorners < 3) {
    //  TRACE("Error:  Triangles must have at least 3 vertices.\n");
    exit(1);
  }
  m->eextras = attribs;

  initializetrisubpools(m, b);

  /* Create the triangles. */
  for (elementnumber = 1; elementnumber <= m->inelements; elementnumber++) {
    maketriangle(m, b, &triangleloop);
    /* Mark the triangle as living. */
    triangleloop.tri[3] = (triangle) triangleloop.tri;
  }

  if (b->poly) {

    m->insegments = numberofsegments;
    segmentmarkers = segmentmarkerlist != (int *) NULL;

    /* Create the subsegments. */
    for (segmentnumber = 1; segmentnumber <= m->insegments; segmentnumber++) {
      makesubseg(m, &subsegloop);
      /* Mark the subsegment as living. */
      subsegloop.ss[2] = (subseg) subsegloop.ss;
    }
  }

  vertexindex = 0;
  attribindex = 0;

  if (!b->quiet) {
    //  TRACE("Reconstructing mesh.\n");
  }
  /* Allocate a temporary array that maps each vertex to some adjacent */
  /*   triangle.  I took care to allocate all the permanent memory for */
  /*   triangles and subsegments first.                                */
  vertexarray = (triangle *)
                trimalloc(m->vertices.items * (int) sizeof(triangle));
  /* Each vertex is initially unrepresented. */
  for (i = 0; i < m->vertices.items; i++) {
    vertexarray[i] = (triangle) m->dummytri;
  }

  if (b->verbose) {
    //  TRACE("  Assembling triangles.\n");
  }
  /* Read the triangles from the .ele file, and link */
  /*   together those that share an edge.            */
  traversalinit(&m->triangles);
  triangleloop.tri = triangletraverse(m);
  elementnumber = b->firstnumber;
  while (triangleloop.tri != (triangle *) NULL) {

    /* Copy the triangle's three corners. */
    for (j = 0; j < 3; j++) {
      corner[j] = trianglelist[vertexindex++];
      if ((corner[j] < b->firstnumber) ||
          (corner[j] >= b->firstnumber + m->invertices)) {
        //  TRACE("Error:  Triangle %ld has an invalid vertex index.\n",
//               elementnumber);
        exit(1);
      }
    }

    /* Find out about (and throw away) extra nodes. */
    for (j = 3; j < incorners; j++) {

      killvertexindex = trianglelist[vertexindex++];

        if ((killvertexindex >= b->firstnumber) &&
            (killvertexindex < b->firstnumber + m->invertices)) {
          /* Delete the non-corner vertex if it's not already deleted. */
          killvertex = getvertex(m, b, killvertexindex);
          if (vertextype(killvertex) != DEADVERTEX) {
            vertexdealloc(m, killvertex);
          }
        }

    }

    /* Read the triangle's attributes. */
    for (j = 0; j < m->eextras; j++) {

      setelemattribute(triangleloop, j, triangleattriblist[attribindex++]);

    }

    if (b->vararea) {

      area = trianglearealist[elementnumber - b->firstnumber];

      setareabound(triangleloop, area);
    }

    /* Set the triangle's vertices. */
    triangleloop.orient = 0;
    setorg(triangleloop, getvertex(m, b, corner[0]));
    setdest(triangleloop, getvertex(m, b, corner[1]));
    setapex(triangleloop, getvertex(m, b, corner[2]));
    /* Try linking the triangle to others that share these vertices. */
    for (triangleloop.orient = 0; triangleloop.orient < 3;
         triangleloop.orient++) {
      /* Take the number for the origin of triangleloop. */
      aroundvertex = corner[triangleloop.orient];
      /* Look for other triangles having this vertex. */
      nexttri = vertexarray[aroundvertex - b->firstnumber];
      /* Link the current triangle to the next one in the stack. */
      triangleloop.tri[6 + triangleloop.orient] = nexttri;
      /* Push the current triangle onto the stack. */
      vertexarray[aroundvertex - b->firstnumber] = encode(triangleloop);
      decode(nexttri, checktri);
      if (checktri.tri != m->dummytri) {
        dest(triangleloop, tdest);
        apex(triangleloop, tapex);
        /* Look for other triangles that share an edge. */
        do {
          dest(checktri, checkdest);
          apex(checktri, checkapex);
          if (tapex == checkdest) {
            /* The two triangles share an edge; bond them together. */
            lprev(triangleloop, triangleleft);
            bond(triangleleft, checktri);
          }
          if (tdest == checkapex) {
            /* The two triangles share an edge; bond them together. */
            lprev(checktri, checkleft);
            bond(triangleloop, checkleft);
          }
          /* Find the next triangle in the stack. */
          nexttri = checktri.tri[6 + checktri.orient];
          decode(nexttri, checktri);
        } while (checktri.tri != m->dummytri);
      }
    }
    triangleloop.tri = triangletraverse(m);
    elementnumber++;
  }


  vertexindex = 0;


  hullsize = 0;                      /* Prepare to count the boundary edges. */
  if (b->poly) {
    if (b->verbose) {
      //  TRACE("  Marking segments in triangulation.\n");
    }
    /* Read the segments from the .poly file, and link them */
    /*   to their neighboring triangles.                    */
    boundmarker = 0;
    traversalinit(&m->subsegs);
    subsegloop.ss = subsegtraverse(m);
    segmentnumber = b->firstnumber;
    while (subsegloop.ss != (subseg *) NULL) {

      end[0] = segmentlist[vertexindex++];
      end[1] = segmentlist[vertexindex++];
      if (segmentmarkers) {
        boundmarker = segmentmarkerlist[segmentnumber - b->firstnumber];
      }

      for (j = 0; j < 2; j++) {
        if ((end[j] < b->firstnumber) ||
            (end[j] >= b->firstnumber + m->invertices)) {
          //  TRACE("Error:  Segment %ld has an invalid vertex index.\n", 
//                 segmentnumber);
          exit(1);
        }
      }

      /* set the subsegment's vertices. */
      subsegloop.ssorient = 0;
      setsorg(subsegloop, getvertex(m, b, end[0]));
      setsdest(subsegloop, getvertex(m, b, end[1]));
      setmark(subsegloop, boundmarker);
      /* Try linking the subsegment to triangles that share these vertices. */
      for (subsegloop.ssorient = 0; subsegloop.ssorient < 2;
           subsegloop.ssorient++) {
        /* Take the number for the destination of subsegloop. */
        aroundvertex = end[1 - subsegloop.ssorient];
        /* Look for triangles having this vertex. */
        prevlink = &vertexarray[aroundvertex - b->firstnumber];
        nexttri = vertexarray[aroundvertex - b->firstnumber];
        decode(nexttri, checktri);
        sorg(subsegloop, shorg);
        notfound = 1;
        /* Look for triangles having this edge.  Note that I'm only       */
        /*   comparing each triangle's destination with the subsegment;   */
        /*   each triangle's apex is handled through a different vertex.  */
        /*   Because each triangle appears on three vertices' lists, each */
        /*   occurrence of a triangle on a list can (and does) represent  */
        /*   an edge.  In this way, most edges are represented twice, and */
        /*   every triangle-subsegment bond is represented once.          */
        while (notfound && (checktri.tri != m->dummytri)) {
          dest(checktri, checkdest);
          if (shorg == checkdest) {
            /* We have a match.  Remove this triangle from the list. */
            *prevlink = checktri.tri[6 + checktri.orient];
            /* Bond the subsegment to the triangle. */
            tsbond(checktri, subsegloop);
            /* Check if this is a boundary edge. */
            sym(checktri, checkneighbor);
            if (checkneighbor.tri == m->dummytri) {
              /* The next line doesn't insert a subsegment (because there's */
              /*   already one there), but it sets the boundary markers of  */
              /*   the existing subsegment and its vertices.                */
              insertsubseg(m, b, &checktri, 1);
              hullsize++;
            }
            notfound = 0;
          }
          /* Find the next triangle in the stack. */
          prevlink = &checktri.tri[6 + checktri.orient];
          nexttri = checktri.tri[6 + checktri.orient];
          decode(nexttri, checktri);
        }
      }
      subsegloop.ss = subsegtraverse(m);
      segmentnumber++;
    }
  }

  /* Mark the remaining edges as not being attached to any subsegment. */
  /* Also, count the (yet uncounted) boundary edges.                   */
  for (i = 0; i < m->vertices.items; i++) {
    /* Search the stack of triangles adjacent to a vertex. */
    nexttri = vertexarray[i];
    decode(nexttri, checktri);
    while (checktri.tri != m->dummytri) {
      /* Find the next triangle in the stack before this */
      /*   information gets overwritten.                 */
      nexttri = checktri.tri[6 + checktri.orient];
      /* No adjacent subsegment.  (This overwrites the stack info.) */
      tsdissolve(checktri);
      sym(checktri, checkneighbor);
      if (checkneighbor.tri == m->dummytri) {
        insertsubseg(m, b, &checktri, 1);
        hullsize++;
      }
      decode(nexttri, checktri);
    }
  }

  trifree((int *) vertexarray);
  return hullsize;
}

#endif /* not CDT_ONLY */


enum finddirectionresult finddirection(struct mesh *m, struct behavior *b,
                                       struct otri *searchtri,
                                       vertex searchpoint)
{
  struct otri checktri;
  vertex startvertex;
  vertex leftvertex, rightvertex;
  sgFloat leftccw, rightccw;
  int leftflag, rightflag;
  triangle ptr;           /* Temporary variable used by onext() and oprev(). */

  org(*searchtri, startvertex);
  dest(*searchtri, rightvertex);
  apex(*searchtri, leftvertex);
  /* Is `searchpoint' to the left? */
  leftccw = counterclockwise(m, b, searchpoint, startvertex, leftvertex);
  leftflag = leftccw > 0.0;
  /* Is `searchpoint' to the right? */
  rightccw = counterclockwise(m, b, startvertex, searchpoint, rightvertex);
  rightflag = rightccw > 0.0;
  if (leftflag && rightflag) {
    /* `searchtri' faces directly away from `searchpoint'.  We could go left */
    /*   or right.  Ask whether it's a triangle or a boundary on the left.   */
    onext(*searchtri, checktri);
    if (checktri.tri == m->dummytri) {
      leftflag = 0;
    } else {
      rightflag = 0;
    }
  }
  while (leftflag) {
    /* Turn left until satisfied. */
    onextself(*searchtri);
    if (searchtri->tri == m->dummytri) {
      //  TRACE("Internal error in finddirection():  Unable to find a\n");
      //  TRACE("  triangle leading from (%.12g, %.12g) to", startvertex[0],
//             startvertex[1]);
      //  TRACE("  (%.12g, %.12g).\n", searchpoint[0], searchpoint[1]);
      internalerror();
    }
    apex(*searchtri, leftvertex);
    rightccw = leftccw;
    leftccw = counterclockwise(m, b, searchpoint, startvertex, leftvertex);
    leftflag = leftccw > 0.0;
  }
  while (rightflag) {
    /* Turn right until satisfied. */
    oprevself(*searchtri);
    if (searchtri->tri == m->dummytri) {
      //  TRACE("Internal error in finddirection():  Unable to find a\n");
      //  TRACE("  triangle leading from (%.12g, %.12g) to", startvertex[0],
//             startvertex[1]);
      //  TRACE("  (%.12g, %.12g).\n", searchpoint[0], searchpoint[1]);
      internalerror();
    }
    dest(*searchtri, rightvertex);
    leftccw = rightccw;
    rightccw = counterclockwise(m, b, startvertex, searchpoint, rightvertex);
    rightflag = rightccw > 0.0;
  }
  if (leftccw == 0.0) {
    return LEFTCOLLINEAR;
  } else if (rightccw == 0.0) {
    return RIGHTCOLLINEAR;
  } else {
    return WITHIN;
  }
}

void segmentintersection(struct mesh *m, struct behavior *b,
                         struct otri *splittri, struct osub *splitsubseg,
                         vertex endpoint2)
{
  vertex endpoint1;
  vertex torg, tdest;
  vertex leftvertex, rightvertex;
  vertex newvertex;
  enum insertvertexresult success;
  enum finddirectionresult collinear;
  sgFloat ex, ey;
  sgFloat tx, ty;
  sgFloat etx, ety;
  sgFloat split, denom;
  int i;
  triangle ptr;                       /* Temporary variable used by onext(). */

  /* Find the other three segment endpoints. */
  apex(*splittri, endpoint1);
  org(*splittri, torg);
  dest(*splittri, tdest);
  /* Segment intersection formulae; see the Antonio reference. */
  tx = tdest[0] - torg[0];
  ty = tdest[1] - torg[1];
  ex = endpoint2[0] - endpoint1[0];
  ey = endpoint2[1] - endpoint1[1];
  etx = torg[0] - endpoint2[0];
  ety = torg[1] - endpoint2[1];
  denom = ty * ex - tx * ey;
  if (denom == 0.0) {
    //  TRACE("Internal error in segmentintersection():");
    //  TRACE("  Attempt to find intersection of parallel segments.\n");
    internalerror();
  }
  split = (ey * etx - ex * ety) / denom;
  /* Create the new vertex. */
  newvertex = (vertex) poolalloc(&m->vertices);
  /* Interpolate its coordinate and attributes. */
  for (i = 0; i < 2 + m->nextras; i++) {
    newvertex[i] = torg[i] + split * (tdest[i] - torg[i]);
  }
  setvertexmark(newvertex, mark(*splitsubseg));
  setvertextype(newvertex, INPUTVERTEX);
  if (b->verbose > 1) {
    //  TRACE(
 // "  Splitting subsegment (%.12g, %.12g) (%.12g, %.12g) at (%.12g, %.12g).\n",
  //         torg[0], torg[1], tdest[0], tdest[1], newvertex[0], newvertex[1]);
  }
  /* Insert the intersection vertex.  This should always succeed. */
  success = insertvertex(m, b, newvertex, splittri, splitsubseg, 0, 0, 0.0);
  if (success != SUCCESSFULVERTEX) {
    //  TRACE("Internal error in segmentintersection():\n");
    //  TRACE("  Failure to split a segment.\n");
    internalerror();
  }
  if (m->steinerleft > 0) {
    m->steinerleft--;
  }
  /* Inserting the vertex may have caused edge flips.  We wish to rediscover */
  /*   the edge connecting endpoint1 to the new intersection vertex.         */
  collinear = finddirection(m, b, splittri, endpoint1);
  dest(*splittri, rightvertex);
  apex(*splittri, leftvertex);
  if ((leftvertex[0] == endpoint1[0]) && (leftvertex[1] == endpoint1[1])) {
    onextself(*splittri);
  } else if ((rightvertex[0] != endpoint1[0]) ||
             (rightvertex[1] != endpoint1[1])) {
    //  TRACE("Internal error in segmentintersection():\n");
    //  TRACE("  Topological inconsistency after splitting a segment.\n");
    internalerror();
  }
  /* `splittri' should have destination endpoint1. */
}

int scoutsegment(struct mesh *m, struct behavior *b, struct otri *searchtri,
                 vertex endpoint2, int newmark)
{
  struct otri crosstri;
  struct osub crosssubseg;
  vertex leftvertex, rightvertex;
  enum finddirectionresult collinear;
  subseg sptr;                      /* Temporary variable used by tspivot(). */

  collinear = finddirection(m, b, searchtri, endpoint2);
  dest(*searchtri, rightvertex);
  apex(*searchtri, leftvertex);
  if (((leftvertex[0] == endpoint2[0]) && (leftvertex[1] == endpoint2[1])) ||
      ((rightvertex[0] == endpoint2[0]) && (rightvertex[1] == endpoint2[1]))) {
    /* The segment is already an edge in the mesh. */
    if ((leftvertex[0] == endpoint2[0]) && (leftvertex[1] == endpoint2[1])) {
      lprevself(*searchtri);
    }
    /* Insert a subsegment, if there isn't already one there. */
    insertsubseg(m, b, searchtri, newmark);
    return 1;
  } else if (collinear == LEFTCOLLINEAR) {
    /* We've collided with a vertex between the segment's endpoints. */
    /* Make the collinear vertex be the triangle's origin. */
    lprevself(*searchtri);
    insertsubseg(m, b, searchtri, newmark);
    /* Insert the remainder of the segment. */
    return scoutsegment(m, b, searchtri, endpoint2, newmark);
  } else if (collinear == RIGHTCOLLINEAR) {
    /* We've collided with a vertex between the segment's endpoints. */
    insertsubseg(m, b, searchtri, newmark);
    /* Make the collinear vertex be the triangle's origin. */
    lnextself(*searchtri);
    /* Insert the remainder of the segment. */
    return scoutsegment(m, b, searchtri, endpoint2, newmark);
  } else {
    lnext(*searchtri, crosstri);
    tspivot(crosstri, crosssubseg);
    /* Check for a crossing segment. */
    if (crosssubseg.ss == m->dummysub) {
      return 0;
    } else {
      /* Insert a vertex at the intersection. */
      segmentintersection(m, b, &crosstri, &crosssubseg, endpoint2);
      otricopy(crosstri, *searchtri);
      insertsubseg(m, b, searchtri, newmark);
      /* Insert the remainder of the segment. */
      return scoutsegment(m, b, searchtri, endpoint2, newmark);
    }
  }
}

#ifndef REDUCED
#ifndef CDT_ONLY

void conformingedge(struct mesh *m, struct behavior *b,
                    vertex endpoint1, vertex endpoint2, int newmark)
{
  struct otri searchtri1, searchtri2;
  struct osub brokensubseg;
  vertex newvertex;
  vertex midvertex1, midvertex2;
  enum insertvertexresult success;
  int i;
  subseg sptr;                      /* Temporary variable used by tspivot(). */

  if (b->verbose > 2) {
    //  TRACE("Forcing segment into triangulation by recursive splitting:\n");
    //  TRACE("  (%.12g, %.12g) (%.12g, %.12g)\n", endpoint1[0], endpoint1[1],
//           endpoint2[0], endpoint2[1]);
  }
  /* Create a new vertex to insert in the middle of the segment. */
  newvertex = (vertex) poolalloc(&m->vertices);
  /* Interpolate coordinates and attributes. */
  for (i = 0; i < 2 + m->nextras; i++) {
    newvertex[i] = 0.5 * (endpoint1[i] + endpoint2[i]);
  }
  setvertexmark(newvertex, newmark);
  setvertextype(newvertex, SEGMENTVERTEX);
  /* No known triangle to search from. */
  searchtri1.tri = m->dummytri;
  /* Attempt to insert the new vertex. */
  success = insertvertex(m, b, newvertex, &searchtri1, (struct osub *) NULL,
                         0, 0, 0.0);
  if (success == DUPLICATEVERTEX) {
    if (b->verbose > 2) {
      //  TRACE("  Segment intersects existing vertex (%.12g, %.12g).\n",
//             newvertex[0], newvertex[1]);
    }
    /* Use the vertex that's already there. */
    vertexdealloc(m, newvertex);
    org(searchtri1, newvertex);
  } else {
    if (success == VIOLATINGVERTEX) {
      if (b->verbose > 2) {
        //  TRACE("  Two segments intersect at (%.12g, %.12g).\n",
//               newvertex[0], newvertex[1]);
      }
      /* By fluke, we've landed right on another segment.  Split it. */
      tspivot(searchtri1, brokensubseg);
      success = insertvertex(m, b, newvertex, &searchtri1, &brokensubseg,
                             0, 0, 0.0);
      if (success != SUCCESSFULVERTEX) {
        //  TRACE("Internal error in conformingedge():\n");
        //  TRACE("  Failure to split a segment.\n");
        internalerror();
      }
    }
    /* The vertex has been inserted successfully. */
    if (m->steinerleft > 0) {
      m->steinerleft--;
    }
  }
  otricopy(searchtri1, searchtri2);
  /* `searchtri1' and `searchtri2' are fastened at their origins to         */
  /*   `newvertex', and will be directed toward `endpoint1' and `endpoint2' */
  /*   respectively.  First, we must get `searchtri2' out of the way so it  */
  /*   won't be invalidated during the insertion of the first half of the   */
  /*   segment.                                                             */
  finddirection(m, b, &searchtri2, endpoint2);
  if (!scoutsegment(m, b, &searchtri1, endpoint1, newmark)) {
    /* The origin of searchtri1 may have changed if a collision with an */
    /*   intervening vertex on the segment occurred.                    */
    org(searchtri1, midvertex1);
    conformingedge(m, b, midvertex1, endpoint1, newmark);
  }
  if (!scoutsegment(m, b, &searchtri2, endpoint2, newmark)) {
    /* The origin of searchtri2 may have changed if a collision with an */
    /*   intervening vertex on the segment occurred.                    */
    org(searchtri2, midvertex2);
    conformingedge(m, b, midvertex2, endpoint2, newmark);
  }
}

#endif /* not CDT_ONLY */
#endif /* not REDUCED */

void delaunayfixup(struct mesh *m, struct behavior *b,
                   struct otri *fixuptri, int leftside)
{
  struct otri neartri;
  struct otri fartri;
  struct osub faredge;
  vertex nearvertex, leftvertex, rightvertex, farvertex;
  triangle ptr;                         /* Temporary variable used by sym(). */
  subseg sptr;                      /* Temporary variable used by tspivot(). */

  lnext(*fixuptri, neartri);
  sym(neartri, fartri);
  /* Check if the edge opposite the origin of fixuptri can be flipped. */
  if (fartri.tri == m->dummytri) {
    return;
  }
  tspivot(neartri, faredge);
  if (faredge.ss != m->dummysub) {
    return;
  }
  /* Find all the relevant vertices. */
  apex(neartri, nearvertex);
  org(neartri, leftvertex);
  dest(neartri, rightvertex);
  apex(fartri, farvertex);
  /* Check whether the previous polygon vertex is a reflex vertex. */
  if (leftside) {
    if (counterclockwise(m, b, nearvertex, leftvertex, farvertex) <= 0.0) {
      /* leftvertex is a reflex vertex too.  Nothing can */
      /*   be done until a convex section is found.      */
      return;
    }
  } else {
    if (counterclockwise(m, b, farvertex, rightvertex, nearvertex) <= 0.0) {
      /* rightvertex is a reflex vertex too.  Nothing can */
      /*   be done until a convex section is found.       */
      return;
    }
  }
  if (counterclockwise(m, b, rightvertex, leftvertex, farvertex) > 0.0) {
    /* fartri is not an inverted triangle, and farvertex is not a reflex */
    /*   vertex.  As there are no reflex vertices, fixuptri isn't an     */
    /*   inverted triangle, either.  Hence, test the edge between the    */
    /*   triangles to ensure it is locally Delaunay.                     */
    if (incircle(m, b, leftvertex, farvertex, rightvertex, nearvertex) <=
        0.0) {
      return;
    }
    /* Not locally Delaunay; go on to an edge flip. */
  }        /* else fartri is inverted; remove it from the stack by flipping. */
  flip(m, b, &neartri);
  lprevself(*fixuptri);    /* Restore the origin of fixuptri after the flip. */
  /* Recursively process the two triangles that result from the flip. */
  delaunayfixup(m, b, fixuptri, leftside);
  delaunayfixup(m, b, &fartri, leftside);
}

void constrainededge(struct mesh *m, struct behavior *b,
                     struct otri *starttri, vertex endpoint2, int newmark)
{
  struct otri fixuptri, fixuptri2;
  struct osub crosssubseg;
  vertex endpoint1;
  vertex farvertex;
  sgFloat area;
  int collision;
  int done;
  triangle ptr;             /* Temporary variable used by sym() and oprev(). */
  subseg sptr;                      /* Temporary variable used by tspivot(). */

  org(*starttri, endpoint1);
  lnext(*starttri, fixuptri);
  flip(m, b, &fixuptri);
  /* `collision' indicates whether we have found a vertex directly */
  /*   between endpoint1 and endpoint2.                            */
  collision = 0;
  done = 0;
  do {
    org(fixuptri, farvertex);
    /* `farvertex' is the extreme point of the polygon we are "digging" */
    /*   to get from endpoint1 to endpoint2.                           */
    if ((farvertex[0] == endpoint2[0]) && (farvertex[1] == endpoint2[1])) {
      oprev(fixuptri, fixuptri2);
      /* Enforce the Delaunay condition around endpoint2. */
      delaunayfixup(m, b, &fixuptri, 0);
      delaunayfixup(m, b, &fixuptri2, 1);
      done = 1;
    } else {
      /* Check whether farvertex is to the left or right of the segment */
      /*   being inserted, to decide which edge of fixuptri to dig      */
      /*   through next.                                                */
      area = counterclockwise(m, b, endpoint1, endpoint2, farvertex);
      if (area == 0.0) {
        /* We've collided with a vertex between endpoint1 and endpoint2. */
        collision = 1;
        oprev(fixuptri, fixuptri2);
        /* Enforce the Delaunay condition around farvertex. */
        delaunayfixup(m, b, &fixuptri, 0);
        delaunayfixup(m, b, &fixuptri2, 1);
        done = 1;
      } else {
        if (area > 0.0) {        /* farvertex is to the left of the segment. */
          oprev(fixuptri, fixuptri2);
          /* Enforce the Delaunay condition around farvertex, on the */
          /*   left side of the segment only.                        */
          delaunayfixup(m, b, &fixuptri2, 1);
          /* Flip the edge that crosses the segment.  After the edge is */
          /*   flipped, one of its endpoints is the fan vertex, and the */
          /*   destination of fixuptri is the fan vertex.               */
          lprevself(fixuptri);
        } else {                /* farvertex is to the right of the segment. */
          delaunayfixup(m, b, &fixuptri, 0);
          /* Flip the edge that crosses the segment.  After the edge is */
          /*   flipped, one of its endpoints is the fan vertex, and the */
          /*   destination of fixuptri is the fan vertex.               */
          oprevself(fixuptri);
        }
        /* Check for two intersecting segments. */
        tspivot(fixuptri, crosssubseg);
        if (crosssubseg.ss == m->dummysub) {
          flip(m, b, &fixuptri);    /* May create inverted triangle at left. */
        } else {
          /* We've collided with a segment between endpoint1 and endpoint2. */
          collision = 1;
          /* Insert a vertex at the intersection. */
          segmentintersection(m, b, &fixuptri, &crosssubseg, endpoint2);
          done = 1;
        }
      }
    }
  } while (!done);
  /* Insert a subsegment to make the segment permanent. */
  insertsubseg(m, b, &fixuptri, newmark);
  /* If there was a collision with an interceding vertex, install another */
  /*   segment connecting that vertex with endpoint2.                     */
  if (collision) {
    /* Insert the remainder of the segment. */
    if (!scoutsegment(m, b, &fixuptri, endpoint2, newmark)) {
      constrainededge(m, b, &fixuptri, endpoint2, newmark);
    }
  }
}

static vertex  my_org(struct otri* otr)
{
	return (vertex)(otr->tri[plus1mod3[otr->orient] + 3]);
}

void insertsegment(struct mesh *m, struct behavior *b,
                   vertex endpoint1, vertex endpoint2, int newmark)
{
  struct otri searchtri1, searchtri2;
  triangle encodedtri;
  vertex checkvertex;
  triangle ptr;                         /* Temporary variable used by sym(). */

  if (b->verbose > 1) {
    //  TRACE("  Connecting (%.12g, %.12g) to (%.12g, %.12g).\n",
//           endpoint1[0], endpoint1[1], endpoint2[0], endpoint2[1]);
  }

  /* Find a triangle whose origin is the segment's first endpoint. */
  checkvertex = (vertex) NULL;
  encodedtri = vertex2tri(endpoint1);
  if (encodedtri != (triangle) NULL) {
    decode(encodedtri, searchtri1);
/*
	try
	{
		checkvertex = my_org(&searchtri1);
	}
	catch (...)
	{
		checkvertex = NULL;
	}*/
	org(searchtri1, checkvertex);
  }
  if (checkvertex != endpoint1) {
    /* Find a boundary triangle to search from. */
    searchtri1.tri = m->dummytri;
    searchtri1.orient = 0;
    symself(searchtri1);
    /* Search for the segment's first endpoint by point location. */
    if (locate(m, b, endpoint1, &searchtri1) != ONVERTEX) {
      //  TRACE(
//        "Internal error in insertsegment():  Unable to locate PSLG vertex\n");
      //  TRACE("  (%.12g, %.12g) in triangulation.\n",
          //   endpoint1[0], endpoint1[1]);
      internalerror();
    }
  }
  /* Remember this triangle to improve subsequent point location. */
  otricopy(searchtri1, m->recenttri);
  /* Scout the beginnings of a path from the first endpoint */
  /*   toward the second.                                   */
  if (scoutsegment(m, b, &searchtri1, endpoint2, newmark)) {
    /* The segment was easily inserted. */
    return;
  }
  /* The first endpoint may have changed if a collision with an intervening */
  /*   vertex on the segment occurred.                                      */
  org(searchtri1, endpoint1);

  /* Find a triangle whose origin is the segment's second endpoint. */
  checkvertex = (vertex) NULL;
  encodedtri = vertex2tri(endpoint2);
  if (encodedtri != (triangle) NULL) {
    decode(encodedtri, searchtri2);
    org(searchtri2, checkvertex);
  }
  if (checkvertex != endpoint2) {
    /* Find a boundary triangle to search from. */
    searchtri2.tri = m->dummytri;
    searchtri2.orient = 0;
    symself(searchtri2);
    /* Search for the segment's second endpoint by point location. */
    if (locate(m, b, endpoint2, &searchtri2) != ONVERTEX) {
      //  TRACE(
//        "Internal error in insertsegment():  Unable to locate PSLG vertex\n");
      //  TRACE("  (%.12g, %.12g) in triangulation.\n",
       //      endpoint2[0], endpoint2[1]);
      internalerror();
    }
  }
  /* Remember this triangle to improve subsequent point location. */
  otricopy(searchtri2, m->recenttri);
  /* Scout the beginnings of a path from the second endpoint */
  /*   toward the first.                                     */
  if (scoutsegment(m, b, &searchtri2, endpoint1, newmark)) {
    /* The segment was easily inserted. */
    return;
  }
  /* The second endpoint may have changed if a collision with an intervening */
  /*   vertex on the segment occurred.                                       */
  org(searchtri2, endpoint2);

#ifndef REDUCED
#ifndef CDT_ONLY
  if (b->splitseg) {
    /* Insert vertices to force the segment into the triangulation. */
    conformingedge(m, b, endpoint1, endpoint2, newmark);
  } else {
#endif /* not CDT_ONLY */
#endif /* not REDUCED */
    /* Insert the segment directly into the triangulation. */
    constrainededge(m, b, &searchtri1, endpoint2, newmark);
#ifndef REDUCED
#ifndef CDT_ONLY
  }
#endif /* not CDT_ONLY */
#endif /* not REDUCED */
}

void markhull(struct mesh *m, struct behavior *b)
{
  struct otri hulltri;
  struct otri nexttri;
  struct otri starttri;
  triangle ptr;             /* Temporary variable used by sym() and oprev(). */

  /* Find a triangle handle on the hull. */
  hulltri.tri = m->dummytri;
  hulltri.orient = 0;
  symself(hulltri);
  /* Remember where we started so we know when to stop. */
  otricopy(hulltri, starttri);
  /* Go once counterclockwise around the convex hull. */
  do {
    /* Create a subsegment if there isn't already one here. */
    insertsubseg(m, b, &hulltri, 1);
    /* To find the next hull edge, go clockwise around the next vertex. */
    lnextself(hulltri);
    oprev(hulltri, nexttri);
    while (nexttri.tri != m->dummytri) {
      otricopy(nexttri, hulltri);
      oprev(hulltri, nexttri);
    }
  } while (!otriequal(hulltri, starttri));
}

void formskeleton(struct mesh *m, struct behavior *b, int *segmentlist,
                  int *segmentmarkerlist, int numberofsegments)
{

  char polyfilename[6];
  int index;

  vertex endpoint1, endpoint2;
  int segmentmarkers;
  int end1, end2;
  int boundmarker;
  int i;

  if (b->poly) {
    if (!b->quiet) {
      //  TRACE("Recovering segments in Delaunay triangulation.\n");
    }

    strcpy(polyfilename, "input");
    m->insegments = numberofsegments;
    segmentmarkers = segmentmarkerlist != (int *) NULL;
    index = 0;

    /* If the input vertices are collinear, there is no triangulation, */
    /*   so don't try to insert segments.                              */
    if (m->triangles.items == 0) {
      return;
    }

    /* If segments are to be inserted, compute a mapping */
    /*   from vertices to triangles.                     */
    if (m->insegments > 0) {
      makevertexmap(m, b);
      if (b->verbose) {
        //  TRACE("  Recovering PSLG segments.\n");
      }
    }

    boundmarker = 0;
    /* Read and insert the segments. */
    for (i = 0; i < m->insegments; i++) {

      end1 = segmentlist[index++];
      end2 = segmentlist[index++];
      if (segmentmarkers) {
        boundmarker = segmentmarkerlist[i];
      }

      if ((end1 < b->firstnumber) ||
          (end1 >= b->firstnumber + m->invertices)) {
        if (!b->quiet) {
          //  TRACE("Warning:  Invalid first endpoint of segment %d in %s.\n",
//                 b->firstnumber + i, polyfilename);
			assert(0);
        }
      } else if ((end2 < b->firstnumber) ||
                 (end2 >= b->firstnumber + m->invertices)) {
        if (!b->quiet) {
          //  TRACE("Warning:  Invalid second endpoint of segment %d in %s.\n",
//                 b->firstnumber + i, polyfilename);
			assert(0);
        }
      } else {
        endpoint1 = getvertex(m, b, end1);
        endpoint2 = getvertex(m, b, end2);
        if ((endpoint1[0] == endpoint2[0]) && (endpoint1[1] == endpoint2[1])) {
          if (!b->quiet) {
            //  TRACE("Warning:  Endpoints of segment %d are coincident in %s.\n",
//                   b->firstnumber + i, polyfilename);
			  assert(0);
          }
        } else {
			insertsegment(m, b, endpoint1, endpoint2, boundmarker);
        }
      }
    }
  } else {
    m->insegments = 0;
  }
  if (b->convex || !b->poly) {
    /* Enclose the convex hull with subsegments. */
    if (b->verbose) {
      //  TRACE("  Enclosing convex hull with segments.\n");
    }
    markhull(m, b);
  }
}

void infecthull(struct mesh *m, struct behavior *b)
{
  struct otri hulltri;
  struct otri nexttri;
  struct otri starttri;
  struct osub hullsubseg;
  triangle **deadtriangle;
  vertex horg, hdest;
  triangle ptr;                         /* Temporary variable used by sym(). */
  subseg sptr;                      /* Temporary variable used by tspivot(). */

  if (b->verbose) {
    //  TRACE("  Marking concavities (external triangles) for elimination.\n");
  }
  /* Find a triangle handle on the hull. */
  hulltri.tri = m->dummytri;
  hulltri.orient = 0;
  symself(hulltri);
  /* Remember where we started so we know when to stop. */
  otricopy(hulltri, starttri);
  /* Go once counterclockwise around the convex hull. */
  do {
    /* Ignore triangles that are already infected. */
    if (!infected(hulltri)) {
      /* Is the triangle protected by a subsegment? */
      tspivot(hulltri, hullsubseg);
      if (hullsubseg.ss == m->dummysub) {
        /* The triangle is not protected; infect it. */
        if (!infected(hulltri)) {
          infect(hulltri);
          deadtriangle = (triangle **) poolalloc(&m->viri);
          *deadtriangle = hulltri.tri;
        }
      } else {
        /* The triangle is protected; set boundary markers if appropriate. */
        if (mark(hullsubseg) == 0) {
          setmark(hullsubseg, 1);
          org(hulltri, horg);
          dest(hulltri, hdest);
          if (vertexmark(horg) == 0) {
            setvertexmark(horg, 1);
          }
          if (vertexmark(hdest) == 0) {
            setvertexmark(hdest, 1);
          }
        }
      }
    }
    /* To find the next hull edge, go clockwise around the next vertex. */
    lnextself(hulltri);
    oprev(hulltri, nexttri);
    while (nexttri.tri != m->dummytri) {
      otricopy(nexttri, hulltri);
      oprev(hulltri, nexttri);
    }
  } while (!otriequal(hulltri, starttri));
}

void plague(struct mesh *m, struct behavior *b)
{
  struct otri testtri;
  struct otri neighbor;
  triangle **virusloop;
  triangle **deadtriangle;
  struct osub neighborsubseg;
  vertex testvertex;
  vertex norg, ndest;
  vertex deadorg, deaddest, deadapex;
  int killorg;
  triangle ptr;             /* Temporary variable used by sym() and onext(). */
  subseg sptr;                      /* Temporary variable used by tspivot(). */

  if (b->verbose) {
    //  TRACE("  Marking neighbors of marked triangles.\n");
  }
  /* Loop through all the infected triangles, spreading the virus to */
  /*   their neighbors, then to their neighbors' neighbors.          */
  traversalinit(&m->viri);
  virusloop = (triangle **) traverse(&m->viri);
  while (virusloop != (triangle **) NULL) {
    testtri.tri = *virusloop;
    /* A triangle is marked as infected by messing with one of its pointers */
    /*   to subsegments, setting it to an illegal value.  Hence, we have to */
    /*   temporarily uninfect this triangle so that we can examine its      */
    /*   adjacent subsegments.                                              */
    uninfect(testtri);
    if (b->verbose > 2) {
      /* Assign the triangle an orientation for convenience in */
      /*   checking its vertices.                              */
      testtri.orient = 0;
      org(testtri, deadorg);
      dest(testtri, deaddest);
      apex(testtri, deadapex);
      //  TRACE("    Checking (%.12g, %.12g) (%.12g, %.12g) (%.12g, %.12g)\n",
    //         deadorg[0], deadorg[1], deaddest[0], deaddest[1],
//             deadapex[0], deadapex[1]);
    }
    /* Check each of the triangle's three neighbors. */
    for (testtri.orient = 0; testtri.orient < 3; testtri.orient++) {
      /* Find the neighbor. */
      sym(testtri, neighbor);
      /* Check for a subsegment between the triangle and its neighbor. */
      tspivot(testtri, neighborsubseg);
      /* Check if the neighbor is nonexistent or already infected. */
      if ((neighbor.tri == m->dummytri) || infected(neighbor)) {
        if (neighborsubseg.ss != m->dummysub) {
          /* There is a subsegment separating the triangle from its      */
          /*   neighbor, but both triangles are dying, so the subsegment */
          /*   dies too.                                                 */
          subsegdealloc(m, neighborsubseg.ss);
          if (neighbor.tri != m->dummytri) {
            /* Make sure the subsegment doesn't get deallocated again */
            /*   later when the infected neighbor is visited.         */
            uninfect(neighbor);
            tsdissolve(neighbor);
            infect(neighbor);
          }
        }
      } else {                   /* The neighbor exists and is not infected. */
        if (neighborsubseg.ss == m->dummysub) {
          /* There is no subsegment protecting the neighbor, so */
          /*   the neighbor becomes infected.                   */
          if (b->verbose > 2) {
            org(neighbor, deadorg);
            dest(neighbor, deaddest);
            apex(neighbor, deadapex);
            //  TRACE(
           //   "    Marking (%.12g, %.12g) (%.12g, %.12g) (%.12g, %.12g)\n",
           //        deadorg[0], deadorg[1], deaddest[0], deaddest[1],
//                   deadapex[0], deadapex[1]);
          }
          infect(neighbor);
          /* Ensure that the neighbor's neighbors will be infected. */
          deadtriangle = (triangle **) poolalloc(&m->viri);
          *deadtriangle = neighbor.tri;
        } else {               /* The neighbor is protected by a subsegment. */
          /* Remove this triangle from the subsegment. */
          stdissolve(neighborsubseg);
          /* The subsegment becomes a boundary.  Set markers accordingly. */
          if (mark(neighborsubseg) == 0) {
            setmark(neighborsubseg, 1);
          }
          org(neighbor, norg);
          dest(neighbor, ndest);
          if (vertexmark(norg) == 0) {
            setvertexmark(norg, 1);
          }
          if (vertexmark(ndest) == 0) {
            setvertexmark(ndest, 1);
          }
        }
      }
    }
    /* Remark the triangle as infected, so it doesn't get added to the */
    /*   virus pool again.                                             */
    infect(testtri);
    virusloop = (triangle **) traverse(&m->viri);
  }

  if (b->verbose) {
    //  TRACE("  Deleting marked triangles.\n");
  }

  traversalinit(&m->viri);
  virusloop = (triangle **) traverse(&m->viri);
  while (virusloop != (triangle **) NULL) {
    testtri.tri = *virusloop;

    /* Check each of the three corners of the triangle for elimination. */
    /*   This is done by walking around each vertex, checking if it is  */
    /*   still connected to at least one live triangle.                 */
    for (testtri.orient = 0; testtri.orient < 3; testtri.orient++) {
      org(testtri, testvertex);
      /* Check if the vertex has already been tested. */
      if (testvertex != (vertex) NULL) {
        killorg = 1;
        /* Mark the corner of the triangle as having been tested. */
        setorg(testtri, NULL);
        /* Walk counterclockwise about the vertex. */
        onext(testtri, neighbor);
        /* Stop upon reaching a boundary or the starting triangle. */
        while ((neighbor.tri != m->dummytri) &&
               (!otriequal(neighbor, testtri))) {
          if (infected(neighbor)) {
            /* Mark the corner of this triangle as having been tested. */
            setorg(neighbor, NULL);
          } else {
            /* A live triangle.  The vertex survives. */
            killorg = 0;
          }
          /* Walk counterclockwise about the vertex. */
          onextself(neighbor);
        }
        /* If we reached a boundary, we must walk clockwise as well. */
        if (neighbor.tri == m->dummytri) {
          /* Walk clockwise about the vertex. */
          oprev(testtri, neighbor);
          /* Stop upon reaching a boundary. */
          while (neighbor.tri != m->dummytri) {
            if (infected(neighbor)) {
            /* Mark the corner of this triangle as having been tested. */
              setorg(neighbor, NULL);
            } else {
              /* A live triangle.  The vertex survives. */
              killorg = 0;
            }
            /* Walk clockwise about the vertex. */
            oprevself(neighbor);
          }
        }
        if (killorg) {
          if (b->verbose > 1) {
            //  TRACE("    Deleting vertex (%.12g, %.12g)\n",
//                   testvertex[0], testvertex[1]);
          }
          setvertextype(testvertex, UNDEADVERTEX);
          m->undeads++;
        }
      }
    }

    /* Record changes in the number of boundary edges, and disconnect */
    /*   dead triangles from their neighbors.                         */
    for (testtri.orient = 0; testtri.orient < 3; testtri.orient++) {
      sym(testtri, neighbor);
      if (neighbor.tri == m->dummytri) {
        /* There is no neighboring triangle on this edge, so this edge    */
        /*   is a boundary edge.  This triangle is being deleted, so this */
        /*   boundary edge is deleted.                                    */
        m->hullsize--;
      } else {
        /* Disconnect the triangle from its neighbor. */
        dissolve(neighbor);
        /* There is a neighboring triangle on this edge, so this edge */
        /*   becomes a boundary edge when this triangle is deleted.   */
        m->hullsize++;
      }
    }
    /* Return the dead triangle to the pool of triangles. */
    triangledealloc(m, testtri.tri);
    virusloop = (triangle **) traverse(&m->viri);
  }
  /* Empty the virus pool. */
  poolrestart(&m->viri);
}

void regionplague(struct mesh *m, struct behavior *b,
                  sgFloat attribute, sgFloat area)
{
  struct otri testtri;
  struct otri neighbor;
  triangle **virusloop;
  triangle **regiontri;
  struct osub neighborsubseg;
  vertex regionorg, regiondest, regionapex;
  triangle ptr;             /* Temporary variable used by sym() and onext(). */
  subseg sptr;                      /* Temporary variable used by tspivot(). */

  if (b->verbose > 1) {
    //  TRACE("  Marking neighbors of marked triangles.\n");
  }
  /* Loop through all the infected triangles, spreading the attribute      */
  /*   and/or area constraint to their neighbors, then to their neighbors' */
  /*   neighbors.                                                          */
  traversalinit(&m->viri);
  virusloop = (triangle **) traverse(&m->viri);
  while (virusloop != (triangle **) NULL) {
    testtri.tri = *virusloop;
    /* A triangle is marked as infected by messing with one of its pointers */
    /*   to subsegments, setting it to an illegal value.  Hence, we have to */
    /*   temporarily uninfect this triangle so that we can examine its      */
    /*   adjacent subsegments.                                              */
    uninfect(testtri);
    if (b->regionattrib) {
      /* Set an attribute. */
      setelemattribute(testtri, m->eextras, attribute);
    }
    if (b->vararea) {
      /* Set an area constraint. */
      setareabound(testtri, area);
    }
    if (b->verbose > 2) {
      /* Assign the triangle an orientation for convenience in */
      /*   checking its vertices.                              */
      testtri.orient = 0;
      org(testtri, regionorg);
      dest(testtri, regiondest);
      apex(testtri, regionapex);
      //  TRACE("    Checking (%.12g, %.12g) (%.12g, %.12g) (%.12g, %.12g)\n",
     //        regionorg[0], regionorg[1], regiondest[0], regiondest[1],
//             regionapex[0], regionapex[1]);
    }
    /* Check each of the triangle's three neighbors. */
    for (testtri.orient = 0; testtri.orient < 3; testtri.orient++) {
      /* Find the neighbor. */
      sym(testtri, neighbor);
      /* Check for a subsegment between the triangle and its neighbor. */
      tspivot(testtri, neighborsubseg);
      /* Make sure the neighbor exists, is not already infected, and */
      /*   isn't protected by a subsegment.                          */
      if ((neighbor.tri != m->dummytri) && !infected(neighbor)
          && (neighborsubseg.ss == m->dummysub)) {
        if (b->verbose > 2) {
          org(neighbor, regionorg);
          dest(neighbor, regiondest);
          apex(neighbor, regionapex);
          //  TRACE("    Marking (%.12g, %.12g) (%.12g, %.12g) (%.12g, %.12g)\n",
           //      regionorg[0], regionorg[1], regiondest[0], regiondest[1],
           //      regionapex[0], regionapex[1]);
        }
        /* Infect the neighbor. */
        infect(neighbor);
        /* Ensure that the neighbor's neighbors will be infected. */
        regiontri = (triangle **) poolalloc(&m->viri);
        *regiontri = neighbor.tri;
      }
    }
    /* Remark the triangle as infected, so it doesn't get added to the */
    /*   virus pool again.                                             */
    infect(testtri);
    virusloop = (triangle **) traverse(&m->viri);
  }

  /* Uninfect all triangles. */
  if (b->verbose > 1) {
    //  TRACE("  Unmarking marked triangles.\n");
  }
  traversalinit(&m->viri);
  virusloop = (triangle **) traverse(&m->viri);
  while (virusloop != (triangle **) NULL) {
    testtri.tri = *virusloop;
    uninfect(testtri);
    virusloop = (triangle **) traverse(&m->viri);
  }
  /* Empty the virus pool. */
  poolrestart(&m->viri);
}

void carveholes(struct mesh *m, struct behavior *b, sgFloat *holelist, int holes,
                sgFloat *regionlist, int regions)
{
  struct otri searchtri;
  struct otri triangleloop;
  struct otri *regiontris;
  triangle **holetri;
  triangle **regiontri;
  vertex searchorg, searchdest;
  enum locateresult intersect;
  int i;
  triangle ptr;                         /* Temporary variable used by sym(). */

  if (!(b->quiet || (b->noholes && b->convex))) {
    //  TRACE("Removing unwanted triangles.\n");
    if (b->verbose && (holes > 0)) {
      //  TRACE("  Marking holes for elimination.\n");
    }
  }
  if (regions > 0) {
    /* Allocate storage for the triangles in which region points fall. */
    regiontris = (struct otri *)
                 trimalloc(regions * (int) sizeof(struct otri));
  }

  if (((holes > 0) && !b->noholes) || !b->convex || (regions > 0)) {
    /* Initialize a pool of viri to be used for holes, concavities, */
    /*   regional attributes, and/or regional area constraints.     */
    poolinit(&m->viri, sizeof(triangle *), VIRUSPERBLOCK, VIRUSPERBLOCK,
             POINTER, 0);
  }

  if (!b->convex) {
    /* Mark as infected any unprotected triangles on the boundary. */
    /*   This is one way by which concavities are created.         */
    infecthull(m, b);
  }

  if ((holes > 0) && !b->noholes) {
    /* Infect each triangle in which a hole lies. */
    for (i = 0; i < 2 * holes; i += 2) {
      /* Ignore holes that aren't within the bounds of the mesh. */
      if ((holelist[i] >= m->xmin) && (holelist[i] <= m->xmax)
          && (holelist[i + 1] >= m->ymin) && (holelist[i + 1] <= m->ymax)) {
        /* Start searching from some triangle on the outer boundary. */
        searchtri.tri = m->dummytri;
        searchtri.orient = 0;
        symself(searchtri);
        /* Ensure that the hole is to the left of this boundary edge; */
        /*   otherwise, locate() will falsely report that the hole    */
        /*   falls within the starting triangle.                      */
        org(searchtri, searchorg);
        dest(searchtri, searchdest);
        if (counterclockwise(m, b, searchorg, searchdest, &holelist[i]) >
            0.0) {
          /* Find a triangle that contains the hole. */
          intersect = locate(m, b, &holelist[i], &searchtri);
          if ((intersect != OUTSIDE) && (!infected(searchtri))) {
            /* Infect the triangle.  This is done by marking the triangle  */
            /*   as infected and including the triangle in the virus pool. */
            infect(searchtri);
            holetri = (triangle **) poolalloc(&m->viri);
            *holetri = searchtri.tri;
          }
        }
      }
    }
  }

  /* Now, we have to find all the regions BEFORE we carve the holes, because */
  /*   locate() won't work when the triangulation is no longer convex.       */
  /*   (Incidentally, this is the reason why regional attributes and area    */
  /*   constraints can't be used when refining a preexisting mesh, which     */
  /*   might not be convex; they can only be used with a freshly             */
  /*   triangulated PSLG.)                                                   */
  if (regions > 0) {
    /* Find the starting triangle for each region. */
    for (i = 0; i < regions; i++) {
      regiontris[i].tri = m->dummytri;
      /* Ignore region points that aren't within the bounds of the mesh. */
      if ((regionlist[4 * i] >= m->xmin) && (regionlist[4 * i] <= m->xmax) &&
          (regionlist[4 * i + 1] >= m->ymin) &&
          (regionlist[4 * i + 1] <= m->ymax)) {
        /* Start searching from some triangle on the outer boundary. */
        searchtri.tri = m->dummytri;
        searchtri.orient = 0;
        symself(searchtri);
        /* Ensure that the region point is to the left of this boundary */
        /*   edge; otherwise, locate() will falsely report that the     */
        /*   region point falls within the starting triangle.           */
        org(searchtri, searchorg);
        dest(searchtri, searchdest);
        if (counterclockwise(m, b, searchorg, searchdest, &regionlist[4 * i]) >
            0.0) {
          /* Find a triangle that contains the region point. */
          intersect = locate(m, b, &regionlist[4 * i], &searchtri);
          if ((intersect != OUTSIDE) && (!infected(searchtri))) {
            /* Record the triangle for processing after the */
            /*   holes have been carved.                    */
            otricopy(searchtri, regiontris[i]);
          }
        }
      }
    }
  }

  if (m->viri.items > 0) {
    /* Carve the holes and concavities. */
    plague(m, b);
  }
  /* The virus pool should be empty now. */

  if (regions > 0) {
    if (!b->quiet) {
      if (b->regionattrib) {
        if (b->vararea) {
          //  TRACE("Spreading regional attributes and area constraints.\n");
        } else {
          //  TRACE("Spreading regional attributes.\n");
        }
      } else { 
        //  TRACE("Spreading regional area constraints.\n");
      }
    }
    if (b->regionattrib && !b->refine) {
      /* Assign every triangle a regional attribute of zero. */
      traversalinit(&m->triangles);
      triangleloop.orient = 0;
      triangleloop.tri = triangletraverse(m);
      while (triangleloop.tri != (triangle *) NULL) {
        setelemattribute(triangleloop, m->eextras, 0.0);
        triangleloop.tri = triangletraverse(m);
      }
    }
    for (i = 0; i < regions; i++) {
      if (regiontris[i].tri != m->dummytri) {
        /* Make sure the triangle under consideration still exists. */
        /*   It may have been eaten by the virus.                   */
        if (!deadtri(regiontris[i].tri)) {
          /* Put one triangle in the virus pool. */
          infect(regiontris[i]);
          regiontri = (triangle **) poolalloc(&m->viri);
          *regiontri = regiontris[i].tri;
          /* Apply one region's attribute and/or area constraint. */
          regionplague(m, b, regionlist[4 * i + 2], regionlist[4 * i + 3]);
          /* The virus pool should be empty now. */
        }
      }
    }
    if (b->regionattrib && !b->refine) {
      /* Note the fact that each triangle has an additional attribute. */
      m->eextras++;
    }
  }

  /* Free up memory. */
  if (((holes > 0) && !b->noholes) || !b->convex || (regions > 0)) {
    pooldeinit(&m->viri);
  }
  if (regions > 0) {
    trifree((int *) regiontris);
  }
}

#ifndef CDT_ONLY

void tallyencs(struct mesh *m, struct behavior *b)
{
  struct osub subsegloop;
  int dummy;

  traversalinit(&m->subsegs);
  subsegloop.ssorient = 0;
  subsegloop.ss = subsegtraverse(m);
  while (subsegloop.ss != (subseg *) NULL) {
    /* If the segment is encroached, add it to the list. */
    dummy = checkseg4encroach(m, b, &subsegloop, 0.0);
    subsegloop.ss = subsegtraverse(m);
  }
}

#endif /* not CDT_ONLY */

#ifndef CDT_ONLY

void precisionerror()
{
  //  TRACE("Try increasing the area criterion and/or reducing the minimum\n");
  //  TRACE("  allowable angle so that tiny triangles are not created.\n");
#ifdef SINGLE
  //  TRACE("Alternatively, try recompiling me with sgFloat precision\n");
  //  TRACE("  arithmetic (by removing \"#define SINGLE\" from the\n");
  //  TRACE("  source file or \"-DSINGLE\" from the makefile).\n");
#endif /* SINGLE */
}

#endif /* not CDT_ONLY */

#ifndef CDT_ONLY

void splitencsegs(struct mesh *m, struct behavior *b, int triflaws)
{
  struct otri enctri;
  struct otri testtri;
  struct osub testsh;
  struct osub currentenc;
  struct badsubseg *encloop;
  vertex eorg, edest, eapex;
  vertex newvertex;
  enum insertvertexresult success;
  sgFloat segmentlength, nearestpoweroftwo;
  sgFloat split;
  sgFloat multiplier, divisor;
  int acuteorg, acuteorg2, acutedest, acutedest2;
  int dummy;
  int i;
  triangle ptr;                     /* Temporary variable used by stpivot(). */
  subseg sptr;                        /* Temporary variable used by snext(). */

  /* Note that steinerleft == -1 if an unlimited number */
  /*   of Steiner points is allowed.                    */
  while ((m->badsubsegs.items > 0) && (m->steinerleft != 0)) {
    traversalinit(&m->badsubsegs);
    encloop = badsubsegtraverse(m);
    while ((encloop != (struct badsubseg *) NULL) && (m->steinerleft != 0)) {
      sdecode(encloop->encsubseg, currentenc);
      sorg(currentenc, eorg);
      sdest(currentenc, edest);
      /* Make sure that this segment is still the same segment it was   */
      /*   when it was determined to be encroached.  If the segment was */
      /*   enqueued multiple times (because several newly inserted      */
      /*   vertices encroached it), it may have already been split.     */
      if (!deadsubseg(currentenc.ss) &&
          (eorg == encloop->subsegorg) && (edest == encloop->subsegdest)) {
      
        /* Is the origin shared with another segment? */
        stpivot(currentenc, enctri);
        lnext(enctri, testtri);
        tspivot(testtri, testsh);
        acuteorg = testsh.ss != m->dummysub;
        /* Is the destination shared with another segment? */
        lnextself(testtri);
        tspivot(testtri, testsh);
        acutedest = testsh.ss != m->dummysub;

        /* If we're using diametral lenses (rather than diametral circles) */
        /*   to define encroachment, delete free vertices from the         */
        /*   subsegment's diametral circle.                                */
        if (!b->nolenses && !acuteorg && !acutedest) {
          apex(enctri, eapex);
          while ((vertextype(eapex) == FREEVERTEX) &&
                 ((eorg[0] - eapex[0]) * (edest[0] - eapex[0]) +
                  (eorg[1] - eapex[1]) * (edest[1] - eapex[1]) < 0.0)) {
            deletevertex(m, b, &testtri);
            stpivot(currentenc, enctri);
            apex(enctri, eapex);
            lprev(enctri, testtri);
          }
        }

        /* Now, check the other side of the segment, if there's a triangle */
        /*   there.                                                        */
        sym(enctri, testtri);
        if (testtri.tri != m->dummytri) {
          /* Is the destination shared with another segment? */
          lnextself(testtri);
          tspivot(testtri, testsh);
          acutedest2 = testsh.ss != m->dummysub;
          acutedest = acutedest || acutedest2;
          /* Is the origin shared with another segment? */
          lnextself(testtri);
          tspivot(testtri, testsh);
          acuteorg2 = testsh.ss != m->dummysub;
          acuteorg = acuteorg || acuteorg2;

          /* Delete free vertices from the subsegment's diametral circle. */
          if (!b->nolenses && !acuteorg2 && !acutedest2) {
            org(testtri, eapex);
            while ((vertextype(eapex) == FREEVERTEX) &&
                   ((eorg[0] - eapex[0]) * (edest[0] - eapex[0]) +
                    (eorg[1] - eapex[1]) * (edest[1] - eapex[1]) < 0.0)) {
              deletevertex(m, b, &testtri);
              sym(enctri, testtri);
              apex(testtri, eapex);
              lprevself(testtri);
            }
          }
        }

        /* Use the concentric circles if exactly one endpoint is shared */
        /*   with another adjacent segment.                             */
        if (acuteorg || acutedest) {
          segmentlength = sqrt((edest[0] - eorg[0]) * (edest[0] - eorg[0]) +
                               (edest[1] - eorg[1]) * (edest[1] - eorg[1]));
          /* Find the power of two that most evenly splits the segment.  */
          /*   The worst case is a 2:1 ratio between subsegment lengths. */
          nearestpoweroftwo = 1.0;
          while (segmentlength > 3.0 * nearestpoweroftwo) {
            nearestpoweroftwo *= 2.0;
          }
          while (segmentlength < 1.5 * nearestpoweroftwo) {
            nearestpoweroftwo *= 0.5;
          }
          /* Where do we split the segment? */
          split = nearestpoweroftwo / segmentlength;
          if (acutedest) {
            split = 1.0 - split;
          }
        } else {
          /* If we're not worried about adjacent segments, split */
          /*   this segment in the middle.                       */
          split = 0.5;
        }

        /* Create the new vertex. */
        newvertex = (vertex) poolalloc(&m->vertices);
        /* Interpolate its coordinate and attributes. */
        for (i = 0; i < 2 + m->nextras; i++) {
          newvertex[i] = eorg[i] + split * (edest[i] - eorg[i]);
        }

        if (!b->noexact) {
          /* Roundoff in the above calculation may yield a `newvertex'   */
          /*   that is not precisely collinear with `eorg' and `edest'.  */
          /*   Improve collinearity by one step of iterative refinement. */
          multiplier = counterclockwise(m, b, eorg, edest, newvertex);
          divisor = ((eorg[0] - edest[0]) * (eorg[0] - edest[0]) +
                     (eorg[1] - edest[1]) * (eorg[1] - edest[1]));
          if ((multiplier != 0.0) && (divisor != 0.0)) {
            multiplier = multiplier / divisor;
            /* Watch out for NANs. */
            if (multiplier == multiplier) {
              newvertex[0] += multiplier * (edest[1] - eorg[1]);
              newvertex[1] += multiplier * (eorg[0] - edest[0]);
            }
          }
        }

        setvertexmark(newvertex, mark(currentenc));
        setvertextype(newvertex, SEGMENTVERTEX);
        if (b->verbose > 1) {
          //  TRACE(
  //"  Splitting subsegment (%.12g, %.12g) (%.12g, %.12g) at (%.12g, %.12g).\n",
  //               eorg[0], eorg[1], edest[0], edest[1],
    //             newvertex[0], newvertex[1]);
        }
        /* Check whether the new vertex lies on an endpoint. */
        if (((newvertex[0] == eorg[0]) && (newvertex[1] == eorg[1])) ||
            ((newvertex[0] == edest[0]) && (newvertex[1] == edest[1]))) {
          //  TRACE("Error:  Ran out of precision at (%.12g, %.12g).\n",
//                 newvertex[0], newvertex[1]);
          //  TRACE("I attempted to split a segment to a smaller size than\n");
          //  TRACE("  can be accommodated by the finite precision of\n");
          //  TRACE("  floating point arithmetic.\n");
          precisionerror();
          exit(1);
        }
        /* Insert the splitting vertex.  This should always succeed. */
        success = insertvertex(m, b, newvertex, &enctri, &currentenc,
                               1, triflaws, 0.0);
        if ((success != SUCCESSFULVERTEX) && (success != ENCROACHINGVERTEX)) {
          //  TRACE("Internal error in splitencsegs():\n");
          //  TRACE("  Failure to split a segment.\n");
          internalerror();
        }
        if (m->steinerleft > 0) {
          m->steinerleft--;
        }
        /* Check the two new subsegments to see if they're encroached. */
        dummy = checkseg4encroach(m, b, &currentenc, 0.0);
        snextself(currentenc);
        dummy = checkseg4encroach(m, b, &currentenc, 0.0);
      }

      badsubsegdealloc(m, encloop);
      encloop = badsubsegtraverse(m);
    }
  }
}

#endif /* not CDT_ONLY */

#ifndef CDT_ONLY

void tallyfaces(struct mesh *m, struct behavior *b)
{
  struct otri triangleloop;

  if (b->verbose) {
    //  TRACE("  Making a list of bad triangles.\n");
  }
  traversalinit(&m->triangles);
  triangleloop.orient = 0;
  triangleloop.tri = triangletraverse(m);
  while (triangleloop.tri != (triangle *) NULL) {
    /* If the triangle is bad, enqueue it. */
    testtriangle(m, b, &triangleloop);
    triangleloop.tri = triangletraverse(m);
  }
}

#endif /* not CDT_ONLY */

#ifndef CDT_ONLY

void splittriangle(struct mesh *m, struct behavior *b,
                   struct badtriang *badtri)
{
  struct otri badotri;
  vertex borg, bdest, bapex;
  vertex newvertex;
  sgFloat xi, eta;
  sgFloat minedge;
  enum insertvertexresult success;
  int errorflag;
  int i;

  decode(badtri->poortri, badotri);
  org(badotri, borg);
  dest(badotri, bdest);
  apex(badotri, bapex);
  /* Make sure that this triangle is still the same triangle it was      */
  /*   when it was tested and determined to be of bad quality.           */
  /*   Subsequent transformations may have made it a different triangle. */
  if (!deadtri(badotri.tri) && (borg == badtri->triangorg) &&
      (bdest == badtri->triangdest) && (bapex == badtri->triangapex)) {
    if (b->verbose > 1) {
      //  TRACE("  Splitting this triangle at its circumcenter:\n");
      //  TRACE("    (%.12g, %.12g) (%.12g, %.12g) (%.12g, %.12g)\n", borg[0],
//             borg[1], bdest[0], bdest[1], bapex[0], bapex[1]);
    }

    errorflag = 0;
    /* Create a new vertex at the triangle's circumcenter. */
    newvertex = (vertex) poolalloc(&m->vertices);
    findcircumcenter(m, b, borg, bdest, bapex, newvertex, &xi, &eta, &minedge,
                     1);

    /* Check whether the new vertex lies on a triangle vertex. */
    if (((newvertex[0] == borg[0]) && (newvertex[1] == borg[1])) ||
        ((newvertex[0] == bdest[0]) && (newvertex[1] == bdest[1])) ||
        ((newvertex[0] == bapex[0]) && (newvertex[1] == bapex[1]))) {
      if (!b->quiet) {
        //  TRACE(
        //    "Warning:  New vertex (%.12g, %.12g) falls on existing vertex.\n",
        //       newvertex[0], newvertex[1]);
        errorflag = 1;
      }
      vertexdealloc(m, newvertex);
    } else {
      for (i = 2; i < 2 + m->nextras; i++) {
        /* Interpolate the vertex attributes at the circumcenter. */
        newvertex[i] = borg[i] + xi * (bdest[i] - borg[i])
                              + eta * (bapex[i] - borg[i]);
      }
      /* The new vertex must be in the interior, and therefore is a */
      /*   free vertex with a marker of zero.                       */
      setvertexmark(newvertex, 0);
      setvertextype(newvertex, FREEVERTEX);

      /* Ensure that the handle `badotri' does not represent the longest  */
      /*   edge of the triangle.  This ensures that the circumcenter must */
      /*   fall to the left of this edge, so point location will work.    */
      /*   (If the angle org-apex-dest exceeds 90 degrees, then the       */
      /*   circumcenter lies outside the org-dest edge, and eta is        */
      /*   negative.  Roundoff error might prevent eta from being         */
      /*   negative when it should be, so I test eta against xi.)         */
      if (eta < xi) {
        lprevself(badotri);
      }

      /* Insert the circumcenter, searching from the edge of the triangle, */
      /*   and maintain the Delaunay property of the triangulation.        */
      success = insertvertex(m, b, newvertex, &badotri, (struct osub *) NULL,
                             1, 1, minedge);
      if (success == SUCCESSFULVERTEX) {
        if (m->steinerleft > 0) {
          m->steinerleft--;
        }
      } else if (success == ENCROACHINGVERTEX) {
        /* If the newly inserted vertex encroaches upon a subsegment, */
        /*   delete the new vertex.                                   */
        undovertex(m, b);
        if (b->verbose > 1) {
          //  TRACE("  Rejecting (%.12g, %.12g).\n", newvertex[0], newvertex[1]);
        }
        vertexdealloc(m, newvertex);
      } else if (success == VIOLATINGVERTEX) {
        /* Failed to insert the new vertex, but some subsegment was */
        /*   marked as being encroached.                            */
        vertexdealloc(m, newvertex);
      } else {                                 /* success == DUPLICATEVERTEX */
        /* Couldn't insert the new vertex because a vertex is already there. */
        if (!b->quiet) {
          //  TRACE(
         //   "Warning:  New vertex (%.12g, %.12g) falls on existing vertex.\n",
          //       newvertex[0], newvertex[1]);
          errorflag = 1;
        }
        vertexdealloc(m, newvertex);
      }
    }
    if (errorflag) {
      if (b->verbose) {
        //  TRACE("  The new vertex is at the circumcenter of triangle\n");
        //  TRACE("    (%.12g, %.12g) (%.12g, %.12g) (%.12g, %.12g)\n",
//               borg[0], borg[1], bdest[0], bdest[1], bapex[0], bapex[1]);
      }
      //  TRACE("This probably means that I am trying to refine triangles\n");
      //  TRACE("  to a smaller size than can be accommodated by the finite\n");
      //  TRACE("  precision of floating point arithmetic.  (You can be\n");
      //  TRACE("  sure of this if I fail to terminate.)\n");
      precisionerror();
    }
  }
}

#endif /* not CDT_ONLY */

#ifndef CDT_ONLY

void enforcequality(struct mesh *m, struct behavior *b)
{
  struct badtriang *badtri;
  int i;

  if (!b->quiet) {
    //  TRACE("Adding Steiner points to enforce quality.\n");
  }
  /* Initialize the pool of encroached subsegments. */
  poolinit(&m->badsubsegs, sizeof(struct badsubseg), BADSUBSEGPERBLOCK,
           BADSUBSEGPERBLOCK, POINTER, 0);
  if (b->verbose) {
    //  TRACE("  Looking for encroached subsegments.\n");
  }
  /* Test all segments to see if they're encroached. */
  tallyencs(m, b);
  if (b->verbose && (m->badsubsegs.items > 0)) {
    //  TRACE("  Splitting encroached subsegments.\n");
  }
  /* Fix encroached subsegments without noting bad triangles. */
  splitencsegs(m, b, 0);
  /* At this point, if we haven't run out of Steiner points, the */
  /*   triangulation should be (conforming) Delaunay.            */

  /* Next, we worry about enforcing triangle quality. */
  if ((b->minangle > 0.0) || b->vararea || b->fixedarea || b->usertest) {
    /* Initialize the pool of bad triangles. */
    poolinit(&m->badtriangles, sizeof(struct badtriang), BADTRIPERBLOCK,
             BADTRIPERBLOCK, POINTER, 0);
    /* Initialize the queues of bad triangles. */
    for (i = 0; i < 64; i++) {
      m->queuefront[i] = (struct badtriang *) NULL;
    }
    m->firstnonemptyq = -1;
    /* Test all triangles to see if they're bad. */
    tallyfaces(m, b);
    /* Initialize the pool of recently flipped triangles. */
    poolinit(&m->flipstackers, sizeof(struct flipstacker), FLIPSTACKERPERBLOCK,
             FLIPSTACKERPERBLOCK, POINTER, 0);
    m->checkquality = 1;
    if (b->verbose) {
      //  TRACE("  Splitting bad triangles.\n");
    }
    while ((m->badtriangles.items > 0) && (m->steinerleft != 0)) {
      /* Fix one bad triangle by inserting a vertex at its circumcenter. */
      badtri = dequeuebadtriang(m);
      splittriangle(m, b, badtri);
      if (m->badsubsegs.items > 0) {
        /* Put bad triangle back in queue for another try later. */
        enqueuebadtriang(m, b, badtri);
        /* Fix any encroached subsegments that resulted. */
        /*   Record any new bad triangles that result.   */
        splitencsegs(m, b, 1);
      } else {
        /* Return the bad triangle to the pool. */
        pooldealloc(&m->badtriangles, (int *) badtri);
      }
    }
  }
  /* At this point, if we haven't run out of Steiner points, the */
  /*   triangulation should be (conforming) Delaunay and have no */
  /*   low-quality triangles.                                    */

  /* Might we have run out of Steiner points too soon? */
  if (!b->quiet && (m->badsubsegs.items > 0) && (m->steinerleft == 0)) {
    //  TRACE("\nWarning:  I ran out of Steiner points, but the mesh has\n");
    if (m->badsubsegs.items == 1) {
      //  TRACE("  an encroached subsegment, and therefore might not be truly\n");
    } else {
      //  TRACE("  %ld encroached subsegments, and therefore might not be truly\n"
//             , m->badsubsegs.items);
    }
    //  TRACE("  Delaunay.  If the Delaunay property is important to you,\n");
    //  TRACE("  try increasing the number of Steiner points (controlled by\n");
    //  TRACE("  the -S switch) slightly and try again.\n\n");
  }
}

#endif /* not CDT_ONLY */

void highorder(struct mesh *m, struct behavior *b)
{
  struct otri triangleloop, trisym;
  struct osub checkmark;
  vertex newvertex;
  vertex torg, tdest;
  int i;
  triangle ptr;                         /* Temporary variable used by sym(). */
  subseg sptr;                      /* Temporary variable used by tspivot(). */

  if (!b->quiet) {
    //  TRACE("Adding vertices for second-order triangles.\n");
  }
  
  m->vertices.deaditemstack = (int *) NULL;

  traversalinit(&m->triangles);
  triangleloop.tri = triangletraverse(m);
  
  while (triangleloop.tri != (triangle *) NULL) {
    for (triangleloop.orient = 0; triangleloop.orient < 3;
         triangleloop.orient++) {
      sym(triangleloop, trisym);
      if ((triangleloop.tri < trisym.tri) || (trisym.tri == m->dummytri)) {
        org(triangleloop, torg);
        dest(triangleloop, tdest);
        /* Create a new node in the middle of the edge.  Interpolate */
        /*   its attributes.                                         */
        newvertex = (vertex) poolalloc(&m->vertices);
        for (i = 0; i < 2 + m->nextras; i++) {
          newvertex[i] = 0.5 * (torg[i] + tdest[i]);
        }
        /* Set the new node's marker to zero or one, depending on */
        /*   whether it lies on a boundary.                       */
        setvertexmark(newvertex, trisym.tri == m->dummytri);
        setvertextype(newvertex,
                      trisym.tri == m->dummytri ? FREEVERTEX : SEGMENTVERTEX);
        if (b->usesegments) {
          tspivot(triangleloop, checkmark);
          /* If this edge is a segment, transfer the marker to the new node. */
          if (checkmark.ss != m->dummysub) {
            setvertexmark(newvertex, mark(checkmark));
            setvertextype(newvertex, SEGMENTVERTEX);
          }
        }
        if (b->verbose > 1) {
          //  TRACE("  Creating (%.12g, %.12g).\n", newvertex[0], newvertex[1]);
        }
        /* Record the new node in the (one or two) adjacent elements. */
        triangleloop.tri[m->highorderindex + triangleloop.orient] =
                (triangle) newvertex;
        if (trisym.tri != m->dummytri) {
          trisym.tri[m->highorderindex + trisym.orient] = (triangle) newvertex;
        }
      }
    }
    triangleloop.tri = triangletraverse(m);
  }
}

void transfernodes(struct mesh *m, struct behavior *b, sgFloat *pointlist,
                   sgFloat *pointattriblist, int *pointmarkerlist,
                   int numberofpoints, int numberofpointattribs)
{
  vertex vertexloop;
  sgFloat x, y;
  int i, j;
  int coordindex;
  int attribindex;

  m->invertices = numberofpoints;
  m->mesh_dim = 2;
  m->nextras = numberofpointattribs;
  m->readnodefile = 0;
  if (m->invertices < 3) {
    //  TRACE("Error:  Input must have at least three input vertices.\n");
    exit(1);
  }
  if (m->nextras == 0) {
    b->weighted = 0;
  }

  initializevertexpool(m, b);

  /* Read the vertices. */
  coordindex = 0;
  attribindex = 0;
  for (i = 0; i < m->invertices; i++) {
    vertexloop = (vertex) poolalloc(&m->vertices);
    /* Read the vertex coordinates. */
    x = vertexloop[0] = pointlist[coordindex++];
    y = vertexloop[1] = pointlist[coordindex++];
    /* Read the vertex attributes. */
    for (j = 0; j < numberofpointattribs; j++) {
      vertexloop[2 + j] = pointattriblist[attribindex++];
    }
    if (pointmarkerlist != (int *) NULL) {
      /* Read a vertex marker. */
      setvertexmark(vertexloop, pointmarkerlist[i]);
    } else {
      /* If no markers are specified, they default to zero. */
      setvertexmark(vertexloop, 0);
    }
    setvertextype(vertexloop, INPUTVERTEX);
    /* Determine the smallest and largest x and y coordinates. */
    if (i == 0) {
      m->xmin = m->xmax = x;
      m->ymin = m->ymax = y;
    } else {
      m->xmin = (x < m->xmin) ? x : m->xmin;
      m->xmax = (x > m->xmax) ? x : m->xmax;
      m->ymin = (y < m->ymin) ? y : m->ymin;
      m->ymax = (y > m->ymax) ? y : m->ymax;
    }
  }

  m->xminextreme = 10 * m->xmin - 9 * m->xmax;
}

void writenodes(struct mesh *m, struct behavior *b, sgFloat **pointlist,
                sgFloat **pointattriblist, int **pointmarkerlist)
{
  sgFloat *plist;
  sgFloat *palist;
  int *pmlist;
  int coordindex;
  int attribindex;

  vertex vertexloop;
  long outvertices;
  int vertexnumber;
  int i;

  if (b->jettison) {
    outvertices = m->vertices.items - m->undeads;
  } else {
    outvertices = m->vertices.items;
  }

  if (!b->quiet) {
    //  TRACE("Writing vertices.\n");
  }
  /* Allocate memory for output vertices if necessary. */
  if (*pointlist == (sgFloat *) NULL) {
    *pointlist = (sgFloat *) trimalloc(outvertices * 2 * sizeof(sgFloat));
  }
  /* Allocate memory for output vertex attributes if necessary. */
  if ((m->nextras > 0) && (*pointattriblist == (sgFloat *) NULL)) {
    *pointattriblist = (sgFloat *) trimalloc(outvertices * m->nextras *
                                          sizeof(sgFloat));
  }
  /* Allocate memory for output vertex markers if necessary. */
  if (!b->nobound && (*pointmarkerlist == (int *) NULL)) {
    *pointmarkerlist = (int *) trimalloc(outvertices * sizeof(int));
  }
  plist = *pointlist;
  palist = *pointattriblist;
  pmlist = *pointmarkerlist;
  coordindex = 0;
  attribindex = 0;

  traversalinit(&m->vertices);
  vertexnumber = b->firstnumber;
  vertexloop = vertextraverse(m);
  while (vertexloop != (vertex) NULL) {
    if (!b->jettison || (vertextype(vertexloop) != UNDEADVERTEX)) {
      /* X and y coordinates. */
      plist[coordindex++] = vertexloop[0];
      plist[coordindex++] = vertexloop[1];
      /* Vertex attributes. */
      for (i = 0; i < m->nextras; i++) {
        palist[attribindex++] = vertexloop[2 + i];
      }
      if (!b->nobound) {
        /* Copy the boundary marker. */
        pmlist[vertexnumber - b->firstnumber] = vertexmark(vertexloop);
      }
      setvertexmark(vertexloop, vertexnumber);
      vertexnumber++;
    }
    vertexloop = vertextraverse(m);
  }

}

void numbernodes(struct mesh *m, struct behavior *b)
{
  vertex vertexloop;
  int vertexnumber;

  traversalinit(&m->vertices);
  vertexnumber = b->firstnumber;
  vertexloop = vertextraverse(m);
  while (vertexloop != (vertex) NULL) {
    setvertexmark(vertexloop, vertexnumber);
    if (!b->jettison || (vertextype(vertexloop) != UNDEADVERTEX)) {
      vertexnumber++;
    }
    vertexloop = vertextraverse(m);
  }
}

void writeelements(struct mesh *m, struct behavior *b,
                   int **trianglelist, sgFloat **triangleattriblist)
{

  int *tlist;
  sgFloat *talist;
  int vertexindex;
  int attribindex;

  struct otri triangleloop;
  vertex p1, p2, p3;
  vertex mid1, mid2, mid3;
  long elementnumber;
  int i;

  if (!b->quiet) {
    //  TRACE("Writing triangles.\n");
  }
  /* Allocate memory for output triangles if necessary. */
  if (*trianglelist == (int *) NULL) {
    *trianglelist = (int *) trimalloc(m->triangles.items *
                                      ((b->order + 1) * (b->order + 2) / 2) *
                                      sizeof(int));
  }
  /* Allocate memory for output triangle attributes if necessary. */
  if ((m->eextras > 0) && (*triangleattriblist == (sgFloat *) NULL)) {
    *triangleattriblist = (sgFloat *) trimalloc(m->triangles.items * m->eextras *
                                             sizeof(sgFloat));
  }
  tlist = *trianglelist;
  talist = *triangleattriblist;
  vertexindex = 0;
  attribindex = 0;

  traversalinit(&m->triangles);
  triangleloop.tri = triangletraverse(m);
  triangleloop.orient = 0;
  elementnumber = b->firstnumber;
  while (triangleloop.tri != (triangle *) NULL) {
    org(triangleloop, p1);
    dest(triangleloop, p2);
    apex(triangleloop, p3);
    if (b->order == 1) {

      tlist[vertexindex++] = vertexmark(p1);
      tlist[vertexindex++] = vertexmark(p2);
      tlist[vertexindex++] = vertexmark(p3);

    } else {
      mid1 = (vertex) triangleloop.tri[m->highorderindex + 1];
      mid2 = (vertex) triangleloop.tri[m->highorderindex + 2];
      mid3 = (vertex) triangleloop.tri[m->highorderindex];

      tlist[vertexindex++] = vertexmark(p1);
      tlist[vertexindex++] = vertexmark(p2);
      tlist[vertexindex++] = vertexmark(p3);
      tlist[vertexindex++] = vertexmark(mid1);
      tlist[vertexindex++] = vertexmark(mid2);
      tlist[vertexindex++] = vertexmark(mid3);

    }

    for (i = 0; i < m->eextras; i++) {
      talist[attribindex++] = elemattribute(triangleloop, i);
    }

    triangleloop.tri = triangletraverse(m);
    elementnumber++;
  }

}

void writepoly(struct mesh *m, struct behavior *b,
               int **segmentlist, int **segmentmarkerlist)

{

  int *slist;
  int *smlist;
  int index;

  struct osub subsegloop;
  vertex endpoint1, endpoint2;
  long subsegnumber;

  if (!b->quiet) {
    //  TRACE("Writing segments.\n");
  }
  /* Allocate memory for output segments if necessary. */
  if (*segmentlist == (int *) NULL) {
    *segmentlist = (int *) trimalloc(m->subsegs.items * 2 * sizeof(int));
  }
  /* Allocate memory for output segment markers if necessary. */
  if (!b->nobound && (*segmentmarkerlist == (int *) NULL)) {
    *segmentmarkerlist = (int *) trimalloc(m->subsegs.items * sizeof(int));
  }
  slist = *segmentlist;
  smlist = *segmentmarkerlist;
  index = 0;

  traversalinit(&m->subsegs);
  subsegloop.ss = subsegtraverse(m);
  subsegloop.ssorient = 0;
  subsegnumber = b->firstnumber;
  while (subsegloop.ss != (subseg *) NULL) {
    sorg(subsegloop, endpoint1);
    sdest(subsegloop, endpoint2);
    /* Copy indices of the segment's two endpoints. */
    slist[index++] = vertexmark(endpoint1);
    slist[index++] = vertexmark(endpoint2);
    if (!b->nobound) {
      /* Copy the boundary marker. */
      smlist[subsegnumber - b->firstnumber] = mark(subsegloop);
    }

    subsegloop.ss = subsegtraverse(m);
    subsegnumber++;
  }
}

void writeedges(struct mesh *m, struct behavior *b,
                int **edgelist, int **edgemarkerlist)

{

  int *elist;
  int *emlist;
  int index;

  struct otri triangleloop, trisym;
  struct osub checkmark;
  vertex p1, p2;
  long edgenumber;
  triangle ptr;                         /* Temporary variable used by sym(). */
  subseg sptr;                      /* Temporary variable used by tspivot(). */

  if (!b->quiet) {
    //  TRACE("Writing edges.\n");
  }
  /* Allocate memory for edges if necessary. */
  if (*edgelist == (int *) NULL) {
    *edgelist = (int *) trimalloc(m->edges * 2 * sizeof(int));
  }
  /* Allocate memory for edge markers if necessary. */
  if (!b->nobound && (*edgemarkerlist == (int *) NULL)) {
    *edgemarkerlist = (int *) trimalloc(m->edges * sizeof(int));
  }
  elist = *edgelist;
  emlist = *edgemarkerlist;
  index = 0;


  traversalinit(&m->triangles);
  triangleloop.tri = triangletraverse(m);
  edgenumber = b->firstnumber;
  /* To loop over the set of edges, loop over all triangles, and look at   */
  /*   the three edges of each triangle.  If there isn't another triangle  */
  /*   adjacent to the edge, operate on the edge.  If there is another     */
  /*   adjacent triangle, operate on the edge only if the current triangle */
  /*   has a smaller pointer than its neighbor.  This way, each edge is    */
  /*   considered only once.                                               */
  while (triangleloop.tri != (triangle *) NULL) {
    for (triangleloop.orient = 0; triangleloop.orient < 3;
         triangleloop.orient++) {
      sym(triangleloop, trisym);
      if ((triangleloop.tri < trisym.tri) || (trisym.tri == m->dummytri)) {
        org(triangleloop, p1);
        dest(triangleloop, p2);

        elist[index++] = vertexmark(p1);
        elist[index++] = vertexmark(p2);

        if (b->nobound) {

        } else {
          /* Edge number, indices of two endpoints, and a boundary marker. */
          /*   If there's no subsegment, the boundary marker is zero.      */
          if (b->usesegments) {
            tspivot(triangleloop, checkmark);
            if (checkmark.ss == m->dummysub) {

              emlist[edgenumber - b->firstnumber] = 0;

            } else {

              emlist[edgenumber - b->firstnumber] = mark(checkmark);

            }
          } else {

            emlist[edgenumber - b->firstnumber] = trisym.tri == m->dummytri;

          }
        }
        edgenumber++;
      }
    }
    triangleloop.tri = triangletraverse(m);
  }

}

void writevoronoi(struct mesh *m, struct behavior *b, sgFloat **vpointlist,
                  sgFloat **vpointattriblist, int **vpointmarkerlist,
                  int **vedgelist, int **vedgemarkerlist, sgFloat **vnormlist)
{
  sgFloat *plist;
  sgFloat *palist;
  int *elist;
  sgFloat *normlist;
  int coordindex;
  int attribindex;

  struct otri triangleloop, trisym;
  vertex torg, tdest, tapex;
  sgFloat circumcenter[2];
  sgFloat xi, eta;
  sgFloat dum;
  long vnodenumber, vedgenumber;
  int p1, p2;
  int i;
  triangle ptr;                         /* Temporary variable used by sym(). */

  if (!b->quiet) {
    //  TRACE("Writing Voronoi vertices.\n");
  }
  /* Allocate memory for Voronoi vertices if necessary. */
  if (*vpointlist == (sgFloat *) NULL) {
    *vpointlist = (sgFloat *) trimalloc(m->triangles.items * 2 * sizeof(sgFloat));
  }
  /* Allocate memory for Voronoi vertex attributes if necessary. */
  if (*vpointattriblist == (sgFloat *) NULL) {
    *vpointattriblist = (sgFloat *) trimalloc(m->triangles.items * m->nextras *
                                           sizeof(sgFloat));
  }
  *vpointmarkerlist = (int *) NULL;
  plist = *vpointlist;
  palist = *vpointattriblist;
  coordindex = 0;
  attribindex = 0;

  traversalinit(&m->triangles);
  triangleloop.tri = triangletraverse(m);
  triangleloop.orient = 0;
  vnodenumber = b->firstnumber;
  while (triangleloop.tri != (triangle *) NULL) {
    org(triangleloop, torg);
    dest(triangleloop, tdest);
    apex(triangleloop, tapex);
    findcircumcenter(m, b, torg, tdest, tapex, circumcenter, &xi, &eta, &dum,
                     0);

    /* X and y coordinates. */
    plist[coordindex++] = circumcenter[0];
    plist[coordindex++] = circumcenter[1];
    for (i = 2; i < 2 + m->nextras; i++) {
      /* Interpolate the vertex attributes at the circumcenter. */
      palist[attribindex++] = torg[i] + xi * (tdest[i] - torg[i])
                                     + eta * (tapex[i] - torg[i]);
    }


    * (int *) (triangleloop.tri + 6) = (int) vnodenumber;
    triangleloop.tri = triangletraverse(m);
    vnodenumber++;
  }

  if (!b->quiet) {
    //  TRACE("Writing Voronoi edges.\n");
  }
  /* Allocate memory for output Voronoi edges if necessary. */
  if (*vedgelist == (int *) NULL) {
    *vedgelist = (int *) trimalloc(m->edges * 2 * sizeof(int));
  }
  *vedgemarkerlist = (int *) NULL;
  /* Allocate memory for output Voronoi norms if necessary. */
  if (*vnormlist == (sgFloat *) NULL) {
    *vnormlist = (sgFloat *) trimalloc(m->edges * 2 * sizeof(sgFloat));
  }
  elist = *vedgelist;
  normlist = *vnormlist;
  coordindex = 0;

  traversalinit(&m->triangles);
  triangleloop.tri = triangletraverse(m);
  vedgenumber = b->firstnumber;
  /* To loop over the set of edges, loop over all triangles, and look at   */
  /*   the three edges of each triangle.  If there isn't another triangle  */
  /*   adjacent to the edge, operate on the edge.  If there is another     */
  /*   adjacent triangle, operate on the edge only if the current triangle */
  /*   has a smaller pointer than its neighbor.  This way, each edge is    */
  /*   considered only once.                                               */
  while (triangleloop.tri != (triangle *) NULL) {
    for (triangleloop.orient = 0; triangleloop.orient < 3;
         triangleloop.orient++) {
      sym(triangleloop, trisym);
      if ((triangleloop.tri < trisym.tri) || (trisym.tri == m->dummytri)) {
        /* Find the number of this triangle (and Voronoi vertex). */
        p1 = * (int *) (triangleloop.tri + 6);
        if (trisym.tri == m->dummytri) {
          org(triangleloop, torg);
          dest(triangleloop, tdest);
          /* Copy an infinite ray.  Index of one endpoint, and -1. */
          elist[coordindex] = p1;
          normlist[coordindex++] = tdest[1] - torg[1];
          elist[coordindex] = -1;
          normlist[coordindex++] = torg[0] - tdest[0];

        } else {
          /* Find the number of the adjacent triangle (and Voronoi vertex). */
          p2 = * (int *) (trisym.tri + 6);
          /* Finite edge.  Write indices of two endpoints. */
          elist[coordindex] = p1;
          normlist[coordindex++] = 0.0;
          elist[coordindex] = p2;
          normlist[coordindex++] = 0.0;

        }
        vedgenumber++;
      }
    }
    triangleloop.tri = triangletraverse(m);
  }

}

void writeneighbors(struct mesh *m, struct behavior *b, int **neighborlist)

{

  int *nlist;
  int index;

  struct otri triangleloop, trisym;
  long elementnumber;
  int neighbor1, neighbor2, neighbor3;
  triangle ptr;                         /* Temporary variable used by sym(). */

  if (!b->quiet) {
    //  TRACE("Writing neighbors.\n");
  }
  /* Allocate memory for neighbors if necessary. */
  if (*neighborlist == (int *) NULL) {
    *neighborlist = (int *) trimalloc(m->triangles.items * 3 * sizeof(int));
  }
  nlist = *neighborlist;
  index = 0;

  traversalinit(&m->triangles);
  triangleloop.tri = triangletraverse(m);
  triangleloop.orient = 0;
  elementnumber = b->firstnumber;
  while (triangleloop.tri != (triangle *) NULL) {
    * (int *) (triangleloop.tri + 6) = (int) elementnumber;
    triangleloop.tri = triangletraverse(m);
    elementnumber++;
  }
  * (int *) (m->dummytri + 6) = -1;

  traversalinit(&m->triangles);
  triangleloop.tri = triangletraverse(m);
  elementnumber = b->firstnumber;
  while (triangleloop.tri != (triangle *) NULL) {
    triangleloop.orient = 1;
    sym(triangleloop, trisym);
    neighbor1 = * (int *) (trisym.tri + 6);
    triangleloop.orient = 2;
    sym(triangleloop, trisym);
    neighbor2 = * (int *) (trisym.tri + 6);
    triangleloop.orient = 0;
    sym(triangleloop, trisym);
    neighbor3 = * (int *) (trisym.tri + 6);

    nlist[index++] = neighbor1;
    nlist[index++] = neighbor2;
    nlist[index++] = neighbor3;


    triangleloop.tri = triangletraverse(m);
    elementnumber++;
  }

}

void quality_statistics(struct mesh *m, struct behavior *b)
{
  struct otri triangleloop;
  vertex p[3];
  sgFloat cossquaretable[8];
  sgFloat ratiotable[16];
  sgFloat dx[3], dy[3];
  sgFloat edgelength[3];
  sgFloat dotproduct;
  sgFloat cossquare;
  sgFloat triarea;
  sgFloat shortest, longest;
  sgFloat trilongest2;
  sgFloat smallestarea, biggestarea;
  sgFloat triminaltitude2;
  sgFloat minaltitude;
  sgFloat triaspect2;
  sgFloat worstaspect;
  sgFloat smallestangle, biggestangle;
  sgFloat radconst, degconst;
  int angletable[18];
  int aspecttable[16];
  int aspectindex;
  int tendegree;
  int acutebiggest;
  int i, ii, j, k;

  //  TRACE("Mesh quality statistics:\n\n");
  radconst = M_PI / 18.0;
  degconst = 180.0 / M_PI;
  for (i = 0; i < 8; i++) {
    cossquaretable[i] = cos(radconst * (sgFloat) (i + 1));
    cossquaretable[i] = cossquaretable[i] * cossquaretable[i];
  }
  for (i = 0; i < 18; i++) {
    angletable[i] = 0;
  }

  ratiotable[0]  =      1.5;      ratiotable[1]  =     2.0;
  ratiotable[2]  =      2.5;      ratiotable[3]  =     3.0;
  ratiotable[4]  =      4.0;      ratiotable[5]  =     6.0;
  ratiotable[6]  =     10.0;      ratiotable[7]  =    15.0;
  ratiotable[8]  =     25.0;      ratiotable[9]  =    50.0;
  ratiotable[10] =    100.0;      ratiotable[11] =   300.0;
  ratiotable[12] =   1000.0;      ratiotable[13] = 10000.0;
  ratiotable[14] = 100000.0;      ratiotable[15] =     0.0;
  for (i = 0; i < 16; i++) {
    aspecttable[i] = 0;
  }

  worstaspect = 0.0;
  minaltitude = m->xmax - m->xmin + m->ymax - m->ymin;
  minaltitude = minaltitude * minaltitude;
  shortest = minaltitude;
  longest = 0.0;
  smallestarea = minaltitude;
  biggestarea = 0.0;
  worstaspect = 0.0;
  smallestangle = 0.0;
  biggestangle = 2.0;
  acutebiggest = 1;

  traversalinit(&m->triangles);
  triangleloop.tri = triangletraverse(m);
  triangleloop.orient = 0;
  while (triangleloop.tri != (triangle *) NULL) {
    org(triangleloop, p[0]);
    dest(triangleloop, p[1]);
    apex(triangleloop, p[2]);
    trilongest2 = 0.0;

    for (i = 0; i < 3; i++) {
      j = plus1mod3[i];
      k = minus1mod3[i];
      dx[i] = p[j][0] - p[k][0];
      dy[i] = p[j][1] - p[k][1];
      edgelength[i] = dx[i] * dx[i] + dy[i] * dy[i];
      if (edgelength[i] > trilongest2) {
        trilongest2 = edgelength[i];
      }
      if (edgelength[i] > longest) {
        longest = edgelength[i];
      }
      if (edgelength[i] < shortest) {
        shortest = edgelength[i];
      }
    }

    triarea = counterclockwise(m, b, p[0], p[1], p[2]);
    if (triarea < smallestarea) {
      smallestarea = triarea;
    }
    if (triarea > biggestarea) {
      biggestarea = triarea;
    }
    triminaltitude2 = triarea * triarea / trilongest2;
    if (triminaltitude2 < minaltitude) {
      minaltitude = triminaltitude2;
    }
    triaspect2 = trilongest2 / triminaltitude2;
    if (triaspect2 > worstaspect) {
      worstaspect = triaspect2;
    }
    aspectindex = 0;
    while ((triaspect2 > ratiotable[aspectindex] * ratiotable[aspectindex])
           && (aspectindex < 15)) {
      aspectindex++;
    }
    aspecttable[aspectindex]++;

    for (i = 0; i < 3; i++) {
      j = plus1mod3[i];
      k = minus1mod3[i];
      dotproduct = dx[j] * dx[k] + dy[j] * dy[k];
      cossquare = dotproduct * dotproduct / (edgelength[j] * edgelength[k]);
      tendegree = 8;
      for (ii = 7; ii >= 0; ii--) {
        if (cossquare > cossquaretable[ii]) {
          tendegree = ii;
        }
      }
      if (dotproduct <= 0.0) {
        angletable[tendegree]++;
        if (cossquare > smallestangle) {
          smallestangle = cossquare;
        }
        if (acutebiggest && (cossquare < biggestangle)) {
          biggestangle = cossquare;
        }
      } else {
        angletable[17 - tendegree]++;
        if (acutebiggest || (cossquare > biggestangle)) {
          biggestangle = cossquare;
          acutebiggest = 0;
        }
      }
    }
    triangleloop.tri = triangletraverse(m);
  }

  shortest = sqrt(shortest);
  longest = sqrt(longest);
  minaltitude = sqrt(minaltitude);
  worstaspect = sqrt(worstaspect);
  smallestarea *= 0.5;
  biggestarea *= 0.5;
  if (smallestangle >= 1.0) {
    smallestangle = 0.0;
  } else {
    smallestangle = degconst * acos(sqrt(smallestangle));
  }
  if (biggestangle >= 1.0) {
    biggestangle = 180.0;
  } else {
    if (acutebiggest) {
      biggestangle = degconst * acos(sqrt(biggestangle));
    } else {
      biggestangle = 180.0 - degconst * acos(sqrt(biggestangle));
    }
  }

  //  TRACE("  Smallest area: %16.5g   |  Largest area: %16.5g\n",
//         smallestarea, biggestarea);
  //  TRACE("  Shortest edge: %16.5g   |  Longest edge: %16.5g\n",
    //     shortest, longest);
  //  TRACE("  Shortest altitude: %12.5g   |  Largest aspect ratio: %8.5g\n\n",
    //     minaltitude, worstaspect);

  //  TRACE("  Triangle aspect ratio histogram:\n");
  //  TRACE("  1.1547 - %-6.6g    :  %8d    | %6.6g - %-6.6g     :  %8d\n",
    //     ratiotable[0], aspecttable[0], ratiotable[7], ratiotable[8],
    //     aspecttable[8]);
  for (i = 1; i < 7; i++) {
    //  TRACE("  %6.6g - %-6.6g    :  %8d    | %6.6g - %-6.6g     :  %8d\n",
        //   ratiotable[i - 1], ratiotable[i], aspecttable[i],
        //   ratiotable[i + 7], ratiotable[i + 8], aspecttable[i + 8]);
  }
  //  TRACE("  %6.6g - %-6.6g    :  %8d    | %6.6g -            :  %8d\n",
      //   ratiotable[6], ratiotable[7], aspecttable[7], ratiotable[14],
       //  aspecttable[15]);
  //  TRACE("  (Aspect ratio is longest edge divided by shortest altitude)\n\n");

  //  TRACE("  Smallest angle: %15.5g   |  Largest angle: %15.5g\n\n",
        // smallestangle, biggestangle);

  //  TRACE("  Angle histogram:\n");
  for (i = 0; i < 9; i++) {
    //  TRACE("    %3d - %3d degrees:  %8d    |    %3d - %3d degrees:  %8d\n",
         //  i * 10, i * 10 + 10, angletable[i],
         //  i * 10 + 90, i * 10 + 100, angletable[i + 9]);
  }
  //  TRACE("\n");
}

void statistics(struct mesh *m, struct behavior *b)
{
  //  TRACE("\nStatistics:\n\n");
  //  TRACE("  Input vertices: %d\n", m->invertices);
  if (b->refine) {
    //  TRACE("  Input triangles: %d\n", m->inelements);
  }
  if (b->poly) {
    //  TRACE("  Input segments: %d\n", m->insegments);
    if (!b->refine) {
      //  TRACE("  Input holes: %d\n", m->holes);
    }
  }

  //  TRACE("\n  Mesh vertices: %ld\n", m->vertices.items - m->undeads);
  //  TRACE("  Mesh triangles: %ld\n", m->triangles.items);
  //  TRACE("  Mesh edges: %ld\n", m->edges);
  //  TRACE("  Mesh exterior boundary edges: %ld\n", m->hullsize);
  if (b->poly || b->refine) {
    //  TRACE("  Mesh interior boundary edges: %ld\n",
          // m->subsegs.items - m->hullsize);
    //  TRACE("  Mesh subsegments (constrained edges): %ld\n",
        //   m->subsegs.items);
  }
  //  TRACE("\n");

  if (b->verbose) {
    quality_statistics(m, b);
    //  TRACE("Memory allocation statistics:\n\n");
    //  TRACE("  Maximum number of vertices: %ld\n", m->vertices.maxitems);
    //  TRACE("  Maximum number of triangles: %ld\n", m->triangles.maxitems);
    if (m->subsegs.maxitems > 0) {
      //  TRACE("  Maximum number of subsegments: %ld\n", m->subsegs.maxitems);
    }
    if (m->viri.maxitems > 0) {
      //  TRACE("  Maximum number of viri: %ld\n", m->viri.maxitems);
    }
    if (m->badsubsegs.maxitems > 0) {
      //  TRACE("  Maximum number of encroached subsegments: %ld\n",
         //    m->badsubsegs.maxitems);
    }
    if (m->badtriangles.maxitems > 0) {
      //  TRACE("  Maximum number of bad triangles: %ld\n",
          //   m->badtriangles.maxitems);
    }
    if (m->flipstackers.maxitems > 0) {
      //  TRACE("  Maximum number of stacked triangle flips: %ld\n",
           //  m->flipstackers.maxitems);
    }
    if (m->splaynodes.maxitems > 0) {
      //  TRACE("  Maximum number of splay tree nodes: %ld\n",
          //   m->splaynodes.maxitems);
    }
    //  TRACE("  Approximate heap memory use (bytes): %ld\n\n",
         //  m->vertices.maxitems * m->vertices.itembytes +
         //  m->triangles.maxitems * m->triangles.itembytes +
         //  m->subsegs.maxitems * m->subsegs.itembytes +
         //  m->viri.maxitems * m->viri.itembytes +
         //  m->badsubsegs.maxitems * m->badsubsegs.itembytes +
         //  m->badtriangles.maxitems * m->badtriangles.itembytes +
         //  m->flipstackers.maxitems * m->flipstackers.itembytes +
         //  m->splaynodes.maxitems * m->splaynodes.itembytes);

    //  TRACE("Algorithmic statistics:\n\n");
    if (!b->weighted) {
      //  TRACE("  Number of incircle tests: %ld\n", m->incirclecount);
    } else {
      //  TRACE("  Number of 3D orientation tests: %ld\n", m->orient3dcount);
    }
    //  TRACE("  Number of 2D orientation tests: %ld\n", m->counterclockcount);
    if (m->hyperbolacount > 0) {
      //  TRACE("  Number of right-of-hyperbola tests: %ld\n",
           //  m->hyperbolacount);
    }
    if (m->circletopcount > 0) {
      //  TRACE("  Number of circle top computations: %ld\n",
           //  m->circletopcount);
    }
    if (m->circumcentercount > 0) {
      //  TRACE("  Number of triangle circumcenter computations: %ld\n",
          //   m->circumcentercount);
    }
    //  TRACE("\n");
  }
}

void triangulate(char *triswitches, struct triangulateio *in,
                 struct triangulateio *out, struct triangulateio *vorout)
{
  struct mesh m;
  struct behavior b;
  sgFloat *holearray;                                        /* Array of holes. */
  sgFloat *regionarray;   /* Array of regional attributes and area constraints. */
  /* Variables for timing the performance of Triangle.  The types are */
  /*   defined in sys/time.h.                                         */
    triangleinit(&m);

  parsecommandline(1, &triswitches, &b);
  m.steinerleft = b.steiner;

  transfernodes(&m, &b, in->pointlist, in->pointattributelist,
                in->pointmarkerlist, in->numberofpoints,
                in->numberofpointattributes);

#ifdef CDT_ONLY
  m.hullsize = delaunay(&m, &b);                /* Triangulate the vertices. */
#else /* not CDT_ONLY */
  if (b.refine) {
    /* Read and reconstruct a mesh. */
    m.hullsize = reconstruct(&m, &b, in->trianglelist,
                             in->triangleattributelist, in->trianglearealist,
                             in->numberoftriangles, in->numberofcorners,
                             in->numberoftriangleattributes,
                             in->segmentlist, in->segmentmarkerlist,
                             in->numberofsegments);  
  } else {
    m.hullsize = delaunay(&m, &b);              /* Triangulate the vertices. */
  }
#endif /* not CDT_ONLY */


  /* Ensure that no vertex can be mistaken for a triangular bounding */
  /*   box vertex in insertvertex().                                 */
  m.infvertex1 = (vertex) NULL;
  m.infvertex2 = (vertex) NULL;
  m.infvertex3 = (vertex) NULL;

  if (b.usesegments) {
    m.checksegments = 1;                /* Segments will be introduced next. */
    if (!b.refine) {
      /* Insert PSLG segments and/or convex hull segments. */
      formskeleton(&m, &b, in->segmentlist,
                   in->segmentmarkerlist, in->numberofsegments);

    }
  }

  if (b.poly && (m.triangles.items > 0)) {

    holearray = in->holelist;
    m.holes = in->numberofholes;
    regionarray = in->regionlist;
    m.regions = in->numberofregions;

    if (!b.refine) {
      /* Carve out holes and concavities. */
      carveholes(&m, &b, holearray, m.holes, regionarray, m.regions);
    }
  } else {
    /* Without a PSLG, there can be no holes or regional attributes   */
    /*   or area constraints.  The following are set to zero to avoid */
    /*   an accidental free() later.                                  */
    m.holes = 0;
    m.regions = 0;
  }

#ifndef CDT_ONLY
  if (b.quality && (m.triangles.items > 0)) {
    enforcequality(&m, &b);           /* Enforce angle and area constraints. */
  }
#endif /* not CDT_ONLY */

  /* Calculate the number of edges. */
  m.edges = (3l * m.triangles.items + m.hullsize) / 2l;

  if (b.order > 1) {
    highorder(&m, &b);       /* Promote elements to higher polynomial order. */
  }
  if (!b.quiet) {
    //  TRACE("\n");
  }

  out->numberofpoints = m.vertices.items;
  out->numberofpointattributes = m.nextras;
  out->numberoftriangles = m.triangles.items;
  out->numberofcorners = (b.order + 1) * (b.order + 2) / 2;
  out->numberoftriangleattributes = m.eextras;
  out->numberofedges = m.edges;
  if (b.usesegments) {
    out->numberofsegments = m.subsegs.items;
  } else {
    out->numberofsegments = m.hullsize;
  }
  if (vorout != (struct triangulateio *) NULL) {
    vorout->numberofpoints = m.triangles.items;
    vorout->numberofpointattributes = m.nextras;
    vorout->numberofedges = m.edges;
  }

  /* If not using iteration numbers, don't write a .node file if one was */
  /*   read, because the original one would be overwritten!              */
  if (b.nonodewritten || (b.noiterationnum && m.readnodefile)) {
    if (!b.quiet) {

      //  TRACE("NOT writing vertices.\n");

    }
    numbernodes(&m, &b);         /* We must remember to number the vertices. */
  } else {
    /* writenodes() numbers the vertices too. */

    writenodes(&m, &b, &out->pointlist, &out->pointattributelist,
               &out->pointmarkerlist);

  }
  if (b.noelewritten) {
    if (!b.quiet) {

      //  TRACE("NOT writing triangles.\n");

    }
  } else {

    writeelements(&m, &b, &out->trianglelist, &out->triangleattributelist);

  }
  /* The -c switch (convex switch) causes a PSLG to be written */
  /*   even if none was read.                                  */
  if (b.poly || b.convex) 
  {
    /* If not using iteration numbers, don't overwrite the .poly file. */
    if (b.nopolywritten || b.noiterationnum) {
      if (!b.quiet) {

        //  TRACE("NOT writing segments.\n");

      }
    } else {

      writepoly(&m, &b, &out->segmentlist, &out->segmentmarkerlist);
      out->numberofholes = m.holes;
      out->numberofregions = m.regions;
      if (b.poly) {
        out->holelist = in->holelist;
        out->regionlist = in->regionlist;
      } else {
        out->holelist = (sgFloat *) NULL;
        out->regionlist = (sgFloat *) NULL;
      }

    }
  }

  if (b.edgesout) {

    writeedges(&m, &b, &out->edgelist, &out->edgemarkerlist);

  }
  if (b.voronoi) {

    writevoronoi(&m, &b, &vorout->pointlist, &vorout->pointattributelist,
                 &vorout->pointmarkerlist, &vorout->edgelist,
                 &vorout->edgemarkerlist, &vorout->normlist);

  }
  if (b.neighbors) {

    writeneighbors(&m, &b, &out->neighborlist);

  }

  if (!b.quiet) {
    statistics(&m, &b);
  }
  
  if (b.docheck) {
    checkmesh(&m, &b);
    checkdelaunay(&m, &b);
  }


  triangledeinit(&m, &b);

}



void reportTr(struct triangulateio *io,
int markers,
int reporttriangles,
int reportneighbors,
int reportsegments,
int reportedges,
int reportnorms)
{
	int i, j;

	for (i = 0; i < io->numberofpoints; i++) {
		//  TRACE("Point %4d:", i);
		for (j = 0; j < 2; j++) {
			//  TRACE("  %.6g", io->pointlist[i * 2 + j]);
		}
		if (io->numberofpointattributes > 0) {
			//  TRACE("   attributes");
		}
		for (j = 0; j < io->numberofpointattributes; j++) {
			//  TRACE("  %.6g",
//				io->pointattributelist[i * io->numberofpointattributes + j]);
		}
		if (markers) {
			//  TRACE("   marker %d\n", io->pointmarkerlist[i]);
		} else {
			//  TRACE("\n");
		}
	}
	//  TRACE("\n");

	if (reporttriangles || reportneighbors) {
		for (i = 0; i < io->numberoftriangles; i++) {
			if (reporttriangles) {
				//  TRACE("Triangle %4d points:", i);
				for (j = 0; j < io->numberofcorners; j++) {
					//  TRACE("  %4d", io->trianglelist[i * io->numberofcorners + j]);
				}
				if (io->numberoftriangleattributes > 0) {
					//  TRACE("   attributes");
				}
				for (j = 0; j < io->numberoftriangleattributes; j++) {
					//  TRACE("  %.6g", io->triangleattributelist[i *
						//io->numberoftriangleattributes + j]);
				}
				//  TRACE("\n");
			}
			if (reportneighbors) {
				//  TRACE("Triangle %4d neighbors:", i);
				for (j = 0; j < 3; j++) {
					//  TRACE("  %4d", io->neighborlist[i * 3 + j]);
				}
				//  TRACE("\n");
			}
		}
		//  TRACE("\n");
	}

	if (reportsegments) {
		for (i = 0; i < io->numberofsegments; i++) {
			//  TRACE("Segment %4d points:", i);
			for (j = 0; j < 2; j++) {
				//  TRACE("  %4d", io->segmentlist[i * 2 + j]);
			}
			if (markers) {
				//  TRACE("   marker %d\n", io->segmentmarkerlist[i]);
			} else {
				//  TRACE("\n");
			}
		}
		//  TRACE("\n");
	}

	if (reportedges) {
		for (i = 0; i < io->numberofedges; i++) {
			//  TRACE("Edge %4d points:", i);
			for (j = 0; j < 2; j++) {
				//  TRACE("  %4d", io->edgelist[i * 2 + j]);
			}
			if (reportnorms && (io->edgelist[i * 2 + 1] == -1)) {
				for (j = 0; j < 2; j++) {
					//  TRACE("  %.6g", io->normlist[i * 2 + j]);
				}
			}
			if (markers) {
				//  TRACE("   marker %d\n", io->edgemarkerlist[i]);
			} else {
				//  TRACE("\n");
			}
		}
		//  TRACE("\n");
	}
}


#pragma warning(default:4311)
#pragma warning(default:4312)








CTriangulator::CTriangulator()
{
	m_in = malloc(sizeof(struct triangulateio));
	m_out = malloc(sizeof(struct triangulateio));
	memset(m_in,0,sizeof(struct triangulateio));
	memset(m_out,0,sizeof(struct triangulateio));
}

CTriangulator::CTriangulator(unsigned int points_count)
{
	m_in = malloc(sizeof(struct triangulateio));
	m_out = malloc(sizeof(struct triangulateio));
	memset(m_in,0,sizeof(struct triangulateio));
	memset(m_out,0,sizeof(struct triangulateio));

	struct triangulateio*  in_data = reinterpret_cast<struct triangulateio*>(m_in);
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	in_data->numberofpoints = points_count;
	in_data->numberofpointattributes = 1;
	in_data->pointlist = (sgFloat *)malloc(in_data->numberofpoints * 2 * sizeof(sgFloat));
	memset(in_data->pointlist,0,in_data->numberofpoints * 2 * sizeof(sgFloat));
	in_data->pointmarkerlist = (int *)malloc(in_data->numberofpoints * sizeof(int));
	memset(in_data->pointmarkerlist,0,in_data->numberofpoints * sizeof(int));
	in_data->pointattributelist = (sgFloat *)malloc(in_data->numberofpoints *
															in_data->numberofpointattributes *
															sizeof(sgFloat));
	memset(in_data->pointattributelist,0,in_data->numberofpoints *
															in_data->numberofpointattributes *
															sizeof(sgFloat));
}

CTriangulator::CTriangulator(unsigned int points_count, const sgFloat* all_points)
{
	m_in = malloc(sizeof(struct triangulateio));
	m_out = malloc(sizeof(struct triangulateio));
	memset(m_in,0,sizeof(struct triangulateio));
	memset(m_out,0,sizeof(struct triangulateio));

	struct triangulateio*  in_data = reinterpret_cast<struct triangulateio*>(m_in);
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	in_data->numberofpoints = points_count;
	in_data->numberofpointattributes = 1;
	in_data->pointlist = (sgFloat *)malloc(in_data->numberofpoints * 2 * sizeof(sgFloat));
	memcpy(in_data->pointlist,all_points,in_data->numberofpoints * 2 * sizeof(sgFloat));
	in_data->pointmarkerlist = (int *)malloc(in_data->numberofpoints * sizeof(int));
	memset(in_data->pointmarkerlist,0,in_data->numberofpoints * sizeof(int));
	in_data->pointattributelist = (sgFloat *)malloc(in_data->numberofpoints *
		in_data->numberofpointattributes *
		sizeof(sgFloat));
	memset(in_data->pointattributelist,0,in_data->numberofpoints *
		in_data->numberofpointattributes *
		sizeof(sgFloat));
}

CTriangulator::~CTriangulator()
{
	ClearAll();
	free(m_in);
	free(m_out);
}

static   void ClearStructure(struct triangulateio* trio)
{
	if (trio->pointlist)
		free(trio->pointlist);
	if (trio->pointattributelist)
		free(trio->pointattributelist);
	if (trio->pointmarkerlist)
		free(trio->pointmarkerlist);
	if (trio->trianglelist)
		free(trio->trianglelist);
	if (trio->triangleattributelist)
		free(trio->triangleattributelist);
	if (trio->trianglearealist)
		free(trio->trianglearealist);
	if (trio->neighborlist)
		free(trio->neighborlist);
	if (trio->segmentlist)
		free(trio->segmentlist);
	if (trio->segmentmarkerlist)
		free(trio->segmentmarkerlist);
	if (trio->holelist)
		free(trio->holelist);
	if (trio->regionlist)
		free(trio->regionlist);
	if (trio->edgelist)
		free(trio->edgelist);
	if (trio->edgemarkerlist)
		free(trio->edgemarkerlist);
	if (trio->normlist)
		free(trio->normlist);
}

bool CTriangulator::ClearAll()
{
	struct triangulateio*  in_data = reinterpret_cast<struct triangulateio*>(m_in);
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	ClearStructure(in_data);
	memset(m_in,0,sizeof(struct triangulateio));

	ClearStructure(out_data);
	memset(m_out,0,sizeof(struct triangulateio));

	return true;
}

bool CTriangulator::ClearNotAll()
{
	struct triangulateio*  in_data = reinterpret_cast<struct triangulateio*>(m_in);
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	ClearStructure(in_data);
	memset(m_in,0,sizeof(struct triangulateio));

	if (out_data->triangleattributelist)
		free(out_data->triangleattributelist);
	if (out_data->trianglearealist)
		free(out_data->trianglearealist);
	if (out_data->neighborlist)
		free(out_data->neighborlist);
	if (out_data->segmentlist)
		free(out_data->segmentlist);
	if (out_data->segmentmarkerlist)
		free(out_data->segmentmarkerlist);
	if (out_data->holelist)
		free(out_data->holelist);
	if (out_data->regionlist)
		free(out_data->regionlist);
	if (out_data->edgelist)
		free(out_data->edgelist);
	if (out_data->edgemarkerlist)
		free(out_data->edgemarkerlist);
	if (out_data->normlist)
		free(out_data->normlist);

	out_data->triangleattributelist = NULL;
	out_data->trianglearealist = NULL;
	out_data->neighborlist = NULL;
	out_data->segmentlist = NULL;
	out_data->segmentmarkerlist = NULL;
	out_data->holelist = NULL;
	out_data->regionlist = NULL;
	out_data->edgelist = NULL;
	out_data->edgemarkerlist = NULL;
	out_data->normlist = NULL;
	
	return true;
}

bool    CTriangulator::SetPointsCount(unsigned int points_count)
{
	if (points_count<3)
		return false;

	ClearAll();

	struct triangulateio*  in_data = reinterpret_cast<struct triangulateio*>(m_in);
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	in_data->numberofpoints = points_count;
	in_data->numberofpointattributes = 6;
	in_data->pointlist = (sgFloat *)malloc(in_data->numberofpoints * 2 * sizeof(sgFloat));
	memset(in_data->pointlist,0,in_data->numberofpoints * 2 * sizeof(sgFloat));
	in_data->pointmarkerlist = (int *)malloc(in_data->numberofpoints * sizeof(int));
	memset(in_data->pointmarkerlist,0,in_data->numberofpoints * sizeof(int));
	in_data->pointattributelist = (sgFloat *)malloc(in_data->numberofpoints *
		in_data->numberofpointattributes *
		sizeof(sgFloat));
	memset(in_data->pointattributelist,0,in_data->numberofpoints *
		in_data->numberofpointattributes *
		sizeof(sgFloat));

	return true;
}

bool    CTriangulator::SetPoint(unsigned int point_index, sgFloat pX, sgFloat pY,
								sgFloat point_attr_1/*=0.0*/, 
								sgFloat point_attr_2/*=0.0*/, 
								sgFloat point_attr_3/*=0.0*/, 
								int point_mark/*=0*/)
{
	struct triangulateio*  in_data = reinterpret_cast<struct triangulateio*>(m_in);
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	if (in_data==NULL || in_data->pointlist==NULL || point_index>=(unsigned int)in_data->numberofpoints ||
		in_data->pointmarkerlist==NULL || in_data->pointattributelist==NULL)
		return false;

	in_data->pointlist[2*point_index] = pX;
	in_data->pointlist[2*point_index+1] = pY;
	in_data->pointmarkerlist[point_index] = point_mark;
	in_data->pointattributelist[in_data->numberofpointattributes*point_index] = point_attr_1;
	in_data->pointattributelist[in_data->numberofpointattributes*point_index+1] = point_attr_1;
	in_data->pointattributelist[in_data->numberofpointattributes*point_index+2] = point_attr_1;
	return true;
}

bool    CTriangulator::SetAllPoints(unsigned int points_count, const sgFloat* all_points,
									const sgFloat* points_attrs/*=NULL*/, 
									const int* points_marks/*=NULL*/)
{
	if (!SetPointsCount(points_count))
		return false;

	struct triangulateio*  in_data = reinterpret_cast<struct triangulateio*>(m_in);
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	if (in_data==NULL || in_data->pointlist==NULL || all_points==NULL ||
		in_data->pointmarkerlist==NULL || in_data->pointattributelist==NULL)
		return false;

	memcpy(in_data->pointlist,all_points,in_data->numberofpoints * 2 * sizeof(sgFloat));
	if (points_marks)
		memcpy(in_data->pointmarkerlist,points_marks,in_data->numberofpoints * sizeof(int));
	if (points_attrs)
		memcpy(in_data->pointattributelist,points_attrs,in_data->numberofpoints*
		in_data->numberofpointattributes*sizeof(sgFloat));

	return true;
}

void  CTriangulator::CorrectPointsCollisions()
{
	struct triangulateio*  in_data = reinterpret_cast<struct triangulateio*>(m_in);
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	if (!in_data || in_data->numberofpoints<1 || !in_data->pointlist)
		return;

	SG_POINT resP;

	for (int j=0;j<2*(in_data->numberofpoints-1);j+=2)
		for (int i=j+4;i<2*(in_data->numberofpoints-1);i+=2)
		{
			if (fabs(in_data->pointlist[i]-in_data->pointlist[j])<FLT_MIN &&
				fabs(in_data->pointlist[i+1]-in_data->pointlist[j+1])<FLT_MIN)
			{
				sgFloat dir_to_prev[2];
				dir_to_prev[0] = in_data->pointlist[i-2]-in_data->pointlist[i];
				dir_to_prev[1] = in_data->pointlist[i-1]-in_data->pointlist[i+1];
				sgFloat vec_dist = sqrt(dir_to_prev[0]*dir_to_prev[0]+dir_to_prev[1]*dir_to_prev[1]);
				if (vec_dist>FLT_MIN)
				{
					if (vec_dist>1.0)
					{
						dir_to_prev[0]/=vec_dist;
						dir_to_prev[1]/=vec_dist;
					}
					in_data->pointlist[i]+=dir_to_prev[0]*0.000001;
					in_data->pointlist[i+1]+=dir_to_prev[1]*0.000001;
				}
			}
		}
}

bool CTriangulator::SetSegmentsCount(unsigned int segments_count)
{
	if (segments_count==0)
		return false;

	struct triangulateio*  in_data = reinterpret_cast<struct triangulateio*>(m_in);
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	ClearStructure(out_data);
	memset(m_out,0,sizeof(struct triangulateio));

	if (in_data->segmentlist)
		free(in_data->segmentlist);
	if (in_data->segmentmarkerlist)
		free(in_data->segmentmarkerlist);

	in_data->numberofsegments = segments_count;
	in_data->segmentlist = (int*)malloc(in_data->numberofsegments*2*sizeof(int));
	memset(in_data->segmentlist,0,in_data->numberofsegments*2*sizeof(int));

	in_data->segmentmarkerlist = (int *) malloc(in_data->numberofsegments * sizeof(int));
	memset(in_data->segmentmarkerlist,0,in_data->numberofsegments * sizeof(int));

	return true;
}

bool CTriangulator::SetSegment(unsigned int segment_index, int start_ind, int end_ind,
							   int segment_mark/*=0*/)
{
	struct triangulateio*  in_data = reinterpret_cast<struct triangulateio*>(m_in);
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	if (in_data==NULL || in_data->pointlist==NULL || in_data->segmentlist==NULL || 
		segment_index>=(unsigned int)in_data->numberofsegments ||
		start_ind>=in_data->numberofpoints ||
		end_ind>=in_data->numberofpoints ||
		in_data->segmentmarkerlist==NULL)
		return false;

	in_data->segmentlist[2*segment_index] = start_ind;
	in_data->segmentlist[2*segment_index+1] = end_ind;
	in_data->segmentmarkerlist[segment_index] = segment_mark;

	return true;
}

bool CTriangulator::SetAllSegments(unsigned int segments_count, const int* all_segments,
								   const int* segments_marks/*=NULL*/)
{
	if (!SetSegmentsCount(segments_count))
		return false;

	struct triangulateio*  in_data = reinterpret_cast<struct triangulateio*>(m_in);
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	if (in_data==NULL || in_data->pointlist==NULL || 
		in_data->segmentlist==NULL || all_segments==NULL ||
		in_data->numberofsegments!=segments_count ||
		in_data->segmentmarkerlist==NULL)
		return false;

	memcpy(in_data->segmentlist,all_segments,in_data->numberofsegments*2*sizeof(int));
	if (segments_marks)
		memcpy(in_data->segmentmarkerlist,segments_marks,in_data->numberofsegments*sizeof(int));

	return true;
}

bool  CTriangulator::SetHolesCount(unsigned int holes_count)
{
	if (holes_count==0)
		return false;

	struct triangulateio*  in_data = reinterpret_cast<struct triangulateio*>(m_in);
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	ClearStructure(out_data);
	memset(m_out,0,sizeof(struct triangulateio));

	if (in_data->holelist)
		free(in_data->holelist);
	
	in_data->numberofholes = holes_count;
	in_data->holelist = (sgFloat*)malloc(in_data->numberofholes * 2 * sizeof(sgFloat));
	memset(in_data->holelist,0,in_data->numberofholes * 2 * sizeof(sgFloat));

	return true;
}

bool  CTriangulator::SetHole(unsigned int hole_index, sgFloat pX, sgFloat pY)
{
	struct triangulateio*  in_data = reinterpret_cast<struct triangulateio*>(m_in);
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	if (in_data==NULL || in_data->pointlist==NULL || in_data->segmentlist==NULL || 
		in_data->holelist==NULL ||
		hole_index>=(unsigned int)in_data->numberofholes)
			return false;

	in_data->holelist[2*hole_index] = pX;
	in_data->holelist[2*hole_index+1] = pY;

	return true;
}

bool  CTriangulator::SetAllHoles(unsigned int holes_count, const sgFloat* all_holes)
{
	if (!SetHolesCount(holes_count))
		return false;

	struct triangulateio*  in_data = reinterpret_cast<struct triangulateio*>(m_in);
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	if (in_data==NULL || in_data->pointlist==NULL || 
		in_data->holelist==NULL || all_holes==NULL ||
		in_data->numberofholes!=holes_count)
		return false;

	memcpy(in_data->holelist,all_holes,in_data->numberofholes * 2 * sizeof(sgFloat));

	return true;
}




bool   CTriangulator::StartTriangulate()
{
	struct triangulateio*  in_data = reinterpret_cast<struct triangulateio*>(m_in);
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	if (in_data==NULL || 
		in_data->numberofpoints==0 ||
		in_data->numberofpointattributes==0 ||
		in_data->pointlist==NULL ||
		in_data->pointmarkerlist==NULL ||
		in_data->pointattributelist==NULL ||
		in_data->numberofsegments==0 ||
		in_data->segmentlist==NULL ||
		in_data->segmentmarkerlist==NULL)
			return false;

	try
	{
        // ROBLOX CHANGE
		if (triangulation_temp_flag==SG_DELAUNAY_TRIANGULATION)
			triangulate("pzqPQ", in_data, out_data, (struct triangulateio *) NULL);//Delone
		else
		    triangulate("pzPQ", in_data, out_data, (struct triangulateio *) NULL);
        // END ROBLOX CHANGE
	}
	catch (...) 
	{
		return false;
	}
	return true;
}

bool  CTriangulator::GetPointsCount(unsigned int& points_count) const
{
	struct triangulateio*  in_data = reinterpret_cast<struct triangulateio*>(m_in);
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	if (out_data==NULL || out_data->pointlist==NULL)
	{
		points_count=0;
		return false;
	}
	points_count =out_data->numberofpoints;
	return true;
}

bool  CTriangulator::GetPoint(unsigned int point_index, sgFloat& pX, sgFloat& pY) const
{
	struct triangulateio*  in_data = reinterpret_cast<struct triangulateio*>(m_in);
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	if (out_data==NULL || out_data->pointlist==NULL || point_index>=(unsigned int)out_data->numberofpoints)
	{
		pX=0;
		pY=0;
		return false;
	}
	pX=out_data->pointlist[2*point_index];
	pY=out_data->pointlist[2*point_index+1];
	return true;
}

bool  CTriangulator::GetTrianglesCount(unsigned int& tr_count) const
{
	struct triangulateio*  in_data = reinterpret_cast<struct triangulateio*>(m_in);
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	if (out_data==NULL || out_data->pointlist==NULL || out_data->trianglelist==NULL)
	{
		tr_count=0;
		return false;
	}
	tr_count =out_data->numberoftriangles;
	return true;
}

bool  CTriangulator::GetTriangle(unsigned int tr_index, int& ver1, int& ver2, int& ver3) const
{
	struct triangulateio*  in_data = reinterpret_cast<struct triangulateio*>(m_in);
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	if (out_data==NULL || out_data->pointlist==NULL || 
		tr_index>=(unsigned int)out_data->numberoftriangles ||
		out_data->trianglelist==NULL)
	{
		ver1=0;
		ver2=0;
		ver3=0;
		return false;
	}
	ver1=out_data->trianglelist[tr_index * out_data->numberofcorners];
	ver2=out_data->trianglelist[tr_index * out_data->numberofcorners+1];
	ver3=out_data->trianglelist[tr_index * out_data->numberofcorners+2];
	return true;
}

const sgFloat* CTriangulator::GetPointsAttributes() const
{
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	if (out_data==NULL)
		return NULL;
	return out_data->pointattributelist;
}

const int* CTriangulator::GetPointsMarks() const
{
	struct triangulateio*  out_data = reinterpret_cast<struct triangulateio*>(m_out);

	if (out_data==NULL)
		return NULL;
	return out_data->pointmarkerlist;
}


