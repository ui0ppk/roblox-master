#include "stdafx.h"

#include "v8xml/Serializer.h"
#include "v8xml/XmlSerializer.h"
#include "rbx/Debug.h"

#include "ReflectionMetadata.h"
#ifdef QT_ROBLOX_STUDIO
#include "RobloxSettings.h"
#endif

#include "StringConv.h"

LOGGROUP(ReflectionMetadata)
LOGVARIABLE(ReflectionMetadata, 1);

using namespace RBX;
using namespace Reflection;
			
const char* const Metadata::sReflection	= "ReflectionMetadata";
const char* const Metadata::sClasses	= "ReflectionMetadataClasses";
const char* const Metadata::sItem		= "ReflectionMetadataItem";
const char* const Metadata::sClass		= "ReflectionMetadataClass";
const char* const Metadata::sProperties	= "ReflectionMetadataProperties";
const char* const Metadata::sFunctions	= "ReflectionMetadataFunctions";
const char* const Metadata::sYieldFunctions	= "ReflectionMetadataYieldFunctions";
const char* const Metadata::sEvents		= "ReflectionMetadataEvents";
const char* const Metadata::sCallbacks	= "ReflectionMetadataCallbacks";
const char* const Metadata::sMember		= "ReflectionMetadataMember";
const char* const Metadata::sEnums		= "ReflectionMetadataEnums";
const char* const Metadata::sEnum		= "ReflectionMetadataEnum";
const char* const Metadata::sEnumItem	= "ReflectionMetadataEnumItem";

BoundProp<bool> Metadata::Item::prop_browsable("Browsable", "Reflection", &Metadata::Item::browsable);
BoundProp<bool> Metadata::Item::prop_deprecated("Deprecated", "Reflection", &Metadata::Item::deprecated);
BoundProp<bool> Metadata::Item::prop_backend("IsBackend", "Reflection", &Metadata::Item::backend);
BoundProp<std::string> Metadata::Item::prop_description("summary", "Reflection", &Metadata::Item::description);
BoundProp<double> Metadata::Item::prop_minimum("UIMinimum", "Reflection", &Metadata::Item::minimum);
BoundProp<double> Metadata::Item::prop_maximum("UIMaximum", "Reflection", &Metadata::Item::maximum);

BoundProp<int> Metadata::Class::prop_ExplorerOrder("ExplorerOrder", "Reflection", &Metadata::Class::explorerOrder);
BoundProp<int> Metadata::Class::prop_ExplorerImageIndex("ExplorerImageIndex", "Reflection", &Metadata::Class::explorerImageIndex);
BoundProp<std::string> Metadata::Class::prop_PreferredParent("PreferredParent", "Reflection", &Metadata::Class::preferredParent);
BoundProp<bool> Metadata::Class::prop_Insertable("Insertable", "Reflection", &Metadata::Class::insertable);

RBX_REGISTER_CLASS(Metadata::Reflection);
RBX_REGISTER_CLASS(Metadata::Class);
RBX_REGISTER_CLASS(Metadata::Item);
RBX_REGISTER_CLASS(Metadata::Classes);
RBX_REGISTER_CLASS(Metadata::Functions);
RBX_REGISTER_CLASS(Metadata::YieldFunctions);
RBX_REGISTER_CLASS(Metadata::Member);
RBX_REGISTER_CLASS(Metadata::Properties);
RBX_REGISTER_CLASS(Metadata::Events);
RBX_REGISTER_CLASS(Metadata::Callbacks);
RBX_REGISTER_CLASS(Metadata::Enums);
RBX_REGISTER_CLASS(Metadata::Enum);
RBX_REGISTER_CLASS(Metadata::EnumItem);

void Metadata::Reflection::registerClasses()
{
	// This code ensure that the function isn't optimized away
	Properties::classDescriptor();
}

