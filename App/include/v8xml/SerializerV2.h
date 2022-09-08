#pragma once

#include <vector> 
#include <list> 
#include <string>
#include "V8Xml/XmlElement.h"
#include "V8tree/instance.h"
#include "V8Xml/Reference.h"
#include "Util/Object.h"
//typedef std::vector<class PVItem*> ModelList;



namespace RBX {
	class DataModel;
}


#if defined(G3D_WIN32)
#pragma warning(push)
#pragma warning(disable:4290)
#endif

// TODO: Refactor: Call this RBX::DOM or something
class SerializerV2 {
protected:
	int schemaVersionLoading;
public:
	static const int CURRENT_SCHEMA_VERSION = 4;

	// writing:
	static XmlElement* newRootElement();
	static XmlElement* newRootElement(const std::string& type);

	// reading:
	void loadInstancesFromText(const XmlElement* root, RBX::Instances& result);
	
	// Until DataModel becomes an Instance and it can handle "globals" like Workspace, we need to treat
	// it specially during reads:
	void load(std::istream& stream, RBX::DataModel* dataModel);
	void loadInstances(std::istream& stream, RBX::Instances& result);

private:
	void loadXML(std::istream& stream, RBX::DataModel* dataModel);
	void loadInstancesXML(const XmlElement* root, RBX::Instances& result, RBX::IReferenceBinder& binder, RBX::CreatorRole creatorRole);
	shared_ptr<RBX::Instance> loadInstanceXML(const XmlElement* itemElement, RBX::IReferenceBinder& binder, RBX::CreatorRole creatorRole);
};

#if defined(G3D_WIN32)
#pragma warning(pop)
#endif


namespace RBX
{
	// MergeBinder is used to merge an XML stream into an existing world (for undo/redo operations)
	class MergeBinder : public IReferenceBinder
	{
		struct IDREFItem {
			const IIDREF* idref;
			Reflection::DescribedBase* propertyOwner;
			RBX::InstanceHandle value;
		};

		// TODO: vector or list???
		std::vector<IDREFItem> deferredIDREFItems;

	public:
		virtual void announceID(const XmlNameValuePair* valueID, Reflection::DescribedBase* target) {
			processID(valueID, target);			
		}
		virtual void announceIDREF(const XmlNameValuePair* valueIDREF, Reflection::DescribedBase* propertyOwner, const IIDREF* idref) {
			bool processedIDREF = processIDREF(valueIDREF, propertyOwner, idref);
			RBXASSERT(processedIDREF);
		}

		virtual bool resolveRefs() {
			for (std::vector<IDREFItem>::iterator iter = deferredIDREFItems.begin(); iter!=deferredIDREFItems.end(); ++iter)
			{
				assign(iter->idref, iter->propertyOwner, iter->value);


			}
			deferredIDREFItems.clear();

			return true;
		}

	protected:
		virtual bool processID(const XmlNameValuePair* valueID, Reflection::DescribedBase* source) {
			RBX::InstanceHandle h;
			if (valueID->getValue(h)) {
				h.linkTo(shared_from(source));
				return true;
			} else if (valueID->isValueEqual(&value_IDREF_nil))
				return true;		// "nil" means "skip this value", like xsi:nil does for other types
			else
				return false;
		}
		virtual bool processIDREF(const XmlNameValuePair* valueIDREF, Reflection::DescribedBase* propertyOwner, const IIDREF* idref) {
			RBX::InstanceHandle value;
			if (valueIDREF->getValue(value)) {
				if (!value.empty())
					assign(idref, propertyOwner, value);
				else {
					IDREFItem item = {idref, propertyOwner, value};
					deferredIDREFItems.push_back(item);
				}
				return true;
			} else if (valueIDREF->isValueEqual(&value_IDREF_nil))
				return true;		// "nil" means "skip this value", like xsi:nil does for other types
			else
				return false;
		}
	};
}





