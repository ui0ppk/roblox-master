#include "..//Core//sg.h"

#include <vector>

static   void ExtractPointsAndKnotsFromSplineDAT(SPLY_DAT* splD, 
											SG_POINT*&    pnts,
											int& pnts_cnt,
											SG_POINT*&    knts,
											int& knts_cnt)
{

	if (pnts)
	{
		delete [] pnts;
		pnts = NULL;
		pnts_cnt=0;
	}
	if (knts)
	{
		delete [] knts;
		knts = NULL;
		knts_cnt=0;
	}

	std::vector<SG_POINT>   Pnt_vector;
	Pnt_vector.reserve(50);

	BOOL    	knot;
	D_POINT 	pnt;
	SG_POINT    sPnt;
	
	get_first_sply_point(splD, 0., FALSE, &pnt);
	memcpy(&sPnt,&pnt,sizeof(SG_POINT));
	Pnt_vector.push_back(sPnt);
	while( get_next_sply_point_and_knot(splD, &pnt, &knot) ) 
	{
		memcpy(&sPnt,&pnt,sizeof(SG_POINT));
		Pnt_vector.push_back(sPnt);
	}

	pnts_cnt = Pnt_vector.size();

	pnts = new SG_POINT[pnts_cnt];

	memcpy(pnts, &Pnt_vector[0], sizeof(SG_POINT)*pnts_cnt);

	Pnt_vector.clear();
	Pnt_vector.reserve(30);

	short     i;
	sgFloat 	  w;
	
	if( splD->sdtype & SPL_APPR ) 
	{      // .
		Create_3D_From_4D( &splD->P[0], (sgFloat *)&pnt, &w );
		memcpy(&sPnt,&pnt,sizeof(SG_POINT));
		Pnt_vector.push_back(sPnt);
		for( i=0; i<splD->nump-1; i++) 
		{
			Create_3D_From_4D( &splD->P[i+1], (sgFloat *)&pnt, &w );
			memcpy(&sPnt,&pnt,sizeof(SG_POINT));
			Pnt_vector.push_back(sPnt);
		}
	}

	knts_cnt = Pnt_vector.size();

	knts = new SG_POINT[knts_cnt];

	memcpy(knts, &Pnt_vector[0], sizeof(SG_POINT)*knts_cnt);

	Pnt_vector.clear();
}

SG_SPLINE::SG_SPLINE()
{
	m_handle = NULL;
	m_points = NULL;
	m_points_count=0;
	m_knots = NULL;
	m_knots_count=0;
}

SG_SPLINE::SG_SPLINE(void* SplObjHndl)
{
	m_handle = NULL;
	m_points = NULL;
	m_points_count=0;
	m_knots = NULL;
	m_knots_count=0;

	assert(SplObjHndl);

	lpOBJ pobj = (lpOBJ)SplObjHndl;

	m_handle = SGMalloc(sizeof(SPLY_DAT));
	//----------------------------------->
	OBJ        oldobj;
	GEO_SPLINE geo_sply;
	get_simple_obj(SplObjHndl, &oldobj, &geo_sply);

	short 		 type_s = (geo_sply.type & SPLY_APPR) ? SPL_APPR : SPL_INT;;
	//     
	if(!init_sply_dat(SPL_NEW, geo_sply.degree, type_s, (SPLY_DAT*)m_handle))
	{
		assert(0);
		global_sg_error = SG_ER_INTERNAL;
		return;
	}
	
	unpack_geo_sply((lpGEO_SPLINE)(pobj->geo_data), (SPLY_DAT*)m_handle);

	ExtractPointsAndKnotsFromSplineDAT((SPLY_DAT*)m_handle, 
		m_points,m_points_count,	m_knots,m_knots_count   );
}

void  SG_SPLINE::Recalc()
{
	assert(m_handle);
	ExtractPointsAndKnotsFromSplineDAT((SPLY_DAT*)m_handle, 
		m_points,m_points_count,	m_knots,m_knots_count   );
}

