#include "../../sg.h"

BOOL cyl_gru(lpNPW np);

short create_cyl_np(lpLISTH listh, sgFloat  *par, BOOL constr)
{
	sgFloat 	    r = par[0];
	sgFloat 		  h = par[1];
	int				  n = (int)par[2];
	short         i;	//,gru;
	sgFloat      alfa, d_alfa;
	NP_STR      str;
	VLD         vld;
	MESHDD      mdd;
	MNODE       node;
	lpNPW       np = NULL;
	sgFloat      k_i, step_i;

	c_num_np = -32767;
	init_vdim(&mdd.vdim,sizeof(node));
	if ( !np_init_list(&c_list_str) ) goto err;
	d_alfa = 2 * M_PI / n;
	step_i = n / 4.;
	for (i = 0, k_i=0.; i < n+1 ; i++) {
		alfa = ( i != n ) ? (d_alfa*i) : 2*M_PI;
		node.p.x = r*cos(alfa);
		node.p.y = r*sin(alfa);
		node.p.z = 0;
		node.num = (short)CONSTR_U;
		if ( constr ) node.num |= CONSTR_V;
		else {
			if ( i == (short)(k_i+0.5) ) {
				node.num |= COND_V;
				k_i += step_i;
			}
		}
		if ( !add_elem(&mdd.vdim,&node) ) goto err2;
	}
	for (i = 0 ; i < n+1 ; i++) {
		alfa = ( i != n ) ? (d_alfa*i) : 2*M_PI;
		node.p.x = r*cos(alfa);
		node.p.y = r*sin(alfa);
		node.p.z = h;
		node.num = (short)CONSTR_U;
		if ( !add_elem(&mdd.vdim,&node) ) {
err2:
			free_vdim(&mdd.vdim);
			return FALSE;
		}
	}
	mdd.n = n+1;
	mdd.m = 2;

	if(!(np_mesh4p(&c_list_str,&c_num_np,&mdd,0.))) goto err;
	free_vdim(&mdd.vdim);

	if ( c_list_str.number_all == 1) {
		init_vld(&vld);
		read_elem(&c_list_str.vdim,0,&str);
		if (!(read_np(&c_list_str.bnp,str.hnp,npwg))) goto err;
		if ( !(cyl_gru(npwg)) ) goto err;
/*		gru = 0;
		for (i=1 ; i <= npwg->noe ; i++) {      //   
			if (npwg->efc[i].fp < 0 || npwg->efc[i].fm < 0) gru++;
		}
*/
		add_np_mem(&vld,npwg,TNPB);

		np_face(npwg, r, n, 0.,&c_num_np);   	//  
		for (i = 1 ; i <= npwg->noe ; i++) {
			npwg->efc[i].fm = -i;
			npwg->efc[i].em = -32767;
			npwg->gru[i] = (float)M_PI/2.0f;
		}
		npwg->f[1].fl = ST_FACE_PLANE;

		np_inv(npwg,NP_ALL);
		add_np_mem(&vld,npwg,TNPB);

		np_face(npwg, r, n, h, &c_num_np);     	//  
		for (i = 1 ; i <= npwg->noe ; i++) {
			npwg->efc[i].fm = -i;
			npwg->efc[i].em = -32767;
			npwg->gru[i] = (float)M_PI*1.5f;
		}
		npwg->f[1].fl = ST_FACE_PLANE;

		add_np_mem(&vld,npwg,TNPB);
		*listh = vld.listh;
		np_end_of_put(&c_list_str,NP_CANCEL,0,NULL);
    c_num_np = 3;
		goto end;
	}

//   
	if ((np = creat_np_mem(TNPW,MAXNOV,MAXNOE,MAXNOC, MAXNOF,
													MAXNOE)) == NULL)  goto err1;
	np_init((lpNP)np);

	np_face(np, r, n, 0.,&c_num_np);								  //  
	np->f[1].fl = ST_FACE_PLANE;

	if( np->nov > MAXNOV ||	np->noe > MAXNOE ){
		if( !sub_division( np ) ) goto err1; }
	else	if ( !np_put(np,&c_list_str) ) goto err1;

	np_face(np, r, n, h, &c_num_np);								 //  
	np->f[1].fl = ST_FACE_PLANE;

	if( np->nov > MAXNOV ||	np->noe > MAXNOE ){
		if( !sub_division( np ) ) goto err1; }
	else if ( !np_put(np,&c_list_str) ) goto err1;

	//   
	if ((c_num_np = np_end_of_put(&c_list_str,NP_FIRST,30,listh)) == 0) goto err1;
	free_np_mem(&np);
  goto end;

err1:
	free_np_mem(&np);
err:
	free_vdim(&mdd.vdim);
	np_end_of_put(&c_list_str,NP_CANCEL,0,NULL);
  c_num_np = 0;
end:
  return c_num_np;
}
BOOL cyl_gru(lpNPW np)
{
	short   i;

	for (i = 1 ; i <= np->noe ; i++) {
		if (np->efc[i].ep != 0 && np->efc[i].em != 0) continue;
		np->gru[i] = (float)M_PI*1.5f;
		if (np->efc[i].em == 0) {
			np->efc[i].fm = -i;
			if (np->v[np->efr[i].bv].z == 0) np->efc[i].em = -32766;
			else                             np->efc[i].em = -32765;
		} else {
			np->efc[i].fp = -i;
			if (np->v[np->efr[i].bv].z == 0) np->efc[i].ep = -32766;
			else                             np->efc[i].ep = -32765;
		}
	}
	return TRUE;
}
