#include "UndoRedo.h"

std::list<sgCObject*>          detached_objects;

bool        ignore_undo_redo = false;

UR_ELEMENT::UR_ELEMENT()
{
	op_type = NONE;
	obj = NULL;
	matrix = NULL;
}

CommandsGroup::~CommandsGroup()
{
	size_t sz = m_commands.size();
	for (size_t i=0;i<sz;i++)
	{
		if (m_commands[i].matrix)
			delete[] m_commands[i].matrix;
		m_commands[i].m_children.clear();
	}
	m_commands.clear();
}

void CommandsGroup::ChangeOrder()
{
	int sz = m_commands.size();
	std::vector<UR_ELEMENT>  tmpBuffer(sz);
	for (int i=sz-1;i>=0;i--)
	{
		tmpBuffer[sz-i-1]=m_commands[i];
	}
	m_commands=tmpBuffer;
}

void CommandsGroup::RemoveElementWithObject(hOBJ ooo)
{
	for (int i=0;i<(int)m_commands.size();i++)
	{
		if (m_commands[i].obj==ooo)
		{
			delete[] m_commands[i].matrix;
			m_commands[i].m_children.clear();
			m_commands.erase(m_commands.begin()+i);
			i--;
		}
	}
}

bool CommandsGroup::AddElement(UR_ELEMENT nEl)
{
	m_commands.push_back(nEl);
	return true;
}

bool CommandsGroup::Inverse()
{
	int sz = m_commands.size();
	std::vector<UR_ELEMENT>  tmpBuffer(sz);
	for (int i=sz-1;i>=0;i--)
	{
		switch(m_commands[i].op_type) 
		{
		case ADD_OBJ:
			m_commands[i].op_type = DELETE_OBJ;
			break;
		case DELETE_OBJ:
			m_commands[i].op_type = ADD_OBJ;
			break;
		case TRANS_OBJ:
			{
				if (m_commands[i].matrix==NULL)
				{
					assert(0);
					return false;
				}
				sgCMatrix mmm(m_commands[i].matrix);
				if (!mmm.Inverse())
					return false;
				memcpy(m_commands[i].matrix, mmm.GetData(), sizeof(MATR));
			}
			break;
		case CREATE_GROUP:
			m_commands[i].op_type = BREAK_GROUP;
			break;
		case BREAK_GROUP:
			m_commands[i].op_type = CREATE_GROUP;
			break;
		default:
			assert(0);
			return false;
		}
		tmpBuffer[sz-i-1]=m_commands[i];
	}
	m_commands=tmpBuffer;
	return true;
}