SG_SPLINE::~SG_SPLINE()
{
	assert(m_handle);

	lpSPLY_DAT   spl = (lpSPLY_DAT)m_handle;
	free_sply_dat(spl);
	SGFree(m_handle);

	if (m_points)
	{
		delete [] m_points;
		m_points = NULL;
		m_points_count=0;
	}
	if (m_knots)
	{
		delete [] m_knots;
		m_knots = NULL;
		m_knots_count=0;
	}
}

SG_SPLINE*   SG_SPLINE::Create()
{
	SG_SPLINE*  res = new SG_SPLINE();

	res->m_handle = SGMalloc(sizeof(SPLY_DAT));
	//----------------------------------->
	short degree = 2;
	short type_s = SPL_APPR; // 
	//----------------------------------->
	if(!init_sply_dat(SPL_NEW, degree, type_s, (SPLY_DAT*)res->m_handle))
	{
		global_sg_error = SG_ER_INTERNAL;
		return NULL;
	}
	global_sg_error = SG_ER_SUCCESS;
	return res;
}

void   SG_SPLINE::Delete(SG_SPLINE* splG)
{
	assert(splG);
	delete splG;
}	

bool   SG_SPLINE::AddKnot(const SG_POINT& pnt, int nmbr)
{
	if (nmbr>=MAX_POINT_ON_SPLINE)
		return false;

	spl_work=CRE_SPL;
	D_POINT pnt_old;
	memcpy(&pnt_old, &pnt, sizeof(SG_POINT));
	if (nmbr<0 || nmbr>m_knots_count)
	{
		assert(0);
		global_sg_error = SG_ER_BAD_ARGUMENT_UNKNOWN;
		return false;
	}

	add_sply_point((SPLY_DAT*)m_handle, &pnt_old, nmbr);

	ExtractPointsAndKnotsFromSplineDAT((SPLY_DAT*)m_handle, 
		m_points,m_points_count,	m_knots,m_knots_count   );

	global_sg_error = SG_ER_SUCCESS;
	return true;
}

bool  SG_SPLINE::MoveKnot(int nmbr, const SG_POINT& pnt)
{
	D_POINT pnt_old;
	memcpy(&pnt_old, &pnt, sizeof(SG_POINT));
	if (nmbr<0 || nmbr>m_knots_count)
	{
		assert(0);
		global_sg_error = SG_ER_BAD_ARGUMENT_UNKNOWN;
		return false;
	}

	move_sply_point((SPLY_DAT*)m_handle, nmbr, &pnt_old);

	ExtractPointsAndKnotsFromSplineDAT((SPLY_DAT*)m_handle, 
		m_points,m_points_count,	m_knots,m_knots_count   );
	global_sg_error = SG_ER_SUCCESS;
	return true;
}

bool  SG_SPLINE::DeleteKnot(int nmbr)
{
	spl_work=CRE_SPL;
	if (nmbr<0 || nmbr>m_knots_count)
	{
		assert(0);
		global_sg_error = SG_ER_BAD_ARGUMENT_UNKNOWN;
		return false;
	}

	delete_sply_point((SPLY_DAT*)m_handle, (short)nmbr);

	ExtractPointsAndKnotsFromSplineDAT((SPLY_DAT*)m_handle, 
		m_points,m_points_count,	m_knots,m_knots_count   );
	global_sg_error = SG_ER_SUCCESS;
	return true;
}

bool  SG_SPLINE::IsClosed()  const
{
	if (((SPLY_DAT*)m_handle)->sdtype & SPL_CLOSE)
		return true;
	else
		return false;
}

bool  SG_SPLINE::Close()
{
	if (!close_sply((SPLY_DAT*)m_handle))
	{
		global_sg_error = SG_ER_INTERNAL;
		return false;
	}
	ExtractPointsAndKnotsFromSplineDAT((SPLY_DAT*)m_handle, 
		m_points,m_points_count,	m_knots,m_knots_count   );
	global_sg_error = SG_ER_SUCCESS;
	return true;
}

