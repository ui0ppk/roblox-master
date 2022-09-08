#include "../sg.h"
//#include <o_util.h>

static  VADR alloc_page(void);
static  BOOL add_dim_page(lpVDIM dim);
static  VADR get_hpage(lpVDIM dim, short num_page);

BOOL init_vdim(lpVDIM dim, short size_elem)
{
  memset(dim,0,sizeof(VDIM));
  dim->num_elem_page = SIZE_VPAGE/size_elem;
  dim->size_elem     = size_elem;
	return TRUE;
}

BOOL add_elem(lpVDIM dim,void * buf)
{
	short num;
	short num_page;
	char * p;
  VADR  hpage;

	num = (short)(dim->num_elem %  dim->num_elem_page);
	if ( !num )
		if ( !add_dim_page(dim) ) return FALSE;
	num_page = (short)(dim->num_elem / dim->num_elem_page);
	hpage = get_hpage(dim,num_page);
  p   = (char *)hpage + num*dim->size_elem;
  if(buf)
	  memcpy(p,buf,dim->size_elem);
  else
	  memset(p,0,dim->size_elem);
	dim->num_elem++;
	return TRUE;
}

BOOL add_dim_elem(lpVDIM dim,void * buf,short number)
{
  short num,num_page,num_tmp;
	char * p;
  VADR      hpage;

once:
	num = (short)(dim->num_elem %  dim->num_elem_page);
	num_page = (short)(dim->num_elem / dim->num_elem_page);
  if ( !num )  if ( !add_dim_page(dim) ) return FALSE;
	hpage = get_hpage(dim,num_page);
  p   = (char *)hpage + num*dim->size_elem;
  num_tmp =  dim->num_elem_page - num - number;
  if ( num_tmp > number ) num_tmp = number;
	memcpy(p,buf,dim->size_elem*num_tmp);
  number -= num_tmp;
  dim->num_elem += num_tmp;
  if ( number ) {
    reinterpret_cast<char * &>(buf) += num_tmp*dim->size_elem;
//nb     num = 0;
    goto once;
  }
	return TRUE;
}

BOOL read_dim_elem(lpVDIM dim,int num,void * buf,short number)
{
	short num_page,num_tmp;
	char * p;
	VADR  hpage;

once:
	num  %= dim->num_elem_page;
	num_page = (short)(dim->num_elem / dim->num_elem_page);
	hpage = get_hpage(dim,num_page);
	p   = (char *)hpage + (short)(num)*dim->size_elem;
	num_tmp =  dim->num_elem_page - (short)num - number;
  if ( num_tmp > number ) num_tmp = number;
  memcpy(p,buf,dim->size_elem*num_tmp);
  number -= num_tmp;
  dim->num_elem += num_tmp;
  if ( number ) {
    reinterpret_cast<char * &>(buf) += num_tmp*dim->size_elem;
    num = 0;
    goto once;
  }
	return TRUE;
}
BOOL write_elem(lpVDIM dim,int num, void * data)
{
	short num_page;
	char * p;
	VADR  hpage;

	if ( num >= dim->num_elem ) return FALSE;
	num_page = (short)(num / dim->num_elem_page);
	num     %= dim->num_elem_page;
	hpage = get_hpage(dim,num_page);
	p = (char *)hpage + (short)(num)*dim->size_elem;
  if(data)
	  memcpy(p,data,dim->size_elem);
  else
	  memset(p,0,dim->size_elem);
	return TRUE;
}
void * get_elem(lpVDIM dim, int num)
{
	short num_page;
	char  *p;
	VADR  hpage;

	if ( num >= dim->num_elem ) return NULL;
	num_page = (short)(num / dim->num_elem_page);
	num     %= dim->num_elem_page;
	hpage = get_hpage(dim,num_page);
	p = (char *)hpage;
	return ( p + (short)num*dim->size_elem);
}

BOOL begin_rw(lpVDIM dim, int num)
{
	dim->elem       = NULL;
	dim->cur_elem   = num;
	return TRUE;
}

