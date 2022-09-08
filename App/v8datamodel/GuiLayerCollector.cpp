#include "stdafx.h"
#include "V8DataModel/GuiLayerCollector.h"

#include "V8DataModel/GuiObject.h"
#include "v8datamodel/TextBox.h"
#include "v8datamodel/ScrollingFrame.h"
#include "reflection/Object.h"
#include "v8datamodel/GuiService.h"
#include "v8datamodel/Folder.h"


DYNAMIC_FASTFLAGVARIABLE(FixClippedScrollingFrameNavigation, true)

namespace RBX {
	const char* const sLayerCollector = "LayerCollector";

	static bool isAllGuiQueuesHaveCorrectNumberOfLayers( std::vector<  std::vector<shared_ptr<GuiBase> > > vectors[])
	{
		for (int queueIdx = 0; queueIdx < GUIQUEUE_COUNT; ++queueIdx)
		{
			if (vectors[queueIdx].size() != GuiBase::maxZIndex() + 1)
			{
				return false;
			}
		}

		return true;
	}

	GuiLayerCollector::GuiLayerCollector(const char* name)
		: DescribedNonCreatable<GuiLayerCollector, GuiBase2d, sLayerCollector>(name)
		, rebuildGuiVector(true)
	{
		FASTLOG1(FLog::GuiTargetLifetime, "GuiLayerCollector created: %p", this);

		RBXASSERT(GuiBase::minZIndex() == 0);
		for (int queueIdx = 0; queueIdx < GUIQUEUE_COUNT; ++queueIdx)
		{
			// ignoring zIdx on purpose, just need to push maxZIndex items onto each queue
			for (int zIdx = 0; zIdx <= GuiBase::maxZIndex(); ++zIdx) 
			{
				GuiVector temp;
				mGuiVectors[queueIdx].push_back(temp);
			}
		}
	}

	GuiLayerCollector::~GuiLayerCollector()
	{
		FASTLOG1(FLog::GuiTargetLifetime, "GuiLayerCollector destroyed: %p", this);
	}
    
    void GuiLayerCollector::onDescendantAdded(Instance* instance)
    {
        Super::onDescendantAdded(instance);
        
		rebuildGuiVector = true;
		if (GuiBase* gb = RBX::Instance::fastDynamicCast<GuiBase>(instance))
		{
			shared_ptr<GuiBase> sharedGb = shared_from(gb);
			propertyConnections[instance] = instance->propertyChangedSignal.connect(boost::bind(&GuiLayerCollector::descendantPropertyChanged,this,sharedGb, _1));
		}
    }
    void GuiLayerCollector::onDescendantRemoving(const shared_ptr<Instance>& instance)
    {
        Super::onDescendantRemoving(instance);
        
		rebuildGuiVector = true;

		if (Instance* rawInstance = instance.get())
		{
			propertyConnections[rawInstance].disconnect();
			propertyConnections.erase(rawInstance);
		}
    }

    void GuiLayerCollector::descendantPropertyChanged(const shared_ptr<GuiBase>& gb, const Reflection::PropertyDescriptor* descriptor)
    {
		if (descriptor == &GuiObject::prop_Visible || descriptor == &GuiObject::prop_ZIndex)
		{
			rebuildGuiVector = true;
		}
	}

