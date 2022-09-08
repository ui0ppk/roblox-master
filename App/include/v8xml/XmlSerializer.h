#ifndef V8XML_XMLSERIALIZER_H
#define V8XML_XMLSERIALIZER_H

#include <vector>
#include <stack>
#include <ostream>

#include <boost/unordered_map.hpp>

#include "V8Xml/XmlElement.h"

namespace RBX
{
	class ContentProvider;
}
class RBXBaseClass XmlWriter : public boost::noncopyable {

protected:
	std::map<RBX::InstanceHandle, int> handles;
	typedef boost::unordered_map<std::string, RBX::InstanceHandle> IdValidationMap;
	IdValidationMap idValidationMap;
	std::ostream& stream;
	XmlWriter(std::ostream& stream);
public:
	virtual void serialize(const XmlElement* xmlNode) = 0;

	int getHandleIndex(RBX::InstanceHandle h) {
		int i;
		std::map<RBX::InstanceHandle, int>::iterator iter = handles.find(h);
		if (iter==handles.end()) {
			i = static_cast<int>(handles.size());	// TODO:  Should this be size_t getHandleIndex?
			handles[h] = i;
			return i;
		} else
			i = iter->second;
		return i;
	}

	bool isValidId(const std::string& id, const RBX::InstanceHandle& h) const
	{
		IdValidationMap::const_iterator itr = idValidationMap.find(id);
		return itr == idValidationMap.end() || (itr->second.getTarget() == h.getTarget());
	}

	void recordId(const std::string& id, const RBX::InstanceHandle& h)
	{
		RBXASSERT(isValidId(id, h));
		idValidationMap[id] = h;
	}
};

class TextXmlWriter : public XmlWriter
{
public:
	TextXmlWriter(std::ostream& stream)
		:XmlWriter(stream)
	{}
	void serialize(const XmlElement* xmlNode);
protected:
	void serialize(const XmlElement* xmlNode, int depth);
	void writeOpenTag(const XmlElement* element, int depth);
	void writeCloseTag(const XmlElement* element, int depth);

public:
	static void xmlEncodedWrite(std::ostream& stream, const std::string& text);
	static void xmlOrCDataEncodedWrite(std::ostream& stream, const std::string& text);
protected:
	void serializeNode(const XmlElement* xmlNode, int depth);

};

class XmlParser : public boost::noncopyable {

protected:
	std::streambuf* buffer;
	XmlParser(std::streambuf* buffer);
	std::stack<XmlElement*> elements;		// stack
public:
	virtual std::auto_ptr<XmlElement> parse() = 0;
};

class TextXmlParser : public XmlParser {

	std::map<std::string, std::string> legacyHashes;		// workaround for a bug in the MD5 hasher
public:
	TextXmlParser(std::streambuf* buffer)
		:XmlParser(buffer)
	{}
	std::auto_ptr<XmlElement> parse();

private:
	void skipWhitespace(); 

	std::string readTag();
	std::string readFirstTag();
	std::string readText(bool decode);

	// attribute handling
	std::string removeTag(const std::string& contents, int &index);
	std::string findNextToken(const std::string& contents, int &index);
	std::string findText(const std::string& attribute);
	XmlElement* parseAttributes(const std::string& currentTag);
};



#endif
