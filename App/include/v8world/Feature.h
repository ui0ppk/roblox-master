/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/G3DCore.h"
#include "rbx/Debug.h"

namespace RBX {

	class Primitive;

	namespace GEO {

	class RBXBaseClass Feature
	{
	private:
		Primitive*		primitive;
		int				index;

	public:
		typedef enum {VERTEX, EDGE, FACE} FeatureType;

		Feature(Primitive* primitive, int index) : primitive(primitive), index(index)
		{}

		virtual FeatureType getFeatureType() const = 0;
	};

	class Vertex : public Feature
	{
	public:
		Vertex(Primitive* primitive, int index) : Feature(primitive, index)
		{}

		/*override*/ FeatureType getFeatureType() const {return VERTEX;}
	};

	class Edge : public Feature
	{
	public:
		Edge(Primitive* primitive, int index) : Feature(primitive, index)
		{}

		/*override*/ FeatureType getFeatureType() const {return EDGE;}
	};

	class Face : public Feature
	{
	public:
		Face(Primitive* primitive, int index) : Feature(primitive, index)
		{}

		/*override*/ FeatureType getFeatureType() const {return FACE;}
	};

	} // namespace Feature

} // namespace