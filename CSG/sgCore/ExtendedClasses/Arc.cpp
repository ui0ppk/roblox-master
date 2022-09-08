#include "..//Core//sg.h"

//      a = new SG_POINT[].    delete[] ;
//static  SG_POINT*  CalculateArcPoints(SG_ARC* arc, int& pnts_count);


static  SG_POINT     arc_points_buffer[NUM_SEGMENTS+1];

static SG_POINT*   GetPointsFromArcGeo(const SG_ARC* arc, int& pnts_count)
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

	lpGEO_ARC    arcData = (lpGEO_ARC)arc;

	o_hcunit(mFrom3DTo2D);
	o_tdtran(mFrom3DTo2D, dpoint_inv(&arcData->vc, &vc));
	o_hcrot1(mFrom3DTo2D, &arcData->n);

	SG_POINT     drP1,drP2;

	o_minv(mFrom3DTo2D,mFrom2DTo3D);

	o_hcncrd(mFrom3DTo2D, &arcData->vb, &vb);
	o_hcncrd(mFrom3DTo2D, &arcData->ve, &ve);
	o_hcncrd(mFrom3DTo2D, &arcData->vc, &vc);

	n = (short)(NUM_SEGMENTS*(fabs(arcData->angle)/(2*M_PI)));
	if(n < MIN_NUM_SEGMENTS) n = MIN_NUM_SEGMENTS;
	dd = arcData->angle/n;

	pnts_count = n+1;
	memset(arc_points_buffer,0,sizeof(SG_POINT)*(NUM_SEGMENTS+1));


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
		arc_points_buffer[i++] = drP1;
		dpoint_copy(&p1_2D, &p2_2D);
		dpoint_copy(&p1_3D, &p2_3D);
	}
	arc_points_buffer[i++] = drP2;
	arc_points_buffer[i].x = arcData->ve.x;
	arc_points_buffer[i].y = arcData->ve.y;
	arc_points_buffer[i].z = arcData->ve.z;

	return arc_points_buffer;
}


bool SG_ARC::FromThreePoints(const SG_POINT& begP,
							const SG_POINT& endP,
							const SG_POINT& midP,
							bool invert)//  ,     
{
	radius = 0.0;
	normal.x = normal.y = normal.z = 0.0; 
	center.x = center.y = center.z = 0.0;
	begin.x = begin.y = begin.z = 0.0;
	end.x = end.y = end.z = 0.0;
	begin_angle = angle =0.0;

	sgFloat    tmpD;
	SG_VECTOR tmpVec;
	if (!sgSpaceMath::PlaneFromPoints(begP,endP,midP,tmpVec,tmpD))
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
		return false;
	}
	if (!sgSpaceMath::NormalVector(tmpVec))
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
		return false;
	}

	MATR         mFrom2DTo3D;
	MATR         mFrom3DTo2D;

	o_hcunit(mFrom3DTo2D);
	lpD_POINT  tmpP = reinterpret_cast<lpD_POINT>(&tmpVec);
	o_hcrot1(mFrom3DTo2D, tmpP);
	o_minv(mFrom3DTo2D,mFrom2DTo3D);

	D_POINT p_2D[3];
	//  ,  ,    
	p_2D[0].x = begP.x;p_2D[0].y = begP.y;p_2D[0].z = begP.z;
	o_hcncrd(mFrom3DTo2D, &p_2D[0], &p_2D[0]);
	p_2D[1].x = endP.x;p_2D[1].y = endP.y;p_2D[1].z = endP.z;
	o_hcncrd(mFrom3DTo2D, &p_2D[1], &p_2D[1]);
	p_2D[2].x = midP.x;p_2D[2].y = midP.y;p_2D[2].z = midP.z;
	o_hcncrd(mFrom3DTo2D, &p_2D[2], &p_2D[2]);

	GEO_ARC pArc;
	memcpy(&pArc, this, sizeof(SG_ARC));
	if (!arc_p_p_p(0,1,2,p_2D ,NULL,0,invert,&pArc))
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
		return false;
	}

	pArc.n.x = tmpVec.x; pArc.n.y = tmpVec.y; pArc.n.z = tmpVec.z;

	o_hcncrd(mFrom2DTo3D,&pArc.vb,&pArc.vb);
	o_hcncrd(mFrom2DTo3D,&pArc.ve,&pArc.ve);
	pArc.vc.z = p_2D[0].z;
	o_hcncrd(mFrom2DTo3D,&pArc.vc,&pArc.vc);

	radius = pArc.r;
	normal.x = pArc.n.x;
	normal.y = pArc.n.y;
	normal.z = pArc.n.z; 
	center.x = pArc.vc.x;
	center.y = pArc.vc.y;
	center.z = pArc.vc.z;
	begin.x = pArc.vb.x;
	begin.y = pArc.vb.y;
	begin.z = pArc.vb.z;
	end.x = pArc.ve.x;
	end.y = pArc.ve.y;
	end.z = pArc.ve.z;
	begin_angle = pArc.ab;
	angle = pArc.angle;

	global_sg_error = SG_ER_SUCCESS;
	return true;
}

