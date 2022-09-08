#include "..//Core//sg.h"

sgCCylinder::sgCCylinder():sgC3DObject()
{
}

sgCCylinder::~sgCCylinder()
{
}

sgCCylinder::sgCCylinder(SG_OBJ_HANDLE objH):sgC3DObject(objH)
{
	char* typeID = "{0000000000000-0000-0003-000000000009}";
	set_hobj_attr_value(id_TypeID, objH, typeID);

	PostCreate();

}

sgCCylinder* sgCCylinder::Create(sgFloat rad, sgFloat heig, short merid)
{
	if (rad<eps_d ||
		heig<eps_d ||
		merid<2)
		return NULL;

	sgFloat par[3];
	par[0] = rad;
	par[1] = heig;
	par[2] = (sgFloat)merid;
	
	hOBJ hO = create_primitive_obj(CYL, par); 
	assert(hO);

	sgCCylinder*   newC=new sgCCylinder(hO);

	newC->PostCreate();

	return  newC;
}


void sgCCylinder::GetGeometry(SG_CYLINDER& res) const
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
	res.Height = pars[1];
	res.MeridiansCount = static_cast<short>(pars[2]);
	
	free(pars);
}