#ifndef V8XML_XMLELEMENT_H
#define V8XML_XMLELEMENT_H

#include "G3D/Vector3.h"
#include "G3D/CoordinateFrame.h"
#include "G3D/Color3.h"
#include "rbx/Debug.h"
#include "Util/Name.h"
#include "Util/Memory.h"
#include "util/Utilities.h"
#include "util/ContentId.h"

#include <string>
#include <map>
#include <list> 
#include <vector> 

#include"Util/Utilities.h"
#include"Util/Handle.h"

//typedef std::string XmlTag;
typedef RBX::Name XmlTag;


// w3 XSD schema specification states that IDREF can't have empty values and can't have the xsi:nil attribute.
// We use xsi:nil to signify "don't change your value when reading me"
// As a result, we define 2 special IDREF strings "null" and "nil"
//
// "nil" means "don't change your current value
// 
// for an IDREF "null" means "set your value to NULL"
extern const RBX::Name& value_IDREF_null;
extern const RBX::Name& value_IDREF_nil;

// TODO: Put these in a file that knows about the Roblox schema
extern const XmlTag& name_root;
extern const XmlTag& tag_roblox;
extern const XmlTag& name_xsinil;
extern const XmlTag& name_xsitype;
extern const XmlTag& tag_xmlnsxsi;
extern const XmlTag& tag_xsinoNamespaceSchemaLocation;
extern const XmlTag& tag_version;
extern const XmlTag& tag_assettype;
extern const XmlTag& name_referent;
extern const XmlTag& tag_External;
extern const XmlTag& name_Ref;
extern const XmlTag& name_token;
extern const XmlTag& name_name;
extern const XmlTag& tag_bool;
extern const XmlTag& tag_Refs;
extern const XmlTag& tag_X;
extern const XmlTag& tag_Y;
extern const XmlTag& tag_Z;

extern const XmlTag& tag_R00;
extern const XmlTag& tag_R01;
extern const XmlTag& tag_R02;
extern const XmlTag& tag_R10;
extern const XmlTag& tag_R11;
extern const XmlTag& tag_R12;
extern const XmlTag& tag_R20;
extern const XmlTag& tag_R21;
extern const XmlTag& tag_R22;

extern const XmlTag& tag_R;
extern const XmlTag& tag_G;
extern const XmlTag& tag_B;
extern const XmlTag& tag_class;
extern const XmlTag& tag_Item;
extern const XmlTag& tag_Properties;
extern const XmlTag& tag_Feature;

extern const XmlTag& tag_hash;
extern const XmlTag& tag_null;
extern const XmlTag& tag_mimeType;

extern const XmlTag& tag_S;
extern const XmlTag& tag_O;

extern const XmlTag& tag_XS;
extern const XmlTag& tag_XO;
extern const XmlTag& tag_YS;
extern const XmlTag& tag_YO;

extern const XmlTag& tag_faces;
extern const XmlTag& tag_axes;

extern const XmlTag& tag_Origin;
extern const XmlTag& tag_Direction;

extern const XmlTag& tag_Min;
extern const XmlTag& tag_Max;

extern const XmlTag& tag_WebTable;
extern const XmlTag& tag_WebList;
extern const XmlTag& tag_WebEntry;
extern const XmlTag& tag_WebKey;
extern const XmlTag& tag_WebValue;
extern const XmlTag& tag_WebType;

extern const XmlTag& tag_customPhysProp;
extern const XmlTag& tag_customDensity;
extern const XmlTag& tag_customFriction;
extern const XmlTag& tag_customElasticity;
extern const XmlTag& tag_customFrictionWeight;
extern const XmlTag& tag_customElasticityWeight;



// TODO: Optimization:  Create a base class that has not children or attributes

class XmlElement;
class XmlWriter;

class XmlNameValuePair
{
public:

#ifndef _WIN32
#ifdef UINT
#undef UINT
#endif
#endif

	typedef enum { NONE, NAME, STRING, CONTENTID, BOOL, INT, UINT, FLOAT, HANDLE, DOUBLE } ValueType;

private:
	const XmlTag& tag;
	mutable ValueType valueType;
	union {
		mutable std::string* stringValue;
		mutable RBX::ContentId* contentIdValue;
		mutable bool boolValue;
		mutable int intValue;
		mutable unsigned int uintValue;
		mutable float floatValue;
		mutable double doubleValue;
		mutable const RBX::Name* nameValue;
		mutable RBX::InstanceHandle* handleValue;		// TODO: with in-place constructor/destructor, could we avoid a "new"
	};

