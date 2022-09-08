/* Copyright 2003-2015 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "v8tree/Instance.h"

LOGGROUP(GuiTargetLifetime)

namespace RBX {

	class GuiResponse
	{
	private:
		enum ResponseType {NOT_SUNK, SUNK};
		enum FinishedType {NOT_FINISHED, FINISHED};
		enum MouseOverType {NOT_MOUSE_OVER, MOUSE_OVER};

		ResponseType	response;
		FinishedType	finished;
		MouseOverType	mouseWasOver;
		weak_ptr<Instance>		target;

		GuiResponse(ResponseType response, FinishedType finished, MouseOverType mouseWasOver, Instance* newTarget)
			: response(response)
			, finished(finished)
			, mouseWasOver(mouseWasOver)
			, target(weak_from(newTarget))
		{}

		GuiResponse(ResponseType response)
			: response(response)
			, finished(NOT_FINISHED)
			, mouseWasOver(NOT_MOUSE_OVER)
		{}

	public:
		GuiResponse()
			: response(NOT_SUNK)
			, finished(NOT_FINISHED)
			, mouseWasOver(NOT_MOUSE_OVER)
		{}

		bool getMouseWasOver() const			{return mouseWasOver == MOUSE_OVER;}
		void setMouseWasOver()					{mouseWasOver = MOUSE_OVER;}

		bool wasSunk()		{return (response == SUNK);}
		bool wasSunkAndFinished()
		{
			RBXASSERT(!((finished == FINISHED) && !wasSunk()));
			return (finished == FINISHED);
		}

		static GuiResponse notSunk()							{return GuiResponse(NOT_SUNK);}
		static GuiResponse notSunkMouseWasOver()				{return GuiResponse(NOT_SUNK, NOT_FINISHED, MOUSE_OVER, NULL);}
		static GuiResponse sunk()								{return GuiResponse(SUNK);}
		static GuiResponse sunkAndFinished()					{return GuiResponse(SUNK, FINISHED, NOT_MOUSE_OVER, NULL);}
		static GuiResponse sunkWithTarget(Instance* target)	{return GuiResponse(SUNK, NOT_FINISHED, NOT_MOUSE_OVER, target);}

		shared_ptr<Instance> getTarget() { return target.lock(); }
		void setTarget(Instance* value) { target = weak_from(value); }
	};
}	// namespace
