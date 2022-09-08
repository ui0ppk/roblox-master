#include "../sg.h"

static void zc_exit(void);

void zc_init(lpZCUT cut, sgFloat z)
//    
{
	memset(cut, 0, sizeof(ZCUT));
	init_vdim(&cut->vdv, sizeof(ZCV));
	init_vdim(&cut->vdl, sizeof(ZCP));
	cut->z = z;
}

void zc_free(lpZCUT cut)
//    
{
	free_vdim(&cut->vdv);
	free_vdim(&cut->vdl);
	memset(cut, 0, sizeof(ZCUT));
}

IDENT_V zcf_create_new_face(lpZCUT cut, UCHAR status)
//   cut    
//     0    
{
ZCP      f;
IDENT_V  id;

	memset(&f, 0, sizeof(ZCP));
	f.status = status;
	if(!il_add_elem(&cut->vdl, &cut->freell, &cut->flisth, &f, 0, &id)){
		if(id < 0) return 0;
		else       zc_exit();
	}
	return id;
}

IDENT_V zcp_create_new_path(lpZCUT cut, IDENT_V idf, UCHAR status)
//   idf   cut    
//  hole = TRUE -     -   
//     0    
{
ZCP      p;
ZCP      f;
IDENT_V  idp;

	if(!read_elem(&cut->vdl, idf - 1, &f)) zc_exit();
	memset(&p, 0, sizeof(ZCP));
	p.idf = idf;
	p.status = status;
	if(!il_add_elem(&cut->vdl, &cut->freell, &f.listh, &p, 0, &idp)){
		if(idp < 0) return 0;
		else        zc_exit();
	}
	if(!write_elem(&cut->vdl, idf - 1, &f)) zc_exit();
	return idp;
}

IDENT_V zcv_add_to_tail(lpZCUT cut, IDENT_V idp, void* v, sgFloat k)
//   cut   v    idp
//      0    
{
ZCP      p;
ZCV      vi;
IDENT_V  id, idv;

	if(!read_elem(&cut->vdl, idp - 1, &p)) zc_exit();
	id = (p.status & ZC_CLOSE) ? p.listh.tail : 0;
	memcpy(&vi.v, v, sizeof(D_PLPOINT));
	vi.k = k;
	if(!il_add_elem(&cut->vdv, &cut->freelv, &p.listh, &vi, id, &idv)){
		if(idv < 0)  return 0;
		else         zc_exit();
	}
	if(!write_elem(&cut->vdl, idp - 1, &p)) zc_exit();
	return idv;
}

void zcp_close(lpZCUT cut, IDENT_V idp)
//   cut   idp
{
ZCP p;

	if(!read_elem(&cut->vdl, idp - 1, &p))  zc_exit();
	if(!il_close(&cut->vdv, &p.listh)) zc_exit();
	if(p.listh.num > 1) p.status |= ZC_CLOSE;
	if(!write_elem(&cut->vdl, idp - 1, &p)) zc_exit();
}

UCHAR zcp_status(lpZCUT cut, IDENT_V idp)
//   cut      idp
{
ZCP p;

	if(!read_elem(&cut->vdl, idp - 1, &p))  zc_exit();
	return p.status;
}

void zcp_set_status(lpZCUT cut, IDENT_V idp, UCHAR status)
//   cut      idp
{
lpZCP p;

	if( (p = (ZCP *)get_elem(&cut->vdl, idp - 1))== NULL )  zc_exit();
	p->status = status;
}

IDENT_V zcf_get_first_face(lpZCUT cut)
//   cut    
//     0,   
{
  return cut->flisth.head;
}

IDENT_V zcf_get_next_face(lpZCUT cut, IDENT_V idf)
//   cut   ,   idf
//     0,  idf -  
{
  if(idf)
    if(!il_get_next_item(&cut->vdl, idf, &idf)) zc_exit();
  return idf;
}

IDENT_V zcp_get_first_path_in_face(lpZCUT cut, IDENT_V idf)
//   idf  cut    
//      
//     0,   
{
lpZCP    f;
IDENT_V  idp;

	if( (f = (ZCP *)get_elem(&cut->vdl, idf - 1)) == NULL ) zc_exit();
  idp = f->listh.head;
  return idp;
}


IDENT_V zcp_get_next_path(lpZCUT cut, IDENT_V idp)
//    cut,   idp
//  0,  idp -     
{
  if(idp)
    if(!il_get_next_item(&cut->vdl, idp, &idp)) zc_exit();
  return idp;
}


IDENT_V zcv_get_first_point(lpZCUT cut, IDENT_V idp, void* v, sgFloat* k)
//   v    idp   cut
//     0,   

{
lpZCP    p;
IDENT_V  id;

	if( (p = (ZCP *)get_elem(&cut->vdl, idp - 1)) == NULL )   zc_exit();
	id = p->listh.head;
	if(id) zcv_get_point(cut, id, v, k);
	return id;
}


