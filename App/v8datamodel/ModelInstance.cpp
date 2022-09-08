/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/ModelInstance.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/IModelModifier.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/GameBasicSettings.h"
#include "Humanoid/Humanoid.h"
#include "V8World/Primitive.h"
#include "Tool/DragUtilities.h"
#include "AppDraw/Draw.h"
#include "AppDraw/DrawAdorn.h"
#include "Util/Math.h"

FASTFLAG(StudioDE6194FixEnabled)

DYNAMIC_FASTFLAGVARIABLE(CacheModelExtents, false)

namespace RBX {

bool ModelInstance::showModelCoordinateFrames = false;
G3D::Color4 ModelInstance::sPrimaryPartSelectColor = G3D::Color3::gray();
G3D::Color4 ModelInstance::sPrimaryPartHoverOverColor = G3D::Color3::gray();
float ModelInstance::sPrimaryPartLineThickness = 0.04;

const char* const sModel = "Model";

// Declare some dictionary entries
static const RBX::Name& partName = RBX::Name::declare(sModel);

////////////////////////////////////////////////////////////////////////////////////

REFLECTION_BEGIN();
static Reflection::PropDescriptor<ModelInstance, CoordinateFrame> desc_ModelInPrimary("ModelInPrimary", category_Data, &ModelInstance::getModelInPrimary, &ModelInstance::setModelInPrimary, Reflection::PropertyDescriptor::STREAMING);
static Reflection::RefPropDescriptor<ModelInstance, PartInstance> desc_primaryPartSetByUserProp("PrimaryPart", category_Data, &ModelInstance::getPrimaryPartSetByUser, &ModelInstance::setPrimaryPartSetByUser);

static Reflection::BoundFuncDesc<ModelInstance, void()> desc_BreakJoints(&ModelInstance::breakJoints, "BreakJoints", Security::None);
static Reflection::BoundFuncDesc<ModelInstance, void()> desc_makeJoints(&ModelInstance::makeJoints, "MakeJoints", Security::None);

static Reflection::BoundFuncDesc<ModelInstance, G3D::Vector3()> model_getExtents(&ModelInstance::calculateModelSize, "GetExtentsSize", Security::None);

static Reflection::BoundFuncDesc<ModelInstance, void(G3D::Vector3)> model_translateByFunc(&ModelInstance::translateBy, "TranslateBy", "delta", Security::None);

static Reflection::BoundFuncDesc<ModelInstance, CoordinateFrame()> model_getPrimaryCFrame(&ModelInstance::getPrimaryCFrame, "GetPrimaryPartCFrame", Security::None);
static Reflection::BoundFuncDesc<ModelInstance, void(CoordinateFrame)> model_setPrimaryCFrame(&ModelInstance::setPrimaryCFrame, "SetPrimaryPartCFrame", "cframe", Security::None);

static Reflection::BoundFuncDesc<ModelInstance, void(G3D::Vector3)> model_moveFunction(&ModelInstance::moveToPointAndJoin, "MoveTo", "position", Security::None);

///////////////////////////////////////////////////////////////////////////////
// Deprecated Functions
static Reflection::BoundFuncDesc<ModelInstance, G3D::Vector3()> model_getModelSize(&ModelInstance::calculateModelSize, "GetModelSize", Security::None, Reflection::Descriptor::Attributes::deprecated(model_getExtents));
static Reflection::BoundFuncDesc<ModelInstance, CoordinateFrame()> model_getModelCFrame(&ModelInstance::calculateModelCFrame, "GetModelCFrame", Security::None, Reflection::Descriptor::Attributes::deprecated(model_getPrimaryCFrame));
static Reflection::BoundFuncDesc<ModelInstance, void()> desc_makeJointsOld(&ModelInstance::makeJoints, "makeJoints", Security::None, Reflection::Descriptor::Attributes::deprecated(desc_makeJoints));
static Reflection::BoundFuncDesc<ModelInstance, void()> dep_breakJoints(&ModelInstance::breakJoints, "breakJoints", Security::None, Reflection::Descriptor::Attributes::deprecated(desc_BreakJoints));
static Reflection::BoundFuncDesc<ModelInstance, void(G3D::Vector3)> dep_moveFunction(&ModelInstance::moveToPointAndJoin, "moveTo", "location", Security::None, Reflection::Descriptor::Attributes::deprecated(model_moveFunction));
static Reflection::BoundFuncDesc<ModelInstance, void(G3D::Vector3)> model_moveFunctionOld(&ModelInstance::moveToPointAndJoin, "move", "location", Security::None, Reflection::Descriptor::Attributes::deprecated(model_moveFunction));
static Reflection::BoundFuncDesc<ModelInstance, void()> model_setIdentityOrientation(&ModelInstance::setIdentityOrientation, "SetIdentityOrientation", Security::None, Reflection::Descriptor::Attributes::deprecated(model_setPrimaryCFrame));
static Reflection::BoundFuncDesc<ModelInstance, void()> model_resetOrientationToIdentity(&ModelInstance::resetOrientationToIdentity, "ResetOrientationToIdentity", Security::None, Reflection::Descriptor::Attributes::deprecated(model_setPrimaryCFrame));
REFLECTION_END();

ModelInstance::ModelInstance() 
	:DescribedCreatable<ModelInstance, PVInstance, sModel>("Model")
	,computedPrimaryPart(NULL)
	,lockedName(false)
	,primaryPartSetByUser()
	,modelCFrame()
	,modelSize(0,0,0)
	,needsSizeRecalc(true)
	,needsCFrameRecalc(true)
{
}

ModelInstance::~ModelInstance()
{
}

void ModelInstance::hackPhysicalCharacter()
{
	// This hack is for handling very old legacy files that have class type called "PhysicalCharacter"
	const ICreator* c = &static_getCreator();
	const Name* name = &Name::declare("PhysicalCharacter");
	Creatable<Instance>::getCreators()[name] = c;
}


void ModelInstance::setName(const std::string& value)
{
	if(lockedName){
		RBX::Security::Context::current().requirePermission(RBX::Security::WritePlayer, "set a Character's name");
	}
	Super::setName(value);
}

static void Translate(shared_ptr<Instance> instance, const Vector3* offset)
{
	if(PartInstance* part = Instance::fastDynamicCast<PartInstance>(instance.get())){
		CoordinateFrame cframe = part->getCoordinateFrame();
		cframe.translation += (*offset);
		part->setCoordinateFrame(cframe);
	}
}
void ModelInstance::translateBy(Vector3 offset)
{
	visitDescendants(boost::bind(&Translate, _1, &offset));
}

CoordinateFrame ModelInstance::getPrimaryCFrame()
{
	if (PartInstance* primaryPart = getPrimaryPartSetByUser())
	{
		return primaryPart->getCoordinateFrame();
	}
	
	throw std::runtime_error("Model:GetPrimaryCFrame() failed because no PrimaryPart has been set, or the PrimaryPart no longer exists. Please set Model.PrimaryPart before using this.");
}

static void setCFramePart(shared_ptr<Instance> instance, const CoordinateFrame& rotationCoord, const Vector3& deltaPos, const Vector3& centerPoint, const PartInstance* primaryPart)
{
	if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(instance.get()))
	{
		if (part == primaryPart)
		{
			return;
		}
		
		CoordinateFrame partCFrame = part->getCoordinateFrame();

		partCFrame.translation += deltaPos;

		partCFrame.translation -= centerPoint;	// move center to origin
		partCFrame = rotationCoord * partCFrame;			// rotate
		partCFrame.translation += centerPoint;	// move back

		if(!partCFrame.rotation.isOrthonormal())
		{
			partCFrame.rotation.orthonormalize();
		}

		part->setCoordinateFrame(partCFrame);
	}
}

