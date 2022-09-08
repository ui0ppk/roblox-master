#include "GfxBase/GfxPart.h"

#include "v8datamodel/DataModelMesh.h"
#include "v8datamodel/Decal.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/BasicPartInstance.h"
#include "v8datamodel/ExtrudedPartInstance.h"
#include "v8datamodel/PrismInstance.h"
#include "v8datamodel/PyramidInstance.h"
#include "v8datamodel/FileMesh.h"
#include "v8datamodel/SpecialMesh.h"
#include "v8datamodel/BlockMesh.h"
#include "v8datamodel/CylinderMesh.h"
#include "v8datamodel/PartOperation.h"
#include "humanoid/Humanoid.h"

#include "v8world/Primitive.h"
#include "v8datamodel/PartCookie.h"

namespace
{
	void updateCookie(RBX::PartInstance* part)
	{
		if (part)
			part->setCookie(RBX::PartCookie::compute(part));
	}
}

namespace RBX
{	
	GfxBinding::~GfxBinding()
	{
		RBXASSERT(!isBound());
	}

		// connects property change event listeners.
	void GfxBinding::bindProperties(const shared_ptr<RBX::PartInstance>& part)
	{
		updateCookie(part.get());
		connections.push_back(part->combinedSignal.connect(boost::bind(&GfxPart::onCombinedSignal, this, _1, _2)));
		part->visitChildren(boost::bind(&GfxPart::onChildAdded, this, _1));
	}

		
	void GfxBinding::zombify()
	{
		unbind();

		invalidateEntity();
	}

	void GfxBinding::unbind()
	{
		if(partInstance)
		{
			partInstance->setGfxPart(NULL);
		}

		for(size_t i = 0; i < connections.size(); ++i)
		{
			connections[i].disconnect();
		}
		connections.clear();
	}

 
	bool GfxBinding::isBound()
	{ 
		return !connections.empty(); 
	}

	void GfxBinding::onChildAdded(const shared_ptr<Instance>& child)
	{
		if (DataModelMesh* specShape = Instance::fastDynamicCast<DataModelMesh>(child.get())) 
		{
			connections.push_back(specShape->propertyChangedSignal.connect(boost::bind(&GfxPart::onSpecialShapeChangedEx, this)));
			onSpecialShapeChangedEx();
		}
		else if (DecalTexture* tex = Instance::fastDynamicCast<DecalTexture>(child.get())) 
		{
			connections.push_back(tex->propertyChangedSignal.connect(boost::bind(&GfxPart::onTexturePropertyChanged, this, _1)));
			updateCookie(partInstance.get());
			invalidateEntity();
		}
		else if (Decal* decal = Instance::fastDynamicCast<Decal>(child.get())) 
		{
			connections.push_back(decal->propertyChangedSignal.connect(boost::bind(&GfxPart::onDecalPropertyChanged, this, _1)));
			updateCookie(partInstance.get());
			invalidateEntity();
		}
	}

	void GfxBinding::onChildRemoved(const shared_ptr<Instance>& child)
	{
		if (Instance::fastDynamicCast<DataModelMesh>(child.get())) 
		{
			// todo: need a disconnect for propertyChangedEvents...
			onSpecialShapeChangedEx();
		}
		else if (Instance::fastDynamicCast<DecalTexture>(child.get())) 
		{
			// todo: need a disconnect for propertyChangedEvents...
			updateCookie(partInstance.get());
			invalidateEntity();
		}
		else if (Instance::fastDynamicCast<Decal>(child.get())) 
		{
			// todo: need a disconnect for propertyChangedEvents...
			updateCookie(partInstance.get());
			invalidateEntity();
		}
	}

	bool GfxBinding::isInWorkspace(RBX::Instance* part)
	{
		Instance* ws = Workspace::findWorkspace(part);
		return ws && part->isDescendantOf(ws);
	}

	void GfxBinding::onAncestorChanged(const shared_ptr<Instance>& ancestor)
	{
		// Remove me from the scene if I am being removed from the Workspace
		if (partInstance && !isInWorkspace(partInstance.get()))
		{
			// will cause a delete on next updateEntity()
			zombify();
		}
		else
		{
			// part was removed from its ancestor (potentially deleted, or moved to a different cluster)
			updateCookie(partInstance.get());
			invalidateEntity();
		}
	}


	void GfxBinding::onSpecialShapeChangedEx()
	{
		updateCookie(partInstance.get());
		onSpecialShapeChanged();
	}


