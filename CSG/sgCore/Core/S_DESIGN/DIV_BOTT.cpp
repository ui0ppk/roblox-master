#include "../sg.h"

static void form_countor( lpNPW np, short contour, short nov_old );
static BOOL  b_up1(lpNPW np, short face, lpNP_VERTEX_LIST ver, lpVDIM add_vdim );
static void  b_def_pl(lpNPW np, short face, lpD_PLANE pl);
static short control_face( lpNPW np );

BOOL create_contour_appr(lpVDIM list_vdim, hOBJ hpath, BOOL *closed){
	VDIM				vdim;
	OSTATUS			status;

	if (!get_status_path(hpath, &status)) return FALSE;
	*closed = (status & ST_CLOSE) ? TRUE : FALSE;
	if (!apr_path(&vdim,hpath)) return FALSE;
/*	if ( vdim.num_elem > MAXINT ) {
		cinema_handler_err(CIN_OVERFLOW_MDD);
		return FALSE;
	}
*/
	if( !add_elem( list_vdim, &vdim ) ) return FALSE;
	return TRUE;
}

BOOL put_bottom( lpNPW np, lpVDIM list_vdim ){
short			 i, i1;
short 		 nov_old;
lpVDIM 	 vdim;
lpMNODE  node;

if (!begin_rw( list_vdim, 0)) return FALSE;
np->nov = np->noe = np->noc = np->nof = 0;

for (i = 0; i < list_vdim->num_elem; i++) {//    
	if ((vdim = (lpVDIM)get_next_elem(list_vdim)) == NULL) goto err;

	nov_old = np->nov;

	if( !begin_rw( vdim, 0) ) goto err;
//   -  
	if( np->nov + vdim->num_elem > np->maxnov )
			if( !my_expand_face( np, (short)vdim->num_elem )) goto err1;

	for( i1=1; i1<vdim->num_elem; i1++) {
		if( (node = (lpMNODE)get_next_elem(vdim)) == NULL) goto err1;
		np->v[++np->nov] = node->p;
	}
	end_rw(vdim);
/*	if ( vdim->num_elem > MAXINT ) {
		cinema_handler_err(CIN_OVERFLOW_MDD);
		goto err;
	}
*/	form_countor( np, (short)vdim->num_elem, nov_old );
}
np->f[1].fl |= ST_FACE_PLANE;
end_rw(list_vdim);
return TRUE;

err1:
 end_rw(vdim);
err:
 end_rw(list_vdim);
 return FALSE;
}

BOOL sub_division_face( lpNPW np, lpVDIM add_vdim ){
	BOOL           rt=FALSE;
	NP_VERTEX_LIST ver = {0,0,NULL,NULL};
	D_PLANE        pl;
	short            face=1;// !!!!!!!   

	while(1){
		if ( !np_ver_alloc(&ver) ) return FALSE;

		b_def_pl(np, face, &pl);           //   

		ver.uk = 0;
		np->nov_tmp = np->maxnov;
		if ( !np_pl(np, face, &ver, &pl) )  goto err;
		if (ver.uk == 0){	 //    
			rt=TRUE;
			goto err;
		}
		np_axis = np_defc(&np->p[face].v, &pl.v);
		np_sort_ver(np->v, &ver, np_axis);
		if ( !b_up1( np, face, &ver, add_vdim ) ) goto err;

//analisis of loops and faces
		if ( !np_fa( np, face ) )  goto err;

//control wether all faces < NP
		if( ( face = control_face( np ) ) == 0 ) break;
		np_free_ver(&ver);
		np_free_stack();
	}
	rt=TRUE;
err:
	np_free_ver(&ver);
	np_free_stack();
	return rt;
}

