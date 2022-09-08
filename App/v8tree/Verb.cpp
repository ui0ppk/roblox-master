#include "stdafx.h"

#include "v8tree/Verb.h"

#include "util/Name.h"

namespace RBX {

Verb::Verb(VerbContainer* container, const std::string& name, bool blacklisted)
	: name(Name::declare(name.c_str()))
	, container(container)
    , verbSecurity(blacklisted)
{
	if(container)
    {
        if (!blacklisted)
		    container->addWhitelistVerb(this);
        else
            container->addBlacklistVerb(this);
    }
}

Verb::~Verb()
{
	if (container!=NULL)
		container->removeVerb(this);
}

VerbContainer::VerbContainer(VerbContainer* parent)
	: parent(parent)
{
}

void VerbContainer::setVerbParent(VerbContainer* parent)
{
	this->parent = parent;
}

VerbContainer::~VerbContainer()
{
	// delete any remaining verbs
	while (whitelistVerbs.begin()!=whitelistVerbs.end())
		delete whitelistVerbs.begin()->second;
    while (blacklistVerbs.begin()!=blacklistVerbs.end())
        delete blacklistVerbs.begin()->second;
}

Verb* VerbContainer::getVerb(const std::string& name)
{
	return getVerb(Name::lookup(name));
}

Verb* VerbContainer::getWhitelistVerb(const std::string& name)
{
    return getWhitelistVerb(Name::lookup(name));
}

Verb* VerbContainer::getWhitelistVerb(const std::string& prefix, const std::string& name, const std::string& suffix)
{
    return getWhitelistVerb(Name::lookup(prefix + name + suffix));
}

Verb* VerbContainer::getVerb(const Name& name)
{
	if (name.empty())
		return NULL;

	std::map<const Name*, Verb*>::iterator iter = whitelistVerbs.find(&name);
	if (iter!=whitelistVerbs.end())
		return iter->second;

    iter = blacklistVerbs.find(&name);
    if (iter!=blacklistVerbs.end())
        return iter->second;

	if (parent!=NULL)
		return parent->getVerb(name);
	else
		return NULL;
}

Verb* VerbContainer::getWhitelistVerb(const Name& name)
{
    if (name.empty())
        return NULL;

    std::map<const Name*, Verb*>::iterator iter = whitelistVerbs.find(&name);
    if (iter!=whitelistVerbs.end())
        return iter->second;

    if (parent!=NULL)
        return parent->getWhitelistVerb(name);
    else
        return NULL;
}

void VerbContainer::addWhitelistVerb(Verb* verb)
{
	RBXASSERT((whitelistVerbs.find(&verb->getName())==whitelistVerbs.end())
        && (blacklistVerbs.find(&verb->getName())==blacklistVerbs.end()));
	whitelistVerbs[&verb->getName()] = verb;
}

void VerbContainer::addBlacklistVerb(Verb* verb)
{
    RBXASSERT((whitelistVerbs.find(&verb->getName())==whitelistVerbs.end())
        && (blacklistVerbs.find(&verb->getName())==blacklistVerbs.end()));
    blacklistVerbs[&verb->getName()] = verb;
}

void VerbContainer::removeVerb(Verb* verb)
{
	int verEraseCount = whitelistVerbs.erase(&verb->getName()) + blacklistVerbs.erase(&verb->getName());
	RBXASSERT(verEraseCount==1);
}

}  // namespace RBX