void * get_next_elem(lpVDIM dim)
{
	short num, num_page;

  if ( dim->cur_elem >= dim->num_elem )  goto err;
	if ( !(dim->cur_elem % dim->num_elem_page) || !dim->elem) {
		num_page = (short)(dim->cur_elem / dim->num_elem_page);
		num      = (short)(dim->cur_elem % dim->num_elem_page);
		dim->hpagerw = get_hpage(dim,num_page);
    if ( !dim->hpagerw ) goto err;
		dim->elem = (char *)dim->hpagerw;
        dim->elem += num*dim->size_elem;
		goto m;
	}
    dim->elem += dim->size_elem;
m:
	dim->cur_elem++;
	return dim->elem;
err:
  end_rw(dim);
  return NULL;
}
void end_rw(lpVDIM dim)
{
	dim->elem = NULL;
}
BOOL read_elem(lpVDIM dim, int num, void * buf)
{
	short num_page;
	char * p;
	VADR  hpage;

	if ( num >= dim->num_elem ) return FALSE;
	num_page = (short)(num / dim->num_elem_page);
	num     %= dim->num_elem_page;
	hpage = get_hpage(dim,num_page);
	p = (char *)hpage + (short)(num)*dim->size_elem;
	memcpy(buf,p,dim->size_elem);
	return TRUE;
}
void free_vdim(lpVDIM dim)
{
  delete_elem_list(dim,0L);
	memset(dim,0,sizeof(VDIM));
}
static  BOOL add_dim_page(lpVDIM dim)
{
	VADR hpage;
	VADR * p;
	short num_page;

	num_page = (short)(dim->num_elem / dim->num_elem_page);
	if ( (hpage = alloc_page()) == NULL ) return FALSE;
	if ( num_page < NUM_VDIM_PAGE ) {
		dim->page[num_page] = hpage;
		return TRUE;
	}
	if ( !dim->hpage_a )  {  //   
		if ( (dim->hpage_a = alloc_page()) == NULL ) goto err;
	}
	num_page -= NUM_VDIM_PAGE;
	if ( num_page >= SIZE_VPAGE/sizeof(VADR) ) {
		u_handler_err(UE_OVERFLOW_VDIM);
		return FALSE;
	}
	p = (VADR*)dim->hpage_a;
	p[num_page] = hpage;
	return TRUE;
err:
	SGFree(hpage);
	return FALSE;
}
static  VADR alloc_page(void)
{
	void * p;
	VADR			hpage;

	if ( (hpage = SGMalloc(SIZE_VPAGE)) == NULL ) return NULL;
	p = (void*)hpage;
	memset(p, 0, SIZE_VPAGE);
	return hpage;
}
static  VADR get_hpage(lpVDIM dim, short num_page)
{
  if ( num_page < NUM_VDIM_PAGE ) return ( dim->page[num_page] );
  else {
		VADR hpage;
		VADR * pa;
    num_page -= NUM_VDIM_PAGE;
    pa = (VADR*)dim->hpage_a;
    hpage = pa[num_page];
		return hpage;
  }
}
BOOL find_elem(lpVDIM dim,int *num_first,
      BOOL (*func)(void * elem, void * user_data), void * user_data)
{
  void * elem;
  if ( !begin_rw( dim, *num_first) ) return FALSE;
	while ( (elem=get_next_elem(dim)) != NULL ) {
    if ( !func(elem, user_data) ) continue;
    *num_first = dim->cur_elem - 1;
    end_rw(dim);
    return TRUE;
  }
  end_rw(dim);
  return FALSE;
}

void delete_elem_list(lpVDIM dim,int number)
{
	short   i, num_page;

	if ( number >= dim->num_elem )  return;
	num_page = (short)(number / dim->num_elem_page);


	if ( number % dim->num_elem_page )   num_page++;
	for ( i=num_page; i<NUM_VDIM_PAGE; i++) {
		if ( !dim->page[i] ) break;
		SGFree(dim->page[i]);
		dim->page[i] = NULL;
	}
	if ( dim->hpage_a ) {
		VADR * pa;
		num_page = (short)(dim->num_elem / dim->num_elem_page + 1);
		num_page -=NUM_VDIM_PAGE;
		pa = (VADR *)dim->hpage_a;
		for ( i=0; i<num_page; i++) {
			if ( !pa[i] ) continue;
			SGFree(pa[i]);
			pa[i] = NULL;
		}
		SGFree(dim->hpage_a);
    dim->hpage_a = NULL;
	}
	dim->num_elem = number;
}


typedef struct {
	VADR			hpage;
	int				ind;
	int				ind_max;
	void *	data;
}SORT_ELEM;