void ModelInstance::setPrimaryCFrame(CoordinateFrame newCFrame)
{
	if (PartInstance* primaryPart = getPrimaryPartSetByUser())
	{
		const CoordinateFrame oldCFrame = primaryPart->getCoordinateFrame();

		if(!newCFrame.rotation.isOrthonormal())
		{
			newCFrame.rotation.orthonormalize();
		}

		if (oldCFrame == newCFrame) // no reason to do this if it is the same
		{
			return;
		}

		CoordinateFrame deltaCFrame;
		deltaCFrame.translation = newCFrame.translation - oldCFrame.translation;
		deltaCFrame.rotation = newCFrame.rotation * oldCFrame.rotation.transpose();

		const CoordinateFrame rotationCoord(deltaCFrame.rotation,Vector3::zero());
		visitDescendants(boost::bind(&setCFramePart, _1, boost::ref(rotationCoord), boost::ref(deltaCFrame.translation), boost::ref(newCFrame.translation), primaryPart));

		// update primary part separately, since it may not be a descendant of this model
		setCFramePart(shared_from(primaryPart),rotationCoord,deltaCFrame.translation, newCFrame.translation, NULL);
	}
	else
	{
		throw std::runtime_error("Model:SetPrimaryCFrame() failed because no PrimaryPart has been set, or the PrimaryPart no longer exists. Please set Model.PrimaryPart before using this.");
	}
}


