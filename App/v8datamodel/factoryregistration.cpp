/* Copyright 2003-2010 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/factoryregistration.h"

#include "FastLog.h"

#include "V8DataModel/Adornment.h"
#include "V8DataModel/BillboardGui.h"
#include "v8datamodel/SurfaceGui.h"
#include "V8DataModel/Accoutrement.h"
#include "V8DataModel/Backpack.h"
#include "V8DataModel/BadgeService.h"
#include "V8DataModel/BasicPartInstance.h"
#include "V8DataModel/BevelMesh.h"
#include "V8DataModel/BlockMesh.h"
#include "v8datamodel/CacheableContentProvider.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/ChangeHistory.h"
#include "V8DataModel/CharacterAppearance.h"
#include "V8DataModel/CharacterMesh.h"
#include "V8DataModel/ChatService.h"
#include "V8DataModel/ClickDetector.h"
#include "V8DataModel/ContentProvider.h"
#include "V8DataModel/Configuration.h"
#include "V8DataModel/CollectionService.h"
#include "V8DataModel/CSGDictionaryService.h"
#include "V8DataModel/CustomEvent.h"
#include "V8DataModel/CustomEventReceiver.h"
#include "V8DataModel/CylinderMesh.h"
#include "V8DataModel/VirtualUser.h"
#include "V8DataModel/LogService.h"
#include "V8DataModel/DataModelMesh.h"
#include "V8DataModel/DebrisService.h"
#include "V8DataModel/Decal.h"
#include "V8DataModel/DialogRoot.h"
#include "V8DataModel/DialogChoice.h"
#include "V8DataModel/DebugSettings.h"
#include "V8DataModel/PhysicsSettings.h"
#include "V8DataModel/ExtrudedPartInstance.h"
#include "V8DataModel/FriendService.h"
#include "V8DataModel/Folder.h"
#include "V8DataModel/RenderHooksService.h"
#include "V8DataModel/Test.h"
#include "V8DataModel/CookiesEngineService.h"
#include "V8DataModel/TeleportService.h"
#include "V8DataModel/PersonalServerService.h"
#include "V8DataModel/ScriptService.h"
#include "V8DataModel/UserInputService.h"
#include "v8datamodel/AssetService.h"
#include "v8datamodel/HttpService.h"
#include "v8datamodel/HttpRbxApiService.h"
#include "v8datamodel/DataStoreService.h"
#include "v8datamodel/TerrainRegion.h"
#include "v8datamodel/PathfindingService.h"
#include "v8datamodel/StarterPlayerService.h"
#include "v8datamodel/HandleAdornment.h"
#include "util/CellID.h"

#ifdef _PRISM_PYRAMID_
#include "V8DataModel/PrismInstance.h"
#include "V8DataModel/PyramidInstance.h"
#include "V8DataModel/ParallelRampInstance.h"
#include "V8DataModel/RightAngleRampInstance.h"
#include "V8DataModel/CornerWedgeInstance.h"
#endif

#include "V8DataModel/Explosion.h"
#include "V8DataModel/FaceInstance.h"
#include "V8DataModel/Feature.h"
#include "V8DataModel/FileMesh.h"
#include "V8DataModel/Fire.h"
#include "V8DataModel/Flag.h"
#include "V8DataModel/FlagStand.h"
#include "V8DataModel/FlyweightService.h"
#include "V8DataModel/ForceField.h"
#include "V8DataModel/GameSettings.h"
#include "V8DataModel/GameBasicSettings.h"
#include "Script/LuaSettings.h"
#include "Script/DebuggerManager.h"
#include "Script/ModuleScript.h"
#include "Script/LuaSourceContainer.h"
#include "V8DataModel/GeometryService.h"
#include "V8DataModel/GlobalSettings.h"
#include "V8DataModel/Gyro.h"
#include "V8DataModel/Handles.h"
#include "V8DataModel/HandlesBase.h"
#include "V8DataModel/ArcHandles.h"
#include "V8DataModel/Hopper.h"
#include "V8DataModel/JointInstance.h"
#include "V8DataModel/JointsService.h"
#include "V8DataModel/Lighting.h"
#include "V8DataModel/MeshContentProvider.h"
#include "V8DataModel/Message.h"
#include "V8DataModel/Mouse.h"
#include "V8DataModel/NonReplicatedCSGDictionaryService.h"
#include "V8DataModel/ParametricPartInstance.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/PhysicsService.h"
#include "V8DataModel/Platform.h"
#include "V8DataModel/PlayerGui.h"
#include "V8DataModel/PlayerScripts.h"
#include "V8DataModel/PlayerMouse.h"
#include "V8DataModel/PluginManager.h"
#include "V8DataModel/PluginMouse.h"
#include "V8DataModel/Seat.h"
#include "V8DataModel/SelectionBox.h"
#include "V8DataModel/SelectionSphere.h"
#include "V8dataModel/Sky.h"
#include "V8dataModel/SkateboardPlatform.h"
#include "V8DataModel/SkateboardController.h"
#include "V8DataModel/Smoke.h"
#include "v8datamodel/CustomParticleEmitter.h"
#include "V8DataModel/SolidModelContentProvider.h"
#include "V8DataModel/Sparkles.h"
#include "V8DataModel/SpawnLocation.h"
#include "V8Datamodel/SpecialMesh.h"
#include "V8DataModel/Stats.h"
#include "V8DataModel/SurfaceSelection.h"
#include "V8DataModel/Teams.h"
#include "V8DataModel/TextService.h"
#include "V8DataModel/TextureContentProvider.h"
#include "V8DataModel/TimerService.h"
#include "V8DataModel/Tool.h"
#include "V8DataModel/StudioTool.h"
#include "V8DataModel/TouchTransmitter.h"
#include "V8DataModel/usercontroller.h"
#include "V8DataModel/Value.h"
#include "V8DataModel/VehicleSeat.h"
#include "V8DataModel/Visit.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/LocalWorkspace.h"
#include "Humanoid/Humanoid.h"
#include "Humanoid/StatusInstance.h"
#include "Humanoid/HumanoidState.h"
#include "script/scriptcontext.h"
#include "Script/Script.h"
#include "Script/CoreScript.h"
#include "V8DataModel/MarketplaceService.h"
#include "V8DataModel/GuiService.h"
#include "V8DataModel/GuiBase.h"
#include "V8DataModel/GuiBase3d.h"
#include "V8DataModel/TweenService.h"
#include "V8DataModel/GuiObject.h"
#include "V8DataModel/ScreenGui.h"
#include "V8DataModel/Frame.h"
#include "V8DataModel/Scale9Frame.h"
#include "V8DataModel/ImageButton.h"
#include "V8DataModel/ImageLabel.h"
#include "V8DataModel/TextButton.h"
#include "V8DataModel/TextLabel.h"
#include "V8DataModel/TextBox.h"
#include "V8DataModel/SelectionLasso.h"
#include "V8DataModel/TextureTrail.h"
#include "V8DataModel/FloorWire.h"
#include "V8DataModel/Animation.h"
#include "V8DataModel/AnimationController.h"
#include "V8DataModel/AnimationTrack.h"
#include "V8DataModel/AnimationTrackState.h"
#include "V8DataModel/Animator.h"
#include "V8DataModel/KeyframeSequenceProvider.h"
#include "V8DataModel/KeyframeSequence.h"
#include "V8DataModel/Keyframe.h"
#include "V8DataModel/Pose.h"
#include "V8DataModel/MegaCluster.h"
#include "V8DataModel/Bindable.h"
#include "V8DataModel/Light.h"
#include "V8DataModel/Remote.h"
#include "V8DataModel/PartOperation.h"
#include "V8DataModel/PartOperationAsset.h"
#include "V8DataModel/Attachment.h"
#include "v8datamodel/TouchInputService.h"
#include "v8datamodel/HapticService.h"

#include "util/CellID.h"

#include "network/NetworkPacketCache.h"
#include "network/NetworkClusterPacketCache.h"
#include "network/ChatFilter.h"

#include "util/Sound.h"
#include "util/SoundService.h"
#include "util/UDim.h"
#include "util/Faces.h"
#include "util/Axes.h"
#include "Util/ScriptInformationProvider.h"
#include "util/Action.h"
#include "util/Region3.h"
#include "util/KeywordFilter.h"
#include "util/SystemAddress.h"
#include "util/LuaWebService.h"
#include "util/rbxrandom.h"
#include "util/RunStateOwner.h"
#include "util/PhysicalProperties.h"
#include "V8DataModel/InsertService.h"
#include "V8DataModel/SocialService.h"
#include "V8DataModel/GamePassService.h"
#include "V8DataModel/ContextActionService.h"
#include "V8DataModel/LoginService.h"
#include "Util/ContentFilter.h"
#include "Tool/LuaDragger.h"
#include "Tool/AdvLuaDragger.h"
#include "RbxG3D/RbxTime.h"
#include "v8datamodel/InputObject.h"
#include "v8datamodel/ReplicatedStorage.h"
#include "v8datamodel/RobloxReplicatedStorage.h"
#include "v8datamodel/ReplicatedFirst.h"
#include "v8datamodel/ServerScriptService.h"
#include "v8datamodel/ServerStorage.h"
#include "util/standardout.h"
#include "util/KeyCode.h"
#include "v8datamodel/PointsService.h"
#include "v8datamodel/ScrollingFrame.h"
#include "V8datamodel/AdService.h"
#include "V8datamodel/NotificationService.h"
#include "V8datamodel/GroupService.h"
#include "V8datamodel/GamepadService.h"

#include "v8datamodel/NumberSequence.h"
#include "v8datamodel/NumberRange.h"
#include "v8datamodel/ColorSequence.h"



using namespace RBX;

RBX_REGISTER_TYPE(void);
RBX_REGISTER_TYPE(bool);
RBX_REGISTER_TYPE(float);
RBX_REGISTER_TYPE(int);
RBX_REGISTER_TYPE(long);
RBX_REGISTER_TYPE(double);
RBX_REGISTER_TYPE(std::string);
RBX_REGISTER_TYPE(RBX::ProtectedString);
RBX_REGISTER_TYPE(const Reflection::PropertyDescriptor*);
RBX_REGISTER_TYPE(RBX::BrickColor);
RBX_REGISTER_TYPE(RBX::SystemAddress);
RBX_REGISTER_TYPE(RBX::MeshId);
RBX_REGISTER_TYPE(RBX::AnimationId);
RBX_REGISTER_TYPE(boost::shared_ptr<const Reflection::Tuple>);
RBX_REGISTER_TYPE(G3D::Vector3);
RBX_REGISTER_TYPE(G3D::Vector3int16);
RBX_REGISTER_TYPE(RBX::Region3);
RBX_REGISTER_TYPE(RBX::RbxRay);
RBX_REGISTER_TYPE(G3D::Rect2D);
RBX_REGISTER_TYPE(RBX::PhysicalProperties);
RBX_REGISTER_TYPE(RBX::Vector2);
RBX_REGISTER_TYPE(G3D::Vector2int16);
RBX_REGISTER_TYPE(G3D::Color3);
RBX_REGISTER_TYPE(G3D::CoordinateFrame);
RBX_REGISTER_TYPE(RBX::ContentId);
RBX_REGISTER_TYPE(RBX::TextureId);
RBX_REGISTER_TYPE(RBX::UDim);
RBX_REGISTER_TYPE(RBX::UDim2);
RBX_REGISTER_TYPE(RBX::Faces);
RBX_REGISTER_TYPE(RBX::Axes);
RBX_REGISTER_TYPE(boost::shared_ptr<const Instances>);
RBX_REGISTER_TYPE(boost::shared_ptr<class Reflection::DescribedBase>);
RBX_REGISTER_TYPE(boost::shared_ptr<class RBX::Instance>);
RBX_REGISTER_TYPE(Lua::WeakFunctionRef);
RBX_REGISTER_TYPE(shared_ptr<Lua::GenericFunction>);
RBX_REGISTER_TYPE(shared_ptr<Lua::GenericAsyncFunction>);
RBX_REGISTER_TYPE(boost::shared_ptr<const Reflection::ValueArray>);
RBX_REGISTER_TYPE(boost::shared_ptr<const Reflection::ValueMap>);
RBX_REGISTER_TYPE(boost::shared_ptr<const Reflection::ValueTable>);
RBX_REGISTER_TYPE(CellID);
RBX_REGISTER_TYPE(Soundscape::SoundId);
RBX_REGISTER_TYPE(RBX::NumberSequenceKeypoint);
RBX_REGISTER_TYPE(RBX::ColorSequenceKeypoint);
RBX_REGISTER_TYPE(RBX::NumberSequence);
RBX_REGISTER_TYPE(RBX::ColorSequence);
RBX_REGISTER_TYPE(RBX::NumberRange);
RBX_REGISTER_TYPE(RBX::Guid::Data);

RBX_REGISTER_CLASS(InputObject);
RBX_REGISTER_CLASS(TestService);
RBX_REGISTER_CLASS(FunctionalTest);
RBX_REGISTER_CLASS(Lighting);
RBX_REGISTER_CLASS(DebugSettings);
RBX_REGISTER_CLASS(PhysicsSettings);
RBX_REGISTER_CLASS(TaskSchedulerSettings);
RBX_REGISTER_CLASS(DataModel);
RBX_REGISTER_CLASS(PhysicsService);
RBX_REGISTER_CLASS(BadgeService);
RBX_REGISTER_CLASS(DialogRoot);
RBX_REGISTER_CLASS(DialogChoice);
RBX_REGISTER_CLASS(Tool);
RBX_REGISTER_CLASS(StudioTool);
RBX_REGISTER_CLASS(LuaDragger);
RBX_REGISTER_CLASS(AdvLuaDragger);
RBX_REGISTER_CLASS(Accoutrement);
RBX_REGISTER_CLASS(Backpack);
RBX_REGISTER_CLASS(BodyColors);
RBX_REGISTER_CLASS(ClickDetector);
RBX_REGISTER_CLASS(ControllerService);
RBX_REGISTER_CLASS(ChatService);
RBX_REGISTER_CLASS(TextService);
RBX_REGISTER_CLASS(VirtualUser);
RBX_REGISTER_CLASS(Explosion);
RBX_REGISTER_CLASS(Team);
RBX_REGISTER_CLASS(Instance);
RBX_REGISTER_CLASS(Flag);
RBX_REGISTER_CLASS(FlagStand);
RBX_REGISTER_CLASS(FlagStandService);
RBX_REGISTER_CLASS(ForceField);
RBX_REGISTER_CLASS(Fire);
RBX_REGISTER_CLASS(GameSettings);
RBX_REGISTER_CLASS(GameBasicSettings);
RBX_REGISTER_CLASS(GeometryService);
RBX_REGISTER_CLASS(Settings);
RBX_REGISTER_CLASS(GlobalAdvancedSettings);
RBX_REGISTER_CLASS(GlobalBasicSettings);
RBX_REGISTER_CLASS(Hat);
RBX_REGISTER_CLASS(Accessory);
RBX_REGISTER_CLASS(Hint);
RBX_REGISTER_CLASS(Humanoid);
RBX_REGISTER_CLASS(StatusInstance);
RBX_REGISTER_CLASS(RunService);
RBX_REGISTER_CLASS(LegacyHopperService);
RBX_REGISTER_CLASS(LocalScript);
RBX_REGISTER_CLASS(LocalWorkspace);
RBX_REGISTER_CLASS(LuaSettings);
RBX_REGISTER_CLASS(CoreScript);
RBX_REGISTER_CLASS(Message);
RBX_REGISTER_CLASS(Selection);
RBX_REGISTER_CLASS(ShirtGraphic);
RBX_REGISTER_CLASS(Shirt);
RBX_REGISTER_CLASS(Pants);
RBX_REGISTER_CLASS(Smoke);
RBX_REGISTER_CLASS(CustomParticleEmitter);
RBX_REGISTER_CLASS(Sparkles);
RBX_REGISTER_CLASS(StarterPackService);
RBX_REGISTER_CLASS(StarterPlayerService);
RBX_REGISTER_CLASS(StarterGuiService);
RBX_REGISTER_CLASS(UserInputService);
RBX_REGISTER_CLASS(CoreGuiService);
RBX_REGISTER_CLASS(StarterGear);
RBX_REGISTER_CLASS(Visit);
RBX_REGISTER_CLASS(ObjectValue);
RBX_REGISTER_CLASS(IntValue);
RBX_REGISTER_CLASS(DoubleValue);
RBX_REGISTER_CLASS(BoolValue);
RBX_REGISTER_CLASS(StringValue);
RBX_REGISTER_CLASS(BinaryStringValue);
RBX_REGISTER_CLASS(Vector3Value);
RBX_REGISTER_CLASS(RayValue);
RBX_REGISTER_CLASS(CFrameValue);
RBX_REGISTER_CLASS(Color3Value);
RBX_REGISTER_CLASS(BrickColorValue);
RBX_REGISTER_CLASS(IntConstrainedValue);
RBX_REGISTER_CLASS(DoubleConstrainedValue);
RBX_REGISTER_CLASS(Platform);
RBX_REGISTER_CLASS(SkateboardPlatform);
RBX_REGISTER_CLASS(Seat);
RBX_REGISTER_CLASS(VehicleSeat);
RBX_REGISTER_CLASS(DebrisService);
RBX_REGISTER_CLASS(TimerService);
RBX_REGISTER_CLASS(SpawnerService);
RBX_REGISTER_CLASS(ContentFilter);
RBX_REGISTER_CLASS(InsertService);
RBX_REGISTER_CLASS(LuaWebService);
RBX_REGISTER_CLASS(FriendService);
RBX_REGISTER_CLASS(RenderHooksService);
RBX_REGISTER_CLASS(CookiesService);
RBX_REGISTER_CLASS(SocialService);
RBX_REGISTER_CLASS(GamePassService);
RBX_REGISTER_CLASS(MarketplaceService);
RBX_REGISTER_CLASS(GroupService);
RBX_REGISTER_CLASS(ContextActionService);
RBX_REGISTER_CLASS(PersonalServerService);
RBX_REGISTER_CLASS(AssetService);
RBX_REGISTER_CLASS(ScriptService);
RBX_REGISTER_CLASS(ContentProvider);
RBX_REGISTER_CLASS(MeshContentProvider);
RBX_REGISTER_CLASS(TextureContentProvider);
RBX_REGISTER_CLASS(SolidModelContentProvider);
RBX_REGISTER_CLASS(CacheableContentProvider);
RBX_REGISTER_CLASS(ChangeHistoryService);
RBX_REGISTER_CLASS(HttpService);
RBX_REGISTER_CLASS(HttpRbxApiService);
RBX_REGISTER_CLASS(DataStoreService);
RBX_REGISTER_CLASS(PathfindingService);
RBX_REGISTER_CLASS(Path);
RBX_REGISTER_CLASS(Clothing);
RBX_REGISTER_CLASS(Skin);
RBX_REGISTER_CLASS(CharacterMesh);
RBX_REGISTER_CLASS(DataModelMesh);
RBX_REGISTER_CLASS(FileMesh);
RBX_REGISTER_CLASS(SpecialShape);
RBX_REGISTER_CLASS(BevelMesh);
RBX_REGISTER_CLASS(BlockMesh);
RBX_REGISTER_CLASS(CylinderMesh);
//RBX_REGISTER_CLASS(EggMesh);
RBX_REGISTER_CLASS(ServiceProvider);
RBX_REGISTER_CLASS(RootInstance);
RBX_REGISTER_CLASS(ModelInstance);
RBX_REGISTER_CLASS(BaseScript);
RBX_REGISTER_CLASS(Script);
RBX_REGISTER_CLASS(ScriptContext);
RBX_REGISTER_CLASS(RuntimeScriptService);
RBX_REGISTER_CLASS(ScriptInformationProvider);
RBX_REGISTER_CLASS(Workspace);
RBX_REGISTER_CLASS(Controller);
RBX_REGISTER_CLASS(HumanoidController);
RBX_REGISTER_CLASS(VehicleController);
RBX_REGISTER_CLASS(SkateboardController);
RBX_REGISTER_CLASS(Pose);
RBX_REGISTER_CLASS(Keyframe);
RBX_REGISTER_CLASS(KeyframeSequence);
RBX_REGISTER_CLASS(KeyframeSequenceProvider);
RBX_REGISTER_CLASS(Animation);
RBX_REGISTER_CLASS(AnimationController);
RBX_REGISTER_CLASS(AnimationTrack);
RBX_REGISTER_CLASS(AnimationTrackState);
RBX_REGISTER_CLASS(Animator);
RBX_REGISTER_CLASS(TeleportService);
RBX_REGISTER_CLASS(CharacterAppearance);
RBX_REGISTER_CLASS(LogService);
RBX_REGISTER_CLASS(ScrollingFrame);
RBX_REGISTER_CLASS(FlyweightService);
RBX_REGISTER_CLASS(CSGDictionaryService);
RBX_REGISTER_CLASS(NonReplicatedCSGDictionaryService);
RBX_REGISTER_CLASS(TouchInputService);


// network
RBX_REGISTER_CLASS(Network::PhysicsPacketCache);
RBX_REGISTER_CLASS(Network::InstancePacketCache);
RBX_REGISTER_CLASS(Network::ClusterPacketCache);
RBX_REGISTER_CLASS(Network::OneQuarterClusterPacketCache);
RBX_REGISTER_CLASS(Network::ChatFilter);

// Joints - in alpha order
RBX_REGISTER_CLASS(JointsService);
RBX_REGISTER_CLASS(Glue);
RBX_REGISTER_CLASS(Motor);
RBX_REGISTER_CLASS(Motor6D);
RBX_REGISTER_CLASS(Rotate);
RBX_REGISTER_CLASS(RotateP);
RBX_REGISTER_CLASS(RotateV);
RBX_REGISTER_CLASS(Snap);
RBX_REGISTER_CLASS(Weld);
RBX_REGISTER_CLASS(ManualSurfaceJointInstance);
RBX_REGISTER_CLASS(ManualWeld);
RBX_REGISTER_CLASS(ManualGlue);
RBX_REGISTER_CLASS(BodyMover);
RBX_REGISTER_CLASS(TouchTransmitter);
RBX_REGISTER_CLASS(FaceInstance);
RBX_REGISTER_CLASS(Sky);
RBX_REGISTER_CLASS(PVInstance);
RBX_REGISTER_CLASS(VelocityMotor);
RBX_REGISTER_CLASS(Feature);
RBX_REGISTER_CLASS(DynamicRotate);
RBX_REGISTER_CLASS(JointInstance);
RBX_REGISTER_CLASS(Attachment);
RBX_REGISTER_CLASS(SpawnLocation);
RBX_REGISTER_CLASS(Mouse);
RBX_REGISTER_CLASS(PlayerMouse);
RBX_REGISTER_CLASS(Teams);
RBX_REGISTER_CLASS(BackpackItem);
RBX_REGISTER_CLASS(HopperBin);
RBX_REGISTER_CLASS(Camera);
RBX_REGISTER_CLASS(BasePlayerGui);
RBX_REGISTER_CLASS(PlayerGui);
RBX_REGISTER_CLASS(PlayerScripts);
RBX_REGISTER_CLASS(StarterPlayerScripts);
RBX_REGISTER_CLASS(StarterCharacterScripts);
RBX_REGISTER_CLASS(PartInstance);
RBX_REGISTER_CLASS(FormFactorPart);
RBX_REGISTER_CLASS(BasicPartInstance);
RBX_REGISTER_CLASS(ExtrudedPartInstance);
RBX_REGISTER_CLASS(PART::Wedge);
RBX_REGISTER_CLASS(Decal);
RBX_REGISTER_CLASS(DecalTexture);
RBX_REGISTER_CLASS(TweenService);
RBX_REGISTER_CLASS(GuiItem);
RBX_REGISTER_CLASS(GuiBase);
RBX_REGISTER_CLASS(GuiBase2d);
RBX_REGISTER_CLASS(GuiBase3d);
RBX_REGISTER_CLASS(GuiRoot);
RBX_REGISTER_CLASS(GuiObject);
RBX_REGISTER_CLASS(GuiButton);
RBX_REGISTER_CLASS(GuiLabel);
RBX_REGISTER_CLASS(GuiMain);
RBX_REGISTER_CLASS(GuiLayerCollector);
RBX_REGISTER_CLASS(BillboardGui);
RBX_REGISTER_CLASS(ScreenGui);
RBX_REGISTER_CLASS(SurfaceGui);
RBX_REGISTER_CLASS(SelectionLasso);
RBX_REGISTER_CLASS(SelectionPartLasso);
RBX_REGISTER_CLASS(SelectionPointLasso);
RBX_REGISTER_CLASS(TextureTrail);
RBX_REGISTER_CLASS(FloorWire);
RBX_REGISTER_CLASS(GuiService);
RBX_REGISTER_CLASS(Frame);
RBX_REGISTER_CLASS(Scale9Frame);
RBX_REGISTER_CLASS(GuiImageButton);
RBX_REGISTER_CLASS(ImageLabel);
RBX_REGISTER_CLASS(GuiTextButton);
RBX_REGISTER_CLASS(TextBox);
RBX_REGISTER_CLASS(TextLabel);
RBX_REGISTER_CLASS(PartAdornment);
RBX_REGISTER_CLASS(PVAdornment);
RBX_REGISTER_CLASS(Handles);
RBX_REGISTER_CLASS(HandlesBase);
RBX_REGISTER_CLASS(ArcHandles);
RBX_REGISTER_CLASS(SelectionBox);
RBX_REGISTER_CLASS(SelectionSphere);
RBX_REGISTER_CLASS(HandleAdornment);
RBX_REGISTER_CLASS(BoxHandleAdornment);
RBX_REGISTER_CLASS(ConeHandleAdornment);
RBX_REGISTER_CLASS(CylinderHandleAdornment);
RBX_REGISTER_CLASS(SphereHandleAdornment);
RBX_REGISTER_CLASS(LineHandleAdornment);
RBX_REGISTER_CLASS(ImageHandleAdornment);
RBX_REGISTER_CLASS(SurfaceSelection);
RBX_REGISTER_CLASS(CollectionService);
RBX_REGISTER_CLASS(Configuration);
RBX_REGISTER_CLASS(Folder);
RBX_REGISTER_CLASS(MotorFeature);
RBX_REGISTER_CLASS(Hole);
RBX_REGISTER_CLASS(MegaClusterInstance);
RBX_REGISTER_CLASS(PluginMouse);
RBX_REGISTER_CLASS(PluginManager);
RBX_REGISTER_CLASS(Plugin);
RBX_REGISTER_CLASS(Toolbar);
RBX_REGISTER_CLASS(RBX::Button);
//Conditional parts here
#ifdef _PRISM_PYRAMID_
RBX_REGISTER_CLASS(PrismInstance);
RBX_REGISTER_CLASS(PyramidInstance);
RBX_REGISTER_CLASS(ParallelRampInstance);
RBX_REGISTER_CLASS(RightAngleRampInstance);
RBX_REGISTER_CLASS(CornerWedgeInstance);
#endif // _PRISM_PYRAMID_
RBX_REGISTER_CLASS(CustomEvent);
RBX_REGISTER_CLASS(CustomEventReceiver);
//RBX_REGISTER_CLASS(PropertyInstance);
RBX_REGISTER_CLASS(BindableFunction);
RBX_REGISTER_CLASS(BindableEvent);
RBX_REGISTER_CLASS(RBX::Scripting::DebuggerManager);
RBX_REGISTER_CLASS(RBX::Scripting::ScriptDebugger);
RBX_REGISTER_CLASS(RBX::Scripting::DebuggerBreakpoint);
RBX_REGISTER_CLASS(RBX::Scripting::DebuggerWatch);
RBX_REGISTER_CLASS(Light);
RBX_REGISTER_CLASS(PointLight);
RBX_REGISTER_CLASS(SpotLight);
RBX_REGISTER_CLASS(SurfaceLight);
RBX_REGISTER_CLASS(LoginService);
RBX_REGISTER_CLASS(ReplicatedStorage);
RBX_REGISTER_CLASS(RobloxReplicatedStorage);
RBX_REGISTER_CLASS(ServerScriptService);
RBX_REGISTER_CLASS(ServerStorage);
RBX_REGISTER_CLASS(RemoteFunction);
RBX_REGISTER_CLASS(RemoteEvent);
RBX_REGISTER_CLASS(TerrainRegion);
RBX_REGISTER_CLASS(ModuleScript);
RBX_REGISTER_CLASS(PointsService);
RBX_REGISTER_CLASS(AdService);
RBX_REGISTER_CLASS(NotificationService);
RBX_REGISTER_CLASS(ReplicatedFirst);
RBX_REGISTER_CLASS(PartOperation);
RBX_REGISTER_CLASS(PartOperationAsset);
RBX_REGISTER_CLASS(UnionOperation);
RBX_REGISTER_CLASS(NegateOperation);
RBX_REGISTER_CLASS(Soundscape::SoundService);
RBX_REGISTER_CLASS(Soundscape::SoundChannel);
RBX_REGISTER_CLASS(GamepadService);
RBX_REGISTER_CLASS(LuaSourceContainer);
RBX_REGISTER_CLASS(HapticService);

// Xbox
#if defined(RBX_PLATFORM_DURANGO)
#include "v8datamodel/PlatformService.h"
RBX_REGISTER_CLASS(PlatformService);
#endif

static void onSlotException(std::exception& ex)
{
	FASTLOG(FLog::Error, "Slot Exception");
	RBX::StandardOut::singleton()->printf(MESSAGE_ERROR, "exception while signalling: %s", ex.what());
}

FactoryRegistrator::FactoryRegistrator()
{
	G3D::System::time();// Initialize the Program Start Time.
	registerSound();
	RBX::registerScriptDescriptors();
	registerBodyMovers();

	registerValueClasses();
	RBX::registerStatsClasses();
	RBX::Surface::registerSurfaceDescriptors();

	rbx::signals::slot_exception_handler = onSlotException;

	srand(RBX::randomSeed());

	ModelInstance::hackPhysicalCharacter();
}

// Enum types
RBX_REGISTER_ENUM(ChangeHistoryService::RuntimeUndoBehavior);
RBX_REGISTER_ENUM(FunctionalTest::Result);
RBX_REGISTER_ENUM(TaskScheduler::PriorityMethod);
RBX_REGISTER_ENUM(TaskScheduler::Job::SleepAdjustMethod);
RBX_REGISTER_ENUM(TaskScheduler::ThreadPoolConfig);
RBX_REGISTER_ENUM(Action::ActionType);
RBX_REGISTER_ENUM(Controller::Button);
RBX_REGISTER_ENUM(HopperBin::BinType);
RBX_REGISTER_ENUM(GuiObject::SizeConstraint);
RBX_REGISTER_ENUM(GuiObject::TweenEasingStyle);
RBX_REGISTER_ENUM(GuiObject::TweenStatus);
RBX_REGISTER_ENUM(GuiObject::TweenEasingDirection);
RBX_REGISTER_ENUM(TextService::XAlignment);
RBX_REGISTER_ENUM(TextService::YAlignment);
RBX_REGISTER_ENUM(TextService::FontSize);
RBX_REGISTER_ENUM(TextService::Font);
RBX_REGISTER_ENUM(Camera::CameraType);
RBX_REGISTER_ENUM(Camera::CameraMode);
RBX_REGISTER_ENUM(Camera::CameraPanMode);
RBX_REGISTER_ENUM(LegacyController::InputType);
RBX_REGISTER_ENUM(DataModelArbiter::ConcurrencyModel);
RBX_REGISTER_ENUM(DataModelMesh::LODType);
RBX_REGISTER_ENUM(DebugSettings::ErrorReporting);
RBX_REGISTER_ENUM(EThrottle::EThrottleType);
RBX_REGISTER_ENUM(Feature::InOut);
RBX_REGISTER_ENUM(Feature::LeftRight);
RBX_REGISTER_ENUM(Feature::TopBottom);
RBX_REGISTER_ENUM(Joint::JointType);
RBX_REGISTER_ENUM(KeywordFilterType);
RBX_REGISTER_ENUM(Legacy::SurfaceConstraint);
RBX_REGISTER_ENUM(NormalId);
RBX_REGISTER_ENUM(Vector3::Axis);
RBX_REGISTER_ENUM(Humanoid::Status);
RBX_REGISTER_ENUM(Humanoid::HumanoidRigType);
RBX_REGISTER_ENUM(Humanoid::NameOcclusion);
RBX_REGISTER_ENUM(Humanoid::HumanoidDisplayDistanceType);
RBX_REGISTER_ENUM(RBX::HUMAN::StateType);
RBX_REGISTER_ENUM(DataModel::CreatorType);
RBX_REGISTER_ENUM(DataModel::Genre);
RBX_REGISTER_ENUM(DataModel::GearGenreSetting);
RBX_REGISTER_ENUM(DataModel::GearType);
RBX_REGISTER_ENUM(Instance::SaveFilter);
RBX_REGISTER_ENUM(BasicPartInstance::LegacyPartType);
RBX_REGISTER_ENUM(KeyframeSequence::Priority);
RBX_REGISTER_ENUM(SocialService::StuffType);
RBX_REGISTER_ENUM(PersonalServerService::PrivilegeType);
RBX_REGISTER_ENUM(ExtrudedPartInstance::VisualTrussStyle);
#ifdef _PRISM_PYRAMID_
RBX_REGISTER_ENUM(PrismInstance::NumSidesEnum);
RBX_REGISTER_ENUM(PyramidInstance::NumSidesEnum);
#endif
RBX_REGISTER_ENUM(FriendService::FriendStatus);
RBX_REGISTER_ENUM(FriendService::FriendEventType);
RBX_REGISTER_ENUM(Handles::VisualStyle);
RBX_REGISTER_ENUM(SkateboardPlatform::MoveState);
RBX_REGISTER_ENUM(SoundType);
RBX_REGISTER_ENUM(SpecialShape::MeshType);
RBX_REGISTER_ENUM(SurfaceType);
RBX_REGISTER_ENUM(PartInstance::FormFactor);
RBX_REGISTER_ENUM(CollisionFidelity);
RBX_REGISTER_ENUM(UserInputService::SwipeDirection);
RBX_REGISTER_ENUM(UserInputService::Platform);
RBX_REGISTER_ENUM(UserInputService::MouseType);
RBX_REGISTER_ENUM(PartMaterial);
RBX_REGISTER_ENUM(PhysicalPropertiesMode);
RBX_REGISTER_ENUM(NetworkOwnership);
RBX_REGISTER_ENUM(Time::SampleMethod);
RBX_REGISTER_ENUM(GuiService::SpecialKey);
RBX_REGISTER_ENUM(GuiService::CenterDialogType);
RBX_REGISTER_ENUM(GuiService::UiMessageType);
RBX_REGISTER_ENUM(ChatService::ChatColor);
RBX_REGISTER_ENUM(MarketplaceService::CurrencyType);
RBX_REGISTER_ENUM(MarketplaceService::InfoType);
RBX_REGISTER_ENUM(CharacterMesh::BodyPart);
RBX_REGISTER_ENUM(GameSettings::VideoQuality);
RBX_REGISTER_ENUM(GameSettings::UploadSetting);
RBX_REGISTER_ENUM(GameBasicSettings::ControlMode);
RBX_REGISTER_ENUM(GameBasicSettings::RenderQualitySetting);
RBX_REGISTER_ENUM(GameBasicSettings::CameraMode);
RBX_REGISTER_ENUM(GameBasicSettings::TouchCameraMovementMode);
RBX_REGISTER_ENUM(GameBasicSettings::ComputerCameraMovementMode);
RBX_REGISTER_ENUM(GameBasicSettings::TouchMovementMode);
RBX_REGISTER_ENUM(GameBasicSettings::ComputerMovementMode);
RBX_REGISTER_ENUM(GameBasicSettings::RotationType);
RBX_REGISTER_ENUM(Frame::Style);
RBX_REGISTER_ENUM(GuiButton::Style);
RBX_REGISTER_ENUM(DialogRoot::DialogPurpose);
RBX_REGISTER_ENUM(DialogRoot::DialogTone);
RBX_REGISTER_ENUM(Voxel::CellMaterial);
RBX_REGISTER_ENUM(Voxel::CellBlock);
RBX_REGISTER_ENUM(Voxel::CellOrientation);
RBX_REGISTER_ENUM(Voxel::WaterCellForce);
RBX_REGISTER_ENUM(Voxel::WaterCellDirection);
RBX_REGISTER_ENUM(Explosion::ExplosionType);
RBX_REGISTER_ENUM(InputObject::UserInputType);
RBX_REGISTER_ENUM(InputObject::UserInputState);
RBX_REGISTER_ENUM(AssetService::AccessType);
RBX_REGISTER_ENUM(HttpService::HttpContentType);
RBX_REGISTER_ENUM(StarterGuiService::CoreGuiType);
RBX_REGISTER_ENUM(StarterPlayerService::DeveloperTouchCameraMovementMode);
RBX_REGISTER_ENUM(StarterPlayerService::DeveloperComputerCameraMovementMode);
RBX_REGISTER_ENUM(StarterPlayerService::DeveloperCameraOcclusionMode);
RBX_REGISTER_ENUM(StarterPlayerService::DeveloperTouchMovementMode);
RBX_REGISTER_ENUM(StarterPlayerService::DeveloperComputerMovementMode);
RBX_REGISTER_ENUM(TeleportService::TeleportState);
RBX_REGISTER_ENUM(TeleportService::TeleportType);
RBX_REGISTER_ENUM(KeyCode);
RBX_REGISTER_ENUM(MessageType);
RBX_REGISTER_ENUM(MarketplaceService::ProductPurchaseDecision);
RBX_REGISTER_ENUM(ThrottlingPriority);
RBX_REGISTER_ENUM(Soundscape::ReverbType);
RBX_REGISTER_ENUM(Soundscape::ListenerType);
RBX_REGISTER_ENUM(Soundscape::RollOffMode);
RBX_REGISTER_ENUM(PlayerActionType);
RBX_REGISTER_ENUM(RunService::RenderPriority);
RBX_REGISTER_ENUM(AdvArrowToolBase::JointCreationMode);
RBX_REGISTER_ENUM(GuiObject::ImageScale);
RBX_REGISTER_ENUM(UserInputService::OverrideMouseIconBehavior);
RBX_REGISTER_ENUM(Pose::PoseEasingStyle);
RBX_REGISTER_ENUM(Pose::PoseEasingDirection);
RBX_REGISTER_ENUM(HapticService::VibrationMotor);
RBX_REGISTER_ENUM(UserInputService::UserCFrame);

#if defined(RBX_PLATFORM_DURANGO)
RBX_REGISTER_ENUM(XboxKeyBoardType)
RBX_REGISTER_ENUM(VoiceChatState)
#endif