bool  SG_SPLINE::UnClose(int interv)
{
	if(!break_close_sply((SPLY_DAT*)m_handle,(short)interv))
	{
		global_sg_error = SG_ER_INTERNAL;
		return false;
	}
	ExtractPointsAndKnotsFromSplineDAT((SPLY_DAT*)m_handle, 
		m_points,m_points_count,	m_knots,m_knots_count   );
	global_sg_error = SG_ER_SUCCESS;
	return true;
}


const    SG_POINT*   SG_SPLINE::GetPoints() const
{
	return  m_points;
}

int  SG_SPLINE::GetPointsCount() const
{
	return m_points_count;
}

const    SG_POINT*   SG_SPLINE::GetKnots() const
{
	return m_knots;
}

int  SG_SPLINE::GetKnotsCount() const
{
	return m_knots_count;
}





static bool  is_spline_on_one_line(const SG_SPLINE* splG)
{
	const SG_POINT* pnts =  splG->GetKnots();
	const int       pntsCnt = splG->GetKnotsCount();
	
	if (pntsCnt<=2)
		return true;

	SG_POINT p1 = pnts[0];
	SG_POINT p2 = pnts[1];

	for (int i=2;i<pntsCnt;i++)
		if (!sgSpaceMath::IsPointsOnOneLine(p1,p2,pnts[i]))
			return false;

	return true;
}

sgCSpline::sgCSpline():sgC2DObject()
{
	m_spline_geo = NULL;
}


bool	sgCSpline::ApplyTempMatrix()
{
	if (!sgCObject::ApplyTempMatrix())
		return false;
	if (m_spline_geo)
	{
		delete m_spline_geo;
		m_spline_geo = NULL;
		m_spline_geo = new SG_SPLINE(GetObjectHandle(this));

		lpOBJ obj = (lpOBJ)GetObjectHandle(this);
		if (obj->status & ST_FLAT)
		{
			D_PLANE plane;

	      // ROBLOX change
          //#if (SG_CURRENT_PLATFORM==SG_PLATFORM_IOS)
			sgFloat savingEpsD = eps_d;
			eps_d *=100.0;    //  fix for sgFloat = float  (not double )
          //#endif
			if (set_flat_on_path(GetObjectHandle(this), &plane))
			{
				obj->status |= ST_FLAT;
				memcpy(&m_plane_normal,&plane.v,sizeof(D_POINT));
				m_plane_D =plane.d;
			}
			else  
			{
				assert(0);
				obj->status &= ~ST_FLAT;
			}
	        // ROBLOX change
			//#if (SG_CURRENT_PLATFORM==SG_PLATFORM_IOS)
               eps_d =savingEpsD;
			 //#endif

		}
	}
	else
	{
		assert(0);
		return false;
	}
	return true;
}

sgCSpline::~sgCSpline()
{
	if (m_spline_geo)
		delete m_spline_geo;
}

sgCSpline::sgCSpline(SG_OBJ_HANDLE objH):sgC2DObject(objH)
{
  char* typeID = "{0000000000000-0000-0000-000000000005}";
  set_hobj_attr_value(id_TypeID, objH, typeID);

  m_spline_geo = new SG_SPLINE(objH);

  lpOBJ obj = (lpOBJ)objH;

  /*if (is_spline_on_one_line(m_spline_geo))
  {
	  obj->status |= ST_ON_LINE;
	  obj->status &= ~ST_FLAT;
  }
  else
  {
    D_PLANE plane;
	if (set_flat_on_path(m_object_handle, &plane))
	{
		obj->status |= ST_FLAT;
		memcpy(&m_plane_normal,&plane.v,sizeof(D_POINT));
		m_plane_D =plane.d;
	}
	else      
		obj->status &= ~ST_FLAT;
  }*/
  if (obj->status & ST_FLAT)
  {
	  D_PLANE plane;
	  if (set_flat_on_path(objH, &plane))
	  {
		  obj->status |= ST_FLAT;
		  memcpy(&m_plane_normal,&plane.v,sizeof(D_POINT));
		  m_plane_D =plane.d;
	  }
	  else  
	  {
		  assert(0);
		  obj->status &= ~ST_FLAT;
	  }
  }

  PostCreate();

}

