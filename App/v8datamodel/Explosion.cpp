/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Explosion.h"
#include "V8DataModel/PVInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/ForceField.h"
#include "V8World/World.h"
#include "V8World/ContactManager.h"
#include "V8Tree/Service.h"
#include "V8Kernel/Constants.h"
#include "V8Kernel/Body.h"
#include "AppDraw/DrawPrimitives.h"
#include "GfxBase/Adorn.h"
#include "V8DataModel/TimerService.h"

#include "V8DataModel/MegaCluster.h" // for terrain damage
#include "Humanoid/Humanoid.h"

FASTFLAGVARIABLE(RenderNewExplosionEnable, true)


namespace RBX {

const char* const sExplosion = "Explosion";

REFLECTION_BEGIN();
static const Reflection::PropDescriptor<Explosion, float> propBlastRadius("BlastRadius", category_Data, &Explosion::getBlastRadius, &Explosion::setBlastRadius);
static const Reflection::PropDescriptor<Explosion, float> propDestroyJoints("DestroyJointRadiusPercent", category_Data, &Explosion::getDestroyJoints, &Explosion::setDestroyJoints);
Reflection::BoundProp<float> Explosion::propBlastPressure("BlastPressure", category_Data, &Explosion::blastPressure);
Reflection::BoundProp<Vector3> Explosion::propPosition("Position", category_Data, &Explosion::position);
static Reflection::EnumPropDescriptor<Explosion, Explosion::ExplosionType> prop_ExplosionType("ExplosionType", category_Data, &Explosion::getExplosionType, &Explosion::setExplosionType);
Reflection::EventDesc<Explosion, void(shared_ptr<Instance>, float)> event_Hit(&Explosion::hitSignal, "Hit", "part", "distance");
REFLECTION_END();

namespace Reflection {
	template<>
	EnumDesc<Explosion::ExplosionType>::EnumDesc()
		:EnumDescriptor("ExplosionType")
	{
		addPair(Explosion::NO_EFFECT, "NoCraters");
		addPair(Explosion::NO_DEBRIS, "Craters");
		addPair(Explosion::WITH_DEBRIS, "CratersAndDebris");
	}
}


Explosion::Explosion() 
	: blastRadius(4)
	, blastPressure(500000.0f)
	, explosionType(NO_DEBRIS)
	, destroyJointRadiusPercent(1.0f)
{
	setName("Explosion");
	FASTLOG1(FLog::ISteppedLifetime, "Explosion created - %p", this);
}

Explosion::~Explosion()
{
	FASTLOG1(FLog::ISteppedLifetime, "Explosion destroyed - %p", this);
}

float Explosion::visualRadius() const			{return blastRadius;}

float Explosion::killRadius() const				{return 0.0f;}

float Explosion::blastMaxObjectRadius() const	{return blastRadius * 2.0f;}

float Explosion::killMaxObjectRadius() const	{return blastRadius;}

void Explosion::setExplosionType(ExplosionType value)
{
	if(explosionType != value)
	{
		explosionType = value;
		raisePropertyChanged(prop_ExplosionType);
	}
}

void Explosion::setBlastRadius(float _blastRadius)
{
	float _set = G3D::clamp(_blastRadius, 0.0f, 100.0f);
	if (blastRadius != _set) {
		blastRadius = _set;
		raisePropertyChanged(propBlastRadius);
	}
}

void Explosion::setDestroyJoints(float _destroyJointRadiusPercent)
{
	float _set = G3D::clamp(_destroyJointRadiusPercent, 0.0f, 1.0f);
	if (destroyJointRadiusPercent != _set) {
		destroyJointRadiusPercent = _set;
		raisePropertyChanged(propDestroyJoints);
	}
}

void Explosion::signalBlast(const std::vector<shared_ptr<PartInstance> >& parts)
{
	if (!hitSignal.empty()) {
		for (size_t i = 0; i < parts.size(); ++i) {
			Primitive* p = parts[i]->getPartPrimitive();
			if (p->getRadius() < blastMaxObjectRadius()) {
				const Vector3 delta(p->getCoordinateFrame().translation - position);
				hitSignal(parts[i], delta.magnitude());
			}
		}
	}
}

void Explosion::doBlast(MegaClusterInstance* terrain, const std::vector<shared_ptr<PartInstance> >& parts)
{
	if (blastPressure > 0.0f) {
		World* world = Workspace::getWorldIfInWorkspace(this);
		if (!world)
			return;

		const std::vector<shared_ptr<PartInstance> >* unjoinParts = &parts;
		std::vector<shared_ptr<PartInstance> > ujParts;

		if (destroyJointRadiusPercent < 1.0f && destroyJointRadiusPercent > 0.0f)
		{
			float unlinkRadius = blastRadius * destroyJointRadiusPercent;
			Extents blastExtents = Extents::fromCenterRadius(position, unlinkRadius);

			G3D::Array<Primitive*> primitives;
			world->getContactManager()->getPrimitivesTouchingExtents(blastExtents, NULL, 0, primitives);
			PartInstance::primitivesToParts(primitives, ujParts);
			unjoinParts = &ujParts;
		}

		if (destroyJointRadiusPercent > 0.0f)
		{
			// Unjoin primitives if 1. smaller than maxRadius, and 2. Not in a field
			for (size_t i = 0; i < unjoinParts->size(); ++i) {
				PartInstance* part = (*unjoinParts)[i].get();
				Primitive* p = part->getPartPrimitive();
				if (p->getWorld()) {
					if (p->getRadius() < blastMaxObjectRadius()) {
						if (!ForceField::partInForceField(part)) {
							part->destroyJoints();
						}
					}
				}
			}
			world->assemble();					// reassemble the world - HACK - to do: find better place for this
			RBXASSERT(world->isAssembled());
		}

		// Give force and torque to primitives and unjoin them
		for (size_t i = 0; i < parts.size(); ++i) {
			Primitive* p = parts[i]->getPartPrimitive();
			if (p->getWorld()) {
				if (p->getRadius() < blastMaxObjectRadius()) {
					world->ticklePrimitive(p, true);
					Vector3 delta = (p->getCoordinateFrame().translation - position);
					Vector3 normal = (delta == Vector3::zero()) 
											? Vector3::unitY()
											: delta.direction();

					RBXASSERT(normal.length() <= 1.001);
					float radius = p->getRadius();
					float surfaceArea = radius * radius;
					Vector3 impulse = normal * blastPressure * surfaceArea * (1.0f / 4560.0f);	// normalizing factor

					PartInstance *pPart = parts[i].get();
					Humanoid *pHuman = Humanoid::humanoidFromBodyPart(pPart);
					if (pHuman) 
					{
						pHuman->setActivatePhysics(true, impulse);
					}

					p->getBody()->accumulateLinearImpulse(impulse, p->getCoordinateFrame().translation);
					p->getBody()->accumulateRotationalImpulse(impulse * 0.5 * radius);		// a somewhat arbitrary, but nice torque
				}
			}
		}
		
		// crater the terrain
		if (explosionType != NO_EFFECT){
			if (terrain){
                Extents blastExtents = Extents::fromCenterRadius(position, blastRadius);
                Extents terrainExtents = Voxel::getTerrainExtents();

                if (blastExtents.clampToOverlap(terrainExtents))
                {
					if (terrain->isSmooth())
					{
						try
						{
							terrain->fillBallInternal(position, blastRadius, AIR_MATERIAL, /* skipWater= */ true);
						}
						catch (RBX::base_exception& e)
						{
							StandardOut::singleton()->printf(MESSAGE_ERROR, "Explosion could not deform terrain: %s", e.what());
						}
					}
					else
					{
                        Vector3int16 blastRegionMin = Voxel::worldToCell_floor(blastExtents.min());
                        Vector3int16 blastRegionMax = Voxel::worldToCell_floor(blastExtents.max());

                        // take out a spherical crater!
                        for( int i = blastRegionMin.x; i <= blastRegionMax.x; ++i )
                        {
                            for( int j = blastRegionMin.y; j <= blastRegionMax.y; ++j )
                            {
                                for( int k = blastRegionMin.z; k <= blastRegionMax.z; ++k )
                                {
                                    Vector3int16 cellLocation(i,j,k);
                                    Vector3 cellCenterToExplosion =
                                        Voxel::cellToWorld_smallestCorner(cellLocation) + 
                                        (Vector3(Voxel::kCELL_SIZE, Voxel::kCELL_SIZE, Voxel::kCELL_SIZE) / 2) -
                                        position;
                                    if (cellCenterToExplosion.dot(cellCenterToExplosion) < blastRadius*blastRadius){
                                        terrain->getVoxelGrid()->setCell(cellLocation, Voxel::Constants::kUniqueEmptyCellRepresentation, Voxel::CELL_MATERIAL_Water);
                                    }
                                }
                            }
                        }

                        // now autowedge 'em
                        Region3int16 blastRegion(blastRegionMin, blastRegionMax);
                        terrain->autoWedgeCellsInternal(blastRegion);
					}
				}
			}
		}
	}
}

void Explosion::doKill()
{
	if (this->killRadius() > 0.0f) {
		World* world = Workspace::getWorldIfInWorkspace(this);
		if (world)
		{
			G3D::Array<Primitive*> primitives;
			world->getContactManager()->getPrimitivesTouchingExtents(
											Extents::fromCenterRadius(position, killRadius()),
											NULL, 0,
											primitives);

			for (int i = 0; i < primitives.size(); ++i) {
				Primitive* p = primitives[i];
				if (p->getRadius() < killMaxObjectRadius()) {
					PartInstance* part = PartInstance::fromPrimitive(p);

					if (!ForceField::partInForceField(part)) {
						part->setParent(NULL);
					}
				}
			}
		}
	}
}

bool Explosion::askSetParent(const Instance* instance) const
{
	// Explosion can be a child anything
	return true;
}



void Explosion::onStepped(const Stepped& event)
{
	stopStepping();

	if (blastRadius>0) {
		World* world = Workspace::getWorldIfInWorkspace(this);
		// Make sure the Explosion is still in the world
		if (world)
		{
			Extents blastExtents = Extents::fromCenterRadius(position, blastRadius);

			G3D::Array<Primitive*> primitives;
			world->getContactManager()->getPrimitivesTouchingExtents(blastExtents, NULL, 0, primitives);
			std::vector<shared_ptr<PartInstance> > parts;
			PartInstance::primitivesToParts(primitives, parts);

			signalBlast(parts);

			Workspace* myWorkspace = Workspace::getWorkspaceIfInWorkspace(this);

			MegaClusterInstance* terrainInstance = (MegaClusterInstance*)(myWorkspace->getTerrain());
			doBlast(terrainInstance, parts);
		}
	}

	doKill();

	// Remove myself from the world after a little while (need to wait to ensure explosion is drawn!)
	float lifeTime = 1.0f;
    if (FFlag::RenderNewExplosionEnable) 
    {{{ 
        lifeTime = 3.0f;
    }}}
	if (blastRadius >= 1.0f)
		lifeTime -= (1.0f / blastRadius);

	TimerService* ts = ServiceProvider::create<TimerService>(this);
	if (ts)
		ts->delay(boost::bind(&Instance::setParent, shared_from(this), (Instance*)NULL), lifeTime);	
	else
		setParent(NULL);
}

void Explosion::render3dAdorn(Adorn* adorn) 
{
	adorn->setObjectToWorldMatrix(position);
	adorn->explosion(Sphere(Vector3::zero(), visualRadius()));
}


} // namespace
