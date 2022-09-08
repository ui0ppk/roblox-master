#include "stdafx.h"
#include "Gui/EquationDisplay.h"
#include "Util/IMetric.h"

namespace RBX {

///////////////////////////////////////////////////////////////////////////
//
// EquationDisplay
//

EquationDisplay::EquationDisplay(
	const std::string& title, 
	const std::string& eqText) 
	: 
		TextDisplay(title, title),
		equation(eqText)
{}


EquationDisplay::EquationDisplay(
	const std::string& title, 
	const std::string& label, 
	const std::string& eqText) 
	:	TextDisplay(title, label)
	,	equation(eqText)
{}


std::string EquationDisplay::getLabel() const
{
	const Instance* root = Instance::getRootAncestor(this);
	const IMetric* metric = dynamic_cast<const IMetric*>(root);
	if (metric) {
		RBXASSERT(metric);
		std::string answer = metric->getMetric(equation);
		return label + " " + answer;
	}
	else {
		return label;
	}
}

void EquationDisplay::render2d(Adorn* adorn)
{
	if (isVisible())
	{
		label2d(
			adorn,
			getLabel(),
			fontColor,
			borderColor,
			align);
	}
}



} // namespace