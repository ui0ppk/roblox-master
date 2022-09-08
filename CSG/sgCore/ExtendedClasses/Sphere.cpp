#include "..//Core//sg.h"

sgCSphere::sgCSphere():sgC3DObject()
{
}

sgCSphere::~sgCSphere()
{
}

sgCSphere::sgCSphere(SG_OBJ_HANDLE objH):sgC3DObject(objH)
{
	char* typeID = "{0000000000000-0000-0002-000000000009}";
	set_hobj_attr_value(id_TypeID, objH, typeID);

	PostCreate();

}

sgCSphere* sgCSphere::Create(sgFloat rad, short merid, short parall)
{
	if (rad<eps_d ||
		merid<2 ||
		parall<2)
		return NULL;

	sgFloat par[3];
	par[0] = rad;
	par[1] = (sgFloat)merid;
	par[2] = (sgFloat)parall;

	hOBJ hO = create_primitive_obj(SPHERE, par); 
	assert(hO);

	sgCSphere*   newSp=new sgCSphere(hO);

	newSp->PostCreate();

	return  newSp;
}


void sgCSphere::GetGeometry(SG_SPHERE& res) const
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

	res.Radius = pars[0];
	res.MeridiansCount = static_cast<short>(pars[1]);
	res.ParallelsCount = static_cast<short>(pars[2]);

	free(pars);
}