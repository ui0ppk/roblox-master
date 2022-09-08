#ifndef  __sgObject__
#define  __sgObject__

#define  SG_OBJ_NAME_MAX_LEN			64
#define  SG_OBJ_GEOMETRY_ID_MAX_LEN     39

typedef enum 
{
	SG_OT_BAD_OBJECT,
	SG_OT_POINT,
	SG_OT_LINE,
	SG_OT_CIRCLE,
	SG_OT_ARC,
	SG_OT_SPLINE,
	SG_OT_TEXT,
	SG_OT_CONTOUR,
	SG_OT_DIM,
	SG_OT_3D,
	SG_OT_GROUP
} SG_OBJECT_TYPE;

typedef enum
{
	SG_OA_COLOR,
	SG_OA_LINE_TYPE,
	SG_OA_LINE_THICKNESS,
	SG_OA_LAYER,
	SG_OA_DRAW_STATE
} SG_OBJECT_ATTR_ID;

/* SG_OA_DRAW_STATE values*/
#define  SG_DS_FRAME     0x0200 
#define  SG_DS_HIDE      0x0100 
#define  SG_DS_GABARITE  0x0080 
#define  SG_DS_FULL      0x0040 

#define  SG_DS_USER_1    0x8000 
#define  SG_DS_USER_2    0x4000
#define  SG_DS_USER_3    0x2000
#define  SG_DS_USER_4    0x1000
#define  SG_DS_USER_5    0x0800
#define  SG_DS_USER_6    0x0400
#define  SG_DS_USER_7    0x0020
#define  SG_DS_USER_8    0x0010
#define  SG_DS_USER_9    0x0008
#define  SG_DS_USER_10   0x0004
#define  SG_DS_USER_11   0x0002
#define  SG_DS_USER_12   0x0001


class sgCore_API  sgCObject
{
	SG_OBJ_HANDLE    m_object_handle;
protected:
	sgCObject();
	sgCObject(SG_OBJ_HANDLE);
	virtual          ~sgCObject();	
	void             PostCreate();

public:
	sgCObject*       Clone();
	SG_OBJECT_TYPE	 GetType() const;

	bool             SetUserGeometry(const char* user_geometry_ID,
										const unsigned short user_geometry_size,
										const void* user_geometry_data);
	const char*		 GetUserGeometryID() const;
	const void*      GetUserGeometry(unsigned short& user_geometry_size) const;

	bool                SetUserDynamicData(const SG_USER_DYNAMIC_DATA* u_d_d);
	SG_USER_DYNAMIC_DATA*  GetUserDymanicData() const;

	unsigned short   GetAttribute(SG_OBJECT_ATTR_ID attributeId) const;
	virtual  bool    SetAttribute(SG_OBJECT_ATTR_ID attributeId, unsigned short attributeValue);

	const char*      GetName() const;
	bool             SetName(const char* object_name);

	const sgCObject* GetParent() const;

	void             GetGabarits(SG_POINT& p_min, SG_POINT& p_max);

	void             Select(bool);
	bool             IsSelect() const;
	bool             IsAttachedToScene() const;

	sgCMatrix*       InitTempMatrix();
	bool             DestroyTempMatrix();
	sgCMatrix*       GetTempMatrix();
	virtual  bool    ApplyTempMatrix();

public:
	static   void    DeleteObject(sgCObject*);

protected:
	SG_POINT             m_min;
	SG_POINT             m_max;
	sgCMatrix*           m_temp_matrix;
	SG_USER_DYNAMIC_DATA* m_user_dynamic_data;

	PRIVATE_ACCESS
};

#define    sgDeleteObject   sgCObject::DeleteObject


class  IObjectsList
{
public:
	virtual  int         GetCount()	const				= 0;
	virtual  sgCObject*  GetHead()	const				= 0;
	virtual  sgCObject*  GetNext(sgCObject*) const      = 0;  
	virtual  sgCObject*  GetTail()	const				= 0;
	virtual  sgCObject*  GetPrev(sgCObject*) const      = 0;  
};



#endif