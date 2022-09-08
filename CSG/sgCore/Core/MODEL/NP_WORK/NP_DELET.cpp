#include "../../sg.h"

BOOL np_del_edge(lpNPW np)
{
	short i,kyda,min,k;
	short r,rn;
	short face,count;
	short tmp;

	kyda = 1;
	for ( i=1 ; i <= np->noe ; i++ ) {
    if (np->efc[i].fp > 0 || np->efc[i].fm > 0) {   //  
      if (i != kyda) {                     // i -   
																		  	 // kyda -   
				np->efr[kyda] = np->efr[i];
				np->efc[kyda] = np->efc[i];
				np->gru[kyda] = np->gru[i];
				if ( np->efc[kyda].fp < 0 ) np->efc[kyda].fp = - kyda;
				if ( np->efc[kyda].fm < 0 ) np->efc[kyda].fm = - kyda;
				min =  - 1;
				for ( k=1; k <= 2 ; k++ ) {
					min = -min;
					r = i*min;
					if (r > 0) face = np->efc[ r].fp;
					else       face = np->efc[-r].fm;
					if (face < 0) continue;      // 
					count = 0;
					for(;;) {
						count++;
						rn = r;
            r = SL(r,np);
						if (r == 0) break;
						if (r > np->noe || r < -np->noe || count > np->noe) {
              np_handler_err(NP_DEL_EDGE);
							return FALSE;
						}
						if (r == i*min) {
							if (rn < 0) np->efc[-rn].em = kyda*min;
							else        np->efc[ rn].ep = kyda*min;
							break;
						}
					}
				}
				tmp = - i;	// -   
				for( count = 1; count <= np->noc ; count++ ) {
					if (np->c[count].fe ==  i)       np->c[count].fe =  kyda;
					else if (np->c[count].fe == tmp) np->c[count].fe = -kyda;
				}
			}
			kyda++;
		}
	}
	np->noe = --kyda;
  return TRUE;
}

BOOL np_del_face(lpNPW np)
{
	short kyda,kolt;
	short r,rf,nextr;
  short face,count;
  NP_BIT_FACE *bf;

	kyda = 1;
	for (face = 1 ; face <= np->nof ; face++) {
    bf=(NP_BIT_FACE*)&np->f[face].fl;
		if ( bf->met == 1 )  {            //   
			if (face != kyda) {
        bf->met = 0;
        bf->flg = 0;
				np->f[kyda] = np->f[face];
				np->p[kyda] = np->p[face];
				count = np->f[face].fc;
        do {                  //      
					rf = r = np->c[count].fe;
					kolt = 0;
					do {
						kolt++;
						if (r < 0)  {
							np->efc[-r].fm = kyda;
							r = np->efc[-r].em;
						} else {
							np->efc[r].fp = kyda;
							r = np->efc[r].ep;
						}
						if (r > np->noe || r < -np->noe || kolt > np->noe) {
                np_handler_err(NP_DEL_FACE);
								return FALSE;
						}
					} while (r != rf);
					count = np->c[count].nc;
				} while (count > 0);
			}
      bf = (NP_BIT_FACE*)&np->f[kyda].fl;
      bf->flg = 0;
      bf->met = 0;
			kyda++;
		} else {                       //    
			count = np->f[face].fc;
			do {                       //      
				rf = r = np->c[count].fe;
				kolt = 0;
				do {
					kolt++;
					if (r < 0)  {
						nextr = np->efc[-r].em;
						np->efc[-r].fm = 0;
						np->efc[-r].em = 0;
					} else {
						nextr = np->efc[r].ep;
						np->efc[r].fp = 0;
						np->efc[r].ep = 0;
					}
					r = nextr;
					if (r > np->noe || r < -np->noe || kolt > np->noe) {
            np_handler_err(NP_DEL_FACE);
						return FALSE;
					}
				} while (r != rf);
				np->c[count].fe = 0;                //    
        count = np->c[count].nc;
      } while (count > 0);
    }
  }
  np->nof = --kyda;
	return TRUE;
}

BOOL np_del_loop(lpNPW np)
{
	short kyda,i,j;

	kyda = 1;
	for (i=1 ; i <= np->noc ; i++) {
		if (np->c[i].fe != 0) {
			if (i != kyda) {
				np->c[kyda].nc = np->c[i].nc;
				np->c[kyda].fe = np->c[i].fe;
				for (j=1 ; j <= kyda-1 ; j++) {
					if (np->c[j].nc == i) {             //    
						np->c[j].nc = kyda;               //    
						goto met;
					}
				}
				for (j=i+1 ;j <= np->noc ; j++) {
					if (np->c[j].nc == i) {             //    
						np->c[j].nc = kyda;               //    
						goto met;
					}
				}
				for (j=1 ; j <= np->nof ; j++) {
					if (np->f[j].fc == i) {
						np->f[j].fc = kyda;
						break;
					}
				}
met:;
			}
			kyda++;
		}
	}
	np->noc = --kyda;
	return TRUE;
}

BOOL np_del_vertex(lpNPW np)
{
	short kyda,k,i,j;

  kyda = 1;
	for (i=1 ; i <= np->nov ; i++) {
    k = 0;
		for (j=1 ; j <= np->noe ; j++) {
      if (np->efr[j].bv == i) {
        if (k == 0) {
          if (i != kyda)	np->v[kyda] = np->v[i];
					k = kyda;
          kyda++;
        }
        np->efr[j].bv = k;
      } else
        if (np->efr[j].ev == i) {
          if (k == 0) {
            if (i != kyda)	np->v[kyda] = np->v[i];
						k = kyda;
						kyda++;
					}
					np->efr[j].ev = k;
				}
		}
  }
  np->nov = --kyda;
	return TRUE;
}
