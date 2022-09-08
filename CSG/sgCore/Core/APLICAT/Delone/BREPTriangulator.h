#ifndef  __BREP_TRIANGULATOR__
#define  __BREP_TRIANGULATOR__

#include "Delone.h"
#include <list>

typedef struct
{
  MATR           face_invert_matr;
  CTriangulator* face_triangulator;
  SG_VECTOR      face_normal;
  SG_POINT*      points_of_triangles_of_3_or_4_faces;
  VERTINFO*      vertInfo_of_triangles_of_3_or_4_faces;
  unsigned int   count_of_points_of_triangles_of_3_or_4_faces;
  SG_VECTOR*     normals_of_3_or_4_faces;
} ONE_FACE_TRIANGLULATOR;

class CBREPTriangulator
{
public:
  CBREPTriangulator(hOBJ objH);
  ~CBREPTriangulator();

#ifndef NEW_TRIANGULATION
  bool    TriangulateOneNP(NP_STR_LIST* list_str, NPW* cur_np);
#else
  bool    TriangulateOneNP(short, int&);
#endif
  bool    EndTriangulate();

private:
  hOBJ                    m_obj_handle;
  NP_STR_LIST*            m_list_str;
  NPW*                    m_cur_np; 
  BYTE			          m_TypeFace;

  int*                    m_markers;
  
  sgFloat*                 m_nodes_buffer;
  unsigned int            m_nodes_count;

  sgFloat*                 m_normals_buffer;

  int*                    m_segments_buffer;
  unsigned int            m_segments_count;

  sgFloat*                 m_holes_buffer;
  unsigned int            m_holes_count;

  std::list<ONE_FACE_TRIANGLULATOR> m_triangulators_buffer;

  int                     m_nTr;      
  SG_POINT*               m_allVertex;
  SG_VECTOR*              m_allNormals;
  SG_POINT*               m_allColors;

  bool                    CalcOptimalTrianglesFor4Face();
  SG_POINT                GetAnyInsidePointOfControur(sgFloat* start_pnt, unsigned int count);
public:
	int                     GetTrianglesCount() const {return m_nTr;};
	SG_POINT*               GetTrianglesPoints() const {return m_allVertex;};
	SG_VECTOR*              GetTrianglesNormals() const {return m_allNormals;};
	SG_POINT*               GetTrianglesColors() const {return m_allColors;};
};

#endif