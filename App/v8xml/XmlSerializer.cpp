#include "stdafx.h"

#include "RbxAssert.h"
#include "V8Xml/XmlSerializer.h"
#include "reflection/type.h"
#include "rbx/Debug.h"
#include <sstream>
#include "util/base64.hpp"
#include "util/exception.h"
#include "V8DataModel/ContentProvider.h"
#include <boost/algorithm/string.hpp>

static const char* kCDATA_OPEN = "<![CDATA[";
static const char* kCDATA_CLOSE = "]]>";

using std::vector;
using std::string;


bool isCloseTag(const char* s) {

	if (s[0] != '<')
		return false;
	if (s[1] != '/')
		return false;
	return true;
}

bool endsWithClose(const std::string& test)
{
	size_t size = test.size();
    if (size < 2)
		return false;
	const char* s = test.c_str();
	if (s[size-2] != '/')
		return false;
	if (s[size-1] != '>')
		return false;
	return true;
}

////////////////////////////////////////////////////////////////////

class Whitespaces
{
public:
	char data[256];
	Whitespaces()
	{
		memset(data, 0, 256);
		data['\n'] = 1;
		data['\t'] = 1;
		data[' '] = 1;
		data['\r'] = 1;
		data['\f'] = 1;
	}
};

static Whitespaces whitespaces;

#define myIsWhiteSpace(c) (whitespaces.data[c])



void TextXmlParser::skipWhitespace() 
{
	while (true)
	{
		const int ch = buffer->sgetc();
		if (ch==EOF)
			return;
		if (!myIsWhiteSpace(static_cast<char>(ch)))
			return;
		buffer->sbumpc();
	}
}

string TextXmlParser::readFirstTag()
{
	// TODO: Opt: Can this be refined for speed???
    skipWhitespace();

	char c = 0;
	// Skip past any "Byte-Order-Mark": http://en.wikipedia.org/wiki/Byte_Order_Mark
	int count = 0;
	do
	{
		if (buffer->sgetc()==EOF)
			throw std::runtime_error("Expected '<' but got EOF in Xml stream");
		if (count++>4)
		{
			std::string message = "tag expected after Byte-Order-Mark";
			throw std::runtime_error(message);
		}
		c = static_cast<char>(buffer->sbumpc());
	}
	while (c!='<');

	std::string sb;
	sb += c;
	do
	{
		if (buffer->sgetc()==EOF)
			throw std::runtime_error("Expected '>' but got EOF in Xml stream");
		c = static_cast<char>(buffer->sbumpc());
		sb += c;
	}
    while (c != '>');

    return sb;
}



string TextXmlParser::readTag()
{
	// TODO: Opt: Can this be refined for speed???
    skipWhitespace();

	if (buffer->sgetc()==EOF)
		throw std::runtime_error("EOF encountered while reading Tag start");
	char c = static_cast<char>(buffer->sbumpc());
	if (c!='<')
		throw std::runtime_error("tag expected");

	string sb;
	sb += c;

	do
	{
		if (buffer->sgetc()==EOF)
			throw std::runtime_error("EOF encountered while reading Tag");
		c = static_cast<char>(buffer->sbumpc());
		sb += c;
	}
    while (c != '>');

    return sb;
}

bool needsDecoding(const std::string& source)
{
	return source.find('&') != std::string::npos;
}

string decodeString(const std::string& source) 
{
	string result;
	size_t pos = 0;
	while (pos<source.size()) {
		char c = source[pos++];
		if (c=='&')
		{
			// Get the entity between & and ;
			string entity;
			while (pos<source.size())
			{
				c = source[pos++];
				if (c!=';')
					entity += c;
				else
					break;
			}

			if (entity=="lt")
				result += '<';
			else if (entity=="gt")
				result += '>';
			else if (entity=="amp")
				result += '&';
			else if (entity=="quot")
				result += '"';
			else if (entity=="apos")
				result += '\'';
			else if (entity=="nbsp")
				// TODO: Should we support this???  Some files have it, I'm afraid
				result += ' ';
			else if (entity[0] == '#')
			{
				if (entity.size()<2)
					throw std::runtime_error("bad XML. No character code following #");

				// TODO: Handle hexidecimal characters
				if (entity[1]=='x')
					throw std::runtime_error("Unable to parse hexidecimal character code");

				result += atoi(entity.substr(1).c_str());
			}
			else
			{
				// TODO: Should we throw a parse error???
				RBXASSERT(false);
				result += "&" + entity + ";";
			}
		}
		else
			result += c;
    }

	return result;
}