static void rotateModelPart(shared_ptr<Instance> instance, const CoordinateFrame* rotCF, const Vector3* offsetFromOrigin)
{
	if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(instance.get())){
		CoordinateFrame cframe = part->getCoordinateFrame();
		cframe.translation -= (*offsetFromOrigin);	// move centroid to origin
		cframe = (*rotCF) * cframe;					// rotate
		cframe.translation += (*offsetFromOrigin);	// move back
		part->setCoordinateFrame(cframe);
	}
}

void ModelInstance::setPrimaryPartSetByUser(PartInstance* set)
{
	if (set == getPrimaryPartSetByUser())
		return;

	primaryPartSetByUser = shared_from(set);
	setExtentsDirty();
	raiseChanged(desc_primaryPartSetByUserProp);
}

PartInstance* ModelInstance::getPrimaryPartSetByUser() const 
{
	return primaryPartSetByUser.lock().get();
}


void ModelInstance::setExtentsDirty()
{
	if (DFFlag::CacheModelExtents)
	{
		modelCoordinateFrame.reset();
		primaryPartSpaceExtents.reset();
	}
	else
	{
		needsCFrameRecalc = true;
		needsSizeRecalc = true;
	}
	

	Instance* parent = getParent();
	while(parent)
	{
		if(ModelInstance* model = fastDynamicCast<ModelInstance>(parent))
		{
			model->setExtentsDirty();
			break;
		}
		parent = parent->getParent();
	}
}


void ModelInstance::onChildAdded(Instance* child)
{
	if (dynamic_cast<IModelModifier*>(child))
	{
		RBXASSERT_IF_VALIDATING(std::find(modelModifiers.begin(), modelModifiers.end(), child) == modelModifiers.end());
		modelModifiers.push_back(child);
	}

	setExtentsDirty();
}


void ModelInstance::onChildRemoving(Instance* child)
{
	if (dynamic_cast<IModelModifier*>(child))
	{
		std::vector<Instance*>::iterator it = std::find(modelModifiers.begin(), modelModifiers.end(), child);
		RBXASSERT(it != modelModifiers.end());
		modelModifiers.erase(it);
	}

	setExtentsDirty();
}

void ModelInstance::onChildChanged(Instance* instance, const PropertyChanged& event)
{
	setExtentsDirty();

	Super::onChildChanged(instance, event);
}

void ModelInstance::dirtyAll() 
{
	computedPrimaryPart = NULL;
}


bool ModelInstance::askSetParent(const Instance* instance) const
{
	return Instance::fastDynamicCast<ModelInstance>(instance)!=NULL;
}

void ModelInstance::onDescendantAdded(Instance* instance)
{
	Super::onDescendantAdded(instance);
	dirtyAll();

	this->shouldRenderSetDirty();		// does adding an instance ever change whether we should render?
}

