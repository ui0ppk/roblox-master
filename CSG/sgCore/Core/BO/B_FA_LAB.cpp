#include "../sg.h"

short b_fa_lab1(lpNPW np)
{
  NP_BIT_FACE *bf;
  BO_BIT_EDGE *be;
  short pr,mt,i;
  short fedge,edge;
	short loop,face;

  i=0;                      
  for (face=1 ; face <= np->nof ; face++) {
    bf = (NP_BIT_FACE*)&np->f[face].fl;
		bf->flg = 0;
		pr = 0;                     
		loop = np->f[face].fc;
		do {
			edge = fedge = np->c[loop].fe;
			do {
                be = (BO_BIT_EDGE*)&np->efr[(int)abs(edge)].el;
				if (be->flg != 0) {
					if (edge > 0)  mt = be->ep;
					else           mt = be->em;
					if (pr == 0) {
						pr = 1;
						bf->met = b_face_label(mt);   
						bf->flg = 1;
						i++;
					}
					else {
						mt = b_face_label(mt);
						if (mt != bf->met) {
							put_message(LAB_ERR,NULL,0);
							return (0);
						}
					}
				}
				edge = SL(edge,np);
			} while (fedge != edge);
			loop = np->c[loop].nc;
		} while (loop > 0);
	}
	return (i);
}

BOOL b_fa_lab2(lpNPW np)
{
	NP_BIT_FACE *bf,*bt;
	short edge,fedge,loop;
	short face,sface;            
	short i,ipr;

	ipr = 0;
	do {
		i = 0;
		for (face = 1 ; face <= np->nof ; face++) {
			bf = (NP_BIT_FACE*)&np->f[face].fl;
			if (bf->flg == 1) {
				i++;   continue;            
			}
			loop = np->f[face].fc;
			do {
				edge = fedge = np->c[loop].fe;
				do {
					sface = SF(edge,np);
					if (sface > 0) {         
						bt = (NP_BIT_FACE*)&np->f[sface].fl;
						if (bt->flg  == 1) {
							bf->met = bt->met;
							bf->flg = 1;
							i++;
							goto alab;
						}
					}
					edge = SL(edge,np);
				} while (fedge != edge);
				loop = np->c[loop].nc;
			} while (loop > 0);
 alab:
			;
		}
		if (ipr == i) {
			put_message(INTERNAL_ERROR,GetIDS(IDS_SG022),0);
			return FALSE;
		}
		ipr = i;
	} while (i != np->nof);
	return TRUE;
}

short b_face_label(short met)
{
	switch (key) {
		case  UNION:
			switch (met) {
				case 0:
					return 1;
				case 3:
					if (object_number == 1)
						return 1;
					else
						return 0;
				default:
					return 0;
			}
		case INTER:
			switch (met) {
				case 1:
					return 1;
				case 3:
					if (object_number == 1)
						return 1;
					else
						return 0;
				default:
					return 0;
			}
		case  SUB:
				if (object_number == 1) {
					if (met == 0 || met == 2) return 1;
					else                      return 0;
				} else if (met == 1) return 1;
							 else          return 0;
	}
	return 0;
}
