#include "stdafx.h"

#include "script/LuaAtomicClasses.h"
#include "script/LuaEnum.h"
#include "util/PartMaterial.h"
#include "v8world/MaterialProperties.h"
#include "G3D/Quat.h"
#include "rbxformat.h"
#include "script/LuaInstanceBridge.h"

#include "lstate.h"
#include "lfunc.h"

FASTFLAGVARIABLE(PhysPropConstructFromMaterial, false)

namespace RBX { namespace Lua {

const char* safe_lua_tostring(lua_State *L, int idx)
{
	static const char* empty = "";
	const char* s = lua_tostring(L, idx);
	return s ? s : empty;
}

const char* throwable_lua_tostring(lua_State *L, int idx)
{
	const char* s = luaL_checkstring(L, idx);
	
	// keep this in sync with string length limits imposed in networking code
	static const size_t kMaxStringLength = 200000;

	if (strlen(s) >= kMaxStringLength) {
		throw std::runtime_error("String too long");
	}

	return s;
}

const char* lua_checkstring_secure(lua_State* L, int idx)
{
    const char* s = lua_tolstringsecure(L, idx, NULL);
    if (!s) luaL_typerror(L, idx, lua_typename(L, LUA_TSTRING));
    return s;
}

void lua_resetstack(lua_State* L, int idx)
{
	RBXASSERT(idx >= 0 && idx <= lua_gettop(L));

    if (idx < lua_gettop(L))
		luaF_close(L, L->base + idx);

    lua_settop(L, idx);
}

float lua_tofloat(lua_State *L, int idx)
{
	double value = lua_tonumber(L, idx);

	if (value==std::numeric_limits<double>::infinity())
		return std::numeric_limits<float>::infinity();

	if (value==-std::numeric_limits<double>::infinity())
		return -std::numeric_limits<float>::infinity();

	// NAN:
	if (!((value < 0.0) || (value >= 0.0)))
		return (float) value;

	if (value > (double)std::numeric_limits<float>::max())
		return std::numeric_limits<float>::max();

	if (value < (double)-std::numeric_limits<float>::max())
		return -std::numeric_limits<float>::max();

	return (float) value;
}


/// G3D::Color3 has a default implementation for on_tostring() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
/// G3D::Color3 has a default implementation for registerClass() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
template<>
const char* Bridge<G3D::Color3>::className("Color3");

const luaL_reg Color3Bridge::classLibrary[] = {
	{"new", newColor3},
	{"fromRGB", newRGBColor3},
	{NULL, NULL}
};
    
void Color3Bridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}

void Color3Bridge::pushColor3(lua_State *L, const G3D::Color3& color)
{
    pushNewObject(L, color);
}
    
int Color3Bridge::newColor3(lua_State *L)
{
	float color[3];

	// There should be up to 3 numerical parameters (r,g,b). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(3,lua_gettop(L));
	for (int i = 0; i<count; i++)
		color[i] = lua_tofloat(L, i+1);
	for (int i = count; i<3; i++)
		color[i] = 0.0;
	pushNewObject(L, color);
	return 1;
}

int Color3Bridge::newRGBColor3(lua_State *L)
{
	float color[3];

	// There should be up to 3 numerical parameters (r,g,b). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(3,lua_gettop(L));
	for (int i = 0; i<count; i++)
		color[i] = lua_tofloat(L, i+1) / 255;
	for (int i = count; i<3; i++)
		color[i] = 0.0;
	pushNewObject(L, color);
	return 1;
}

template<>
int Bridge<G3D::Color3>::on_index(const G3D::Color3& object, const char* name, lua_State *L)
{
	if (strcmp(name,"r")==0)
	{
		lua_pushnumber(L, object.r);
		return 1;
	}
	if (strcmp(name,"g")==0)
	{
		lua_pushnumber(L, object.g);
		return 1;
	}
	if (strcmp(name,"b")==0)
	{
		lua_pushnumber(L, object.b);
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<G3D::Color3>::on_newindex(G3D::Color3& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

/// RBX::RbxRay has a default implementation for on_tostring() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
/// RBX::RbxRay has a default implementation for registerClass() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
template<>
const char* Bridge<RBX::RbxRay>::className("Ray");

const luaL_reg RbxRayBridge::classLibrary[] = {
	{"new", newRbxRay},
	{NULL, NULL}
};

void RbxRayBridge::registerClassLibrary (lua_State *L) {
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}

int RbxRayBridge::newRbxRay(lua_State *L)
{
	Vector3 origin;
	Vector3 direction;

	// There should be up to 2 Vector3 objects. Following Lua conventions ignore others and use 0 for missing
	int count = std::min(2,lua_gettop(L));

	//First extract the Origin
	if (count >= 1)
	{
		origin = Vector3Bridge::getObject(L, 1);
	}
	if (count >= 2)
	{
		direction = Vector3Bridge::getObject(L, 2);
	}
	pushNewObject(L, RBX::RbxRay::fromOriginAndDirection(origin, direction));

	return 1;
}

static int closestPointVector3(lua_State *L)
{
	RBX::RbxRay& self	= Bridge<RBX::RbxRay>::getObject(L, 1);
	G3D::Vector3& point = Bridge<G3D::Vector3>::getObject(L, 2);
	Bridge<G3D::Vector3>::pushNewObject(L, self.closestPoint(point));
	return 1;
}

static int distanceVector3(lua_State *L)
{
	RBX::RbxRay& self	= Bridge<RBX::RbxRay>::getObject(L, 1);
	G3D::Vector3& point = Bridge<G3D::Vector3>::getObject(L, 2);
	lua_pushnumber(L, self.distance(point));
	return 1;
}

//Lower case are legacy
template<>
int Bridge<RBX::RbxRay>::on_index(const RBX::RbxRay& object, const char* name, lua_State *L)
{
	if (strcmp(name,"Origin")==0)
	{
		Bridge<G3D::Vector3>::pushNewObject(L, object.origin());
		return 1;
	}
	if (strcmp(name,"Direction")==0)
	{
    		Bridge<G3D::Vector3>::pushNewObject(L, object.direction());
		return 1;
	}
	if (strcmp(name,"unit")==0 || strcmp(name,"Unit")==0)
	{
		Bridge<RBX::RbxRay>::pushNewObject(L, object.unit());
		return 1;
	}
	if (strcmp(name,"ClosestPoint")==0)
	{
		lua_pushcfunction(L, closestPointVector3);
		return 1;
	}
	if (strcmp(name,"Distance")==0)
	{
		lua_pushcfunction(L, distanceVector3);
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<RBX::RbxRay>::on_newindex(RBX::RbxRay& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}



/// Region3 has a default implementation for on_tostring() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
template<>
const char* Bridge<RBX::Region3>::className("Region3");

const luaL_reg Region3Bridge::classLibrary[] = {
	{"new", newRegion3},
	{NULL, NULL}
};

void Region3Bridge::registerClassLibrary (lua_State *L) {
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int Region3Bridge::newRegion3(lua_State *L)
{
	Vector3 vector[2];
	// There should be up to 2 vector3 parameters (min,max). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(2,lua_gettop(L));
	for (int i = 0; i<count; i++)
		vector[i] = Vector3Bridge::getObject(L, i+1);
	for (int i = count; i<2; i++)
		vector[i] = Vector3();
	pushNewObject(L, Region3(vector[0], vector[1]));
	return 1;
}

int Region3Bridge::expandToGrid(lua_State *L)
{
	Region3 region = Region3Bridge::getObject(L, 1);

	if (2 > lua_gettop(L))
        throw std::runtime_error("Argument 1 missing or nil");

	float resolution = lua_tofloat(L, 2);

	if (Math::isNanInf(resolution) || resolution <= 0)
		throw std::runtime_error("Resolution has to be a positive number");

	Vector3 min = region.minPos() / resolution;
	Vector3 max = region.maxPos() / resolution;

	Vector3 emin(floorf(min.x) * resolution, floorf(min.y) * resolution, floorf(min.z) * resolution);
	Vector3 emax(ceilf(max.x) * resolution, ceilf(max.y) * resolution, ceilf(max.z) * resolution);

	Region3Bridge::pushNewObject(L, Region3(emin, emax));
	return 1;
}

//Lower case are legacy
template<>
int Bridge<RBX::Region3>::on_index(const RBX::Region3& object, const char* name, lua_State *L)
{
	if (strcmp(name,"CFrame")==0)
	{
		CoordinateFrameBridge::pushCoordinateFrame(L, object.getCFrame());
		return 1;
	}
	if (strcmp(name,"Size")==0)
	{
		Vector3Bridge::pushVector3(L, object.getSize());
		return 1;
	}
	if (strcmp(name,"ExpandToGrid")==0)
	{
		lua_pushcfunction(L, Region3Bridge::expandToGrid);
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<RBX::Region3>::on_newindex(RBX::Region3& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

template<>
const char* Bridge<RBX::Region3int16>::className("Region3int16");

const luaL_reg Region3int16Bridge::classLibrary[] = {
	{"new", newRegion3int16},
	{NULL, NULL}
};

void Region3int16Bridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int Region3int16Bridge::newRegion3int16(lua_State *L)
{
	Vector3int16 vector[2];
	// There should be up to 2 vector3int16 parameters (min,max). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(2,lua_gettop(L));
	for (int i = 0; i<count; i++)
		vector[i] = Vector3int16Bridge::getObject(L, i+1);
	for (int i = count; i<2; i++)
		vector[i] = Vector3int16();
	pushNewObject(L, Region3int16(vector[0], vector[1]));
	return 1;
}

//Lower case are legacy
template<>
int Bridge<RBX::Region3int16>::on_index(const RBX::Region3int16& object, const char* name, lua_State *L)
{
	if (strcmp(name,"Min")==0)
	{
		Vector3int16Bridge::pushVector3int16(L, object.getMinPos());
		return 1;
	}
	if (strcmp(name,"Max")==0)
	{
		Vector3int16Bridge::pushVector3int16(L, object.getMaxPos());
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<RBX::Region3int16>::on_newindex(RBX::Region3int16& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}


// PhysicalProperties class implementation
template<>
const char* Bridge<PhysicalProperties>::className("PhysicalProperties");

const luaL_reg PhysicalPropertiesBridge::classLibrary[] =
{
	{"new", newPhysicalProperties},
	{NULL, NULL}
};
void PhysicalPropertiesBridge::registerClassLibrary(lua_State *L)
{
	// Register "new" function
	luaL_register(L, className, classLibrary);
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);	// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int PhysicalPropertiesBridge::newPhysicalProperties(lua_State *L)
{
	PhysicalProperties properties;
	// 1 Enum.Material;
	// 3 numbers;  density, friction, elasticity
	// 5 numbers;  density, friction, elasticity, frictionWeight, elasticityWeight
	const int count = lua_gettop(L);
	if (FFlag::PhysPropConstructFromMaterial && count == 1)
	{
		EnumDescriptorItemPtr item;
		if (EnumItem::getItem(L, 1, item))
		{
			if(!item->owner.isType<RBX::PartMaterial>())
				throw RBX::runtime_error("PhysicalProperties.new with 1 argument expects Enum.Material inputs");

			RBX::PartMaterial partMaterial = (RBX::PartMaterial)item->value;
			properties = MaterialProperties::generatePhysicalMaterialFromPartMaterial(partMaterial);
		}
	}
	else if (count == 3)
	{
		properties = PhysicalProperties(lua_tofloat(L, 1), lua_tofloat(L, 2), lua_tofloat(L, 3));
	}
	else if (count == 5)
	{
		properties = PhysicalProperties(lua_tofloat(L, 1), lua_tofloat(L, 2), lua_tofloat(L, 3), lua_tofloat(L, 4), lua_tofloat(L, 5));
	}
	else
	{
		if (FFlag::PhysPropConstructFromMaterial)
			throw RBX::runtime_error("Invalid number of arguments: %d, PhysicalProperties objects expect 1,3 or 5", count);
		else
			throw RBX::runtime_error("Invalid number of arguments: %d, PhysicalProperties objects expect 3 or 5", count);
	}

	pushNewObject(L, properties);
	return 1;
}


template<>
int Bridge<PhysicalProperties>::on_index(const PhysicalProperties& object, const char* name, lua_State *L)
{
	if (strcmp(name, "Density") == 0)
	{
		lua_pushnumber(L, object.getDensity());
		return 1;
	}
	if (strcmp(name, "Friction") == 0)
	{
		lua_pushnumber(L, object.getFriction());
		return 1;
	}
	if (strcmp(name, "Elasticity") == 0)
	{
		lua_pushnumber(L, object.getElasticity());
		return 1;
	}
	if (strcmp(name, "FrictionWeight") == 0)
	{
		lua_pushnumber(L, object.getFrictionWeight());
		return 1;
	}
	if (strcmp(name, "ElasticityWeight") == 0)
	{
		lua_pushnumber(L, object.getElasticityWeight());
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member of PhysicalProperties", name);
}

template<>
void Bridge<PhysicalProperties>::on_newindex(PhysicalProperties& object, const char* name, lua_State *L)
{
	// Failure CAN OVERRIDE THIS TO MAKE SETTING WORK
	throw RBX::runtime_error("PhysicalProperties.%s cannot be assigned to", name);
}

/// G3D::Rect2D has a default implementation for on_tostring() invoked from LuaBridge.cpp.
template<>
const char* Bridge<G3D::Rect2D>::className("Rect");

const luaL_reg Rect2DBridge::classLibrary[] = {
    {"new", newRect2D},
    {NULL, NULL}
};
void Rect2DBridge::registerClassLibrary (lua_State *L) {

	// Register the "new" function
	luaL_register(L, className, classLibrary);
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
};

int Rect2DBridge::newRect2D(lua_State *L)
{
    Rect2D rect;
   	// 0 args:   emptyRect
	// 2 vectors:   minVec, maxVec
	// 4 numbers:   x0,y0,x1,y1
	const int count = lua_gettop(L);
    if (count==0)
    {
        rect = Rect2D::xyxy(0.0f,0.0f,0.0f,0.0f);
    }
	else if (count == 2)
    {
        Vector2 min = Vector2Bridge::getObject(L, 1);
        Vector2 max = Vector2Bridge::getObject(L, 2);
        rect = Rect2D::xyxy(min,max);
    }
    else if (count == 4)
    {
        rect = Rect2D::xyxy(lua_tofloat(L, 1),lua_tofloat(L, 2),lua_tofloat(L, 3),lua_tofloat(L, 4));
    }
    else
    {
        throw RBX::runtime_error("Invalid number of arguments: %d", count);
	}
    pushNewObject(L, rect);
    return 1;
}
  
template<>
int Bridge<G3D::Rect2D>::on_index(const G3D::Rect2D& object, const char* name, lua_State *L)
{
    if (strcmp(name,"Min")==0)
	{
		Vector2Bridge::pushVector2(L, object.x0y0());
		return 1;
	}
	if (strcmp(name,"Max")==0)
	{
		Vector2Bridge::pushVector2(L, object.x1y1());
		return 1;
	}

	if (strcmp(name,"Width")==0)
	{
		lua_pushnumber(L, object.width());
		return 1;
	}
	if (strcmp(name,"Height")==0)
	{
		lua_pushnumber(L, object.height());
		return 1;
	}
    // Failure
    throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<G3D::Rect2D>::on_newindex(G3D::Rect2D& object, const char* name, lua_State *L)
{
    // Failure
    throw RBX::runtime_error("%s cannot be assigned to", name);
}
    
    
/// G3D::Vector3 has a default implementation for on_tostring() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
template<>
const char* Bridge<G3D::Vector3>::className("Vector3");

const luaL_reg Vector3Bridge::classLibrary[] = {
	{"new", newVector3},
	{"FromNormalId", newVector3FromNormalId},
	{"FromAxis", newVector3FromAxis},
	{NULL, NULL}
};

void Vector3Bridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int Vector3Bridge::on_add(lua_State *L)
{
	const G3D::Vector3& a = Vector3Bridge::getObject(L, 1);
	const G3D::Vector3& b = Vector3Bridge::getObject(L, 2);
	pushVector3(L, a + b);
	return 1;
};

int Vector3Bridge::on_sub(lua_State *L)
{
	const G3D::Vector3& a = Vector3Bridge::getObject(L, 1);
	const G3D::Vector3& b = Vector3Bridge::getObject(L, 2);
	pushVector3(L, a - b);
	return 1;
};

int Vector3Bridge::on_mul(lua_State *L)
{
	G3D::Vector3 a;
	if (Vector3Bridge::getValue(L, 1, a))
	{
		G3D::Vector3 b;
		if (Vector3Bridge::getValue(L, 2, b))
			pushVector3(L, a * b);
        else if(!lua_isnumber(L, 2))
        {
            throw std::runtime_error("attempt to multiply a Vector3 with an incompatible value type or nil");
        }
		else
		{
            float c = lua_tofloat(L, 2);
			pushVector3(L, a * c);
		}
	}
    else if(!lua_isnumber(L, 1))
    {
        throw std::runtime_error("attempt to multiply a Vector3 with an incompatible value type or nil");
    }
	else
    {
		a = Vector3Bridge::getObject(L, 2);
		float b = lua_tofloat(L, 1);
		pushVector3(L, b * a);
	}
	return 1;
};

int Vector3Bridge::on_div(lua_State *L)
{
	G3D::Vector3 a;
	if (Vector3Bridge::getValue(L, 1, a))
	{
		G3D::Vector3 b;
		if (Vector3Bridge::getValue(L, 2, b))
			pushVector3(L, a / b);
        else if(!lua_isnumber(L, 2))
        {
            throw std::runtime_error("attempt to divide a Vector3 with an incompatible value type or nil");
        }
		else
		{
			float c = lua_tofloat(L, 2);
			pushVector3(L, a / c);
		}
	}
    else if(!lua_isnumber(L, 1))
    {
        throw std::runtime_error("attempt to divide a Vector3 with an incompatible value type or nil");
    }
	else {
		a = Vector3Bridge::getObject(L, 2);
		float b = lua_tofloat(L, 1);
		pushVector3(L, G3D::Vector3(b,b,b) / a);
	}
	return 1;
};

int Vector3Bridge::on_unm(lua_State *L)
{
	pushVector3(L, -Vector3Bridge::getObject(L, 1));
	return 1;
};


int Vector3Bridge::newVector3(lua_State *L)
{
	float vector[3];

	// There should be up to 3 numerical parameters (r,g,b). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(3,lua_gettop(L));
	for (int i = 0; i<count; i++)
		vector[i] = lua_tofloat(L, i+1);
	for (int i = count; i<3; i++)
		vector[i] = 0.0;
	pushNewObject(L, vector);
	return 1;
}

int Vector3Bridge::newVector3FromNormalId(lua_State *L)
{
	EnumDescriptorItemPtr item;
	if (EnumItem::getItem(L, 1, item))
	{
		if(!item->owner.isType<RBX::NormalId>()){
			throw RBX::runtime_error("Vector3.FromNormalId expects Enum.NormalId input");
		}
		Bridge<G3D::Vector3>::pushNewObject(L,RBX::normalIdToVector3((RBX::NormalId)item->value));
	}
	else {
		throw RBX::runtime_error("Vector3.FromNormalId expects Enum.NormalId input");
	}
	return 1;
}

int Vector3Bridge::newVector3FromAxis(lua_State *L)
{
	EnumDescriptorItemPtr item;
	if (EnumItem::getItem(L, 1, item))
	{
		if(!item->owner.isType<RBX::Vector3::Axis>()){
			throw RBX::runtime_error("Vector3.FromAxis expects Enum.Axis input");
		}
		Bridge<G3D::Vector3>::pushNewObject(L,
			RBX::normalIdToVector3(
				Axes::axisToNormalId((RBX::Vector3::Axis)item->value)
				)
			);
	}
	else {
		throw RBX::runtime_error("Vector3.FromAxis expects Enum.Axis input");
	}
	return 1;
}

static int lerpVector3(lua_State *L)
{
	G3D::Vector3& self = Bridge<G3D::Vector3>::getObject(L, 1);
	G3D::Vector3& v = Bridge<G3D::Vector3>::getObject(L, 2);
	float alpha = lua_tofloat(L, 3);
	Bridge<G3D::Vector3>::pushNewObject(L, self.lerp(v, alpha));
	return 1;
}
static int crossVector3(lua_State *L)
{
	G3D::Vector3& self = Bridge<G3D::Vector3>::getObject(L, 1);
	G3D::Vector3& v = Bridge<G3D::Vector3>::getObject(L, 2);
	Bridge<G3D::Vector3>::pushNewObject(L, self.cross(v));
	return 1;
}

static int dotVector3(lua_State *L)
{
	G3D::Vector3& self = Bridge<G3D::Vector3>::getObject(L, 1);
	G3D::Vector3& v = Bridge<G3D::Vector3>::getObject(L, 2);
	lua_pushnumber(L, self.dot(v));
	return 1;
}

static int isCloseVector3(lua_State *L)
{
	int count = lua_gettop(L);
	G3D::Vector3& self = Bridge<G3D::Vector3>::getObject(L, 1);
	G3D::Vector3& v = Bridge<G3D::Vector3>::getObject(L, 2);

	bool result;
	if (count > 2) {
		result = Math::fuzzyEq(self, v, fabsf(lua_tofloat(L, 3)));
	} else {
		result = Math::fuzzyEq(self, v);
	}

	lua_pushboolean(L, result);
	return 1;
}

//Lower case are legacy
template<>
int Bridge<G3D::Vector3>::on_index(const G3D::Vector3& object, const char* name, lua_State *L)
{
	if (strcmp(name,"x")==0 || strcmp(name,"X")==0)
	{
		lua_pushnumber(L, object.x);
		return 1;
	}
	if (strcmp(name,"y")==0 || strcmp(name,"Y")==0)
	{
		lua_pushnumber(L, object.y);
		return 1;
	}
	if (strcmp(name,"z")==0 || strcmp(name,"Z")==0)
	{
		lua_pushnumber(L, object.z);
		return 1;
	}
	if (strcmp(name,"unit")==0 || strcmp(name,"Unit")==0)
	{
		Bridge<G3D::Vector3>::pushNewObject(L, object.unit());
		return 1;
	}
	if (strcmp(name,"magnitude")==0 || strcmp(name,"Magnitude") == 0)
	{
		lua_pushnumber(L, object.magnitude());
		return 1;
	}
	if (strcmp(name,"lerp")==0 || strcmp(name,"Lerp")==0)
	{
		lua_pushcfunction(L, lerpVector3);
		return 1;
	}
	if (strcmp(name,"Cross")==0)
	{
		lua_pushcfunction(L, crossVector3);
		return 1;
	}
	if (strcmp(name,"Dot")==0)
	{
		lua_pushcfunction(L, dotVector3);
		return 1;
	}
	if (strcmp(name,"isClose") == 0)
	{
		lua_pushcfunction(L, isCloseVector3);
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<G3D::Vector3>::on_newindex(G3D::Vector3& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

template<>
const char* Bridge<RBX::Vector3int16>::className("Vector3int16");

const luaL_reg Vector3int16Bridge::classLibrary[] = {
	{"new", newVector3int16},
	{NULL, NULL}
};

void Vector3int16Bridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int Vector3int16Bridge::on_add(lua_State *L)
{
	const RBX::Vector3int16& a = Vector3int16Bridge::getObject(L, 1);
	const RBX::Vector3int16& b = Vector3int16Bridge::getObject(L, 2);
	pushVector3int16(L, a + b);
	return 1;
};

int Vector3int16Bridge::on_sub(lua_State *L)
{
	const RBX::Vector3int16& a = Vector3int16Bridge::getObject(L, 1);
	const RBX::Vector3int16& b = Vector3int16Bridge::getObject(L, 2);
	pushVector3int16(L, a - b);
	return 1;
};

int Vector3int16Bridge::on_mul(lua_State *L)
{
	RBX::Vector3int16 a;
	if (Vector3int16Bridge::getValue(L, 1, a))
	{
		RBX::Vector3int16 b;
		if (Vector3int16Bridge::getValue(L, 2, b))
			pushVector3int16(L, a * b);
		else
		{
			float c = lua_tofloat(L, 2);
			pushVector3int16(L, a * c);
		}
	}
	else {
		a = Vector3int16Bridge::getObject(L, 2);
		float b = lua_tofloat(L, 1);
		pushVector3int16(L, a * b);
	}
	return 1;
};

int Vector3int16Bridge::on_div(lua_State *L)
{
	RBX::Vector3int16 a;
	if (Vector3int16Bridge::getValue(L, 1, a))
	{
		RBX::Vector3int16 b;
		if (Vector3int16Bridge::getValue(L, 2, b))
		{
			if (b.x == 0 || b.y == 0 || b.z == 0)
			{
				// Failure
				throw RBX::runtime_error("Divide by zero exception");
			}
				
			Vector3int16 c( a.x / b.x, a.y / b.y, a.z / b.z );
			pushVector3int16(L, c);
		}
		else
		{
			float c = lua_tofloat(L, 2);
			Vector3int16 d( a.x / c, a.y / c, a.z / c );
			pushVector3int16(L, d);
		}
	}
	else 
	{
		a = Vector3int16Bridge::getObject(L, 2);
		int b = lua_tointeger(L, 1);
		if (a.x == 0 || a. y == 0 || a.z == 0)
		{
			// Failure
			throw RBX::runtime_error("Divide by zero exception");
		}

		Vector3int16 c( b / a.x, b / a.y, b / a.z );
		pushVector3int16(L, c);
	}
	return 1;
};

int Vector3int16Bridge::on_unm(lua_State *L)
{
	Vector3int16 a = Vector3int16Bridge::getObject(L, 1);	
	Vector3int16 b( -a.x, -a.y, -a.z );
	pushVector3int16(L, b);
	return 1;
};


int Vector3int16Bridge::newVector3int16(lua_State *L)
{
	int vector[3];

	// There should be up to 3 numerical parameters (r,g,b). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(3,lua_gettop(L));
	for (int i = 0; i<count; i++)
		vector[i] = lua_tointeger(L, i+1);
	for (int i = count; i<3; i++)
		vector[i] = 0;
	pushNewObject(L, vector);
	return 1;
}


template<>
int Bridge<RBX::Vector3int16>::on_index(const RBX::Vector3int16& object, const char* name, lua_State *L)
{
	if (strcmp(name,"x")==0 || strcmp(name,"X")==0)
	{
		lua_pushinteger(L, object.x);
		return 1;
	}
	if (strcmp(name,"y")==0 || strcmp(name,"Y")==0)
	{
		lua_pushinteger(L, object.y);
		return 1;
	}
	if (strcmp(name,"z")==0 || strcmp(name,"Z")==0)
	{
		lua_pushinteger(L, object.z);
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}


template<>
void Bridge<RBX::Vector3int16>::on_newindex(RBX::Vector3int16& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

template<>
const char* Bridge<RBX::Vector2int16>::className("Vector2int16");

const luaL_reg Vector2int16Bridge::classLibrary[] = {
	{"new", newVector2int16},
	{NULL, NULL}
};

void Vector2int16Bridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int Vector2int16Bridge::on_add(lua_State *L)
{
	const RBX::Vector2int16& a = Vector2int16Bridge::getObject(L, 1);
	const RBX::Vector2int16& b = Vector2int16Bridge::getObject(L, 2);
	pushVector2int16(L, a + b);
	return 1;
};

int Vector2int16Bridge::on_sub(lua_State *L)
{
	const RBX::Vector2int16& a = Vector2int16Bridge::getObject(L, 1);
	const RBX::Vector2int16& b = Vector2int16Bridge::getObject(L, 2);
	pushVector2int16(L, a - b);
	return 1;
};

int Vector2int16Bridge::on_mul(lua_State *L)
{
	RBX::Vector2int16 a;
	if (Vector2int16Bridge::getValue(L, 1, a))
	{
		RBX::Vector2int16 b;
		if (Vector2int16Bridge::getValue(L, 2, b))
			pushVector2int16(L, a * b);
		else
		{
			float c = lua_tofloat(L, 2);
			pushVector2int16(L, a * c);
		}
	}
	else {
		a = Vector2int16Bridge::getObject(L, 2);
		float b = lua_tofloat(L, 1);
		pushVector2int16(L, b * a);
	}
	return 1;
};

int Vector2int16Bridge::on_div(lua_State *L)
{
	RBX::Vector2int16 a;
	if (Vector2int16Bridge::getValue(L, 1, a))
	{
		RBX::Vector2int16 b;
		if (Vector2int16Bridge::getValue(L, 2, b))
		{
			if (b.x == 0 || b.y == 0)
			{
				// Failure
				throw RBX::runtime_error("Divide by zero exception");
			}

			pushVector2int16(L, a / b);
		}
		else
		{
			float c = lua_tofloat(L, 2);
			pushVector2int16(L, a / c);
		}
	}
	else 
	{
		a = Vector2int16Bridge::getObject(L, 2);
		int b = lua_tointeger(L, 1);

		if (a.x == 0 || a.y == 0)
		{
			// Failure
			throw RBX::runtime_error("Divide by zero exception");
		}
		
		pushVector2int16(L, RBX::Vector2int16(b,b) / a);
	}
	return 1;
};

int Vector2int16Bridge::on_unm(lua_State *L)
{
	pushVector2int16(L, -Vector2int16Bridge::getObject(L, 1));
	return 1;
};


int Vector2int16Bridge::newVector2int16(lua_State *L)
{
	int vector[2];

	// There should be up to 2 numerical parameters (x, y). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(2,lua_gettop(L));
	for (int i = 0; i<count; i++)
		vector[i] = lua_tointeger(L, i+1);
	for (int i = count; i<2; i++)
		vector[i] = 0;
	pushNewObject(L, vector);
	return 1;
}


template<>
int Bridge<RBX::Vector2int16>::on_index(const RBX::Vector2int16& object, const char* name, lua_State *L)
{
	if (strcmp(name,"x")==0 || strcmp(name,"X")==0)
	{
		lua_pushinteger(L, object.x);
		return 1;
	}
	if (strcmp(name,"y")==0 || strcmp(name,"Y")==0)
	{
		lua_pushinteger(L, object.y);
		return 1;
	}
	//if (strcmp(name,"unit")==0)
	//{
	//	Bridge<RBX::Vector2int16>::pushNewObject(L, object.direction());
	//	return 1;
	//}
	//if (strcmp(name,"magnitude")==0)
	//{
	//	lua_pushnumber(L, object.length());
	//	return 1;
	//}
	//if (strcmp(name,"lerp")==0)
	//{
	//	lua_pushcfunction(L, lerpVector2);
	//	return 1;
	//}

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}


template<>
void Bridge<RBX::Vector2int16>::on_newindex(RBX::Vector2int16& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

/// RBX::Vector2 has a default implementation for on_tostring() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
template<>
const char* Bridge<RBX::Vector2>::className("Vector2");

const luaL_reg Vector2Bridge::classLibrary[] = {
	{"new", newVector2},
	{NULL, NULL}
};

void Vector2Bridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary);
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int Vector2Bridge::on_add(lua_State *L)
{
	const RBX::Vector2& a = Vector2Bridge::getObject(L, 1);
	const RBX::Vector2& b = Vector2Bridge::getObject(L, 2);
	pushVector2(L, a + b);
	return 1;
};

int Vector2Bridge::on_sub(lua_State *L)
{
	const RBX::Vector2& a = Vector2Bridge::getObject(L, 1);
	const RBX::Vector2& b = Vector2Bridge::getObject(L, 2);
	pushVector2(L, a - b);
	return 1;
};

int Vector2Bridge::on_mul(lua_State *L)
{
	RBX::Vector2 a;
	if (Vector2Bridge::getValue(L, 1, a))
	{
		RBX::Vector2 b;
		if (Vector2Bridge::getValue(L, 2, b))
			pushVector2(L, a * b);
        else if(!lua_isnumber(L, 2))
        {
            throw std::runtime_error("attempt to multiply a Vector2 with an incompatible value type or nil");
        }
		else
		{
			float c = lua_tofloat(L, 2);
			pushVector2(L, a * c);
		}
	}
    else if(!lua_isnumber(L, 1))
    {
        throw std::runtime_error("attempt to multiply a Vector2 with an incompatible value type or nil");
    }
	else {
		a = Vector2Bridge::getObject(L, 2);
		float b = lua_tofloat(L, 1);
		pushVector2(L, b * a);
	}
	return 1;
};

int Vector2Bridge::on_div(lua_State *L)
{
	RBX::Vector2 a;
	if (Vector2Bridge::getValue(L, 1, a))
	{
		RBX::Vector2 b;
		if (Vector2Bridge::getValue(L, 2, b))
			pushVector2(L, a / b);
        else if(!lua_isnumber(L, 2))
        {
            throw std::runtime_error("attempt to divide a Vector2 with an incompatible value type or nil");
        }
		else
		{
			float c = lua_tofloat(L, 2);
			pushVector2(L, a / c);
		}
	}
    else if(!lua_isnumber(L, 1))
    {
        throw std::runtime_error("attempt to divide a Vector2 with an incompatible value type or nil");
    }
	else 
	{
		a = Vector2Bridge::getObject(L, 2);
		float b = lua_tofloat(L, 1);
		pushVector2(L, RBX::Vector2(b,b) / a);
	}
	return 1;
};

int Vector2Bridge::on_unm(lua_State *L)
{
	pushVector2(L, -Vector2Bridge::getObject(L, 1));
	return 1;
};


int Vector2Bridge::newVector2(lua_State *L)
{
	float vector[2];

	// There should be up to 3 numerical parameters (r,g,b). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(2,lua_gettop(L));
	for (int i = 0; i<count; i++)
		vector[i] = lua_tofloat(L, i+1);
	for (int i = count; i<2; i++)
		vector[i] = 0.0;
	pushNewObject(L, vector);
	return 1;
}

static int lerpVector2(lua_State *L)
{
	RBX::Vector2& self = Bridge<RBX::Vector2>::getObject(L, 1);
	RBX::Vector2& v = Bridge<RBX::Vector2>::getObject(L, 2);
	float alpha = lua_tofloat(L, 3);
	Bridge<RBX::Vector2>::pushNewObject(L, self.lerp(v, alpha));
	return 1;
}

template<>
int Bridge<RBX::Vector2>::on_index(const RBX::Vector2& object, const char* name, lua_State *L)
{
	if (strcmp(name,"x")==0 || strcmp(name,"X")==0)
	{
		lua_pushnumber(L, object.x);
		return 1;
	}
	if (strcmp(name,"y")==0 || strcmp(name,"Y")==0)
	{
		lua_pushnumber(L, object.y);
		return 1;
	}
	if (strcmp(name,"unit")==0)
	{
		Bridge<RBX::Vector2>::pushNewObject(L, object.direction());
		return 1;
	}
	if (strcmp(name,"magnitude")==0)
	{
		lua_pushnumber(L, object.length());
		return 1;
	}
	if (strcmp(name,"lerp")==0)
	{
		lua_pushcfunction(L, lerpVector2);
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<RBX::Vector2>::on_newindex(RBX::Vector2& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

/// RBX::BrickColor has a default implementation for on_tostring() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
/// RBX::BrickColor has a default implementation for registerClass() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
template<>
const char* Bridge<RBX::BrickColor>::className("BrickColor");

void BrickColorBridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
static int pushRed(lua_State *L)
{
	BrickColorBridge::pushNewObject(L, BrickColor::brickRed());
	return 1;
}

static int pushWhite(lua_State *L)
{
	BrickColorBridge::pushNewObject(L, BrickColor::brickWhite());
	return 1;
}

static int pushGray(lua_State *L)
{
	BrickColorBridge::pushNewObject(L, BrickColor::brickGray());
	return 1;
}

static int pushDarkGray(lua_State *L)
{
	BrickColorBridge::pushNewObject(L, BrickColor::brickDarkGray());
	return 1;
}

static int pushBlack(lua_State *L)
{
	BrickColorBridge::pushNewObject(L, BrickColor::brickBlack());
	return 1;
}

static int pushYellow(lua_State *L)
{
	BrickColorBridge::pushNewObject(L, BrickColor::brickYellow());
	return 1;
}
static int pushGreen(lua_State *L)
{
	BrickColorBridge::pushNewObject(L, BrickColor::brickGreen());
	return 1;
}
static int pushBlue(lua_State *L)
{
	BrickColorBridge::pushNewObject(L, BrickColor::brickBlue());
	return 1;
}

const luaL_reg BrickColorBridge::classLibrary[] = {
	{"new", newBrickColor},
	{"random", randomBrickColor},
	{"palette", paletteBrickColor},
	{"New", newBrickColor},
	{"Random", randomBrickColor},
	{"White", pushWhite },
	{"Gray", pushGray },
	{"DarkGray", pushDarkGray },
	{"Black", pushBlack },
	{"Red", pushRed },
	{"Yellow", pushYellow },
	{"Green", pushGreen },
	{"Blue", pushBlue },
	{NULL, NULL}
};

int BrickColorBridge::newBrickColor(lua_State *L)
{
	// There should be up to 3 numerical parameters (r,g,b). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(3,lua_gettop(L));

	if (count==0)
	{
		pushNewObject(L, BrickColor::defaultColor());
	}
	if (count==1)
	{
		// TODO: Handle Color3 as option
		if (lua_isnumber(L, 1))
			pushNewObject(L, BrickColor(lua_tointeger(L, 1)));
		else if (lua_isstring(L, 1))
			pushNewObject(L, BrickColor::parse(lua_tostring(L, 1)));
		else 
			pushNewObject(L, BrickColor::closest(Color3Bridge::getObject(L, 1)));
	}
	else
	{
		G3D::Color4 color(0,0,0);
		for (int i = 0; i<count; i++)
			color[i] = lua_tofloat(L, i+1);
		pushNewObject(L, BrickColor::closest(color));
	}
	return 1;
}
int BrickColorBridge::paletteBrickColor(lua_State *L)
{
	// There should be 1 numerical parameter (index). Following Lua conventions ignore others and use 0 if it is missing
	int count = std::min(1,lua_gettop(L));
	int index = 0;
	if (count==1)
	{
		index = lua_tointeger(L, 1);
	}
	if(index < 0 || index >= int(BrickColor::colorPalette().size())){
		throw RBX::runtime_error("palette index out of bounds (%d)", index);
	}
	pushNewObject(L, BrickColor::colorPalette().at(index));
	return 1;
}
int BrickColorBridge::randomBrickColor(lua_State *L)
{
	pushNewObject(L, BrickColor::random());
	return 1;
}

template<>
int Bridge<RBX::BrickColor>::on_index(const RBX::BrickColor& object, const char* name, lua_State *L)
{
	if (strcmp(name,"number")==0)
	{
		lua_pushinteger(L, object.number);
		return 1;
	}
	if (strcmp(name,"Number")==0)
	{
		lua_pushinteger(L, object.number);
		return 1;
	}
	if (strcmp(name,"Color")==0)
	{
		Color3Bridge::pushNewObject(L, object.color3());
		return 1;
	}
	if (strcmp(name,"r")==0)
	{
		lua_pushnumber(L, object.color3().r);
		return 1;
	}
	if (strcmp(name,"g")==0)
	{
		lua_pushnumber(L, object.color3().g);
		return 1;
	}
	if (strcmp(name,"b")==0)
	{
		lua_pushnumber(L, object.color3().b);
		return 1;
	}
	if (strcmp(name,"name")==0)
	{
		lua_pushstring(L, object.name());
		return 1;
	}
	if (strcmp(name,"Name")==0)
	{
		lua_pushstring(L, object.name());
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<RBX::BrickColor>::on_newindex(RBX::BrickColor& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

/// G3D::CoordinateFrame has a default implementation for on_tostring() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
template<>
const char* Bridge<G3D::CoordinateFrame>::className("CFrame");

const luaL_reg CoordinateFrameBridge::classLibrary[] = {
	{"new", newCoordinateFrame},
	{"fromEulerAnglesXYZ", fromEulerAnglesXYZ},
	{"Angles", fromEulerAnglesXYZ},	//Synonym, much shorter for 
	{"fromAxisAngle", fromAxisAngle},
	{NULL, NULL}
};

void CoordinateFrameBridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int CoordinateFrameBridge::newCoordinateFrame(lua_State *L)
{
	G3D::CoordinateFrame cf;

	// 0 args:   identity
	// 1 vector:   translation
	// 2 vectors:   translation, lookAt
	// 3 numbers:   translation
	// 7 numbers:   translation, quat
	// 12 numbers:  translation, rotation matrix
	const int count = lua_gettop(L);
	switch (count)
	{
	case 0:
		// Identity
		break;

	case 1:
		cf.translation = Vector3Bridge::getObject(L, 1);
		break;

	case 2:
		cf.translation = Vector3Bridge::getObject(L, 1);
		cf.lookAt(Vector3Bridge::getObject(L, 2));
		break;

	case 3:
		for (int i = 0; i<3; i++)
			cf.translation[i] = lua_tofloat(L, i+1);
		break;

	case 7:
		{
			for (int i = 0; i<3; i++)
				cf.translation[i] = lua_tofloat(L, i+1);
			G3D::Quat q;
			q.x = lua_tofloat(L, 3+1);
			q.y = lua_tofloat(L, 4+1);
			q.z = lua_tofloat(L, 5+1);
			q.w = lua_tofloat(L, 6+1);
			cf.rotation = q;
		}
		break;

	case 12:
		for (int i = 0; i<3; i++)
			cf.translation[i] = lua_tofloat(L, i+1);
		for (int i = 0; i<3; ++i)
			for (int j = 0; j<3; ++j)
				cf.rotation[i][j] = lua_tofloat(L, 3 + 3*i + j +1);
		break;

	default:
		throw RBX::runtime_error("Invalid number of arguments: %d", count);
		break;

	}

	pushNewObject(L, cf);
	return 1;
}


int CoordinateFrameBridge::on_add(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	const G3D::Vector3& b = Vector3Bridge::getObject(L, 2);
	pushCoordinateFrame(L, a + b);
	return 1;
};

int CoordinateFrameBridge::on_sub(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	const G3D::Vector3& b = Vector3Bridge::getObject(L, 2);
	pushCoordinateFrame(L, a - b);
	return 1;
};

int CoordinateFrameBridge::on_mul(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);

	// Try Ma * Mb
	G3D::CoordinateFrame b;
	if (CoordinateFrameBridge::getValue(L, 2, b))
	{
		pushCoordinateFrame(L, a * b);
		return 1;
	}

	// Try Ma * Vb
	Vector3Bridge::pushVector3(L, a.toWorldSpace(G3D::Vector4(Vector3Bridge::getObject(L, 2),1)).xyz());

	return 1;
};

int CoordinateFrameBridge::on_inverse(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	pushCoordinateFrame(L, a.inverse());
	return 1;
};

int CoordinateFrameBridge::on_lerp(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	const G3D::CoordinateFrame& other = CoordinateFrameBridge::getObject(L, 2);
	float alpha = lua_tofloat(L, 3);
	pushCoordinateFrame(L, a.lerp(other, alpha));
	return 1;
};

int CoordinateFrameBridge::on_toWorldSpace(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	int count = lua_gettop(L)-1;
	if (count==0)
	{
		pushCoordinateFrame(L, a);
		return 1;
	}
	for (int i=0; i<count; ++i)
	{
		const G3D::CoordinateFrame& b = CoordinateFrameBridge::getObject(L, i+2);
		pushCoordinateFrame(L, a * b);
	}
	return count;
};

int CoordinateFrameBridge::on_toObjectSpace(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	int count = lua_gettop(L)-1;
	if (count==0)
	{
		pushCoordinateFrame(L, a.inverse());
		return 1;
	}
	for (int i=0; i<count; ++i)
	{
		const G3D::CoordinateFrame& b = CoordinateFrameBridge::getObject(L, i+2);
		pushCoordinateFrame(L, a.toObjectSpace(b));
	}
	return count;
};

int CoordinateFrameBridge::on_pointToWorldSpace(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	int count = lua_gettop(L)-1;
	if (count==0)
	{
		Vector3Bridge::pushVector3(L, a.pointToWorldSpace(G3D::Vector3::zero()));
		return 1;
	}
	for (int i=0; i<count; ++i)
	{
		const G3D::Vector3& b = Vector3Bridge::getObject(L, i+2);
		Vector3Bridge::pushVector3(L, a.pointToWorldSpace(b));
	}
	return count;
}
int CoordinateFrameBridge::on_pointToObjectSpace(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	int count = lua_gettop(L)-1;
	if (count==0)
	{
		Vector3Bridge::pushVector3(L, a.pointToObjectSpace(G3D::Vector3::zero()));
		return 1;
	}
	for (int i=0; i<count; ++i)
	{
		const G3D::Vector3& b = Vector3Bridge::getObject(L, i+2);
		Vector3Bridge::pushVector3(L, a.pointToObjectSpace(b));
	}
	return count;
}
int CoordinateFrameBridge::on_vectorToWorldSpace(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	int count = lua_gettop(L)-1;
	if (count==0)
	{
		Vector3Bridge::pushVector3(L, a.vectorToWorldSpace(G3D::Vector3::zero()));
		return 1;
	}
	for (int i=0; i<count; ++i)
	{
		const G3D::Vector3& b = Vector3Bridge::getObject(L, i+2);
		Vector3Bridge::pushVector3(L, a.vectorToWorldSpace(b));
	}
	return count;
}

int CoordinateFrameBridge::on_components(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	lua_pushnumber(L, a.translation.x);
	lua_pushnumber(L, a.translation.y);
	lua_pushnumber(L, a.translation.z);
	for (int i=0; i<3; ++i)
		for (int j=0; j<3; ++j)
			lua_pushnumber(L, a.rotation[i][j]);
	return 12;
}

int CoordinateFrameBridge::on_toEulerAnglesXYZ(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	float x,y,z;
	a.rotation.toEulerAnglesXYZ(x,y,z);
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	lua_pushnumber(L, z);
	return 3;
}

int CoordinateFrameBridge::on_vectorToObjectSpace(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	int count = lua_gettop(L)-1;
	if (count==0)
	{
		Vector3Bridge::pushVector3(L, a.vectorToObjectSpace(G3D::Vector3::zero()));
		return 1;
	}
	for (int i=0; i<count; ++i)
	{
		const G3D::Vector3& b = Vector3Bridge::getObject(L, i+2);
		Vector3Bridge::pushVector3(L, a.vectorToObjectSpace(b));
	}
	return count;
}



int CoordinateFrameBridge::fromEulerAnglesXYZ(lua_State *L)
{
	G3D::CoordinateFrame cf;
	cf.rotation = G3D::Matrix3::fromEulerAnglesXYZ((float) luaL_checknumber(L, 1), (float) luaL_checknumber(L, 2), (float) luaL_checknumber(L, 3));
	CoordinateFrameBridge::pushNewObject(L, cf);
	return 1;
};

int CoordinateFrameBridge::fromAxisAngle(lua_State *L)
{
	G3D::CoordinateFrame cf;
	cf.rotation = G3D::Matrix3::fromAxisAngle(Vector3Bridge::getObject(L, 1), (float) luaL_checknumber(L, 2));
	CoordinateFrameBridge::pushNewObject(L, cf);
	return 1;
};

template<>
int Bridge<G3D::CoordinateFrame>::on_index(const G3D::CoordinateFrame& object, const char* name, lua_State *L)
{
	if (strcmp(name,"p")==0)
	{
		Vector3Bridge::pushVector3(L, object.translation);
		return 1;
	}
	if (strcmp(name,"lookVector")==0)	// lookVector is a property
	{
		Vector3Bridge::pushVector3(L, object.lookVector());
		return 1;
	}

	if (strcmp(name,"inverse")==0)		// inverse() is a function
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_inverse, 1);
		return 1;
	}

	if (strcmp(name,"lerp")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_lerp, 1);
		return 1;
	} 

	if (strcmp(name,"toWorldSpace")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_toWorldSpace, 1);
		return 1;
	}
	if (strcmp(name,"toObjectSpace")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_toObjectSpace, 1);
		return 1;
	}
	if (strcmp(name,"pointToWorldSpace")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_pointToWorldSpace, 1);
		return 1;
	}
	if (strcmp(name,"pointToObjectSpace")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_pointToObjectSpace, 1);
		return 1;
	}
	if (strcmp(name,"vectorToWorldSpace")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_vectorToWorldSpace, 1);
		return 1;
	}
	if (strcmp(name,"vectorToObjectSpace")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_vectorToObjectSpace, 1);
		return 1;
	}
	if (strcmp(name,"toEulerAnglesXYZ")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_toEulerAnglesXYZ, 1);
		return 1;
	}
	if (strcmp(name,"components")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_components, 1);
		return 1;
	}

	if (strcmp(name,"x")==0 || strcmp(name,"X")==0)
	{
		lua_pushnumber(L, object.translation.x);
		return 1;
	}
	if (strcmp(name,"y")==0 || strcmp(name,"Y")==0)
	{
		lua_pushnumber(L, object.translation.y);
		return 1;
	}
	if (strcmp(name,"z")==0 || strcmp(name,"Z")==0)
	{
		lua_pushnumber(L, object.translation.z);
		return 1;
	}


	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<G3D::CoordinateFrame>::on_newindex(G3D::CoordinateFrame& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}


template<>
const char* Bridge<RBX::UDim>::className("UDim");


const luaL_reg UDimBridge::classLibrary[] = {
	{"new", newUDim},
	{NULL, NULL}
};
    
void UDimBridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int UDimBridge::newUDim(lua_State *L)
{
	// There should be up to 2 numerical parameters (scale,offset). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(3,lua_gettop(L));

	if (count==0)
	{
		pushNewObject(L, UDim(0.0f, 0));
	}
	if (count==1)
	{
		pushNewObject(L, UDim(lua_tofloat(L, 1), 0));
	}
	else
	{
		pushNewObject(L, UDim(lua_tofloat(L, 1), lua_tointeger(L, 2)));
	}
	return 1;
}

int UDimBridge::on_add(lua_State *L)
{
	const UDim& a = UDimBridge::getObject(L, 1);
	const UDim& b = UDimBridge::getObject(L, 2);
	pushNewObject(L, a + b);
	return 1;
};

int UDimBridge::on_sub(lua_State *L)
{

	const UDim& a = UDimBridge::getObject(L, 1);
	const UDim& b = UDimBridge::getObject(L, 2);
	pushNewObject(L, a - b);
	return 1;
};

int UDimBridge::on_unm(lua_State *L)
{
	pushNewObject(L, -UDimBridge::getObject(L, 1));
	return 1;
};

template<>
int Bridge<RBX::UDim>::on_index(const RBX::UDim& object, const char* name, lua_State *L)
{
	if (strcmp(name,"Scale")==0)
	{
		lua_pushnumber(L, object.scale);
		return 1;
	}
	if (strcmp(name,"Offset")==0)
	{
		lua_pushinteger(L, object.offset);
		return 1;
	}

	if(name && !isupper(name[0]))
		throw RBX::runtime_error("%s is not a valid member, did you forget to capitalize the first letter?", name);

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<RBX::UDim>::on_newindex(RBX::UDim& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

template<>
const char* Bridge<RBX::UDim2>::className("UDim2");


const luaL_reg UDim2Bridge::classLibrary[] = {
	{"new", newUDim2},
	{NULL, NULL}
};
    
void UDim2Bridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int UDim2Bridge::newUDim2(lua_State *L)
{
	// There should be up to 4 numerical parameters (scalex,offsetx, scaley,offset) . Following Lua conventions ignore others and use 0 for missing
	int count = std::min(5,lua_gettop(L));

	float scaleX = 0.0f, scaleY = 0.0f;
	int offsetX = 0, offsetY = 0;
	switch(count){
		case 5:
		case 4:
			offsetY = lua_tointeger(L, 4);
		case 3:
			scaleY = lua_tofloat(L, 3);
		case 2:
			offsetX = lua_tointeger(L, 2);
		case 1:
			scaleX = lua_tofloat(L, 1);
		case 0:
			pushNewObject(L, UDim2(scaleX, offsetX, scaleY, offsetY));
			break;
	}
	return 1;
}

int UDim2Bridge::on_add(lua_State *L)
{
	const UDim2& a = UDim2Bridge::getObject(L, 1);
	const UDim2& b = UDim2Bridge::getObject(L, 2);
	pushNewObject(L, a + b);
	return 1;
};

int UDim2Bridge::on_sub(lua_State *L)
{
	const UDim2& a = UDim2Bridge::getObject(L, 1);
	const UDim2& b = UDim2Bridge::getObject(L, 2);
	pushNewObject(L, a - b);
	return 1;
};

int UDim2Bridge::on_unm(lua_State *L)
{
	pushNewObject(L, -UDim2Bridge::getObject(L, 1));
	return 1;
};

template<>
int Bridge<RBX::UDim2>::on_index(const RBX::UDim2& object, const char* name, lua_State *L)
{
	if (strcmp(name,"X")==0 || strcmp(name,"Width")==0)
	{
		UDimBridge::pushUDim(L, object.x);
		return 1;
	}
	if (strcmp(name,"Y")==0 || strcmp(name,"Height")==0)
	{
		UDimBridge::pushUDim(L, object.y);
		return 1;
	}

	if(name && !isupper(name[0]))
		throw RBX::runtime_error("%s is not a valid member, did you forget to capitalize the first letter?", name);

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<RBX::UDim2>::on_newindex(RBX::UDim2& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

/// RBX::Faces has a default implementation for on_tostring() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
/// RBX::Faces has a default implementation for registerClass() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
template<>
const char* Bridge<RBX::Faces>::className("Faces");


const luaL_reg FacesBridge::classLibrary[] = {
	{"new", newFaces},
	{NULL, NULL}
};

void FacesBridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int FacesBridge::newFaces(lua_State *L)
{
	// There should be up to 6 Enum parameters () . Following Lua conventions ignore others and use 0 for missing
	int count = std::min(6,lua_gettop(L));

	int normalIdMask = RBX::NORM_NONE_MASK;
	for(int index = 1; index<= count; index++){
		//Select each enum, 
		EnumDescriptorItemPtr item;
		if (EnumItem::getItem(L, index, item))
		{
			if(!item->owner.isType<RBX::NormalId>())
				throw RBX::runtime_error("Faces.new expects Enum.NormalId inputs");

			normalIdMask |= normalIdToMask((RBX::NormalId)item->value);
		}
	}

	pushNewObject(L, Faces(normalIdMask));

	return 1;
}

template<>
int Bridge<RBX::Faces>::on_index(const RBX::Faces& object, const char* name, lua_State *L)
{
	if (strcmp(name,"Top")==0)
	{
		lua_pushboolean(L, object.getNormalId(RBX::NORM_Y));
		return 1;
	}
	if (strcmp(name,"Bottom")==0)
	{
		lua_pushboolean(L, object.getNormalId(RBX::NORM_Y_NEG));
		return 1;
	}
	if (strcmp(name,"Back")==0)
	{
		lua_pushboolean(L, object.getNormalId(RBX::NORM_Z));
		return 1;
	}
	if (strcmp(name,"Front")==0)
	{
		lua_pushboolean(L, object.getNormalId(RBX::NORM_Z_NEG));
		return 1;
	}
	if (strcmp(name,"Right")==0)
	{
		lua_pushboolean(L, object.getNormalId(RBX::NORM_X));
		return 1;
	}
	if (strcmp(name,"Left")==0)
	{
		lua_pushboolean(L, object.getNormalId(RBX::NORM_X_NEG));
		return 1;
	}

	if(name && !isupper(name[0]))
		throw RBX::runtime_error("%s is not a valid member, did you forget to capitalize the first letter?", name);
	// Failure
	throw RBX::runtime_error("%s is not a valid member. Valid members are Top,Bottom,Left,Right,Back,Front", name);
}

template<>
void Bridge<RBX::Faces>::on_newindex(RBX::Faces& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

/// RBX::Axes has a default implementation for on_tostring() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
/// RBX::Axes has a default implementation for registerClass() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
template<>
const char* Bridge<RBX::Axes>::className("Axes");

const luaL_reg AxesBridge::classLibrary[] = {
	{"new", newAxes},
	{NULL, NULL}
};

void AxesBridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}

int AxesBridge::newAxes(lua_State *L)
{
	// There should be up to 6 Enum parameters () . Following Lua conventions ignore others and use 0 for missing
	int count = std::min(6,lua_gettop(L));

	int axisMask = 0;
	for(int index = 1; index<= count; index++){
		//Select each enum, 
		EnumDescriptorItemPtr item;
		if (EnumItem::getItem(L, index, item))
		{
			if(!item->owner.isType<RBX::Vector3::Axis>() && !item->owner.isType<RBX::NormalId>() )
				throw RBX::runtime_error("Axes.new expects Enum.Axis or Enum.NormalId inputs");
			if(item->owner.isType<RBX::Vector3::Axis>()){
				axisMask |= RBX::Axes::axisToMask((RBX::Vector3::Axis)item->value);
			}
			else{
				axisMask |= RBX::Axes::axisToMask(RBX::Axes::normalIdToAxis((RBX::NormalId)item->value));

			}

		}
	}

	pushNewObject(L, RBX::Axes(axisMask));

	return 1;
}

template<>
int Bridge<RBX::Axes>::on_index(const RBX::Axes& object, const char* name, lua_State *L)
{
	if (strcmp(name,"X")==0)
	{
		lua_pushboolean(L, object.getAxis(RBX::Vector3::X_AXIS));
		return 1;
	}
	if (strcmp(name,"Y")==0)
	{
		lua_pushboolean(L, object.getAxis(RBX::Vector3::Y_AXIS));
		return 1;
	}
	if (strcmp(name,"Z")==0)
	{
		lua_pushboolean(L, object.getAxis(RBX::Vector3::Z_AXIS));
		return 1;
	}


	if (strcmp(name,"Top")==0)
	{
		lua_pushboolean(L, object.getAxisByNormalId(RBX::NORM_Y));
		return 1;
	}
	if (strcmp(name,"Bottom")==0)
	{
		lua_pushboolean(L, object.getAxisByNormalId(RBX::NORM_Y_NEG));
		return 1;
	}
	if (strcmp(name,"Back")==0)
	{
		lua_pushboolean(L, object.getAxisByNormalId(RBX::NORM_Z));
		return 1;
	}
	if (strcmp(name,"Front")==0)
	{
		lua_pushboolean(L, object.getAxisByNormalId(RBX::NORM_Z_NEG));
		return 1;
	}
	if (strcmp(name,"Right")==0)
	{
		lua_pushboolean(L, object.getAxisByNormalId(RBX::NORM_X));
		return 1;
	}
	if (strcmp(name,"Left")==0)
	{
		lua_pushboolean(L, object.getAxisByNormalId(RBX::NORM_X_NEG));
		return 1;
	}

	if(name && !isupper(name[0]))
		throw RBX::runtime_error("%s is not a valid member, did you forget to capitalize the first letter?", name);
	// Failure
	throw RBX::runtime_error("%s is not a valid member, valid members are X,Y,Z,Top,Bottom,Left,Right,Front,Back", name);
}

template<>
void Bridge<RBX::Axes>::on_newindex(RBX::Axes& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

template<>
const char* Bridge<CellID>::className("CellId");

const luaL_reg CellIDBridge::classLibrary[] = {
	{"new", newCellID},
	{NULL, NULL}
};

void CellIDBridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html

}
    
int CellIDBridge::newCellID(lua_State *L)
{
	bool IsNil;
	float vector[3];
	shared_ptr<Instance> objectRef;

	// There should be up to 1 boolean parameter and 3 numerical parameters (IsNil,x,y,z). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(4,lua_gettop(L));
	IsNil = count > 0 ? lua_toboolean(L, 0) != 0 : false;
	for (int i = 1; i<count; i++)
		vector[i-1] = lua_tofloat(L, i);
	for (int i = count+1; i<3; i++)
		vector[i] = 0.0;

	pushNewObject(L, CellID::fromParameters( IsNil, vector, objectRef ) );
	

	return 1;
}

template<>
int Bridge<CellID>::on_index(const CellID& object, const char* name, lua_State *L)
{
	if (strcmp(name,"IsNil")==0)
	{
		lua_pushboolean(L, object.getIsNil());
		return 1;
	}
	if (strcmp(name,"Location")==0)
	{
		Vector3Bridge::pushVector3(L, object.getLocation());
		return 1;
	}
	if (strcmp(name,"TerrainPart")==0)
	{
		ObjectBridge::push(L, object.getTerrainPart());
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member, valid members are IsNil,Location,TerrainPart", name);
}
    
template<>
void Bridge<CellID>::on_newindex(CellID& object, const char* name, lua_State *L)
{
    // Failure
    throw RBX::runtime_error("%s cannot be assigned to", name);
}

//////////////////////////////////////////////////////////////////////////

template<> const char* Bridge<RBX::NumberSequence>::className("NumberSequence");

const luaL_reg NumberSequenceBridge::classLibrary[] = 
{
    { "new", newNumberSequence },
    { 0, 0 },
};

void NumberSequenceBridge::registerClassLibrary(lua_State* L)
{
    // Register the "new" function
    luaL_register(L, className, classLibrary); 
    lua_setreadonly(L, -1, true);
    lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}

int NumberSequenceBridge::newNumberSequence(lua_State* L)
{
    if (lua_isnumber(L,-1))
    {
        pushNumberSequence(L, RBX::NumberSequence(lua_tonumber(L,-1)));
        return 1;
    }

    if (!lua_istable(L,-1))
        throw std::runtime_error("NumberSequence ctor: table of NumberSequenceKeypoints expected.");

    int len = lua_objlen(L, -1); // NOTE: untrusted?

    std::vector<NumberSequence::Key> keys;
    if (len>0) keys.reserve(len);

    for( int j=1; ;++j)
    {
        lua_rawgeti(L,-1,j);

        if (lua_isnil(L,-1)) 
            break;

        NumberSequenceKeypoint kp;
        if (!NumberSequenceKeypointBridge::getValue(L, lua_gettop(L), kp))
        {
            throw RBX::runtime_error("NumberSequence ctor: expected 'NumberSequenceKeypoint' at index %d", j);
        }

        lua_pop(L,1); // pop the keypoint
        keys.push_back(kp);
    }

    pushNumberSequence(L, NumberSequence(keys, true));
    return 1;
}

template<>
int Bridge<NumberSequence>::on_index(const NumberSequence& object, const char* name, lua_State *L)
{
    if (0 == strcmp(name,"Keypoints"))
    {
        const std::vector<NumberSequence::Key>& kp = object.getPoints();
        lua_createtable(L, kp.size(), 0);
        for (unsigned j=0; j<kp.size(); ++j )
        {
            NumberSequenceKeypointBridge::pushNumberSequenceKeypoint(L, kp[j]);
            lua_rawseti(L, -2, j+1);
        }
        return 1;
    }
    throw RBX::runtime_error("'%s' is not a member of NumberSequence", name);
}

template<>
void Bridge<NumberSequence>::on_newindex( NumberSequence& object, const char* name, lua_State *L)
{
    throw RBX::runtime_error("%s cannot be assigned to", name);
}


//////////////////////////////////////////////////////////////////////////
// ColorSequence

template<> const char* Bridge<RBX::ColorSequence>::className("ColorSequence");

const luaL_reg ColorSequenceBridge::classLibrary[] = 
{
    { "new", newColorSequence },
    { 0, 0 },
};

void ColorSequenceBridge::registerClassLibrary(lua_State* L)
{
    // Register the "new" function
    luaL_register(L, className, classLibrary); 
    lua_setreadonly(L, -1, true);
    lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}

int ColorSequenceBridge::newColorSequence(lua_State* L)
{
    do
    {
        Color3 c1, c2;
        bool b1 = Color3Bridge::getValue(L, 1, c1 );
        if (!b1)
            break;

        bool b2 = Color3Bridge::getValue(L, 2, c2 );
        if (!b2)
            c2 = c1;

        pushColorSequence(L, ColorSequence(c1, c2));
        return 1;
    }
    while(0);

    if(1)  // This will do until I implement an editor for ColorSequence props
        throw std::runtime_error("ColorSequence,new(): expected 1 or 2 Color3 arguments");


    if (!lua_istable(L,-1))
    {
        throw std::runtime_error("ColorSequence.new(): table expected.");
    }

    int len = lua_objlen(L, -1); // NOTE: untrusted?

    std::vector<ColorSequence::Key> keys;
    if (len>0) keys.reserve(len);

    for( int j=1; ;++j)
    {
        ColorSequence::Key k;

        lua_rawgeti(L,-1,j);
        if (lua_isnil(L,-1)) 
            break;

        ColorSequenceKeypoint kp;
        if (!ColorSequenceKeypointBridge::getValue(L, lua_gettop(L), kp))
        {
            throw RBX::runtime_error("ColorSequence ctor: expected 'ColorSequenceKeypoint' at index %d", j);
        }

        lua_pop(L,1); // pop the keypoint
        keys.push_back(kp);
    }

    pushColorSequence(L, ColorSequence(keys, true));
    return 1;
}

template<>
int Bridge<ColorSequence>::on_index(const ColorSequence& object, const char* name, lua_State *L)
{
    if (0 == strcmp(name,"Keypoints"))
    {
        const std::vector<ColorSequence::Key>& kp = object.getPoints();
        lua_createtable(L, kp.size(), 0);
        for (unsigned j=0; j<kp.size(); ++j )
        {
            ColorSequenceKeypointBridge::pushColorSequenceKeypoint(L, kp[j]);
            lua_rawseti(L, -2, j+1);
        }
        return 1;
    }
    throw RBX::runtime_error("'%s' is not a member of ColorSequence", name);
}

template<>
void Bridge<ColorSequence>::on_newindex( ColorSequence& object, const char* name, lua_State *L)
{
    throw RBX::runtime_error("%s cannot be assigned to", name);
}



//////////////////////////////////////////////////////////////////////////
// NumberSequenceKeypoint

template<> const char* Bridge<RBX::NumberSequenceKeypoint>::className("NumberSequenceKeypoint");

const luaL_reg NumberSequenceKeypointBridge::classLibrary[] = 
{
    { "new", newNumberSequenceKeypoint },
    { 0, 0 },
};

void NumberSequenceKeypointBridge::registerClassLibrary(lua_State* L)
{
    // Register the "new" function
    luaL_register(L, className, classLibrary); 
    lua_setreadonly(L, -1, true);
    lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}

int NumberSequenceKeypointBridge::newNumberSequenceKeypoint(lua_State* L)
{
    NumberSequenceKeypoint kp;
    kp.time = luaL_checknumber(L,1);
    kp.value = luaL_checknumber(L,2);
    kp.envelope = lua_tonumber(L,3); // optional, 0.0
    pushNumberSequenceKeypoint(L, kp);
    return 1;
}

template<>
int Bridge<NumberSequenceKeypoint>::on_index(const NumberSequenceKeypoint& object, const char* name, lua_State *L)
{
    if (0 == strcmp(name, "Time")) 
    {
        return lua_pushnumber(L, object.time), 1;
    }
    if (0 == strcmp(name, "Value"))
    {
        return lua_pushnumber(L,object.value), 1;
    }
    if (0 == strcmp(name, "Envelope"))
    {
        return lua_pushnumber(L,object.envelope), 1;
    }
    throw RBX::runtime_error("'%s' is not a valid member of NumberSequenceKeypoint", name);
}

template<>
void Bridge<NumberSequenceKeypoint>::on_newindex( NumberSequenceKeypoint& object, const char* name, lua_State *L)
{
    throw RBX::runtime_error("%s cannot be assigned to", name);
}

//////////////////////////////////////////////////////////////////////////
// ColorSequenceKeypoint

template<> const char* Bridge<RBX::ColorSequenceKeypoint>::className("ColorSequenceKeypoint");

const luaL_reg ColorSequenceKeypointBridge::classLibrary[] = 
{
    { "new", newColorSequenceKeypoint },
    { 0, 0 },
};

void ColorSequenceKeypointBridge::registerClassLibrary(lua_State* L)
{
    // Register the "new" function
    luaL_register(L, className, classLibrary); 
    lua_setreadonly(L, -1, true);
    lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}

int ColorSequenceKeypointBridge::newColorSequenceKeypoint(lua_State* L)
{
    ColorSequenceKeypoint kp;
    kp.time = luaL_checknumber(L,1);
    bool b = Color3Bridge::getValue(L, 2, kp.value );
    if (!b)
        throw RBX::runtime_error("could not parse arg #2 to ColorSequenceKeypoint.new(), Color3 expected.");
    kp.envelope = 0; //lua_tonumber(L,3); // disabled for now
    pushColorSequenceKeypoint(L, kp);
    return 1;
}

template<>
int Bridge<ColorSequenceKeypoint>::on_index(const ColorSequenceKeypoint& object, const char* name, lua_State *L)
{
    if (0 == strcmp(name, "Time")) 
    {
        return lua_pushnumber(L, object.time), 1;
    }
    if (0 == strcmp(name, "Value"))
    {
        return Color3Bridge::pushColor3(L,object.value), 1;
    }
    /* disabled for now
    if (0 == strcmp(name, "Envelope"))
    {
        return lua_pushnumber(L,object.envelope), 1;
    }
    */
    throw RBX::runtime_error("'%s' is not a valid member of ColorSequenceKeypoint", name);
}

template<>
void Bridge<ColorSequenceKeypoint>::on_newindex( ColorSequenceKeypoint& object, const char* name, lua_State *L)
{
    throw RBX::runtime_error("%s cannot be assigned to", name);
}
//////////////////////////////////////////////////////////////////////////


template<> const char* Bridge<RBX::NumberRange>::className("NumberRange");

const luaL_reg NumberRangeBridge::classLibrary[] = 
{
    { "new", newNumberRange },
    { 0, 0 },
};

void NumberRangeBridge::registerClassLibrary(lua_State* L)
{
    // Register the "new" function
    luaL_register(L, className, classLibrary); 
    lua_setreadonly(L, -1, true);
    lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}

int NumberRangeBridge::newNumberRange(lua_State* L)
{
    float a,b;
    a = luaL_checknumber(L,1);
    if (lua_isnumber(L,2))
        b = lua_tonumber(L,2);
    else
        b = a;
    
    if (b < a)
        throw RBX::runtime_error("NumberRange: invalid range");

    pushNumberRange(L, NumberRange(a,b));
    return 1;
}

template<>
int Bridge<NumberRange>::on_index(const NumberRange& object, const char* name, lua_State *L)
{
    if (0 == strcmp(name, "Min")) 
    {
        return lua_pushnumber(L, object.min), 1;
    }
    if (0 == strcmp(name, "Max"))
    {
        return lua_pushnumber(L, object.max), 1;
    }
    throw RBX::runtime_error("'%s' is not a valid member of NumberRange", name);
}

template<>
void Bridge<NumberRange>::on_newindex( NumberRange& object, const char* name, lua_State *L)
{
    throw RBX::runtime_error("%s cannot be assigned to", name);
}


//////////////////////////////////////////////////////////////////////////

// The default implementation for registerClass() is available in LuaBridge.cpp. This is a specialization.		
template<>
void Bridge<G3D::Vector3int16>::registerClass (lua_State *L)
{
	// Register the object events
	luaL_newmetatable(L, className);
	Lua::protect_metatable(L, -1);

	lua_pushstring(L, "__type");
	lua_pushstring(L, className);
	lua_settable(L, -3);

	lua_pushstring(L, "__index");
	lua_pushcfunction(L, on_index);
	lua_settable(L, -3);

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, on_newindex);
	lua_settable(L, -3);

	lua_pushstring(L, "__eq");
	lua_pushcfunction(L, on_eq);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, on_tostring);
	lua_settable(L, -3);

	lua_pushstring(L, "__add");
	lua_pushcfunction(L, Vector3int16Bridge::on_add);
	lua_settable(L, -3);

	lua_pushstring(L, "__sub");
	lua_pushcfunction(L, Vector3int16Bridge::on_sub);
	lua_settable(L, -3);

	lua_pushstring(L, "__mul");
	lua_pushcfunction(L, Vector3int16Bridge::on_mul);
	lua_settable(L, -3);

	lua_pushstring(L, "__div");
	lua_pushcfunction(L, Vector3int16Bridge::on_div);
	lua_settable(L, -3);

	lua_pushstring(L, "__unm");
	lua_pushcfunction(L, Vector3int16Bridge::on_unm);
	lua_settable(L, -3);

	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
}

// The default implementation for registerClass() is available in LuaBridge.cpp. This is a specialization.		
template<>
void Bridge<G3D::Vector2int16>::registerClass (lua_State *L)
{
	// Register the object events
	luaL_newmetatable(L, className);
	Lua::protect_metatable(L, -1);

	lua_pushstring(L, "__type");
	lua_pushstring(L, className);
	lua_settable(L, -3);

	lua_pushstring(L, "__index");
	lua_pushcfunction(L, on_index);
	lua_settable(L, -3);

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, on_newindex);
	lua_settable(L, -3);

	lua_pushstring(L, "__eq");
	lua_pushcfunction(L, on_eq);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, on_tostring);
	lua_settable(L, -3);

	lua_pushstring(L, "__add");
	lua_pushcfunction(L, Vector2int16Bridge::on_add);
	lua_settable(L, -3);

	lua_pushstring(L, "__sub");
	lua_pushcfunction(L, Vector2int16Bridge::on_sub);
	lua_settable(L, -3);

	lua_pushstring(L, "__mul");
	lua_pushcfunction(L, Vector2int16Bridge::on_mul);
	lua_settable(L, -3);

	lua_pushstring(L, "__div");
	lua_pushcfunction(L, Vector2int16Bridge::on_div);
	lua_settable(L, -3);

	lua_pushstring(L, "__unm");
	lua_pushcfunction(L, Vector2int16Bridge::on_unm);
	lua_settable(L, -3);

	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
}

// The default implementation for registerClass() is available in LuaBridge.cpp. This is a specialization.
template<>
void Bridge<G3D::Vector3>::registerClass (lua_State *L)
{
	// Register the object events
	luaL_newmetatable(L, className);
	Lua::protect_metatable(L, -1);

	lua_pushstring(L, "__type");
	lua_pushstring(L, className);
	lua_settable(L, -3);

	lua_pushstring(L, "__index");
	lua_pushcfunction(L, on_index);
	lua_settable(L, -3);

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, on_newindex);
	lua_settable(L, -3);

	lua_pushstring(L, "__eq");
	lua_pushcfunction(L, on_eq);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, on_tostring);
	lua_settable(L, -3);

	lua_pushstring(L, "__add");
	lua_pushcfunction(L, Vector3Bridge::on_add);
	lua_settable(L, -3);

	lua_pushstring(L, "__sub");
	lua_pushcfunction(L, Vector3Bridge::on_sub);
	lua_settable(L, -3);

	lua_pushstring(L, "__mul");
	lua_pushcfunction(L, Vector3Bridge::on_mul);
	lua_settable(L, -3);

	lua_pushstring(L, "__div");
	lua_pushcfunction(L, Vector3Bridge::on_div);
	lua_settable(L, -3);

	lua_pushstring(L, "__unm");
	lua_pushcfunction(L, Vector3Bridge::on_unm);
	lua_settable(L, -3);

	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
}

// The default implementation for registerClass() is available in LuaBridge.cpp. This is a specialization.		
template<>
void Bridge<RBX::Vector2>::registerClass (lua_State *L)
{
	// Register the object events
	luaL_newmetatable(L, className);
	Lua::protect_metatable(L, -1);

	lua_pushstring(L, "__type");
	lua_pushstring(L, className);
	lua_settable(L, -3);

	lua_pushstring(L, "__index");
	lua_pushcfunction(L, on_index);
	lua_settable(L, -3);

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, on_newindex);
	lua_settable(L, -3);

	lua_pushstring(L, "__eq");
	lua_pushcfunction(L, on_eq);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, on_tostring);
	lua_settable(L, -3);

	lua_pushstring(L, "__add");
	lua_pushcfunction(L, Vector2Bridge::on_add);
	lua_settable(L, -3);

	lua_pushstring(L, "__sub");
	lua_pushcfunction(L, Vector2Bridge::on_sub);
	lua_settable(L, -3);

	lua_pushstring(L, "__mul");
	lua_pushcfunction(L, Vector2Bridge::on_mul);
	lua_settable(L, -3);

	lua_pushstring(L, "__div");
	lua_pushcfunction(L, Vector2Bridge::on_div);
	lua_settable(L, -3);

	lua_pushstring(L, "__unm");
	lua_pushcfunction(L, Vector2Bridge::on_unm);
	lua_settable(L, -3);

	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
}

// The default implementation for registerClass() is available in LuaBridge.cpp. This is a specialization.		
template<>
void Bridge<G3D::CoordinateFrame>::registerClass (lua_State *L)
{
	// Register the object events
	luaL_newmetatable(L, className);
	Lua::protect_metatable(L, -1);

	lua_pushstring(L, "__type");
	lua_pushstring(L, className);
	lua_settable(L, -3);

	lua_pushstring(L, "__index");
	lua_pushcfunction(L, on_index);
	lua_settable(L, -3);

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, on_newindex);
	lua_settable(L, -3);

	lua_pushstring(L, "__eq");
	lua_pushcfunction(L, on_eq);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, on_tostring);
	lua_settable(L, -3);

	lua_pushstring(L, "__add");
	lua_pushcfunction(L, CoordinateFrameBridge::on_add);
	lua_settable(L, -3);

	lua_pushstring(L, "__sub");
	lua_pushcfunction(L, CoordinateFrameBridge::on_sub);
	lua_settable(L, -3);

	lua_pushstring(L, "__mul");
	lua_pushcfunction(L, CoordinateFrameBridge::on_mul);
	lua_settable(L, -3);

	lua_pushstring(L, "inverse");
	lua_pushcfunction(L, CoordinateFrameBridge::on_inverse);
	lua_settable(L, -3);

	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
}
    
// The default implementation for registerClass() is available in LuaBridge.cpp. This is a specialization.
template<>
void Bridge<RBX::Rect2D>::registerClass(lua_State *L)
{   
    // Register the object events
    luaL_newmetatable(L, className);
    Lua::protect_metatable(L, -1);
    
	lua_pushstring(L, "__type");
	lua_pushstring(L, className);
	lua_settable(L, -3);

    lua_pushstring(L, "__index");
    lua_pushcfunction(L, on_index);
    lua_settable(L, -3);
            
    lua_pushstring(L, "__newindex");
    lua_pushcfunction(L, on_newindex);
    lua_settable(L, -3);
            
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, on_gc);
    lua_settable(L, -3);
            
    lua_pushstring(L, "__eq");
    lua_pushcfunction(L, on_eq);
    lua_settable(L, -3);
            
    lua_pushstring(L, "__tostring");
    lua_pushcfunction(L, on_tostring);
    lua_settable(L, -3);
            
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
void Bridge<PhysicalProperties>::registerClass(lua_State *L)
{
	// Register the object events
	luaL_newmetatable(L, className);
	Lua::protect_metatable(L, -1);

	lua_pushstring(L, "__type");
	lua_pushstring(L, className);
	lua_settable(L, -3);

	lua_pushstring(L, "__index");
	lua_pushcfunction(L, on_index);
	lua_settable(L, -3);

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, on_newindex);
	lua_settable(L, -3);

	lua_pushstring(L, "__eq");
	lua_pushcfunction(L, on_eq);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, on_tostring);
	lua_settable(L, -3);

	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
}

// The default implementation for registerClass() is available in LuaBridge.cpp. This is a specialization.		
template<>
void Bridge<RBX::UDim>::registerClass (lua_State *L)
{
	// Register the object events
	luaL_newmetatable(L, className);
	Lua::protect_metatable(L, -1);

	lua_pushstring(L, "__type");
	lua_pushstring(L, className);
	lua_settable(L, -3);

	lua_pushstring(L, "__index");
	lua_pushcfunction(L, on_index);
	lua_settable(L, -3);

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, on_newindex);
	lua_settable(L, -3);

	lua_pushstring(L, "__eq");
	lua_pushcfunction(L, on_eq);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, on_tostring);
	lua_settable(L, -3);

	lua_pushstring(L, "__add");
	lua_pushcfunction(L, UDimBridge::on_add);
	lua_settable(L, -3);

	lua_pushstring(L, "__sub");
	lua_pushcfunction(L, UDimBridge::on_sub);
	lua_settable(L, -3);

	lua_pushstring(L, "__unm");
	lua_pushcfunction(L, UDimBridge::on_unm);
	lua_settable(L, -3);

	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
}

// The default implementation for registerClass() is available in LuaBridge.cpp. This is a specialization.		
template<>
void Bridge<RBX::UDim2>::registerClass (lua_State *L)
{
	// Register the object events
	luaL_newmetatable(L, className);
	Lua::protect_metatable(L, -1);

	lua_pushstring(L, "__type");
	lua_pushstring(L, className);
	lua_settable(L, -3);

	lua_pushstring(L, "__index");
	lua_pushcfunction(L, on_index);
	lua_settable(L, -3);

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, on_newindex);
	lua_settable(L, -3);

	lua_pushstring(L, "__eq");
	lua_pushcfunction(L, on_eq);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, on_tostring);
	lua_settable(L, -3);

	lua_pushstring(L, "__add");
	lua_pushcfunction(L, UDim2Bridge::on_add);
	lua_settable(L, -3);

	lua_pushstring(L, "__sub");
	lua_pushcfunction(L, UDim2Bridge::on_sub);
	lua_settable(L, -3);

	lua_pushstring(L, "__unm");
	lua_pushcfunction(L, UDim2Bridge::on_unm);
	lua_settable(L, -3);

	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
}

} }