void ModelInstance::onDescendantRemoving(const shared_ptr<Instance>& instance)
{
	if (primaryPartSetByUser.lock().get() == instance.get()) {
		setPrimaryPartSetByUser(NULL);
		modelInPrimary = CoordinateFrame();
	}

	dirtyAll(); // Note: when this is called (descendants added or removed), modelInPrimary might be invalidated if we are using a computedPrimaryPart

	this->shouldRenderSetDirty();

	Super::onDescendantRemoving(instance);
}


PartInstance* ModelInstance::getPrimaryPart() 
{
	// Clean up here bad primaryPart set by user on the case of legacy read
	if (primaryPartSetByUser.lock().get()) {
		if (!primaryPartSetByUser.lock()->isDescendantOf(this)) {
			RBXASSERT(0);					// just want to see this happening
			primaryPartSetByUser.reset();
		}
	}

	if (primaryPartSetByUser.lock().get()) {
		computedPrimaryPart = NULL;
		return primaryPartSetByUser.lock().get();
	}
	else if (computedPrimaryPart) {
		RBXASSERT(computedPrimaryPart->isDescendantOf(this));
		return computedPrimaryPart;
	}
	else {
		computePrimaryPart();
		return computedPrimaryPart;
	}
}


void VisitModelDescendants(shared_ptr<RBX::Instance> instance, PartInstance** biggestPart, float* biggestSize)
{
	if (PartInstance* descendantPart = Instance::fastDynamicCast<PartInstance>(instance.get())) {
		float descendantBiggest = descendantPart->getPartPrimitive()->getExtentsWorld().areaXZ();
		if (descendantBiggest > *biggestSize) {
			*biggestPart = descendantPart;
			*biggestSize = descendantBiggest;
		}
	}
}

void ModelInstance::computePrimaryPart()
{
	RBXASSERT(computedPrimaryPart == NULL);
	computedPrimaryPart = NULL;
	float biggest = -1.0;

	this->visitDescendants(boost::bind(&VisitModelDescendants, _1, &computedPrimaryPart, &biggest));
}


static void makeJ(shared_ptr<Instance> instance) {
	if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(instance.get())) {
		part->join();
	}
	else if (ModelInstance* model = Instance::fastDynamicCast<ModelInstance>(instance.get())) {
		model->visitChildren(&makeJ);
	}
}

void ModelInstance::makeJoints() 
{
	visitChildren(&makeJ);
}

static void breakJ(shared_ptr<Instance> instance) {
	if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(instance.get())) {
		part->destroyJoints();
	}
	else if (ModelInstance* model = Instance::fastDynamicCast<ModelInstance>(instance.get())) {
		model->visitChildren(&breakJ);
	}
}

void ModelInstance::breakJoints() {
	visitChildren(&breakJ);
}

void ModelInstance::setFrontAndTop(const Vector3& front)
{
	if (!getPrimaryPart()) {		// force recompute
		return;
	}

	if (!primaryPartSetByUser.lock().get()) {
		primaryPartSetByUser = shared_from(computedPrimaryPart);
	}

	if (primaryPartSetByUser.lock().get()) {
		RBXASSERT(primaryPartSetByUser.lock()->isDescendantOf(this));
		CoordinateFrame base;
		base.lookAt(front);
		modelInPrimary = CoordinateFrame(
							primaryPartSetByUser.lock()->getCoordinateFrame().rotation.transpose() * base.rotation,
							Vector3::zero()
							);
		dirtyAll();
	}
}


const CoordinateFrame ModelInstance::getLocation()
{
	getPrimaryPart();		// updates

	if (primaryPartSetByUser.lock().get()) {
		CoordinateFrame c = primaryPartSetByUser.lock()->getCoordinateFrame();
		return CoordinateFrame(c.rotation * modelInPrimary.rotation, c.translation);
	}
	else if (computedPrimaryPart) {
		return computedPrimaryPart->getCoordinateFrame();
		//CoordinateFrame c = computedPrimaryPart->getCoordinateFrame();
		//return CoordinateFrame(c.rotation * modelInPrimary.rotation, c.translation);
	}
	else {
		return CoordinateFrame();
	}
}

bool ModelInstance::isSelectable3d()
{
    return !FFlag::StudioDE6194FixEnabled || getPrimaryPart() != NULL;
}

