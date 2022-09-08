/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/PartInstance.h"

namespace RBX {

extern const char* const sExtrudedPart;

class ExtrudedPartInstance
	: public DescribedCreatable<ExtrudedPartInstance, PartInstance, sExtrudedPart>
{
private:
	typedef DescribedCreatable<ExtrudedPartInstance, PartInstance, sExtrudedPart> Super;
public:
	ExtrudedPartInstance();
	~ExtrudedPartInstance();

	enum VisualTrussStyle
	{
		FULL_ALTERNATING_CROSS_BEAM = 0, //classic style
		BRIDGE_STYLE_CROSS_BEAM,
		NO_CROSS_BEAM,
		//OnlyInternalCrossBeam
	};

	static const Reflection::EnumPropDescriptor<ExtrudedPartInstance, VisualTrussStyle> prop_styleXml;
 
	/*override*/ virtual PartType getPartType() const { return TRUSS_PART; }
	/*override*/ virtual const Vector3& getMinimumUiSize() const;
	/*override*/ virtual Faces getResizeHandleMask() const;
	/*override*/ virtual int getResizeIncrement() const;
	/*override*/ virtual void setPartSizeXml(const Vector3& rbxSize);

	void			 setVisualTrussStyle(VisualTrussStyle value);
	VisualTrussStyle getVisualTrussStyle() const { return visualTrussStyle; }
private:
	VisualTrussStyle visualTrussStyle;
};

} // namespace