    // Only render GuiBase objects that are below a PlayerGui
	void GuiLayerCollector::LoadZ(const shared_ptr<RBX::Instance>& instance, GuiLayers guiVectors[])
	{
		if (GuiBase* gb = instance->fastDynamicCast<GuiBase>())
		{
			if (gb->canProcessMeAndDescendants())
			{
				const int z = gb->getZIndex();
				const int queue = gb->getGuiQueue();
                
				RBXASSERT(queue < GUIQUEUE_COUNT);
				RBXASSERT(z <= GuiBase::maxZIndex());
                
				if (queue < GUIQUEUE_COUNT && (size_t)z < guiVectors[queue].size())
				{
					guiVectors[queue][z].push_back(shared_static_cast<GuiBase>(instance));
                    
                    if (instance->getChildren())
                    {
                        const Instances& children = *instance->getChildren();
                        
                        for (size_t i = 0; i < children.size(); ++i)
                            LoadZ(children[i], guiVectors);
                    }
				}
			}
		} 
		else if (Folder *fold = instance->fastDynamicCast<Folder>())
		{
			if (fold->getChildren())
            {
                const Instances& children = *fold->getChildren();
                        
                for (size_t i = 0; i < children.size(); ++i)
                    LoadZ(children[i], guiVectors);
            }
		}
	}
    
	void GuiLayerCollector::loadZVectors()
	{
		rebuildGuiVector = false;

		RBXASSERT(isAllGuiQueuesHaveCorrectNumberOfLayers(mGuiVectors));
		for (int queue = 0; queue < GUIQUEUE_COUNT; ++queue)
		{
			for (int z = 0; z <= GuiBase::maxZIndex(); ++z)
			{
				mGuiVectors[queue][z].clear();						// should do no memory realloc/shrinking - as it grows, no allocation
			}
		}
        
        if (getChildren())
        {
            const Instances& children = *getChildren();
            
            for (size_t i = 0; i < children.size(); ++i)
                LoadZ(children[i], mGuiVectors);
        }
	}


	// only children - check at each level - go top down (back to front)
	void GuiLayerCollector::render2d(Adorn* adorn)
	{
		render2dContext(adorn,NULL);
	}

	// only children - check at each level - go top down (back to front)
	void GuiLayerCollector::render2dContext(Adorn* adorn, const Instance* context)
	{
		if (rebuildGuiVector)
		{
			loadZVectors();
		}

		RBXASSERT(isAllGuiQueuesHaveCorrectNumberOfLayers(mGuiVectors));

		const Rect2D& viewport = adorn->getViewport();

		for (int z = 0; z <= GuiBase::maxZIndex(); ++z) 
		{
            render2dStandardGuiElements(adorn, context, mGuiVectors[GUIQUEUE_GENERAL][z], viewport);
            render2dTextGuiElements(adorn, context, mGuiVectors[GUIQUEUE_TEXT][z], viewport);
		}
	}

	void GuiLayerCollector::render2dStandardGuiElements(Adorn* adorn, const Instance* context, GuiVector& batch, const Rect2D& viewport)
	{
		// render backgrounds and borders
		for (size_t ii= 0; ii < batch.size(); ++ii) 
		{
			if(batch[ii])
			{
				if (batch[ii]->isVisible(viewport))
				{
					batch[ii]->renderBackground2dContext(adorn, context);
				}
			}
		}

		// render textures
		for (size_t ii = 0; ii < batch.size(); ++ii) 
		{
			if(batch[ii])
			{
				if (batch[ii]->isVisible(viewport))
				{
					batch[ii]->render2dContext(adorn, context);
				}
			}
		}
	}

	// calculating the correct size twice for text takes longer than the additional draw calls to intersperse background rendering with text rendering
	// don't split this up the way we do with other gui elements
	void GuiLayerCollector::render2dTextGuiElements(Adorn* adorn, const Instance* context, GuiVector& batch, const Rect2D& viewport)
	{
		for (size_t ii = 0; ii < batch.size(); ++ii) 
		{
			if(batch[ii])
			{
				if (batch[ii]->isVisible(viewport))
				{
					batch[ii]->render2dContext(adorn, context);
				}
			}
		}
	}
    
