#include "..//Core//sg.h"

sgCCone::sgCCone():sgC3DObject()
{
}

sgCCone::~sgCCone()
{
}

sgCCone::sgCCone(SG_OBJ_HANDLE objH):sgC3DObject(objH)
{
	char* typeID = "{0000000000000-0000-0004-000000000009}";
	set_hobj_attr_value(id_TypeID, objH, typeID);

	PostCreate();

}

sgCCone* sgCCone::Create(sgFloat rad_1,sgFloat rad_2,sgFloat heig, short merid)
{
	if ((rad_1<eps_d &&
		rad_2<eps_d) ||
		heig<eps_d ||
		merid<2)
		return NULL;

	sgFloat par[4];
	par[0] = rad_1;
	par[1] = rad_2;
	par[2] = heig;
	par[3] = (sgFloat)merid;

	hOBJ hO = create_primitive_obj(CONE, par); 
	assert(hO);

	sgCCone*   newC=new sgCCone(hO);

	newC->PostCreate();

	return  newC;
}


void sgCCone::GetGeometry(SG_CONE& res) const
{
	lpOBJ 	obj = static_cast<lpOBJ>(GetObjectHandle(this));
	assert(obj);

	lpGEO_BREP 	geo = (lpGEO_BREP)(obj->geo_data);
	assert(geo);

	LNP		  	lnp;

	if ( !read_elem(&vd_brep,geo->num,&lnp) )
	{
		assert(0);
	}

	lpCSG  csg = static_cast<lpCSG>(lnp.hcsg);
	assert(csg);

	sgFloat*  pars = (sgFloat*)malloc(csg->size);

	memcpy(pars, csg->data, csg->size);

	res.Radius1 = pars[0];
	res.Radius2 = pars[1];
	res.Height = pars[2];
	res.MeridiansCount = static_cast<short>(pars[3]);
	
	free(pars);
}