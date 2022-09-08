#ifndef  __sg3D__
#define  __sg3D__

/************************************************************************/
/* 3DObject                                                                */
/************************************************************************/
typedef struct
{
  unsigned short begin_vertex_index;
  unsigned short end_vertex_index;
  unsigned short edge_type;
}  SG_EDGE;

typedef struct 
{
	long   ver_indexes[3];
} SG_INDEX_TRIANGLE;

/* edge_type:*/
#define  SG_EDGE_1_LEVEL          0x0001
#define  SG_EDGE_2_LEVEL          0x0002
#define  SG_EDGE_3_LEVEL          0x0004

typedef enum
{
  SG_UNKNOWN_3D=0,
  SG_BODY,
  SG_SURFACE,
} SG_3DOBJECT_TYPE;

typedef enum
{
	SG_VERTEX_TRIANGULATION,
	SG_DELAUNAY_TRIANGULATION
} SG_TRIANGULATION_TYPE;

typedef struct
{
  int                nTr;
  SG_POINT*          allVertex;
  SG_VECTOR*         allNormals;
  SG_POINT*          allColors;
  sgFloat*            allUV;
} SG_ALL_TRIANGLES;

typedef  enum
{
  SG_MODULATE_MIX_TYPE=1,
  SG_BLEND_MIX_TYPE   =2,
  SG_REPLACE_MIX_TYPE =3
} SG_MIX_COLOR_TYPE;

typedef  enum
{
  SG_CUBE_UV_TYPE      =1,
  SG_SPHERIC_UV_TYPE   =2,
  SG_CYLINDER_UV_TYPE  =3
} SG_UV_TYPE;

typedef  struct
{
  int       MaterialIndex;
  sgFloat    TextureScaleU;
  sgFloat    TextureScaleV;
  sgFloat    TextureShiftU;
  sgFloat    TextureShiftV;
  sgFloat    TextureAngle;
  bool      TextureSmooth;
  bool      TextureMult;
  SG_MIX_COLOR_TYPE MixColorType;
  SG_UV_TYPE        TextureUVType;
} SG_MATERIAL;

class sgC3DObject;
class sgCBRep;

/************************************************************************/
/* Boundary representation piece                                        */
/************************************************************************/
class sgCore_API   sgCBRepPiece
{
	friend class  sgCBRep;
	PRIVATE_ACCESS
private:
	void*         m_brep_piece_handle;

	SG_POINT*     m_vertexes;
	unsigned int  m_vertexes_count;

	SG_EDGE*      m_edges;
	unsigned int  m_edges_count;

	SG_POINT      m_min;
	SG_POINT      m_max;

	int           m_min_triangle_number;
	int           m_max_triangle_number;
	
	sgCBRepPiece();
	~sgCBRepPiece();
public:
	void             GetLocalGabarits(SG_POINT& p_min, SG_POINT& p_max) const;

	const SG_POINT*  GetVertexes() const;
	unsigned int     GetVertexesCount() const;

	const SG_EDGE*   GetEdges() const;
	unsigned int     GetEdgesCount() const;

	void             GetTrianglesRange(int& min_numb, int& max_numb) const;
};

/************************************************************************/
/* Boundary representation                                              */
/************************************************************************/
class sgCore_API   sgCBRep
{
	friend class sgC3DObject;
	PRIVATE_ACCESS
private:
    sgCBRep();
	~sgCBRep();

	sgCBRepPiece**         m_pieces;
	unsigned int           m_pieces_count;
public:
	sgCBRepPiece*          GetPiece(unsigned int) const;
	unsigned int           GetPiecesCount() const;
};

/************************************************************************/
/* 3D object base class                                                 */
/************************************************************************/
class sgCore_API   sgC3DObject : public sgCObject
{
protected:
  sgC3DObject();
  sgC3DObject(SG_OBJ_HANDLE);

  virtual    ~sgC3DObject();

public:
  static     void         AutoTriangulate(bool,SG_TRIANGULATION_TYPE);

  bool                    Triangulate(SG_TRIANGULATION_TYPE);

  SG_3DOBJECT_TYPE        Get3DObjectType() const;

  sgCBRep*                GetBRep()  const;
  const SG_ALL_TRIANGLES* GetTriangles() const;

  virtual  bool           ApplyTempMatrix();
  const sgFloat*           GetWorldMatrixData() const;

  void                    SetMaterial(const SG_MATERIAL&);
  const  SG_MATERIAL*     GetMaterial();
  bool                    CalculateOptimalUV(sgFloat& optU, sgFloat& optV);

  sgFloat                  GetVolume();
  sgFloat                  GetSquare();

  void                    SetColor(const SG_POINT& color);
  void                    Scale(const SG_POINT& scale);
  void                    Translate(const SG_POINT& translation);
  void                    Transform(const sgCMatrix& matrix);

private:
  SG_3DOBJECT_TYPE    m_objectType;
  sgCBRep*            m_brep;
  bool                CopyBRepStructure();
  SG_ALL_TRIANGLES*   m_triangles;
  void                CalcUV();
  sgCMatrix*          m_world_matrix;
  SG_MATERIAL*        m_material;

