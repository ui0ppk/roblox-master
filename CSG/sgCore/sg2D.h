#ifndef  __sg2D__
#define  __sg2D__


/************************************************************************/
/* POINT                                                                */
/************************************************************************/
class sgCore_API   sgCPoint : public sgCObject
{
private:
	sgCPoint();
	sgCPoint(SG_OBJ_HANDLE);
	virtual    ~sgCPoint();
public:	
	static   sgCPoint*    Create(sgFloat pX,sgFloat pY,sgFloat pZ);
	const    SG_POINT*    GetGeometry() const; 
	PRIVATE_ACCESS
};
#define  sgCreatePoint  sgCPoint::Create



/************************************************************************/
/* BASE CLASS FOR 2D OBJECTS                                            */
/************************************************************************/
class sgCContour;

class sgCore_API   sgC2DObject : public sgCObject
{
protected:
	sgC2DObject();
	sgC2DObject(SG_OBJ_HANDLE);

	SG_VECTOR      m_plane_normal;
	sgFloat         m_plane_D;
public:
	virtual  bool        IsClosed() const											=0;
	virtual  bool        IsPlane(SG_VECTOR*,sgFloat*) const							=0;
	virtual  bool        IsLinear() const											=0;
	virtual  bool        IsSelfIntersecting()  const								=0;

	sgCContour*			 GetEquidistantContour(sgFloat h1, sgFloat h2, bool toRound);

	typedef enum
	{
		OO_ERROR=0,
		OO_CLOCKWISE,
		OO_ANTICLOCKWISE
	} SG_2D_OBJECT_ORIENT;

	SG_2D_OBJECT_ORIENT  GetOrient(const SG_VECTOR& planeNormal) const;
	bool                 ChangeOrient();
	SG_POINT             GetPointFromCoefficient(sgFloat coeff) const;

	static   bool        IsObjectsOnOnePlane(const sgC2DObject& obj1, 
												const sgC2DObject& obj2);
	static   bool        IsObjectsIntersecting(const sgC2DObject& obj1, 
												const sgC2DObject& obj2);
	static   bool        IsFirstObjectInsideSecondObject(const sgC2DObject& obj1, 
															const sgC2DObject& obj2);
};

/************************************************************************/
/* LINE                                                                 */
/************************************************************************/
class sgCore_API   sgCLine : public sgC2DObject
{
private:
	sgCLine();
	sgCLine(SG_OBJ_HANDLE);
	virtual    ~sgCLine();
public:
	static   sgCLine*    Create(sgFloat pX1, sgFloat pY1, sgFloat pZ1,
									sgFloat pX2, sgFloat pY2, sgFloat pZ2);
	const    SG_LINE*    GetGeometry() const; 

	virtual  bool        IsClosed() const;
	virtual  bool        IsPlane(SG_VECTOR*,sgFloat*) const;
	virtual  bool        IsLinear() const;
	virtual  bool        IsSelfIntersecting()  const;

	PRIVATE_ACCESS
};
#define  sgCreateLine  sgCLine::Create

/************************************************************************/
/* CIRCLE                                                                 */
/************************************************************************/
struct sgCore_API  SG_CIRCLE
{
	sgFloat	   radius;
	SG_VECTOR  normal;          // Normal
	SG_POINT   center;
	bool    FromCenterRadiusNormal(const SG_POINT& cen,	sgFloat rad, const SG_VECTOR& nor);
	bool    FromThreePoints(const SG_POINT& p1, const SG_POINT& p2, const SG_POINT& p3);
	bool    Draw(SG_DRAW_LINE_FUNC) const;
};

class sgCore_API   sgCCircle : public sgC2DObject
{
private:
	sgCCircle();
	sgCCircle(SG_OBJ_HANDLE);
	virtual    ~sgCCircle();

	SG_POINT*   m_points;
	int         m_points_count;
public:
	static   sgCCircle*   Create(const SG_CIRCLE&);
	const    SG_CIRCLE*   GetGeometry() const; 
	
	int				      GetPointsCount() const;
	const    SG_POINT*    GetPoints() const;

	virtual  bool		  ApplyTempMatrix();

	virtual  bool         IsClosed() const;
	virtual  bool         IsPlane(SG_VECTOR*,sgFloat*) const;
	virtual  bool         IsLinear() const;
	virtual  bool         IsSelfIntersecting()  const;

	PRIVATE_ACCESS
};
#define  sgCreateCircle  sgCCircle::Create



