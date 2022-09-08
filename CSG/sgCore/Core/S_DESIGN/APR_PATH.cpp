#include "../sg.h"

sgFloat      c_h_tolerance = 0.1;    //   

static OSCAN_COD apr_geo_scan(hOBJ hobj,lpSCAN_CONTROL lpsc);

static  OSCAN_COD  apr_line      (lpOBJ obj,lpSCAN_CONTROL lpsc);
static  OSCAN_COD  apr_circle    (lpOBJ obj,lpSCAN_CONTROL lpsc);
static  OSCAN_COD  apr_arc       (lpOBJ obj,lpSCAN_CONTROL lpsc);
static  OSCAN_COD  apr_spline    (lpOBJ obj,lpSCAN_CONTROL lpsc);

static BOOL first;
static OSCAN_COD  (**apr_typeg)(lpOBJ obj,lpSCAN_CONTROL lpsc);

BOOL apr_path(lpVDIM vdim, hOBJ hobj)
{
	SCAN_CONTROL  sc;
	OSTATUS 			status;
	MNODE					mnode;
	D_POINT				first_point;

	apr_typeg = (OSCAN_COD(**)(lpOBJ obj,lpSCAN_CONTROL lpsc))GetMethodArray(OMT_APR_PATH);

	apr_typeg[OLINE]  = apr_line;		// OLINE
	apr_typeg[OCIRCLE]= apr_circle;	// OCIRCLE
	apr_typeg[OARC]   = apr_arc;		// OARC
	apr_typeg[OSPLINE]= apr_spline;	// OSPLINE

	if ( !init_vdim(vdim, sizeof(MNODE)) ) return FALSE;
	init_scan(&sc);
	sc.user_geo_scan  = apr_geo_scan;
	reinterpret_cast<VDIM* &>(sc.data)   = vdim;
	first = TRUE;
	if (o_scan(hobj,&sc) == OSFALSE) goto err;

	
	if (!get_status_path(hobj, &status)) goto err;
	if (status & ST_CLOSE) {
		if (!read_elem(vdim, 0, &mnode)) goto err;
		first_point = mnode.p;
		if (!read_elem(vdim, vdim->num_elem - 1, &mnode)) goto err;
		mnode.p = first_point;
		if (!write_elem(vdim, vdim->num_elem - 1, &mnode)) goto err;
	}

	return TRUE;
err:
	free_vdim(vdim);
	return FALSE;
}

static  OSCAN_COD apr_geo_scan(hOBJ hobj,lpSCAN_CONTROL lpsc)
{
	lpOBJ obj;
	BOOL cod;

	obj = (lpOBJ)hobj;
	cod = apr_typeg[lpsc->type](obj,lpsc);
	return (OSCAN_COD)cod;
}

#pragma argsused
static  OSCAN_COD apr_line(lpOBJ obj, lpSCAN_CONTROL lpsc)
{
	lpGEO_LINE g = (lpGEO_LINE)obj->geo_data;
	MNODE mnode;
	lpD_POINT p1,p2;
	OSTATUS constr1,constr2;

	if ( lpsc->status&ST_DIRECT ) {
		p1 = &g->v2; p2 = &g->v1;
		constr1 = (OSTATUS)(obj->status&ST_CONSTR2);
		constr2 = (OSTATUS)(obj->status&ST_CONSTR1);
	}
	else {
		p1 = &g->v1; p2 = &g->v2;
		constr1 = (OSTATUS)(obj->status&ST_CONSTR1);
		constr2 = (OSTATUS)(obj->status&ST_CONSTR2);
	}
	if ( first ) {
		first = FALSE;
		mnode.p   = *p1;
		mnode.num = (constr1) ? AP_CONSTR : AP_NULL;
		mnode.num |= AP_SOPR;
		if ( !add_elem((lpVDIM)lpsc->data,&mnode) ) return OSFALSE;
	}
	mnode.num = (constr2) ? AP_CONSTR : AP_NULL;
	mnode.num |= AP_SOPR;
	mnode.p = *p2;
	if ( !add_elem((lpVDIM)lpsc->data,&mnode) ) return OSFALSE;
	return OSTRUE;
}