string TextXmlParser::readText(bool decode) 
{
	// <![CDATA[
	std::istream tmp(buffer);
	size_t curPos = tmp.tellg();
	char firstNine[9] = { 0 };
	buffer->sgetn(firstNine, 9);
	if (memcmp(firstNine, kCDATA_OPEN, 9) == 0)
	{
		char lastThree[3];
		lastThree[0] = buffer->sbumpc();
		lastThree[1] = buffer->sbumpc();
		lastThree[2] = buffer->sbumpc();
		std::stringstream ss;

		while (true)
		{
			if (buffer->sgetc() == EOF)
				break;

			if (memcmp(lastThree, kCDATA_CLOSE, 3) == 0)
				break;

			ss << lastThree[0];
			lastThree[0] = lastThree[1];
			lastThree[1] = lastThree[2];
			lastThree[2] = buffer->sbumpc();
		}

		// advance to EOF or <
		while (buffer->sgetc() != '<' && buffer->sgetc() != EOF)
			buffer->sbumpc();

		return ss.str();
	}
	else
	{
		tmp.seekg(curPos);
	}

	skipWhitespace();

	string sb;

	while (true)
	{
		const int ch = buffer->sgetc();
		if (ch == EOF)
			break;
		if (static_cast<char>(ch) == '<')
			break;
		sb += buffer->sbumpc();
	}

	// TODO: Optimize this by doing it inline with the above loop
	if (decode && needsDecoding(sb))
		return decodeString(sb);
	else
		return sb;
}

void TextXmlWriter::xmlOrCDataEncodedWrite(std::ostream& stream, const std::string& textStr)
{
	// if the text has a newline and does not have the CDATA close tag, then use cdata to encode
	if ((textStr.find("\n") != std::string::npos) &&
		(textStr.find(kCDATA_CLOSE) == std::string::npos))
	{
		stream << kCDATA_OPEN << textStr.c_str() << kCDATA_CLOSE;
	}
	else
	{
		xmlEncodedWrite(stream, textStr);
	}
}

void TextXmlWriter::xmlEncodedWrite(std::ostream& stream, const std::string& textStr) 
{
	const char* text = textStr.c_str();
	size_t l = textStr.size();

	for (size_t i = 0; i < l; ++i) {
		// very primitive encoding of special characters!
		unsigned char c = *text++;
		if (c=='<')
			stream << "&lt;";
		else if (c=='>')
			stream << "&gt;";
		else if (c=='&')
			stream << "&amp;";
		else if (c=='"')
			stream << "&quot;";
		else if (c=='\'')
			stream << "&apos;";
		else if ((c<32 && c!=0xA && c!=0xD) || c>126)
		{
			char num[8];
			sprintf(num, "&#%d;", c);
			stream << num;
		}
		else
			stream << c;
	}
}

void TextXmlWriter::writeOpenTag(const XmlElement* element, int depth)
{
	for (int i = 0; i < depth; ++i)
		stream << '\t';

	stream << '<' << element->getTag().toString();

	const XmlAttribute* attribute = element->getFirstAttribute();
	while (attribute) {
		stream << ' ' << attribute->getTag().toString() << "=\"";
		xmlEncodedWrite(stream, attribute->toString(this));
		stream << '\"';
		attribute = element->getNextAttribute(attribute);
	}

	stream << '>';
}

void TextXmlWriter::writeCloseTag(const XmlElement* element, int depth) 
{
	for (int i = 0; i < depth; ++i)
		stream << '\t';
	stream << "</" << element->getTag().toString() << '>';
}

string TextXmlParser::removeTag(const string& contents, int& index)
{
	RBXASSERT (contents[0] == '<');
	int start = 1;
	while (myIsWhiteSpace(contents[start]) && (start < (int)contents.length()))
		start++;

	index = start;
	while (!myIsWhiteSpace(contents[index]) && contents[index] != '>' && (index < (int)contents.length()))
		index++;
	RBXASSERT(index > start);

	return contents.substr(start, index - start);
}