shared_ptr<Metadata::Reflection> Metadata::Reflection::safe_static_do_get_singleton()
{
	static shared_ptr<Reflection> sing;
	if (!sing)
	{
		sing = RBX::Creatable<RBX::Instance>::create<Reflection>();

#ifdef QT_ROBLOX_STUDIO
		QByteArray xmlFilePathUtf8 = RobloxSettings::getResourcesFolder().toUtf8();
        boost::filesystem::path bfsp = RBX::utf8_decode(std::string(xmlFilePathUtf8.constData(), xmlFilePathUtf8.size()));
		sing->load( bfsp / "ReflectionMetadata.xml" );
#else
		wchar_t buf[MAX_PATH] = {0};
		GetModuleFileNameW(NULL, buf, MAX_PATH);
        boost::filesystem::path exePath(buf);
        exePath.remove_filename();
		
		sing->load(exePath / "ReflectionMetadata.xml");
#endif
		
	}
	return sing;
}

Metadata::Reflection::Reflection()
:classes(0)
,enums(0)
{
}
// TODO: Merge with GlobalSettings and make part of Serializer class????
void Metadata::Reflection::save(const boost::filesystem::path& filePath)
{
	boost::scoped_ptr<XmlElement> root(Serializer::newRootElement());
	writeChildren(root.get(), EngineCreator);
	std::ofstream stream(filePath.native().c_str(), std::ios_base::out | std::ios_base::binary);
	TextXmlWriter machine(stream);
	machine.serialize(root.get());
}

// TODO: Merge with GlobalSettings and make part of Serializer class????
void Metadata::Reflection::load(const boost::filesystem::path& filePath)
{
	if (filePath.empty())
		return;

	FASTLOGS(FLog::ReflectionMetadata, "Reflection::load %s", filePath.string().c_str());

	std::ifstream stream(filePath.native().c_str(), std::ios_base::in | std::ios_base::binary);

	TextXmlParser machine(stream.rdbuf());
	std::auto_ptr<XmlElement> root(machine.parse());

	MergeBinder binder;

	readChildren(root.get(), binder, EngineCreator);

	bool bound = binder.resolveRefs();
	RBXASSERT(bound);

	// TODO: Check for duplicate names!
	classes = this->findFirstChildOfType<Classes>();
	enums = this->findFirstChildOfType<Enums>();
}

Metadata::Class* Metadata::Reflection::get(const ClassDescriptor& descriptor, bool findBestMatch) const
{
	return classes ? classes->get(descriptor, findBestMatch) : 0;
}

Metadata::Class* Metadata::Classes::get(const ClassDescriptor& descriptor, bool findBestMatch)
{
	Metadata::Class* c = boost::polymorphic_downcast<Metadata::Class*>(findFirstChildByName(descriptor.name.toString()));
	if (c)
		return c;
	if (!findBestMatch)
		return NULL;
	if (!descriptor.getBase())
		return NULL;
	return get(*descriptor.getBase(), true);
}

Metadata::Member* Metadata::Reflection::get(const PropertyDescriptor& descriptor) const
{
	Metadata::Class* c = get(descriptor.owner, false);
	const Metadata::Properties* members = c ? c->findFirstChildOfType<Metadata::Properties>() : 0;
	return members ? boost::polymorphic_downcast<Metadata::Member*>(members->findFirstChildByNameDangerous(descriptor.name.toString())) : 0;
}

Metadata::Member* Metadata::Reflection::get(const FunctionDescriptor& descriptor) const
{
	Metadata::Class* c = get(descriptor.owner, false);
	const Metadata::Functions* members = c ? c->findFirstChildOfType<Metadata::Functions>() : 0;
	return members ? boost::polymorphic_downcast<Metadata::Member*>(members->findFirstChildByNameDangerous(descriptor.name.toString())) : 0;
}

Metadata::Member* Metadata::Reflection::get(const YieldFunctionDescriptor& descriptor) const
{
	Metadata::Class* c = get(descriptor.owner, false);
	const Metadata::YieldFunctions* members = c ? c->findFirstChildOfType<Metadata::YieldFunctions>() : 0;
	return members ? boost::polymorphic_downcast<Metadata::Member*>(members->findFirstChildByNameDangerous(descriptor.name.toString())) : 0;
}

Metadata::Member* Metadata::Reflection::get(const EventDescriptor& descriptor) const
{
	Metadata::Class* c = get(descriptor.owner, false);
	const Metadata::Events* members = c ? c->findFirstChildOfType<Metadata::Events>() : 0;
	return members ? boost::polymorphic_downcast<Metadata::Member*>(members->findFirstChildByNameDangerous(descriptor.name.toString())) : 0;
}