/************************************************************************/
/* ARC                                                                 */
/************************************************************************/
struct sgCore_API SG_ARC
{
	sgFloat	  radius;
	SG_VECTOR normal; 
	SG_POINT  center;
	SG_POINT  begin;
	SG_POINT  end;
	sgFloat    begin_angle;
	sgFloat    angle;
	// angle in degree = Arc->angle*180.0/PI	
    bool    FromThreePoints(const SG_POINT& begP,const SG_POINT& endP,
							const SG_POINT& midP,bool invert);
	
	bool    FromCenterBeginEnd(const SG_POINT& cenP,const SG_POINT& begP,
								const SG_POINT& endP,	bool invert);
	
	bool    FromBeginEndNormalRadius(const SG_POINT& begP,	const SG_POINT& endP,
									const SG_VECTOR& nrmlV,	sgFloat rad,	bool invert);

	bool    FromCenterBeginNormalAngle(const SG_POINT& cenP, const SG_POINT& begP,
										const SG_VECTOR& nrmlV, sgFloat ang);

	bool    FromBeginEndNormalAngle(const SG_POINT& begP, const SG_POINT& endP,
										const SG_VECTOR& nrmlV,	sgFloat ang);

	bool    Draw(SG_DRAW_LINE_FUNC) const;
};

class sgCore_API   sgCArc : public sgC2DObject
{
private:
	sgCArc();
	sgCArc(SG_OBJ_HANDLE);
	virtual    ~sgCArc();

	SG_POINT*   m_points;
	int         m_points_count;
public:
	static   sgCArc*      Create(const SG_ARC&);
	const    SG_ARC*      GetGeometry() const; 
	
	int		              GetPointsCount() const;
	const    SG_POINT*    GetPoints() const;

	virtual  bool	      ApplyTempMatrix();

	virtual  bool         IsClosed() const;
	virtual  bool         IsPlane(SG_VECTOR*,sgFloat*) const;
	virtual  bool         IsLinear() const;
	virtual  bool         IsSelfIntersecting()  const;

	PRIVATE_ACCESS
};
#define  sgCreateArc  sgCArc::Create




/************************************************************************/
/* SPLINE                                                               */
/************************************************************************/

class  sgCore_API SG_SPLINE
{
	friend class sgCSpline;

	void*        m_handle;
	SG_POINT*    m_points;
	int			 m_points_count;
	SG_POINT*    m_knots;
	int			 m_knots_count;
private:
	SG_SPLINE();
	SG_SPLINE(void*);
	~SG_SPLINE();
	void                   Recalc();
public:
	static   SG_SPLINE*    Create();
	static   void          Delete(SG_SPLINE*);

	bool                   AddKnot(const SG_POINT& pnt, int nmbr);
	bool                   MoveKnot(int nmbr, const SG_POINT& pnt);
	bool                   DeleteKnot(int nmbr);

	bool                   IsClosed() const;
	bool                   Close();
	bool                   UnClose(int);

	const    SG_POINT*     GetPoints() const;
	int                    GetPointsCount() const;

	const    SG_POINT*     GetKnots() const;
	int                    GetKnotsCount() const;
};

class sgCore_API   sgCSpline : public sgC2DObject
{
private:
	sgCSpline();
	sgCSpline(SG_OBJ_HANDLE);
	virtual    ~sgCSpline();
private:
	SG_SPLINE*    m_spline_geo;
public:

	static   sgCSpline*   Create(const SG_SPLINE&);
	const    SG_SPLINE*   GetGeometry() const;

	virtual  bool         IsClosed() const;
	virtual  bool         IsPlane(SG_VECTOR*,sgFloat*) const;
	virtual  bool         IsLinear() const;
	virtual  bool         IsSelfIntersecting()  const;

	virtual  bool		  ApplyTempMatrix();

	PRIVATE_ACCESS
};
#define  sgCreateSpline  sgCSpline::Create


/************************************************************************/
/* Contour                                                                */
/************************************************************************/
class sgCore_API   sgCContour : public sgC2DObject
{
private:
	sgCContour();
	sgCContour(SG_OBJ_HANDLE);
	virtual    ~sgCContour();

	static   bool         TopologySort(sgCObject**, int);

	IObjectsList*         m_children;
public:

	IObjectsList*         GetChildrenList() const;

	virtual  bool         SetAttribute(SG_OBJECT_ATTR_ID, unsigned short);

	virtual  bool	      ApplyTempMatrix();

	virtual  bool         IsClosed() const;
	virtual  bool         IsPlane(SG_VECTOR*,sgFloat*) const;
	virtual  bool         IsLinear() const;
	virtual  bool         IsSelfIntersecting()  const;

	static   sgCContour*  CreateContour(sgCObject** objcts, int cnt);
	bool                  BreakContour(sgCObject** objcts);

	PRIVATE_ACCESS
};

#endif