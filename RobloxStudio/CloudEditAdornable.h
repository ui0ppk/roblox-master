#pragma once

#include "GfxBase/IAdornable.h"
#include "GfxBase/Part.h"
#include "V8Tree/Instance.h"
#include "V8DataModel/DataModel.h"

#include <boost/shared_ptr.hpp>

namespace RBX
{
	class Camera;
}

class PlayersDataManager;

class CloudEditAdornable : public RBX::IAdornable
{
	shared_ptr<RBX::DataModel>     m_pDataModel;
	shared_ptr<PlayersDataManager> m_pPlayersDataManager;
		
	static void renderSelection(RBX::Color3 playerColor,
		shared_ptr<RBX::Instance> instance, RBX::Part part, float lineThickness, RBX::Adorn* adorn);
	static void renderCamera(RBX::Color3 playerColor, shared_ptr<RBX::Network::Player> player,
		const RBX::Camera* localCamera, RBX::Adorn* adorn);
	
protected:
	bool shouldRender3dAdorn() const override { return true; }

public:
	void attach(shared_ptr<RBX::DataModel> dataModel, shared_ptr<PlayersDataManager> playersDataManager); // requires DataModel write lock is held
	void render3dAdorn(RBX::Adorn* adorn) override;
};
