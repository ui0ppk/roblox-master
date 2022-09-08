#include "../sg.h"

short b_diver(lpNPW np, lpNP_VERTEX_LIST ver, short ukt, short r,
						lpD_POINT vn,lpD_POINT vk)
{
  short    ir,r_old,r1,r2,edge;
  short    v1,v2,w1,w2,wn,bv;

	if ( (w1 = np_new_vertex(np, vn)) == 0)    return 0;
	if ( (w2 = np_new_vertex(np, vk)) == 0)    return 0;
  r_old = r;
  r1 = ver->v[ukt-1].r;
  r2 = ver->v[ukt  ].r;
  v1 = ver->v[ukt-1].v;
  v2 = ver->v[ukt  ].v;
  ir = abs(r);
  bv = np->efr[ir].bv;
  if ((v1 == w1) && (v2 == w2)) return (r);
	

  if (w2 == v2) {                           
    if ( !(edge = b_dive(np, ir, w1)) ) return 0;
    if (bv == v1)  r = edge*isg(r);
    return (r);
  }
  ver->v[ukt-1].v = w2;
  if (w1 == v1) {                            
    if ( !(edge = b_dive(np,ir,w2)) ) return 0;
    if (bv == v2)  r = edge*isg(r);
  } else {             
    if (bv == v2) {
      wn=w2;  w2=w1;  w1=wn;
    }
    if ( !(b_dive(np,ir,w2)) ) return 0;
    if ( !(edge = b_dive(np,ir,w1)) ) return 0;
    r = edge*isg(r);
  }
	if (r_old == r1) ver->v[ukt-1].r = SL(r2,np);
	else {
		ver->v[ukt-1].r = r;
		ver->v[ukt  ].r = SL(r,np);
  }
  return r;
}


short b_dive(lpNPW np, short edge, short v)
{
	short           bv, ev, vv;
	short           iedge, edged, ident;
	lpNPW         npd;
  hINDEX        hindex, hindex2;
  short           num_index, num_index2;
	VNP           vnp;
	lpBO_USER     user;
  lpNP_STR_LIST listd, listdd;

	iedge = abs(edge);
	bv = np->efr[iedge].bv;
	ev = np->efr[iedge].ev;

	if (np->efc[iedge].fp < 0 || np->efc[iedge].fm < 0) {  
		if (np->efc[iedge].fp < 0) ident = np->efc[iedge].ep;
		else                       ident = np->efc[iedge].em;
		if (np == npa)		npd = npb;
		else 							npd = npa;
		if ( object_number == 1) {
			listd = &lista;  listdd = &listb;
			hindex = hindex_a; num_index = num_index_a;
			hindex2 = hindex_b; num_index2 = num_index_b;
		} else {
			listd = &listb;  listdd = &lista;
			hindex = hindex_b; num_index = num_index_b;
			hindex2 = hindex_a; num_index2 = num_index_a;
		}

		if ( !np_write_np_tmp(npd) ) return 0;           
		if ( !(vnp=b_get_hnp(hindex,num_index,ident)) ) {
			listd = listdd;
			if ( !(vnp=b_get_hnp(hindex2,num_index2,ident)) ) {
				put_message(INTERNAL_ERROR,GetIDS(IDS_SG020),0);
				goto err;
			}
		}
		if ( !read_np(&listd->bnp,vnp,npd) )  goto err;
		if ( (edged=b_search_edge(npd,&np->v[bv],&np->v[ev],
															np->ident)) == 0) {
			put_message(INTERNAL_ERROR,GetIDS(IDS_SG021),0);
			goto err;
		}
		npd->nov_tmp = npd->maxnov;
		if ( (vv=np_new_vertex(npd,&np->v[v])) == 0) goto err;
		if ( (edged=np_divide_edge(npd,edged,vv)) == 0) goto err;
		user = (lpBO_USER)npd->user;
		user[edged].bnd = 0;
		user[edged].ind = 0;
		if ( !overwrite_np(&listd->bnp,vnp,npd,TNPW) ) goto err;
		if ( !np_read_np_tmp(npd) ) return 0;
	}
	if (	(edged=np_divide_edge(np,edge,v)) == 0) return 0;
	user = (lpBO_USER)np->user;
	user[edged].bnd = 0;
	user[edged].ind = 0;

	return edged;
err:
	np_free_np_tmp();
	return 0;
}

short b_search_edge(lpNPW npw, lpD_POINT bv, lpD_POINT ev, short ident)
{
  short edge;
  short bv_npw, ev_npw;

  for (edge=1 ; edge <= npw->noe ; edge++) {
		if (npw->efc[edge].fp > 0 && npw->efc[edge].fm > 0) continue;
    if (npw->efc[edge].ep == ident || npw->efc[edge].em == ident) {
      bv_npw = npw->efr[edge].bv;
      ev_npw = npw->efr[edge].ev;
      if ( !(dpoint_eq(&npw->v[bv_npw],bv,eps_d)) &&
           !(dpoint_eq(&npw->v[bv_npw],ev,eps_d))) continue;
      if ( !(dpoint_eq(&npw->v[ev_npw],bv,eps_d)) &&
           !(dpoint_eq(&npw->v[ev_npw],ev,eps_d))) continue;
        break;
    }
	}
  if (edge > npw->noe) return 0;
  return edge;
}