IDENT_V zcv_get_next_point(lpZCUT cut, IDENT_V idv, void* v, sgFloat* k)
//   v  ,   idv   cut
//      0,  idv -   

{
	if(idv){
		if(!il_get_next_item(&cut->vdv, idv, &idv)) zc_exit();
		if(idv) zcv_get_point(cut, idv, v, k);
	}
	return idv;
}


IDENT_V zcv_get_prev_point(lpZCUT cut, IDENT_V idv, void* v, sgFloat* k)
//   v  ,   idv   cut
//      0,  idv -   
//    -     .   
// ,    , ,   
// ,       
// (. zcv_get_first_point)

{
	if(idv){
		if(!il_get_prev_item(&cut->vdv, idv, &idv)) zc_exit();
		if(idv) zcv_get_point(cut, idv, v, k);
	}
	return idv;
}


IDENT_V zcv_get_point(lpZCUT cut, IDENT_V idv, void* v, sgFloat* k)
//         idv   cut
//      0,  idv - 
//  

{
lpZCV vi;

	if(idv){
		if( (vi = (ZCV *)get_elem(&cut->vdv, idv - 1)) == NULL ) zc_exit();
		memcpy(v, &vi->v, sizeof(D_PLPOINT));
		*k = vi->k;
		idv = vi->list.next;
	}
	return idv;
}
void zcv_overwrite_point(lpZCUT cut, IDENT_V idv, void* v, sgFloat k)
//        idv   cut
{
	lpZCV vi;

	if(idv){
		if( (vi = (ZCV *)get_elem(&cut->vdv, idv - 1)) == NULL ) zc_exit();
		memcpy(&vi->v, v, sizeof(D_PLPOINT));
		vi->k = k;
	}
}

int zcp_get_num_point(lpZCUT cut, IDENT_V idp)
//   cut      idp
{
ZCP p;
  if(!read_elem(&cut->vdl, idp - 1, &p))  zc_exit();
  return p.listh.num;
}

UCHAR zc_get_status(lpZCUT cut, IDENT_V idp)
//      idp
{
ZCP p;
  if(!read_elem(&cut->vdl, idp - 1, &p))  zc_exit();
  return p.status;
}


void zcv_detach_point(lpZCUT cut, IDENT_V idp, IDENT_V idv)
//   cut   idv   idp
{
ZCP p;

  if(!read_elem(&cut->vdl, idp - 1, &p))                 zc_exit();
  if(!il_detach_item(&cut->vdv, &p.listh, idv))          zc_exit();
  if(!il_attach_item_tail(&cut->vdv, &cut->freelv, idv)) zc_exit();
  if((p.status & ZC_CLOSE) && p.listh.num <= 1)
    p.status -= ZC_CLOSE;
	if(!write_elem(&cut->vdl, idp - 1, &p))                zc_exit();
}

IDENT_V zcv_insert_point(lpZCUT cut, IDENT_V idp, IDENT_V idv, void* v, sgFloat k)
//   cut   v   idp   idv
//  idv = 0,     
//      0    
{
ZCP      p;
ZCV      vi;
IDENT_V  id;

	if(!read_elem(&cut->vdl, idp - 1, &p)) zc_exit();
	if(idv) id = idv;
	else    id = (p.status & ZC_CLOSE) ? p.listh.tail : 0;
	memcpy(&vi.v, v, sizeof(D_PLPOINT));
  vi.k = k;
	if(!il_add_elem(&cut->vdv, &cut->freelv, &p.listh, &vi, id, &idv)){
		if(idv < 0)  return 0;
		else         zc_exit();
	}
	if(!write_elem(&cut->vdl, idp - 1, &p)) zc_exit();
	return idv;
}

void zcp_free_path(lpZCUT cut, IDENT_V idp)
//      idp   cut
{
ZCP      p;

  if(!read_elem(&cut->vdl, idp - 1, &p)) zc_exit();
  if(!il_attach_list_befo(&cut->vdv, &cut->freelv, 0, &p.listh)) zc_exit();
  il_init_listh(&p.listh);
  if(p.status & ZC_CLOSE) p.status -= ZC_CLOSE;
  if(!write_elem(&cut->vdl, idp - 1, &p)) zc_exit();
}

void zcp_delete_path(lpZCUT cut, IDENT_V idf, IDENT_V idp)
//   idf   cut   idp
{
ZCP      f;

  zcp_free_path(cut, idp);
  if(!read_elem(&cut->vdl, idf - 1, &f))                 zc_exit();
  if(!il_detach_item(&cut->vdl, &f.listh, idp))          zc_exit();
  if(!il_attach_item_tail(&cut->vdl, &cut->freell, idp)) zc_exit();
  if(!write_elem(&cut->vdl, idf - 1, &f))                zc_exit();
}

static void zc_exit(void)
{
	exit(1);
//  SG_save_and_exit(INTERNAL_ERROR, "ZCUTS");
}