bool CommandsGroup::Execute()
{
	size_t sz = m_commands.size();
	for (size_t i=0;i<sz;i++)
	{
		switch(m_commands[i].op_type) 
		{
		case ADD_OBJ:
			assert(objects.num!=0);
			assert(((lpOBJ)(m_commands[i].obj))->extendedClass!=NULL);
			detached_objects.push_back(((lpOBJ)(m_commands[i].obj))->extendedClass);
			detach_item_z(OBJ_LIST, &objects, m_commands[i].obj); 
			break;
		case DELETE_OBJ:
			assert(!detached_objects.empty());
			attach_and_regen(m_commands[i].obj);
			detached_objects.remove(((lpOBJ)(m_commands[i].obj))->extendedClass);
			break;
		case TRANS_OBJ:
			assert(objects.num!=0);
			assert(((lpOBJ)(m_commands[i].obj))->extendedClass!=NULL);
			{
				sgCMatrix mmm(m_commands[i].matrix);
				ignore_undo_redo = true;
				((lpOBJ)(m_commands[i].obj))->extendedClass->InitTempMatrix()->SetMatrix(&mmm);
				((lpOBJ)(m_commands[i].obj))->extendedClass->ApplyTempMatrix();
				((lpOBJ)(m_commands[i].obj))->extendedClass->DestroyTempMatrix();
				ignore_undo_redo = false;
			}
			break;
		case CREATE_GROUP:
			{
				// 
				lpOBJ obj = (lpOBJ)m_commands[i].obj;
				if (obj->type==OGROUP)
				{
						GEO_GROUP geo;
						memcpy(&geo, obj->geo_data, sizeof(GEO_GROUP));
						hOBJ hobj = geo.listh.hhead;
						while(hobj) 
						{
							//   
							obj = (lpOBJ)hobj;
							obj->hhold = NULL;
							hOBJ nextObj = NULL;
							get_next_item_z(OBJ_LIST, hobj, &nextObj);
							detach_item_z(OBJ_LIST, &geo.listh, hobj);
							hobj = nextObj;
						}
						obj = (lpOBJ)m_commands[i].obj;
						memcpy(obj->geo_data, &geo, sizeof(GEO_GROUP));
				}
				else
					if (obj->type==OPATH)
					{
						GEO_PATH geo;
						memcpy(&geo, obj->geo_data, geo_size[OPATH]);
						hOBJ hobj = geo.listh.hhead;
						while(hobj) 
						{
							//   
							obj = (lpOBJ)hobj;
							obj->hhold = NULL;
							hOBJ nextObj = NULL;
							get_next_item_z(OBJ_LIST, hobj, &nextObj);
							detach_item_z(OBJ_LIST, &geo.listh, hobj);
							hobj = nextObj;
						}
						obj = (lpOBJ)m_commands[i].obj;
						memcpy(obj->geo_data, &geo, geo_size[OPATH]);
					}
					else
						assert(0);
			}
			break;
		case BREAK_GROUP:
			{
				//  
				if (m_commands[i].m_children.size()<1)
				{
					assert(0);
					return false;
				}

				lpOBJ obj = (lpOBJ)m_commands[i].obj;
				if (obj->type==OGROUP)
				{
					GEO_GROUP	group;
					memcpy(&group,obj->geo_data, sizeof(GEO_GROUP));
					for (int j=0;j<(int)m_commands[i].m_children.size();j++)
					{
						assert(m_commands[i].m_children[j]);
						detach_item_z(SEL_LIST,&selected,m_commands[i].m_children[j]);
						obj = (lpOBJ)m_commands[i].m_children[j];
						if (obj->isAttachedToScene)
							detach_item_z(OBJ_LIST, &objects,m_commands[i].m_children[j]);
						else
							if (obj->extendedClass)
								detached_objects.remove(obj->extendedClass);
						obj->status &= (~(ST_SELECT|ST_BLINK));
						obj->hhold = m_commands[i].obj;
						attach_item_tail_z(OBJ_LIST,&group.listh,m_commands[i].m_children[j]);
					}
					obj = (lpOBJ)m_commands[i].obj;
					memcpy(obj->geo_data, &group, sizeof(GEO_GROUP));
				}
				else
					if (obj->type==OPATH)
					{
						GEO_PATH	geo;
						memcpy(&geo,obj->geo_data, geo_size[OPATH]);
						for (int j=0;j<(int)m_commands[i].m_children.size();j++)
						{
							assert(m_commands[i].m_children[j]);
							detach_item_z(SEL_LIST,&selected,m_commands[i].m_children[j]);
							obj = (lpOBJ)m_commands[i].m_children[j];
							if (obj->isAttachedToScene)
								detach_item_z(OBJ_LIST, &objects,m_commands[i].m_children[j]);
							else
								if (obj->extendedClass)
									detached_objects.remove(obj->extendedClass);
							obj->status &= (~(ST_SELECT|ST_BLINK));
							obj->hhold = m_commands[i].obj;
							attach_item_tail_z(OBJ_LIST,&geo.listh,m_commands[i].m_children[j]);
						}
						obj = (lpOBJ)m_commands[i].obj;
						memcpy(obj->geo_data, &geo, geo_size[OPATH]);
					}
					else
						assert(0);
			}
			break;
		default:
			assert(0);
			return false;
		}
	}
	return true;
}

