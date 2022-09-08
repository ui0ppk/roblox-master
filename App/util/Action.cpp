#include "stdafx.h"
#include "Util/Action.h"
#include "RbxAssert.h"
#include "reflection/enumconverter.h"


namespace RBX 
{
	namespace Reflection
	{
		template<>
		EnumDesc<Action::ActionType>::EnumDesc()
			:EnumDescriptor("ActionType")
		{
			addPair(Action::NO_ACTION,"Nothing");
			addPair(Action::PAUSE_ACTION,"Pause");
			addPair(Action::LOSE_ACTION,"Lose");
			addPair(Action::DRAW_ACTION,"Draw");
			addPair(Action::WIN_ACTION,"Win");
		}
	} // namespace Reflection
}	// namespace RBX
