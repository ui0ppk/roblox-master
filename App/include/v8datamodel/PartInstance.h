/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/PVInstance.h"
#include "V8DataModel/Surface.h"
#include "util/PartMaterial.h"
#include "V8World/Primitive.h"
#include "V8World/IMoving.h"
#include "V8World/MaterialProperties.h"
#include "V8DataModel/Camera.h"
#include "GfxBase/Part.h"
#include "GfxBase/IAdornable.h"
#include "Util/CameraSubject.h"
#include "Util/Selectable.h"
#include "Util/BrickColor.h"
#include "Util/SystemAddress.h"
#include "Util/Faces.h"
#include "Util/PathInterpolatedCFrame.h"
#include "Util/CompactEnum.h"
#include "rbx/rbxTime.h"
#include "RBX/Intrusive/Set.h"
#include "Reflection/Property.h"
#include "Util/Average.h"
#include <vector>
#include <boost/unordered_set.hpp>

LOGGROUP(TouchedSignal)

namespace RBX {

class PartAttribute;
class World;
class Primitive;
class Assembly;
class GfxPart;
class Clump;
class TouchTransmitter;
class SurfaceGui;
class Humanoid;
class Workspace;
struct RootPrimitiveOwnershipData;

typedef RBX::Intrusive::Set< class PartInstance, class PhysicsService >::Hook PhysicsServiceHook;

extern const char* const sPart;
class PartInstance 
	: public DescribedNonCreatable<PartInstance, PVInstance,sPart>
	, public IMoving
	, public IAdornable
	, public CameraSubject
	, public Diagnostics::Countable<PartInstance>
	, public PhysicsServiceHook
{
private:
	typedef DescribedNonCreatable<PartInstance, PVInstance,sPart> Super;
	friend class Surface;
	friend class Surfaces;
public:
    static bool showContactPoints;
    static bool showJointCoordinates;
	static bool highlightSleepParts;
	static bool highlightAwakeParts;
	static bool showBodyTypes;
	static bool showAnchoredParts;
	static bool showPartCoordinateFrames;
	static bool showUnalignedParts;
	static bool showSpanningTree;
	static bool showMechanisms;
	static bool showAssemblies;
	static bool showReceiveAge;
	static bool disableInterpolation;
	static bool showInterpolationPath;

	float getReceiveInterval() const;

	enum FormFactor
    {
        SYMETRIC = 0,
        BRICK,
        PLATE,
        CUSTOM
    };

	enum LegacyPartType
	{
		BALL_LEGACY_PART = 0,
		BLOCK_LEGACY_PART,
		CYLINDER_LEGACY_PART,
		SPECIAL_LEGACY_PART
	};

	static float plateHeight()	{return 0.4f;}
	static float brickHeight()	{return 1.2f;}

	static float cameraTransparentDistance()	{return 3.0f;}
	static float cameraTranslucentDistance()	{return 6.0f;}

	void resetNetworkOwnerTime(double extraTime);
	bool networkOwnerTimeUp() const;
	
	virtual shared_ptr<const Instances> getTouchingParts();

	// util for below functions
	static bool partIsLegacyCustomPhysProperties(const PartInstance* part);
	// conversion functions for insert and dataModel load
	static void convertToNewPhysicalPropRecursive(RBX::Instance* instance);
	static bool instanceOrChildrenContainsNewCustomPhysics(RBX::Instance* instance);


	// Used to - judiciously - extend data into PartInstance
	boost::uint64_t raknetTime;

private:
	Vector3 xmlToUiSize(const Vector3& xmlSize) const;
    
	boost::scoped_ptr<Primitive> primitive;			// engine object

	GfxPart*					gfxPart;			// simple link to graphics object.
	unsigned int cookie;
	boost::scoped_ptr<PathInterpolatedCFrame>		renderingCoordinateFrame;
	Time						networkOwnerTime;	// prevent too much chatter

	// streamable data stored in Primitive
	// friction, elasticity, partSize, position, velocity

	// Streamable data

protected:
	BrickColor color;
	float transparency;
	float reflectance;
	float localTransparencyModifier;
	bool cachedNetworkOwnerIsSomeoneElse;
	bool locked;
	CompactEnum<FormFactor, uint8_t> formFactor;
	CompactEnum<LegacyPartType, uint8_t> legacyPartType;

private:
	bool computeSurfacesNeedAdorn() const;

protected:
	void safeMove();			// call PVInstance::safeMove

public:
	rbx::remote_signal<void(RBX::SystemAddress)>* getOrCreateNetworkOwnerChangedSignal(bool create);

	//helper function
	static void printNetAPIDisabledMessage();

	// Networking - streaming, no UI
	static const Reflection::PropDescriptor<PartInstance, RBX::SystemAddress> prop_NetworkOwner;
	static const Reflection::PropDescriptor<PartInstance, bool> prop_NetworkIsSleeping;
	
    static const Reflection::PropDescriptor<PartInstance, RBX::SystemAddress> prop_ConfirmedNetworkOwner;
    static const Reflection::PropDescriptor<PartInstance, RBX::SystemAddress> prop_TransitionalNetworkOwner;
	
	// debug - HIDDEN_SCRIPTING
	static const Reflection::PropDescriptor<PartInstance, float> prop_ReceiveAge;

	// 
	static const Reflection::PropDescriptor<PartInstance, Color3> prop_Color;
	static const Reflection::PropDescriptor<PartInstance, BrickColor> prop_BrickColor;
	static const Reflection::PropDescriptor<PartInstance, float> prop_Transparency;
	static const Reflection::PropDescriptor<PartInstance, float> prop_LocalTransparencyModifier;
	static const Reflection::EnumPropDescriptor<PartInstance, PartMaterial> prop_renderMaterial;
	static const Reflection::PropDescriptor<PartInstance, float> prop_Reflectance;
	static const Reflection::PropDescriptor<PartInstance, bool> prop_Locked;
	static const Reflection::PropDescriptor<PartInstance, bool> prop_CanCollide;
	static const Reflection::PropDescriptor<PartInstance, bool> prop_Anchored;
	static const Reflection::PropDescriptor<PartInstance, Vector3> prop_Size;
	static const Reflection::PropDescriptor<PartInstance, CoordinateFrame> prop_CFrame;
	static const Reflection::PropDescriptor<PartInstance, float> prop_Elasticity;
	static const Reflection::PropDescriptor<PartInstance, float> prop_Friction;
	static const Reflection::PropDescriptor<PartInstance, PhysicalProperties> prop_CustomPhysicalProperties;
	static const Reflection::PropDescriptor<PartInstance, Vector3> prop_Velocity;
	static const Reflection::PropDescriptor<PartInstance, Vector3> prop_RotVelocity;

	class TouchedSignal : public rbx::signal<void(shared_ptr<Instance>)>
	{
		private:
			typedef rbx::signal<void(shared_ptr<Instance>)> Super;

		class TouchedSlot
		{
			boost::function<void(shared_ptr<Instance>)> inner;
			boost::weak_ptr<PartInstance> const source;

		public:
			TouchedSlot(const boost::function<void(shared_ptr<Instance>)>& inner, PartInstance* source):inner(inner),source(shared_from(source))
			{
				source->incrementTouchedSlotCount();
			}
			~TouchedSlot()
			{
				shared_ptr<PartInstance> source = this->source.lock();
				if (source)
					source->decrementTouchedSlotCount();
			}
			void operator()(shared_ptr<Instance> instance) { inner(instance); }

			TouchedSlot(const TouchedSlot& other) : inner(other.inner), source(other.source)
			{
				shared_ptr<PartInstance> source = this->source.lock();
				if (source)
					source->incrementTouchedSlotCount();
			}
			
			TouchedSlot& operator=(const TouchedSlot& other);
		};

	public:
		PartInstance* part;
		template<typename F>
		rbx::signals::connection connect(F slot)
		{
			FASTLOG1(FLog::TouchedSignal, "Signal connected (no upper message = lua) - %p", this);
			rbx::signals::connection con = Super::connect(TouchedSlot(slot, part));
			if(FLog::TouchedSignal)
			{
				FASTLOG1(FLog::TouchedSignal, "Signal connected - %p", this);
				con.flogPrint();
				Super::flogPrint();
			}

			return con;
		}

		void operator()(shared_ptr<Instance> instance)
		{
			if(FLog::TouchedSignal)
				Super::flogPrint();
			Super::operator()(instance);
		}
	};

	class OnDemandPartInstance : public OnDemandInstance, public Allocator<OnDemandPartInstance>
	{
	public:
		OnDemandPartInstance(PartInstance* owner);
		void *operator new(size_t sz) 
		{ 
			return Allocator<OnDemandPartInstance>::operator new(sz);
		}
		void operator delete(void*p) 
		{ 
			return Allocator<OnDemandPartInstance>::operator delete(p);
		}

		rbx::signal<void(bool)> sleepingChangedSignal;
        rbx::signal<void(shared_ptr<Instance>)> clumpChangedSignal;

		// Fired for humanoid state when CFrame changed from reflection
		rbx::signal<void()> cframeChangedFromReflectionSignal;

		TouchTransmitter* touchTransmitter;
		rbx::atomic<int> touchedSlotCount;
		TouchedSignal touchedSignal;
		TouchedSignal touchEndedSignal;

		rbx::signal<void(shared_ptr<Instance>)> deprecatedStoppedTouchingSignal;
		rbx::signal<void()> outfitChangedSignal;

		rbx::signal<void(shared_ptr<Instance>)> localSimulationTouchedSignal;
        rbx::signal<void(bool)> buoyancyChangedSignal;

        rbx::signal<void(PartInstance* , CoordinateFrame&, RBX::Velocity&, float&)> onPositionUpdatedByNetworkSignal;

		bool isCurrentlyStreamRemovingPart;
		std::vector< weak_ptr<SurfaceGui> > surfaceGuiCookies;
	};

	void reportTouch(const shared_ptr<PartInstance>& other);
	void reportUntouch(const shared_ptr<PartInstance>& other);

	TouchedSignal* getOrCreateTouchedSignal(bool create = true) { return (onDemandRead() || create) ? &onDemandWrite()->touchedSignal : NULL; }
	TouchedSignal* getOrCreateTouchedEndedSignal(bool create = true) { return (onDemandRead() || create) ? &onDemandWrite()->touchEndedSignal : NULL; }
	rbx::signal<void(shared_ptr<Instance>)>* getOrCreateDeprecatedStoppedTouchingSignal(bool create = true) { return (onDemandRead() || create) ? &onDemandWrite()->deprecatedStoppedTouchingSignal : NULL; }
	rbx::signal<void(shared_ptr<Instance>)>* getOrCreateLocalSimulationTouchedSignal(bool create = true) { return (onDemandRead() || create) ? &onDemandWrite()->localSimulationTouchedSignal : NULL; }
	rbx::signal<void()>* getOrCreateOutfitChangedSignal(bool create = true) { return (onDemandRead() || create) ? &onDemandWrite()->outfitChangedSignal : NULL; }

	static bool nonNullInWorkspace(const shared_ptr<PartInstance>& part);

	//PartInstance();								
	PartInstance(const Vector3& initialSize = Vector3(4.0f, 1.2f, 2.0f));
	virtual ~PartInstance();

	virtual bool hasThreeDimensionalSize()          {return true;}

	virtual PartType getPartType() const			{return BLOCK_PART; }
	virtual const Vector3& getMinimumUiSize() const {static Vector3 minSize(1.0f,1.0f,1.0f); return minSize;}
	virtual const Vector3& getMinimumUiSizeCustom() const {static Vector3 minSize(0.2f,0.2f,0.2f); return minSize;}
	virtual Faces getResizeHandleMask() const		{return Faces(NORM_ALL_MASK); }
	Primitive* getPartPrimitive()					{return primitive.get();}
	const Primitive* getConstPartPrimitive() const	{return primitive.get();}
    virtual const Primitive* getConstPartPrimitiveVirtual() const {return primitive.get();}
	virtual bool getDragUtilitiesSupport() const { return true; }

	GfxPart* getGfxPart() const { return gfxPart; };
	void setGfxPart(GfxPart* p) { gfxPart = p; };

	unsigned int getCookie() const { return cookie; };
	void setCookie(unsigned int cookie) { this->cookie = cookie; };

	virtual void humanoidChanged();

    Humanoid* findAncestorModelWithHumanoid();

	const std::vector< weak_ptr<SurfaceGui> >* getSurfaceGuiCookiesRead() const  { return onDemandRead()? &onDemandRead()->surfaceGuiCookies : 0; }
	std::vector< weak_ptr<SurfaceGui> >* getSurfaceGuiCookiesWrite()             { return &onDemandWrite()->surfaceGuiCookies; }

	static Primitive* getPrimitive(PartInstance* p)						{return p ? p->getPartPrimitive() : NULL;}
	static const Primitive* getConstPrimitive(const PartInstance* p)	{return p ? p->getConstPartPrimitive() : NULL;}

	static PartInstance* fromPrimitive(Primitive* p);
	static const PartInstance* fromConstPrimitive(const Primitive* p);

	Clump* getClump();
	const Clump* getConstClump() const;

	static PartInstance* fromAssembly(Assembly* a);
	static const PartInstance* fromConstAssembly(const Assembly* a);

	static void primitivesToParts(const G3D::Array<Primitive*>& primitives,
									std::vector<shared_ptr<PartInstance> >& parts);

	static void findParts(	Instance* instance, 	
							std::vector<weak_ptr<PartInstance> >& parts);


	///////////////////////////////////////////////////////////////////////////////////
	// STREAMABLE access functions for PartInstance
	//
	// Data stored in the Primitive
	//

private:
	bool resizeImpl(NormalId localNormalId, int amount);
	bool advResizeImpl(NormalId localNormalId, float amount, bool checkIntersection = true);
    void getConnectedPartsRecursiveImpl(shared_ptr<Instances> &objs, boost::unordered_set<PartInstance*> &seen) const;

	void gatherResetOwnershipDataPreAnchor(std::vector<Primitive*>& futureMechRoots, RootPrimitiveOwnershipData& ownershipData);
	void updateOwnershipSettingsPostAnchor(std::vector<Primitive*>& futureMechRoots, RootPrimitiveOwnershipData& ownershipData);
	void gatherResetOwnershipDataPreUnanchor(std::vector<Primitive*>& previousOwnershipRoots);
	void updateOwnershipSettingsPostUnAnchor(std::vector<Primitive*>& previousOwnershipRoots);
	void updateResetOwnershipSettingsOnUnAnchor();


public:
//private:	THESE WANT TO BE PRIVATE, BUT CAN"T BE
	virtual void setPartSizeUi(const Vector3& uiSize);
	void setPartSizeUnjoined(const Vector3& uiSize);
	Vector3 getPartSizeUi() const;
	virtual void setTranslationUi(const Vector3& set);
	virtual const Vector3& getTranslationUi() const;
    virtual void setRotationUi(const Vector3& set);
	virtual const Vector3 getRotationUi() const;

	SurfaceType getSurfaceType(NormalId surfId) const;
	void setSurfaceType(NormalId surfId, SurfaceType type);

	LegacyController::InputType getInput(NormalId surfId) const;
	void setSurfaceInput(NormalId surfId, LegacyController::InputType inputType);

	float getParamA(NormalId surfId) const;
	void setParamA(NormalId surfId, float paramA);

	float getParamB(NormalId surfId) const;
	void setParamB(NormalId surfId, float paramB);

	bool isStandardPart() const;

protected:
	void refreshPartSizeUi();
	virtual Vector3 uiToXmlSize(const Vector3& uiSize) const;
	bool hasLegacyOffset();
	virtual OnDemandInstance* initOnDemand();
public:
	const OnDemandPartInstance* onDemandRead() const { return static_cast<const OnDemandPartInstance*>(Super::onDemandRead()); };
	OnDemandPartInstance* onDemandWrite() { return static_cast<OnDemandPartInstance*>(Super::onDemandWrite()); };
	virtual void setPartSizeXml(const Vector3& rbxSize);
	const Vector3& getPartSizeXml() const;

	virtual int getResizeIncrement() const { return 1; }
	virtual float getMinimumResizeIncrement() const { return 0.01f; }
	virtual float getMinimumYDimension() const;
	virtual float getMinimumXOrZDimension() const;
	virtual bool resize(NormalId localNormalId, int amount);
	bool resizeFloat(NormalId localNormalId, float amount, bool checkIntersection = true);

	virtual FormFactor getFormFactor() const { return SYMETRIC; }

	// these function should be called from network
	// "timeStamp" parameter used for the actual time the change happened, used for interpolation
	void addInterpolationSample(const CoordinateFrame& cf, const Velocity& vel, const RemoteTime& timeStamp, Time now, float localTimeOffset = 0.f, int numNodesAhead = 0);
	void setInterpolationDelay(float value);

	// these function should be called from setCoordinateFrame()
	void setPhysics(const PV& pv);
	void setPhysics(const CoordinateFrame& cf);

	virtual void setCoordinateFrame(const CoordinateFrame& value);
	void setCoordinateFrameRoot(const CoordinateFrame& value);
	const CoordinateFrame& getCoordinateFrame() const;

	CoordinateFrame calcRenderingCoordinateFrame();		// calculates a new interpolated render position
	CoordinateFrame calcRenderingCoordinateFrame(const Time& t);

	float getMass() const;
	int getMassScript(lua_State* L);
	Vector3 getPrincipalMoment() const;
	const Velocity& getVelocity() const;

	virtual void setLinearVelocity(const Vector3& set);
	const Vector3& getLinearVelocity() const;

	virtual void setRotationalVelocity(const Vector3& set);
	void setRotationalVelocityRoot(const Vector3& set);
	const Vector3& getRotationalVelocity() const;

	virtual void setFriction(float friction);
	float getFriction() const;

	virtual void setElasticity(float elasticity);
	float getElasticity() const;

	virtual void setPhysicalProperties(const PhysicalProperties& prop);
	const PhysicalProperties& getPhysicalProperties() const;

	float getSpecificGravity() const;

	virtual Surface getSurface(const RbxRay& gridRay, int& surfaceId);

	bool getCanCollide() const;
	virtual void setCanCollide(bool value);

	bool getAnchored() const;
	virtual void setAnchored(bool value);

	bool getDragging() const;
	void setDragging(bool value);

    
    bool isGrounded();
    shared_ptr<const Instances> getConnectedParts(bool recursive);
	shared_ptr<Instance> getRootPart();

	/////////////////////////////////////////////////////////////////////
	//
	virtual void join();			// functions - try to join or unjoin a part
	void destroyImplicitJoints();
	virtual void destroyJoints();

	///////////////////////////////////////////////////////////////////////
	// Data stored in the PartInstance
	//
	const RBX::SystemAddress getNetworkOwner() const;
	void setNetworkOwner(const RBX::SystemAddress value);
	void setNetworkOwnerNotifyIfServer(const RBX::SystemAddress value, bool ownershipBecomingManual);
	void setNetworkOwnershipRuleIfServer(NetworkOwnership value);
	shared_ptr<Instance> getNetworkOwnerScript();
    void setNetworkOwnerScript(const shared_ptr<Instance> playerInstance);
	bool getNetworkOwnershipAuto();
	void setNetworkOwnershipAuto();
	shared_ptr<const Reflection::Tuple> canSetNetworkOwnershipScript();
	bool canSetNetworkOwnership(Primitive*& rootPrimitive, std::string& statusMessage);
	void setNetworkOwnerAndNotify(const RBX::SystemAddress value);
	void notifyNetworkOwnerChanged(const RBX::SystemAddress oldOwner);
#ifdef RBX_TEST_BUILD
	rbx::signal<void(float, float)> ownershipChangeSignal;
#endif

	bool getNetworkIsSleeping() const;
	void setNetworkIsSleeping(bool value);

	PartMaterial getRenderMaterial() const;
	virtual void setRenderMaterial(PartMaterial value);

	void setNetworkOwnershipManualReplicate(bool value);
	const bool getNetworkOwnershipManualReplicate() const;

	void setNetworkOwnershipRule(NetworkOwnership value);
	const NetworkOwnership getNetworkOwnershipRule() const;

	float getTransparencyXml() const		{return transparency;}
	float getTransparencyUi() const			{return (1.0f - ((1.0f - localTransparencyModifier) * (1.0f - getTransparencyXml()))); }
	bool getIsTransparent() const			{return getTransparencyUi() > 0.1f;}
	virtual void setTransparency(float value);
	void setLocalTransparencyModifier(float value);
	float getLocalTransparencyModifier() const			{return localTransparencyModifier;}
	float getReflectance() const			{return reflectance;}// if(transparency != 0) return this->reflectance; else return 0; }
	virtual void setReflectance(float value);

	BrickColor getColor() const				{return color;}
	virtual void setColor(BrickColor value);

	Color3 getColor3() const				{return getColor().color3();}
	void setColor3(const Color3& value) {
		setColor(BrickColor::closest(value));
	}

	bool getPartLocked() const				{return locked;}
	virtual void setPartLocked(bool value);

	void incrementTouchedSlotCount();
	void decrementTouchedSlotCount();

	///////////////////////////////////////////////////////////////////////
	//
	// Utilities

	// used for SetNetworkOwnership API

	static bool isPlayerCharacterPart(PartInstance* part);

	static void checkConsistentOwnerAndRuleResetRoots(RBX::SystemAddress& consistentAddress, bool& hasConsistentOwner, bool& hasConsistentManualOwnershipRule, std::vector<Primitive*>& primRoots);

	// version of PartInstance used by Draw.cpp
	Part getPart();

	static bool getLocked(Instance* instance);

	static void setLocked(Instance* instance, bool value);

	float alpha() const {return (1.0f - getTransparencyUi());}

	bool lockedInPlace() const;		// true if you could not normally "pull" this part out

	bool aligned(bool useGlobalGridSetting = false) const;			// true if aligned to the 0.1 grid and rotated to xyz

	CoordinateFrame worldSnapLocation() const;

	bool computeNetworkOwnerIsSomeoneElse() const;
	bool isProjectile() const;

	virtual void onChildRemoved(Instance* child);
	void updatePrimitiveState();
	void setIsCurrentlyStreamRemovingPart();

	////////////////////////////////////////////////////////////////////////
	// IMoving
	//
	/*implement*/ void onSleepingChanged(bool sleeping);
	/*implement*/ bool reportTouches() const;
	/*implement*/ void onClumpChanged();
	/*implement*/ void onNetworkIsSleepingChanged(Time now);			// callback to PartInstance from Primitive 
	/*implement*/ void onBuoyancyChanged( bool value );			// callback to PartInstance from Primitive
	/*implement*/ bool isInContinousMotion();			// callback to PartInstance from Primitive

	void fireOutfitChanged();

	////////////////////////////////////////////////////////////////////////
	// Instance
	//
	/*override*/ void onAncestorChanged(const AncestorChanged& event);
	/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
	/*override*/ bool askSetParent(const Instance* instance) const;
	/*override*/ void onGuidChanged();
	/*override*/ void setName(const std::string& value);
	/*override*/ int getPersistentDataCost() const 
	{
		return Super::getPersistentDataCost() + 10;
	}
    /*override*/ void processRemoteEvent(const Reflection::EventDescriptor& descriptor, const Reflection::EventArguments& args, const SystemAddress& source);

	////////////////////////////////////////////////////////////////////////
	// IHasLocation
	//
	/*override*/ const CoordinateFrame getLocation() {return getCoordinateFrame();}

	////////////////////////////////////////////////////////////////////////
	// CameraSubject
	//
	/*override*/ const CoordinateFrame getRenderLocation() {return calcRenderingCoordinateFrame();}
	/*override*/ const Vector3 getRenderSize() { return getPartSizeUi(); }
	/*override*/ void onCameraNear(float distance);
	/*override*/ virtual void getCameraIgnorePrimitives(std::vector<const Primitive*>& primitives) {primitives.push_back(getPartPrimitive());}

	////////////////////////////////////////////////////////////////////////
	// IAdornable3d
	//
	/*override*/ virtual bool shouldRender3dAdorn() const;
	/*override*/ virtual void render3dAdorn(Adorn* adorn);
	/*override*/ virtual void render3dSelect(Adorn* adorn, SelectState selectState);

	////////////////////////////////////////////////////////////////////////
	// IHasPrimaryPart
	//
	/*implement*/ PartInstance* getPrimaryPart() {return this;}

	////////////////////////////////////////////////////////////////////////
	// overrides from PVInstance
	//
	/*override*/ bool hitTestImpl(const RbxRay& worldRay, Vector3& worldHitPoint);
	/*override*/ Extents computeExtentsWorld() const;

	bool containedByFrustum(const RBX::Frustum& frustum) const;
    bool intersectFrustum(const RBX::Frustum& frustum) const;
	
private:
	void removeTouchTransmitter();
	void addTouchTransmitter();
	bool hasTouchTransmitter() const { return onDemandRead() && onDemandRead()->touchTransmitter;  }

	void updateHumanoidCookie();

	void onPVChangedFromReflection();

	void createRenderingCoordinateFrame();
	CoordinateFrame calcRenderingCoordinateFrameImpl(const Time& t);
	CoordinateFrame computeRenderingCoordinateFrame(PartInstance* mechanismRootPart, const Time& t);

	bool getIsCurrentlyStreamRemovingPart() const;

	////////////////////////////////////////////////////////////////////////
	// PartInstance
	//
	void onSurfaceChanged(NormalId surfId);	// callback from Surface

	// TODO: Nuke this when we refactor Surface
	void raiseSurfacePropertyChanged(const Reflection::PropertyDescriptor& desc) {
		this->raisePropertyChanged(desc);
	}

	static float	defaultFriction()			{return 0.3f;}
	static float	defaultElasticity()			{return 0.5f;}
};

} // namespace
