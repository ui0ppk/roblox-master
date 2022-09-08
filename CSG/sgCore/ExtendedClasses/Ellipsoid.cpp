#include "..//Core//sg.h"

sgCEllipsoid::sgCEllipsoid():sgC3DObject()
{
}

sgCEllipsoid::~sgCEllipsoid()
{
}

sgCEllipsoid::sgCEllipsoid(SG_OBJ_HANDLE objH):sgC3DObject(objH)
{
	char* typeID = "{0000000000000-0000-0008-000000000009}";
	set_hobj_attr_value(id_TypeID, objH, typeID);

	PostCreate();

}

sgCEllipsoid* sgCEllipsoid::Create(sgFloat radius1, sgFloat radius2, sgFloat radius3,
											short merid_cnt, short parall_cnt)
{

	if (radius1<eps_d ||
		radius2<eps_d ||
		radius3<eps_d ||
		merid_cnt<2 ||
		parall_cnt<2)
			return NULL;

	sgFloat par[5];

	par[0] = radius1;
	par[1] = radius2;
	par[2] = radius3;
	par[3] = (sgFloat)merid_cnt;
	par[4] = (sgFloat)parall_cnt;


	hOBJ hO = create_primitive_obj(ELIPSOID, par); 
	assert(hO);

	sgCEllipsoid*   newE=new sgCEllipsoid(hO);

	newE->PostCreate();

	return  newE;
}


void sgCEllipsoid::GetGeometry(SG_ELLIPSOID& res) const
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
	res.Radius3 = pars[2];
	res.MeridiansCount = static_cast<short>(pars[3]);
	res.ParallelsCount = static_cast<short>(pars[4]);

	free(pars);
}