#pragma argsused
static  OSCAN_COD apr_circle(lpOBJ obj, lpSCAN_CONTROL lpsc)
{
	register short j, i;
	lpGEO_CIRCLE circle = (lpGEO_CIRCLE)obj->geo_data;
	MNODE mnode;
	short beg,num,step;
	ARC_DATA    ad;

	mnode.num = AP_CONSTR;
	/*sgFloat tT = c_h_tolerance;
	if (circle->r<10.0 && circle->r>1.0)
		c_h_tolerance = circle->r/100.0;
	else 
		if (circle->r<1.0)
			c_h_tolerance = circle->r/10.0;*/
	num = o_init_ecs_arc(&ad,(lpGEO_ARC)circle, OECS_CIRCLE,c_h_tolerance);
//	c_h_tolerance = tT;
	i = num % 4;
	if (i) num += (4 - i);
	if ( lpsc->status&ST_DIRECT ) {
		beg = num; step = -1;
	} else {
		beg = 0; step = 1;
	}
	for (j = beg, i = 0; i <= num; j +=step, i++ ) {
		o_get_point_on_arc(&ad,(sgFloat)j / num, &mnode.p);
		if (!(i % (num / 4))) mnode.num |= AP_COND;
		if ( !add_elem((lpVDIM)lpsc->data,&mnode) ) return OSFALSE;
		mnode.num = AP_CONSTR;
	}
	return OSTRUE;
}

#pragma argsused
static  OSCAN_COD apr_arc(lpOBJ obj, lpSCAN_CONTROL lpsc)
{
	register short 	j, i;
	lpD_POINT			p1, p2;
	lpGEO_ARC 		arc = (lpGEO_ARC)&obj->geo_data;
	MNODE 				mnode;
	short 					beg,num,step;
	OSTATUS 			constr1,constr2;
	ARC_DATA    	ad;

	if (fabs(arc->angle) < eps_n) {	//  :   
		if ( lpsc->status&ST_DIRECT ) {
			p1 = &arc->ve; p2 = &arc->vb;
			constr1 = (OSTATUS)(obj->status&ST_CONSTR2);
			constr2 = (OSTATUS)(obj->status&ST_CONSTR1);
		}
		else {
			p1 = &arc->vb; p2 = &arc->ve;
			constr1 = (OSTATUS)(obj->status&ST_CONSTR1);
			constr2 = (OSTATUS)(obj->status&ST_CONSTR2);
		}
		if ( first ) {
			first = FALSE;
			mnode.p   = *p1;
			mnode.num = (constr1) ? AP_CONSTR : AP_NULL;
			mnode.num |= AP_SOPR;
			if ( !add_elem((lpVDIM)lpsc->data,&mnode) ) return OSFALSE;
		}
		mnode.num = (constr2) ? AP_CONSTR : AP_NULL;
		mnode.num |= AP_SOPR;
		mnode.p = *p2;
		if ( !add_elem((lpVDIM)lpsc->data,&mnode) ) return OSFALSE;
	} else {//   
		sgFloat tT = c_h_tolerance;
		/*if (arc->r<10.0 && arc->r>1.0)
			c_h_tolerance = arc->r/100.0;
		else 
			if (arc->r<1.0)
				c_h_tolerance = arc->r/10.0;*/
		num = o_init_ecs_arc(&ad, arc, OECS_ARC, c_h_tolerance);
		c_h_tolerance = tT;
		if ( lpsc->status&ST_DIRECT ) {
			beg = num; step = -1;
			constr1 = (OSTATUS)(obj->status&ST_CONSTR2);
			constr2 = (OSTATUS)(obj->status&ST_CONSTR1);
		} else {
			beg = 0; step = 1;
			constr1 = (OSTATUS)(obj->status&ST_CONSTR1);
			constr2 = (OSTATUS)(obj->status&ST_CONSTR2);
		}
		if ( first )  {
			first = FALSE;
			mnode.num = (constr1) ? AP_CONSTR : AP_NULL;
			mnode.num |= AP_SOPR;
			i = 0;
		} else {
			beg += step;
			i = 1;
			mnode.num = AP_CONSTR;
		}
		for (j = beg; i <= num; j +=step, i++ ) {
			if (i == num) {
				mnode.num = (constr2) ? AP_CONSTR : AP_NULL;
				mnode.num |= AP_SOPR;
			}
			o_get_point_on_arc(&ad,(sgFloat)j / num, &mnode.p);
			if ( !add_elem((lpVDIM)lpsc->data,&mnode) ) return OSFALSE;
			mnode.num = AP_CONSTR;
		}
	}
	return OSTRUE;
}

