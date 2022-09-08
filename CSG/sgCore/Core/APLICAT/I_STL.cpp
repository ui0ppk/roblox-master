#include "../sg.h"

#define MAX_STR_STL 80

static  short stl_ascii(lpBUFFER_DAT bd, lpLISTH listho, char *string,
													lpMATR m, bool solid_checking);
static  short stl_binary(lpBUFFER_DAT bd, lpLISTH listho, lpMATR m, bool solid_checking);

short ImportStl(lpBUFFER_DAT bd, lpLISTH listho, lpMATR m, bool solid_checking)
{
	char	    string[MAX_STR_STL];
	int				i;
	char			str_solid[] = "solid";
//	char			*p;

	string[0] = 0;
	load_str( bd, MAX_STR_STL, string);
	if ( (/*p=*/strstr(string,str_solid)) != NULL ) {
			i = stl_ascii( bd, listho, string, m, solid_checking);
	}	else {
			i = stl_binary( bd, listho, m, solid_checking);
	}
//	string[0] = *p;
	return i;
}
static  short stl_ascii(lpBUFFER_DAT bd, lpLISTH listho, char *string,
													lpMATR m, bool solid_checking)
{
	short			i,j, num, pass;
	char			str_loop[] 	 = "outer loop";
	char			str_vertex[] = "vertex";
	char			*p;
	NPTRP			trp;
	TRI_BIAND	trb;
	D_POINT		pp, min, max;
	MATR			matr, matri;
	hOBJ			hobj;
	long			pointer;

	pass = 1;
	pointer = bd->p_beg+bd->cur_buf;
	min.x = min.y = min.z =  1.e35;
	max.x = max.y = max.z = -1.e35;
//	num_proc = start_grad(GetIDS(IDS_SG178), bd->len_file);
	while ( (i=load_str( bd, MAX_STR_STL, string)) != 0) {
//		step_grad (num_proc , bd->p_beg+bd->cur_buf);
		if ( i == 1 ) continue;
		if ( (p = strstr(string,str_loop)) == NULL) continue;
		for ( j=0; j<3; j++ ) {
once:
			if ( !(i=load_str( bd, MAX_STR_STL, string))) goto err;
			if ( i == 1 ) goto once;
			if ( (p = strstr(string,str_vertex)) == NULL) goto err;
			p += sizeof(str_vertex);
			num = 3;
			if ( !get_nums_from_string(p, (sgFloat*)&pp, &num) ) goto err;
			if ( num != 3 ) goto err;
			modify_limits_by_point(&pp, &min, &max);
		}
	}
//	end_grad  (num_proc , bd->p_beg+bd->cur_buf);
	pp.x = - (min.x + max.x) / 2;
	pp.y = - (min.y + max.y) / 2;
	pp.z = - (min.z + max.z) / 2;
	o_hcunit(matr);
	o_tdtran(matr, &pp);
	o_minv(matr, matri);
	o_hcmult(matr, m);

	pass = 2;
	if ( !seek_buf( bd, pointer) ) return 0;
//	num_proc = start_grad(GetIDS(IDS_SG178), bd->len_file);
//	num_proc = start_grad("", bd->len_file);
	if ( !begin_tri_biand(&trb) ) return 0;
	trp.constr = 0;
	while ( (i=load_str( bd, MAX_STR_STL, string)) != 0) {
//    step_grad (num_proc , bd->p_beg+bd->cur_buf);
		if ( i == 1 ) continue;
		if ( (p = strstr(string,str_loop)) == NULL) continue;
		for ( j=0; j<3; j++ ) {
once2:
			if ( !(i=load_str( bd, MAX_STR_STL, string))) goto err;
			if ( i == 1 ) goto once2;
			if ( (p = strstr(string,str_vertex)) == NULL) goto err;
			p += sizeof(str_vertex);
			num = 3;
			if ( !get_nums_from_string(p, (sgFloat*)&trp.v[j], &num) ) goto err;
			o_hcncrd(matr,&trp.v[j],&trp.v[j]);
			if ( num != 3 ) goto err;
		}
		if ( !put_tri(&trb, &trp, solid_checking)) {
			a_handler_err(AE_ERR_DATA);
			goto err1;
		}
	}
	num = (short)listho->num;
	if ( !end_tri_biand(&trb, listho) )	i = 0;
	else {
		i = (short)listho->num - num;
		hobj = listho->htail;
		for (j = 0; j < i; j++) {
			o_trans(hobj, matri);
			get_prev_item(hobj, &hobj);
		}
	}
end:
//	end_grad  (num_proc , bd->p_beg+bd->cur_buf);
	return i;
err:
	a_handler_err(AE_BAD_STRING,bd->cur_line);
err1:
	if (pass == 2) end_tri_biand(&trb, listho);
	i = 0;
	goto end;
}

