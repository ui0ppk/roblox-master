#include "CORE//sg.h"

bool		sgSpaceMath::NormalVector(SG_VECTOR& vect)
{
	sgFloat cs;

	cs = sqrt(vect.x*vect.x + vect.y*vect.y + vect.z*vect.z);

	if ( cs < MINsgFloat*1000) 
	{
		vect.x = vect.y = vect.z = 0.;
		global_sg_error = SG_ER_BAD_ARGUMENT_ZERO_LENGTH;
		return false;   //  
	}
	vect.x /= cs;                      //  
	vect.y /= cs;
	vect.z /= cs;
	global_sg_error = SG_ER_SUCCESS;
	return true;
}

bool	sgSpaceMath::IsPointsOnOneLine(const SG_POINT& p1,const SG_POINT& p2,const SG_POINT& p3)
{
	global_sg_error = SG_ER_SUCCESS;
	SG_VECTOR v1;
	v1.x = p2.x-p1.x; v1.y = p2.y-p1.y; v1.z = p2.z-p1.z;
	if (!NormalVector(v1))
		return true;
	SG_VECTOR v2;
	v2.x = p3.x-p1.x; v2.y = p3.y-p1.y; v2.z = p3.z-p1.z;
	if (!NormalVector(v2))
		return true;
	if (fabs(VectorsScalarMult(v1,v2))>(1.0-0.000001))
		return true;
	return false;
}

sgFloat		sgSpaceMath::PointsDistance(const SG_POINT& p1 ,const SG_POINT& p2)
{
	sgFloat pr;

	pr = sqrt((p1.x-p2.x)*(p1.x-p2.x)+
		(p1.y-p2.y)*(p1.y-p2.y)+
		(p1.z-p2.z)*(p1.z-p2.z));
	return pr;
}

SG_VECTOR   sgSpaceMath::VectorsAdd(const SG_VECTOR& v1,const SG_VECTOR& v2)
{
	SG_VECTOR tmpV;
	tmpV.x = v1.x + v2.x;
	tmpV.y = v1.y + v2.y;
	tmpV.z = v1.z + v2.z;
	return tmpV;
}

SG_VECTOR   sgSpaceMath::VectorsSub(const SG_VECTOR& v1,const SG_VECTOR& v2)
{
	SG_VECTOR tmpV;
	tmpV.x = v1.x - v2.x;
	tmpV.y = v1.y - v2.y;
	tmpV.z = v1.z - v2.z;
	return tmpV;
}

sgFloat      sgSpaceMath::VectorsScalarMult(const SG_VECTOR& v1, const SG_VECTOR& v2)
{
	sgFloat pr;
	pr = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	return pr;
}

SG_VECTOR   sgSpaceMath::VectorsVectorMult(const SG_VECTOR& v1, const SG_VECTOR& v2)
{
	SG_VECTOR v;
	v.x = v1.y * v2.z - v1.z * v2.y;
	v.y = v1.z * v2.x - v1.x * v2.z;
	v.z = v1.x * v2.y - v1.y * v2.x;
	return v;
}

sgFloat      sgSpaceMath::ProjectPointToLineAndGetDist(const SG_POINT& lineP, 
													  const SG_VECTOR& lineDir , 
													  const SG_POINT& pnt , SG_POINT& resPnt)

{
	sgFloat d;
	d = - lineDir.x*pnt.x - lineDir.y*pnt.y - lineDir.z*pnt.z;
	IntersectPlaneAndLine(lineDir, d, lineP, lineDir, resPnt);
	return PointsDistance(pnt, resPnt);
}


sgSpaceMath::SG_PLANE_AND_LINE  sgSpaceMath::IntersectPlaneAndLine(const SG_VECTOR& planeNorm, 
											   const sgFloat planeD, 
											   const SG_POINT& lineP, 
											   const SG_VECTOR& lineDir,
											   SG_POINT& resP)
      
{
	sgFloat c, d;

	c = planeNorm.x*lineDir.x + planeNorm.y*lineDir.y + planeNorm.z*lineDir.z;
	d = planeNorm.x*lineP.x + planeNorm.y*lineP.y + planeNorm.z*lineP.z + planeD;
	if(fabs(c) < eps_n){
		if(fabs(d) < eps_d)
			return sgSpaceMath::SG_LINE_ON_PLANE;
		return sgSpaceMath::SG_LINE_PARALLEL;
	}
	d = -d/c;
	SG_VECTOR tmpV;
	tmpV.x = lineDir.x*d; tmpV.y = lineDir.y*d; tmpV.z = lineDir.z*d;
	resP.x = lineP.x + tmpV.x; 
	resP.y = lineP.y + tmpV.y; 
	resP.z = lineP.z + tmpV.z; 
	return sgSpaceMath::SG_EXIST_INTERSECT_PONT;
}

bool    sgSpaceMath::IsSegmentsIntersecting(const SG_LINE& ln1, bool as_line1,
											const SG_LINE& ln2, bool as_line2,
											SG_POINT& resP)
{
	GEO_LINE l1;
	GEO_LINE l2;
	memcpy(&l1,&ln1,sizeof(GEO_LINE));
	memcpy(&l2,&ln2,sizeof(GEO_LINE));
	D_POINT pIn;
	short intPCnt;
	if (!intersect_3d_ll(&l1,!as_line1,&l2,!as_line2,&pIn,&intPCnt))
		return false;
	if (intPCnt==1)
	{
		memcpy(&resP,&pIn,sizeof(D_POINT));
		return true;
	}
	return false;
}