    GuiResponse GuiLayerCollector::doProcessGesture(const boost::shared_ptr<GuiBase>& guiBase, const UserInputService::Gesture& gesture, const shared_ptr<const RBX::Reflection::ValueArray>& touchPositions, const shared_ptr<const Reflection::Tuple>& args)
    {
        if (guiBase)
            return guiBase->processGesture(gesture, touchPositions, args);
        
        return GuiResponse::notSunk();
    }
    
    
    // todo: some copy/paste code here, but can't do boost::bind for performance
    GuiResponse GuiLayerCollector::processGesture(const UserInputService::Gesture& gesture, const shared_ptr<const RBX::Reflection::ValueArray>& touchPositions, const shared_ptr<const Reflection::Tuple>& args)
    {
        if ( gesture == UserInputService::GESTURE_NONE || !args)
            return GuiResponse::notSunk();

		if (rebuildGuiVector)
		{
			loadZVectors();
		}
        
		// do in reverse order - top, last first
        
		RBXASSERT(isAllGuiQueuesHaveCorrectNumberOfLayers(mGuiVectors));
        
		// loop over z-index then loop over queues within the current index
		for (int z = GuiBase::maxZIndex(); z >= 0; --z)
		{
			for (int queue = GUIQUEUE_COUNT - 1; queue >= 0; --queue)
			{
				GuiVector::reverse_iterator itr;
				for (itr = mGuiVectors[queue][z].rbegin(); itr != mGuiVectors[queue][z].rend(); ++itr)
				{
					if (*itr)
					{
                        GuiResponse answer = doProcessGesture(*itr,gesture,touchPositions,args);
                        
						if (answer.wasSunk())
						{
							return answer;
						}
					}
				}
			}
		}
		return GuiResponse::notSunk();
    }

	void GuiLayerCollector::getGuiObjectsForSelection(std::vector<GuiObject*>& guiObjects)
	{
		shared_ptr<GuiObject> selectedObject = shared_ptr<GuiObject>();
		if (GuiService* guiService = RBX::ServiceProvider::find<GuiService>(this))
		{
			selectedObject = shared_from(guiService->getSelectedGuiObject());
		}

		if (rebuildGuiVector)
		{
			loadZVectors();
		}

		RBXASSERT(isAllGuiQueuesHaveCorrectNumberOfLayers(mGuiVectors));

		ScrollingFrame* myAncestorScrollingFrame;
		if (selectedObject)
		{
			if ((myAncestorScrollingFrame = selectedObject->findFirstAncestorOfType<ScrollingFrame>()))
			{
				if (!myAncestorScrollingFrame->isCurrentlyVisible())
				{
					// if the scrollingFrame is not visible then throw it away
					myAncestorScrollingFrame = NULL;
				}
			}
		}

		// loop over z-index then loop over queues within the current index
		for (int z = GuiBase::maxZIndex(); z >= 0; --z) 
		{
			for (int queue = GUIQUEUE_COUNT - 1; queue >= 0; --queue)
			{
				for (GuiVector::reverse_iterator itr = mGuiVectors[queue][z].rbegin(); itr != mGuiVectors[queue][z].rend(); ++itr) 
				{
					if (selectedObject == *itr)
					{
						continue;
					}

					if (DFFlag::FixClippedScrollingFrameNavigation)
					{
						if ( GuiObject* guiObject = Instance::fastDynamicCast<GuiObject>((*itr).get()) )
						{
							if (guiObject->getSelectable() && (guiObject->isCurrentlyVisible() || (myAncestorScrollingFrame && guiObject->isDescendantOf(myAncestorScrollingFrame))))
							{
								guiObjects.push_back(guiObject);
							}
						}
					}
					else
					{
						ScrollingFrame* ancestorScrollingFrame = (*itr)->findFirstAncestorOfType<ScrollingFrame>();

						if ( GuiObject* guiObject = Instance::fastDynamicCast<GuiObject>((*itr).get()) )
						{
							if (guiObject->getSelectable() && (guiObject->isCurrentlyVisible() || (ancestorScrollingFrame && ancestorScrollingFrame->isCurrentlyVisible())) )
							{
								guiObjects.push_back(guiObject);
							}
						}
					}
				}
			}
		}
	}

