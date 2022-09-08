#include "../sg.h"

BOOL b_sv(lpNPW np)
{
  short i,j;
  short edge,fedge,face,loop;
  NP_BIT_FACE *bf,*b;

  b = (NP_BIT_FACE*)&np->f[1].fl;
  b->met = 1;
  i = 1;
  do {
    j = i;
    i = 0;
    for (face=1 ; face <= np->nof ; face++) {
      b = (NP_BIT_FACE*)&np->f[face].fl;
      if (b->met == 1) { i++; continue; }
      loop = np->f[face].fc;
      do {
        edge = fedge = np->c[loop].fe;
        do {
          facea = SF(edge,np);
          if (facea > 0) {
            bf = (NP_BIT_FACE*)&np->f[facea].fl;
            if (bf->met == 1) {
              b->met = 1;
              i++;
              break;
            }
          }
          edge = SL(edge,np);
        } while (edge != fedge);
        loop = np->c[loop].nc;
      } while(loop != 0 && b->met == 0);
    }
  } while (i != j && i != np->nof);
  if (i == np->nof) return TRUE;
  return FALSE;
}
