#pragma once

#include "AppDraw/DrawAdorn.h"
#include "GfxBase/Adorn.h"
#include "GfxBase/IAdornable.h"

namespace RBX {

	class Workspace;
	class Instance;
    class PVInstance;

	class ConstraintSurfacePair 
	{
	protected:
		const Primitive* p0;
		const Primitive* p1;
		size_t s0;
		size_t s1;

	public:

		rbx::signals::scoped_connection p0AncestorChangedConnection;
		rbx::signals::scoped_connection p1AncestorChangedConnection;

		ConstraintSurfacePair(const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id);
		ConstraintSurfacePair();
		virtual ~ConstraintSurfacePair() {}

		void setP0(Primitive* prim) { p0 = prim; }
		void setP1(Primitive* prim) { p1 = prim; }
		void setS0(size_t surfID) { s0 = surfID; }
		void setS1(size_t surfID) { s1 = surfID; }

        const Primitive* getP0(void) { return p0; }
        const Primitive* getP1(void) { return p1; }

        const size_t& getSurf0(void) { return s0; }
        const size_t& getSurf1(void) { return s1; }

		virtual void dynamicDraw(Adorn* adorn) = 0;
		virtual void createJoint(void) { /* override to create joints */ }
	};

	class AutoJointSurfacePair : public ConstraintSurfacePair
	{
	public:
		AutoJointSurfacePair(const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id) : ConstraintSurfacePair(p0, face0Id, p1, face1Id) {}
		AutoJointSurfacePair() {}
		virtual ~AutoJointSurfacePair() {}

		virtual void dynamicDraw(Adorn* adorn) {}
	};

	class StudAutoJointSurfacePair : public AutoJointSurfacePair
	{
	public:
		StudAutoJointSurfacePair(const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id) : AutoJointSurfacePair(p0, face0Id, p1, face1Id) {}
		StudAutoJointSurfacePair() {}
		~StudAutoJointSurfacePair() {}

		/* override */ void dynamicDraw(Adorn* adorn);
	};

	class WeldAutoJointSurfacePair : public AutoJointSurfacePair
	{
	public:
		WeldAutoJointSurfacePair(const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id) : AutoJointSurfacePair(p0, face0Id, p1, face1Id) {}
		WeldAutoJointSurfacePair() {}
		~WeldAutoJointSurfacePair() {}

		/* override */ void dynamicDraw(Adorn* adorn);
	};

	class GlueAutoJointSurfacePair : public AutoJointSurfacePair
	{
	public:
		GlueAutoJointSurfacePair(const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id) : AutoJointSurfacePair(p0, face0Id, p1, face1Id) {}
		GlueAutoJointSurfacePair() {}
		~GlueAutoJointSurfacePair() {}

		/* override */ void dynamicDraw(Adorn* adorn);
	};

	class HingeAutoJointSurfacePair : public AutoJointSurfacePair
	{
	public:
		HingeAutoJointSurfacePair(const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id) : AutoJointSurfacePair(p0, face0Id, p1, face1Id) {}
		HingeAutoJointSurfacePair() {}
		~HingeAutoJointSurfacePair() {}

		/* override */ void dynamicDraw(Adorn* adorn);
	};

	class DisallowedJointSurfacePair : public ConstraintSurfacePair
	{
	public:
		DisallowedJointSurfacePair(const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id) : ConstraintSurfacePair(p0, face0Id, p1, face1Id) {}
		DisallowedJointSurfacePair() {}
		virtual ~DisallowedJointSurfacePair() {}

		/* override */ void dynamicDraw(Adorn* adorn);
	};

	class FeatureSnapJointSurfacePair : public ConstraintSurfacePair
	{
	public:
		FeatureSnapJointSurfacePair(const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id) : ConstraintSurfacePair(p0, face0Id, p1, face1Id) {}
		FeatureSnapJointSurfacePair() {}
		virtual ~FeatureSnapJointSurfacePair() {}

		/* override */ void dynamicDraw(Adorn* adorn) {}
		/* override */ void createJoint(void) {}
	};

