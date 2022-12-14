include_directories(../App.BulletPhysics)

include_project_files(App.BulletPhysics "*")

list(APPEND HEADERS btBulletCollisionCommon.h)
list(APPEND HEADERS btBulletDynamicsCommon.h)
list(APPEND HEADERS Bullet-C-Api.h)
list(APPEND HEADERS BulletCollision/BroadphaseCollision/btAxisSweep3.h)
list(APPEND HEADERS BulletCollision/BroadphaseCollision/btBroadphaseInterface.h)
list(APPEND HEADERS BulletCollision/BroadphaseCollision/btBroadphaseProxy.h)
list(APPEND HEADERS BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h)
list(APPEND HEADERS BulletCollision/BroadphaseCollision/btDbvt.h)
list(APPEND HEADERS BulletCollision/BroadphaseCollision/btDbvtBroadphase.h)
list(APPEND HEADERS BulletCollision/BroadphaseCollision/btDispatcher.h)
list(APPEND HEADERS BulletCollision/BroadphaseCollision/btMultiSapBroadphase.h)
list(APPEND HEADERS BulletCollision/BroadphaseCollision/btOverlappingPairCache.h)
list(APPEND HEADERS BulletCollision/BroadphaseCollision/btOverlappingPairCallback.h)
list(APPEND HEADERS BulletCollision/BroadphaseCollision/btQuantizedBvh.h)
list(APPEND HEADERS BulletCollision/BroadphaseCollision/btSimpleBroadphase.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btActivatingCollisionAlgorithm.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btBox2dBox2dCollisionAlgorithm.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btBoxBoxCollisionAlgorithm.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btBoxBoxDetector.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btCollisionConfiguration.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btCollisionCreateFunc.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btCollisionDispatcher.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btCollisionObject.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btCollisionObjectWrapper.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btCollisionWorld.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btCompoundCollisionAlgorithm.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btCompoundCompoundCollisionAlgorithm.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btConvex2dConvex2dAlgorithm.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btConvexConcaveCollisionAlgorithm.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btConvexConvexAlgorithm.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btConvexPlaneCollisionAlgorithm.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btEmptyCollisionAlgorithm.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btGhostObject.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btHashedSimplePairCache.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btInternalEdgeUtility.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btManifoldResult.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btSimulationIslandManager.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btSphereBoxCollisionAlgorithm.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/btUnionFind.h)
list(APPEND HEADERS BulletCollision/CollisionDispatch/SphereTriangleDetector.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btBox2dShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btBoxShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btCapsuleShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btCollisionMargin.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btCollisionShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btCompoundShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btConcaveShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btConeShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btConvex2dShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btConvexHullShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btConvexInternalShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btConvexPointCloudShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btConvexPolyhedron.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btConvexShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btConvexTriangleMeshShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btCylinderShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btEmptyShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btMaterial.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btMinkowskiSumShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btMultimaterialTriangleMeshShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btMultiSphereShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btOptimizedBvh.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btPolyhedralConvexShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btShapeHull.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btSphereShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btStaticPlaneShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btStridingMeshInterface.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btTetrahedronShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btTriangleBuffer.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btTriangleCallback.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btTriangleIndexVertexArray.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btTriangleIndexVertexMaterialArray.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btTriangleInfoMap.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btTriangleMesh.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btTriangleMeshShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btTriangleShape.h)
list(APPEND HEADERS BulletCollision/CollisionShapes/btUniformScalingShape.h)
list(APPEND HEADERS BulletCollision/Gimpact/btBoxCollision.h)
list(APPEND HEADERS BulletCollision/Gimpact/btClipPolygon.h)
list(APPEND HEADERS BulletCollision/Gimpact/btCompoundFromGimpact.h)
list(APPEND HEADERS BulletCollision/Gimpact/btContactProcessing.h)
list(APPEND HEADERS BulletCollision/Gimpact/btGenericPoolAllocator.h)
list(APPEND HEADERS BulletCollision/Gimpact/btGeometryOperations.h)
list(APPEND HEADERS BulletCollision/Gimpact/btGImpactBvh.h)
list(APPEND HEADERS BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h)
list(APPEND HEADERS BulletCollision/Gimpact/btGImpactMassUtil.h)
list(APPEND HEADERS BulletCollision/Gimpact/btGImpactQuantizedBvh.h)
list(APPEND HEADERS BulletCollision/Gimpact/btGImpactShape.h)
list(APPEND HEADERS BulletCollision/Gimpact/btQuantization.h)
list(APPEND HEADERS BulletCollision/Gimpact/btTriangleShapeEx.h)
list(APPEND HEADERS BulletCollision/Gimpact/gim_array.h)
list(APPEND HEADERS BulletCollision/Gimpact/gim_basic_geometry_operations.h)
list(APPEND HEADERS BulletCollision/Gimpact/gim_bitset.h)
list(APPEND HEADERS BulletCollision/Gimpact/gim_box_collision.h)
list(APPEND HEADERS BulletCollision/Gimpact/gim_box_set.h)
list(APPEND HEADERS BulletCollision/Gimpact/gim_clip_polygon.h)
list(APPEND HEADERS BulletCollision/Gimpact/gim_contact.h)
list(APPEND HEADERS BulletCollision/Gimpact/gim_geom_types.h)
list(APPEND HEADERS BulletCollision/Gimpact/gim_geometry.h)
list(APPEND HEADERS BulletCollision/Gimpact/gim_hash_table.h)
list(APPEND HEADERS BulletCollision/Gimpact/gim_linear_math.h)
list(APPEND HEADERS BulletCollision/Gimpact/gim_math.h)
list(APPEND HEADERS BulletCollision/Gimpact/gim_memory.h)
list(APPEND HEADERS BulletCollision/Gimpact/gim_radixsort.h)
list(APPEND HEADERS BulletCollision/Gimpact/gim_tri_collision.h)
list(APPEND HEADERS BulletCollision/NarrowPhaseCollision/btContinuousConvexCollision.h)
list(APPEND HEADERS BulletCollision/NarrowPhaseCollision/btConvexCast.h)
list(APPEND HEADERS BulletCollision/NarrowPhaseCollision/btConvexPenetrationDepthSolver.h)
list(APPEND HEADERS BulletCollision/NarrowPhaseCollision/btDiscreteCollisionDetectorInterface.h)
list(APPEND HEADERS BulletCollision/NarrowPhaseCollision/btGjkConvexCast.h)
list(APPEND HEADERS BulletCollision/NarrowPhaseCollision/btGjkEpa2.h)
list(APPEND HEADERS BulletCollision/NarrowPhaseCollision/btGjkEpaPenetrationDepthSolver.h)
list(APPEND HEADERS BulletCollision/NarrowPhaseCollision/btGjkPairDetector.h)
list(APPEND HEADERS BulletCollision/NarrowPhaseCollision/btManifoldPoint.h)
list(APPEND HEADERS BulletCollision/NarrowPhaseCollision/btMinkowskiPenetrationDepthSolver.h)
list(APPEND HEADERS BulletCollision/NarrowPhaseCollision/btPersistentManifold.h)
list(APPEND HEADERS BulletCollision/NarrowPhaseCollision/btPointCollector.h)
list(APPEND HEADERS BulletCollision/NarrowPhaseCollision/btPolyhedralContactClipping.h)
list(APPEND HEADERS BulletCollision/NarrowPhaseCollision/btRaycastCallback.h)
list(APPEND HEADERS BulletCollision/NarrowPhaseCollision/btSimplexSolverInterface.h)
list(APPEND HEADERS BulletCollision/NarrowPhaseCollision/btSubSimplexConvexCast.h)
list(APPEND HEADERS BulletCollision/NarrowPhaseCollision/btVoronoiSimplexSolver.h)
list(APPEND HEADERS BulletDynamics/Character/btCharacterControllerInterface.h)
list(APPEND HEADERS BulletDynamics/Character/btKinematicCharacterController.h)
list(APPEND HEADERS BulletDynamics/ConstraintSolver/btConeTwistConstraint.h)
list(APPEND HEADERS BulletDynamics/ConstraintSolver/btConstraintSolver.h)
list(APPEND HEADERS BulletDynamics/ConstraintSolver/btContactConstraint.h)
list(APPEND HEADERS BulletDynamics/ConstraintSolver/btContactSolverInfo.h)
list(APPEND HEADERS BulletDynamics/ConstraintSolver/btFixedConstraint.h)
list(APPEND HEADERS BulletDynamics/ConstraintSolver/btGearConstraint.h)
list(APPEND HEADERS BulletDynamics/ConstraintSolver/btGeneric6DofConstraint.h)
list(APPEND HEADERS BulletDynamics/ConstraintSolver/btGeneric6DofSpringConstraint.h)
list(APPEND HEADERS BulletDynamics/ConstraintSolver/btHinge2Constraint.h)
list(APPEND HEADERS BulletDynamics/ConstraintSolver/btHingeConstraint.h)
list(APPEND HEADERS BulletDynamics/ConstraintSolver/btJacobianEntry.h)
list(APPEND HEADERS BulletDynamics/ConstraintSolver/btPoint2PointConstraint.h)
list(APPEND HEADERS BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h)
list(APPEND HEADERS BulletDynamics/ConstraintSolver/btSliderConstraint.h)
list(APPEND HEADERS BulletDynamics/ConstraintSolver/btSolve2LinearConstraint.h)
list(APPEND HEADERS BulletDynamics/ConstraintSolver/btSolverBody.h)
list(APPEND HEADERS BulletDynamics/ConstraintSolver/btSolverConstraint.h)
list(APPEND HEADERS BulletDynamics/ConstraintSolver/btTypedConstraint.h)
list(APPEND HEADERS BulletDynamics/ConstraintSolver/btUniversalConstraint.h)
list(APPEND HEADERS BulletDynamics/Dynamics/btActionInterface.h)
list(APPEND HEADERS BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h)
list(APPEND HEADERS BulletDynamics/Dynamics/btDynamicsWorld.h)
list(APPEND HEADERS BulletDynamics/Dynamics/btRigidBody.h)
list(APPEND HEADERS BulletDynamics/Dynamics/btSimpleDynamicsWorld.h)
list(APPEND HEADERS BulletDynamics/Featherstone/btMultiBody.h)
list(APPEND HEADERS BulletDynamics/Featherstone/btMultiBodyConstraint.h)
list(APPEND HEADERS BulletDynamics/Featherstone/btMultiBodyConstraintSolver.h)
list(APPEND HEADERS BulletDynamics/Featherstone/btMultiBodyDynamicsWorld.h)
list(APPEND HEADERS BulletDynamics/Featherstone/btMultiBodyJointLimitConstraint.h)
list(APPEND HEADERS BulletDynamics/Featherstone/btMultiBodyJointMotor.h)
list(APPEND HEADERS BulletDynamics/Featherstone/btMultiBodyLink.h)
list(APPEND HEADERS BulletDynamics/Featherstone/btMultiBodyLinkCollider.h)
list(APPEND HEADERS BulletDynamics/Featherstone/btMultiBodyPoint2Point.h)
list(APPEND HEADERS BulletDynamics/Featherstone/btMultiBodySolverConstraint.h)
list(APPEND HEADERS BulletDynamics/MLCPSolvers/btDantzigLCP.h)
list(APPEND HEADERS BulletDynamics/MLCPSolvers/btDantzigSolver.h)
list(APPEND HEADERS BulletDynamics/MLCPSolvers/btMLCPSolver.h)
list(APPEND HEADERS BulletDynamics/MLCPSolvers/btMLCPSolverInterface.h)
list(APPEND HEADERS BulletDynamics/MLCPSolvers/btPATHSolver.h)
list(APPEND HEADERS BulletDynamics/MLCPSolvers/btSolveProjectedGaussSeidel.h)
list(APPEND HEADERS BulletDynamics/Vehicle/btRaycastVehicle.h)
list(APPEND HEADERS BulletDynamics/Vehicle/btVehicleRaycaster.h)
list(APPEND HEADERS BulletDynamics/Vehicle/btWheelInfo.h)
list(APPEND HEADERS Extras/ConvexDecomposition/bestfit.h)
list(APPEND HEADERS Extras/ConvexDecomposition/bestfitobb.h)
list(APPEND HEADERS Extras/ConvexDecomposition/cd_hull.h)
list(APPEND HEADERS Extras/ConvexDecomposition/cd_vector.h)
list(APPEND HEADERS Extras/ConvexDecomposition/cd_wavefront.h)
list(APPEND HEADERS Extras/ConvexDecomposition/concavity.h)
list(APPEND HEADERS Extras/ConvexDecomposition/ConvexBuilder.h)
list(APPEND HEADERS Extras/ConvexDecomposition/ConvexDecomposition.h)
list(APPEND HEADERS Extras/ConvexDecomposition/fitsphere.h)
list(APPEND HEADERS Extras/ConvexDecomposition/float_math.h)
list(APPEND HEADERS Extras/ConvexDecomposition/meshvolume.h)
list(APPEND HEADERS Extras/ConvexDecomposition/planetri.h)
list(APPEND HEADERS Extras/ConvexDecomposition/raytri.h)
list(APPEND HEADERS Extras/ConvexDecomposition/splitplane.h)
list(APPEND HEADERS Extras/ConvexDecomposition/vlookup.h)
list(APPEND HEADERS Extras/GIMPACTUtils/btGImpactConvexDecompositionShape.h)
list(APPEND HEADERS LinearMath/btAabbUtil2.h)
list(APPEND HEADERS LinearMath/btAlignedAllocator.h)
list(APPEND HEADERS LinearMath/btAlignedObjectArray.h)
list(APPEND HEADERS LinearMath/btConvexHull.h)
list(APPEND HEADERS LinearMath/btConvexHullComputer.h)
list(APPEND HEADERS LinearMath/btDefaultMotionState.h)
list(APPEND HEADERS LinearMath/btGeometryUtil.h)
list(APPEND HEADERS LinearMath/btGrahamScan2dConvexHull.h)
list(APPEND HEADERS LinearMath/btHashMap.h)
list(APPEND HEADERS LinearMath/btIDebugDraw.h)
list(APPEND HEADERS LinearMath/btList.h)
list(APPEND HEADERS LinearMath/btMatrix3x3.h)
list(APPEND HEADERS LinearMath/btMatrixX.h)
list(APPEND HEADERS LinearMath/btMinMax.h)
list(APPEND HEADERS LinearMath/btMotionState.h)
list(APPEND HEADERS LinearMath/btPolarDecomposition.h)
list(APPEND HEADERS LinearMath/btPoolAllocator.h)
list(APPEND HEADERS LinearMath/btQuadWord.h)
list(APPEND HEADERS LinearMath/btQuaternion.h)
list(APPEND HEADERS LinearMath/btQuickprof.h)
list(APPEND HEADERS LinearMath/btRandom.h)
list(APPEND HEADERS LinearMath/btScalar.h)
list(APPEND HEADERS LinearMath/btSerializer.h)
list(APPEND HEADERS LinearMath/btStackAlloc.h)
list(APPEND HEADERS LinearMath/btTransform.h)
list(APPEND HEADERS LinearMath/btTransformUtil.h)
list(APPEND HEADERS LinearMath/btVector3.h)
list(APPEND HEADERS MiniCL/cl.h)
list(APPEND HEADERS MiniCL/cl_gl.h)
list(APPEND HEADERS MiniCL/cl_MiniCL_Defs.h)
list(APPEND HEADERS MiniCL/cl_platform.h)
list(APPEND HEADERS MiniCL/MiniCLTask/MiniCLTask.h)
list(APPEND HEADERS MiniCL/MiniCLTaskScheduler.h)
list(APPEND HEADERS stdafx.h)
list(APPEND HEADERS vectormath/neon/boolInVec.h)
list(APPEND HEADERS vectormath/neon/floatInVec.h)
list(APPEND HEADERS vectormath/neon/mat_aos.h)
list(APPEND HEADERS vectormath/neon/quat_aos.h)
list(APPEND HEADERS vectormath/neon/vec_aos.h)
list(APPEND HEADERS vectormath/neon/vectormath_aos.h)
list(APPEND HEADERS vectormath/scalar/boolInVec.h)
list(APPEND HEADERS vectormath/scalar/floatInVec.h)
list(APPEND HEADERS vectormath/scalar/mat_aos.h)
list(APPEND HEADERS vectormath/scalar/quat_aos.h)
list(APPEND HEADERS vectormath/scalar/vec_aos.h)
list(APPEND HEADERS vectormath/scalar/vectormath_aos.h)
list(APPEND HEADERS vectormath/sse/boolInVec.h)
list(APPEND HEADERS vectormath/sse/floatInVec.h)
list(APPEND HEADERS vectormath/sse/mat_aos.h)
list(APPEND HEADERS vectormath/sse/quat_aos.h)
list(APPEND HEADERS vectormath/sse/vec_aos.h)
list(APPEND HEADERS vectormath/sse/vecidx_aos.h)
list(APPEND HEADERS vectormath/sse/vectormath_aos.h)
list(APPEND HEADERS vectormath/vmInclude.h)