  PRIVATE_ACCESS
};



/************************************************************************/
/* Box                                                                  */
/************************************************************************/

typedef struct
{
  sgFloat  SizeX;
  sgFloat  SizeY;
  sgFloat  SizeZ;
} SG_BOX;

class sgCore_API   sgCBox : public sgC3DObject
{
private:
  sgCBox();
  sgCBox(SG_OBJ_HANDLE);
  virtual    ~sgCBox();

public:
  static   sgCBox*    Create(sgFloat sizeX, sgFloat sizeY, sgFloat sizeZ);
  void                GetGeometry(SG_BOX& ) const;
  PRIVATE_ACCESS
};
#define  sgCreateBox  sgCBox::Create



/************************************************************************/
/* Sphere                                                                 */
/************************************************************************/

typedef struct
{
  sgFloat  Radius;
  short   MeridiansCount;
  short   ParallelsCount;
} SG_SPHERE;

class sgCore_API   sgCSphere : public sgC3DObject
{
private:
  sgCSphere();
  sgCSphere(SG_OBJ_HANDLE);
  virtual    ~sgCSphere();

public:
  static   sgCSphere*    Create(sgFloat rad, short merid, short parall);
  void                   GetGeometry(SG_SPHERE&) const;
  PRIVATE_ACCESS
};
#define  sgCreateSphere  sgCSphere::Create



/************************************************************************/
/* Cylinder                                                                 */
/************************************************************************/

typedef struct
{
  sgFloat  Radius;
  sgFloat  Height;
  short   MeridiansCount;
} SG_CYLINDER;

class sgCore_API   sgCCylinder : public sgC3DObject
{
private:
  sgCCylinder();
  sgCCylinder(SG_OBJ_HANDLE);
  virtual    ~sgCCylinder();

public:

  static   sgCCylinder*    Create(sgFloat rad, sgFloat heig, short merid);
  void           GetGeometry(SG_CYLINDER&) const;
  PRIVATE_ACCESS
};
#define  sgCreateCylinder  sgCCylinder::Create




/************************************************************************/
/* Cone                                                                 */
/************************************************************************/

typedef struct
{
  sgFloat  Radius1;
  sgFloat  Radius2;
  sgFloat  Height;
  short   MeridiansCount;
} SG_CONE;

class sgCore_API   sgCCone : public sgC3DObject
{
private:
  sgCCone();
  sgCCone(SG_OBJ_HANDLE);
  virtual    ~sgCCone();

public:
  static   sgCCone*    Create(sgFloat rad_1,sgFloat rad_2,sgFloat heig, short merid);
  void         GetGeometry(SG_CONE&) const;
  PRIVATE_ACCESS
};
#define  sgCreateCone  sgCCone::Create

/************************************************************************/
/* Torus                                                                 */
/************************************************************************/

typedef struct
{
  sgFloat  Radius1;
  sgFloat  Radius2;
  short   MeridiansCount1;
  short   MeridiansCount2;
} SG_TORUS;

class sgCore_API   sgCTorus : public sgC3DObject
{
private:
  sgCTorus();
  sgCTorus(SG_OBJ_HANDLE);
  virtual    ~sgCTorus();

public:

  static   sgCTorus*    Create(sgFloat r1,sgFloat r2,short m1,short m2);
  void          GetGeometry(SG_TORUS&) const;
  PRIVATE_ACCESS
};
#define  sgCreateTorus  sgCTorus::Create

/************************************************************************/
/* Ellipsoid                                                                 */
/************************************************************************/

typedef struct
{
  sgFloat  Radius1;
  sgFloat  Radius2;
  sgFloat  Radius3;
  short   MeridiansCount;
  short   ParallelsCount;
} SG_ELLIPSOID;

class sgCore_API   sgCEllipsoid : public sgC3DObject
{
private:
  sgCEllipsoid();
  sgCEllipsoid(SG_OBJ_HANDLE);
  virtual    ~sgCEllipsoid();

public:

  static   sgCEllipsoid*    Create(sgFloat radius1, sgFloat radius2, sgFloat radius3,
                    short merid_cnt, short parall_cnt);
  void            GetGeometry(SG_ELLIPSOID&) const;
  PRIVATE_ACCESS
};
#define  sgCreateEllipsoid  sgCEllipsoid::Create

/************************************************************************/
/* SphericBand                                                                 */
/************************************************************************/

typedef struct
{
  sgFloat  Radius;
  sgFloat  BeginCoef;
  sgFloat  EndCoef;
  short   MeridiansCount;
} SG_SPHERIC_BAND;

class sgCore_API   sgCSphericBand : public sgC3DObject
{
private:
  sgCSphericBand();
  sgCSphericBand(SG_OBJ_HANDLE);
  virtual    ~sgCSphericBand();

public:
  static   sgCSphericBand*    Create(sgFloat radius, sgFloat beg_koef,
                      sgFloat end_koef,
                      short merid_cnt);
  void              GetGeometry(SG_SPHERIC_BAND&) const;

  PRIVATE_ACCESS
};
#define  sgCreateSphericBand    sgCSphericBand::Create


#endif