#include "../../sg.h"

sgFloat	c_smooth_angle = 30.*M_PI/180;//    

#define B_FIRST  (char)0x01
#define B_LAST   (char)0x02
#define B_LEFT   (char)0x04
#define B_RIGHT  (char)0x08

typedef struct {               //   0 -  
	char bound;                  //    
	D_POINT p[4];                //    
} M2_TYPE;

typedef M2_TYPE *         lpM2_TYPE;

static  lpM2_TYPE def_surf_type_m24(int ku, int kv, lpVDIM vdim);
static  void def_dim4(int *nu,int *nv,BOOL pr);
static  void zav_edge_face4(lpNPW npw,short nu,short nv);
static  void zav_first4(lpNPW npw,short nu,short nv,BOOL pr);
static  void zav_last4(lpNPW npw,short nu, short nv,BOOL pr);
static  BOOL np_clmesh4(lpNPW npw,short nu,short nv,short cod_zam);
static  BOOL np_clmesh3(lpNPW npw,short nu,short nv,short cod_zam);
static  void zav_edge_face3(lpNPW npw,short nu,short nv);
short  np_24_init(short kup, short kvp, lpVDIM vdimp,
										short* ident_freep, BOOL pr);
BOOL np_m24_next(lpNPW npw);
BOOL np_m23_next(lpNPW npw);
static  BOOL revers_mesh(int *ku, int *kv, lpVDIM vdim);
static  BOOL np_updown(int ku, int kv, lpVDIM vdimp);
static  BOOL np_rightleft(int ku, int kv, lpVDIM vdimp);

static int ku,kv;         								//   
static short nu,nv;      											  //  
static lpVDIM vdim;
static short start_u, start_v, u,v, fin_u, fin_v, firstv;
static int  ident_free;    			   //    
static BOOL left_right, up_down;
static	short 	num_np_v;                        // -   V
static	short 	num_np_u;                        // -   U

static lpM2_TYPE     b;


BOOL np_mesh4p(lpNP_STR_LIST list, short *ident, lpMESHDD mdd,sgFloat angle)
{
	short num_np,j;

	npwg->type = TNPW;
	num_np = np_24_init(mdd->m, mdd->n, &mdd->vdim, ident, TRUE);
	if ( !num_np ) return FALSE;

	for ( j=0; j<num_np; j++ ) {
		if ( !np_m24_next(npwg) ) return FALSE;
		if ( !np_cplane(npwg) ) return FALSE;      //  - -
		if ( angle != 0 ) np_constr_to_approch(npwg, (float)angle);   // .  
		if ( !(np_put(npwg,list)) ) return FALSE;
	}
	return TRUE;
}

short np_24_init(short kup, short kvp, lpVDIM vdimp, short* ident_freep,BOOL pr)
{
//	short 	num_np_v;                        // -   V
//	short 	num_np_u;                        // -   U
	short 	num_np;
	int 	tmp_u, tmp_v;

	vdim = vdimp;
	kv = kvp;
	ku = kup;

	 //   
	b = def_surf_type_m24(ku,kv,vdim);

	if ( b->bound&B_LEFT || b->bound&B_RIGHT) {
		if ( !revers_mesh(&ku,&kv,vdim)) return 0;
		b = def_surf_type_m24(ku,kv,vdim);
	}

	start_v = b->bound&B_LEFT;
	start_u = b->bound&B_FIRST;
	u=start_u;
	v=start_v;
	//   
	up_down = np_updown(ku,kv,vdim);
	left_right = np_rightleft(ku,kv,vdim);

	//   
	tmp_u=ku-start_u;    //      
	tmp_v=kv-start_v;    //   1,    0
	def_dim4(&tmp_u,&tmp_v,pr);
	nu=(short)tmp_u;
	nv=(short)tmp_v;
	if ( kv-start_v == nv ) num_np_v = 1;
	else{
		num_np_v =(short)(kv/(nv-1));
		if ( kv > num_np_v*nv-(num_np_v-1)) num_np_v++;
	}
	if ( ku-start_u == nu ) num_np_u = 1;
	else{
		num_np_u = (short)((ku)/(nu-1));
		if ( ku > num_np_u*nu-(num_np_u-1)+ start_u )	num_np_u++; //  
	}
	num_np = num_np_v*num_np_u;
	ident_free = *ident_freep;
	*ident_freep += num_np;
	return num_np;
}
static  BOOL np_updown(int ku, int kv, lpVDIM vdimp)
{
	int          j;
	MNODE					mnode1,mnode2;
	int					ind;

	for ( j = 0; j < kv; j++ ) {
			ind = (ku-1) * kv + j;
			read_elem(vdimp,j,&mnode1);
			read_elem(vdimp,ind,&mnode2);
		if (!dpoint_eq(&mnode1.p,&mnode2.p,eps_d))
																				return FALSE;
	}
	return TRUE;
}
static  BOOL np_rightleft(int ku, int kv, lpVDIM vdimp)
{
	int          j;
	MNODE					mnode1,mnode2;
	int					ind;

	for ( j = 0; j < ku; j++ ) {
			ind = (j)*kv;
			read_elem(vdimp,ind,&mnode1);
			ind = (j+1)*kv-1;
			read_elem(vdimp,ind,&mnode2);
		if (!dpoint_eq(&mnode1.p,&mnode2.p,eps_d))
																				return FALSE;
	}
	return TRUE;
}

static  BOOL revers_mesh(int *ku, int *kv, lpVDIM vdimp)
{
	register short	i,j;
	VDIM					vdn;
	MNODE					mnode;
	int					ind;

	init_vdim(&vdn,vdimp->size_elem);
	for ( i=0; i<*ku; i++ ) {
		for ( j=0; j<*kv; j++ ) {
			ind = j*(*ku)+i;
			read_elem(vdimp,ind,&mnode);
			if ( !add_elem(&vdn,&mnode) ) goto err;
		}
	}
	free_vdim(vdimp);
	*vdimp = vdn;
	ind = *ku; 	*ku = *kv;	*kv = ind;
	return TRUE;
err:
	free_vdim(&vdn);
	return FALSE;
}

