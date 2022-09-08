#include "stdafx.h"

#include "V8DataModel/FloorWire.h"
#include "GfxBase/Adorn.h"
#include "GfxBase/TextureProxyBase.h"
#include "reflection/reflection.h"
#include "util/TextureId.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/Filters.h"
#include "V8DataModel/GuiBase3d.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/TextureTrail.h"
#include "V8DataModel/Workspace.h"
#include "V8Tree/Service.h"
#include "V8World/ContactManager.h"
#include "V8World/World.h"

namespace RBX {

const char* const sFloorWire = "FloorWire";
const float FloorWire::kMinDistanceFromBlocks = 0.2f;
const int FloorWire::kMaxSegments = 7;

Reflection::RefPropDescriptor<FloorWire, PartInstance>
FloorWire::prop_From("From", category_Data,
		&FloorWire::getFrom, &FloorWire::setFrom,
		Reflection::PropertyDescriptor::STANDARD);

Reflection::RefPropDescriptor<FloorWire, PartInstance>
FloorWire::prop_To("To", category_Data,
		&FloorWire::getTo, &FloorWire::setTo,
		Reflection::PropertyDescriptor::STANDARD);

Reflection::PropDescriptor<FloorWire, TextureId>
FloorWire::prop_Texture("Texture", category_Appearance,
		&FloorWire::getTexture, &FloorWire::setTexture,
		Reflection::PropertyDescriptor::STANDARD);

Reflection::PropDescriptor<FloorWire, Vector2>
FloorWire::prop_TextureSize("TextureSize", category_Appearance,
		&FloorWire::getTextureSize, &FloorWire::setTextureSize,
		Reflection::PropertyDescriptor::STANDARD);

Reflection::PropDescriptor<FloorWire, float>
FloorWire::prop_Velocity("Velocity", category_Data,
		&FloorWire::getVelocity, &FloorWire::setVelocity,
		Reflection::PropertyDescriptor::STANDARD);

Reflection::PropDescriptor<FloorWire, float>
FloorWire::prop_StudsBetweenTextures("StudsBetweenTextures", category_Data,
		&FloorWire::getStudsBetweenTextures,
		&FloorWire::setStudsBetweenTextures,
		Reflection::PropertyDescriptor::STANDARD);

Reflection::PropDescriptor<FloorWire, float>
FloorWire::prop_CycleOffset("CycleOffset", category_Data,
		&FloorWire::getCycleOffset, &FloorWire::setCycleOffset,
		Reflection::PropertyDescriptor::STANDARD);

Reflection::PropDescriptor<FloorWire, float>
FloorWire::prop_WireRadius("WireRadius", category_Data,
		&FloorWire::getWireRadius, &FloorWire::setWireRadius,
		Reflection::PropertyDescriptor::STANDARD);

FloorWire::FloorWire()
	: DescribedCreatable<FloorWire, GuiBase3d, sFloorWire>(sFloorWire),
		textureSize(1, 1),
		velocity(2.0f),
		studsBetweenTextures(4.0f),
		cycleOffset(0.0f),
		wireRadius(.0625)
{
}

void FloorWire::setFrom(PartInstance* value) {
	setPartInstance(from, value, prop_From);
}

PartInstance* FloorWire::getFrom() const {
	return from.lock().get();
}

void FloorWire::setTo(PartInstance* value) {
	setPartInstance(to, value, prop_To);
}

PartInstance* FloorWire::getTo() const {
	return to.lock().get();
}

void FloorWire::setTexture(TextureId texture) {
	this->texture = texture;
}

TextureId FloorWire::getTexture() const {
	return texture;
}

void FloorWire::setTextureSize(Vector2 textureSize) {
	this->textureSize.x = textureSize.x;
	this->textureSize.y = textureSize.y;
}

Vector2 FloorWire::getTextureSize() const {
	return textureSize;
}

void FloorWire::setVelocity(float velocity) {
	this->velocity = velocity;
}

float FloorWire::getVelocity() const {
	return velocity;
}

void FloorWire::setStudsBetweenTextures(float spacing) {
	if (spacing > 0)
	{
		this->studsBetweenTextures = spacing;
	}
	raisePropertyChanged(prop_StudsBetweenTextures);
}

float FloorWire::getStudsBetweenTextures() const {
	return studsBetweenTextures;
}

void FloorWire::setCycleOffset(float cycleOffset) {
	this->cycleOffset = cycleOffset;
}

float FloorWire::getCycleOffset() const {
	return cycleOffset;
}

void FloorWire::setWireRadius(float wireRadius) {
	this->wireRadius = wireRadius;
}

float FloorWire::getWireRadius() const {
	return wireRadius;
}

void FloorWire::render3dAdorn(Adorn *adorn) {
	shared_ptr<PartInstance> from(this->from.lock());
	shared_ptr<PartInstance> to(this->to.lock());
	if (!from.get() || !to.get()) {
		return;
	}

	Workspace* workspace = ServiceProvider::find<Workspace>(this);
	if (!workspace) {
		return;
	}

	const Camera* camera = workspace->getConstCamera();
	if (!camera) {
		return;
	}

	if (!DataModel::get(from.get()) || !DataModel::get(to.get())) {
		return;
	}

	std::vector<Vector3> trailSegments;
	buildTrailSegments(workspace, from, to, &trailSegments);
	drawSegments(workspace, camera, trailSegments, adorn);
}

void FloorWire::setPartInstance(weak_ptr<PartInstance>& data,
		PartInstance* newValue, const PartProp& prop) {
	if(data.lock().get() != newValue){
		data = shared_from(newValue);
		raisePropertyChanged(prop);
	}
}

void FloorWire::computeSurfacePosition(const shared_ptr<PartInstance>& part,
		const RbxRay& ray, Vector3* out) {
	int surfaceId;
	Surface surface = part->getSurface(ray, surfaceId);

	if (surface.getPartInstance() == NULL) {
		return;
	}

	const Vector3 halfPartSize(part->getPartSizeUi() / 2);
	Vector3 offset(0, 0, 0);
	switch (surface.getNormalId()) {
		case NORM_X:
			offset.x = halfPartSize.x + kMinDistanceFromBlocks;
			break;
		case NORM_Y:
			offset.y = halfPartSize.y + kMinDistanceFromBlocks;
			break;
		case NORM_Z:
			offset.z = halfPartSize.z + kMinDistanceFromBlocks;
			break;
		case NORM_X_NEG:
			offset.x = -halfPartSize.x - kMinDistanceFromBlocks;
			break;
		case NORM_Y_NEG:
			offset.y = -halfPartSize.y - kMinDistanceFromBlocks;
			break;
		case NORM_Z_NEG:
			offset.z = -halfPartSize.z - kMinDistanceFromBlocks;
			break;
        default:
            break;
	}

	(*out) = part->getCoordinateFrame().pointToWorldSpace(offset);
}

bool FloorWire::incrementalBuildSegments(const Workspace* workspace,
		const ContactManager* contactManager, const Vector3& dest,
		bool moveInX, std::vector<Vector3>* out) {
	const Vector3& source(out->back());
	const Vector3 deltaGoal = dest - source;

	// if the xz distance is small, declare the path complete
	if ((source.xz() - dest.xz()).length() < 2 * kMinDistanceFromBlocks) {
		return true;
	}

	// if the path is too long, give up
	if (out->size() > kMaxSegments) {
		return false;
	}

	// if we need to move in the x/z direction, and there is no more space
	// to move in that dimension, and we aren't close enough to be completed
	// (see above check) then give up.
	if ((moveInX && fabs(deltaGoal.x) < kMinDistanceFromBlocks) ||
			(!moveInX && fabs(deltaGoal.z) < kMinDistanceFromBlocks)) {
		return false;
	}

	// cast a ray in the dx or dz direction
	RbxRay hitRay(source, Vector3(moveInX ? deltaGoal.x : 0, 0, !moveInX ? deltaGoal.z : 0));

	Vector3 hitPoint;
	FilterHumanoidParts filterHumanoidParts;
	Primitive* hitPrimitive = contactManager->getHit(hitRay,
			NULL, &filterHumanoidParts,
			hitPoint);

	float achievableDelta = moveInX ? deltaGoal.x : deltaGoal.z;
	if (hitPrimitive) {
		if (moveInX) {
			achievableDelta = hitPoint.x - source.x;
		} else {
			achievableDelta = hitPoint.z - source.z;
		}
		if (achievableDelta > 0) {
			achievableDelta -= kMinDistanceFromBlocks;
		} else {
			achievableDelta += kMinDistanceFromBlocks;
		}
	}

	// if we need to move in the x/z direction, but there is a very close
	// object in that direction, then give up.
	if (fabs(achievableDelta) < 2 * kMinDistanceFromBlocks) {
		return false;
	}

	if (moveInX) {
		out->push_back(Vector3(achievableDelta, 0, 0) + source);
	} else {
		out->push_back(Vector3(0, 0, achievableDelta) + source);
	}

	return incrementalBuildSegments(workspace, contactManager, dest, !moveInX, out);
}

void FloorWire::buildTrailSegments(const Workspace* workspace,
		const shared_ptr<PartInstance>& from,
		const shared_ptr<PartInstance>& to,
		std::vector<Vector3>* out) {

	// get centers of the end parts
	Vector3 fromPosition(from->getTranslationUi());
	Vector3 toPosition(to->getTranslationUi());

	// draw no segments if the ends are too close together
	if ((fromPosition - toPosition).magnitude() < 2 * kMinDistanceFromBlocks) {
		return;
	}

	// get terminal positions on one of the faces of each end part
	computeSurfacePosition(from, RbxRay(toPosition, fromPosition - toPosition),
			&fromPosition);
	computeSurfacePosition(to, RbxRay(fromPosition, toPosition - fromPosition),
			&toPosition);

	// cast a ray down from the start point to approximate "ground"
	const ContactManager* contactManager =
			workspace->getWorld()->getContactManager();
	RbxRay downAtFromPosition(fromPosition,
			Vector3(0, -1 * (toPosition - fromPosition).magnitude(), 0));
	Vector3 hitPoint;
	FilterHumanoidParts filterHumanoidParts;
	Primitive* ground = contactManager->getHit(downAtFromPosition,
			NULL, &filterHumanoidParts,
			hitPoint);

	out->push_back(fromPosition);

	if (ground) {
		float groundHeight = hitPoint.y + kMinDistanceFromBlocks;
		Vector3 groundPoint(fromPosition.x, groundHeight, fromPosition.z);
		out->push_back(groundPoint);

		// try building segments starting in the x direction first
		if (!incrementalBuildSegments(workspace, contactManager, toPosition,
					true, out)) {
			// failed, reset state and try building segmentsz in the z direction first
			out->clear();
			out->push_back(fromPosition);
			out->push_back(groundPoint);
			if (!incrementalBuildSegments(workspace, contactManager, toPosition,
					false, out)) {
				out->clear();
				out->push_back(fromPosition);
			}
		}
	}

	out->push_back(toPosition);
}

void FloorWire::drawSegments(const Workspace* workspace, const Camera* camera,
		const std::vector<Vector3>& segments, Adorn* adorn) {

	if (segments.empty()) {
		return;
	}

	Color4 colorWire(color, 1.0f - getTransparency());	
	Color4 colorTexture(Color3::white(), 1.0f - getTransparency());	
	
	Matrix3 rotateXtoNegZ(0,0,1,0,1,0,-1,0,0); // rotate about y by 90 degrees. (+x -> -z)

	// simulate a combined texture trail by keeping track of how much slack
	// (in studs) the previous segment added.
	// Example: segments of size 3, 7, and 4, with texture spacing of 4
	// at time 0. Put a texture at the end of the first segment. When
	// computing the textures for the 7 segment, we should simulate the
	// segment being (3 mod 4) = 3 segments longer (cycle offset of 3/4).
	// when computing the texture locations for the 4 segment, we should
	// simulate that the line was an addition (3 + 7 mod 4) = 2 segments
	// longer (cycle offset of 2/4).
	float lastOffset = 0;

	for (size_t vertex = segments.size() - 1; vertex > 0; --vertex) {
		const Vector3& from(segments[vertex - 1]);
		const Vector3& to(segments[vertex]);

		float distance = (to - from).magnitude();

		CoordinateFrame cframe((from + to) / 2);
		cframe.lookAt(to);
		cframe.rotation *= rotateXtoNegZ;
		
		adorn->setObjectToWorldMatrix(cframe);
		adorn->cylinderAlongX(wireRadius, distance, colorWire);

		// add ball joint at all intersections except the last one
		if (vertex < segments.size() - 1) {
			adorn->setObjectToWorldMatrix(CoordinateFrame());
			adorn->sphere(Sphere(to, 1.1 * wireRadius), colorWire);
		}

		if (!texture.isNull()) {
			TextureTrail::renderInternal(workspace, camera,
					from, to, texture, getFullName() + ".Texture",
					textureSize, velocity, studsBetweenTextures,
					(lastOffset / studsBetweenTextures) + cycleOffset,
					colorTexture, adorn);
			lastOffset = fmod(lastOffset + distance, studsBetweenTextures);
		}
	}
}

}