static BOOL  b_up1(lpNPW np, short face, lpNP_VERTEX_LIST ver, lpVDIM add_vdim ){
	short        p = -1, pr = -1, pt = -1;
	short        ukt = 0;
	short        loop1, loop2;
	short        w1, w2;
	short        edge, last_edge1, last_edge2;
	POINT3     point3;
	D_POINT    v1, v2;
	BOOL       le = FALSE;                     //   
																		 // (TRUE - , FALSE - )
	while (ukt < ver->uk) {
		np_av(ver->v[ukt].m,&p,&pr,&pt,&le); //    
		ukt++;
		if (p == 1) {          //  
			if (ver->v[ukt-1].v > np->nov) {
				v1 = np->v[ver->v[ukt-1].v];
				if ( (w1 = np_new_vertex(np, &v1)) == 0)    return FALSE;
			} else w1 = ver->v[ukt-1].v;
			if (ver->v[ukt-1].m == 1){
				point3.begin = np->v[BE( ver->v[ukt-1].r, np )];
				point3.end   = np->v[EE( ver->v[ukt-1].r, np )];
				if ((edge=np_divide_edge(np,ver->v[ukt-1].r,w1)) == 0) return FALSE;
				if( w1==np->nov ){//  
					point3.middle = np->v[w1];
					add_elem(add_vdim, &point3);
				}
			}
			if (ver->v[ukt].v > np->nov) {
				v2 = np->v[ver->v[ukt].v];
				if ( (w2 = np_new_vertex(np, &v2)) == 0)    return FALSE;
			} else w2 = ver->v[ukt].v;
			if (ver->v[ukt].m == 1){
				point3.begin = np->v[BE( ver->v[ukt].r, np )];
				point3.end   = np->v[EE( ver->v[ukt].r, np )];
				if ((edge=np_divide_edge(np,ver->v[ukt].r,w2)) == 0) return FALSE;
				if( w2==np->nov ){//  
					point3.middle= np->v[w2];
					add_elem(add_vdim, &point3);
				}
			}
			if ( !(edge = np_new_edge(np,w1,w2)) ) return FALSE;
			if ( edge < 0 ) continue; //RA
			np->efr[edge].el = ST_TD_DUMMY;
			//if ( edge < 0 ) continue;//RA
			np->efc[edge].fp = np->efc[edge].fm = face;
			loop1 = np_def_loop(np,face,         edge,  &last_edge1);
			if (loop1 == 0) continue;//RA
			loop2 = np_def_loop(np,face,(short)(-edge), &last_edge2);
			if (loop2 == 0) continue;//RA
			np_insert_edge(np,          edge,  last_edge1);
			np_insert_edge(np, (short)(-edge), last_edge2);
			//if (loop1 == 0 || loop2 == 0) continue; //RA
			if ( !np_an_loops(np, face, loop1, loop2, edge) ) return FALSE;
		}
	}
	return TRUE;
}

static void  b_def_pl(lpNPW np, short face, lpD_PLANE pl){
	short       loop, edge, fedge, vn;
	sgFloat    dx, dy, dz;
	REGION_3D g = {{1.e35,1.e35,1.e35},{-1.e35,-1.e35,-1.e35}};

	pl->v.x = pl->v.y = pl->v.z = 0;
	loop = np->f[face].fc;        //    
	do {
		fedge = np->c[loop].fe;
		edge = fedge;
//		vn = BE(edge,np);
		do {
			vn = BE(edge,np);
			if (g.min.x > np->v[vn].x)  g.min.x = np->v[vn].x;
			if (g.min.y > np->v[vn].y)  g.min.y = np->v[vn].y;
			if (g.min.z > np->v[vn].z)  g.min.z = np->v[vn].z;
			if (g.max.x < np->v[vn].x)  g.max.x = np->v[vn].x;
			if (g.max.y < np->v[vn].y)  g.max.y = np->v[vn].y;
			if (g.max.z < np->v[vn].z)  g.max.z = np->v[vn].z;
			edge = SL(edge,np);
		} while(edge != fedge);
		loop = np->c[loop].nc;
	} while (loop != 0);

	dx = fabs(g.max.x - g.min.x);
	dy = fabs(g.max.y - g.min.y);
	dz = fabs(g.max.z - g.min.z);
	if (dx > dy) {
		if (dx > dz) { pl->v.x = 1; pl->d = - (g.min.x + dx/2); }
		else         { pl->v.z = 1; pl->d = - (g.min.z + dz/2); }
	} else {
		if (dy > dz) { pl->v.y = 1; pl->d = - (g.min.y + dy/2); }
		else         { pl->v.z = 1; pl->d = - (g.min.z + dz/2); }
	}
}

