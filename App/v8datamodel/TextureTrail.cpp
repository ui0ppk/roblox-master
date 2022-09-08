#include "stdafx.h"

#include "V8DataModel/TextureTrail.h"

#include "GfxBase/Adorn.h"
#include "GfxBase/TextureProxyBase.h"
#include "rbx/rbxTime.h"
#include "reflection/reflection.h"
#include "util/TextureId.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/GuiBase3d.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8Tree/Service.h"

namespace RBX {

const char* const sTextureTrail = "TextureTrail";

Reflection::RefPropDescriptor<TextureTrail, PartInstance>
TextureTrail::prop_From("From", category_Data,
		&TextureTrail::getFrom, &TextureTrail::setFrom,
		Reflection::PropertyDescriptor::STANDARD, Security::RobloxPlace);

Reflection::RefPropDescriptor<TextureTrail, PartInstance>
TextureTrail::prop_To("To", category_Data,
		&TextureTrail::getTo, &TextureTrail::setTo,
		Reflection::PropertyDescriptor::STANDARD, Security::RobloxPlace);

Reflection::PropDescriptor<TextureTrail, TextureId>
TextureTrail::prop_Texture("Texture", category_Appearance,
		&TextureTrail::getTexture, &TextureTrail::setTexture,
		Reflection::PropertyDescriptor::STANDARD, Security::RobloxPlace);

Reflection::PropDescriptor<TextureTrail, Vector2>
TextureTrail::prop_TextureSize("TextureSize", category_Appearance,
		&TextureTrail::getTextureSize, &TextureTrail::setTextureSize,
		Reflection::PropertyDescriptor::STANDARD, Security::RobloxPlace);

Reflection::PropDescriptor<TextureTrail, float>
TextureTrail::prop_Velocity("Velocity", category_Data,
		&TextureTrail::getVelocity, &TextureTrail::setVelocity,
		Reflection::PropertyDescriptor::STANDARD, Security::RobloxPlace);

Reflection::PropDescriptor<TextureTrail, float>
TextureTrail::prop_StudsBetweenTextures("StudsBetweenTextures", category_Data,
		&TextureTrail::getStudsBetweenTextures,
		&TextureTrail::setStudsBetweenTextures,
		Reflection::PropertyDescriptor::STANDARD, Security::RobloxPlace);

Reflection::PropDescriptor<TextureTrail, float>
TextureTrail::prop_CycleOffset("CycleOffset", category_Data,
		&TextureTrail::getCycleOffset, &TextureTrail::setCycleOffset,
		Reflection::PropertyDescriptor::STANDARD, Security::RobloxPlace);

TextureTrail::TextureTrail()
	: DescribedCreatable<TextureTrail, GuiBase3d, sTextureTrail>(sTextureTrail),
	textureSize(1, 1),
	velocity(2.0f),
	studsBetweenTextures(4.0f),
	cycleOffset(0.0f)
{
}

void TextureTrail::setFrom(PartInstance* value) {
	setPartInstance(from, value, prop_From);
}

PartInstance* TextureTrail::getFrom() const {
	return from.lock().get();
}

void TextureTrail::setTo(PartInstance* value) {
	setPartInstance(to, value, prop_To);
}

PartInstance* TextureTrail::getTo() const {
	return to.lock().get();
}

void TextureTrail::setTexture(TextureId texture) {
	this->texture = texture;
}

TextureId TextureTrail::getTexture() const {
	return texture;
}

void TextureTrail::setTextureSize(Vector2 textureSize) {
	this->textureSize.x = textureSize.x;
	this->textureSize.y = textureSize.y;
}

Vector2 TextureTrail::getTextureSize() const {
	return textureSize;
}

void TextureTrail::setVelocity(float velocity) {
	this->velocity = velocity;
}

float TextureTrail::getVelocity() const {
	return velocity;
}

void TextureTrail::setStudsBetweenTextures(float spacing) {
	this->studsBetweenTextures = spacing;
}

float TextureTrail::getStudsBetweenTextures() const {
	return studsBetweenTextures;
}

void TextureTrail::setCycleOffset(float cycleOffset) {
	this->cycleOffset = cycleOffset;
}

float TextureTrail::getCycleOffset() const {
	return cycleOffset;
}

void TextureTrail::render3dAdorn(Adorn *adorn) {
	Vector3 fromPosition, toPosition;
	if (!getPosition(from, &fromPosition) || !getPosition(to, &toPosition)) {
		return;
	}
	
	if (texture.isNull()) {
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

	renderInternal(workspace, camera, fromPosition, toPosition, texture, getFullName() + ".Texture",
			textureSize, velocity, studsBetweenTextures, cycleOffset, Color4(color, 1.0f - getTransparency()), adorn);
}

void TextureTrail::renderInternal(
		const Workspace* workspace, const Camera* camera,
		const Vector3& fromPosition, const Vector3& toPosition,
		const TextureId& texture, std::string context, const Vector2& textureSize,
		float velocity, float studsBetweenTextures, float cycleOffset,
		const Color4& color, Adorn* adorn) {

	// create a coordinate frame centered on From with -z pointed at To
	CoordinateFrame trailOnFrom(fromPosition);
	trailOnFrom.lookAt(toPosition);
	float distance = (fromPosition - toPosition).length();

	CoordinateFrame cameraInObjectSpace(
			trailOnFrom.toObjectSpace(camera->getCameraCoordinateFrame()));

	float spaceBetweenIcons = studsBetweenTextures;
	double gameTime = Time::now<Time::Fast>().timestampSeconds();

	// calculate an offset based on time to make the textures move along
	// the line.
	// Velocity is measured in studs per second.
	// CycleOffset is treated as fraction of the space between textures
	// (-1 to 1) to move the texture forward or backward in the loop.
	float timeOffset = spaceBetweenIcons *
			fmod((velocity / spaceBetweenIcons) * gameTime + cycleOffset, 1);
	bool waitingFallback;

	TextureProxyBaseRef textureRef = 
			adorn->createTextureProxy(texture, waitingFallback, true /* blocking */, context);
	adorn->setTexture(0, textureRef);

	bool cameraPositiveX = cameraInObjectSpace.translation.x > 0;
	float halfWidth = textureSize.x / 2;
	float halfHeight = textureSize.y / 2;

	for (float i = 0; i < distance + spaceBetweenIcons; i += spaceBetweenIcons) {
		// build icons starting at "To" so that the stream will look anchored on
		// that end
		float zLoc = distance - i + timeOffset;
		if (zLoc >= 0 && zLoc <= distance) {
			CoordinateFrame objectLocationLookingAtCamera(Vector3(0, 0, -zLoc));
			objectLocationLookingAtCamera.lookAt(cameraInObjectSpace.translation,
					Vector3::unitZ());

			Vector2 topLeft, bottomRight;
			if (cameraPositiveX) {
				topLeft = Vector2(0, 0);
				bottomRight = Vector2(1, 1);
			} else {
				// The look at function will cause the sprite to be upside down
				// when the camera is in negative X space. To compensate,
				// invert the Y axis of the sprite (the X axis is maintained,
				// so that if a sprite has left-right directionality, that will
				// be preserved).
				topLeft = Vector2(0, 1);
				bottomRight = Vector2(1, 0);
			}

			adorn->setObjectToWorldMatrix(trailOnFrom * objectLocationLookingAtCamera);
			adorn->quad(
					Vector3(-halfHeight,  halfWidth, 0),
					Vector3(-halfHeight, -halfWidth, 0),
					Vector3( halfHeight,  halfWidth, 0),
					Vector3( halfHeight, -halfWidth, 0),
					color, topLeft, bottomRight);
		}
	}

	// clear texture; if this is not done, other adorns may accidentally
	// gain a texture
	adorn->setTexture(0, TextureProxyBaseRef());
}

void TextureTrail::setPartInstance(weak_ptr<PartInstance>& data,
		PartInstance* newValue, const PartProp& prop) {
	if(data.lock().get() != newValue){
		data = shared_from(newValue);
		raisePropertyChanged(prop);
	}
}

bool TextureTrail::getPosition(const weak_ptr<PartInstance>& dataSource,
		Vector3* out) {
	PartInstance* dataPtr = dataSource.lock().get();
	if (dataPtr) {
		(*out) = dataPtr->getTranslationUi();
		return true;
	}
	return false;
}

}
