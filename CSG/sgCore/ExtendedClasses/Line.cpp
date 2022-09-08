#include "..//Core//sg.h"

sgCLine::sgCLine():sgC2DObject()
{
}

sgCLine::~sgCLine()
{
}

sgCLine::sgCLine(SG_OBJ_HANDLE objH):sgC2DObject(objH)
{
	char* typeID = "{0000000000000-0000-0000-000000000002}";
	set_hobj_attr_value(id_TypeID, objH, typeID);

	PostCreate();

}

sgCLine* sgCLine::Create(sgFloat pX1, sgFloat pY1, sgFloat pZ1,
										sgFloat pX2, sgFloat pY2, sgFloat pZ2)
{
	SG_LINE gp;
	gp.p1.x = pX1;
	gp.p1.y = pY1;
	gp.p1.z = pZ1;
	gp.p2.x = pX2;
	gp.p2.y = pY2;
	gp.p2.z = pZ2;

	if (sgSpaceMath::PointsDistance(gp.p1,gp.p2)<eps_d)
	{
		global_sg_error = SG_ER_BAD_ARGUMENT_ZERO_LENGTH;
		return NULL;   //  
	}

	global_sg_error = SG_ER_SUCCESS;
	
	sgCLine*   newLn=new sgCLine;
	SetObjectHandle(newLn,create_simple_obj_loc(OLINE, &gp));
	((lpOBJ)(GetObjectHandle(newLn)))->extendedClass = newLn;

	char* typeID = "{0000000000000-0000-0000-000000000002}";
	set_hobj_attr_value(id_TypeID, GetObjectHandle(newLn), typeID);

	newLn->PostCreate();

	return  newLn;
}

const SG_LINE* sgCLine::GetGeometry() const
{
	lpOBJ pobj = (lpOBJ)(GetObjectHandle(this));
	return static_cast<SG_LINE*>((void*)(pobj->geo_data));
}

bool  sgCLine::IsClosed() const {return false;}
bool  sgCLine::IsPlane(SG_VECTOR*,sgFloat*) const {return false;}
bool  sgCLine::IsLinear() const {return true;}
bool  sgCLine::IsSelfIntersecting()  const {return false;}
