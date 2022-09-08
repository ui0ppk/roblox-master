#ifndef  __sgIApp__
#define  __sgIApp__

//#include "sgObject.h"

class  IProgresser
{
public:
	virtual bool InitProgresser(unsigned int steps_count) = 0;
	virtual bool Progress(unsigned int progress_percent)  = 0;
	virtual bool StopProgresser()                = 0;
};

class  ISceneTreeControl
{
public:
	typedef  void*  TREENODEHANDLE;
	virtual  TREENODEHANDLE  AddNode(sgCObject**, TREENODEHANDLE)		= 0;
	virtual  bool            ShowNode(sgCObject*)						= 0;
	virtual  bool            HideNode(TREENODEHANDLE)					= 0;
	virtual  bool            UpdateNode(TREENODEHANDLE)					= 0;
	virtual  bool            RemoveNode(TREENODEHANDLE)					= 0;
	virtual  bool            ClearTree()                                = 0;
};

sgCore_API  void  sgSetToObjectTreeNodeHandle(sgCObject*,
											  ISceneTreeControl::TREENODEHANDLE);
sgCore_API  ISceneTreeControl::TREENODEHANDLE  sgGetFromObjectTreeNodeHandle(sgCObject*);

class  ICoreAppInterface
{
public:
	virtual IProgresser*			GetProgresser()        =0;
	virtual ISceneTreeControl*		GetSceneTreeControl()  =0;     
};

sgCore_API  bool		 sgSetApplicationInterface(ICoreAppInterface*);


#endif