#pragma once

namespace RBX {

	class NavKeys {
	public:
		bool forward_arrow;
		bool backward_arrow;
		bool left_arrow;
		bool right_arrow;
		bool forward_asdw;
		bool backward_asdw;
		bool left_asdw;
		bool right_asdw;
		bool strafe_left_q;
		bool strafe_right_e;
		bool space;
		bool backspace;
		bool shift;

		NavKeys() : forward_arrow(false), 
					backward_arrow(false), 
					left_arrow(false), 
					right_arrow(false),
					forward_asdw(false),
					backward_asdw(false), 
					left_asdw(false), 
					right_asdw(false), 
					strafe_left_q(false),
					strafe_right_e(false),
					space(false),
					backspace(false),
					shift(false)
		{}

		bool forward() const		{return (forward_arrow || forward_asdw);}

		bool backward() const		{return (backward_arrow || backward_asdw);}

		bool left() const			{return (left_arrow || left_asdw);}
		
		bool right() const			{return (right_arrow || right_asdw);}

		bool up() const				{return strafe_left_q;}

		bool down() const			{return strafe_right_e;}

		bool backspaceDown() const {return backspace;}

		bool arrowKeyDown() const	{return (forward_arrow || backward_arrow || left_arrow || right_arrow);}

		bool asdwKeyDown() const	{return (forward_asdw || backward_asdw || left_asdw || right_asdw);}

		bool qeKeyDown() const		{return (strafe_left_q || strafe_right_e);}

		bool navKeyDown() const		{return (arrowKeyDown() || asdwKeyDown() || qeKeyDown() || space) || backspaceDown();}

		int leftRightASDW() const	{return left_asdw ? 1 : (right_asdw ? -1 : 0);}

		int strafeQE() const		{return strafe_left_q ? 1 : (strafe_right_e ? -1 : 0);}

		int leftRightArrow() const {
			return left_arrow ? 1 : (right_arrow ? -1 : 0);
		}

		int forwardBackwardArrow() const {
			return forward_arrow ? 1 : (backward_arrow ? -1 : 0);
		}

		int forwardBackwardASDW() const {
			return forward_asdw ? 1 : (backward_asdw ? -1 : 0);
		}

		int strafeLeftRightQE() const {
			return strafe_left_q ? 1 : (strafe_right_e ? -1 : 0);
		}

		bool shiftKeyDown() const
		{
			return shift;
		}


	};

} // namespace