static bool findNextToken(const string& contents, int& index)
{
	// Find the first non-whitespace character starting at index
	const char* c = contents.c_str() + index;
	while (true)
	{
		RBXASSERT(*c);

		if (*c == '>')
			return false;
		if (*c == 0)	// for safety
			return false;
		if (!myIsWhiteSpace(*c))
			return true;

		index++;
		c++;
	}
}

XmlElement* TextXmlParser::parseAttributes(const string& currentTag)
{
	int index = 0;
	const string tagName(removeTag(currentTag, index));
	XmlElement* newElement = new XmlElement(XmlTag::lookup(tagName));

	while (::findNextToken(currentTag, index)) 
	{
		const size_t equal = currentTag.find('=', index);
		const string tag(currentTag.substr(index, equal - index));

		// if we didn't find an equals, we need to exit or we can potentially be 
		//  stuck in an infinite loop and continuously generate attributes
		if( equal == std::string::npos )
			throw std::runtime_error("Unable to parse XML attributes. '=' not found");
			
		const int firstQuote = equal + 1;
		const int lastQuote = currentTag.find('\"', firstQuote + 1);

		if (lastQuote == std::string::npos)
			throw std::runtime_error("Unable to parse XML attributes. '\"' not found");

		string text(currentTag.substr(firstQuote + 1, lastQuote - firstQuote - 1));
		if (needsDecoding(text))
			text = decodeString(text);

		index = lastQuote + 1;

		newElement->addAttribute(XmlTag::lookup(tag), text);
	}

	return newElement;
}


XmlParser::XmlParser(std::streambuf* buffer)
:buffer(buffer)
{
}


/*****

While Not EOF(Input XML Document)
  Tag = Next tag from the document
  LastOpenTag = Top tag in Stack

  If Tag is an open tag
    Add Tag as the child of LastOpenTag
    Push Tag in Stack
  Else
    // Tag is a close tag
    If Tag is the matching close tag of LastOpenTag
      Pop Stack
      If Stack is empty
        Parse is complete
      End If
    Else
      // Invalid tag nesting
      Report error
    End If
  End If
End While

The centerpiece of this algorithm is the tag stack, which keeps track of the open tags 
that have been taken from the input document but have not been matched by their close tags.
The top item on the stack is always the last open tag encountered. 

Except for the first tag, each new open tag will be a child tag of the last open tag. 
So the parser adds the new tag as a child of the last open tag and then pushes it onto 
the stack, where it becomes the new last open tag. On the other hand, if the input tag 
is a close tag, it has to match the last open tag. A non-matching close tag indicates 
an XML syntax error based on the proper-nesting rule. When the close tag matches the last 
open tag, the parser pops the last open tag from the stack because parsing for that tag is
complete. This process continues until the stack is empty. At that point, you're finished 
parsing the entire document. Listing 2 shows the entire source code for the 
SimpleDOMParser.parse method. 
**/

