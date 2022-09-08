#include "..//Core//sg.h"

sgC2DObject::sgC2DObject():sgCObject()
{
  m_plane_normal.x = m_plane_normal.y = m_plane_normal.z = m_plane_D = 0.0;
}

sgC2DObject::sgC2DObject(SG_OBJ_HANDLE objH):sgCObject(objH)
{
  m_plane_normal.x = m_plane_normal.y = m_plane_normal.z = m_plane_D = 0.0;
}


sgCContour*    sgC2DObject::GetEquidistantContour(sgFloat h1, sgFloat h2, bool toRound)
{
  if (IsLinear() || !IsPlane(NULL,NULL))
    return NULL;

  hOBJ eq_obj=NULL;
  if (!create_equ_path2(GetObjectHandle(this), NULL, h1, h2, false, &eq_obj))
    return NULL;
  if (eq_obj==NULL)
    return NULL;
  sgCObject* retO = ObjectFromHandle(eq_obj);
  if (retO->GetType()!=SG_OT_CONTOUR)
  {
    assert(0);
    return NULL;
  }

  char* typeID = "{0000000000000-0000-0000-000000000007}";
  set_hobj_attr_value(id_TypeID, eq_obj, typeID);

  return reinterpret_cast<sgCContour*>(retO);
}

/*void   sgC2DObject::GetEndPoints(SG_POINT* p1, SG_POINT* p2)
{
  D_POINT tmpP;
  if (p1)
  {
    get_endpoint_on_path(GetObjectHandle(this),&tmpP,(OSTATUS)0);
    memcpy(p1,&tmpP,sizeof(SG_POINT));
  }
  if (p2)
  {
    get_endpoint_on_path(GetObjectHandle(this),&tmpP,ST_DIRECT);
    memcpy(p2,&tmpP,sizeof(SG_POINT));
  }
}*/

sgC2DObject::SG_2D_OBJECT_ORIENT   sgC2DObject::GetOrient(const SG_VECTOR& planeNormal) const
{
  if (!IsClosed())
    return sgC2DObject::OO_ERROR;

  D_PLANE pll;
  pll.d = 0.0;
  pll.v.x = planeNormal.x;pll.v.y = planeNormal.y;pll.v.z = planeNormal.z;
  switch(test_orient_path(GetObjectHandle(this), &pll))
  {
  case 1:
    return sgC2DObject::OO_ANTICLOCKWISE;
  case -1:
    return sgC2DObject::OO_CLOCKWISE;
  }
  return sgC2DObject::OO_ERROR;
}

bool      sgC2DObject::ChangeOrient()
{
  lpOBJ obj = (lpOBJ)GetObjectHandle(this);
  obj->status ^= ST_DIRECT;
  return true;
}

SG_POINT  sgC2DObject::GetPointFromCoefficient(sgFloat coeff) const
{
  D_POINT   tmpP;
  SG_POINT  resP = {0.0, 0.0, 0.0};
  if (coeff<eps_d)
  {
    get_endpoint_on_path(GetObjectHandle(this),&tmpP,(OSTATUS)0);
    memcpy(&resP,&tmpP,sizeof(SG_POINT));
    return resP;
  }
  if (coeff>(1.0-eps_d))
  {
    get_endpoint_on_path(GetObjectHandle(this),&tmpP,ST_DIRECT);
    memcpy(&resP,&tmpP,sizeof(SG_POINT));
    return resP;
  }
  if (!init_get_point_on_path(GetObjectHandle(this), NULL))
  {
    assert(0);
    return resP;
  }
  if (!get_point_on_path(coeff, &tmpP))
  {
    assert(0);
    return resP;
  }
  memcpy(&resP,&tmpP,sizeof(SG_POINT));
  term_get_point_on_path();
  return resP;
}

