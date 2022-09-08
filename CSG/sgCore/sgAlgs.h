#ifndef  __sgAlgs__
#define  __sgAlgs__

namespace  sgBoolean
{
	sgCore_API      sgCGroup*	Intersection(const sgC3DObject& aOb,
												const sgC3DObject& bOb);
	sgCore_API      sgCGroup*	Union(const sgC3DObject& aOb,
											const sgC3DObject& bOb, int& errcode);
	sgCore_API      sgCGroup*	Sub(const sgC3DObject& aOb,
											const sgC3DObject& bOb, int& errcode);
	sgCore_API      sgCGroup*	IntersectionContour(const sgC3DObject& aOb,
														const sgC3DObject& bOb);
	sgCore_API      sgCGroup*	Section(const sgC3DObject& obj, 
											const SG_VECTOR& planeNormal, 
											sgFloat planeD);
};

namespace  sgKinematic
{
	sgCore_API     sgCObject*  Rotation(const sgC2DObject&  rotObj,
											const SG_POINT& axePnt1,
											const SG_POINT& axePnt2,
											sgFloat angl_degree,
											bool isClose,
											short  meridians_count=24);

	sgCore_API     sgCObject*  Extrude(const sgC2DObject&  outContour,
											const sgC2DObject** holes,
											int holes_count,
											const SG_VECTOR& extrDir,
											bool isClose);

	sgCore_API     sgCObject*  Spiral(const sgC2DObject&  outContour,
											const sgC2DObject** holes,
											int holes_count,
											const SG_POINT& axePnt1, 
											const SG_POINT& axePnt2,
											sgFloat screw_step,
											sgFloat screw_height,
											short  meridians_count,
											bool isClose);

	sgCore_API     sgCObject*  Pipe(const sgC2DObject&  outContour,
										const sgC2DObject** holes,
										int holes_count,
										const sgC2DObject& guideContour,
										const SG_POINT& point_in_outContour_plane,
										sgFloat angle_around_point_in_outContour_plane,
										bool& isClose);
};


namespace  sgSurfaces
{
	sgCore_API     sgCObject*  Face(const sgC2DObject&  outContour,
										const sgC2DObject** holes,
										int holes_count);

	sgCore_API     sgCObject*  Coons(const sgC2DObject& firstSide,
										const sgC2DObject& secondSide,
										const sgC2DObject& thirdSide,
										const sgC2DObject* fourthSide,
										short uSegments,
										short vSegments,
										short uVisEdges,
										short vVisEdges);

	sgCore_API     sgCObject*  Mesh(short dimens_1, 
										short dimens_2, 
										const SG_POINT* pnts);

	sgCore_API     sgCObject*  SewSurfaces(const sgC3DObject** surfaces, int surf_count);

	sgCore_API     sgCObject*  LinearSurfaceFromSections(const sgC2DObject& firstSide,
															const sgC2DObject& secondSide,
															sgFloat firstParam,
															bool   isClose);
	sgCore_API     sgCObject*  SplineSurfaceFromSections(const sgC2DObject** sections,
															const sgFloat* params,
															int sections_count,
															bool isClose);
};


#endif