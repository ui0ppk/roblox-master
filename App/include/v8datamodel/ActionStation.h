/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/JointInstance.h"
#include "V8DataModel/Workspace.h"
#include "Humanoid/Humanoid.h"
#include "V8World/Primitive.h"
#include "V8World/Assembly.h"
#include "rbx/rbxTime.h"

DYNAMIC_FASTINT(ActionStationDebounceTime)

namespace RBX {


template<class Base>
class ActionStation : public Base
{
	private:			
		typedef Base Super;
	protected:
		Time sleepTime;
		Time debounceTime;

		bool sleepTimeUp() const {
			return (Time::now<Time::Fast>() - sleepTime).seconds() > 3.0;
		}

		bool debounceTimeUp() const {
				return (Time::now<Time::Fast>() - debounceTime).seconds() > DFInt::ActionStationDebounceTime;
		}

		// Instance
		// TODO: Ultra mega super hack - setName is setting the internal Primitive::sizeMultiplier
		void setName(const std::string& value) {
			Super::setName(value);
			this->getPartPrimitive()->setSizeMultiplier(Primitive::SEAT_SIZE);
		}

	public:
		ActionStation() : sleepTime(Time::now<Time::Fast>() - Time::Interval(4.0)), debounceTime(Time::now<Time::Fast>())
		{
			RBXASSERT(this->sleepTimeUp());
			RBXASSERT(this->getPartPrimitive());
			this->getPartPrimitive()->setSizeMultiplier(Primitive::SEAT_SIZE);
		}

		virtual ~ActionStation() {}
};


} // namespace