BOOL np_m24_next(lpNPW npw)
{
	short  		u_np,v_np;
	short  		ver; //,i,ind_ver;
	lpMNODE node;
//	MNODE   mnode;
//	short 		num_np_v;                        // -   V
	short 		u_cur, v_cur;
	short 		cod_zam;
	short 		index;
	int 		lindex;

	npw->type = TNPW;
	if (v >= kv-1 ) {
		u=u+nu-1;
		v=start_v;
		if ( u >= ku-1  ) {
			np_handler_err(NP_MESH4);
			return FALSE;
		}
	}
	if (v == start_v) firstv=1;
	else 							firstv=0;

	if ( kv == nv ) num_np_v = 1;
	else{
	  num_np_v =(short)kv/(nv-1);
	  if ( kv > num_np_v*nv-(num_np_v-1)) num_np_v++;
	}

  np_init((lpNP)npw);
	ver = 0;                   //   
  npw->ident = ident_free++;
	if (u+nu < ku )       fin_u=u+nu;
	else if ( b->bound&B_LAST ) fin_u=(short)ku-1;
				else                   fin_u=(short)ku;     //  ku 
	if (v+nv < kv )             fin_v=v+nv;
	else {
		firstv=-1;
		if (b->bound&B_RIGHT)  fin_v=(short)kv-1;
		else                   fin_v=(short)kv;     //  kv 
	}
  u_cur=fin_u-u;
  v_cur=fin_v-v;
  zav_edge_face4(npw,u_cur,v_cur);

	for (u_np=u ; u_np < fin_u ; u_np++) {
		lindex=u_np*kv+v;    //
		begin_rw(vdim,lindex);
		for (v_np=v ; v_np < fin_v ; v_np++) {
			ver++;
			node = (MNODE*)get_next_elem(vdim);
			memcpy(&npw->v[ver],&node->p,sizeof(D_POINT));
//  
      if( node->num ) {
        if ( v_np == fin_v-1) {
        	if ( u_np != fin_u-1) {
          	if( node->num & COND_V ) {
            	index=(v_np-v+1)*2-1+(2*v_cur-1)*(u_np-u); //  . 
							npw->efr[index].el |= ST_TD_COND;
						}
          	if( node->num & CONSTR_V ) {
            	index=(v_np-v+1)*2-1+(2*v_cur-1)*(u_np-u); //  . 
							npw->efr[index].el |= ST_TD_CONSTR;
          	}
					}
				} else
        if ( u_np != fin_u-1) {
          if( node->num & COND_U ) {
            index=(v_np-v+1)*2+(2*v_cur-1)*(u_np-u); //  . 
            npw->efr[index].el |= ST_TD_COND;
          }
					if( node->num & CONSTR_U ) {
						index=(v_np-v+1)*2+(2*v_cur-1)*(u_np-u); //  . 
            npw->efr[index].el |= ST_TD_CONSTR;
					}
          if( node->num & COND_V ) {
            index=(v_np-v+1)*2-1+(2*v_cur-1)*(u_np-u); //  . 
            npw->efr[index].el |= ST_TD_COND;
          }
          if( node->num & CONSTR_V ) {
            index=(v_np-v+1)*2-1+(2*v_cur-1)*(u_np-u); //  . 
            npw->efr[index].el |= ST_TD_CONSTR;
          }
				}
        else {
					if( node->num & COND_U ) {
						index=(v_np-v+1)+(2*v_cur-1)*(u_np-u); //  . 
            npw->efr[index].el |= ST_TD_COND;
          }
          if( node->num & CONSTR_U ) {
            index=(v_np-v+1)+(2*v_cur-1)*(u_np-u); //  . 
            npw->efr[index].el |= ST_TD_CONSTR;
          }
        }
			}
		}
		end_rw(vdim);
	}
	npw->nov = ver;

//  .    . 
/* 
	if ( u > 1 ) { //   
		lindex=(u-1)*kv+v;    //
		begin_rw(vdim,lindex);
		for (i=1 ; i < v_cur ; i++) {  //
			index=i*2;
			npw->efc[index].em = npw->ident - num_np_v; // - -   V
			node = get_next_elem(vdim);
			npw->efc[index].fm = -index;
			npw->gru[index] = calc_body_angle(&npw->v[i+v_cur],&npw->v[i],
													&npw->v[i+1],&node->p);
		}
		end_rw(vdim);
	}
//  .    . 
	if (u+nu < ku )       { //   
//	if ( fin_u < ku-1 ) { //   
		lindex=(fin_u)*kv+v;    //
		begin_rw(vdim,lindex);
		for (i=1 ; i < v_cur ; i++) {  //
			index=(2*v_cur-1)*(u_cur-1)+i;
			npw->efc[index].ep = npw->ident + num_np_v; // - -   V
			node = get_next_elem(vdim);
			ind_ver=(u_cur-1) * v_cur + i;
			npw->efc[index].fp = - index;
			npw->gru[index] = calc_body_angle(&npw->v[ind_ver-v_cur],&npw->v[ind_ver+1],
													&npw->v[ind_ver],&node->p);
		}
		end_rw(vdim);
	}
//  .    . 
	if ( v > 1 ) { //   
		for (i=1 ; i < u_cur ; i++) {  //
			lindex=(u+i-1)*kv+v-1;    //
			read_elem(vdim,lindex,&mnode);
			index=(2*v_cur-1)*(i-1)+1;
			npw->efc[index].ep = npw->ident - 1; //
			ind_ver=(i-1)* v_cur + 1;
			npw->efc[index].fp = -index;
			npw->gru[index] = calc_body_angle(&npw->v[ind_ver+1],&npw->v[ind_ver+v_cur],
													&npw->v[ind_ver],&mnode.p);
		}
	}
//  .    . 
  if (firstv != -1)       { //      . 11.03.97
//	if (v+nv < kv )       { //   
//	if ( fin_v < kv-1 ) { //   
		for (i=1 ; i < u_cur ; i++) {  //
			lindex=(u+i-1)*kv+v+v_cur;    //
			read_elem(vdim,lindex,&mnode);
			index=(2*v_cur-1)*i;
			npw->efc[index].em = npw->ident + 1; //
			ind_ver=i* v_cur;
			npw->efc[index].fm = -index;
			npw->gru[index] = calc_body_angle(&npw->v[ind_ver-1],&npw->v[ind_ver],
													&npw->v[ind_ver+v_cur],&mnode.p);
		}
	}
	if( up_down ) { //    
//  .     . 
		if ( u == 0 ) { //    
			lindex=(ku-2)*kv+v+1;    //        ()
			begin_rw(vdim,lindex);
			for (i=1 ; i < v_cur ; i++) {
				index=i*2;
				npw->efc[index].em = npw->ident + (num_np_v)*(num_np_u-1);
				node = get_next_elem(vdim);
				npw->efc[index].fm = -index;
				npw->gru[index] = calc_body_angle(&npw->v[i+v_cur],&npw->v[i],
														&npw->v[i+1],&node->p);
			}
			end_rw(vdim);
		}
//  .     . 
		if ( u+nu >= ku ) { //    
			lindex=kv+v+1;    //        ()
			begin_rw(vdim,lindex);
			for (i=1 ; i < v_cur ; i++) {
				index=(2*v_cur-1)*(u_cur-1)+i;
				npw->efc[index].ep = npw->ident - (num_np_v)*(num_np_u-1);
				node = get_next_elem(vdim);
				npw->efc[index].fp = -index;
				ind_ver=(u_cur-1) * v_cur + i;
				npw->gru[index] = calc_body_angle(&npw->v[ind_ver-v_cur],&npw->v[ind_ver+1],
														&npw->v[ind_ver],&node->p);
			}
			end_rw(vdim);
		}
	}
	if( left_right ) { //    
	//  .     . 
		if ( v == 0 ) { //    
			for (i=1 ; i < u_cur ; i++) {  //
				lindex=(u+i)*kv-2;    //
				read_elem(vdim,lindex,&mnode);
				index=(2*v_cur-1)*(i-1)+1;
				npw->efc[index].ep = npw->ident + num_np_v - 1; //
				ind_ver=(i-1)* v_cur + 1;
				npw->efc[index].fp = -index;
				npw->gru[index] = calc_body_angle(&npw->v[ind_ver+1],&npw->v[ind_ver+v_cur],
														&npw->v[ind_ver],&mnode.p);
			}
		}
	//  .     . 
		if (v+nv >= kv )       { //    
			for (i=1 ; i < u_cur ; i++) {  //
				lindex=(u+i-1)*kv+1;    //
				read_elem(vdim,lindex,&mnode);
				index=(2*v_cur-1)*i;
				npw->efc[index].em = npw->ident - num_np_v + 1; //
				ind_ver=i* v_cur;
				npw->efc[index].fm = -index;
				npw->gru[index] = calc_body_angle(&npw->v[ind_ver-1],&npw->v[ind_ver],
														&npw->v[ind_ver+v_cur],&mnode.p);
			}
		}
	}
*/

	cod_zam = 0;
	if (u == 1 && b->bound&B_FIRST)
			{ cod_zam= 1 ; zav_first4(npw,u_cur,v_cur,TRUE);}     //  
	if (fin_u == ku-1 && b->bound&B_LAST)
			{ cod_zam |= 2; zav_last4(npw,u_cur,v_cur,TRUE);}  // 
//	if (v == 1 && b->bound&B_LEFT)
//			{ cod_zam |= 4; zav_left4(npw,u_cur,v_cur);}  //  
//	if (fin_v == kv-1 && b->bound&B_RIGHT)
//			{ cod_zam |= 8; zav_rigth4(npw,u_cur,v_cur);} //  

	if (ku == nu || kv == nv) {                   //  
		if (np_clmesh4(npw,u_cur,v_cur,cod_zam)) {
		  if ( !(np_del_edge(npw)) ) return FALSE;
		  np_del_vertex(npw);
		}
	}

	v=v+nv-1;
	return TRUE;
}