BOOL	sort_vdim(lpVDIM vdim, VDFCMP func)
//BOOL sort_vdim(lpVDIM vdim,short (*func)(const void * elem1,
//																				const void * elem2))
{
	short   i,j,num_page,num_elem_last, tmp, num_page1;
	char	* p;
	SORT_ELEM* fe;
	VDIM	vd_new;
	VADR * pa;

	if (vdim->num_elem == 0) return TRUE;
	num_page = (short)(vdim->num_elem / vdim->num_elem_page);
	num_elem_last = (short)(vdim->num_elem % vdim->num_elem_page);
	if ( num_page < NUM_VDIM_PAGE )	tmp = num_page;
	else														tmp = NUM_VDIM_PAGE;
	for ( i= 0; i<tmp; i++) {
		if ( !vdim->page[i] ) return FALSE;
		p = (char*)vdim->page[i];
		qsort(p,vdim->num_elem_page,vdim->size_elem,func);
	}
	if ( num_elem_last && tmp < NUM_VDIM_PAGE) {
		p = (char*)vdim->page[i];
		qsort(p,num_elem_last,vdim->size_elem,func);
		goto m_merge;
	}
	if ( vdim->hpage_a ) {
		num_page1 = num_page - NUM_VDIM_PAGE;
		pa = (VADR*)vdim->hpage_a;
		for ( i=0; i<num_page1; i++) {
			if ( !pa[i] ) continue;
			p = (char*)pa[i];
			qsort(p,vdim->num_elem_page,vdim->size_elem,func);
		}
		//   
		if ( num_elem_last )  {
			p = (char*)pa[i];
			qsort(p,num_elem_last,vdim->size_elem,func);
		}
	}
m_merge:  //   
	if ( num_elem_last )	num_page++;
	if ( num_page == 1 ) return TRUE;
	if ( 0 ==(fe = (SORT_ELEM*)SGMalloc( num_page*(sizeof(SORT_ELEM)+vdim->size_elem))) )
		return FALSE;
	if ( num_page < NUM_VDIM_PAGE )	tmp = num_page;
	else					 									tmp = NUM_VDIM_PAGE;
	for ( i= 0; i<tmp; i++) {
		fe[i].hpage = vdim->page[i];
		fe[i].ind	  	= 0;
		fe[i].ind_max	= vdim->num_elem_page;
		fe[i].data		= (char*)&fe[num_page]+i*vdim->size_elem;
		p = (char*)vdim->page[i];
		memcpy(fe[i].data,p,vdim->size_elem);
	}
	if ( vdim->hpage_a ) {
		j = i;
		num_page1 = num_page - NUM_VDIM_PAGE;
		pa = (VADR*)vdim->hpage_a;
		for ( i=0; i<num_page1; i++,j++ ) {
			p = (char*)pa[i];
			fe[j].hpage		= pa[i];
			fe[j].ind	  	= 0;
			fe[j].ind_max	= vdim->num_elem_page;
			fe[j].data		= (char*)&fe[num_page]+j*vdim->size_elem;
			memcpy(fe[j].data,p,vdim->size_elem);
		}
	}
	//   
	if ( !num_elem_last ) fe[num_page-1].ind_max = vdim->num_elem_page;
	else	fe[num_page-1].ind_max = (short)(vdim->num_elem % vdim->num_elem_page);
	vd_new = *vdim;
	if ( !init_vdim(&vd_new, vdim->size_elem)) goto err;

	while (1) {
		//   
		for ( j=0; j< num_page; j++) {
			if ( fe[j].ind == fe[j].ind_max ) continue;	//   
			break;
		}
		if ( j == num_page ) break;  //  
		tmp = j;
		for ( i=j+1; i< num_page; i++) {
			if ( fe[i].ind == fe[i].ind_max ) continue;	//   
			if ( func(fe[tmp].data,fe[i].data) > 0 ) tmp = i;
		}
		if ( !add_elem(&vd_new,fe[tmp].data)) goto err;
		fe[tmp].ind++;
		if ( fe[tmp].ind == fe[tmp].ind_max ) continue;
		p = (char*)fe[tmp].hpage;
		memcpy(fe[tmp].data,p + fe[tmp].ind*vdim->size_elem, vdim->size_elem);
	}
	SGFree(fe);
	free_vdim(vdim);
	*vdim = vd_new;
	return TRUE;
err:
	free_vdim(&vd_new);
	SGFree(fe);
	return FALSE;
}

int	bsearch_vdim(const void *key,lpVDIM vdim, VDFCMP func)
{
	short   i,num_page,num_elem_last, tmp;
	char	* p, * p1;
	VADR * pa;
	int	ind = 0;

	num_page = (short)(vdim->num_elem / vdim->num_elem_page);
	num_elem_last = (short)(vdim->num_elem % vdim->num_elem_page);
	if ( num_page < NUM_VDIM_PAGE )	tmp = num_page;
	else														tmp = NUM_VDIM_PAGE;
	for ( i= 0; i<tmp; i++,ind++) {
		if ( !vdim->page[i] ) return FALSE;
		p = (char*)vdim->page[i];
		p1 = (char*)bsearch(key,p,vdim->num_elem_page,vdim->size_elem,func);
		if ( p1 ) goto find1;
	}
	if ( num_elem_last && tmp < NUM_VDIM_PAGE) {
		p = (char*)vdim->page[i];
		p1 = (char*)bsearch(key,p,num_elem_last,vdim->size_elem,func);
		if ( p1 ) goto find1;
	}
	if ( vdim->hpage_a ) {
		num_page -= NUM_VDIM_PAGE;
		pa = (VADR*)vdim->hpage_a;
		for ( i=0; i<num_page; i++,ind++) {
			if ( !pa[i] ) continue;
			p = (char*)pa[i];
			p1 = (char*)bsearch(key,p,vdim->num_elem_page,vdim->size_elem,func);
			if ( p1 ) goto find1;
		}
		//    
		if ( num_elem_last )  {
			p = (char*)pa[i];
			p1 = (char*)bsearch(key,p,num_elem_last,vdim->size_elem,func);
			if ( p1 ) goto find1;
		}
	}
	return -1;
find1:
	ind *= vdim->num_elem_page;
	ind += (p1 - p)/ vdim->size_elem;
	return ind;
}

