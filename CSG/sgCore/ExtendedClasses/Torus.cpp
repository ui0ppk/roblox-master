#include "..//Core//sg.h"

sgCTorus::sgCTorus():sgC3DObject()
{
}

sgCTorus::~sgCTorus()
{
}

sgCTorus::sgCTorus(SG_OBJ_HANDLE objH):sgC3DObject(objH)
{
	char* typeID = "{0000000000000-0000-0005-000000000009}";
	set_hobj_attr_value(id_TypeID, objH, typeID);

	PostCreate();

}

sgCTorus* sgCTorus::Create(sgFloat r1,sgFloat r2,short m1,short m2)
{
	if (r1<eps_d ||
		r2<eps_d ||
		r2>r1 ||
		m1<2 ||
		m2<2)
		return NULL;

	sgFloat par[4];
	par[0] = r1;  
	par[1] = r2;
	par[2] = (sgFloat)m1;
	par[3] = (sgFloat)m2;

	hOBJ hO = create_primitive_obj(TOR, par); 
	assert(hO);

	sgCTorus*   newT=new sgCTorus(hO);

	newT->PostCreate();

	return  newT;
}


void sgCTorus::GetGeometry(SG_TORUS& res) const
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
	res.MeridiansCount1 = static_cast<short>(pars[2]);
	res.MeridiansCount2 = static_cast<short>(pars[3]);
	
	free(pars);
}