static  BOOL np_clmesh4(lpNPW npw,short nu,short nv,short cod_zam)
{
	short i,pu=0,pv=0;
	short last_edge,edge,edge2;

	if ( nu == 1) {         //  
		if (dpoint_eq(&npw->v[1],&npw->v[nv],eps_d)) {
			npw->efr[nv-1].ev = 1;              //  
      last_edge = nv-1;
			if (cod_zam & 1) {               //  
				edge=2*nv-1;  //   
				edge2=nv;
				npw->efc[edge2].fm = npw->efc[edge].fm; //   
				npw->efc[edge2].em = npw->efc[edge].em;
        npw->efc[edge].em = 0;
        npw->efc[edge].fm = 0;
				edge= edge-1;
        npw->efc[edge].ep = -edge2;
				last_edge=last_edge+nv;
//				last_edge=edge;
			}
			if (cod_zam & 2) {               //  
				edge=last_edge+nv;  //   
				edge2=last_edge+1;
        npw->efc[edge2].fp = npw->efc[edge].fp; //   
        npw->efc[edge2].ep = npw->efc[edge].ep;
        npw->efc[edge].ep = 0;
        npw->efc[edge].fp = 0;
				edge = nv-1;
        npw->efc[edge].ep = edge2;
			}
		}
		return TRUE;
	}

	for (i=1; i <= nv; i++) {
    if (!dpoint_eq(&npw->v[i],&npw->v[(nu-1)*nv+i],eps_d)) {
			pv=1;       //   V 
			break;
		}
	}
	for (i=1; i <= nu; i++) {
    if (!dpoint_eq(&npw->v[(i-1)*nv+1],&npw->v[(i)*nv],eps_d)) {
			pu=1;       //   U 
			break;
		}
	}
	last_edge=(2*nv-1)*(nu-1)+nv-1;
	if (pv == 0){       //   V
		for (i=1 ; i < nv ; i++) {
			edge = (2*nv - 1)*(nu - 1) + i;      	//  
      npw->efc[i*2].fm = npw->efc[edge].fm; //   
			npw->efc[i*2].em = npw->efc[edge].em;
			npw->efc[edge].em = 0;
      npw->efc[edge].fm = 0;
			edge2 = (2*nv - 1)*(nu - 2) + 2*i+1;  	//   . 
      npw->efc[edge2].ep = -2*i;
      npw->efr[edge2].ev = npw->efr[i*2].ev;
      npw->efr[edge2-2].ev = npw->efr[i*2].bv;
		}
/*
		if (cod_zam & 4) {               //  
			edge=last_edge+nu;  //   
			edge2=last_edge+1;
			npw->efc[edge2].fp = npw->efc[edge].fp; //   
			npw->efc[edge2].ep = npw->efc[edge].ep;
			npw->efc[edge].ep = 0;
			npw->efc[edge].fp = 0;
			edge= (2*nv - 1)*(nu-2)+1;
			npw->efc[edge].ep = edge2;
//        last_edge=edge;
			last_edge=last_edge+nu;
		}
		if (cod_zam & 8) {               //  
      edge=last_edge+nu;  //   
      edge2=last_edge+1;
      npw->efc[edge2].fp = npw->efc[edge].fp; //   
			npw->efc[edge2].ep = npw->efc[edge].ep;
      npw->efc[edge].ep = 0;
      npw->efc[edge].fp = 0;
			edge= edge-1;
      npw->efc[edge].em = edge2;
//        last_edge=edge;
      last_edge=last_edge+nu;
    }
*/
	}
	if (pu == 0){        //   U
		for (i=1 ; i < nu ; i++) {
			edge  = (2*nv - 1)*(i);              	//  
			edge2 = (2*nv - 1)*(i-1)+1;
			npw->efc[edge2].fp = npw->efc[edge].fp; //   
			npw->efc[edge2].ep = npw->efc[edge].ep;
      npw->efc[edge].ep = 0;
      npw->efc[edge].fp = 0;
      npw->efc[edge-1].ep = edge2;
      npw->efr[edge-1].ev = npw->efr[edge2].bv;
      edge = abs(npw->efc[edge2].ep);
      npw->efr[edge].ev = npw->efr[edge2].ev;

		}
    if (cod_zam & 1) {               //  
      edge=last_edge+nv;  //   
      edge2=last_edge+1;
      npw->efc[edge2].fm = npw->efc[edge].fm; //   
			npw->efc[edge2].em = npw->efc[edge].em;
      npw->efc[edge].em = 0;
      npw->efc[edge].fm = 0;
      edge= edge-1;
      npw->efc[edge].ep = -edge2;
//        last_edge=edge;
      last_edge=last_edge+nv;
		}
    if (cod_zam & 2) {               //  
      edge=last_edge+nv;  //   
      edge2=last_edge+1;
      npw->efc[edge2].fp = npw->efc[edge].fp; //   
      npw->efc[edge2].ep = npw->efc[edge].ep;
      npw->efc[edge].ep = 0;
			npw->efc[edge].fp = 0;
			edge = (2*nv-1)*(nu-1)+nv-1;
      npw->efc[edge].ep = edge2;
    }
		return TRUE;
	}
	if (pv == 0)       //   V
		return TRUE;
	else
		return FALSE;
}