	GuiResponse GuiLayerCollector::process(const shared_ptr<InputObject>& event, bool sinkIfMouseOver)
	{
		if (event->isMouseEvent() && event->getUserInputType() == InputObject::TYPE_MOUSEIDLE) 
		{
			return GuiResponse::notSunk();
		}
        
        GuiResponse guiResponse = processDescendants(event);
		
		if (    event->isMouseEvent()
			&&  !guiResponse.wasSunk()
			&&	guiResponse.getMouseWasOver()
			&&	sinkIfMouseOver )
		{
			GuiResponse answer = GuiResponse::sunk();
			answer.setTarget(guiResponse.getTarget().get());
			return answer;		// if mouse was over any player gui, don't pass along any further - sink it in the player gui
		}
		else
        {
			return guiResponse;
		}
	}

	void GuiLayerCollector::tryReleaseLastButtonDown(const shared_ptr<InputObject>& event)
	{
		if (event->isMouseUpEvent())
		{
			if(UserInputService* inputService = RBX::ServiceProvider::find<UserInputService>(this))
			{
				if (event->getUserInputType() == event->TYPE_MOUSEBUTTON1)
				{
					if (shared_ptr<GuiObject> object = inputService->getLastDownGuiObject(InputObject::TYPE_MOUSEBUTTON1).lock())
					{
						if (object->isDescendantOf(this))
						{
							object->setGuiState(Gui::NOTHING);
							GuiObject* none = NULL;
							inputService->setLastDownGuiObject(weak_from(none), InputObject::TYPE_MOUSEBUTTON1);
						}
					}
				}
				else if (event->getUserInputType() == event->TYPE_MOUSEBUTTON2)
				{
					if (shared_ptr<GuiObject> object = inputService->getLastDownGuiObject(InputObject::TYPE_MOUSEBUTTON2).lock())
					{
						if (object->isDescendantOf(this))
						{
							object->setGuiState(Gui::NOTHING);
							GuiObject* none = NULL;
							inputService->setLastDownGuiObject(weak_from(none), InputObject::TYPE_MOUSEBUTTON2);
						}
					}
				}
			}
		}
	}

	GuiResponse GuiLayerCollector::processDescendants(const shared_ptr<InputObject>& event)
	{
		if (rebuildGuiVector)
		{
			loadZVectors();
		}
        
        // do in reverse order - top, last first
        bool mouseWasOver = false;
		Instance* mouseOverGuiBase = NULL;
		int maxZIndex = -1;
        
        RBXASSERT(isAllGuiQueuesHaveCorrectNumberOfLayers(mGuiVectors));

        // loop over z-index then loop over queues within the current index
        for (int z = GuiBase::maxZIndex(); z >= 0; --z) 
        {
            for (int queue = GUIQUEUE_COUNT - 1; queue >= 0; --queue)
            {
                GuiVector::reverse_iterator itr;
                for (itr = mGuiVectors[queue][z].rbegin(); itr != mGuiVectors[queue][z].rend(); ++itr) 
                {
                    if((*itr))
                    {
                        GuiResponse answer = (*itr)->process(event);
                        if (answer.wasSunk()) 
						{
							if (!answer.getTarget())
							{
								answer.setTarget((*itr).get());
							}
							tryReleaseLastButtonDown(event);
                            return answer;
						}
                        if (answer.getMouseWasOver()) 
						{
							if (!answer.getTarget())
							{
								answer.setTarget((*itr).get());
							}
							if (maxZIndex < (*itr)->getZIndex())
							{
								maxZIndex = (*itr)->getZIndex();
								mouseOverGuiBase = answer.getTarget().get();
							}
                            mouseWasOver = true;
						}
                    }
                }
            }
        }

		tryReleaseLastButtonDown(event);

		GuiResponse answer = mouseWasOver ? GuiResponse::notSunkMouseWasOver() : GuiResponse::notSunk();
		answer.setTarget(mouseOverGuiBase);
        return answer;
	}


}