bool  sgC2DObject::IsObjectsOnOnePlane(const sgC2DObject& obj1,
                     const sgC2DObject& obj2)
{
  if (obj1.GetType()==SG_OT_LINE || obj2.GetType()==SG_OT_LINE)
  {
    if (obj1.GetType()==SG_OT_LINE && obj2.GetType()==SG_OT_LINE)
    {
      SG_POINT pp[4];
      sgCLine* f_l = reinterpret_cast<sgCLine*>(const_cast<sgC2DObject*>(&obj1));
      pp[0] = f_l->GetGeometry()->p1;
      pp[1] = f_l->GetGeometry()->p2;
      sgCLine* s_l = reinterpret_cast<sgCLine*>(const_cast<sgC2DObject*>(&obj2));
      pp[2] = s_l->GetGeometry()->p1;
      pp[3] = s_l->GetGeometry()->p2;
      if (!sgSpaceMath::IsPointsOnOneLine(pp[0],pp[1],pp[2]))
      {
        SG_VECTOR plNN;
        sgFloat    plDD;
        sgSpaceMath::PlaneFromPoints(pp[0],pp[1],pp[2],plNN,plDD);
        if (sgSpaceMath::GetPlanePointDistance(pp[3],plNN,plDD)<eps_d)
          return true;
        else
          return false;
      }
      else
        if (!sgSpaceMath::IsPointsOnOneLine(pp[0],pp[1],pp[3]))
        {
          SG_VECTOR plNN;
          sgFloat    plDD;
          sgSpaceMath::PlaneFromPoints(pp[0],pp[1],pp[3],plNN,plDD);
          if (sgSpaceMath::GetPlanePointDistance(pp[2],plNN,plDD)<eps_d)
            return true;
          else
            return false;
        }
        else
          return true;
    }
    else
      if (obj1.GetType()==SG_OT_LINE && obj2.GetType()!=SG_OT_LINE)
      {
        SG_VECTOR plNN;
        sgFloat    plDD;
        if (!obj2.IsPlane(&plNN,&plDD))
          return false;
        SG_POINT pp[2];
        sgCLine* f_l = reinterpret_cast<sgCLine*>(const_cast<sgC2DObject*>(&obj1));
        pp[0] = f_l->GetGeometry()->p1;
        pp[1] = f_l->GetGeometry()->p2;

        if (sgSpaceMath::GetPlanePointDistance(pp[0],plNN,plDD)<eps_d &&
          sgSpaceMath::GetPlanePointDistance(pp[1],plNN,plDD)<eps_d)
          return true;
        else
          return false;
      }
      else
        if (obj2.GetType()==SG_OT_LINE && obj1.GetType()!=SG_OT_LINE)
        {
          SG_VECTOR plNN;
          sgFloat    plDD;
          if (!obj1.IsPlane(&plNN,&plDD))
            return false;
          SG_POINT pp[2];
          sgCLine* s_l = reinterpret_cast<sgCLine*>(const_cast<sgC2DObject*>(&obj2));
          pp[0] = s_l->GetGeometry()->p1;
          pp[1] = s_l->GetGeometry()->p2;

          if (sgSpaceMath::GetPlanePointDistance(pp[0],plNN,plDD)<eps_d &&
            sgSpaceMath::GetPlanePointDistance(pp[1],plNN,plDD)<eps_d)
            return true;
          else
            return false;
        }
        else
        {
          assert(0);
          return false;
        }
  }


  D_POINT f_normal;
  D_POINT sec_normal;
  sgFloat  f_D;
  sgFloat  s_D;
  SG_VECTOR f_n;
  SG_VECTOR s_n;

  if (!obj1.IsPlane(&f_n,&f_D))
    return false;
  if (!obj2.IsPlane(&s_n,&s_D))
    return false;
  memcpy(&f_normal,&f_n,sizeof(D_POINT));
  memcpy(&sec_normal,&s_n,sizeof(D_POINT));

  if (!((dpoint_eq(&f_normal,&sec_normal,eps_n) &&
    fabs(f_D - s_D) < eps_d) ||
    (dpoint_eq(&f_normal,dpoint_inv(&sec_normal,&sec_normal),eps_n) &&
    fabs(f_D + s_D) < eps_d)))
  {
    return false;
  }
  return true;
}

bool  sgC2DObject::IsObjectsIntersecting(const sgC2DObject& obj1,
                     const sgC2DObject& obj2)
{
  if (!IsObjectsOnOnePlane(obj1,obj2))
    return false;

  OSCAN_COD scc = test_self_cross_path(GetObjectHandle(&obj1), GetObjectHandle(&obj2));
  if (scc==OSTRUE)
    return false;
  return true;
}

bool  sgC2DObject::IsFirstObjectInsideSecondObject(const sgC2DObject& obj1,
                           const sgC2DObject& obj2)
{
  if (!IsObjectsOnOnePlane(obj1,obj2))
    return false;

  D_PLANE   pll;
  SG_VECTOR f_n;

  if (!obj1.IsPlane(&f_n,&pll.d))
    return false;

  memcpy(&pll.v,&f_n,sizeof(D_POINT));

  short scc = test_inside_path(GetObjectHandle(&obj1), GetObjectHandle(&obj2),&pll);
  if (scc==1)
    return true;
  return false;
}