static  BOOL np_clmesh3(lpNPW npw,short nu,short nv,short cod_zam)
{
	short i,pu=0,pv=0;
	short last_edge,edge,edge2;

	if ( nu == 1) {         //  
    if (dpoint_eq(&npw->v[1],&npw->v[nv],eps_d)) {
			npw->efr[nv-1].ev = 1;              //  
      last_edge = nv-1;
			if (cod_zam & 1) {               //  
				edge=2*nv-1;  //   
				edge2=nv;
        npw->efc[edge2].fm = npw->efc[edge].fm; //   
        npw->efc[edge2].em = npw->efc[edge].em;
        npw->efc[edge].em = 0;
				npw->efc[edge].fm = 0;
				edge= edge-1;
        npw->efc[edge].ep = -edge2;
				last_edge=last_edge+nv;
//				last_edge=edge;
			}
			if (cod_zam & 2) {               //  
				edge=last_edge+nv;  //   
				edge2=last_edge+1;
        npw->efc[edge2].fp = npw->efc[edge].fp; //   
        npw->efc[edge2].ep = npw->efc[edge].ep;
        npw->efc[edge].ep = 0;
        npw->efc[edge].fp = 0;
				edge = nv-1;
        npw->efc[edge].ep = edge2;
			}
		}
		return TRUE;
	}

	for (i=1; i <= nv; i++) {
    if (!dpoint_eq(&npw->v[i],&npw->v[(nu-1)*nv+i],eps_d)) {
			pv=1;       //   V 
			break;
		}
	}
	for (i=1; i <= nu; i++) {
    if (!dpoint_eq(&npw->v[(i-1)*nv+1],&npw->v[(i)*nv],eps_d)) {
			pu=1;       //   U 
			break;
		}
	}
//  last_edge=(2*nv-1)*(nu-1)+nv-1;
  last_edge=(3*nv-2)*(nu-1)+nv-1;
  if (pv == 0){       //   V (  . )
		for (i=1 ; i < nv ; i++) {
			edge = (3*nv - 2)*(nu - 1) + i;       //  
      npw->efc[i*3].fm = npw->efc[edge].fm; //   
      npw->efc[i*3].em = npw->efc[edge].em;
      npw->efc[edge].em = 0;
			npw->efc[edge].fm = 0;
      edge2 = (3*nv - 2)*(nu - 2) + 3*i-1;  //  .  . 
      npw->efc[edge2].ep = -3*i;
      npw->efr[edge2].ev = npw->efr[i*3].ev;
      npw->efr[edge2-1].ev = npw->efr[i*3].bv; // . 
		}
    npw->efr[edge2+2].ev = npw->efr[edge2].ev; // . . 
	}
	if (pu == 0){        //   U (  .)
		for (i=1 ; i < nu ; i++) {
			edge  = (3*nv - 2)*(i);               //  
      edge2 = (3*nv - 2)*(i-1)+1;
      npw->efc[edge2].fp = npw->efc[edge].fp; //   
      npw->efc[edge2].ep = npw->efc[edge].ep;
      npw->efc[edge].ep = 0;
      npw->efc[edge].fp = 0;
      npw->efc[edge-1].ep = edge2;
      npw->efr[edge-1].ev = npw->efr[edge2].bv;
      edge = abs(npw->efc[edge2].ep);           // . 
      npw->efr[edge].ev = npw->efr[edge2].ev;
		}
    npw->efr[last_edge].ev = npw->efr[edge2].ev;

    if (cod_zam & 1) {               //  
			edge=last_edge+nv;  //   
      edge2=last_edge+1;
      npw->efc[edge2].fm = npw->efc[edge].fm; //   
      npw->efc[edge2].em = npw->efc[edge].em;
			npw->efc[edge].em = 0;
      npw->efc[edge].fm = 0;
      edge= edge-1;
      npw->efc[edge].ep = -edge2;
//        last_edge=edge;
      last_edge=last_edge+nv;
    }
    if (cod_zam & 2) {               //  
      edge=last_edge+nv;  //   
      edge2=last_edge+1;
      npw->efc[edge2].fp = npw->efc[edge].fp; //   
      npw->efc[edge2].ep = npw->efc[edge].ep;
			npw->efc[edge].ep = 0;
      npw->efc[edge].fp = 0;
      edge = (3*nv-2)*(nu-1)+nv-1;
      npw->efc[edge].ep = edge2;
    }
		return TRUE;
	}
	if (pv == 0)       //   V
		return TRUE;
	else
		return FALSE;
}

static  void def_dim4(int *nu,int *nv,BOOL pr)
{
	int nov,noe,nof;

	nov = ( *nu ) * ( *nv );
	if ( pr ) {
		noe = 2*(*nu)*(*nv) - 2*(*nu) - (*nv);
		nof = ( *nu - 1 )*( *nv - 1 );
	} else {
		noe = 3*(*nu)*(*nv) - 2*(*nu) - (*nv);
  	nof = 2*( *nu - 1 )*( *nv - 1 );
	}
  while (nov > MAXNOV/2 || noe > MAXNOE || nof > MAXNOF) {
    if (*nu > *nv) *nu = *nu/2+1;
    else           *nv = *nv/2+1;
    nov = (*nu)*(*nv);
		if ( pr ) {
  		noe = 2*(*nu)*(*nv) - 2*(*nu) - (*nv);
  		nof = ( *nu - 1 )*( *nv - 1 );
		} else {
  		noe = 3*(*nu)*(*nv) - 2*(*nu) - (*nv);
			nof = 2*( *nu - 1 )*( *nv - 1 );
		}
  }
//	if (*nv == kv) *nv = (kv+2)/2;
//	if (*nu == ku) *nu = (ku+2)/2;
}

