/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/GuiBase3d.h"
#include "GfxBase/IAdornable.h"

namespace RBX
{
	class PartInstance;
	class Humanoid;

	extern const char* const sSelectionLasso;

	class SelectionLasso 
		: public DescribedNonCreatable<SelectionLasso, GuiBase3d, sSelectionLasso>
	{
	private:
		typedef DescribedNonCreatable<SelectionLasso, GuiBase3d, sSelectionLasso> Super;
	public:
		SelectionLasso(const char* name);

		const Humanoid*		getHumanoid() const				{ return humanoid.lock().get(); }
		Humanoid*			getHunanoid()					{ return humanoid.lock().get(); }
		void				setHumanoid(Humanoid* value);
		Humanoid*			getHumanoidDangerous() const	{ return humanoid.lock().get(); }

		virtual bool getPosition(Vector3& output) const = 0;

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IAdornable
		/*override*/ bool shouldRender3dAdorn() const;
		/*override*/ void render3dAdorn(Adorn* adorn);
	protected:
		bool getHumanoidPosition(Vector3& output) const;
		
		
		weak_ptr<Humanoid> humanoid;
	};


	extern const char* const sSelectionPartLasso;

	class SelectionPartLasso 
		: public DescribedCreatable<SelectionPartLasso, SelectionLasso, sSelectionPartLasso>
	{
	private:
		typedef DescribedCreatable<SelectionPartLasso, SelectionLasso, sSelectionPartLasso> Super;
	public:
		SelectionPartLasso();
		
		/*override*/ bool getPosition(Vector3& output) const;

		const PartInstance* getPart() const					{ return part.lock().get(); }
		PartInstance*		getPart()						{ return part.lock().get(); }
		void				setPart(PartInstance* value);
		PartInstance*		getPartDangerous() const		{ return part.lock().get(); }

	protected:
		/*override*/ bool shouldRender3dAdorn() const;
		weak_ptr<PartInstance> part;
	};

	extern const char* const sSelectionPointLasso;

	class SelectionPointLasso 
		: public DescribedCreatable<SelectionPointLasso, SelectionLasso, sSelectionPointLasso>
	{
	public:
		SelectionPointLasso();

		void setPoint(Vector3 value);
		Vector3 getPoint() const { return point; }
		
		/*override*/ bool getPosition(Vector3& output) const { output = point; return true; }
	protected:
		Vector3 point;
	};
}