// TODO: return closest part within the model?...
bool ModelInstance::hitTestImpl(const class RBX::RbxRay& worldRay, Vector3& worldHitPoint)
{
	for (size_t i = 0; i < numChildren(); i++) 
	{
		if (PVInstance* child = queryTypedChild<PVInstance>(i)) {
			if (child->hitTest(worldRay, worldHitPoint)) return true;
		}
	}
	return false;
}

bool ModelInstance::shouldRender3dAdorn() const
{
	return (ModelInstance::showModelCoordinateFrames && this->isTopLevelPVInstance());
}

void ModelInstance::render3dAdorn(Adorn* adorn) 
{
    if (ModelInstance::showModelCoordinateFrames) {
        if (this->isTopLevelPVInstance()) 
        {
			renderCoordinateFrame(adorn);
		}
	}
}



static void testFrustumParts(shared_ptr<Instance> descendant, const RBX::Frustum& frustum, bool& isContained)
{
	if (isContained) {																// Only test if we have a chance!
		if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(descendant.get())) {
			isContained = frustum.containsAABB(part->getPartPrimitive()->getExtentsLocal(), part->getCoordinateFrame());
		}
	}
}


bool ModelInstance::containedByFrustum(const RBX::Frustum& frustum) const
{
	bool isContained = true;
	this->visitDescendants(boost::bind(&testFrustumParts, _1, boost::ref(frustum), boost::ref(isContained)));
	return isContained;
}


static void countModelParts(shared_ptr<Instance> descendant, int& answer)
{
	if (Instance::fastDynamicCast<PartInstance>(descendant.get())) {
		answer++;
	}
}
		
int ModelInstance::computeNumParts() const
{
	int answer = 0;
	this->visitDescendants(boost::bind(&countModelParts, _1, boost::ref(answer)));
	return answer;
}

static void getInstances(shared_ptr<Instance> descendant, std::vector<PartInstance*>& partInstances)
{
	if(PartInstance* part = Instance::fastDynamicCast<PartInstance>(descendant.get()))
		partInstances.push_back(part);

	if(descendant->getChildren())
	{
		Instances::const_iterator end = descendant->getChildren()->end();
		for (Instances::const_iterator iter = descendant->getChildren()->begin(); iter != end; ++iter)
		{
			if(PartInstance* part = Instance::fastDynamicCast<PartInstance>(iter->get()))
				partInstances.push_back(part);
		}
	}
}

std::vector<PartInstance*> ModelInstance::getDescendantPartInstances() const
{
	std::vector<PartInstance*> allParts;
	this->visitDescendants(boost::bind(&getInstances, _1, boost::ref(allParts)));
	return allParts;
}

static void unionPartExtentsWorld(shared_ptr<Instance> descendant, Extents& answer)
{
	if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(descendant.get())) {
		answer.unionWith(part->computeExtentsWorld());
	}
}

// TODO - performance - anyone calling this each frame?
Extents ModelInstance::computeExtentsWorld() const
{
	Extents answer = Extents::negativeMaxExtents();
	this->visitDescendants(boost::bind(&unionPartExtentsWorld, _1, boost::ref(answer)));
	if (answer == Extents::negativeMaxExtents()) {
		answer = Extents::zero();						// if we don't find anything, set the extents to 0,0,0 point
	}
	return answer;
}


static void unionPartExtentsLocal(shared_ptr<Instance> descendant, Extents& answer, const CoordinateFrame& modelLocation)
{
	if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(descendant.get())) {
		Primitive* primitive = part->getPartPrimitive();
		Extents partExtentsInModel = primitive->getExtentsLocal().express(primitive->getCoordinateFrame(), modelLocation);
		answer.unionWith(partExtentsInModel);
	}
}

Extents ModelInstance::computeExtentsLocal(const CoordinateFrame& modelLocation)
{
	Extents answer = Extents::zero();
	this->visitDescendants(boost::bind(&unionPartExtentsLocal, _1, boost::ref(answer), boost::ref(modelLocation)));
	return answer;
}

