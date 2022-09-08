#ifndef  __sgSpaceMath__
#define  __sgSpaceMath__

namespace  sgSpaceMath
{
	sgCore_API   bool		 IsPointsOnOneLine(const SG_POINT& p1,
												const SG_POINT& p2,
												const SG_POINT& p3);

	sgCore_API   sgFloat		 PointsDistance(const SG_POINT& p1, const SG_POINT& p2);

	sgCore_API   bool		 NormalVector(SG_VECTOR& vect);
	sgCore_API   SG_VECTOR   VectorsAdd(const SG_VECTOR& v1,const SG_VECTOR& v2);
	sgCore_API   SG_VECTOR   VectorsSub(const SG_VECTOR& v1,const SG_VECTOR& v2);
	sgCore_API   sgFloat      VectorsScalarMult(const SG_VECTOR& v1,const SG_VECTOR& v2);
	sgCore_API   SG_VECTOR   VectorsVectorMult(const SG_VECTOR& v1,const SG_VECTOR& v2);

	sgCore_API   sgFloat      ProjectPointToLineAndGetDist(const SG_POINT& lineP, 
														    const SG_VECTOR& lineDir, 
															const SG_POINT& pnt, 
															SG_POINT& resPnt);

	typedef enum
	{
		SG_LINE_PARALLEL = -1,
		SG_LINE_ON_PLANE = 0,
		SG_EXIST_INTERSECT_PONT = 1
	}  SG_PLANE_AND_LINE;

	sgCore_API   SG_PLANE_AND_LINE  IntersectPlaneAndLine(const SG_VECTOR& planeNorm, 
																const sgFloat planeD, 
																const SG_POINT& lineP, 
																const SG_VECTOR& lineDir,
																SG_POINT& resP);

	sgCore_API   bool        IsSegmentsIntersecting(const SG_LINE& ln1, 
														bool as_line1,
														const SG_LINE& ln2, 
														bool as_line2,
														SG_POINT& resP);

	sgCore_API   bool        PlaneFromPoints(const SG_POINT& p1, 
												const SG_POINT& p2, 
												const SG_POINT& p3,
												SG_VECTOR& resPlaneNorm,
												sgFloat& resPlaneD);

	sgCore_API   void        PlaneFromNormalAndPoint(const SG_POINT& planePnt, 
														const SG_VECTOR& planeNorm,
														sgFloat& resPlaneD); 

	sgCore_API   sgFloat      GetPlanePointDistance(const SG_POINT& pnt, 
													const SG_VECTOR& planeNorm, 
													const sgFloat planeD);

    sgCore_API   sgFloat      GetLinePointDistance(const SG_POINT& lineP,
                                                  const SG_VECTOR& lineDir,
                                                  const SG_POINT& pnt,
                                                  SG_POINT& projectionPnt);

    sgCore_API   bool         Intersect3Plane(const SG_VECTOR& planeNorm1,
                                              const sgFloat planeD1,
                                              const SG_VECTOR& planeNorm2,
                                              const sgFloat planeD2,
                                              const SG_VECTOR& planeNorm3,
                                              const sgFloat planeD3,
                                              SG_POINT& resP);

    sgCore_API   bool         Intersect2Plane(const SG_VECTOR& planeNorm1,
                                              const sgFloat planeD1,
                                              const SG_VECTOR& planeNorm2,
                                              const sgFloat planeD2,
                                              SG_POINT& resP,
                                              SG_VECTOR& resVector);

};



#endif