bool        sgSpaceMath::PlaneFromPoints(const SG_POINT& p1, const SG_POINT& p2, 
										 const SG_POINT& p3,
										 SG_VECTOR& resPlaneNorm,
										 sgFloat& resPlaneD)
{
	if (IsPointsOnOneLine(p1,p2,p3))
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_UNKNOWN;
		return false;
	}
	resPlaneNorm.x = (p1.y - p2.y)*(p1.z - p3.z) - (p1.y - p3.y)*(p1.z - p2.z);
	resPlaneNorm.y = (p1.z - p2.z)*(p1.x - p3.x) - (p1.z - p3.z)*(p1.x - p2.x);
	resPlaneNorm.z = (p1.x - p2.x)*(p1.y - p3.y) - (p1.x - p3.x)*(p1.y - p2.y);
	resPlaneD = hypot(hypot(resPlaneNorm.x, resPlaneNorm.y), resPlaneNorm.z);
	if(resPlaneD < MINsgFloat*10)
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_UNKNOWN;
		return false;
	}
	resPlaneNorm.x /= resPlaneD; resPlaneNorm.y /= resPlaneD; resPlaneNorm.z /= resPlaneD;
	resPlaneD = -p1.x*(resPlaneNorm.x) - p1.y*(resPlaneNorm.y) - p1.z*(resPlaneNorm.z);
	global_sg_error = SG_ER_SUCCESS;
	return true;
}

void        sgSpaceMath::PlaneFromNormalAndPoint(const SG_POINT& planePnt, 
												 const SG_VECTOR& planeNorm,
												 sgFloat& resPlaneD)
{
	resPlaneD = -(planeNorm.x*planePnt.x + planeNorm.y*planePnt.y + planeNorm.z*planePnt.z);
}

sgFloat      sgSpaceMath::GetPlanePointDistance(const SG_POINT& pnt, 
											   const SG_VECTOR& planeNorm, 
											   const sgFloat planeD)
{
	return planeNorm.x*pnt.x + planeNorm.y*pnt.y + planeNorm.z*pnt.z + planeD;
}

sgFloat      sgSpaceMath::GetLinePointDistance(const SG_POINT& lineP,
                                              const SG_VECTOR& lineDir,
                                              const SG_POINT& pnt,
                                              SG_POINT& projectionPnt)
{
    sgFloat d;
    d = - lineDir.x*pnt.x - lineDir.y*pnt.y - lineDir.z*pnt.z;
    IntersectPlaneAndLine(lineDir, d, lineP, lineDir, projectionPnt);
    return PointsDistance(pnt, projectionPnt);

}

bool        sgSpaceMath::Intersect3Plane(const SG_VECTOR& planeNorm1,
                                          const sgFloat planeD1,
                                          const SG_VECTOR& planeNorm2,
                                          const sgFloat planeD2,
                                          const SG_VECTOR& planeNorm3,
                                          const sgFloat planeD3,
                                          SG_POINT& resP)
{
    D_PLANE  pl1;   pl1.v.x = planeNorm1.x; pl1.v.y = planeNorm1.y;  pl1.v.z = planeNorm1.z;  pl1.d = planeD1;
    D_PLANE  pl2;   pl2.v.x = planeNorm2.x; pl2.v.y = planeNorm2.y;  pl2.v.z = planeNorm2.z;  pl2.d = planeD2;
    D_PLANE  pl3;   pl3.v.x = planeNorm3.x; pl3.v.y = planeNorm3.y;  pl3.v.z = planeNorm3.z;  pl3.d = planeD3;

    D_POINT resPnt;

        if (CalcPointOf3PlanesInter(&pl1, &pl2, &pl3, &resPnt))
        {
            resP.x = resPnt.x;  resP.y = resPnt.y;  resP.z = resPnt.z;
            return true;
        }
        return false;
}

bool        sgSpaceMath::Intersect2Plane(const SG_VECTOR& planeNorm1,
                                          const sgFloat planeD1,
                                          const SG_VECTOR& planeNorm2,
                                          const sgFloat planeD2,
                                          SG_POINT& resP,
                                          SG_VECTOR& resVector)
{
    D_PLANE  pl1;   pl1.v.x = planeNorm1.x; pl1.v.y = planeNorm1.y;  pl1.v.z = planeNorm1.z;  pl1.d = planeD1;
        D_PLANE  pl2;   pl2.v.x = planeNorm2.x; pl2.v.y = planeNorm2.y;  pl2.v.z = planeNorm2.z;  pl2.d = planeD2;

        D_AXIS resAxe;

        if (CalcAxisOfPlanesInter(&pl1, &pl2, &resAxe))
        {
            resP.x = resAxe.p.x;       resP.y = resAxe.p.y;       resP.z = resAxe.p.z;
            resVector.x = resAxe.v.x;  resVector.y = resAxe.v.y;  resVector.z = resAxe.v.z;
            return true;
        }
        return false;
}