static  void zav_edge_face4(lpNPW npw,short nu,short nv)
{
  short first_edge,edge,ver,face;
  short u,v;

	edge = 1;
  ver = 1;
  face = 1;
	if ( nu == 1) {
		for (v=1 ; v < nv ; v++) {      //   
			npw->efr[edge].bv = ver;      //   
			npw->efr[edge].ev = ver+1;
			npw->efc[edge].ep = 0;
			npw->efc[edge].em = 0;
			npw->efc[edge].fp = 0;
			npw->efc[edge].fm = 0;
			npw->efr[edge].el = ST_TD_APPROCH;
			edge++;
			ver++;
		}
		npw->noe = edge-1;
		npw->nof = 0;
		npw->noc = 0;
		return;
	}
  for (u=1 ; u < nu ; u++) {
		first_edge = edge;
    for (v=1 ; v < nv ; v++) {
      npw->efr[edge].bv = ver;                     // edge - 
			npw->efr[edge].ev = ver+nv;
      npw->efc[edge].ep = -(edge+(nv-1)*2);
      npw->efc[edge].em = edge+1;
      npw->efc[edge].fp = face-1;
      npw->efc[edge].fm = face;
			npw->efr[edge].el = ST_TD_APPROCH;
			npw->efr[edge+1].bv=ver;                 // edge+1 - 
			npw->efr[edge+1].ev=ver+1;
			npw->efc[edge+1].ep = edge+2;
			npw->efc[edge+1].em = -(edge-2*(nv-1)-1);
			npw->efc[edge+1].fp = face;
			npw->efc[edge+1].fm = face-nv+1;
			npw->efr[edge+1].el = ST_TD_APPROCH;
			npw->f[face].fc = face;
			npw->f[face].fl = 0;
			npw->c[face].nc = 0;
			npw->c[face].fe = edge+1;
			edge = edge+2;
			ver++;
			face++;
		}
		npw->efr[edge].bv = ver;               //   
		npw->efr[edge].ev = ver+nv;
		npw->efc[edge].ep = -(edge+2*(nv-1));
		npw->efc[edge].fp = face-1;
		npw->efc[edge].em = 0;
		npw->efc[edge].fm = 0;
		npw->efr[edge].el = ST_TD_APPROCH;
		npw->efc[first_edge].ep = 0;             //   
		npw->efc[first_edge].fp = 0;
		edge++;
		ver++;
	}
	for (v=1 ; v < nv ; v++) {      //    
		npw->efr[edge].bv = ver;
		npw->efr[edge].ev = ver+1;
		npw->efc[edge].ep = 0;
		npw->efc[edge].em = -(edge-v-2*(nv-1)+2*(v-1));
		npw->efc[edge].fp = 0;
		npw->efc[edge].fm = face-nv+v;
		npw->efr[edge].el = ST_TD_APPROCH;
		npw->efc[edge-v-2*(nv-1)+2*v].ep = -edge;
		edge++;
		ver++;
	}
	npw->noe = edge-1;
	npw->nof = face-1;
	npw->noc = face-1;
	for (v=1 ; v < nv ; v++) {      //    
		edge = v*2;
		npw->efc[edge].em = 0;
		npw->efc[edge].fm = 0;
	}
}

static  void  zav_first4(lpNPW npw,short nu, short nv,BOOL pr)
{
	lpMNODE node;
	short 		ver_new, ver;
	short 		edge, edge_n, noe, edge_dn;
	short 		face, i;
//	short  		index,ind_ver;
//	int 		lindex;
//	MNODE   mnode;

	ver_new = ++npw->nov;
	memcpy(&npw->v[ver_new],&b->p[2],sizeof(D_POINT));
	noe = npw->noe;              //    
	ver = 1;
	if ( nu == 1 ){
		edge_n = 1;                  //  
		edge_dn = 1;                 //   . 
	}
	else {
		if ( pr ) {
			edge_n = 2;                  //  
			edge_dn = 2;
		} else {
			edge_n = 3;                  //  
			edge_dn = 3;
		}
	}
	face = ++npw->nof;
	begin_rw(vdim,v);
	for (i = 1 ;  i < nv ; i++) {
		edge = ++npw->noe;
		npw->efr[edge].bv = ver;
		npw->efr[edge].ev = ver_new;
		npw->efc[edge].ep = -(edge+1);
		npw->efc[edge].fp = face;
		npw->efc[edge].em = -(edge_n-edge_dn);
		npw->efc[edge].fm = face-1;
		node = (MNODE*)get_next_elem(vdim);
		npw->efr[edge].el = 0;
		if( node->num & CONSTR_V ) {
			npw->efr[edge].el |= ST_TD_CONSTR;
		} else {
			npw->efr[edge].el |= ST_TD_APPROCH;
			if( node->num & COND_V ) {
				npw->efr[edge].el |= ST_TD_COND;
			}
		}
		npw->efc[edge_n].em = edge;
		npw->efc[edge_n].fm = face;
		npw->f[face].fc = face;
		npw->f[face].fl = 0;
		npw->c[face].nc = 0;
		npw->c[face].fe = edge;
		ver++;
		edge_n += edge_dn;
		face++;
	}
	npw->efc[noe+1].em = 0;               //  
	npw->efc[noe+1].fm = 0;
	edge = ++npw->noe;              //  
	npw->efr[edge].bv = ver;
	npw->efr[edge].ev = ver_new;
	npw->efc[edge].ep = 0;
	npw->efc[edge].fp = 0;
	node = (MNODE*)get_next_elem(vdim);
	npw->efr[edge].el = 0;
	if( node->num & CONSTR_V ) {
		npw->efr[edge].el |= ST_TD_CONSTR;
	} else {
		npw->efr[edge].el |= ST_TD_APPROCH;
		if( node->num & COND_V ) {
			npw->efr[edge].el |= ST_TD_COND;
		}
	}
	npw->efc[edge].em = -(edge_n-edge_dn);
	npw->efc[edge].fm = face-1;
	npw->noc = npw->nof = --face;
	end_rw(vdim);
/*	
  if (firstv != -1)       { //      . 11.03.97
//	if ( fin_v < kv-1 ) { //   
//	if ( fin_v < kv-1) { //   
			lindex=(u)*kv+v+nv;         //  .  VDIM
			read_elem(vdim,lindex,&mnode);
			index=edge;                   //    
			npw->efc[index].ep = npw->ident + 1; //
			ind_ver=ver;                   //  .  
			npw->efc[index].fp = -index;
			npw->gru[index] = calc_body_angle(&npw->v[ind_ver-1],&npw->v[ver_new],
																			 &npw->v[ind_ver],&mnode.p);
	}

	if ( v > 1 ) {                  //   
			lindex=(u)*kv+v-1;            //  .  VDIM
			read_elem(vdim,lindex,&mnode);
			index=noe+1;                 //    
			npw->efc[index].em = npw->ident - 1; //
			ind_ver=1;                   //  .  
			npw->efc[index].fm = -index;
			npw->gru[index] = calc_body_angle(&npw->v[ind_ver+1],&npw->v[ind_ver],
																				&npw->v[ver_new],	&mnode.p);
	}
	if ( left_right ) {
    if (firstv == -1)       { //      . 11.03.97
//		if ( fin_v >= kv-1) { //   
				lindex=kv+1;         //  .  VDIM
				read_elem(vdim,lindex,&mnode);
				index=edge;                   //    
				npw->efc[index].ep = npw->ident - num_np_v + 1; //
				ind_ver=ver;                   //  .  
				npw->efc[index].fp = -index;
				npw->gru[index] = calc_body_angle(&npw->v[ind_ver-1],&npw->v[ver_new],
																				 &npw->v[ind_ver],&mnode.p);
		}

		if ( v == 0 ) {                  //   
				lindex=2*kv-2;            //  .  VDIM
				read_elem(vdim,lindex,&mnode);
				index=noe+1;                 //    
				npw->efc[index].em = npw->ident + num_np_v - 1; //
				ind_ver=1;                   //  .  
				npw->efc[index].fm = -index;
				npw->gru[index] = calc_body_angle(&npw->v[ind_ver+1],&npw->v[ind_ver],
																					&npw->v[ver_new],	&mnode.p);
		}
	}
*/
}

