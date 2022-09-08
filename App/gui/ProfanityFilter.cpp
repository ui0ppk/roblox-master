/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Network/Player.h"
#include "Util/Hash.h"
#include "Util/Color.h"
#include "Gui/ProfanityFilter.h"
#include <boost/algorithm/string.hpp>
#include "v8datamodel/contentprovider.h"
#include "Util/SafeToLower.h"
#include "StringConv.h"

namespace RBX {


void WordList::decrypt(std::string& str)
{
	for(unsigned i = 0; i<str.size(); ++i){
		str[i] = 0x55 ^ str[i];
	}
}

WordList::WordList()
{
	/* Load blacklist

	
	*/

	std::string filename = ContentProvider::getAssetFile("Fonts\\diogenes.fnt");
	
	std::ifstream infile(utf8_decode(filename).c_str());
	std::string line;
	while( std::getline(infile, line, '\n'))
	{
		safeToLower(line);
		boost::trim(line);
		if (line.length() > 2){
			decrypt(line);
			blacklist.insert(line);
		}
	}

	infile.close();
}
WordList::~WordList()
{

}
bool WordList::ContainsProfanity(std::string str)
{
	/*
	1. Convert query to lower case
	2. Check for in black list
	*/

	safeToLower(str);

	return blacklist.find(str) != blacklist.end();
}


ProfanityFilter::ProfanityFilter()
	: wordlist(0)
{
}

ProfanityFilter::~ProfanityFilter()
{
	if(wordlist)
	{
		delete wordlist;
		wordlist = 0;
	}
}

bool ProfanityFilter::ContainsProfanity(const std::string& str)
{
	shared_ptr<ProfanityFilter> instance = getInstance();
	RBXASSERT(instance->getInitCount() <= 1); // if you hit this, you are using the ScopedSingleton improperly. Save a shared_ptr on a long-lived instance.
	return instance->ContainsProfanityWorker(str);
}


bool ProfanityFilter::ContainsProfanityWorker(std::string str)
{
	/*
		1. Convert string to lower case
		2. Break string into testable "words" (a word is a potential entry in the blacklist)
		3. Test each word
		4. Return false if any fail, true otherwise
		
		TODO - doesn't currently handle bigrams, so bigrams in word list effectively ignored
	*/

	// You might be thinking "wtf". We don't want to lock if we have a wordlist. We need a check after we get the lock that the wordlist is still NULL.
	if (wordlist == NULL) 
	{
		boost::mutex sync;
		boost::mutex::scoped_lock lock(sync);
		if (wordlist == NULL)
			wordlist = new WordList();
	}


	safeToLower(str);

	std::vector<std::string> words;

	boost::split(words, str, boost::is_any_of(" !.?,:;><[]{}|\\/@#$%^&*()-+=\"")); // TODO: add more delimiters

	for(unsigned int i = 0; i < words.size(); i++)
	{
		if (wordlist->ContainsProfanity(words[i])) 
			return true;
	}

	return false;
}

} // namespace


