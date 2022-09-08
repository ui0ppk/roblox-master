#pragma once

#include "Gui/GUI.h"

namespace RBX {
	
	class EquationDisplay : public TextDisplay {
	private:
		std::string			equation;

	protected:
		// TextDisplay
		std::string getLabel() const;
	
	public:
		EquationDisplay(
			const std::string& title, 
			const std::string& equation);

		EquationDisplay(
			const std::string& title, 
			const std::string& label, 
			const std::string& equation);

		/*override*/ void render2d(Adorn* adorn);
	};
} // namespace