/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/ManualJointHelper.h"
#include "GfxBase/IAdornableCollector.h"
#include "V8World/World.h"
#include "Tool/DragUtilities.h"
#include "V8World/ContactManager.h"
#include "V8World/Contact.h"
#include "V8World/Joint.h"
#include "V8World/WeldJoint.h"
#include "V8World/MegaClusterPoly.h"
#include "Tool/ToolsArrow.h"

#include "Humanoid/Humanoid.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/PVInstance.h"
#include "V8DataModel/JointInstance.h"
#include "V8DataModel/MegaCluster.h"

namespace RBX {

const float autoJointLineThickness = 0.1f;

ManualJointHelper::~ManualJointHelper()
{
	if( workspace && isAdornable )
		workspace->getAdornableCollector()->onRenderableDescendantRemoving(this);

	// Clear the existing part/surface pairs containers
	for( unsigned int i = 0; i < jointSurfacePairs.size(); i++ )
		delete jointSurfacePairs[i];

	jointSurfacePairs.clear();
	autoJointsDetected = false;
}

ManualJointHelper::ManualJointHelper(Workspace* ws)
{
	workspace = NULL;
	displayPotentialJoints = AdvArrowTool::advManualJointMode;
    isAdornable = false;
	autoJointsDetected = false;
    targetPrimitive = NULL;
	setWorkspace(ws);
}

ManualJointHelper::ManualJointHelper(void)
{
	workspace = NULL;
	displayPotentialJoints = AdvArrowTool::advManualJointMode;
    isAdornable = false;
	autoJointsDetected = false;
    targetPrimitive = NULL;
}

void ManualJointHelper::clearAndDeleteJointSurfacePairs(void)
{
    // Clear the existing part/surface pairs containers
	for( unsigned int i = 0; i < jointSurfacePairs.size(); i++ )
		delete jointSurfacePairs[i];

	jointSurfacePairs.clear();
	autoJointsDetected = false;
}

void ManualJointHelper::setWorkspace(Workspace* ws) 
{ 
    if( !workspace )
    {
        workspace = ws; 
        if( !isAdornable )
		{
	        workspace->getAdornableCollector()->onRenderableDescendantAdded(this);
			isAdornable = true;
		}
    }
}

void ManualJointHelper::findPermissibleJointSurfacePairs(void) 
{
	// Clear the existing part/surface pairs containers
	clearAndDeleteJointSurfacePairs();

	// find other parts that are contacting the selected parts
	for( unsigned int i = 0; i < selectedPrimitives.size(); i++ )
	{
		G3D::Array<Primitive*> found;
        size_t selectedPrimSurf = (size_t)-1;
	    size_t foundPrimSurf = (size_t)-1;
        if(workspace)
        {
		    workspace->getWorld()->getContactManager()->getPrimitivesTouchingExtents(selectedPrimitives[i]->getFastFuzzyExtents(), selectedPrimitives[i], 0, found);
		    for( int j = 0; j < found.size(); j++ )
		    {
				if( !selectedPrimitives[i]->getGeometry()->isTerrain() )
                {
			        if( std::find(selectedPrimitives.begin(), selectedPrimitives.end(), found[j])== selectedPrimitives.end() )
			        {
			            if( found[j]->getConstGeometry()->findTouchingSurfacesConvex(found[j]->getCoordinateFrame(), foundPrimSurf, 
                                *(selectedPrimitives[i]->getConstGeometry()), selectedPrimitives[i]->getCoordinateFrame(), selectedPrimSurf) )
				            createJointSurfacePair(*found[j], foundPrimSurf, *selectedPrimitives[i], selectedPrimSurf);
			        }
                }
		    }
        }
	} 
    
    // add in joints with the terrain
    Primitive* mCPrim = workspace->getWorld()->getContactManager()->getMegaClusterPrimitive();

	if (mCPrim && mCPrim->getGeometryType() == Geometry::GEOMETRY_MEGACLUSTER)
    {
	    for( unsigned int i = 0; i < selectedPrimitives.size(); i++ )
	    {
            if( selectedPrimitives[i]->getGeometryType() != RBX::Geometry::GEOMETRY_MEGACLUSTER )
            {
                std::vector<Vector3int16> cells;
                static_cast<const MegaClusterPoly*>(mCPrim->getConstGeometry())->findPlanarTouchesWithGeom(mCPrim->getCoordinateFrame(),
                                                                                *(selectedPrimitives[i]->getConstGeometry()),
                                                                                selectedPrimitives[i]->getCoordinateFrame(),
                                                                                &cells);

                // for each cell call createJointSurfacePair
                for(size_t j = 0; j < cells.size(); ++j)
                {
                    createTerrainJointSurfacePair(*mCPrim, *selectedPrimitives[i], cells[j]);
                }
            }
        }
    }

	if (mCPrim && mCPrim->getGeometryType() == Geometry::GEOMETRY_SMOOTHCLUSTER)
    {
	    for( unsigned int i = 0; i < selectedPrimitives.size(); i++ )
	    {
			Geometry::GeometryType geometryType = selectedPrimitives[i]->getGeometryType();
			
			if (geometryType != Geometry::GEOMETRY_SMOOTHCLUSTER && geometryType != Geometry::GEOMETRY_BALL)
            {
				if (ManualWeldJoint::isTouchingTerrain(mCPrim, selectedPrimitives[i]))
					createSmoothTerrainJointSurfacePair(*mCPrim, *selectedPrimitives[i]);
            }
        }
    }
}

void ManualJointHelper::createTerrainJointSurfacePair(Primitive& p0, Primitive& p1, Vector3int16& cellIndex)
{
	PartInstance* part0 = PartInstance::fromPrimitive(&p0);
	RBXASSERT(part0->getParent());
	PartInstance* part1 = PartInstance::fromPrimitive(&p1);
	RBXASSERT(part1->getParent());

	ConstraintSurfacePair* aPair = NULL;

	RBXASSERT( p1.getConstGeometry()->getGeometryType() != RBX::Geometry::GEOMETRY_MEGACLUSTER );
	size_t fix = -1;
	// need to check if p1 is about to be joined to a no_join cell!
	const MegaClusterPoly* terrainPoly = static_cast<const MegaClusterPoly*>(p0.getConstGeometry());
	size_t otherFaceId;
	std::vector<Vector3> intersectPolyInP0 = terrainPoly->findCellIntersectionWithGeom( cellIndex, 
																						p0.getCoordinateFrame(), 
																						*(p1.getConstGeometry()), 
																						p1.getCoordinateFrame(),
																						otherFaceId);
	SurfaceType t1 = Joint::getSurfaceTypeFromNormal(p1, (NormalId)otherFaceId);
	if ( t1 == NO_JOIN )
	{
		DisallowedTerrainJointSurfacePair* pair = new DisallowedTerrainJointSurfacePair(p0, fix, p1, otherFaceId);
		pair->setCellIndex(cellIndex);
		aPair = pair;
	}
	else 
	{
		TerrainManualJointSurfacePair* pair = new TerrainManualJointSurfacePair(p0, fix, p1, fix);
		pair->setCellIndex(cellIndex);
		aPair = pair;
	}

	jointSurfacePairs.push_back(aPair);

	aPair->p0AncestorChangedConnection = part0->ancestryChangedSignal.connect(boost::bind(&ManualJointHelper::onPartAncestryChanged, this, _1, _2));
	aPair->p1AncestorChangedConnection = part1->ancestryChangedSignal.connect(boost::bind(&ManualJointHelper::onPartAncestryChanged, this, _1, _2));
}

void ManualJointHelper::createSmoothTerrainJointSurfacePair(Primitive& p0, Primitive& p1)
{
	PartInstance* part0 = PartInstance::fromPrimitive(&p0);
	RBXASSERT(part0->getParent());
	PartInstance* part1 = PartInstance::fromPrimitive(&p1);
	RBXASSERT(part1->getParent());

	size_t fix = -1;
	SmoothTerrainManualJointSurfacePair* pair = new SmoothTerrainManualJointSurfacePair(p0, fix, p1, fix);

	jointSurfacePairs.push_back(pair);

	pair->p0AncestorChangedConnection = part0->ancestryChangedSignal.connect(boost::bind(&ManualJointHelper::onPartAncestryChanged, this, _1, _2));
	pair->p1AncestorChangedConnection = part1->ancestryChangedSignal.connect(boost::bind(&ManualJointHelper::onPartAncestryChanged, this, _1, _2));
}

bool ManualJointHelper::surfaceIsNoJoin(const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id)
{
	SurfaceType t0 = Joint::getSurfaceTypeFromNormal(p0, (NormalId)face0Id);
	SurfaceType t1 = Joint::getSurfaceTypeFromNormal(p1, (NormalId)face1Id);
	
	return ( t0 == NO_JOIN || t1 == NO_JOIN );
}

void ManualJointHelper::createJointSurfacePair(Primitive& p0, size_t& face0Id, Primitive& p1, size_t& face1Id)
{
	PartInstance* part0 = PartInstance::fromPrimitive(&p0);
	RBXASSERT(part0->getParent());
	PartInstance* part1 = PartInstance::fromPrimitive(&p1);
	RBXASSERT(part1->getParent());

    ConstraintSurfacePair* aPair = NULL;

    // First check if either part is associated with a humanoid (body part or child gear, hat, etc)
    // If so, no joint allowed
    const Instance* parent0 = PartInstance::fromConstPrimitive(&p0)->getParent();
    const Humanoid* aHumanoidModel0 = NULL;
    while( parent0 && !aHumanoidModel0 )
    {
        aHumanoidModel0 = Humanoid::modelIsConstCharacter(parent0);
        parent0 = parent0->getParent();
    }
    const Instance* parent1 = PartInstance::fromConstPrimitive(&p1)->getParent();
    const Humanoid* aHumanoidModel1 = NULL;
    while( parent1 && !aHumanoidModel1 && !aHumanoidModel0 )  // don't bother walking p1's tree if we already found a character
    {
        aHumanoidModel1 = Humanoid::modelIsConstCharacter(parent1);
        parent1 = parent1->getParent();
    }

    if( aHumanoidModel0 || aHumanoidModel1 || surfaceIsNoJoin(p0, face0Id, p1, face1Id) )  // also disallow if either surface type is NO_JOIN
        aPair = new DisallowedJointSurfacePair(p0, face0Id, p1, face1Id);
    else
    {
        RBXASSERT( p1.getConstGeometry()->getGeometryType() != RBX::Geometry::GEOMETRY_MEGACLUSTER );
        if( p0.getConstGeometry()->getGeometryType() == RBX::Geometry::GEOMETRY_MEGACLUSTER )
            aPair = new TerrainManualJointSurfacePair(p0, face0Id, p1, face1Id);
        else if( Joint::compatibleForStudAutoJoint(p0, face0Id, p1, face1Id) )
	        if(Joint::positionedForStudAutoJoint(p0, face0Id, p1, face1Id))
			{
		        aPair = new StudAutoJointSurfacePair(p0, face0Id, p1, face1Id);
				autoJointsDetected = true;
			}
	        else
#ifndef MACONLY_MANUAL_WELD_ALL_SURFACES
		        aPair = new DisallowedJointSurfacePair(p0, face0Id, p1, face1Id);
#else
				aPair = new ManualJointSurfacePair(p0, face0Id, p1, face1Id);
#endif
        else if(Joint::compatibleForGlueAutoJoint(p0, face0Id, p1, face1Id))
		{
	        aPair = new GlueAutoJointSurfacePair(p0, face0Id, p1, face1Id);
			autoJointsDetected = true;
		}
        else if(Joint::compatibleForWeldAutoJoint(p0, face0Id, p1, face1Id))
		{
	        aPair = new WeldAutoJointSurfacePair(p0, face0Id, p1, face1Id);
			autoJointsDetected = true;
		}
        else if(Joint::compatibleForHingeAutoJoint(p0, face0Id, p1, face1Id))
		{
	        aPair = new HingeAutoJointSurfacePair(p0, face0Id, p1, face1Id);
			autoJointsDetected = true;
		}
#ifndef MACONLY_MANUAL_WELD_ALL_SURFACES
		//this prevents welding of incompatible surfaces such as studs to smooth.
        else if(Joint::inCompatibleForAnyJoint(p0, face0Id, p1, face1Id))
	        aPair = new DisallowedJointSurfacePair(p0, face0Id, p1, face1Id);
#endif
        else
	        aPair = new ManualJointSurfacePair(p0, face0Id, p1, face1Id);
    }

	aPair->p0AncestorChangedConnection = part0->ancestryChangedSignal.connect(boost::bind(&ManualJointHelper::onPartAncestryChanged, this, _1, _2));
	aPair->p1AncestorChangedConnection = part1->ancestryChangedSignal.connect(boost::bind(&ManualJointHelper::onPartAncestryChanged, this, _1, _2));

	jointSurfacePairs.push_back(aPair);
}

void ManualJointHelper::setSelectedPrimitives(const std::vector<Instance*>& selectedInstances)
{
	PartArray partInstances;
	DragUtilities::instancesToParts(selectedInstances, partInstances);
	selectedPrimitives.clear();
	DragUtilities::partsToPrimitives(partInstances, selectedPrimitives);
}

void ManualJointHelper::setSelectedPrimitives(const Instances& selectedInstances)
{
	PartArray partInstances;
	DragUtilities::instancesToParts(selectedInstances, partInstances);
	selectedPrimitives.clear();
	DragUtilities::partsToPrimitives(partInstances, selectedPrimitives);
}

void ManualJointHelper::setPVInstanceToJoin(PVInstance& pVInstToJoin) 
{
	std::vector<PVInstance*> pvInstances;
    pvInstances.push_back(&pVInstToJoin);
    PartArray partInstances;
	DragUtilities::pvsToParts(pvInstances, partInstances);
	selectedPrimitives.clear();
	DragUtilities::partsToPrimitives(partInstances, selectedPrimitives);
}

void ManualJointHelper::setPVInstanceTarget(PVInstance& pVInstToJoin) 
{
    PartInstance* part = dynamic_cast<PartInstance*>(&pVInstToJoin);
    if(part)
        targetPrimitive = part->getPartPrimitive();
    else
        targetPrimitive = NULL;
}


void ManualJointHelper::createJoints(void) 
{
	for( unsigned int i = 0; i < jointSurfacePairs.size(); i++ )
		jointSurfacePairs[i]->createJoint();
}

void ManualJointHelper::createJointsIfEnabledFromGui(void) 
{
	if(AdvArrowTool::advManualJointMode)
	{
		for( unsigned int i = 0; i < jointSurfacePairs.size(); i++ )
			jointSurfacePairs[i]->createJoint();
	}
}
		
void ManualJointHelper::render3dAdorn(Adorn* adorn)
{
    for( unsigned int i = 0; i < jointSurfacePairs.size(); i++ )
    {
        // if the target primitive is non NULL, then the client wants to only render the target body outline
        // Otherwise, render all surface pairs
        if(targetPrimitive)
        {
            if(jointSurfacePairs[i]->getP0() == targetPrimitive || jointSurfacePairs[i]->getP1() == targetPrimitive)
	            jointSurfacePairs[i]->dynamicDraw(adorn);
        }
        else
            jointSurfacePairs[i]->dynamicDraw(adorn);
    }
}

void ManualJointHelper::onPartAncestryChanged(shared_ptr<Instance> instance, shared_ptr<Instance> newParent)
{
	if (!newParent)
	{
		if (PartInstance* part = instance->fastDynamicCast<PartInstance>())
		{
			Primitive * prim = PartInstance::getPrimitive(part);

			if (prim == targetPrimitive)
				targetPrimitive = NULL;

			std::vector<ConstraintSurfacePair*>::iterator iter = jointSurfacePairs.begin();
			for (; iter != jointSurfacePairs.end(); iter++)
			{
				if (prim == (*iter)->getP0() || prim == (*iter)->getP1())
				{
					// delete pair if one of the primitive has no parent, it's being deleted
					delete *iter;
					break;
				}
			}

			if (iter != jointSurfacePairs.end())
				jointSurfacePairs.erase(iter);
		}
	}
}

ConstraintSurfacePair::ConstraintSurfacePair()
{
	p0 = NULL;
	p1 = NULL;
	s0 = (size_t)-1;
	s1 = (size_t)-1;
}

ConstraintSurfacePair::ConstraintSurfacePair(const Primitive& prim0, size_t& face0Id, const Primitive& prim1, size_t& face1Id)
{
	p0 = &prim0;
	p1 = &prim1;
	s0 = face0Id;
	s1 = face1Id;
}

void StudAutoJointSurfacePair::dynamicDraw(Adorn* adorn)
{
	std::vector<Vector3> polygon1;
    if( p1 && p0 && p1->getConstGeometry() && p0->getConstGeometry() && s1 != (size_t)-1 && s0 != (size_t)-1 )
    {
	    for( int i = 0; i < p1->getConstGeometry()->getNumVertsInSurface(s1); i++ )
	    {
		    Vector3 p1VertInWorld = p1->getCoordinateFrame().pointToWorldSpace(p1->getConstGeometry()->getSurfaceVertInBody(s1, i));
		    polygon1.push_back(p0->getCoordinateFrame().pointToObjectSpace(p1VertInWorld));
	    }
	
	    std::vector<Vector3> intersectPolyInP0 = p0->getConstGeometry()->polygonIntersectionWithFace(polygon1, s0);

	    DrawAdorn::polygonRelativeToCoord(adorn, p0->getCoordinateFrame(), intersectPolyInP0, Color3::blue(), autoJointLineThickness);
    }
}

void GlueAutoJointSurfacePair::dynamicDraw(Adorn* adorn)
{
	std::vector<Vector3> polygon1;
    if( p1 && p0 && p1->getConstGeometry() && p0->getConstGeometry() && s1 != (size_t)-1 && s0 != (size_t)-1 )
    {
	    for( int i = 0; i < p1->getConstGeometry()->getNumVertsInSurface(s1); i++ )
	    {
		    Vector3 p1VertInWorld = p1->getCoordinateFrame().pointToWorldSpace(p1->getConstGeometry()->getSurfaceVertInBody(s1, i));
		    polygon1.push_back(p0->getCoordinateFrame().pointToObjectSpace(p1VertInWorld));
	    }
    	
	    std::vector<Vector3> intersectPolyInP0 = p0->getConstGeometry()->polygonIntersectionWithFace(polygon1, s0);

	    DrawAdorn::polygonRelativeToCoord(adorn, p0->getCoordinateFrame(), intersectPolyInP0, Color3::blue(), autoJointLineThickness);
    }
}

void WeldAutoJointSurfacePair::dynamicDraw(Adorn* adorn)
{
	std::vector<Vector3> polygon1;
    if( p1 && p0 && p1->getConstGeometry() && p0->getConstGeometry() && s1 != (size_t)-1 && s0 != (size_t)-1 )
    {
	    for( int i = 0; i < p1->getConstGeometry()->getNumVertsInSurface(s1); i++ )
	    {
		    Vector3 p1VertInWorld = p1->getCoordinateFrame().pointToWorldSpace(p1->getConstGeometry()->getSurfaceVertInBody(s1, i));
		    polygon1.push_back(p0->getCoordinateFrame().pointToObjectSpace(p1VertInWorld));
	    }
    	
	    std::vector<Vector3> intersectPolyInP0 = p0->getConstGeometry()->polygonIntersectionWithFace(polygon1, s0);

	    DrawAdorn::polygonRelativeToCoord(adorn, p0->getCoordinateFrame(), intersectPolyInP0, Color3::blue(), autoJointLineThickness);
    }
}

void HingeAutoJointSurfacePair::dynamicDraw(Adorn* adorn)
{
	std::vector<Vector3> polygon1;
    if( p1 && p0 && p1->getConstGeometry() && p0->getConstGeometry() && s1 != (size_t)-1 && s0 != (size_t)-1 )
    {
	    for( int i = 0; i < p1->getConstGeometry()->getNumVertsInSurface(s1); i++ )
	    {
		    Vector3 p1VertInWorld = p1->getCoordinateFrame().pointToWorldSpace(p1->getConstGeometry()->getSurfaceVertInBody(s1, i));
		    polygon1.push_back(p0->getCoordinateFrame().pointToObjectSpace(p1VertInWorld));
	    }
    	
	    std::vector<Vector3> intersectPolyInP0 = p0->getConstGeometry()->polygonIntersectionWithFace(polygon1, s0);

	    DrawAdorn::polygonRelativeToCoord(adorn, p0->getCoordinateFrame(), intersectPolyInP0, Color3::blue(), autoJointLineThickness);
    }
}

void DisallowedJointSurfacePair::dynamicDraw(Adorn* adorn)
{
	std::vector<Vector3> polygon1;
    if( p1 && p0 && p1->getConstGeometry() && p0->getConstGeometry() && s1 != (size_t)-1 && s0 != (size_t)-1 )
    {
	    for( int i = 0; i < p1->getConstGeometry()->getNumVertsInSurface(s1); i++ )
	    {
		    Vector3 p1VertInWorld = p1->getCoordinateFrame().pointToWorldSpace(p1->getConstGeometry()->getSurfaceVertInBody(s1, i));
		    polygon1.push_back(p0->getCoordinateFrame().pointToObjectSpace(p1VertInWorld));
	    }
    	
	    std::vector<Vector3> intersectPolyInP0 = p0->getConstGeometry()->polygonIntersectionWithFace(polygon1, s0);

	    DrawAdorn::polygonRelativeToCoord(adorn, p0->getCoordinateFrame(), intersectPolyInP0, Color3::red(), autoJointLineThickness);
    }
}

void ManualJointSurfacePair::dynamicDraw(Adorn* adorn)
{
	if( p1 && p0 && p1->getConstGeometry() && p0->getConstGeometry() && s1 != (size_t)-1 && s0 != (size_t)-1 )
	{
		std::vector<Vector3> polygon1;
		for( int i = 0; i < p1->getConstGeometry()->getNumVertsInSurface(s1); i++ )
		{
			Vector3 p1VertInWorld = p1->getCoordinateFrame().pointToWorldSpace(p1->getConstGeometry()->getSurfaceVertInBody(s1, i));
			polygon1.push_back(p0->getCoordinateFrame().pointToObjectSpace(p1VertInWorld));
		}
		
		std::vector<Vector3> intersectPolyInP0 = p0->getConstGeometry()->polygonIntersectionWithFace(polygon1, s0);

		Color3 color = AdvArrowTool::advManualJointType == DRAG::WEAK_MANUAL_JOINT ? Color3::brown() : Color3::white();
		DrawAdorn::polygonRelativeToCoord(adorn, p0->getCoordinateFrame(), intersectPolyInP0, color, autoJointLineThickness);
	}
}

void ManualJointSurfacePair::createJoint(void)
{
	PartInstance* PartInst0 = (PartInstance*)PartInstance::fromConstPrimitive(p0);
	PartInstance* PartInst1 = (PartInstance*)PartInstance::fromConstPrimitive(p1);

	if(PartInst0 && PartInst1)
	{
		if( AdvArrowTool::advManualJointType == DRAG::STRONG_MANUAL_JOINT)
		{
		    shared_ptr<ManualWeld> mJoint = Creatable<Instance>::create<ManualWeld>();
		    std::string jointPostPend = " Strong Joint";
		    // The joint will appear as a child of the drag part in the tree view
		    std::string p0Name = PartInst0->getName();
		    std::string p1Name = PartInst1->getName();
		    std::string jointName = p0Name + "-to-" + p1Name + jointPostPend;
		    mJoint->setName(jointName);

		    mJoint->setPart0(PartInst0);
		    mJoint->setPart1(PartInst1);
		    CoordinateFrame c0 = p0->getConstGeometry()->getSurfaceCoordInBody(s0);
		    CoordinateFrame worldC = p0->getCoordinateFrame() * c0;
		    CoordinateFrame c1 = p1->getCoordinateFrame().toObjectSpace(worldC);

		    mJoint->setC0(c0);
		    mJoint->setC1(c1);
		    mJoint->setSurface0(s0);
		    mJoint->setSurface1(s1);
		    mJoint->setParent(PartInst0);
		}
		else
		{
			shared_ptr<ManualGlue> mJoint = Creatable<Instance>::create<ManualGlue>();
			std::string jointPostPend = " Glue Joint";
			// The joint will appear as a child of the drag part in the tree view
			std::string p0Name = PartInst0->getName();
			std::string p1Name = PartInst1->getName();
			std::string jointName = p0Name + "-to-" + p1Name + jointPostPend;
			mJoint->setName(jointName);

			mJoint->setPart0(PartInst0);
			mJoint->setPart1(PartInst1);
			CoordinateFrame c0 = p0->getConstGeometry()->getSurfaceCoordInBody(s0);
			CoordinateFrame worldC = p0->getCoordinateFrame() * c0;
			CoordinateFrame c1 = p1->getCoordinateFrame().toObjectSpace(worldC);

			mJoint->setC0(c0);
			mJoint->setC1(c1);
			mJoint->setSurface0(s0);
			mJoint->setSurface1(s1);
			mJoint->setParent(PartInst0);
		}
	}
}

void TerrainManualJointSurfacePair::dynamicDraw(Adorn* adorn)
{
	if( p1 && p0 && p1->getConstGeometry() && p0->getConstGeometry() )
	{
		
		const MegaClusterPoly* terrainPoly = static_cast<const MegaClusterPoly*>(p0->getConstGeometry());
		size_t tempFaceId;
		std::vector<Vector3> intersectPolyInP0 = terrainPoly->findCellIntersectionWithGeom( cellIndex, 
																							p0->getCoordinateFrame(), 
																							*(p1->getConstGeometry()), 
																							p1->getCoordinateFrame(),
																							tempFaceId);

		Color3 color = AdvArrowTool::advManualJointType == DRAG::WEAK_MANUAL_JOINT ? Color3::brown() : Color3::white();
		DrawAdorn::polygonRelativeToCoord(adorn, p0->getCoordinateFrame(), intersectPolyInP0, color, autoJointLineThickness);
	}
}

void DisallowedTerrainJointSurfacePair::dynamicDraw(Adorn* adorn)
{
	if( p1 && p0 && p1->getConstGeometry() && p0->getConstGeometry() )
	{
		const MegaClusterPoly* terrainPoly = static_cast<const MegaClusterPoly*>(p0->getConstGeometry());
		size_t tempFaceId;
		std::vector<Vector3> intersectPolyInP0 = terrainPoly->findCellIntersectionWithGeom( cellIndex, 
																							p0->getCoordinateFrame(), 
																							*(p1->getConstGeometry()), 
																							p1->getCoordinateFrame(),
																							tempFaceId);

		DrawAdorn::polygonRelativeToCoord(adorn, p0->getCoordinateFrame(), intersectPolyInP0, Color3::red(), autoJointLineThickness);
	}
}

void TerrainManualJointSurfacePair::createJoint(void)
{
    // check if the non-terrain part, p1, is already welded to this terrain cell
    // in this case, do not create it
    RBXASSERT( p1->getConstGeometry()->getGeometryType() != RBX::Geometry::GEOMETRY_MEGACLUSTER );
    for (int i = 0; i < p1->getNumJoints(); ++i)
    {
        const ManualWeldJoint* existingJoint = dynamic_cast<const ManualWeldJoint*>(p1->getConstJoint(i));
        if( existingJoint && cellIndex == existingJoint->getCell() ) 
            return;
    }

	PartInstance* PartInst0 = (PartInstance*)PartInstance::fromConstPrimitive(p0);
	PartInstance* PartInst1 = (PartInstance*)PartInstance::fromConstPrimitive(p1);

	if(PartInst0 && PartInst1)
	{
		if( AdvArrowTool::advManualJointType == DRAG::STRONG_MANUAL_JOINT)
		{
		    shared_ptr<ManualWeld> mJoint = Creatable<Instance>::create<ManualWeld>();
		    std::string jointPostPend = " Terrain Joint";
		    // The joint will appear as a child of the drag part in the tree view
		    std::string p0Name = PartInst0->getName();
		    std::string p1Name = PartInst1->getName();
		    std::string jointName = p1Name + jointPostPend;
		    mJoint->setName(jointName);

		    mJoint->setPart0(PartInst0);
		    mJoint->setPart1(PartInst1);
		    CoordinateFrame c0 = p0->getCoordinateFrame();
		    CoordinateFrame worldC = p0->getCoordinateFrame() * c0;
		    CoordinateFrame c1 = p1->getCoordinateFrame().toObjectSpace(worldC);

		    mJoint->setC0(c0);
		    mJoint->setC1(c1);
		    mJoint->setParent(PartInst1);

            RBXASSERT(mJoint->getJointType() == Joint::MANUAL_WELD_JOINT);
            static_cast<ManualWeldJoint*>(mJoint->getJoint())->setCell(cellIndex);
		}
	}
}

void SmoothTerrainManualJointSurfacePair::dynamicDraw(Adorn* adorn)
{
}

void SmoothTerrainManualJointSurfacePair::createJoint(void)
{
    // check if the non-terrain part, p1, is already welded to terrain
    // in this case, do not create it
    RBXASSERT( p1->getConstGeometry()->getGeometryType() != RBX::Geometry::GEOMETRY_MEGACLUSTER );

    for (int i = 0; i < p1->getNumJoints(); ++i)
    {
        const Joint* existingJoint = p1->getConstJoint(i);

		if (Joint::isManualJoint(existingJoint) && (existingJoint->getConstPrimitive(0) == p0 || existingJoint->getConstPrimitive(1) == p1))
            return;
    }

	PartInstance* PartInst0 = (PartInstance*)PartInstance::fromConstPrimitive(p0);
	PartInstance* PartInst1 = (PartInstance*)PartInstance::fromConstPrimitive(p1);

	if (PartInst0 && PartInst1)
	{
		if (AdvArrowTool::advManualJointType == DRAG::STRONG_MANUAL_JOINT)
		{
		    shared_ptr<ManualWeld> mJoint = Creatable<Instance>::create<ManualWeld>();
		    std::string jointPostPend = " Terrain Joint";
		    // The joint will appear as a child of the drag part in the tree view
		    std::string p0Name = PartInst0->getName();
		    std::string p1Name = PartInst1->getName();
		    std::string jointName = p1Name + jointPostPend;
		    mJoint->setName(jointName);

		    mJoint->setPart0(PartInst0);
		    mJoint->setPart1(PartInst1);
		    mJoint->setParent(PartInst1);
		}
	}
}

} // namespace