static  void  zav_last4(lpNPW npw,short nu, short nv, BOOL pr)
{
	lpMNODE node;
	short 		ver_new, ver;
	short 		edge, edge_n, noe;
	short 		face, i;
//	short  		index,ind_ver;
//	int 		lindex;
//	MNODE   mnode;

	ver_new = ++npw->nov;
	memcpy(&npw->v[ver_new],&b->p[3],sizeof(D_POINT));
	noe = npw->noe;              //    
	ver = nv*(nu-1)+1;
	if ( pr )
		edge_n = ((nv-1)*2+1)*(nu-1)+1;         //  
	else
		edge_n = ((nv-1)*3+1)*(nu-1)+1;         //  
	face = ++npw->nof;
	begin_rw(vdim,(ku-1)*kv+v);
	for (i = 1 ;  i < nv ; i++) {
		edge = ++npw->noe;
		npw->efr[edge].bv = ver;
		npw->efr[edge].ev = ver_new;
		npw->efc[edge].ep = -(edge-1);
		npw->efc[edge].fp = face-1;
		npw->efc[edge].em = edge_n;
		npw->efc[edge].fm = face;
		node = (MNODE*)get_next_elem(vdim);
		npw->efr[edge].el = 0;
		if( node->num & CONSTR_V ) {
			npw->efr[edge].el |= ST_TD_CONSTR;
		} else {
			npw->efr[edge].el |= ST_TD_APPROCH;
			if( node->num & COND_V ) {
				npw->efr[edge].el |= ST_TD_COND;
			}
		}
		npw->efc[edge_n].ep = edge+1;
		npw->efc[edge_n].fp = face;
		npw->f[face].fc = face;
		npw->f[face].fl = 0;
		npw->c[face].nc = 0;
		npw->c[face].fe = -edge;
		ver++;
		edge_n++;
		face++;
	}
	npw->efc[noe+1].ep = 0;               //  
	npw->efc[noe+1].fp = 0;
	edge = ++npw->noe;              //  
	npw->efr[edge].bv = ver;
	npw->efr[edge].ev = ver_new;
	npw->efc[edge].em = 0;
	npw->efc[edge].fm = 0;
	npw->efc[edge].ep = -(edge-1);
	npw->efc[edge].fp = face-1;
	node = (MNODE*)get_next_elem(vdim);
	npw->efr[edge].el = 0;
	if( node->num & CONSTR_V ) {
		npw->efr[edge].el |= ST_TD_CONSTR;
	} else {
		npw->efr[edge].el |= ST_TD_APPROCH;
		if( node->num & COND_V ) {
			npw->efr[edge].el |= ST_TD_COND;
		}
	}
	npw->noc = npw->nof = --face;
	end_rw(vdim);
/*	
  if (firstv != -1)       { //      . 11.03.97
//	if ( fin_v < kv-1) { //   
			lindex=(u+nu-1)*kv+v+nv;         //  .  VDIM
			read_elem(vdim,lindex,&mnode);
			index=edge;                   //    
			npw->efc[index].em = npw->ident + 1; //
			ind_ver=ver;                   //  .  
			npw->efc[index].fm = -index;
			npw->gru[index] = calc_body_angle(&npw->v[ind_ver-1],&npw->v[ind_ver],
													&npw->v[ver_new],&mnode.p);
	}

	if ( v > 1 ) {                  //   
			lindex=(u+nu-1)*kv+v-1;       //  .  VDIM
			read_elem(vdim,lindex,&mnode);
			index=noe+1;                 //    
			npw->efc[index].ep = npw->ident - 1; //
			ind_ver=nv*(nu-1)+1;         //  .  
			npw->efc[index].fp = -index;
			npw->gru[index] = calc_body_angle(&npw->v[ind_ver+1],&npw->v[ver_new],
																				&npw->v[ind_ver],&mnode.p);
	}
	if ( left_right ) {
    if (firstv == -1)       { //      . 11.03.97
//		if ( fin_v >= kv-1) { //   
				lindex=(ku-1)*kv+1;         //  .  VDIM
				read_elem(vdim,lindex,&mnode);
				index=edge;                   //    
				npw->efc[index].em = npw->ident - num_np_v + 1; //
				ind_ver=ver;                   //  .  
				npw->efc[index].fm = -index;
				npw->gru[index] = calc_body_angle(&npw->v[ind_ver-1],&npw->v[ind_ver],
														&npw->v[ver_new],&mnode.p);
		}

		if ( v == 0 ) {                  //   
				lindex=(ku-1)*kv-2;       //  .  VDIM
				read_elem(vdim,lindex,&mnode);
				index=noe+1;                 //    
				npw->efc[index].ep = npw->ident + num_np_v - 1; //
				ind_ver=nv*(nu-1)+1;         //  .  
				npw->efc[index].fp = -index;
				npw->gru[index] = calc_body_angle(&npw->v[ind_ver+1],&npw->v[ver_new],
																					&npw->v[ind_ver],&mnode.p);
		}
	}
*/
}

static  lpM2_TYPE def_surf_type_m24(int ku, int kv, lpVDIM vdim)
{
	static M2_TYPE 	b;
	MNODE 					n1,n2;

	memset(&b,0,sizeof(b));
	read_elem(vdim,0,&n1);
	read_elem(vdim,1,&n2);

	if ( dpoint_eq(&n1.p,&n2.p,eps_d ) ) {
		b.bound |= B_FIRST;                      //   
		memcpy(&b.p[2],&n1.p,sizeof(D_POINT));
	}
	read_elem(vdim,kv,&n2);
	if ( dpoint_eq(&n1.p,&n2.p,eps_d ) ) {
		b.bound |= B_LEFT;                       //   
		memcpy(&b.p[0],&n1.p,sizeof(D_POINT));
	}
	read_elem(vdim,(int)kv*ku -2 ,&n2);
	read_elem(vdim,(int)kv*ku -1 ,&n1);
	if ( dpoint_eq(&n1.p,&n2.p,eps_d ) ) {
		b.bound |= B_LAST;                       //   
		memcpy(&b.p[3],&n1.p,sizeof(D_POINT));
	}
	read_elem(vdim,(int)kv*(ku-1)-1,&n2);
	if ( dpoint_eq(&n1.p,&n2.p,eps_d ) ) {
		b.bound = B_RIGHT;                       //   
		memcpy(&b.p[1],&n1.p,sizeof(D_POINT));
	}
	return &b;
}

BOOL np_mesh3p(lpNP_STR_LIST list, short *ident, lpMESHDD mdd,sgFloat angle)
{
	short num_np,j;

	npwg->type = TNPW;
	num_np = np_24_init(mdd->m, mdd->n, &mdd->vdim, ident, FALSE);

	for ( j=0; j<num_np; j++ ) {
		if ( !np_m23_next(npwg) ) return FALSE;
		if ( !np_cplane(npwg) ) return FALSE;      //  - -
		if ( angle != 0 ) np_constr_to_approch(npwg, (float)angle);   // .  
		if ( !(np_put(npwg,list)) ) return FALSE;
	}
	return TRUE;
}

