#include "stdafx.h"

#include "V8Xml/WebParser.h"
#include "V8Xml/XmlSerializer.h"
#include "V8Xml/Serializer.h"
#include "rbx/make_shared.h"
#include "Util/SafeToLower.h"
#include <boost/property_tree/ptree.hpp>

#include <boost/property_tree/json_parser.hpp>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

namespace RBX
{
	bool WebParser::parseWebListResponse(std::istream& stream, RBX::Reflection::ValueArray& result)
	{
		TextXmlParser machine(stream.rdbuf());
		std::auto_ptr<XmlElement> root(machine.parse());
		if(root->getTag() == tag_WebList)
		{
			return loadList(root.get(), result);
		}
		return false;
	}
	bool WebParser::parseWebGenericResponse(std::istream& stream, RBX::Reflection::Variant& result)
	{
		TextXmlParser machine(stream.rdbuf());
		std::auto_ptr<XmlElement> root(machine.parse());
		return parseWebGenericResponse(root.get(), result);
	}
	bool WebParser::parseWebGenericResponse(const XmlElement* root, RBX::Reflection::Variant& result)
	{
		if (root->getTag() == tag_WebTable) 
		{
			shared_ptr<Reflection::ValueMap> table(new Reflection::ValueMap());
			if(loadTable(root, *table)){
				result = shared_ptr<const RBX::Reflection::ValueMap>(table);
				return true;
			}
		}
		else if(root->getTag() == tag_WebList)
		{
			shared_ptr<Reflection::ValueArray> list(rbx::make_shared<Reflection::ValueArray>());
			if(loadList(root, *list)){
				result = shared_ptr<const RBX::Reflection::ValueArray>(list);
				return true;
			}
		}
		else if(root->getTag() == tag_WebValue)
		{
			if(loadValue(root, result)){
				return true;
			}
		}
		return false;
	}
	bool WebParser::loadList(const XmlElement* listElement, RBX::Reflection::ValueArray& result)
	{
		const XmlElement* childEntry = listElement->findFirstChildByTag(tag_WebValue);
		while (childEntry)
		{
			Reflection::Variant value;
			if(loadValue(childEntry, value)){
				result.push_back(value);
			}
			else{
				return false;
			}
			childEntry = listElement->findNextChildWithSameTag(childEntry);
		}
		return true;
	}
	bool WebParser::loadTable(const XmlElement* tableElement, RBX::Reflection::ValueMap& result)
	{
		const XmlElement* childEntry = tableElement->findFirstChildByTag(tag_WebEntry);
		while (childEntry)
		{
			std::string key;
			Reflection::Variant value;
			if(loadEntry(childEntry, key, value)){
				result[key] = value;
			}
			else{
				return false;
			}
			childEntry = tableElement->findNextChildWithSameTag(childEntry);
		}
		return true;
	}
	bool WebParser::loadValue(const XmlElement* valueElement, RBX::Reflection::Variant& value)
	{
		if(const XmlElement* tableElement = valueElement->findFirstChildByTag(tag_WebTable)){
			shared_ptr<Reflection::ValueMap> table(new Reflection::ValueMap());
			if(loadTable(tableElement, *table)){
				value = shared_ptr<const RBX::Reflection::ValueMap>(table);
				return true;
			}
		}
		else if((tableElement = valueElement->findFirstChildByTag(tag_WebList))){
			shared_ptr<Reflection::ValueArray> list(rbx::make_shared<Reflection::ValueArray>());
			if(loadList(tableElement, *list)){
				value = shared_ptr<const RBX::Reflection::ValueArray>(list);
				return true;
			}
		}
		else{
			if(const XmlAttribute* typeAttribute = valueElement->findAttribute(tag_WebType))
			{
				std::string type;
				if(typeAttribute->getValue(type))
				{
					if(type == "boolean")
					{
						bool boolResult;
						if(valueElement->getValue(boolResult)){
							value = boolResult;
							return true;
						}
					}
					else if(type == "string")
					{
						std::string stringResult;
						if(valueElement->getValue(stringResult)){
							value = stringResult;
							return true;
						}
					}
					else if(type == "number")
					{
						double doubleResult;
						if(valueElement->getValue(doubleResult)){
							value = doubleResult;
							return true;
						}
					}
					else if(type == "integer")
					{
						int intResult;
						if (valueElement->getValue(intResult))
						{
							value = intResult;
							return true;
						}
					}
					else if(type == "instance")
					{
						if(const XmlElement* robloxRoot = valueElement->findFirstChildByTag(tag_roblox)){
							Serializer serializer;
							Instances instances;
							serializer.loadInstancesFromText(robloxRoot, instances);
							if(instances.size() == 1)
							{
								value = instances[0];
								return true;
							}
						}
					}
					return false;
				}
			}

			//fallback to legacy parsing
			if(valueElement->isValueType<std::string>()){
				std::string stringResult;
				if(valueElement->getValue(stringResult)){
					std::string lowerStringResult = stringResult;
					safeToLower(lowerStringResult);
					if(lowerStringResult == "true")
						value = true;
					else if(lowerStringResult == "false")
						value = false;
					else
						value = stringResult;
					return true;
				}
			}
		}
		return false;
	}
	bool WebParser::loadEntry(const XmlElement* entryElement, std::string& key, RBX::Reflection::Variant& value)
	{
		bool gotKey = false;
		bool gotValue = false;
		if(const XmlElement* keyElement = entryElement->findFirstChildByTag(tag_WebKey)){
			if(keyElement->isValueType<std::string>()){
				if(keyElement->getValue(key)){
					gotKey = true;
				}
			}
		}

		if(const XmlElement* valueElement = entryElement->findFirstChildByTag(tag_WebValue)){
			gotValue = loadValue(valueElement, value);
		}
		return gotKey && gotValue;
	}