#pragma argsused
static  OSCAN_COD apr_spline(lpOBJ obj, lpSCAN_CONTROL lpsc)
{
	SPLY_DAT    	sply_dat;
	MNODE       	mnode;
	lpMNODE     	lpmnode;
	OSCAN_COD   	cod = OSFALSE;
	OSTATUS 			constr1,constr2;
	lpGEO_SPLINE	spline;
	BOOL					knot;

	spline = (lpGEO_SPLINE)obj->geo_data;
	if (spline->type & SPLY_CLOSE) {
		constr1 = constr2 = ST_CONSTR1;
	} else {
		if (lpsc->status & ST_DIRECT) {
			constr1 = (OSTATUS)(obj->status & ST_CONSTR2);
			constr2 = (OSTATUS)(obj->status & ST_CONSTR1);
		} else {
			constr1 = (OSTATUS)(obj->status & ST_CONSTR1);
			constr2 = (OSTATUS)(obj->status & ST_CONSTR2);
		}
	}

	if (obj->extendedClass==NULL || 
		(reinterpret_cast<sgCSpline*>(obj->extendedClass)->GetGeometry()==NULL) ||
		(reinterpret_cast<sgCSpline*>(obj->extendedClass)->GetGeometry()->GetPointsCount()==NULL) ||
		(reinterpret_cast<sgCSpline*>(obj->extendedClass)->GetGeometry()->GetPoints()==NULL))
	{
				if (!begin_use_sply((lpGEO_SPLINE)obj->geo_data, &sply_dat)) return OSFALSE;
				get_first_sply_point(&sply_dat, c_h_tolerance,
														((lpsc->status & ST_DIRECT) ? TRUE : FALSE),
														&mnode.p);
				if (first) {
					first = FALSE;
					mnode.num = (constr1) ? AP_CONSTR : AP_NULL;
					mnode.num |= AP_SOPR;
					if (!add_elem((lpVDIM)lpsc->data,&mnode)) goto err;
				}
				mnode.num = AP_CONSTR;
				while (get_next_sply_point(&sply_dat, &mnode.p)){
					knot = FALSE;
			//  while(get_next_sply_point_tolerance(&sply_dat, &mnode.p, &knot)) {
					if (knot) mnode.num |= AP_COND;
					if (!add_elem((lpVDIM)lpsc->data,&mnode)) goto err;
					if (knot) mnode.num &= ~AP_COND;
				}
				if ( (lpmnode = (MNODE*)get_elem((lpVDIM)lpsc->data,
													((lpVDIM)lpsc->data)->num_elem -1)) == NULL) goto err;
				lpmnode->num = (constr2) ? AP_CONSTR : AP_NULL;
				lpmnode->num |= AP_SOPR;
				cod = OSTRUE;
			err:
				end_use_sply( (lpGEO_SPLINE)obj->geo_data, &sply_dat );
	}
	else
	{
		    const SG_SPLINE* spl_geo = reinterpret_cast<sgCSpline*>(obj->extendedClass)->GetGeometry();
			const int pnts_cnt = spl_geo->GetPointsCount();
			const SG_POINT* pnts = spl_geo->GetPoints();

			if ((lpsc->status & ST_DIRECT))
				memcpy(&mnode.p,&pnts[pnts_cnt-1],sizeof(D_POINT));
			else
				memcpy(&mnode.p,&pnts[0],sizeof(D_POINT));
			if (first) {
				first = FALSE;
				mnode.num = (constr1) ? AP_CONSTR : AP_NULL;
				mnode.num |= AP_SOPR;
				if (!add_elem((lpVDIM)lpsc->data,&mnode)) goto err;
			}
			mnode.num = AP_CONSTR;
			if ((lpsc->status & ST_DIRECT))
			{
				for (int i=pnts_cnt-2;i>=0;i--)
				{
					memcpy(&mnode.p,&pnts[i],sizeof(D_POINT));
					knot = FALSE;
					if (knot) mnode.num |= AP_COND;
					if (!add_elem((lpVDIM)lpsc->data,&mnode))
						return OSFALSE;
					if (knot) mnode.num &= ~AP_COND;
				}
			}
			else
			{
				for (int i=1;i<=pnts_cnt-1;i++)
				{
					memcpy(&mnode.p,&pnts[i],sizeof(D_POINT));
					knot = FALSE;
					if (knot) mnode.num |= AP_COND;
					if (!add_elem((lpVDIM)lpsc->data,&mnode))
						return OSFALSE;
					if (knot) mnode.num &= ~AP_COND;
				}
			}
			if ( (lpmnode = (MNODE*)get_elem((lpVDIM)lpsc->data,
				((lpVDIM)lpsc->data)->num_elem -1)) == NULL) 
					return OSFALSE;
			lpmnode->num = (constr2) ? AP_CONSTR : AP_NULL;
			lpmnode->num |= AP_SOPR;
			cod = OSTRUE;
	}
	return cod;
}