BOOL np_m23_next(lpNPW npw)
{
	short  		u_np,v_np;
	short  		ver;
//	short  		i,ind_ver;
	lpMNODE node;
//	MNODE   mnode;
//	short 		num_np_v;                        // -   V
	short 		u_cur, v_cur;
	short 		index,cod_zam;
	int 		lindex;

	npw->type = TNPW;
	if (v >= kv-1 ) {
		u=u+nu-1;
		v=start_v;
		if ( u >= ku-1 ) return FALSE;
	}

	if ( kv == nv ) num_np_v = 1;
	else{
	  num_np_v = (short)kv/(nv-1);
	  if ( kv > num_np_v*nv-(num_np_v-1)) num_np_v++;
	}

  np_init((lpNP)npw);
	ver = 0;                   //   
  npw->ident = ident_free++;
	if (u+nu < ku )       fin_u=u+nu;
	else if ( b->bound&B_LAST ) fin_u=(short)ku-1;
				else                   fin_u=(short)ku;     //  ku 
	if (v+nv < kv )             fin_v=v+nv;
	else if (b->bound&B_RIGHT)  fin_v=(short)kv-1;
        else                   fin_v=(short)kv;     //  kv 
	u_cur=fin_u-u;
  v_cur=fin_v-v;
  zav_edge_face3(npw,u_cur,v_cur);

  for (u_np=u ; u_np < fin_u ; u_np++) {
		lindex=u_np*kv+v;    //
		begin_rw(vdim,lindex);
		for (v_np=v ; v_np < fin_v ; v_np++) {
			ver++;
			node = (MNODE*)get_next_elem(vdim);
			memcpy(&npw->v[ver],&node->p,sizeof(D_POINT));
//  
      if( node->num ) {
        if ( v_np == fin_v-1) {
        	if ( u_np != fin_u-1) {
          	if( node->num & COND_V ) {
              index=(v_np-v+1)*3-2+(3*v_cur-2)*(u_np-u); //  . 
            	npw->efr[index].el |= ST_TD_COND;
          	}
          	if( node->num & CONSTR_V ) {
							index=(v_np-v+1)*3-2+(3*v_cur-2)*(u_np-u); //  . 
            	npw->efr[index].el |= ST_TD_CONSTR;
          	}
					}
				} else
        if ( u_np != fin_u-1) {
          if( node->num & COND_U ) {
            index=(v_np-v+1)*3+(3*v_cur-2)*(u_np-u); //  . 
            npw->efr[index].el |= ST_TD_COND;
          }
          if( node->num & CONSTR_U ) {
            index=(v_np-v+1)*3+(3*v_cur-2)*(u_np-u); //  . 
            npw->efr[index].el |= ST_TD_CONSTR;
          }
          if( node->num & COND_V ) {
            index=(v_np-v+1)*3-2+(3*v_cur-2)*(u_np-u); //  . 
            npw->efr[index].el |= ST_TD_COND;
          }
          if( node->num & CONSTR_V ) {
            index=(v_np-v+1)*3-2+(3*v_cur-2)*(u_np-u); //  . 
						npw->efr[index].el |= ST_TD_CONSTR;
          }
        }
        else {
          if( node->num & COND_U ) {
            index=(v_np-v+1)+(3*v_cur-2)*(u_np-u); //  . 
            npw->efr[index].el |= ST_TD_COND;
          }
          if( node->num & CONSTR_U ) {
            index=(v_np-v+1)+(3*v_cur-2)*(u_np-u); //  . 
            npw->efr[index].el |= ST_TD_CONSTR;
          }
        }
			}
		}
		end_rw(vdim);
	}
	npw->nov = ver;
/*
//  .    . 
  if ( u > 1 ) { //   
		lindex=(u-1)*kv+v;    //
		begin_rw(vdim,lindex);
		for (i=1 ; i < v_cur ; i++) {  //
			index=i*3;
			npw->efc[index].em = npw->ident - num_np_v; // - -   V
			node = get_next_elem(vdim);
			npw->efc[index].fm = -index;
			npw->gru[index] = calc_body_angle(&npw->v[i+v_cur],&npw->v[i],
													&npw->v[i+1],&node->p);
		}
		end_rw(vdim);
	}
//  .    . 
	if ( fin_u != ku && !(b->bound&B_LAST)) { //   
		lindex=(fin_u)*kv+v;    //
		begin_rw(vdim,lindex);
		for (i=1 ; i < v_cur ; i++) {  //
			index=(3*v_cur-2)*(u_cur-1)+i;
			npw->efc[index].ep = npw->ident + num_np_v; // - -   V
			node = get_next_elem(vdim);
			ind_ver=(u_cur-1) * v_cur + i;
			npw->efc[index].fp = - index;
			npw->gru[index] = calc_body_angle(&npw->v[ind_ver-v_cur],&npw->v[ind_ver+1],
													&npw->v[ind_ver],&node->p);
		}
		end_rw(vdim);
	}
//  .    . 
	if ( v > 1 ) { //   
		for (i=1 ; i < u_cur ; i++) {  //
			lindex=(u+i-1)*kv+v-1;    //
			read_elem(vdim,lindex,&mnode);
			index=(3*v_cur-2)*(i-1)+1;
			npw->efc[index].ep = npw->ident - 1; //
			ind_ver=(i-1)* v_cur + 1;
			npw->efc[index].fp = -index;
			npw->gru[index] = calc_body_angle(&npw->v[ind_ver+1],&npw->v[ind_ver+v_cur],
													&npw->v[ind_ver],&mnode.p);
		}
	}
//  .    . 
	if ( fin_v != kv && !(b->bound&B_RIGHT)) { //   
		for (i=1 ; i < u_cur ; i++) {  //
			lindex=(u+i-1)*kv+v+v_cur;    //
			read_elem(vdim,lindex,&mnode);
			index=(3*v_cur-2)*i;
			npw->efc[index].em = npw->ident + 1; //
			ind_ver=i* v_cur;
			npw->efc[index].fm = -index;
			npw->gru[index] = calc_body_angle(&npw->v[ind_ver-1],&npw->v[ind_ver],
													&npw->v[ind_ver+v_cur],&mnode.p);
		}
	}
	if( up_down ) { //    
//  .     . 
		if ( u == 0 ) { //    
			lindex=(ku-2)*kv+v+1;    //        ()
			begin_rw(vdim,lindex);
			for (i=1 ; i < v_cur ; i++) {
				index=i*3;
				npw->efc[index].em = npw->ident + (num_np_v)*(num_np_u-1);
				node = get_next_elem(vdim);
				npw->efc[index].fm = -index;
				npw->gru[index] = calc_body_angle(&npw->v[i+v_cur],&npw->v[i],
														&npw->v[i+1],&node->p);
			}
			end_rw(vdim);
		}
//  .     . 
		if ( u+nu >= ku ) { //    
			lindex=kv+v+1;    //        ()
			begin_rw(vdim,lindex);
			for (i=1 ; i < v_cur ; i++) {
				index=(3*v_cur-2)*(u_cur-1)+i;
				npw->efc[index].ep = npw->ident - (num_np_v)*(num_np_u-1);
				node = get_next_elem(vdim);
				npw->efc[index].fp = -index;
				ind_ver=(u_cur-1) * v_cur + i;
				npw->gru[index] = calc_body_angle(&npw->v[ind_ver-v_cur],&npw->v[ind_ver+1],
														&npw->v[ind_ver],&node->p);
			}
			end_rw(vdim);
		}
	}
	if( left_right ) { //    
	//  .     . 
		if ( v == 0 ) { //    
			for (i=1 ; i < u_cur ; i++) {  //
				lindex=(u+i)*kv-2;    //
				read_elem(vdim,lindex,&mnode);
				index=(3*v_cur-2)*(i-1)+1;
				npw->efc[index].ep = npw->ident + num_np_v - 1; //
				ind_ver=(i-1)* v_cur + 1;
				npw->efc[index].fp = -index;
				npw->gru[index] = calc_body_angle(&npw->v[ind_ver+1],&npw->v[ind_ver+v_cur],
														&npw->v[ind_ver],&mnode.p);
			}
		}
	//  .     . 
		if (v+nv >= kv )       { //    
			for (i=1 ; i < u_cur ; i++) {  //
				lindex=(u+i-1)*kv+1;    //
				read_elem(vdim,lindex,&mnode);
				index=(3*v_cur-2)*i;
				npw->efc[index].em = npw->ident - num_np_v + 1; //
				ind_ver=i* v_cur;
				npw->efc[index].fm = -index;
				npw->gru[index] = calc_body_angle(&npw->v[ind_ver-1],&npw->v[ind_ver],
														&npw->v[ind_ver+v_cur],&mnode.p);
			}
		}
	}
*/
	cod_zam = 0;
	if (u == 1 && b->bound&B_FIRST)
			{ cod_zam= 1 ; zav_first4(npw,u_cur,v_cur,FALSE);}     //  
	if (fin_u == ku-1 && b->bound&B_LAST)
			{ cod_zam |= 2; zav_last4(npw,u_cur,v_cur,FALSE);}  // 

	if (ku == nu || kv == nv) {                   //  
		if (np_clmesh3(npw,u_cur,v_cur,cod_zam)) {
			if ( !(np_del_edge(npw)) ) return FALSE;
			np_del_vertex(npw);
		}
	}

	v=v+nv-1;
	return TRUE;
}

