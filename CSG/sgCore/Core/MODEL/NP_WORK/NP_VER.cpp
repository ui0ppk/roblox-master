#include "../../sg.h"

static  short np_edge_nxt(short edge, lpNPW np);
static  short np_edge_pr(short first_edge, lpNPW np);
static  short (*np_edge)(short edge, lpNPW np) = np_edge_nxt;

#define MAX_COUNT 200

/*
BOOL np_vertex_normal1(lpNP_STR_LIST list_str, lpNPW np, short v, lpD_POINT p)
{
  short    edge;

  for (edge = 1 ; edge <= np->noe ; edge++) {
    if (np->efr[edge].bv == v || np->efr[edge].ev == v) break;
  }

  if (edge > np->noe)  {               //    
    p->x = 0;
    p->y = 0;
    p->z = 0;
    return TRUE;
  }
  if (v == np->efr[edge].bv) edge = -edge;
  if ( !np_vertex_normal(list_str, np, edge, p, ) ) return FALSE;
  return TRUE;
}
*/


BOOL np_vertex_normal(lpNP_STR_LIST list_str, lpNPW np, short edge, lpD_POINT p,
                      BOOL (*user_last)(lpNPW np, short edge), BOOL brk)
{
  short     fedge, face, pr;
  short     v1, v2;
  short     np_ident, ident, first_ident, last_ident;
  NP_STR  str;
  D_POINT pv1, pv2;
  BOOL    rt = TRUE;
  short     count = 0;

  if (edge > np->noe)
  {
    return FALSE;
  }
  np_ident = ident = np->ident;         //   
  fedge = edge;                              //   
  pr = 0;
  if ( user_last(np,edge) ) pr = 1;
  first_ident = np->ident;
  p->x = p->y = p->z = 0.;
  while (1)  {                    //   
    count++;
    if (count >= MAX_COUNT) goto end;
    face = EF(edge,np);
    if (face > np->nof ) goto end;              //   
    if (face > 0)
    {                             //   
      if ( brk )
        if ( np->f[face].fl & ST_FACE_PLANE )
        {
          p->x = np->p[face].v.x;
          p->y = np->p[face].v.y;
          p->z = np->p[face].v.z;
          goto end;
        }
      p->x += np->p[face].v.x;
      p->y += np->p[face].v.y;
      p->z += np->p[face].v.z;
      edge = -np_edge(edge,np);
    }
    else
    {                                      //  
      if (edge > 0)
        ident = np->efc[ edge].ep;
      else
        ident = np->efc[-edge].em;
      v1 = EE(edge,np);     pv1 = np->v[v1];
      v2 = BE(edge,np);     pv2 = np->v[v2];
      if (ident == 0)
      {                             //  
        if (pr == 0) {                               //  
          np_edge = np_edge_pr;                       //  
          p->x = p->y = p->z = 0.;
          pr = 1;
          edge = -edge;
          first_ident = np->ident;
          continue;
        } else break;                                //  
      }
      else
      {
        if ( !np_get_str(list_str,ident,&str,NULL) )
        { //goto end;  ,  
          if (pr == 0) {                               //  
            np_edge = np_edge_pr;                       //  
            p->x = p->y = p->z = 0.;
            pr = 1;
            edge = -edge;
            first_ident = np->ident;
            continue;
          }
          else
            break;                               //  
        }
        last_ident = np->ident;
        if ( !read_np(&list_str->bnp,str.hnp,np) )
        {
          goto end;
        }

        if ( !(edge=-np_search_edge(np,&pv1,&pv2,last_ident)) )
        {
          goto end;
        }
      }
    }
    if ( user_last(np,edge))
    {          //  
      if (pr == 0) {                                 //  
        np_edge = np_edge_pr;                         //  
        p->x = p->y = p->z = 0.;
        pr = 1;
        edge = -edge;
        first_ident = np->ident;
        continue;
      } else break;                                  //  
    }
    if ( pr == 1 ) continue;                         //  
    if ( first_ident == ident && edge == fedge ) break;
  }
end:
  if ( ident != np_ident ) {                    //   
    if ( !np_get_str(list_str,np_ident,&str,NULL) )
    {
      rt = FALSE; goto l;
    }

    if ( !read_np(&list_str->bnp,str.hnp,np) )
    {
      rt = FALSE; goto l;
    }
  }
  if ( !dnormal_vector(p) ) {                 //  
    face = EF(fedge,np);
    if (face > np->nof ) {
     rt = FALSE;
     p->x = 0.;
     p->y = 0.;
     p->z = 1.;
    } else if (face > 0) *p = np->p[face].v;          //   
    else {        face = EF(-fedge,np);
                  *p = np->p[face].v;
    }
  }
l:
  np_edge = np_edge_nxt;

  return rt;
}



/*

function binarySearch(a, value, left, right)
    while left ? right
        mid := floor((left+right)/2)
        if a[mid] = value
            return mid
        if value < a[mid]
            right := mid-1
        else
            left  := mid+1
    return not found


*/