BOOL add_new_points( lpVDIM list_vdim, lpVDIM add_vdim ){
BOOL		 rt=FALSE;
short 		 i, i1, i2, i3, new_num, num_cont_new;
short			 ipp_cont;
VDIM		 new_vdim, new_list;
lpVDIM	 vdim;
lpPOINT3 add_vdim_cont, p;
lpMNODE	 new_cont, p1;

//  
	init_vdim(&new_list, sizeof(VDIM));

//    
	new_num=(short)add_vdim->num_elem;
	if( (add_vdim_cont=(POINT3 *)SGMalloc(new_num*sizeof(POINT3)))==NULL ) return FALSE;
	if (!begin_rw(add_vdim,  0) ) goto err;
	for(i=0; i<new_num; i++){
			if( (p = (lpPOINT3)get_next_elem(add_vdim)) == NULL){
			end_rw(add_vdim);
			goto err;
		}
		add_vdim_cont[i].begin =p->begin;
		add_vdim_cont[i].end   =p->end;
		add_vdim_cont[i].middle=p->middle;
	}
	end_rw(add_vdim);

//   
	if( !begin_rw(list_vdim, 0) ) goto err;
	for( i=0; i<list_vdim->num_elem; i++) {//    
		if( (vdim = (lpVDIM)get_next_elem(list_vdim)) == NULL) goto err1;

//       + -  /2
		num_cont_new=(short)vdim->num_elem;
		if( (new_cont=(MNODE *)SGMalloc((num_cont_new+new_num)*sizeof(MNODE)))==NULL ) goto err1;
		if (!begin_rw(vdim, 0))  goto err2;
		for(i1=0; i1<num_cont_new; i1++){
			if( (p1=(lpMNODE)get_next_elem(vdim)) == NULL){
				end_rw(vdim);
				goto err2;
			}
			new_cont[i1].p  =p1->p;
			new_cont[i1].num=p1->num;
		}
		end_rw(vdim);

		ipp_cont=0; //  
		for( i1=0; i1<new_num; i1++){//     
			for( i2=1; i2<num_cont_new; i2++) {//    
				if( (dpoint_distance(&add_vdim_cont[i1].begin, &new_cont[i2-1].p )<eps_d&&
						 dpoint_distance(&add_vdim_cont[i1].end,   &new_cont[i2  ].p )<eps_d)||
						(dpoint_distance(&add_vdim_cont[i1].end,   &new_cont[i2-1].p )<eps_d&&
						 dpoint_distance(&add_vdim_cont[i1].begin, &new_cont[i2  ].p )<eps_d)){
//   
//   
						 for(i3=num_cont_new-1; i3>=i2; i3--) new_cont[i3+1]=new_cont[i3];
//     
						 new_cont[i2].p=add_vdim_cont[i1].middle;
						 new_cont[i2].num=AP_SOPR;
						 num_cont_new++;
//      
						 add_vdim_cont[i1].begin.x=add_vdim_cont[i1].end.x=0.;
						 add_vdim_cont[i1].begin.y=add_vdim_cont[i1].end.y=0.;
						 add_vdim_cont[i1].begin.z=add_vdim_cont[i1].end.z=0.;
//    new_cont
						 ipp_cont++;
						 break;
				}
			}//     
		}//      

		if( ipp_cont>0 ){//  
//    
			for(i1=0; i1<ipp_cont; i1++){
				for(i3=0; i3<new_num; i3++ )
					if( add_vdim_cont[i3].begin.x==0. && add_vdim_cont[i3].end.x==0. &&
							add_vdim_cont[i3].begin.y==0. && add_vdim_cont[i3].end.y==0. &&
							add_vdim_cont[i3].begin.z==0. && add_vdim_cont[i3].end.z==0.){
							for( i2=i3; i2<new_num-1; i2++ ) add_vdim_cont[i2]=add_vdim_cont[i2+1];
							new_num--;
							break;
				 }
			}
		}
//  VDIM
		if( !init_vdim(&new_vdim, sizeof(MNODE))) goto err2;
		for( i3=0; i3<num_cont_new; i3++)
				 if( !add_elem(&new_vdim, &new_cont[i3])) goto err2 ;
//  vdim     
		add_elem(&new_list, &new_vdim);
		SGFree(new_cont);
	}
	SGFree(add_vdim_cont);
	end_rw(list_vdim);

	free_list_vdim(list_vdim);

 //   
	if( !init_vdim(list_vdim, sizeof(VDIM))) return FALSE;
	begin_rw(&new_list,0);
	for( i=0; i<new_list.num_elem; i++ ){
		if( (vdim = (lpVDIM)get_next_elem(&new_list)) == NULL) goto err0;
		if( !add_elem(list_vdim, vdim)) goto err0;;
	}
	rt=TRUE;
err0:
	end_rw(&new_list);
	free_vdim(&new_list);
	return rt;

err2:
	SGFree(new_cont);
err1:
	end_rw(list_vdim);
err:
	SGFree(add_vdim_cont);
	return rt;
}