	bool legacyPopulateValueTableFromPtree(const boost::property_tree::ptree& propTree, shared_ptr<Reflection::ValueTable>& valueTable)
	{
		try
		{
			if(propTree.size() > 0)
			{
				boost::property_tree::ptree::const_iterator end = propTree.end();
				int count = 1; // standard to start at 1 for lua
				for (boost::property_tree::ptree::const_iterator it = propTree.begin(); it != end; ++it)
				{
					// make sure we aren't overwriting the empty key in the table
					std::string key = it->first;
					if(key.empty())
					{
						std::ostringstream convert;
						convert << count;
						std::string countString = convert.str();
						key = countString;
					}

					// unfortunately this is the easiest way to get types from ptree
					if(it->second.get_value_optional<int>())
						(*valueTable)[key] = it->second.get_value<int>();
					else if(it->second.get_value_optional<bool>())
						(*valueTable)[key] = it->second.get_value<bool>();
					else if(it->second.get_value_optional<std::string>())
					{
						std::string stringValue = it->second.get_value<std::string>();
						if(!stringValue.empty())
							(*valueTable)[key] = stringValue;
					}

					if(it->second.size() > 0) // we have a nested table, get the info
					{
						shared_ptr<Reflection::ValueTable> subMap(rbx::make_shared<Reflection::ValueTable>());
						if(!legacyPopulateValueTableFromPtree(it->second,subMap))
							return false;

						Reflection::Variant variantValue = shared_ptr<const RBX::Reflection::ValueTable>(subMap);
						(*valueTable)[key] = variantValue;
					}

					count++;
				}
			}
		}
		catch(std::exception const&)
		{
			return false;
		}

		return true;
	}

