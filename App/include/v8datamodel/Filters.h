/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/G3DCore.h"
#include "Util/HitTestFilter.h"
#include "rbx/boost.hpp"

namespace RBX {

	class Instance;
	class Primitive;
	class Humanoid;
	class ModelInstance;
	class PVInstance;
	class PartInstance;

	typedef std::vector<shared_ptr<Instance> > Instances;

	/////////////////////////////////////////////////////////////
	class Unlocked : public HitTestFilter
	{
	public:
		static bool unlocked(const Primitive* testMe);
		/*override*/ Result filterResult(const Primitive* testMe) const 
		{
			return unlocked(testMe) ? HitTestFilter::INCLUDE_PRIM : HitTestFilter::STOP_TEST;
		}
	};

	/////////////////////////////////////////////////////////////
	//
	// exclude the user character from the hit test

	class PartByLocalCharacter : public HitTestFilter		
	{
	protected:
		shared_ptr<ModelInstance> character;
		shared_ptr<PartInstance> head;
	public:
		PartByLocalCharacter(Instance* root);
		/*override*/ Result filterResult(const Primitive* testMe) const;
	};

	/////////////////////////////////////////////////////////////
	//

	class UnlockedPartByLocalCharacter : public PartByLocalCharacter
	{
	public:
		UnlockedPartByLocalCharacter(Instance* root) : PartByLocalCharacter(root) {}
		/*override*/ Result filterResult(const Primitive* testMe) const;
	};

	class FilterInvisibleNonColliding : public HitTestFilter
	{
	public:
		FilterInvisibleNonColliding();
		/*override*/ Result filterResult(const Primitive* testMe) const;
	}; 

	class FilterDescendents : public HitTestFilter
	{
	protected:
		shared_ptr<Instance> inst;
	public:
		FilterDescendents(shared_ptr<Instance> i);
		/*override*/ Result filterResult(const Primitive* testMe) const;
	};

	class FilterDescendentsList : public HitTestFilter
	{
	protected:
		const Instances* instances;
	public:
		FilterDescendentsList(const Instances* i);
		/*override*/ Result filterResult(const Primitive* testMe) const;
	};

	class MergedFilter : public HitTestFilter
	{
	protected:
		const HitTestFilter* aFilter;
		const HitTestFilter* bFilter;
	public:
		MergedFilter(const HitTestFilter *a, const HitTestFilter* b);
		/*override*/ Result filterResult(const Primitive* testMe) const;
	};

	//////////////////////////////
	class FilterCharacterOcclusion : public HitTestFilter
	{
	private:
		float headHeight;
	public:
		FilterCharacterOcclusion(float headHeight);
		/*override*/ Result filterResult(const Primitive* testMe) const;
	};

	//////////////////////////////
	class FilterHumanoidParts : public HitTestFilter
	{
	public:
		/*override*/ Result filterResult(const Primitive* testMe) const;
	};

	//////////////////////////////
	class FilterHumanoidNameOcclusion : public HitTestFilter
	{
	protected:
		shared_ptr<Humanoid> inst;
	public:
		FilterHumanoidNameOcclusion(shared_ptr<Humanoid> i);
		/*override*/ Result filterResult(const Primitive* testMe) const;
	};

	/// Filter for checking if Prim is in same assembly.
	class FilterSameAssembly : public HitTestFilter
	{
	protected:
		shared_ptr<PartInstance> assemblyPart;
	public:
		FilterSameAssembly(shared_ptr<PartInstance> part)
		{
			assemblyPart = part;
		};
		/*override*/ Result filterResult(const Primitive* testMe) const;
	};

} // namespace RBX