#include "..//Core//sg.h"

sgCPoint::sgCPoint():sgCObject()
{
}

sgCPoint::~sgCPoint()
{
}

sgCPoint::sgCPoint(SG_OBJ_HANDLE objH):sgCObject(objH)
{
	char* typeID = "{0000000000000-0000-0000-000000000001}";
	set_hobj_attr_value(id_TypeID, objH, typeID);

	PostCreate();

}
sgCPoint* sgCPoint::Create(sgFloat pX, sgFloat pY, sgFloat pZ)
{
	SG_POINT gp;
	gp.x = pX;
	gp.y = pY;
	gp.z = pZ;
	
	sgCPoint*   newPnt=new sgCPoint;
	SetObjectHandle(newPnt,  create_simple_obj_loc(OPOINT, &gp));
	((lpOBJ)(GetObjectHandle(newPnt)))->extendedClass = newPnt;

	char* typeID = "{0000000000000-0000-0000-000000000001}";
	set_hobj_attr_value(id_TypeID, GetObjectHandle(newPnt), typeID);

	newPnt->PostCreate();

	return  newPnt;
}

const SG_POINT* sgCPoint::GetGeometry() const
{
	lpOBJ pobj = (lpOBJ)(GetObjectHandle(this));
	return static_cast<SG_POINT*>((void*)(pobj->geo_data));
}