static  short stl_binary(lpBUFFER_DAT bd, lpLISTH listho, lpMATR m, bool solid_checking)
{
	int				i, j, num, pass;
	NPTRP			trp;
	TRI_BIAND	trb;
	float			ver[3];
	D_POINT		p, min, max;
	int				number;
	MATR			matr, matri;
	hOBJ			hobj;

	pass = 1;
//	num_proc = start_grad(GetIDS(IDS_SG179), bd->len_file);
	min.x = min.y = min.z =  1.e35;
	max.x = max.y = max.z = -1.e35;
	if ( !seek_buf( bd, 80L) ) return 0;
	if ( load_data( bd, 4, &number) != 4 ) goto err;
	while ( 1 ) {
//		step_grad (num_proc , bd->p_beg+bd->cur_buf);
		if ( load_data( bd, 12, ver) != 12 ) break; // goto err;
		for ( j=0; j<3; j++ ) {
			if ( load_data( bd, 12, ver) != 12 ) goto err;
			p.x = ver[0];	p.y = ver[1];	p.z = ver[2];
			modify_limits_by_point(&p, &min, &max);
		}
		if ( load_data( bd, 2, NULL) != 2 ) break; // goto err;
	}
//	end_grad  (num_proc , bd->p_beg+bd->cur_buf);
	p.x = - (min.x + max.x) / 2;
	p.y = - (min.y + max.y) / 2;
	p.z = - (min.z + max.z) / 2;
	o_hcunit(matr);
	o_tdtran(matr, &p);
	o_minv(matr, matri);
	o_hcmult(matr, m);

	pass = 2;
//	num_proc = start_grad(GetIDS(IDS_SG179), bd->len_file);
//	num_proc = start_grad("", bd->len_file);
	if ( !seek_buf( bd, 80L) ) return 0;
	if ( load_data( bd, 4, &number) != 4 ) goto err;

	if ( !begin_tri_biand(&trb) ) return 0;
	trp.constr = 0;
	while ( 1 ) {
		if ( load_data( bd, 12, ver) != 12 ) break; // goto err;
		for ( j=0; j<3; j++ ) {
			if ( load_data( bd, 12, ver) != 12 ) goto err;
			trp.v[j].x = ver[0];	trp.v[j].y = ver[1];	trp.v[j].z = ver[2];
			o_hcncrd(matr,&trp.v[j],&trp.v[j]);
		}
		if ( load_data( bd, 2, NULL) != 2 ) break; // goto err;

		if ( !put_tri(&trb, &trp, solid_checking)) {
			a_handler_err(AE_ERR_DATA);
			goto err1;
		}
	}
	num = (short)listho->num;
	if ( !end_tri_biand(&trb, listho) )	i = 0;
	else {
		i = (short)listho->num - num;
		hobj = listho->htail;
		for (j = 0; j < i; j++) {
			o_trans(hobj, matri);
			get_prev_item(hobj, &hobj);
		}
	}
end:
//	end_grad  (num_proc , bd->p_beg+bd->cur_buf);
	return i;
err:
	a_handler_err(AE_BAD_FILE);
err1:
	if (pass == 2) end_tri_biand(&trb, listho);
	i = 0;
	goto end;
}