BOOL RA_np_vertex_normal(sgCBRep* brep, short piece, short edge, lpD_POINT p,
            BOOL (*user_last)(lpNPW np, short edge), BOOL brk)
{
  lpNPW     np = GetNPWFromBRepPiece(brep->GetPiece(piece));
    short     zero_ident = GetNPWFromBRepPiece(brep->GetPiece(0))->ident;
  unsigned int  piecesCnt = brep->GetPiecesCount();
  short     fedge, face, pr;
  short     v1, v2;
  short     np_ident, ident, first_ident, last_ident;
//  NP_STR  str;
  D_POINT pv1, pv2;
  BOOL    rt = TRUE;
  short     count = 0;

  if (edge > np->noe)
  {
    return FALSE;
  }
  np_ident = ident = np->ident;         //   
  fedge = edge;                              //   
  pr = 0;
  if ( user_last(np,edge) ) pr = 1;
  first_ident = np->ident;
  p->x = p->y = p->z = 0.;
  while (1)  {                    //   
    count++;
    if (count >= MAX_COUNT) goto end;
    face = EF(edge,np);
    if (face > np->nof ) goto end;              //   
    if (face > 0)
    {                             //   
      if ( brk )
        if ( np->f[face].fl & ST_FACE_PLANE )
        {
          p->x = np->p[face].v.x;
          p->y = np->p[face].v.y;
          p->z = np->p[face].v.z;
          goto end;
        }
        p->x += np->p[face].v.x;
        p->y += np->p[face].v.y;
        p->z += np->p[face].v.z;
        edge = -np_edge(edge,np);
    }
    else
    {                                      //  
      if (edge > 0)
        ident = np->efc[ edge].ep;
      else
        ident = np->efc[-edge].em;
      v1 = EE(edge,np);     pv1 = np->v[v1];
      v2 = BE(edge,np);     pv2 = np->v[v2];
      if (ident == 0)
      {                             //  
        if (pr == 0) {                               //  
          np_edge = np_edge_pr;                       //  
          p->x = p->y = p->z = 0.;
          pr = 1;
          edge = -edge;
          first_ident = np->ident;
          continue;
        } else break;                                //  
      }
      else
      {
        lpNPW tmpNP = NULL;
        short tmpInd = ident-zero_ident;
        if (tmpInd<0)
          tmpInd = -tmpInd;
        if (tmpInd<piecesCnt)
        {
          tmpNP = (GetNPWFromBRepPiece(brep->GetPiece(tmpInd)));
          if (tmpNP->ident==ident)
          {
            goto  RA_label1;
          }
        }
        /*if (tmpInd>21 && tmpInd<piecesCnt)
        {
          short minLim = ((tmpInd+21)<piecesCnt)?(tmpInd+20):piecesCnt;
          for (unsigned int pC = tmpInd-20; pC<minLim; pC++)
          {
            tmpNP =  GetNPWFromBRepPiece(brep->GetPiece(pC));
            if (tmpNP->ident==ident)
            {
              goto  RA_label1;
            }
          }
        }
        for (unsigned int pC = 0; pC<piecesCnt; pC++)
        {
          tmpNP =  GetNPWFromBRepPiece(brep->GetPiece(pC));
          if (tmpNP->ident==ident)
          {
            goto  RA_label1;
          }
        }*/

		// binary search 
		{
			int leftInd = 0;
			int rightInd = piecesCnt-1;
			int mid = 0;
				while(leftInd <= rightInd)
				{
						mid = (int)((leftInd+rightInd)/2);
						tmpNP =  GetNPWFromBRepPiece(brep->GetPiece(mid));
						if (tmpNP->ident==ident)
						{
							goto  RA_label1;
						}
						if (ident < tmpNP->ident)
							rightInd = mid-1;
						else
							leftInd = mid+1;
				}
		}
        //if ( !np_get_str(list_str,ident,&str,NULL) )
        { //goto end;  ,  
          if (pr == 0) {                               //  
            np_edge = np_edge_pr;                       //  
            p->x = p->y = p->z = 0.;
            pr = 1;
            edge = -edge;
            first_ident = np->ident;
            continue;
          }
          else
            break;                               //  
        }
RA_label1:
        last_ident = np->ident;
        np = tmpNP;
        /*if ( !read_np(&list_str->bnp,str.hnp,np) )
        {
          goto end;
        }*/

        if ( !(edge=-np_search_edge(np,&pv1,&pv2,last_ident)) )
        {
          goto end;
        }
      }
    }
    if ( user_last(np,edge))
    {          //  
      if (pr == 0) {                                 //  
        np_edge = np_edge_pr;                         //  
        p->x = p->y = p->z = 0.;
        pr = 1;
        edge = -edge;
        first_ident = np->ident;
        continue;
      } else break;                                  //  
    }
    if ( pr == 1 ) continue;                         //  
    if ( first_ident == ident && edge == fedge ) break;
  }
end:
  if ( ident != np_ident ) {                    //   
  /*  lpNPW tmpNP = NULL;
    for (unsigned int pC = 0; pC<piecesCnt; pC++)
    {
      tmpNP =  GetNPWFromBRepPiece(brep->GetPiece(pC));
      if (tmpNP->ident==np_ident)
        goto  RA_label2;
    }
    //if ( !np_get_str(list_str,np_ident,&str,NULL) )
    {
      rt = FALSE; goto l;
    }
RA_label2:
    np=tmpNP;*/
    /*if ( !read_np(&list_str->bnp,str.hnp,np) )
    {
      rt = FALSE; goto l;
    }*/
    np = GetNPWFromBRepPiece(brep->GetPiece(piece));
  }
  if ( !dnormal_vector(p) ) {                 //  
    face = EF(fedge,np);
    if (face > np->nof ) {
      rt = FALSE;
      p->x = 0.;
      p->y = 0.;
      p->z = 1.;
    } else if (face > 0) *p = np->p[face].v;          //   
    else {        face = EF(-fedge,np);
    *p = np->p[face].v;
    }
  }
l:
  np_edge = np_edge_nxt;

  return rt;
}

static  short np_edge_pr(short first_edge, lpNPW np)
{
  short edge, last_edge;

  edge = first_edge;
  do {
    last_edge =  edge;
    edge = SL(edge,np);
  } while (edge != first_edge);
  return last_edge;
}
static  short np_edge_nxt(short edge, lpNPW np)
{
  if (edge > 0) return np->efc[edge].ep;
  return np->efc[-edge].em;
}