	void clearValue() const;

public:
	XmlNameValuePair(const XmlTag& tag)
		:tag(tag),valueType(NONE)     {}
	XmlNameValuePair(const XmlTag& tag, const std::string& text)
		:tag(tag),stringValue(new std::string(text)),valueType(STRING)     {}
	XmlNameValuePair(const XmlTag& tag, const char* text)
		:tag(tag),stringValue(new std::string(text)),valueType(STRING)     {}
	XmlNameValuePair(const XmlTag& tag, RBX::ContentId contentId)
		:tag(tag),contentIdValue(new RBX::ContentId(contentId)),valueType(CONTENTID)     {}
	XmlNameValuePair(const XmlTag& tag, const int _number)
		:tag(tag),valueType(INT),intValue(_number) {}
	XmlNameValuePair(const XmlTag& tag, const unsigned int _number)
		:tag(tag),valueType(UINT),uintValue(_number) {}
	XmlNameValuePair(const XmlTag& tag, const RBX::Name* value)
		:tag(tag),valueType(NAME),nameValue(value) {}
	XmlNameValuePair(const XmlTag& tag, bool value)
		:tag(tag),valueType(BOOL),boolValue(value) {}
	XmlNameValuePair(const XmlTag& tag, float value)
		:tag(tag),valueType(FLOAT),floatValue(value) {}
	XmlNameValuePair(const XmlTag& tag, double value)
		:tag(tag),valueType(DOUBLE),doubleValue(value) {}
	XmlNameValuePair(const XmlTag& tag, RBX::InstanceHandle value)
		:tag(tag),valueType(HANDLE),handleValue(new RBX::InstanceHandle(value)) {}
	~XmlNameValuePair() {
		clearValue();
	}
	const XmlTag& getTag() const				{return tag;}

	bool isValueEmpty() const { return valueType==NONE; }

	// equality tests (does not change valueType
	bool isValueEqual(const std::string& value) const;
	bool isValueEqual(RBX::ContentId contentId) const;
	bool isValueEqual(int value) const;
	bool isValueEqual(float value) const;
	bool isValueEqual(double value) const;
	bool isValueEqual(bool value) const;
	bool isValueEqual(const RBX::Name* value) const;
	bool isValueEqual(RBX::InstanceHandle value) const;
	// Templated version that can be implemented by clients
	template<class T>
	bool isValueEqual(T value) const;

	std::string toString(XmlWriter* writer) const;

	// returns true if the value is of the given type
	template<class T>
	bool isValueType() const;
	ValueType getValueType() const				{return valueType;}

	// get requests. If possible, these functions will convert valueType to the desired type
	// TODO: refactor to "toValue"?
	bool getValue(std::string &value) const;
	bool getValue(RBX::ContentId& contentId) const;
	bool getValue(int &value) const;
	bool getValue(unsigned int &value) const;
	bool getValue(float &value) const;
	bool getValue(double &value) const;
	bool getValue(bool &value) const;
	bool getValue(const RBX::Name* &value) const;
	bool getValue(RBX::InstanceHandle &value) const;
	// Templated version that can be implemented by clients
	template<class T>
	bool getValue(T& value) const;

	void setValue(std::string value) {clearValue(); stringValue = new std::string(value); valueType=STRING; }
	void setValue(RBX::ContentId contentId) {clearValue(); contentIdValue = new RBX::ContentId(contentId); valueType=CONTENTID; }
	void setValue(const char* value) {clearValue(); stringValue = new std::string(value); valueType=STRING; }
	void setValue(int value) {clearValue(); intValue = value; valueType=INT; }
	void setValue(unsigned int value) {clearValue(); uintValue = value; valueType=UINT; }
	void setValue(bool value) {clearValue(); boolValue = value; valueType=BOOL; };
	void setValue(float value) {clearValue(); floatValue = value; valueType=FLOAT; };
	void setValue(double value) {clearValue(); doubleValue = value; valueType=DOUBLE; };
	void setValue(const RBX::Name* value) {clearValue(); nameValue = value; valueType=NAME; };
	void setValue(RBX::InstanceHandle handle) {clearValue(); handleValue = new RBX::InstanceHandle(handle); valueType=HANDLE; };
	// Templated version that can be implemented by clients
	template<class T>
	void setValue(T value);
};

namespace RBX
{
	// Parent and Child are a variation on the left-child/right-sibling tree, where each node also contains a reference
	// the its right-most child. This makes it fast to push a child at the right-most end
	template <class ChildClass>
	class Parent {
	private:
		ChildClass* first;	// "left child"
		ChildClass* last;	// "right child"

	public:
		Parent()
			:first(NULL)
			,last(NULL)
		{}

		void pushBackChild(ChildClass* child) {
			if (last==NULL)
				first = child;
			else
				last->setNextSibling(child);
			last = child;
		}

		void pushFrontChild(ChildClass* child) {
			if (first==NULL)
				last = child;
			else
				child->setNextSibling(first);
			first = child;
		}

		void addChild(ChildClass* child) {
			pushBackChild(child);
		}

