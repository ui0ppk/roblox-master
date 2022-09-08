/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/SafeChat.h"
#include "V8DataModel/Contentprovider.h"
#include "V8Xml/XmlSerializer.h"
#include "StringConv.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>


using namespace RBX;

static const XmlTag& utterance = XmlTag::declare("utterance");

static scoped_ptr<SafeChat> sing;
static boost::once_flag once_SafeChat_singleton = BOOST_ONCE_INIT;
static void SafeChat_singleton()
{
	sing.reset(new SafeChat());
}

SafeChat& SafeChat::singleton()
{
	boost::call_once(&SafeChat_singleton, once_SafeChat_singleton);
	return *sing;
}


static std::string getChatTreeFile()
{
	std::string filename = ContentProvider::getAssetFile("fonts/safechat.xml");
	return filename;
}

void SafeChat::loadChildren(ChatOption *node, const XmlElement *DOMsubTree)
{
	// Load all my children from the DOM
	// Create ChatOptions for each
	// Foreach call loadChildren

	// this is a recursive depth-first tree traversal that mirrors the DOM on-disk

	for(const XmlElement *x = DOMsubTree->findFirstChildByTag(utterance); x != NULL; x = DOMsubTree->findNextChildWithSameTag(x))
	{
		
		std::string text;
		bool succeeded = x->getValue(text); // assert on malformed file
		RBXASSERT(succeeded);
		boost::trim(text);
		ChatOption* child = new ChatOption(text);
		node->children.push_back(child);

		loadChildren(child, x);
	}
}

void SafeChat::loadChatTree()
{
	std::string filename = getChatTreeFile();

	std::ifstream stream(utf8_decode(filename).c_str(), std::ios_base::in | std::ios_base::binary);

	TextXmlParser machine(stream.rdbuf());

	std::auto_ptr<XmlElement> root(machine.parse());

	chatRoot.reset(new ChatOption());
	chatRoot->text = "ROOT";

	loadChildren(chatRoot.get(), root.get());
}

std::string SafeChat::getMessage(std::vector<std::string> code)
{
	// Returns a message from our chat tree, or "" if no message found
	// code is an array of tokenized args, the first of which is gauranteed to be "sc"
	// code[1]..[n] will be chat option child indices - do a tree descent; return correct node
	RBXASSERT(chatRoot);
	RBXASSERT(code[0] == "sc");

	ChatOption *cur = chatRoot.get();

	for(unsigned int i = 1; i < code.size(); i++)
	{
		unsigned int n = 0;
		try
		{
			n = atoi(code[i].c_str());
		}
		catch(std::runtime_error & e)
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Safe Chat error! Failed because %s", e.what());
			return "";
		}

		if (n < cur->children.size())
			cur = cur->children[n];
		else
			return "";
	}

	return cur->text;
}


// Effective STL, by Scott Meyers. ISBN: 0-201-74968-9
// Item 7, page 38
static void del(ChatOption* ptr)
{
	delete ptr;
};

ChatOption::~ChatOption()
{
	std::for_each(children.begin(), children.end(), del);
}