bool SG_ARC::FromCenterBeginEnd(const SG_POINT& cenP,
								const SG_POINT& begP,
								const SG_POINT& endP,
								bool invert)//  ,   
{
	radius = 0.0;
	normal.x = normal.y = normal.z = 0.0; 
	center.x = center.y = center.z = 0.0;
	begin.x = begin.y = begin.z = 0.0;
	end.x = end.y = end.z = 0.0;
	begin_angle = angle =0.0;

	sgFloat    tmpD;
	SG_VECTOR tmpVec;
	if (!sgSpaceMath::PlaneFromPoints(begP,endP,cenP,tmpVec,tmpD))
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
		return false;
	}
	if (!sgSpaceMath::NormalVector(tmpVec))
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
		return false;
	}

	MATR         mFrom2DTo3D;
	MATR         mFrom3DTo2D;

	o_hcunit(mFrom3DTo2D);
	lpD_POINT  tmpP = reinterpret_cast<lpD_POINT>(&tmpVec);
	o_hcrot1(mFrom3DTo2D, tmpP);
	o_minv(mFrom3DTo2D,mFrom2DTo3D);

	D_POINT p_2D[3];
	// ,  ,   
	p_2D[0].x = cenP.x;p_2D[0].y = cenP.y;p_2D[0].z = cenP.z;
	o_hcncrd(mFrom3DTo2D, &p_2D[0], &p_2D[0]);
	p_2D[1].x = begP.x;p_2D[1].y = begP.y;p_2D[1].z = begP.z;
	o_hcncrd(mFrom3DTo2D, &p_2D[1], &p_2D[1]);
	p_2D[2].x = endP.x;p_2D[2].y = endP.y;p_2D[2].z = endP.z;
	o_hcncrd(mFrom3DTo2D, &p_2D[2], &p_2D[2]);

	GEO_ARC pArc;
	memcpy(&pArc, this, sizeof(SG_ARC));

	if (!arc_b_e_c(&p_2D[1],&p_2D[2],&p_2D[0],FALSE,FALSE,invert,&pArc))
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
		return false;
	}

	pArc.n.x = tmpVec.x; pArc.n.y = tmpVec.y; pArc.n.z = tmpVec.z;

	o_hcncrd(mFrom2DTo3D,&pArc.vb,&pArc.vb);
	o_hcncrd(mFrom2DTo3D,&pArc.ve,&pArc.ve);
	pArc.vc.z = p_2D[0].z;
	o_hcncrd(mFrom2DTo3D,&pArc.vc,&pArc.vc);

	radius = pArc.r;
	normal.x = pArc.n.x;
	normal.y = pArc.n.y;
	normal.z = pArc.n.z; 
	center.x = pArc.vc.x;
	center.y = pArc.vc.y;
	center.z = pArc.vc.z;
	begin.x = pArc.vb.x;
	begin.y = pArc.vb.y;
	begin.z = pArc.vb.z;
	end.x = pArc.ve.x;
	end.y = pArc.ve.y;
	end.z = pArc.ve.z;
	begin_angle = pArc.ab;
	angle = pArc.angle;

	global_sg_error = SG_ER_SUCCESS;
	return true;
}