static void form_countor( lpNPW np, short contour, short nov_old ){
	short i, edge, edge_old, v;

	edge_old = edge = np->noe;
	v = nov_old;
	for (i = 0 ; i < contour - 2; i++){
		np->efr[++edge].bv = ++v;
		np->efr[  edge].ev = v+1;
		np->efr[  edge].el = ST_TD_CONSTR;
		np->efc[  edge].ep = edge + 1;
		np->efc[  edge].fp = 1;
		np->efc[  edge].em = 0;
		np->efc[  edge].fm = 0;
	}
	np->efr[++edge].bv = ++v;
	np->efr[  edge].ev = nov_old+1;
	np->efr[  edge].el = ST_TD_CONSTR;
	np->efc[  edge].ep = edge_old+1;
	np->efc[  edge].fp = 1;
	np->efc[  edge].em = 0;
	np->efc[  edge].fm = 0;
	np->noe = edge;

	if( np->nof == 0) np->f[1].fc = 1;
	else              np->c[np->noc].nc = np->noc+1;
	np->noc++;
	np->c[np->noc].fe = edge_old+1;
	np->c[np->noc].nc = 0;
	np->nof = 1;
}


void multiply_np(lpNPW np, lpMATR matr){
short i;
	for( i=1; i<=np->nov; i++	)	o_hcncrd( matr, &np->v[i], &np->v[i]);
}

/**********************************************************
* wether exist face > çó
*/
static short control_face( lpNPW np ){
	short i, number, edge, first_c, first_v;
	for( i=1; i<=np->nof; i++ ){
		first_c = np->f[i].fc;
		number = 0;
		do{
			edge    = np->c[first_c].fe;
			first_v = BE( edge, np );
			do{
				number++;
				edge = SL( edge, np );
			}while( first_v != EE( edge, np ) );
			number++;
			first_c = np->c[first_c].nc;
		}while( first_c != 0 );
		if( number > MAXNOV || number > MAXNOE ) return (i);
	}
	return 0;
}

BOOL free_list_vdim(lpVDIM list){
short i;
lpVDIM vdim;
	if( !begin_rw(list, 0) ) return FALSE;
	for( i=0; i<list->num_elem; i++) {//    
		if( (vdim = (lpVDIM)get_next_elem(list)) == NULL) return FALSE;
		free_vdim(vdim);
	}
	end_rw(list);
	free_vdim(list);
	return TRUE;
}