	void GfxBinding::onPropertyChanged(const Reflection::PropertyDescriptor* descriptor)
	{
		if (*descriptor==PartInstance::prop_CFrame)
		{
			onCoordinateFrameChanged();
		} 
		else if (*descriptor==PartInstance::prop_Anchored)
		{
			//partInstance->getGfxPart()->onClumpChanged();
		} 
		else if (*descriptor==PartInstance::prop_Size)
		{
			onSizeChanged();
		} 
		else if (*descriptor==PartInstance::prop_Transparency || *descriptor==PartInstance::prop_LocalTransparencyModifier) {
			onTransparencyChanged();
		}
		else if (*descriptor==PartInstance::prop_renderMaterial) {
			invalidateEntity();
		}
		else if (*descriptor==PartInstance::prop_Reflectance) {
			invalidateEntity();
		}
		else if (*descriptor==BasicPartInstance::prop_shapeXml) {
			invalidateEntity();
		}
		else if (*descriptor==ExtrudedPartInstance::prop_styleXml) {
			invalidateEntity();
		}
#ifdef _PRISM_PYRAMID_
		else if (*descriptor==PrismInstance::prop_sidesXML) {
			invalidateEntity();
		}		
		//else if (*descriptor==PrismInstance::prop_slices) {
		//	invalidateEntity();
		//}
		else if (*descriptor==PyramidInstance::prop_sidesXML) {
			invalidateEntity();
		}		
		//else if (*descriptor==PyramidInstance::prop_slices) {
		//	invalidateEntity();
		//}
#endif //_PRISM_PYRAMID_
		else if (Surface::isSurfaceDescriptor(*descriptor)) {
			invalidateEntity();
		}
		else if(*descriptor==PartInstance::prop_Color)
		{
			invalidateEntity();
		}
		else if (*descriptor == PartOperation::desc_MeshData)
		{
			invalidateEntity();
		}
		else if (*descriptor == PartOperation::desc_UsePartColor)
		{
			invalidateEntity();
		}
		else if (*descriptor == PartOperation::desc_FormFactor)
		{
			invalidateEntity();
		}
	}

	void GfxBinding::onTexturePropertyChanged(const Reflection::PropertyDescriptor* descriptor)
	{
		if (*descriptor==FaceInstance::prop_Face)
		{
			updateCookie(partInstance.get());
			invalidateEntity();
		}
		else if (*descriptor==DecalTexture::prop_Texture)
		{
			updateCookie(partInstance.get());
			invalidateEntity();
		}
		else if (*descriptor==DecalTexture::prop_Specular)
			invalidateEntity();
		else if (*descriptor==DecalTexture::prop_Shiny)
			invalidateEntity();
		else if (*descriptor==DecalTexture::prop_StudsPerTileU)
			invalidateEntity();
		else if (*descriptor==DecalTexture::prop_StudsPerTileV)
			invalidateEntity();
		else if (*descriptor==DecalTexture::prop_Transparency || *descriptor==Decal::prop_LocalTransparencyModifier )
			invalidateEntity();
	}

	void GfxBinding::onDecalPropertyChanged(const Reflection::PropertyDescriptor* descriptor)
	{
		if (*descriptor==FaceInstance::prop_Face)
		{
			updateCookie(partInstance.get());
			invalidateEntity();
		}
		else if (*descriptor==RBX::Decal::prop_Texture)
		{
			updateCookie(partInstance.get());
			invalidateEntity();
		}
		else if (*descriptor==RBX::Decal::prop_Specular)
			invalidateEntity();
		else if (*descriptor==RBX::Decal::prop_Shiny)
			invalidateEntity();
		else if (*descriptor==RBX::Decal::prop_Transparency || *descriptor==Decal::prop_LocalTransparencyModifier)
			invalidateEntity();
	}

	void GfxBinding::onCombinedSignal(Instance::CombinedSignalType type, const Instance::ICombinedSignalData* data)
	{
		switch(type)
		{
		case Instance::OUTFIT_CHANGED:
			onOutfitChanged();
			break;
		case Instance::HUMANOID_CHANGED:
			onHumanoidChanged();
			break;
		case Instance::CHILD_ADDED:
			onChildAdded(boost::polymorphic_downcast<const Instance::ChildAddedSignalData*>(data)->child);
			break;
		case Instance::CHILD_REMOVED:
			onChildRemoved(boost::polymorphic_downcast<const Instance::ChildRemovedSignalData*>(data)->child);
			break;
		case Instance::ANCESTRY_CHANGED:
			onAncestorChanged(boost::polymorphic_downcast<const Instance::AncestryChangedSignalData*>(data)->child);
			break;
		case Instance::PROPERTY_CHANGED:
			onPropertyChanged(boost::polymorphic_downcast<const Instance::PropertyChangedSignalData*>(data)->propertyDescriptor);
			break;
        default:
            break;
		}
	}

	void GfxBinding::onOutfitChanged()
	{
		invalidateEntity();
	}

	void GfxBinding::onHumanoidChanged()
	{
		updateCookie(partInstance.get());
		invalidateEntity();
	}

	void GfxBinding::cleanupStaleConnections()
	{
		for(size_t i = connections.size(); i != 0; --i)
		{
			if(!connections[i-1].connected())
			{
				connections.erase(connections.begin()+ (i -1)); // remove dead connection.
			}
		}
	}

	/*override*/ void GfxAttachment::unbind()
	{
		// nb: specifically ignore baseclass impl. we don't want to call setGfxPart.

		for(size_t i = 0; i < connections.size(); ++i)
		{
			connections[i].disconnect();
		}
		connections.clear();
	}

}