Metadata::Member* Metadata::Reflection::get(const CallbackDescriptor& descriptor) const
{
	Metadata::Class* c = get(descriptor.owner, false);
	const Metadata::Callbacks* members = c ? c->findFirstChildOfType<Metadata::Callbacks>() : 0;
	return members ? boost::polymorphic_downcast<Metadata::Member*>(members->findFirstChildByNameDangerous(descriptor.name.toString())) : 0;
}

Metadata::Enum* Metadata::Reflection::get(const EnumDescriptor& descriptor) const
{
	return enums ? boost::polymorphic_downcast<Metadata::Enum*>(enums->findFirstChildByName(descriptor.name.toString())) : 0;
}

Metadata::EnumItem* Metadata::Reflection::get(const EnumDescriptor::Item& item) const
{
	Metadata::Enum* e = get(item.owner);
	return e ? boost::polymorphic_downcast<Metadata::EnumItem*>(e->findFirstChildByNameDangerous(item.name.toString())) : 0;
}

class Writer
{
	std::ostream& stream;
public:
	Writer(std::ostream& stream):stream(stream)
	{}

	void writeMetadata(const Metadata::Item* metadata, const Descriptor& descriptor)
	{
		if (metadata && !metadata->isBrowsable())
			stream << " [notbrowsable]";
		if (Metadata::Item::isDeprecated(metadata, descriptor))
			stream << " [deprecated]";
		if (Metadata::Item::isBackend(metadata, descriptor))
			stream << " [backend]";
	}

	void writeMemberInfo(const Reflection::MemberDescriptor* d)
	{
		switch (d->security)
		{
		default:
			stream << " [security" << d->security << "]";
			break;
		case Security::None:
			break;
        case Security::Plugin:
            stream << " [PluginSecurity]";
            break;
		case Security::RobloxPlace:
			stream << " [RobloxPlaceSecurity]";
			break;
		case Security::RobloxScript:
			stream << " [RobloxScriptSecurity]";
			break;
		case Security::LocalUser:
			stream << " [LocalUserSecurity]";
			break;
		case Security::WritePlayer:
			stream << " [WritePlayerSecurity]";
			break;
		case Security::Roblox:
			stream << " [RobloxSecurity]";
			break;
		}
	}

	void writeProperty(const Reflection::PropertyDescriptor* d, const Reflection::ClassDescriptor* c)
	{
		if (d->owner != *c)
			return;
		if (!d->isScriptable())
			return;
		stream << "\tProperty " << d->type.name.c_str() << ' ' << d->owner.name.c_str() << '.' << d->name.c_str();
		writeMetadata(Metadata::Reflection::singleton()->get(*d), *d);
		if (!d->isPublic())
			stream << " [hidden]";
		if (d->isReadOnly())
			stream << " [readonly]";
		if (d->isWriteOnly())
			stream << " [writeonly]";
		writeMemberInfo(d);
		stream << '\n';
	}

	void writeArg(const Reflection::SignatureDescriptor::Item& arg, bool* first)
	{
		if (*first)
			*first = false;
		else
			stream << ", ";

		stream << arg.type->name.c_str() << ' ' << arg.name->c_str();
		if (arg.hasDefaultValue())
		{
			stream << " = ";
			stream << arg.defaultValue.get<std::string>();
		}
	}

	void writeArgs(const Reflection::SignatureDescriptor& d)
	{
		bool first = true;
		std::for_each(d.arguments.begin(), d.arguments.end(), boost::bind(&Writer::writeArg, this, _1, &first));
	}

	void writeFunction(const Reflection::FunctionDescriptor* d, const Reflection::ClassDescriptor* c)
	{
		if (d->owner != *c)
			return;
		stream << "\tFunction " << d->getSignature().resultType->name.c_str() << ' ' << d->owner.name.c_str() << ':' << d->name.c_str() << '(';
		writeArgs(d->getSignature());
		stream << ')';
		writeMetadata(Metadata::Reflection::singleton()->get(*d), *d);
		writeMemberInfo(d);
		stream << '\n';
	}

