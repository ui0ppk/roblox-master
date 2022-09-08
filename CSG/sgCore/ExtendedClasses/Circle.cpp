#include "..//Core//sg.h"

//      a = new SG_POINT[].    delete[] ;
//static  SG_POINT*  CalculateCirclePoints(SG_CIRCLE* circ, int& pnts_count);

static  SG_POINT     circle_points_buffer[NUM_SEGMENTS];

static SG_POINT*   GetPointsFromCircleGeo(const SG_CIRCLE* circ, int& pnts_count)
{
	D_POINT      p1_2D;
	D_POINT      p2_2D;
	D_POINT      vb, ve, vc;
	D_POINT      p1_3D;
	D_POINT      p2_3D;

	short        n;
	sgFloat       dx, dy, dd, sp, cp;
	MATR         mFrom2DTo3D;
	MATR         mFrom3DTo2D;

	lpGEO_CIRCLE    circData = (lpGEO_CIRCLE)circ;

	o_hcunit(mFrom3DTo2D);
	o_tdtran(mFrom3DTo2D, dpoint_inv(&circData->vc, &vc));
	o_hcrot1(mFrom3DTo2D, &circData->n);

	SG_POINT     drP1,drP2;

	o_minv(mFrom3DTo2D,mFrom2DTo3D);

	o_hcncrd(mFrom3DTo2D, &circData->vc, &vc);

	dpoint_copy(&vb, &vc);
	vb.x += circData->r;
	dpoint_copy(&ve, &vb);

	pnts_count = n = NUM_SEGMENTS;
	memset(circle_points_buffer,0,sizeof(SG_POINT)*pnts_count);

	dd = (2*M_PI)/n;

	sp = sin(dd);
	cp = cos(dd);
	dx = vb.x - vc.x;
	dy = vb.y - vc.y;
	dpoint_copy( &p1_2D, &vb);
	o_hcncrd(mFrom2DTo3D, &p1_2D, &p1_3D);
	int i=0;
	while(--n){
		dd = dx;
		dx = dx*cp - dy*sp;
		dy = dd*sp + dy*cp;
		p2_2D.x = vc.x + dx;
		p2_2D.y = vc.y + dy;
		p2_2D.z = 0.;
		o_hcncrd(mFrom2DTo3D, &p2_2D, &p2_3D);
		drP1.x = p1_3D.x;  drP1.y = p1_3D.y;  drP1.z = p1_3D.z;
		drP2.x = p2_3D.x;  drP2.y = p2_3D.y;  drP2.z = p2_3D.z;
		//lineDrawFunc(&drP1, &drP2);
		circle_points_buffer[i++] = drP1;
		dpoint_copy(&p1_2D, &p2_2D);
		dpoint_copy(&p1_3D, &p2_3D);
	}
	circle_points_buffer[i++] = drP2;
	/*dpoint_copy( &p2_2D, &ve);
	o_hcncrd(mFrom2DTo3D, &p2_2D, &p2_3D);
	drP1.x = p1_3D.x;  drP1.y = p1_3D.y;  drP1.z = p1_3D.z;
	drP2.x = p2_3D.x;  drP2.y = p2_3D.y;  drP2.z = p2_3D.z;*/
	//lineDrawFunc(&drP1, &drP2);

	return circle_points_buffer;
}


bool  SG_CIRCLE::FromCenterRadiusNormal(const SG_POINT& cen, 
										sgFloat rad, 
										const SG_VECTOR& nor)
{
	if (rad<DBL_EPSILON)
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_NON_POSITIVE;
		return false;
	}

	if ((fabs(nor.x)+fabs(nor.y)+fabs(nor.z))<DBL_EPSILON)
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_ZERO_LENGTH;
		return false;
	}
	center = cen;
	radius = rad;
	normal = nor;
	global_sg_error = SG_ER_SUCCESS;
	return true;
}

bool  SG_CIRCLE::FromThreePoints(const SG_POINT& p1, 
								 const SG_POINT& p2, 
								 const SG_POINT& p3)
{
	sgFloat    tmpD;
	if (!sgSpaceMath::PlaneFromPoints(p1,p2,p3,normal,tmpD))
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
		return false;
	}
	if (!sgSpaceMath::NormalVector(normal))
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
		return false;
	}

	MATR         mFrom2DTo3D;
	MATR         mFrom3DTo2D;

	o_hcunit(mFrom3DTo2D);
	lpD_POINT  tmpP = reinterpret_cast<lpD_POINT>(&normal);
	o_hcrot1(mFrom3DTo2D, tmpP);
	o_minv(mFrom3DTo2D,mFrom2DTo3D);

	D_POINT p_2D[3];

	p_2D[0].x = p1.x;p_2D[0].y = p1.y;p_2D[0].z = p1.z;
	o_hcncrd(mFrom3DTo2D, &p_2D[0], &p_2D[0]);
	p_2D[1].x = p2.x;p_2D[1].y = p2.y;p_2D[1].z = p2.z;
	o_hcncrd(mFrom3DTo2D, &p_2D[1], &p_2D[1]);
	p_2D[2].x = p3.x;p_2D[2].y = p3.y;p_2D[2].z = p3.z;
	o_hcncrd(mFrom3DTo2D, &p_2D[2], &p_2D[2]);

	GEO_ARC Arc;
	if (!arc_p_p_p(0,1,2,p_2D ,NULL,0,false,&Arc))
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
		return false;
	}

	tmpP = reinterpret_cast<lpD_POINT>(&center);
	Arc.vc.z = p_2D[0].z;
	o_hcncrd(mFrom2DTo3D,&Arc.vc,tmpP);

	radius = sgSpaceMath::PointsDistance(center, p1);

	global_sg_error = SG_ER_SUCCESS;
	return true;
}