CoordinateFrame ModelInstance::calculateModelCFrame()
{
	if (DFFlag::CacheModelExtents)
	{
		if (!modelCoordinateFrame)
		{
			calculateModelSize();
			modelCoordinateFrame = getLocation();
			modelCoordinateFrame->translation = modelCoordinateFrame->pointToWorldSpace(primaryPartSpaceExtents->center());
		}
		return modelCoordinateFrame.get();
	}

	if(needsCFrameRecalc)
	{
		CoordinateFrame location = getLocation();
		Extents localExtents = computeExtentsLocal(location);
		location.translation = location.pointToWorldSpace(localExtents.center());
		modelCFrame = location;
		needsCFrameRecalc = false;
	}
	return modelCFrame;
}
Vector3 ModelInstance::calculateModelSize()
{
	if (DFFlag::CacheModelExtents)
	{
		if (!primaryPartSpaceExtents)
		{
			CoordinateFrame location = getLocation();
			primaryPartSpaceExtents = computeExtentsLocal(location);
		}
		return primaryPartSpaceExtents->size();
	}

	if(needsSizeRecalc)
	{
		CoordinateFrame location = getLocation();
		Extents localExtents = computeExtentsLocal(location);
		modelSize =  localExtents.size();
		needsSizeRecalc = false;
	}
	return modelSize;
}

void ModelInstance::resetOrientationToIdentity()
{
	Vector3 offsetFromOrigin = calculateModelCFrame().translation;
	CoordinateFrame rotCF;
	rotCF.rotation = calculateModelCFrame().inverse().rotation;
	visitDescendants(boost::bind(&rotateModelPart, _1, &rotCF, &offsetFromOrigin));
	setExtentsDirty();
}

void ModelInstance::setIdentityOrientation()
{
	getPrimaryPart();		// updates here too [basically same as getLocation() code, but don't want to apply modelInPrimary filter here]

	CoordinateFrame c;
	if (primaryPartSetByUser.lock().get()) {
		c = primaryPartSetByUser.lock()->getCoordinateFrame();
	}
	else if (computedPrimaryPart) {
		c = computedPrimaryPart->getCoordinateFrame();
	}
	else {
		c = CoordinateFrame();
	}

	modelInPrimary.rotation = c.inverse().rotation;
	setExtentsDirty();
}

Part ModelInstance::computePart()
{
	if(numChildren() < 1)
		return Part(BLOCK_PART, Vector3(0,0,0), G3D::Color3::white(), getLocation());
	
	if (DFFlag::CacheModelExtents)
		return Part(BLOCK_PART, calculateModelSize(), G3D::Color3::white(), calculateModelCFrame());

	CoordinateFrame location = getLocation();
	Extents localExtents = computeExtentsLocal(location);

	location.translation = location.pointToWorldSpace(localExtents.center());

	return Part(BLOCK_PART, localExtents.size(), G3D::Color3::white(), location);
}
void ModelInstance::render3dSelect(Adorn* adorn, SelectState selectState)
{
    if (FFlag::StudioDE6194FixEnabled && !isSelectable3d())
        return;

	Part part = computePart();

	if (selectState == RBX::SELECT_NORMAL && getPrimaryPartSetByUser())
        RBX::Draw::selectionBox(getPrimaryPartSetByUser()->getPart(), adorn, primaryPartSelectColor(), primaryPartLineThickness());

	Draw::selectionBox(
		part, 
		adorn, 
		selectState,
		0.05f);
}

void ModelInstance::onCameraNear(float distance) 
{
	for (size_t i = 0; i < numChildren(); i++) {
		if (CameraSubject* cameraSubject = queryTypedChild<CameraSubject>(i)) {
			cameraSubject->onCameraNear(distance);
		}
	}
}

void ModelInstance::getCameraIgnorePrimitives(std::vector<const Primitive*>& primitives)
{
	DragUtilities::getPrimitivesConst(this, primitives);

	if (Humanoid* h = Humanoid::modelIsCharacter(this))
	{
		h->getCameraIgnorePrimitives(primitives);
	}
}



} // namespace
