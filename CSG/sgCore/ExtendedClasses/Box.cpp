#include "..//Core//sg.h"

sgCBox::sgCBox():sgC3DObject()
{
}

sgCBox::~sgCBox()
{
}

sgCBox::sgCBox(SG_OBJ_HANDLE objH):sgC3DObject(objH)
{
	char* typeID = "{0000000000000-0000-0001-000000000009}";
	set_hobj_attr_value(id_TypeID, objH, typeID);

	PostCreate();

}

sgCBox* sgCBox::Create(sgFloat sizeX, sgFloat sizeY, sgFloat sizeZ)
{
	if (sizeX<eps_d ||
		sizeY<eps_d ||
		sizeZ<eps_d)
		return NULL;
	sgFloat par[3];
	par[0] = sizeX;
	par[1] = sizeY;
	par[2] = sizeZ;

	hOBJ hO = create_primitive_obj(BOX, par); 
	assert(hO);

	sgCBox*   newBox=new sgCBox(hO);

	newBox->PostCreate();

	return  newBox;
}


void sgCBox::GetGeometry(SG_BOX& res) const
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
	
	memcpy(&res, csg->data, csg->size);
}