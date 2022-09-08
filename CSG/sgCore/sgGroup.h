#ifndef  __sgGroup__
#define  __sgGroup__

/************************************************************************/
/* Group                                                                */
/************************************************************************/
class sgCore_API   sgCGroup : public sgCObject
{
protected:
	sgCGroup();
	sgCGroup(SG_OBJ_HANDLE);
	virtual    ~sgCGroup();

	IObjectsList*           m_children;
public:
	IObjectsList*           GetChildrenList() const;
	virtual  bool			SetAttribute(SG_OBJECT_ATTR_ID, unsigned short);

	virtual  bool			ApplyTempMatrix();

	static   sgCGroup*		CreateGroup(sgCObject** objcts, int cnt);

	bool					BreakGroup(sgCObject** objcts);

	PRIVATE_ACCESS
};

#endif