	Reflection::Variant populateValueTableFromPtree(const boost::property_tree::ptree& propTree)
	{
		if(propTree.size() > 0)
		{
			boost::property_tree::ptree::const_iterator end = propTree.end();
			int count = 1; // standard to start at 1 for lua

			Reflection::Variant value;
			shared_ptr<Reflection::ValueTable> subMap;
			shared_ptr<Reflection::ValueArray> subArray;


			for (boost::property_tree::ptree::const_iterator it = propTree.begin(); it != end; ++it)
			{
				// make sure we aren't overwriting the empty key in the table
				std::string key = it->first;

				// unfortunately this is the easiest way to get types from ptree
				if(it->second.get_value_optional<int>())
					value = it->second.get_value<int>();
				else if(it->second.get_value_optional<bool>())
					value = it->second.get_value<bool>();
				else if(it->second.get_value_optional<std::string>())
				{
					std::string stringValue = it->second.get_value<std::string>();
					if(!stringValue.empty())
						value = stringValue;
				}

				if(it->second.size() > 0) // we have a nested table, get the info
				{
					value = populateValueTableFromPtree(it->second);
				}

				if(key.empty())
				{
					RBXASSERT(!subMap);
					if(!subArray)
						subArray = rbx::make_shared<Reflection::ValueArray>();
					subArray->push_back(value);
				}
				else
				{
					RBXASSERT(!subArray);
					if(!subMap)
						subMap = rbx::make_shared<Reflection::ValueTable>();
					(*subMap)[key] = value;
				}

				count++;
			}

			return subMap ? 
				Reflection::Variant(shared_ptr<const Reflection::ValueTable>(subMap)) :
				Reflection::Variant(shared_ptr<const Reflection::ValueArray>(subArray));
		}
		else 
		{
			return Reflection::Variant();
		}
	}
	
	Reflection::Variant populateValueTableFromRapidJson(rapidjson::Value& node)
	{
		Reflection::Variant v;
		if(node.IsInt())
			v = node.GetInt();
		else if(node.IsNumber())
			v = node.GetDouble();
		else if(node.IsBool())
			v = node.GetBool();
		else if(node.IsString())
			v = std::string(node.GetString());
		else if(node.IsObject())
		{
			shared_ptr<Reflection::ValueTable> subMap = rbx::make_shared<Reflection::ValueTable>();
			for(rapidjson::Value::MemberIterator it = node.MemberBegin(); it != node.MemberEnd(); ++it)
			{
				Reflection::Variant subValue = populateValueTableFromRapidJson(it->value);
				(*subMap)[it->name.GetString()] = subValue;
			}
			v = shared_ptr<const Reflection::ValueTable>(subMap);
		} 
		else if(node.IsArray())
		{
			shared_ptr<Reflection::ValueArray> subArray = rbx::make_shared<Reflection::ValueArray>();
			for(rapidjson::Value::ValueIterator it = node.Begin(); it != node.End(); ++it)
			{
				Reflection::Variant subValue = populateValueTableFromRapidJson(*it);
				subArray->push_back(subValue);
			}
			v = shared_ptr<const Reflection::ValueArray>(subArray);
		}
		return v;
	}

	bool WebParser::parseJSONObject(const std::string& rawWebResponse, Reflection::Variant& result)
	{
		rapidjson::Document root;
		root.Parse<0>(rawWebResponse.c_str());

		if(root.HasParseError())
			return false;

		RBXASSERT(root.IsObject() || root.IsArray());

		result = populateValueTableFromRapidJson(root);

		return true;
	}

	bool checkStringForASCII(const std::string& value)
	{
		for(std::string::const_iterator it = value.begin(); it != value.end(); ++it)
		{
			if(*it < 0)
				return false;
		}

		return true;
	}

