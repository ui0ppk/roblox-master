#ifndef  __sgScene__
#define  __sgScene__


class sgCore_API  sgCScene
{
public:
	static   sgCScene*  GetScene();
private:
	sgCScene();
	~sgCScene();

	IObjectsList*  m_objects_list;
	IObjectsList*  m_selected_objects_list;
public:

	IObjectsList*  GetObjectsList() const;
	IObjectsList*  GetSelectedObjectsList() const;

	bool AttachObject(sgCObject*);
	void DetachObject(sgCObject*);

	/*********************************************************************/
	bool StartUndoGroup();
	bool EndUndoGroup();
	bool IsUndoStackEmpty() const;
	bool IsRedoStackEmpty() const;
	bool Undo();
	bool Redo();
	/*********************************************************************/
	void Clear();

	void GetGabarits(SG_POINT& p_min, SG_POINT& p_max);

	PRIVATE_ACCESS
};

#define  sgGetScene			sgCScene::GetScene


#endif