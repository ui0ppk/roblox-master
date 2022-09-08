/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include <map>
#include <boost/function.hpp>

#include "rbx/Debug.h"
#include "security/FuzzyTokens.h"
#include "v8datamodel/HackDefines.h"

LOGGROUP(Verbs)

namespace RBX {

class Name;
class VerbContainer;

class RBXInterface IDataState
{
public:
	// Sets the document's "dirty" bit
	virtual void setDirty(bool dirty) = 0;
	virtual bool isDirty() const = 0;
};

class RBXBaseClass Verb
{
	const Name& name;
protected:
	VerbContainer* const container;
	Verb(VerbContainer* container, const std::string& name, bool blacklisted = false);
	Verb(VerbContainer* container, const Name& name, bool blacklisted = false);
    bool verbSecurity;
public:
	virtual ~Verb();
	virtual bool isEnabled() const { return true; }
	virtual bool isChecked() const { return false; }
	virtual bool isSelected() const { return false; }

	virtual std::string getText() const { return std::string(); }
	const Name& getName() const {return name;}

	virtual void doIt(IDataState* dataState) = 0;

	VerbContainer* getContainer() const { return container; }

    bool getVerbSecurity() const { return verbSecurity; }

    static inline void doItWithChecks(Verb* const verb, IDataState* dataState)
    {
#if !defined(RBX_STUDIO_BUILD)
        if (verb->getVerbSecurity())
        {
            RBX::Security::setHackFlagVs<LINE_RAND1>(RBX::Security::hackFlag9, HATE_VERB_SNATCH);
        }
        else
#endif
        {
            verb->doIt(dataState);
        }
    }
};

// This version of Verb lets you pass in callbacks for the implementation
class BoundVerb : public Verb
{
	boost::function<void(VerbContainer*)> doItFunction;
	boost::function<bool(VerbContainer*)> isEnabledFunction;
	boost::function<bool(VerbContainer*)> isCheckedFunction;
	boost::function<std::string(VerbContainer*)> getTextFunction;

public:
	BoundVerb(VerbContainer* container, const char* name,
		boost::function<void(VerbContainer*)> doItFunction,
		boost::function<bool(VerbContainer*)> isEnabledFunction = NULL,
		boost::function<bool(VerbContainer*)> isCheckedFunction = NULL,
		boost::function<std::string(VerbContainer*)> getTextFunction = NULL)
		: Verb(container, name)
		, doItFunction(doItFunction)
		, isEnabledFunction(isEnabledFunction)
		, isCheckedFunction(isCheckedFunction)
		, getTextFunction(getTextFunction)
	{
	}

	virtual bool isEnabled() const
	{
		if (!isEnabledFunction)
			return Verb::isEnabled();
		return isEnabledFunction(container);
	}

	virtual bool isChecked() const
	{
		if (!isCheckedFunction)
			return Verb::isChecked();
		return isCheckedFunction(container);
	}

	virtual std::string getText() const
	{
		if (!getTextFunction)
			return Verb::getText();
		return getTextFunction(container);
	}

	virtual void doIt(IDataState* dataState)
	{
		if (doItFunction)
			doItFunction(container);
	}

};

class NullVerb : public Verb
{
public:
	NullVerb(VerbContainer* container, const std::string& name)
		: Verb(container, name)
	{
	}

	/*override*/ bool isEnabled() const { return false; }
	/*override*/ void doIt(IDataState* dataState) {}
};

class VerbContainer
{
	friend class Verb;
	typedef std::map<const Name*, Verb*> Verbs;
    Verbs whitelistVerbs; // used for UI verbs
    Verbs blacklistVerbs; // used for tools
	VerbContainer* parent;

	void addWhitelistVerb(Verb* verb);
    void addBlacklistVerb(Verb* verb);
	void removeVerb(Verb* verb);

public:
	VerbContainer(VerbContainer* parent);
	virtual ~VerbContainer();
	Verb* getVerb(const Name& name);
	Verb* getVerb(const std::string& name);
    Verb* getWhitelistVerb(const Name& name);
    Verb* getWhitelistVerb(const std::string& name);
    // This version of getVerb is used as a minor obsfucation of a string.
    Verb* getWhitelistVerb(const std::string& prefix, const std::string& name, const std::string& suffix);
	void setVerbParent(VerbContainer* parent);
	VerbContainer* getVerbParent() const { return parent; }

	template<class F>
	void eachVerbName(F f, bool includeParent = true)
	{
		for (Verbs::const_iterator iter = whitelistVerbs.begin(); iter != whitelistVerbs.end(); ++iter)
			f(iter->first);
        for (Verbs::const_iterator iter = blacklistVerbs.begin(); iter != blacklistVerbs.end(); ++iter)
            f(iter->first);
		if (includeParent && parent)
			parent->eachVerbName(f);
	}
};

}  // namespace RBX
