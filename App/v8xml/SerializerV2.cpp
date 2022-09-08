/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8Xml/SerializerV2.h"
#include "V8Xml/XmlSerializer.h"
#include "V8DataModel/DataModel.h"
#include "v8datamodel/contentprovider.h"
#include "rbx/Debug.h"
#include "util/standardout.h"

#include "v8xml/SerializerBinary.h"

#include <map>

#ifdef _WIN32
   using std::mem_fun;
#else
#include <ext/functional>
   using __gnu_cxx::mem_fun;
#endif

using std::string;
using std::vector;

using namespace RBX;

// An implementation of IReferenceBinder
class ArchiveBinder : public RBX::MergeBinder
{
private:
	typedef RBX::MergeBinder Super;
	// A map of temporary IDs to Referents
	std::map<std::string, InstanceHandle> idMap;

	struct IDREFBinding {
		const XmlNameValuePair* valueIDREF;
		Reflection::DescribedBase* propertyOwner;
		const IIDREF* idref;
	};
	std::list<IDREFBinding> idrefBindings;

public:

	virtual bool processID(const XmlNameValuePair* valueID, Reflection::DescribedBase* source) {
		if (!Super::processID(valueID, source)) {
			std::string s;
			bool foundString = valueID->getValue(s);
			RBXASSERT(foundString);
//			RBXASSERT(idMap.find(s)==idMap.end());		// This is in BugHost for Erik - implies we are writing references multiple times
														// See this file to make the assert pop:
														// game.Workspace:insertContent("http://roblox.com/asset/?id=2286288")

			idMap[s].linkTo(shared_from(source));
			if (source)
			{
				source->setXmlId(s);
			}
		}
		return true;
	}
	virtual bool processIDREF(const XmlNameValuePair* valueIDREF, Reflection::DescribedBase* propertyOwner, const IIDREF* idref) {
		if (!Super::processIDREF(valueIDREF, propertyOwner, idref)) {
			IDREFBinding binding = {valueIDREF, propertyOwner, idref};
			idrefBindings.push_back( binding );
		}
		return true;
	}

	bool resolveIDREF(IDREFBinding binding)
	{
		std::string s;
		bool foundString = binding.valueIDREF->getValue(s);
		RBXASSERT(foundString);

		// The following 3 cases should have been handled during the processIDREF phase
		RBXASSERT (value_IDREF_nil!=s);
		RBXASSERT (value_IDREF_null!=s);
		RBXASSERT (s!="");

		// Find the InstanceHandle belonging to the requested ID
		std::map<std::string, InstanceHandle>::iterator iter = idMap.find(s);
		if (iter!=idMap.end())
		{
			// TODO: should we give the handle  over to valueIDREF?
			assign(binding.idref, binding.propertyOwner, iter->second);
			return true;
		}

		// Unable to find the requested InstanceHandle
		assign(binding.idref, binding.propertyOwner, InstanceHandle(NULL));
		return false;
	}

	bool resolveRefs()
	{
		int count = 0;
		
		count += count_if(
			idrefBindings.begin(), 
			idrefBindings.end(), 
			std::bind1st(mem_fun(&ArchiveBinder::resolveIDREF),this)
		);

		return Super::resolveRefs() && (count == idrefBindings.size());
	}
};

void SerializerV2::load(std::istream& stream, RBX::DataModel* dataModel)
{
	// See file format spec in CWorkspace::Save()
	char header[8];
	if (!stream.read(header, 8).good())
		throw std::runtime_error("SerializerV2::load can't read header");
	
    stream.clear();
    stream.seekg (0, std::ios::beg);
    
    if (memcmp(header, SerializerBinary::kMagicHeader, 8) == 0)
    {
        // read the binary content
        SerializerBinary::deserialize(stream, dataModel);
    }
    else
    {
        // read the XML content
        loadXML(stream, dataModel);
    }
}

void SerializerV2::loadInstances(std::istream& stream, RBX::Instances& result)
{
	char header[8];
	if (!stream.read(header, 8).good())
		throw std::runtime_error("SerializerV2::loadInstances can't read header");

    stream.clear();
    stream.seekg(0, std::ios::beg);
    
    if (memcmp(header, SerializerBinary::kMagicHeader, 8) == 0)
    {
        // read the binary content
        SerializerBinary::deserialize(stream, result);
    }
    else
    {
        // read the XML content
        TextXmlParser machine(stream.rdbuf());
        std::auto_ptr<XmlElement> root(machine.parse());

        ArchiveBinder binder;
        loadInstancesXML(root.get(), result, binder, RBX::SerializationCreator);
    }
}