std::auto_ptr<XmlElement> TextXmlParser::parse()
{
	if (buffer->sgetc()==EOF)
		throw std::runtime_error("TextXmlParser::parse empty file");

	bool firstTimeThrough = true;
	while (true) {
		std::string currentTag;
		
		if (firstTimeThrough)
		{
			firstTimeThrough = false;
			// Skip the <?> tag
			currentTag = readFirstTag();
			if (currentTag.substr(0,2)=="<?")
				continue;
		}
		else
			currentTag = readTag();		// finds the text between "<" and ">" inclusive

		XmlElement* currentElement = elements.empty() ? NULL : elements.top();
		
		if (isCloseTag(currentTag.c_str())) {

			// no open tag
			if (currentElement == NULL)
				throw RBX::runtime_error("TextXmlParser::parse - Got close tag %s without open tag.", currentTag.c_str());

			// pop up the previous open tag
			elements.pop();

	        if (elements.empty()) {
				// document processing is over
				RBXASSERT(currentElement!=NULL);
				return std::auto_ptr<XmlElement>(currentElement);
			} 
		} 
		else {
			XmlElement* newElement = parseAttributes(currentTag);
			elements.push(newElement);

			// special-case the "Content" tag
			// TODO: Move this into the Reflection::Property reading code instead?
			if (newElement->getTag()==RBX::Reflection::Type::singleton<RBX::ContentId>().tag)
			{
				XmlAttribute* xsinil = newElement->findAttribute(name_xsinil);
				bool val; //Note: 'nil' is already define on OSX
				if (xsinil!=NULL && xsinil->getValue(val) && val)
				{
					// no data
				}
				else
				{
					if (this->readText(false)!="")	
					{
						// Old files might include an integer "ContentId" rather than a sub-element
					}
					else
					{
						string contentChild = readTag();		// finds the text between "<" and ">" inclusive
						string tagName = contentChild.substr(1, contentChild.length()-2);
						
						if (tagName.compare(0, 6, "binary") == 0)	// The binary tag may have attributes, so we have to compare a substring
						{
							// We no longer support binary content
							RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "Not reading binary data");
							readText(false);
							newElement->setValue(RBX::ContentId());
						}
						else if (tag_hash==tagName)
						{
							// We no longer support binary content
							readText(false);
							newElement->setValue(RBX::ContentId());
						}
						else if (tagName.compare(0, 3, "url") == 0)
						{
							newElement->setValue(RBX::ContentId(this->readText(true).c_str()));
						}
						else if (tag_null==tagName)
						{
							newElement->setValue(RBX::ContentId());
						}
						else
							throw RBX::runtime_error("TextXmlParser::parse - Unknown tag '%s'.", tagName.substr(0, 32).c_str());

						std::string closingTag = readTag();	// closing tag
						if (!isCloseTag(closingTag.c_str()))
							throw RBX::runtime_error("TextXmlParser::parse - '%s' should be a closing tag", closingTag.substr(0, 32).c_str());
					}
				}
			}
	        // read the text between the open and close tag
			else
				newElement->setValue(readText(true));
	
	        // add new element as a child element of
	        // the current element
	        if (currentElement != NULL)
				currentElement->addChild(newElement);

			if (endsWithClose(currentTag))
				// pop up this tag
				elements.pop();
        }
	}
}


/*
	Write open tag(depth)
	Write text(0)
	If !children {
		write close tag(0)
	}
	else {
		CR

		depth++

		write each child(depth)

		depth--

		write close tag(depth)
	}

	CR

	return writer.data();
*/
		
XmlWriter::XmlWriter(std::ostream& stream)
: stream(stream)
{
}

void TextXmlWriter::serialize(const XmlElement* xmlNode)
{
	serialize(xmlNode, 0);
}


void TextXmlWriter::serializeNode(const XmlElement* xmlNode, int depth) 
{
	// Special handling for RBX::ContentId
	// TODO: move to Reflection::Property?
	if (xmlNode->isValueType<RBX::ContentId>())
	{
		RBX::ContentId contentId;
		xmlNode->getValue(contentId);

		writeOpenTag(xmlNode, depth);

		if (xmlNode->findAttribute(name_xsinil)!=NULL)
		{
			// Just write out the tag and nothing inside
			return;
		}

		if (contentId.isNull()) 
		{
			stream << "<null></null>";
		} 
		else
		{
			stream << "<url>";
			xmlEncodedWrite(stream, contentId.c_str());
			stream << "</url>";
		}
	
		return;	// done!
	}

	writeOpenTag(xmlNode, depth);
	xmlOrCDataEncodedWrite(stream, xmlNode->toString(this));			// may not have text
}


void TextXmlWriter::serialize(const XmlElement* xmlNode, int depth) 
{
	if (xmlNode) {
		
		serializeNode(xmlNode, depth);

		const XmlElement* child = xmlNode->firstChild();

		if (child!=NULL) {
			do {
				stream << '\n';
				serialize(child, depth+1);
			} while ((child = xmlNode->nextChild(child)));
			stream << '\n';

			writeCloseTag(xmlNode, depth);
		}
		else {
			// no children - write on same line	
			writeCloseTag(xmlNode, 0);
		}
	}
}
	