list(APPEND SOURCES BulletCollision/BroadphaseCollision/btAxisSweep3.cpp)
list(APPEND SOURCES BulletCollision/BroadphaseCollision/btBroadphaseProxy.cpp)
list(APPEND SOURCES BulletCollision/BroadphaseCollision/btCollisionAlgorithm.cpp)
list(APPEND SOURCES BulletCollision/BroadphaseCollision/btDbvt.cpp)
list(APPEND SOURCES BulletCollision/BroadphaseCollision/btDbvtBroadphase.cpp)
list(APPEND SOURCES BulletCollision/BroadphaseCollision/btDispatcher.cpp)
list(APPEND SOURCES BulletCollision/BroadphaseCollision/btMultiSapBroadphase.cpp)
list(APPEND SOURCES BulletCollision/BroadphaseCollision/btOverlappingPairCache.cpp)
list(APPEND SOURCES BulletCollision/BroadphaseCollision/btQuantizedBvh.cpp)
list(APPEND SOURCES BulletCollision/BroadphaseCollision/btSimpleBroadphase.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btActivatingCollisionAlgorithm.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btBox2dBox2dCollisionAlgorithm.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btBoxBoxCollisionAlgorithm.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btBoxBoxDetector.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btCollisionDispatcher.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btCollisionObject.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btCollisionWorld.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btCompoundCollisionAlgorithm.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btCompoundCompoundCollisionAlgorithm.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btConvex2dConvex2dAlgorithm.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btConvexConcaveCollisionAlgorithm.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btConvexConvexAlgorithm.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btConvexPlaneCollisionAlgorithm.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btEmptyCollisionAlgorithm.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btGhostObject.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btHashedSimplePairCache.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btInternalEdgeUtility.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btManifoldResult.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btSimulationIslandManager.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btSphereBoxCollisionAlgorithm.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/btUnionFind.cpp)
list(APPEND SOURCES BulletCollision/CollisionDispatch/SphereTriangleDetector.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btBox2dShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btBoxShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btBvhTriangleMeshShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btCapsuleShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btCollisionShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btCompoundShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btConcaveShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btConeShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btConvex2dShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btConvexHullShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btConvexInternalShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btConvexPointCloudShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btConvexPolyhedron.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btConvexShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btConvexTriangleMeshShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btCylinderShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btEmptyShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btHeightfieldTerrainShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btMinkowskiSumShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btMultimaterialTriangleMeshShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btMultiSphereShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btOptimizedBvh.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btPolyhedralConvexShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btShapeHull.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btSphereShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btStaticPlaneShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btStridingMeshInterface.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btTetrahedronShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btTriangleBuffer.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btTriangleCallback.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btTriangleIndexVertexArray.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btTriangleIndexVertexMaterialArray.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btTriangleMesh.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btTriangleMeshShape.cpp)
list(APPEND SOURCES BulletCollision/CollisionShapes/btUniformScalingShape.cpp)
list(APPEND SOURCES BulletCollision/Gimpact/btContactProcessing.cpp)
list(APPEND SOURCES BulletCollision/Gimpact/btGenericPoolAllocator.cpp)
list(APPEND SOURCES BulletCollision/Gimpact/btGImpactBvh.cpp)
list(APPEND SOURCES BulletCollision/Gimpact/btGImpactCollisionAlgorithm.cpp)
list(APPEND SOURCES BulletCollision/Gimpact/btGImpactQuantizedBvh.cpp)
list(APPEND SOURCES BulletCollision/Gimpact/btGImpactShape.cpp)
list(APPEND SOURCES BulletCollision/Gimpact/btTriangleShapeEx.cpp)
list(APPEND SOURCES BulletCollision/Gimpact/gim_box_set.cpp)
list(APPEND SOURCES BulletCollision/Gimpact/gim_contact.cpp)
list(APPEND SOURCES BulletCollision/Gimpact/gim_memory.cpp)
list(APPEND SOURCES BulletCollision/Gimpact/gim_tri_collision.cpp)
list(APPEND SOURCES BulletCollision/NarrowPhaseCollision/btContinuousConvexCollision.cpp)
list(APPEND SOURCES BulletCollision/NarrowPhaseCollision/btConvexCast.cpp)
list(APPEND SOURCES BulletCollision/NarrowPhaseCollision/btGjkConvexCast.cpp)
list(APPEND SOURCES BulletCollision/NarrowPhaseCollision/btGjkEpa2.cpp)
list(APPEND SOURCES BulletCollision/NarrowPhaseCollision/btGjkEpaPenetrationDepthSolver.cpp)
list(APPEND SOURCES BulletCollision/NarrowPhaseCollision/btGjkPairDetector.cpp)
list(APPEND SOURCES BulletCollision/NarrowPhaseCollision/btMinkowskiPenetrationDepthSolver.cpp)
list(APPEND SOURCES BulletCollision/NarrowPhaseCollision/btPersistentManifold.cpp)
list(APPEND SOURCES BulletCollision/NarrowPhaseCollision/btPolyhedralContactClipping.cpp)
list(APPEND SOURCES BulletCollision/NarrowPhaseCollision/btRaycastCallback.cpp)
list(APPEND SOURCES BulletCollision/NarrowPhaseCollision/btSubSimplexConvexCast.cpp)
list(APPEND SOURCES BulletCollision/NarrowPhaseCollision/btVoronoiSimplexSolver.cpp)
list(APPEND SOURCES BulletDynamics/Character/btKinematicCharacterController.cpp)
list(APPEND SOURCES BulletDynamics/ConstraintSolver/btConeTwistConstraint.cpp)
list(APPEND SOURCES BulletDynamics/ConstraintSolver/btContactConstraint.cpp)
list(APPEND SOURCES BulletDynamics/ConstraintSolver/btFixedConstraint.cpp)
list(APPEND SOURCES BulletDynamics/ConstraintSolver/btGearConstraint.cpp)
list(APPEND SOURCES BulletDynamics/ConstraintSolver/btGeneric6DofConstraint.cpp)
list(APPEND SOURCES BulletDynamics/ConstraintSolver/btGeneric6DofSpringConstraint.cpp)
list(APPEND SOURCES BulletDynamics/ConstraintSolver/btHinge2Constraint.cpp)
list(APPEND SOURCES BulletDynamics/ConstraintSolver/btHingeConstraint.cpp)
list(APPEND SOURCES BulletDynamics/ConstraintSolver/btPoint2PointConstraint.cpp)
list(APPEND SOURCES BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.cpp)
list(APPEND SOURCES BulletDynamics/ConstraintSolver/btSliderConstraint.cpp)
list(APPEND SOURCES BulletDynamics/ConstraintSolver/btSolve2LinearConstraint.cpp)
list(APPEND SOURCES BulletDynamics/ConstraintSolver/btTypedConstraint.cpp)
list(APPEND SOURCES BulletDynamics/ConstraintSolver/btUniversalConstraint.cpp)
list(APPEND SOURCES BulletDynamics/Dynamics/btDiscreteDynamicsWorld.cpp)
list(APPEND SOURCES BulletDynamics/Dynamics/btRigidBody.cpp)
list(APPEND SOURCES BulletDynamics/Dynamics/btSimpleDynamicsWorld.cpp)
list(APPEND SOURCES BulletDynamics/Dynamics/Bullet-C-API.cpp)
list(APPEND SOURCES BulletDynamics/Featherstone/btMultiBody.cpp)
list(APPEND SOURCES BulletDynamics/Featherstone/btMultiBodyConstraint.cpp)
list(APPEND SOURCES BulletDynamics/Featherstone/btMultiBodyConstraintSolver.cpp)
list(APPEND SOURCES BulletDynamics/Featherstone/btMultiBodyDynamicsWorld.cpp)
list(APPEND SOURCES BulletDynamics/Featherstone/btMultiBodyJointLimitConstraint.cpp)
list(APPEND SOURCES BulletDynamics/Featherstone/btMultiBodyJointMotor.cpp)
list(APPEND SOURCES BulletDynamics/Featherstone/btMultiBodyPoint2Point.cpp)
list(APPEND SOURCES BulletDynamics/MLCPSolvers/btDantzigLCP.cpp)
list(APPEND SOURCES BulletDynamics/MLCPSolvers/btMLCPSolver.cpp)
list(APPEND SOURCES BulletDynamics/Vehicle/btRaycastVehicle.cpp)
list(APPEND SOURCES BulletDynamics/Vehicle/btWheelInfo.cpp)
list(APPEND SOURCES Extras/ConvexDecomposition/bestfit.cpp)
list(APPEND SOURCES Extras/ConvexDecomposition/bestfitobb.cpp)
list(APPEND SOURCES Extras/ConvexDecomposition/cd_hull.cpp)
list(APPEND SOURCES Extras/ConvexDecomposition/cd_wavefront.cpp)
list(APPEND SOURCES Extras/ConvexDecomposition/concavity.cpp)
list(APPEND SOURCES Extras/ConvexDecomposition/ConvexBuilder.cpp)
list(APPEND SOURCES Extras/ConvexDecomposition/ConvexDecomposition.cpp)
list(APPEND SOURCES Extras/ConvexDecomposition/fitsphere.cpp)
list(APPEND SOURCES Extras/ConvexDecomposition/float_math.cpp)
list(APPEND SOURCES Extras/ConvexDecomposition/meshvolume.cpp)
list(APPEND SOURCES Extras/ConvexDecomposition/planetri.cpp)
list(APPEND SOURCES Extras/ConvexDecomposition/raytri.cpp)
list(APPEND SOURCES Extras/ConvexDecomposition/splitplane.cpp)
list(APPEND SOURCES Extras/ConvexDecomposition/vlookup.cpp)
list(APPEND SOURCES Extras/GIMPACTUtils/btGImpactConvexDecompositionShape.cpp)
list(APPEND SOURCES LinearMath/btAlignedAllocator.cpp)
list(APPEND SOURCES LinearMath/btConvexHull.cpp)
list(APPEND SOURCES LinearMath/btConvexHullComputer.cpp)
list(APPEND SOURCES LinearMath/btGeometryUtil.cpp)
list(APPEND SOURCES LinearMath/btPolarDecomposition.cpp)
list(APPEND SOURCES LinearMath/btQuickprof.cpp)
list(APPEND SOURCES LinearMath/btSerializer.cpp)
list(APPEND SOURCES LinearMath/btVector3.cpp)



add_library(App.BulletPhysics OBJECT ${SOURCES} ${HEADERS})