	class ManualJointSurfacePair : public ConstraintSurfacePair
	{
	public:
		ManualJointSurfacePair(const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id) : ConstraintSurfacePair(p0, face0Id, p1, face1Id) {}
		ManualJointSurfacePair() {}
		virtual ~ManualJointSurfacePair() {}

		/* override */ void dynamicDraw(Adorn* adorn);
		/* override */ void createJoint(void);
	};

	class TerrainManualJointSurfacePair : public ConstraintSurfacePair
	{
    private:
        Vector3int16 cellIndex;

	public:
		TerrainManualJointSurfacePair(const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id) : ConstraintSurfacePair(p0, face0Id, p1, face1Id) {}
		TerrainManualJointSurfacePair() {}
		virtual ~TerrainManualJointSurfacePair() {}

		/* override */ void dynamicDraw(Adorn* adorn);
		/* override */ void createJoint(void);

        void setCellIndex(Vector3int16& value) { cellIndex = value; }
	};

	class DisallowedTerrainJointSurfacePair : public ConstraintSurfacePair
	{
	private:
        Vector3int16 cellIndex;

	public:
		DisallowedTerrainJointSurfacePair(const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id) : ConstraintSurfacePair(p0, face0Id, p1, face1Id) {}
		DisallowedTerrainJointSurfacePair() {}
		virtual ~DisallowedTerrainJointSurfacePair() {}

		/* override */ void dynamicDraw(Adorn* adorn);

        void setCellIndex(Vector3int16& value) { cellIndex = value; }
	};

	class SmoothTerrainManualJointSurfacePair : public ConstraintSurfacePair
	{
	public:
		SmoothTerrainManualJointSurfacePair(const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id) : ConstraintSurfacePair(p0, face0Id, p1, face1Id) {}
		SmoothTerrainManualJointSurfacePair() {}
		virtual ~SmoothTerrainManualJointSurfacePair() {}

		/* override */ void dynamicDraw(Adorn* adorn);
		/* override */ void createJoint(void);
	};

	class ManualJointHelper : public IAdornable
	{
	private:
		bool displayPotentialJoints;
		bool isAdornable;
		bool autoJointsDetected;

		Workspace* workspace;
		std::vector<Primitive*> selectedPrimitives;
		std::vector<ConstraintSurfacePair*> jointSurfacePairs;
        Primitive* targetPrimitive;

		void onPartAncestryChanged(shared_ptr<Instance> instance, shared_ptr<Instance> newParent);
        void createTerrainJointSurfacePair(Primitive& p0, Primitive& p1, Vector3int16& cellIndex);	
        void createSmoothTerrainJointSurfacePair(Primitive& p0, Primitive& p1);
	public:

		ManualJointHelper(Workspace* ws);
		ManualJointHelper();
		~ManualJointHelper();

		void findPermissibleJointSurfacePairs(void);
		unsigned int getNumJointSurfacePairs(void) { return jointSurfacePairs.size(); }
		bool autoJointsWereDetected(void) { return autoJointsDetected; }
		void createJoints();
		void createJointsIfEnabledFromGui();
		void setSelectedPrimitives(const std::vector<Instance*>& selectedInstances);
		void setSelectedPrimitives(const Instances& selectedInstances);
        void setPVInstanceToJoin(PVInstance& pVInstToJoin);
        void setPVInstanceTarget(PVInstance& pVInstToJoin);
		void createJointSurfacePair(Primitive& p0, size_t& face0Id, Primitive& p1, size_t& face1Id);

        void setWorkspace(Workspace* ws);
        void clearAndDeleteJointSurfacePairs(void);
        void clearSelectedPrimitives(void) { selectedPrimitives.clear(); }
		bool surfaceIsNoJoin(const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id);	

		// IAdornable
        void setDisplayPotentialJoints(bool value) { displayPotentialJoints = value; }
		bool shouldRender3dAdorn() const { return true; }
		void render3dAdorn(Adorn* adorn);
	};

} // namespace