bool  SG_CIRCLE::Draw(SG_DRAW_LINE_FUNC line_func) const
{
	if (line_func==NULL)
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_NULL_POINTER;
		return false;
	}
	int pcnt=0;
	SG_POINT* pnts=NULL;
	pnts = GetPointsFromCircleGeo(this,pcnt);
	for (int i=0;i<pcnt-1;i++)
		line_func(pnts+i, pnts+i+1);
	line_func(pnts+pcnt-1, pnts);
	global_sg_error = SG_ER_SUCCESS;
	return true;
}


sgCCircle::sgCCircle():sgC2DObject()
		, m_points(NULL)
		, m_points_count(0)
{
}

sgCCircle::sgCCircle(SG_OBJ_HANDLE objH):sgC2DObject(objH)
		, m_points(NULL)
		, m_points_count(0)
{
	char* typeID = "{0000000000000-0000-0000-000000000003}";
	set_hobj_attr_value(id_TypeID, objH, typeID);

	m_points_count = NUM_SEGMENTS;
	m_points = new SG_POINT[m_points_count];

	SG_CIRCLE* crG = const_cast<SG_CIRCLE*>(GetGeometry());

	memcpy(m_points,
		GetPointsFromCircleGeo(crG,m_points_count),
		sizeof(SG_POINT)*m_points_count);

	memcpy(&m_plane_normal,&(crG->normal),sizeof(SG_VECTOR));
	sgSpaceMath::PlaneFromNormalAndPoint(crG->center,m_plane_normal,m_plane_D);

	PostCreate();
}


bool	sgCCircle::ApplyTempMatrix()
{
	if (!sgCObject::ApplyTempMatrix())
		return false;
	SG_CIRCLE* crG = const_cast<SG_CIRCLE*>(GetGeometry());
	memcpy(m_points,GetPointsFromCircleGeo(crG,m_points_count),
			sizeof(SG_POINT)*m_points_count);
	memcpy(&m_plane_normal,&(crG->normal),sizeof(SG_VECTOR));
	sgSpaceMath::PlaneFromNormalAndPoint(crG->center,m_plane_normal,m_plane_D);
	return true;
}


sgCCircle::~sgCCircle()
{
	if (m_points)
	{
		delete[] m_points;
		m_points = NULL;
		m_points_count = 0;
	}
}

sgCCircle*    sgCCircle::Create(const SG_CIRCLE& cirG)
{
	SG_VECTOR vvv = cirG.normal;
	if (cirG.radius<eps_d ||
		!sgSpaceMath::NormalVector(vvv))
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_UNKNOWN;
		return NULL;
	}
	global_sg_error = SG_ER_SUCCESS;
	sgCCircle*   newC=new sgCCircle;
	SetObjectHandle(newC, create_simple_obj_loc(OCIRCLE, const_cast<SG_CIRCLE*>(&cirG)));
	((lpOBJ)(GetObjectHandle(newC)))->extendedClass = newC;

	char* typeID = "{0000000000000-0000-0000-000000000003}";
	set_hobj_attr_value(id_TypeID, GetObjectHandle(newC), typeID);

	newC->m_points_count = NUM_SEGMENTS;
	newC->m_points = new SG_POINT[newC->m_points_count];

	SG_CIRCLE* crG = const_cast<SG_CIRCLE*>(newC->GetGeometry());
	memcpy(newC->m_points,
		GetPointsFromCircleGeo(crG,newC->m_points_count),
		sizeof(SG_POINT)*newC->m_points_count);

	memcpy(&(newC->m_plane_normal),&(crG->normal),sizeof(SG_VECTOR));
	sgSpaceMath::PlaneFromNormalAndPoint(crG->center,
		newC->m_plane_normal,newC->m_plane_D);

	newC->PostCreate();

	return  newC;
}

const SG_CIRCLE*  sgCCircle::GetGeometry() const
{
	lpOBJ pobj = (lpOBJ)(GetObjectHandle(this));
	return static_cast<SG_CIRCLE*>((void*)(pobj->geo_data));
}

int	sgCCircle::GetPointsCount() const
{
	return m_points_count;
}

const SG_POINT*  sgCCircle::GetPoints() const
{
	return m_points;
}

bool  sgCCircle::IsClosed() const {return true;}
bool  sgCCircle::IsPlane(SG_VECTOR* plN,sgFloat* plD) const 
{
	if (plN)
		*plN = m_plane_normal;
	if (plD)
		*plD = m_plane_D;
	return true;
}
bool  sgCCircle::IsLinear() const {return false;}
bool  sgCCircle::IsSelfIntersecting()  const {return false;}
