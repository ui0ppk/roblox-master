#include "stdafx.h"

#include "CloudEditAdornable.h"

#include "RobloxDocManager.h"
#include "RobloxIDEDoc.h"
#include "PlayersDataManager.h"

#include "AppDraw/Draw.h"
#include "GfxBase/IAdornableCollector.h"
#include "Network/Player.h"
#include "Network/Players.h"
#include "V8DataModel/Workspace.h"


using namespace RBX;

void CloudEditAdornable::renderSelection(Color3 playerColor, shared_ptr<Instance> instance, Part part, float lineThickness, Adorn* adorn)
{
	static const Vector3 kLipSize = Vector3::one() * .015;
	Vector3 size = (part.gridSize * .5) + kLipSize;
	adorn->box(part.coordinateFrame, size, Color4(playerColor, .25), -1, false);
	Draw::selectionBox(part, adorn, Color4(playerColor, 1), lineThickness);
}

void CloudEditAdornable::renderCamera(Color3 playerColor, shared_ptr<RBX::Network::Player> player, const Camera* localCamera, Adorn* adorn)
{
	CoordinateFrame cameraCFrame = player->getCloudEditCameraCoordinateFrame();
	Color4 cameraColor(playerColor, 0.25);

	adorn->setObjectToWorldMatrix(cameraCFrame);
	adorn->ray(RbxRay(Vector3(0, 0, 1.2), Vector3(0, 0, -3)), cameraColor);
	adorn->sphere(Sphere(Vector3::zero(), .75), cameraColor);

	Vector3 screenLoc = localCamera->project(cameraCFrame.translation);
	adorn->drawFont2D(
		player->getName(),
		screenLoc.xy(),
		14 /*fontSize*/,
		false,
		Color3::white(),
		Color3::black(),
		Text::FONT_ARIALBOLD,
		Text::XALIGN_CENTER,
		Text::YALIGN_BOTTOM);
}

void CloudEditAdornable::attach(shared_ptr<RBX::DataModel> dataModel, shared_ptr<PlayersDataManager> playersDataManager)
{
	m_pDataModel = dataModel;
	m_pPlayersDataManager = playersDataManager;

	RBXASSERT(m_pDataModel->currentThreadHasWriteLock());
	m_pDataModel->getWorkspace()->getAdornableCollector()->onRenderableDescendantAdded(this);
}

void CloudEditAdornable::render3dAdorn(Adorn* adorn)
{
	if (Network::Players* players = m_pDataModel->find<Network::Players>())
	{
		for (auto& playerInstance : *(players->getPlayers()))
		{
			if (shared_ptr<Network::Player> p = Instance::fastSharedDynamicCast<Network::Player>(playerInstance))
			{
				if (p.get() == players->getLocalPlayer()) 
				{
					continue;
				}

				Color3 playerColor = RBX::BrickColor(m_pPlayersDataManager->getPlayerColor(p->getUserID())).color3();

				renderCamera(playerColor, p, m_pDataModel->getWorkspace()->getConstCamera(), adorn);
				shared_ptr<const RBX::Instances> selectedInstances = m_pPlayersDataManager->getLastKnownSelection(p->getUserID());
				if (selectedInstances->size() > 0)
				{
					SelectionHighlightAdornable::renderSelection(m_pDataModel.get(), *selectedInstances, adorn,
						boost::bind(&CloudEditAdornable::renderSelection, playerColor, _1, _2, _3, _4));
				}
			}
		}
	}
}
