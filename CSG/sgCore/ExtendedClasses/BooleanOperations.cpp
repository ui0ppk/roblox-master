#include "..//CORE//sg.h"

sgCGroup*	sgBoolean::Intersection(const sgC3DObject& aOb,const sgC3DObject& bOb)
{
	sgCGroup*  retVal=NULL;
	
	LISTH resList;
	init_listh(&resList);

	hOBJ  reservObj = NULL;

	if (boolean_operation(GetObjectHandle(&aOb), GetObjectHandle(&bOb),INTER,&resList)!=OSTRUE)
	{
		//assert(0);
		return retVal;
	}

	if (resList.num<1)
		return retVal;

	sgCObject** objcts = (sgCObject**)malloc(sizeof(sgCObject*)*resList.num);
	hOBJ list_elem = resList.hhead;
	int i=0;
	while (list_elem) 
	{
		char* typeID = "{0000000000000-0000-0000-000000000000}";
		set_hobj_attr_value(id_TypeID, list_elem, typeID);
		objcts[i++] = ObjectFromHandle(list_elem);
		get_next_item(list_elem,&list_elem);
	}
	
	retVal = sgCGroup::CreateGroup(objcts,resList.num);

	if (reservObj)
	{
		o_free(reservObj,NULL);
	}

	free(objcts);

	return retVal;
}

sgCGroup*	sgBoolean::Union(const sgC3DObject& aOb,const sgC3DObject& bOb, int& errcode)
{
	sgCGroup*  retVal=NULL;

	LISTH resList;
	init_listh(&resList);

	hOBJ  reservObj = NULL;

    errcode = boolean_operation(GetObjectHandle(&aOb), GetObjectHandle(&bOb),UNION,&resList);
	if (errcode!=OSTRUE)
	{
		//assert(0);
		return retVal;
	}

	if (resList.num<1)
		return retVal;

	sgCObject** objcts = (sgCObject**)malloc(sizeof(sgCObject*)*resList.num);
	hOBJ list_elem = resList.hhead;
	int i=0;
	while (list_elem) 
	{
		char* typeID = "{0000000000000-0000-0000-000000000000}";
		set_hobj_attr_value(id_TypeID, list_elem, typeID);
		objcts[i++] = ObjectFromHandle(list_elem);
		get_next_item(list_elem,&list_elem);
	}

	retVal = sgCGroup::CreateGroup(objcts,resList.num);

	if (reservObj)
	{
		o_free(reservObj,NULL);
	}

	free(objcts);

	return retVal;
}

sgCGroup* sgBoolean::Sub(const sgC3DObject& aOb,const sgC3DObject& bOb, int& errcode)
{
	sgCGroup*  retVal=NULL;

	LISTH resList;
	init_listh(&resList);

	hOBJ  reservObj = NULL;

    errcode = boolean_operation(GetObjectHandle(&aOb), GetObjectHandle(&bOb),SUB,&resList);
	if (errcode!=OSTRUE)
	{
		//assert(0);
		return retVal;
	}

	if (resList.num<1)
		return retVal;

	sgCObject** objcts = (sgCObject**)malloc(sizeof(sgCObject*)*resList.num);
	hOBJ list_elem = resList.hhead;
	int i=0;
	while (list_elem) 
	{
		char* typeID = "{0000000000000-0000-0000-000000000000}";
		set_hobj_attr_value(id_TypeID, list_elem, typeID);
		objcts[i++] = ObjectFromHandle(list_elem);
		get_next_item(list_elem,&list_elem);
	}

	retVal = sgCGroup::CreateGroup(objcts,resList.num);

	if (reservObj)
	{
		o_free(reservObj,NULL);
	}
	free(objcts);

	return retVal;
}

sgCGroup*	sgBoolean::IntersectionContour(const sgC3DObject& aOb,const sgC3DObject& bOb)
{
	sgCGroup*  retVal=NULL;

	LISTH resList;
	init_listh(&resList);

	hOBJ  reservObj = NULL;

	if (boolean_operation(GetObjectHandle(&aOb), GetObjectHandle(&bOb),LINE_INTER,&resList)!=OSTRUE)
	{
		//assert(0);
		return retVal;
	}

	if (resList.num<1)
		return retVal;

	sgCObject** objcts = (sgCObject**)malloc(sizeof(sgCObject*)*resList.num);
	hOBJ list_elem = resList.hhead;
	int i=0;
	while (list_elem) 
	{
		char* typeID = "{0000000000000-0000-0000-000000000000}";
		set_hobj_attr_value(id_TypeID, list_elem, typeID);
		objcts[i++] = ObjectFromHandle(list_elem);
		get_next_item(list_elem,&list_elem);
	}

	retVal = sgCGroup::CreateGroup(objcts,resList.num);

	if (reservObj)
	{
		o_free(reservObj,NULL);
	}
	free(objcts);

	return retVal;
}

sgCGroup*	sgBoolean::Section(const sgC3DObject& obj, const SG_VECTOR& planeNormal, 
							   sgFloat planeD)
{
	sgCGroup*  retVal=NULL;

	LISTH resList;
	init_listh(&resList);

	D_PLANE   plane;
	memcpy(&plane.v, &planeNormal, sizeof(SG_VECTOR));
	plane.d = planeD;

	LISTH  obj_list;
	
	init_listh(&obj_list);

	attach_item_tail_z(SEL_LIST, &obj_list, GetObjectHandle(&obj));
		
	if (!cut(&obj_list, SEL_LIST, &plane, &resList))
	{
		//assert(0);
		return retVal;
	}

	detach_item_z(SEL_LIST, &obj_list, GetObjectHandle(&obj));
	init_listh(&obj_list);

	if (resList.num<1)
		return retVal;

	sgCObject** objcts = (sgCObject**)malloc(sizeof(sgCObject*)*resList.num);
	hOBJ list_elem = resList.hhead;
	int i=0;
	while (list_elem) 
	{
		char* typeID = "{0000000000000-0000-0000-000000000000}";
		set_hobj_attr_value(id_TypeID, list_elem, typeID);
		objcts[i++] = ObjectFromHandle(list_elem);
		get_next_item(list_elem,&list_elem);
	}

	retVal = sgCGroup::CreateGroup(objcts,resList.num);

	free(objcts);
		
	return retVal;
}