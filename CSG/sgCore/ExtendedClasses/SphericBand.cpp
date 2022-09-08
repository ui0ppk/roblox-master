#include "..//Core//sg.h"

sgCSphericBand::sgCSphericBand():sgC3DObject()
{
}

sgCSphericBand::~sgCSphericBand()
{
}

sgCSphericBand::sgCSphericBand(SG_OBJ_HANDLE objH):sgC3DObject(objH)
{
  char* typeID = "{0000000000000-0000-0009-000000000009}";
  set_hobj_attr_value(id_TypeID, objH, typeID);

  PostCreate();

}

sgCSphericBand* sgCSphericBand::Create(sgFloat radius, sgFloat beg_koef,
                          sgFloat end_koef,
                          short merid_cnt)
{
  if (radius<eps_d ||
    merid_cnt<2 ||
    beg_koef>1 ||
    beg_koef<-1 ||
    end_koef>1 ||
    end_koef<-1 ||
    end_koef<=beg_koef)
    return NULL;

  sgFloat par[5];

  par[0] = radius;
  par[1] = beg_koef;
  par[2] = end_koef;
  par[3] = 0.0;
  par[4] = (sgFloat)merid_cnt;


  hOBJ hO = create_primitive_obj(SPHERIC_BAND, par);
  assert(hO);

  sgCSphericBand*   newSp=new sgCSphericBand(hO);

  newSp->PostCreate();

  return  newSp;
}


void sgCSphericBand::GetGeometry(SG_SPHERIC_BAND& res) const
{
  lpOBJ   obj = static_cast<lpOBJ>(GetObjectHandle(this));
  assert(obj);

  lpGEO_BREP  geo = (lpGEO_BREP)(obj->geo_data);
  assert(geo);

  LNP       lnp;

  if ( !read_elem(&vd_brep,geo->num,&lnp) )
  {
    assert(0);
  }

  lpCSG  csg = static_cast<lpCSG>(lnp.hcsg);
  assert(csg);

  sgFloat*  pars = (sgFloat*)malloc(csg->size);

  memcpy(pars, csg->data, csg->size);

  res.Radius = pars[0];
  res.BeginCoef = pars[1];
  res.EndCoef = pars[2];
  res.MeridiansCount = static_cast<short>(pars[4]);

  free(pars);
}