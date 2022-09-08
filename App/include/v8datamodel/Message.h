/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"
#include "GfxBase/IAdornable.h"
#include "Util/ContentFilter.h"
#include <string>

namespace RBX {

	extern const char* const sMessage;
	class Message :	public DescribedCreatable<Message, Instance, sMessage>,
					public IAdornable
	{
	private:
		typedef DescribedCreatable<Message, Instance, sMessage> Super;
	protected:
		std::string text;
		void renderFullScreen(Adorn* adorn);
		
		ContentFilter::FilterResult filterState;
	public:
		Message();
		~Message();

		/*override*/ bool shouldRender2d() const {return true;}
		/*override*/ void render2d(Adorn* adorn);

		/*override*/ bool askSetParent(const Instance* instance) const {return true;}

		const std::string& getText() const;
		void setText(const std::string& value);

		
		/*override*/ int getPersistentDataCost() const 
		{
			return Super::getPersistentDataCost() + Instance::computeStringCost(getText());
		}
	};


	extern const char* const sHint;
	class Hint :	
		public DescribedCreatable<Hint, Message, sHint>
	{
		/*override*/ virtual void render2d(Adorn* adorn);
		/*override*/ bool canClientCreate() { return true; }

	};



}	// namespace RBX