static  void zav_edge_face3(lpNPW npw,short nu,short nv)
{
	short first_edge,edge,ver,face;
	short u,v;

	edge = 1;
  ver = 1;
  face = 1;
	if ( nu == 1) {
		for (v=1 ; v < nv ; v++) {      //   
			npw->efr[edge].bv = ver;      //   
			npw->efr[edge].ev = ver+1;
			npw->efc[edge].ep = 0;
			npw->efc[edge].em = 0;
			npw->efc[edge].fp = 0;
			npw->efc[edge].fm = 0;
			npw->efr[edge].el = ST_TD_APPROCH;
			edge++;
			ver++;
		}
		npw->noe = edge-1;
		npw->nof = 0;
		npw->noc = 0;
		return;
	}
  for (u=1 ; u < nu ; u++) {
		first_edge = edge;
    for (v=1 ; v < nv ; v++) {
      npw->efr[edge].bv = ver;                     // edge - 
      npw->efr[edge].ev = ver+nv;
      npw->efc[edge].ep = -(edge-2);
      npw->efc[edge].em = edge+1;
      npw->efc[edge].fp = face-1;
      npw->efc[edge].fm = face+nv-1;
			npw->efr[edge].el = ST_TD_APPROCH;
			npw->efr[edge+1].bv=ver;                 // edge+1 - 
			npw->efr[edge+1].ev=ver+nv+1;
			npw->efc[edge+1].ep = -(edge+3*nv);
			npw->efc[edge+1].em = edge+2;
			npw->efc[edge+1].fp = face+nv-1;
			npw->efc[edge+1].fm = face;
			npw->efr[edge+1].el = ST_TD_DUMMY;
			npw->efr[edge+2].bv=ver;                 // edge+2 - 
			npw->efr[edge+2].ev=ver+1;
			npw->efc[edge+2].ep = edge+3;
			npw->efc[edge+2].em = -(edge-3*(nv-1)-1);
			npw->efc[edge+2].fp = face;
			npw->efc[edge+2].fm = face-nv+1;
			npw->efr[edge+2].el = ST_TD_APPROCH;
			npw->f[face].fc = face;
			npw->f[face].fl = 0;
			npw->c[face].nc = 0;
			npw->c[face].fe = edge+2;
      npw->f[face+nv-1].fc = face+nv-1;
      npw->f[face+nv-1].fl = 0;
      npw->c[face+nv-1].nc = 0;
      npw->c[face+nv-1].fe = edge+1;
      edge = edge+3;
			ver++;
			face++;
		}
		npw->efr[edge].bv = ver;               //   
		npw->efr[edge].ev = ver+nv;
		npw->efc[edge].ep = -(edge-2);
		npw->efc[edge].fp = face-1;
		npw->efc[edge].em = 0;
		npw->efc[edge].fm = 0;
		npw->efr[edge].el = ST_TD_APPROCH;
		npw->efc[first_edge].ep = 0;             //   
		npw->efc[first_edge].fp = 0;
		face = face+nv-1;
		edge++;
		ver++;
	}
	for (v=1 ; v < nv ; v++) {      //    
		npw->efr[edge].bv = ver;
		npw->efr[edge].ev = ver+1;
		npw->efc[edge].ep = 0;
		npw->efc[edge].em = -(edge-v-3*(nv-1)+3*(v-1));
		npw->efc[edge].fp = 0;
		npw->efc[edge].fm = face-nv+v;
		npw->efr[edge].el = ST_TD_APPROCH;
		npw->efc[edge-v-3*(nv-1)+3*(v-1)+1].ep = -edge;
		edge++;
		ver++;
	}
	npw->noe = edge-1;
	npw->nof = face-1;
	npw->noc = face-1;
	for (v=1 ; v < nv ; v++) {      //    
		edge = v*3;
		npw->efc[edge].em = 0;
		npw->efc[edge].fm = 0;
	}
}

BOOL np_create_brep(lpMESHDD mdd, lpLISTH listh)
{
  NP_STR_LIST list_str;
  short c_num_np;

  if ( !np_init_list(&list_str) ) return FALSE;
  c_num_np = -32767;
  if (!(np_mesh3p(&list_str,&c_num_np,mdd,0.))) goto err;
	if ( ! np_end_of_put(&list_str, NP_ALL, (float)c_smooth_angle, listh) )
		return FALSE;

  return TRUE;
err:
	np_end_of_put(&list_str,NP_CANCEL,0,listh);
  return FALSE;
}
