#ifndef __UNDO_REDO__
#define __UNDO_REDO__

#include "CORE//sg.h"

#include <vector>
#include <list>

extern  std::list<sgCObject*>          detached_objects;

typedef enum
{
	NONE,
	ADD_OBJ,
	DELETE_OBJ,
	TRANS_OBJ,
	CREATE_GROUP,
	BREAK_GROUP
} UR_ELEM_TYPE;

struct  UR_ELEMENT
{
	UR_ELEM_TYPE      op_type;
	hOBJ              obj;
	sgFloat*           matrix;
	std::vector<hOBJ> m_children;
	UR_ELEMENT();
};

class CommandsGroup
{
	std::vector<UR_ELEMENT>   m_commands;
public:
	~CommandsGroup();

	void ChangeOrder();
	void RemoveElementWithObject(hOBJ);
	bool IsEmpty() {return m_commands.empty();};
	bool AddElement(UR_ELEMENT);
	bool Inverse();
	bool Execute();
};

#endif