		void removeChild(ChildClass* child) {
			if (first==child)
				first = child->nextSibling();
			else {
				for (ChildClass* sibling = first; sibling!=NULL; sibling = sibling->nextSibling()) {
					if (sibling->nextSibling()==child) {
						sibling->setNextSibling(child->nextSibling());
						if (child==last)
							last = sibling;
						break;
					}
				}
			}
			child->setNextSibling(NULL);
		}

		ChildClass* firstChild() { return first; }
		ChildClass* nextChild(ChildClass* child) { return child->nextSibling(); }
		const ChildClass* firstChild() const { return first; }
		const ChildClass* nextChild(const ChildClass* child) const { return child->nextSibling(); }
	};


	template <class SiblingClass>
	class Sibling {
	private:
		SiblingClass* next;	// "right sibling"
	protected:
		Sibling():next(NULL) {}
		Sibling(SiblingClass* nextSibling):next(nextSibling) {}
	public:
		const SiblingClass* nextSibling() const { return next; }
		SiblingClass* nextSibling() { return next; }
	private:
		friend class Parent<SiblingClass>;	// Only give "ParentClass" access to the setting of the sibling
		void setNextSibling(SiblingClass* nextSibling) { this->next = nextSibling; }
	};
}

class XmlAttribute 
	: public RBX::Sibling<XmlAttribute>
	, public XmlNameValuePair
	, public RBX::Allocator<XmlAttribute> 
{
public:
	XmlAttribute(const XmlTag& tag)
		:XmlNameValuePair(tag) {}
	template <typename T>
	XmlAttribute(const XmlTag& tag, T value)
		:XmlNameValuePair(tag, value) {}
};

class XmlElement 
	: public RBX::Sibling<XmlElement>
	, public RBX::Parent<XmlElement>
	, public XmlNameValuePair
	, public RBX::Allocator<XmlElement> 
{
#ifdef _DEBUG
	char leak[15];
	void recordLeak()
	{
		strcpy(leak,"XmlElement");
	}
#endif
private:
	RBX::Parent<XmlAttribute> attributes;

public:
	XmlElement(const XmlTag& tag)
		:XmlNameValuePair(tag)
	{
#ifdef _DEBUG
	recordLeak();
#endif	
	}
	template <typename T>
	XmlElement(const XmlTag& tag, T value)
		:XmlNameValuePair(tag, value)
	{
#ifdef _DEBUG
	recordLeak();
#endif	
	}

	~XmlElement() {
		XmlAttribute* attr = attributes.firstChild();
		while (attr)
		{
			XmlAttribute* temp = attr;
			attr = attr->nextSibling();
			delete temp;
		}

		XmlElement* child = firstChild();
		while (child)
		{
			XmlElement* temp = child;
			child = child->nextSibling();
			delete temp;
		}
	}


	/////////////////////////////////////////////////////////////////////
	// attributes
	//

	// returns true if this element has an xsi:nil attribute with value "true"
	bool isXsiNil() const;

	template <typename T>
	inline void addAttribute(const XmlTag& _tag, T value) {
		XmlAttribute* a = new XmlAttribute(_tag, value);
		addAttribute(a);
	}

	inline const XmlAttribute* getFirstAttribute() const                              {return attributes.firstChild();}
	inline const XmlAttribute* getNextAttribute(const XmlAttribute* attribute) const  {return attributes.nextChild(attribute);}
	inline XmlAttribute* getFirstAttribute()                                          {return attributes.firstChild();}
	inline XmlAttribute* getNextAttribute(XmlAttribute* attribute)                    {return attributes.nextChild(attribute);}

	const XmlAttribute* findAttribute(const XmlTag& _tag) const;
	XmlAttribute* findAttribute(const XmlTag& _tag);
	inline bool findAttributeValue(const XmlTag& _tag, const RBX::Name*& value) const {
		const XmlAttribute* attribute = findAttribute(_tag);
		if (attribute==NULL)
			return false;
		return attribute->getValue(value);
	}
	inline bool findAttributeValue(const XmlTag& _tag, std::string& value) const {
		const XmlAttribute* attribute = findAttribute(_tag);
		if (attribute==NULL)
			return false;
		return attribute->getValue(value);
	}

	/////////////////////////////////////////////////////////////////////
	// child elements
	//
	inline XmlElement* addChild(XmlElement* element) { pushBackChild(element); return element; }
	XmlElement* addChild(const XmlTag& _tag)	{
		XmlElement* n = new XmlElement(_tag);
		return addChild(n);
	}

	const XmlElement* findFirstChildByTag(const XmlTag& _tag) const;
	const XmlElement* findNextChildWithSameTag(const XmlElement* node) const;	

protected:
	inline void addAttribute(XmlAttribute* attribute) {
		attributes.pushBackChild(attribute);
	}

};


#endif