#include "stdafx.h"

#include "V8Datamodel/Decal.h"

using namespace RBX;

const char* const RBX::sDecal = "Decal";

const Reflection::PropDescriptor<Decal, TextureId> Decal::prop_Texture("Texture", category_Appearance, &Decal::getTexture, &Decal::setTexture);
const Reflection::PropDescriptor<Decal, float> Decal::prop_Specular("Specular", category_Appearance, &Decal::getSpecular, &Decal::setSpecular, Reflection::PropertyDescriptor::Attributes::deprecated());
const Reflection::PropDescriptor<Decal, float> Decal::prop_Shiny("Shiny", category_Appearance, &Decal::getShiny, &Decal::setShiny, Reflection::PropertyDescriptor::Attributes::deprecated());
const Reflection::PropDescriptor<Decal, float> Decal::prop_Transparency("Transparency", category_Appearance, &Decal::getTransparency,
																		&Decal::setTransparency);
const Reflection::PropDescriptor<Decal, float> Decal::prop_LocalTransparencyModifier("LocalTransparencyModifier", category_Appearance, &Decal::getLocalTransparencyModifier,
																		&Decal::setLocalTransparencyModifier, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING);

Decal::Decal(void)
	: specular(0.0)
	, shiny(20.0)
	, transparency(0.0f)
	,localTransparencyModifier(0.0f)
{
	setName("Decal");
}

void Decal::setTexture(TextureId value) 
{ 
	if (this->texture != value)
	{
		this->texture = value; 
		this->raisePropertyChanged(prop_Texture);
	}
}

void Decal::setSpecular(float value) 
{ 
	if (this->specular!=value && value>=0.0)
	{
		this->specular = value; 
		this->raisePropertyChanged(prop_Specular);
	}
}


void Decal::setShiny(float value) 
{ 
	if (this->shiny!=value && value>0.0)
	{
		this->shiny = value; 
		this->raisePropertyChanged(prop_Shiny);
	}
}

float Decal::getTransparencyUi() const
{
	return (1.0f - ((1.0f - localTransparencyModifier) * (1.0f - transparency))); 
}

void Decal::setTransparency(float value)
{
	if (value != transparency)
	{
		this->transparency = value;
		this->raisePropertyChanged(prop_Transparency);
	}
}


void Decal::setLocalTransparencyModifier(float value)
{
	float clamped = G3D::clamp(value,0.0f,1.0f);

	if (clamped != localTransparencyModifier)
	{
		this->localTransparencyModifier = clamped;
		this->raisePropertyChanged(prop_LocalTransparencyModifier);
	}
}

namespace RBX {

// TODO: Refactor: It is a little ugly to have to implement these 6 functions for each "ContentID" derived class
//  Potentially we can template this a little. Maybe define a templated ContentIDPropDescriptor or something.
template<>
std::string StringConverter<TextureId>::convertToString(const TextureId& value)
{
	return value.toString();
}

template<>
bool StringConverter<TextureId>::convertToValue(const std::string& text, TextureId& value)
{
	value = text;
	return true;
}

namespace Reflection {

template<>
const Type& Type::getSingleton<RBX::TextureId>()
{
	return Type::singleton<ContentId>();
}

template<>
void TypedPropertyDescriptor<RBX::TextureId>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		ContentId value;
		if (element->getValue(value))
			setValue(instance, value);
	}
}

template<>
void TypedPropertyDescriptor<RBX::TextureId>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	element->setValue(ContentId(getValue(instance)));
}


template<>
TextureId& Variant::convert<TextureId>(void)
{
	if (_type->isType<std::string>())
	{
		value = TextureId(cast<std::string>());
		_type = &Type::singleton<TextureId>();
	}
	return genericConvert<TextureId>();
}

template<>
int TypedPropertyDescriptor<RBX::TextureId>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(RBX::TextureId) + getValue(instance).toString().size();
}

template<>
bool TypedPropertyDescriptor<RBX::TextureId>::hasStringValue() const {
	return true;
}

template<>
std::string TypedPropertyDescriptor<RBX::TextureId>::getStringValue(const DescribedBase* instance) const{
	return StringConverter<RBX::TextureId>::convertToString(getValue(instance));
}

template<>
bool TypedPropertyDescriptor<RBX::TextureId>::setStringValue(DescribedBase* instance, const std::string& text) const {
	RBX::TextureId value;
	if (StringConverter<RBX::TextureId>::convertToValue(text, value))
	{
		setValue(instance, value);
		return true;
	}
	else
		return false;
}

}} //namespace

const char* const RBX::sDecalTexture = "Texture";

const Reflection::PropDescriptor<DecalTexture, float> DecalTexture::prop_StudsPerTileU("StudsPerTileU", category_Appearance, &DecalTexture::getStudsPerTileU, &DecalTexture::setStudsPerTileU);
const Reflection::PropDescriptor<DecalTexture, float> DecalTexture::prop_StudsPerTileV("StudsPerTileV", category_Appearance, &DecalTexture::getStudsPerTileV, &DecalTexture::setStudsPerTileV);

DecalTexture::DecalTexture(void)
	:studsPerTile(2.0f,2.0f)
{
	setName("Texture");
}


void DecalTexture::setStudsPerTileU(float value) 
{ 
	if (this->studsPerTile.x!=value && value>0.0)
	{
		this->studsPerTile.x = value; 
		this->raisePropertyChanged(prop_StudsPerTileU);
	}
}


void DecalTexture::setStudsPerTileV(float value) 
{ 
	if (this->studsPerTile.y!=value && value>0.0)
	{
		this->studsPerTile.y = value; 
		this->raisePropertyChanged(prop_StudsPerTileV);
	}
}