	bool writeToRapidJSON(const Reflection::Variant& value, rapidjson::Value& node, WebParser::NonJSONBehavior skip,
		rapidjson::Document::AllocatorType& allocator)
	{
		if(value.isVoid())
			node.SetNull();
		else if(value.isFloat())
			node.SetDouble(value.get<double>());
		else if(value.isType<bool>())
			node.SetBool(value.get<bool>());
		else if(value.isNumber())
			node.SetInt(value.get<int>());
		else if(value.isString())
		{
			std::string strValue = value.get<std::string>();
			if(!checkStringForASCII(strValue))
				return false;

			node.SetString(strValue.c_str(),allocator);
		}
		else if(value.isType<shared_ptr<const Reflection::ValueTable> >())
		{
			node.SetObject();
			shared_ptr<const Reflection::ValueTable> table = value.get<shared_ptr<const Reflection::ValueTable> >();
			for(Reflection::ValueTable::const_iterator it = table->begin(); it != table->end(); ++it)
			{
				rapidjson::Value subNode;
				if(!writeToRapidJSON(it->second, subNode, skip, allocator))
					return false;
				node.AddMember(it->first.c_str(), subNode, allocator);
			}
		} 
		else if(value.isType<shared_ptr<const Reflection::ValueArray> >())
		{
			node.SetArray();
			shared_ptr<const Reflection::ValueArray> array = value.get<shared_ptr<const Reflection::ValueArray> >();
			for(Reflection::ValueArray::const_iterator it = array->begin(); it != array->end(); ++it)
			{
				rapidjson::Value subNode;
				if(!writeToRapidJSON(*it, subNode, skip, allocator))
					return false;

				node.PushBack(subNode, allocator);
			}
		}
		else if(skip == WebParser::SkipNonJSON)
		{
			node.SetNull();
		}
		else
		{
			return false;
		}

		return true;
	}

	bool WebParser::writeJSON(const Reflection::Variant& value, std::string& result, NonJSONBehavior skip)
	{
		rapidjson::Document root;
		if (!writeToRapidJSON(value, root, skip, root.GetAllocator()))
			return false;

		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		root.Accept(writer);

		result = buffer.GetString();

		return true;
	}

	bool WebParser::parseJSONTable(const std::string& rawWebResponse, shared_ptr<const Reflection::ValueTable>& valueTable)
	{
		Reflection::Variant result;
		if(!parseJSONObject(rawWebResponse, result))
			return false;

		if(!result.isType<shared_ptr<const Reflection::ValueTable> >())
			return false;

		valueTable = result.cast<shared_ptr<const RBX::Reflection::ValueTable> >();
		return true;

	}

	bool WebParser::parseJSONArray(const std::string& rawWebResponse, shared_ptr<const Reflection::ValueArray>& valueArray)
	{
		Reflection::Variant result;
		if(!parseJSONObject(rawWebResponse, result))
			return false;

		if(!result.isType<shared_ptr<const Reflection::ValueArray> >())
			return false;

		valueArray = result.cast<shared_ptr<const RBX::Reflection::ValueArray> >();
		return true;

	}

	bool WebParser::legacyParseWebJSONResponse(std::stringstream& rawWebResponse, shared_ptr<const Reflection::ValueTable>& valueTable)
	{
		// if the stringstream has nothing to be read, return
		if (rawWebResponse.rdbuf()->in_avail() == 0)
			return false;

		// parse the raw stream into a data structure we can read from
		boost::property_tree::ptree propTree;
		try
		{
			boost::mutex::scoped_lock(JSONmutex);
			boost::property_tree::read_json(rawWebResponse, propTree);
		}
		catch (std::exception const&)
		{
			return false;
		}

		shared_ptr<Reflection::ValueTable> table(rbx::make_shared<Reflection::ValueTable>());
		bool result = legacyPopulateValueTableFromPtree(propTree, table);

		if(result)
			valueTable = table;

		return result;	
	}

	bool WebParser::ptreeParseWebJSONResponse(std::stringstream& rawWebResponse, shared_ptr<const Reflection::ValueTable>& valueTable)
	{
		// if the stringstream has nothing to be read, return
		if (rawWebResponse.rdbuf()->in_avail() == 0)
			return false;

		Reflection::Variant result;
		// parse the raw stream into a data structure we can read from
		boost::property_tree::ptree propTree;
		try
		{
			{
				boost::mutex::scoped_lock(JSONmutex);
				boost::property_tree::read_json(rawWebResponse, propTree);
			}
			
			result = populateValueTableFromPtree(propTree);
		}
		catch (std::exception const&)
		{
			return false;
		}

		if(result.isType<shared_ptr<const Reflection::ValueTable> >())
		{
			valueTable = result.get<shared_ptr<const Reflection::ValueTable> >();
			return true;
		}

		return false;	
	}

}
