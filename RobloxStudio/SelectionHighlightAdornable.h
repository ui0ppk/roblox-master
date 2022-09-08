#pragma once

#include "GfxBase/IAdornable.h"
#include "GfxBase/Part.h"
#include "V8DataModel/DataModel.h"

#include <boost/shared_ptr.hpp>

namespace RBX
{
	class Workspace;
}

class SelectionHighlightAdornable : public RBX::IAdornable
{
	static bool getSelectionDimensions(RBX::Workspace* workspace, shared_ptr<RBX::Instance> instance,
		RBX::Part* out, float* lineThickness, bool* checkChildren);
	static void render(shared_ptr<RBX::Instance> instance, RBX::Part part, float lineThickness, RBX::Adorn* adorn);

protected:
	bool shouldRender3dAdorn() const override { return true; }

public:
	template<class T>
	static void renderSelection(RBX::DataModel* dm, T& collection, RBX::Adorn* adorn,
		boost::function<void(shared_ptr<RBX::Instance>, RBX::Part, float, RBX::Adorn* adorn)> renderer)
	{
		using namespace RBX;
		for (auto& instance : collection)
		{
			Part size;
			float lineSize;
			bool checkChildren;
			if (getSelectionDimensions(dm->getWorkspace(), instance, &size, &lineSize, &checkChildren))
			{
				renderer(instance, size, lineSize, adorn);
			}
			if (checkChildren && instance->getChildren2())
			{
				renderSelection(dm, *instance->getChildren2(), adorn, renderer);
			}
		}
	}
	void render3dAdorn(RBX::Adorn* adorn) override;
};