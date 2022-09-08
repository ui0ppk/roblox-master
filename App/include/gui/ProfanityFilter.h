/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/ScopedSingleton.h"

namespace RBX {
	
	class WordList
	{
	private:
		std::set<std::string> blacklist;
		void decrypt(std::string& str);
	public:
		WordList();
		~WordList();

		bool ContainsProfanity(std::string str);

	};


	class ProfanityFilter : public ScopedSingleton<ProfanityFilter>
	{	
	private:
		WordList *wordlist;
		bool ContainsProfanityWorker(std::string str);
	public:
		ProfanityFilter();
		~ProfanityFilter();

		static bool ContainsProfanity(const std::string& str);
	};
	
} // namespace