#include "stdafx.h"

#include "reflection/object.h"
#include "g3d/format.h"

using namespace RBX;
using namespace RBX::Reflection;

bool Descriptor::lockedDown  = false;

void (*MemberDescriptor::memberHidingHook)(MemberDescriptor*, MemberDescriptor*) = 0;


static ClassDescriptor::ClassDescriptors& staticData2()
{
	static ClassDescriptor::ClassDescriptors result;
	return result;
}
static void initStaticData2()
{
	staticData2();
}

ClassDescriptor::ClassDescriptors& ClassDescriptor::allClasses()
{
	static boost::once_flag flag = BOOST_ONCE_INIT;
	boost::call_once(&initStaticData2, flag);
	return staticData2();
}


ClassDescriptor::ClassDescriptor()
	:Descriptor("<<<ROOT>>>", Descriptor::Attributes())
	,MemberDescriptorContainer<PropertyDescriptor>(NULL)
	,MemberDescriptorContainer<FunctionDescriptor>(NULL)
	,MemberDescriptorContainer<EventDescriptor>(NULL)
	,MemberDescriptorContainer<YieldFunctionDescriptor>(NULL)
	,MemberDescriptorContainer<CallbackDescriptor>(NULL)
	,base(NULL)
	,bReplicateType(true)
	,bCanXmlWrite(true)
	,bIsScriptable(true)
	,security(Security::None)
{
}

unsigned int ClassDescriptor::checksum(const EventDescriptor* eventDesc, boost::crc_32_type& result)
{
    result.process_bytes(eventDesc->name.c_str(), eventDesc->name.toString().size());
    for(std::list<SignatureDescriptor::Item>::const_iterator argIter = eventDesc->getSignature().arguments.begin(); 
        argIter != eventDesc->getSignature().arguments.end(); ++argIter)
    {
        const SignatureDescriptor::Item arg = *argIter;
        result.process_bytes(arg.name->c_str(), arg.name->toString().size());
        result.process_bytes(arg.type->name.c_str(), arg.type->name.toString().size());
    }
    return result.checksum();
}

unsigned int ClassDescriptor::checksum(const PropertyDescriptor* propDesc, boost::crc_32_type& result)
{
    result.process_bytes(propDesc->name.c_str(), propDesc->name.toString().size());
    result.process_bytes(propDesc->type.name.c_str(), propDesc->type.name.toString().size());
    result.process_byte(propDesc->canReplicate());
    if (propDesc->bIsEnum)
    {
        const EnumPropertyDescriptor& enumDesc = static_cast<const EnumPropertyDescriptor&>(*propDesc);
        result.process_byte((unsigned char)enumDesc.enumDescriptor.getEnumCountMSB());
    }
    return result.checksum();
}

unsigned int ClassDescriptor::checksum(const ClassDescriptor* desc, boost::crc_32_type& result)
{
    result.process_bytes(desc->name.c_str(), desc->name.toString().size());
    result.process_byte(desc->getReplicationLevel());
    MemberDescriptorContainer<PropertyDescriptor>::Collection::const_iterator propIter =
        desc->MemberDescriptorContainer<PropertyDescriptor>::descriptors_begin();
    MemberDescriptorContainer<PropertyDescriptor>::Collection::const_iterator propEnd =
        desc->MemberDescriptorContainer<PropertyDescriptor>::descriptors_end();
    for (; propIter != propEnd; propIter++)
    {
        const PropertyDescriptor* propDesc = *propIter;
        checksum(propDesc, result);
    }
    MemberDescriptorContainer<EventDescriptor>::Collection::const_iterator eventIter =
        desc->MemberDescriptorContainer<EventDescriptor>::descriptors_begin();
    MemberDescriptorContainer<EventDescriptor>::Collection::const_iterator eventEnd =
        desc->MemberDescriptorContainer<EventDescriptor>::descriptors_end();
    for (; eventIter != eventEnd; eventIter++)
    {
        const EventDescriptor* eventDesc = *eventIter;
        checksum(eventDesc, result);
    }
    return result.checksum();
}

unsigned int ClassDescriptor::checksum(const Type* typeDesc, boost::crc_32_type& result)
{
    result.process_bytes(typeDesc->name.c_str(), typeDesc->name.toString().size());
    result.process_byte(typeDesc->isFloat);
    result.process_byte(typeDesc->isNumber);
    return result.checksum();
}

unsigned int ClassDescriptor::checksum()
{
    static unsigned int checksumValue = 0;
    if (checksumValue == 0)
    {
        StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Calculating checksum...");
        boost::crc_32_type result;

        ClassDescriptors::const_iterator iter = all_begin();
        while (iter!=all_end())
        {
            const ClassDescriptor* desc = (*iter);
            checksum(desc, result);
            ++iter;
        }
        checksumValue = result.checksum();
    }
    StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "API checksum %d", checksumValue);
    return checksumValue;
}

// Used for sorting
static bool compare2(const ClassDescriptor* a, const ClassDescriptor* b)
{
	return a->name < b->name;
}

int ClassDescriptor::count = 0;

ClassDescriptor::ClassDescriptor(ClassDescriptor& base, const char* name, Attributes attributes, Security::Permissions security)
	:Descriptor(name, attributes)
	,MemberDescriptorContainer<PropertyDescriptor>(&base)
	,MemberDescriptorContainer<FunctionDescriptor>(&base)
	,MemberDescriptorContainer<EventDescriptor>(&base)
	,MemberDescriptorContainer<YieldFunctionDescriptor>(&base)
	,MemberDescriptorContainer<CallbackDescriptor>(&base)
	,base(&base)
	,bReplicateType((attributes.flags>>1) & 0x3)
	,bCanXmlWrite((attributes.flags>>3) & 0x1)
	,bIsScriptable((attributes.flags>>4) & 0x1)
	,security(security)
{
	count++;

	{
		ClassDescriptors::iterator iter = std::lower_bound(base.derivedClasses.begin(), base.derivedClasses.end(), this, compare2);
		RBXASSERT(iter == base.derivedClasses.end() || *iter != this);
		base.derivedClasses.insert(iter, this);
	}

	ClassDescriptors::iterator iter = allClasses().begin();
	while (iter != allClasses().end())
	{
		ClassDescriptor* desc = *iter;
		if (this->name < desc->name)
			break;
		++iter;
	}
	allClasses().insert(iter, this);
}

bool ClassDescriptor::operator==(const ClassDescriptor& other) const
{
	return this == &other;
}

bool ClassDescriptor::operator!=(const ClassDescriptor& other) const
{
	return this != &other;
}

bool ClassDescriptor::isBaseOf(const ClassDescriptor& child) const
{
	return child.base && child.base->isA(*this);
}

bool ClassDescriptor::isA(const ClassDescriptor& test) const
{
	if (name == test.name)
		return true;
	if (base)
		return base->isA(test);
	return false;
}


//bool ClassDescriptor::isBaseOf(const char* childName) const;
bool ClassDescriptor::isA(const char* testName) const
{
	if (name == testName)
		return true;
	if (base)
		return base->isA(testName);
	return false;
}


bool MemberDescriptor::isMemberOf(const ClassDescriptor& classDescriptor) const
{
	const ClassDescriptor* d = &classDescriptor;
	do
	{
		if (*d == this->owner)
			return true;
		d = d->getBase();
	}
	while (*d != ClassDescriptor::rootDescriptor());

	return false;
}



bool MemberDescriptor::isMemberOf(const DescribedBase* instance) const
{
	RBXASSERT(instance != NULL);
	return isMemberOf(instance->getDescriptor());
}