void SerializerV2::loadXML(std::istream& stream, RBX::DataModel* dataModel)
{
	TextXmlParser machine(stream.rdbuf());
	std::auto_ptr<XmlElement> root(machine.parse());
	
	if (root->getTag() == tag_roblox) 
	{
		if(const XmlAttribute* version = root->findAttribute(tag_version))
        {
            if (!version || !version->getValue(schemaVersionLoading))
            {
                throw std::runtime_error("SerializerV2::loadXML no version number");
            }
            else if (schemaVersionLoading<4)
            {
                throw std::runtime_error("SerializerV2::loadXML schemaVersionLoading<4");
            }
            else
			{
                ArchiveBinder binder;

                dataModel->readChildren(root.get(), binder, SerializationCreator);

                binder.resolveRefs();
			}
        }
	}
	else
	{
		schemaVersionLoading = 1;
		throw std::runtime_error("SerializerV2::loadXML ill-formed XML. No Roblox tag");
	}

	// Should we need to do this???
	dataModel->setDirty(false);
}

shared_ptr<Instance> SerializerV2::loadInstanceXML(const XmlElement* itemElement, IReferenceBinder& binder, CreatorRole creatorRole)
{
	const RBX::Name* className = NULL;
	if (itemElement->findAttributeValue(tag_class, className))
	{
		shared_ptr<Instance> instance = Creatable<Instance>::createByName(*className, RBX::SerializationCreator);
		if (instance)
		{
			instance->read(itemElement, binder, creatorRole);
			return instance;
		}
		else
			StandardOut::singleton()->printf(MESSAGE_WARNING, "Unknown object class \"%s\" while reading XML", className ? className->c_str() : "");
	}
	return shared_ptr<Instance>();
}

void SerializerV2::loadInstancesFromText(const XmlElement* root, Instances& result)
{
	ArchiveBinder binder;
	loadInstancesXML(root, result, binder, RBX::SerializationCreator);
}

void SerializerV2::loadInstancesXML(const XmlElement* root, Instances& result, IReferenceBinder& binder, CreatorRole creatorRole)
{
	// TODO: This code has a lot in common with Instance::readChildren and with SerializerV2::merge
	//       Find a way of combining these code chunks
	bool v4model = false;

	if (root->getTag() == tag_roblox) {
		const XmlAttribute* version = root->findAttribute(tag_version);
		if (version!=NULL && version->getValue(schemaVersionLoading) && schemaVersionLoading >= 4) {
			v4model = true;

			const XmlElement* childElement = root->findFirstChildByTag(tag_Item);
			while (childElement)
			{
				if(shared_ptr<Instance> instance = loadInstanceXML(childElement, binder, creatorRole))
					result.push_back(instance);
				childElement = root->findNextChildWithSameTag(childElement);
			}
			binder.resolveRefs();
			// TODO: ASSERT
			//RBXASSERT(resolvedBindings);
		}
	}
};



XmlElement* SerializerV2::newRootElement()
{
	return newRootElement("");	
}
XmlElement* SerializerV2::newRootElement(const std::string& type)
{
	static const XmlTag& tag_xmlnsxmime = Name::declare("xmlns:xmime");

	XmlElement* root = new XmlElement(tag_roblox);
	root->addAttribute(tag_xmlnsxmime, "http://www.w3.org/2005/05/xmlmime");
	root->addAttribute(tag_xmlnsxsi, "http://www.w3.org/2001/XMLSchema-instance");
	root->addAttribute(tag_xsinoNamespaceSchemaLocation, "http://www.roblox.com/roblox.xsd");
	root->addAttribute(tag_version, SerializerV2::CURRENT_SCHEMA_VERSION);
	if(!type.empty()){
		root->addAttribute(tag_assettype, type);
	}

	// Used for schema validation with Roblox.xsd:
	root->addChild(new XmlElement(tag_External, &value_IDREF_null));
	root->addChild(new XmlElement(tag_External, &value_IDREF_nil));

	return root;
}