bool SG_ARC::FromBeginEndNormalRadius(const SG_POINT& begP,
									  const SG_POINT& endP,
									  const SG_VECTOR& nrmlV,
									  sgFloat rad,
									  bool invert)//  ,   
{
	radius = 0.0;
	normal.x = normal.y = normal.z = 0.0; 
	center.x = center.y = center.z = 0.0;
	begin.x = begin.y = begin.z = 0.0;
	end.x = end.y = end.z = 0.0;
	begin_angle = angle =0.0;

	//  ,  , , 
	D_POINT pnts[2];

	pnts[0].x = begP.x; 
	pnts[0].y = begP.y;
	pnts[0].z = begP.z;
	pnts[1].x = endP.x;
	pnts[1].y = endP.y;
	pnts[1].z = endP.z;
	// 
	D_POINT nrml;
	nrml.x = nrmlV.x;
	nrml.y = nrmlV.y;
	nrml.z = nrmlV.z;

	MATR  matrix;
	o_hcunit(matrix);
	o_hcrot1(matrix,&nrml);
	o_hcncrd(matrix,&pnts[0],&pnts[0]);
	o_hcncrd(matrix,&pnts[1],&pnts[1]);
	GEO_ARC pArc;
	memcpy(&pArc, this, sizeof(SG_ARC));
	if (!arc_b_e_r(&pnts[0],&pnts[1],rad,invert,&pArc))
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
		return false;
	}
	dpoint_copy(&pArc.n,&nrml);

	o_minv(matrix,matrix);

	o_hcncrd(matrix,&pArc.vb,&pArc.vb);
	o_hcncrd(matrix,&pArc.ve,&pArc.ve);
	o_hcncrd(matrix,&pArc.vc,&pArc.vc);

	radius = pArc.r;
	normal.x = pArc.n.x;
	normal.y = pArc.n.y;
	normal.z = pArc.n.z; 
	center.x = pArc.vc.x;
	center.y = pArc.vc.y;
	center.z = pArc.vc.z;
	begin.x = pArc.vb.x;
	begin.y = pArc.vb.y;
	begin.z = pArc.vb.z;
	end.x = pArc.ve.x;
	end.y = pArc.ve.y;
	end.z = pArc.ve.z;
	begin_angle = pArc.ab;
	angle = pArc.angle;

	global_sg_error =SG_ER_SUCCESS;
	return true;
}