	void writeYieldFunction(const Reflection::YieldFunctionDescriptor* d, const Reflection::ClassDescriptor* c)
	{
		if (d->owner != *c)
			return;
		stream << "\tYieldFunction " << d->getSignature().resultType->name.c_str() << ' ' << d->owner.name.c_str() << ':' << d->name.c_str() << '(';
		writeArgs(d->getSignature());
		stream << ')';
		writeMetadata(Metadata::Reflection::singleton()->get(*d), *d);
		writeMemberInfo(d);
		stream << '\n';
	}

	void writeEvent(const Reflection::EventDescriptor* d, const Reflection::ClassDescriptor* c)
	{
		if (d->owner != *c)
			return;
		if (!d->isScriptable())
			return;
		stream << "\tEvent " << d->owner.name.c_str() << '.' << d->name.c_str() << '(';
		writeArgs(d->getSignature());
		stream << ')';
		writeMetadata(Metadata::Reflection::singleton()->get(*d), *d);
		writeMemberInfo(d);
		stream << '\n';
	}

	void writeCallback(const Reflection::CallbackDescriptor* d, const Reflection::ClassDescriptor* c)
	{
		if (d->owner != *c)
			return;
		stream << "\tCallback " << d->getSignature().resultType->name.c_str() << ' ' << d->owner.name.c_str() << '.' << d->name.c_str() << '(';
		writeArgs(d->getSignature());
		stream << ')';
        if (!d->isAsync())
            stream << " [noyield]";
		writeMetadata(Metadata::Reflection::singleton()->get(*d), *d);
		writeMemberInfo(d);
		stream << '\n';
	}

	void writeClassHeader(const Reflection::ClassDescriptor* d)
	{
		stream << "Class " << d->name.c_str();
		if (d->getBase() && *d->getBase() != Reflection::ClassDescriptor::rootDescriptor())
			stream << " : " << d->getBase()->name.c_str();

		writeMetadata(Metadata::Reflection::singleton()->get(*d, false), *d);
		if (!d->isScriptCreatable())
			stream << " [notCreatable]";
		stream << '\n';
	}

	void writeClass(const Reflection::ClassDescriptor* d)
	{
		if (*d != Reflection::ClassDescriptor::rootDescriptor())
		{
            writeClassHeader(d);

			// Enumerate Properties
			std::for_each(
				d->begin<PropertyDescriptor>(), d->end<PropertyDescriptor>(), 
				boost::bind(&Writer::writeProperty, this, _1, d)
				);

			// Enumerate Functions
			std::for_each(
				d->begin<FunctionDescriptor>(), d->end<FunctionDescriptor>(), 
				boost::bind(&Writer::writeFunction, this, _1, d)
				);

			// Enumerate Yield-Functions
			std::for_each(
				d->begin<YieldFunctionDescriptor>(), d->end<YieldFunctionDescriptor>(), 
				boost::bind(&Writer::writeYieldFunction, this, _1, d)
				);

			// Enumerate Events
			std::for_each(
				d->begin<EventDescriptor>(), d->end<EventDescriptor>(), 
				boost::bind(&Writer::writeEvent, this, _1, d)
				);

			// Enumerate Callbacks
			std::for_each(
				d->begin<CallbackDescriptor>(), d->end<CallbackDescriptor>(), 
				boost::bind(&Writer::writeCallback, this, _1, d)
				);
		}

		// Recurse
		std::for_each(
			d->derivedClasses_begin(), 
			d->derivedClasses_end(),
			boost::bind(&Writer::writeClass, this, _1)
			);
		
	}

	void writeEnum(const EnumDescriptor* d)
	{
		stream << "Enum " << d->name.c_str() << '\n';
		std::for_each(
			d->begin(), d->end(), 
			boost::bind(&Writer::writeEnumItem, this, _1)
			);
	}

	void writeEnumItem(const EnumDescriptor::Item* d)
	{
		stream << "\tEnumItem " << d->owner.name << '.' << d->name << " : " << d->value;
		writeMetadata(Metadata::Reflection::singleton()->get(*d), *d);
		stream << '\n';
	}

	void writeAPI()
	{
		writeClass(&ClassDescriptor::rootDescriptor());

		std::for_each(
			EnumDescriptor::enumsBegin(),
			EnumDescriptor::enumsEnd(),
			boost::bind(&Writer::writeEnum, this, _1)
			);
	}

