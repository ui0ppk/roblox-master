/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/G3DCore.h"
#include "Util/Memory.h"

namespace RBX {

	class Body;

	class Cofm : public Allocator<Cofm>
	{
	private:
		Body*				body;
		bool				dirty;
		Vector3				cofmInBody;
		float				mass;
		Matrix3				moment;

		void				updateIfDirty();	// true if was dirty

	public:
		Cofm(Body* body);

		bool				getIsDirty() const {return dirty;}

		void				makeDirty() {
			dirty = true;
		}

		const Vector3& getCofmInBody() {
			updateIfDirty();
			return cofmInBody;
		}

        float getMass() {
			updateIfDirty();
			return mass;
		}

		const Matrix3& getMoment() {
			updateIfDirty();
			return moment;
		}
	};

} // namespace