bool SG_ARC::FromCenterBeginNormalAngle(const SG_POINT& cenP,
										const SG_POINT& begP,
										const SG_VECTOR& nrmlV,
										sgFloat ang)//  ,   
{
	radius = 0.0;
	normal.x = normal.y = normal.z = 0.0; 
	center.x = center.y = center.z = 0.0;
	begin.x = begin.y = begin.z = 0.0;
	end.x = end.y = end.z = 0.0;
	begin_angle = angle =0.0;

	// ,  , , 
	D_POINT pnts[2];

	pnts[0].x = cenP.x; 
	pnts[0].y = cenP.y;
	pnts[0].z = cenP.z;
	pnts[1].x = begP.x;
	pnts[1].y = begP.y;
	pnts[1].z = begP.z;
	// 
	D_POINT nrml;
	nrml.x = nrmlV.x;
	nrml.y = nrmlV.y;
	nrml.z = nrmlV.z;

	MATR  matrix;
	MATR  matrix_inv;
	o_hcunit(matrix);
	o_hcrot1(matrix,&nrml);
	o_hcncrd(matrix,&pnts[0],&pnts[0]);
	o_hcncrd(matrix,&pnts[1],&pnts[1]);
	GEO_ARC pArc;
	memcpy(&pArc, this, sizeof(SG_ARC));
	if (!arc_b_c_a(&pnts[1],&pnts[0],ang,&pArc))
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
		return false;
	}
	dpoint_copy(&pArc.n,&nrml);

	o_minv(matrix,matrix_inv);

	o_hcncrd(matrix_inv,&pArc.vb,&pArc.vb);
	pArc.ve.z = pArc.vc.z;
	o_hcncrd(matrix_inv,&pArc.ve,&pArc.ve);
	o_hcncrd(matrix_inv,&pArc.vc,&pArc.vc);

	radius = pArc.r;
	normal.x = pArc.n.x;
	normal.y = pArc.n.y;
	normal.z = pArc.n.z; 
	center.x = pArc.vc.x;
	center.y = pArc.vc.y;
	center.z = pArc.vc.z;
	begin.x = pArc.vb.x;
	begin.y = pArc.vb.y;
	begin.z = pArc.vb.z;
	end.x = pArc.ve.x;
	end.y = pArc.ve.y;
	end.z = pArc.ve.z;
	begin_angle = pArc.ab;
	angle = pArc.angle;

	global_sg_error = SG_ER_SUCCESS;
	return true;
}

bool SG_ARC::FromBeginEndNormalAngle(const SG_POINT& begP,
									 const SG_POINT& endP,
									 const SG_VECTOR& nrmlV,
									 sgFloat ang)//  ,   
{
	radius = 0.0;
	normal.x = normal.y = normal.z = 0.0; 
	center.x = center.y = center.z = 0.0;
	begin.x = begin.y = begin.z = 0.0;
	end.x = end.y = end.z = 0.0;
	begin_angle = angle =0.0;

	//  , , , 
	D_POINT pnts[2];
	pnts[0].x = begP.x;
	pnts[0].y = begP.y;
	pnts[0].z = begP.z;
	pnts[1].x = endP.x;
	pnts[1].y = endP.y;
	pnts[1].z = endP.z;
	// 
	D_POINT nrml;
	nrml.x = nrmlV.x;
	nrml.y = nrmlV.y;
	nrml.z = nrmlV.z;

	MATR  matrix;
	o_hcunit(matrix);
	o_hcrot1(matrix,&nrml);
	o_hcncrd(matrix,&pnts[0],&pnts[0]);
	o_hcncrd(matrix,&pnts[1],&pnts[1]);
	GEO_ARC pArc;
	memcpy(&pArc, this, sizeof(SG_ARC));
	if (!arc_b_e_a(&pnts[0],&pnts[1],ang,&pArc))
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS;
		return false;
	}
	dpoint_copy(&pArc.n,&nrml);

	o_minv(matrix,matrix);

	o_hcncrd(matrix,&pArc.vb,&pArc.vb);
	o_hcncrd(matrix,&pArc.ve,&pArc.ve);
	o_hcncrd(matrix,&pArc.vc,&pArc.vc);

	radius = pArc.r;
	normal.x = pArc.n.x;
	normal.y = pArc.n.y;
	normal.z = pArc.n.z; 
	center.x = pArc.vc.x;
	center.y = pArc.vc.y;
	center.z = pArc.vc.z;
	begin.x = pArc.vb.x;
	begin.y = pArc.vb.y;
	begin.z = pArc.vb.z;
	end.x = pArc.ve.x;
	end.y = pArc.ve.y;
	end.z = pArc.ve.z;
	begin_angle = pArc.ab;
	angle = pArc.angle;

	global_sg_error = SG_ER_SUCCESS;

	return true;
}

bool  SG_ARC::Draw(SG_DRAW_LINE_FUNC line_func) const
{
	if (line_func==NULL)
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_NULL_POINTER;
		return false;
	}
	int pcnt=0;
	SG_POINT* pnts=NULL;
	pnts = GetPointsFromArcGeo(this,pcnt);
	for (int i=0;i<pcnt-1;i++)
		line_func(pnts+i, pnts+i+1);
	global_sg_error = SG_ER_SUCCESS;
	return true;
}