    template <typename T> const T* checkConflict(const T* desc, std::map<std::string, const T*>& map)
	{
        const T*& old = map[desc->name.toString()];

        if (old)
            return old;

        old = desc;
        return NULL;
	}

	void writeConflictingMember(const ClassDescriptor* desc, const MemberDescriptor* member)
	{
		if (const PropertyDescriptor* d = dynamic_cast<const PropertyDescriptor*>(member))
            writeProperty(d, desc);
		else if (const FunctionDescriptor* d = dynamic_cast<const FunctionDescriptor*>(member))
            writeFunction(d, desc);
		else if (const YieldFunctionDescriptor* d = dynamic_cast<const YieldFunctionDescriptor*>(member))
            writeYieldFunction(d, desc);
		else if (const EventDescriptor* d = dynamic_cast<const EventDescriptor*>(member))
            writeEvent(d, desc);
		else if (const CallbackDescriptor* d = dynamic_cast<const CallbackDescriptor*>(member))
            writeCallback(d, desc);
	}

	template <typename T> void writeConflictingMembers(const ClassDescriptor* desc, std::map<std::string, const MemberDescriptor*>& members)
	{
		const MemberDescriptorContainer<T>* container = desc;

		for (typename MemberDescriptorContainer<T>::Collection::const_iterator it = container->descriptors_begin(); it != container->descriptors_end(); ++it)
		{
			const MemberDescriptor* member = *it;

			if (const MemberDescriptor* oldmember = checkConflict(member, members))
			{
                stream << "ERROR: Duplicate declaration of member " << desc->name.toString() << "." << member->name.toString() << ":\n";
                writeConflictingMember(desc, oldmember);
                writeConflictingMember(desc, member);
			}
		}
	}

    void writeConflicts()
	{
        std::map<std::string, const ClassDescriptor*> classTypes;

		for (ClassDescriptor::ClassDescriptors::const_iterator cit = ClassDescriptor::all_begin(); cit != ClassDescriptor::all_end(); ++cit)
		{
            const ClassDescriptor* desc = *cit;

            if (const ClassDescriptor* olddesc = checkConflict(desc, classTypes))
			{
                stream << "ERROR: Duplicate declaration of class " << desc->name.toString() << "\n";
                stream << "\t";
				writeClassHeader(olddesc);
                stream << "\t";
				writeClassHeader(desc);
			}

			std::map<std::string, const MemberDescriptor*> classMembers;

			writeConflictingMembers<PropertyDescriptor>(desc, classMembers);
			writeConflictingMembers<EventDescriptor>(desc, classMembers);
			writeConflictingMembers<FunctionDescriptor>(desc, classMembers);
			writeConflictingMembers<YieldFunctionDescriptor>(desc, classMembers);
			writeConflictingMembers<CallbackDescriptor>(desc, classMembers);
        }

        std::map<std::string, const EnumDescriptor*> enumTypes;

        for (std::vector<const EnumDescriptor*>::const_iterator eit = EnumDescriptor::enumsBegin(); eit != EnumDescriptor::enumsEnd(); ++eit)
		{
            const EnumDescriptor* desc = *eit;

            if (const EnumDescriptor* olddesc = checkConflict(desc, enumTypes))
			{
                stream << "ERROR: Duplicate declaration of enum " << desc->name.toString() << ":\n";
				stream << "\t" << olddesc->name << "\n";
				stream << "\t" << desc->name << "\n";
			}

            std::map<std::string, const EnumDescriptor::Item*> enumValues;

			for (std::vector<const EnumDescriptor::Item*>::const_iterator eiit = desc->begin(); eiit != desc->end(); ++eiit)
			{
                const EnumDescriptor::Item* item = *eiit;

                if (const EnumDescriptor::Item* olditem = checkConflict(item, enumValues))
				{
                    stream << "ERROR: Duplicate declaration of enum value " << desc->name.toString() << "." << item->name.toString() << ":\n";
                    writeEnumItem(item);
                    writeEnumItem(olditem);
				}

                enumValues[item->name.toString()] = item;
			}
		}
	}

};
void Metadata::writeEverything(std::ostream& stream)
{
	Writer w(stream);

	w.writeConflicts();
	w.writeAPI();
}