sgCSpline*   sgCSpline::Create(const SG_SPLINE& splG)
{
	GEO_SPLINE geo_sply;
	//OSTATUS    status;

	spl_work=CRE_SPL;

	if(!create_geo_sply((lpSPLY_DAT)splG.m_handle, &geo_sply))
	{
		assert(0);
		global_sg_error = SG_ER_INTERNAL;
		return NULL;
	}

	//   
	//status = ST_CLOSE;//(spl.sdtype & SPL_CLOSE) ? ST_CLOSE : 0;
	//free_sply_dat(spl);

	sgCSpline*   newSpl=new sgCSpline;
	SetObjectHandle(newSpl,create_simple_obj_loc(OSPLINE, &geo_sply)); 
	((lpOBJ)(GetObjectHandle(newSpl)))->extendedClass = newSpl;

	lpOBJ obj = (lpOBJ)GetObjectHandle(newSpl);
	obj->status &= ~(ST_CLOSE | ST_FLAT | ST_SIMPLE | ST_ON_LINE);

	OSTATUS    status;
	status = (OSTATUS)((((lpSPLY_DAT)splG.m_handle)->sdtype & SPL_CLOSE) ? ST_CLOSE : 0);
	obj->status |= status;

	if (is_spline_on_one_line(&splG))
	{
		obj->status |= ST_ON_LINE;
		obj->status &= ~ST_FLAT;
	}
	else
	{
		D_PLANE plane;
		if (set_flat_on_path(GetObjectHandle(newSpl), &plane))
		{
			obj->status |= ST_FLAT;
			memcpy(&newSpl->m_plane_normal,&plane.v,sizeof(D_POINT));
			newSpl->m_plane_D =plane.d;
		}
		else      
			obj->status &= ~ST_FLAT;
	}

	OSCAN_COD cod;
	if( (cod = test_self_cross_path(GetObjectHandle(newSpl), NULL)) == OSFALSE )
	{
		sgCObject::DeleteObject(newSpl);
		assert(0);
		global_sg_error = SG_ER_INTERNAL;
		return NULL;
	}
	if( cod == OSTRUE ){
		obj->status |= ST_SIMPLE;
	}

	newSpl->m_spline_geo = new SG_SPLINE(GetObjectHandle(newSpl));

	char* typeID = "{0000000000000-0000-0000-000000000005}";
	set_hobj_attr_value(id_TypeID, GetObjectHandle(newSpl), typeID);

	newSpl->PostCreate();

	global_sg_error = SG_ER_SUCCESS;

	return  newSpl;
}

const SG_SPLINE*  sgCSpline::GetGeometry() const
{
	return m_spline_geo;
}

bool   sgCSpline::IsClosed() const
{
	if (!m_spline_geo)
		return false;
	return m_spline_geo->IsClosed();
}

bool    sgCSpline::IsPlane(SG_VECTOR* plN,sgFloat* plD)  const
{
	if (((lpOBJ)GetObjectHandle(this))->status & ST_FLAT)
	{
		if (plN)
			*plN = m_plane_normal;
		if (plD)
			*plD = m_plane_D;
		return true;
	}
	else
		return false;
}

bool  sgCSpline::IsLinear() const
{
	if (((lpOBJ)GetObjectHandle(this))->status & ST_ON_LINE)
		return true;
	else
		return false;
}

bool   sgCSpline::IsSelfIntersecting()  const
{
	if (((lpOBJ)GetObjectHandle(this))->status & ST_SIMPLE)
		return false;
	else
		return true;
}