sgCArc::sgCArc():sgC2DObject()
		, m_points(NULL)
		, m_points_count(0)
{
}

sgCArc::sgCArc(SG_OBJ_HANDLE objH):sgC2DObject(objH)
		, m_points(NULL)
		, m_points_count(0)
{
	char* typeID = "{0000000000000-0000-0000-000000000004}";
	set_hobj_attr_value(id_TypeID, objH, typeID);

	m_points_count = NUM_SEGMENTS+1;
	m_points = new SG_POINT[m_points_count];

	SG_ARC* arG = const_cast<SG_ARC*>(GetGeometry());
	memcpy(m_points,
		GetPointsFromArcGeo(arG,m_points_count),
		sizeof(SG_POINT)*m_points_count);
	memcpy(&m_plane_normal,&(arG->normal),sizeof(SG_VECTOR));
	sgSpaceMath::PlaneFromNormalAndPoint(arG->center,m_plane_normal,m_plane_D);

	PostCreate();
}


bool	sgCArc::ApplyTempMatrix()
{
	if (!sgCObject::ApplyTempMatrix())
		return false;
	
	SG_ARC* arG = const_cast<SG_ARC*>(GetGeometry());
	memcpy(m_points,
		GetPointsFromArcGeo(arG,m_points_count),
		sizeof(SG_POINT)*m_points_count);
	memcpy(&m_plane_normal,&(arG->normal),sizeof(SG_VECTOR));
	sgSpaceMath::PlaneFromNormalAndPoint(arG->center,m_plane_normal,m_plane_D);

	return true;
}

sgCArc::~sgCArc()
{
	if (m_points)
	{
		delete[] m_points;
		m_points = NULL;
		m_points_count = 0;
	}
}

sgCArc*    sgCArc::Create(const SG_ARC& arcG)
{
	sgCArc*   newA=new sgCArc;
	global_sg_error = SG_ER_SUCCESS;
	SetObjectHandle(newA, create_simple_obj_loc(OARC, const_cast<SG_ARC*>(&arcG)));
	((lpOBJ)(GetObjectHandle(newA)))->extendedClass = newA;

	char* typeID = "{0000000000000-0000-0000-000000000004}";
	set_hobj_attr_value(id_TypeID, GetObjectHandle(newA), typeID);

	newA->m_points_count = NUM_SEGMENTS+1;
	newA->m_points = new SG_POINT[newA->m_points_count];
	SG_ARC* arG = const_cast<SG_ARC*>(newA->GetGeometry());
	memcpy(newA->m_points,
		GetPointsFromArcGeo(arG,newA->m_points_count),
		sizeof(SG_POINT)*newA->m_points_count);

	memcpy(&(newA->m_plane_normal),&(arG->normal),sizeof(SG_VECTOR));
	sgSpaceMath::PlaneFromNormalAndPoint(arG->center,
		newA->m_plane_normal,newA->m_plane_D);


	newA->PostCreate();

	return  newA;
}

const SG_ARC*  sgCArc::GetGeometry() const
{
	lpOBJ pobj = (lpOBJ)(GetObjectHandle(this));
	return static_cast<SG_ARC*>((void*)(pobj->geo_data));
}

int	sgCArc::GetPointsCount() const
{
	return m_points_count;
}

const SG_POINT*  sgCArc::GetPoints() const
{
	return m_points;
}

bool  sgCArc::IsClosed() const {return false;}
bool  sgCArc::IsPlane(SG_VECTOR* plN,sgFloat* plD) const 
{
	if (plN)
		*plN = m_plane_normal;
	if (plD)
		*plD = m_plane_D;
	return true;
}
bool  sgCArc::IsLinear() const {return false;}
bool  sgCArc::IsSelfIntersecting()  const {return false;}

