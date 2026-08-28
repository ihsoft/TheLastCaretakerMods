class ABP_Forklift_Possesable_C : public AVoyageVehicleForkliftPawn
{
public:
    class USimpleConstructionScript* SimpleConstructionScript = "SimpleConstructionScript'/Game/Blueprints/Vehicles/BP_Forklift_Possesable.BP_Forklift_Possesable_C:SimpleConstructionScript_0'";
    class UFunction* UberGraphFunction = "Function'/Game/Blueprints/Vehicles/BP_Forklift_Possesable.BP_Forklift_Possesable_C:ExecuteUbergraph_BP_Forklift_Possesable'";
    struct FPointerToUberGraphFrame UberGraphFrame = {};
    double SteeringMul = 80;
    double VehicleMass = 3000;
    struct FTransform ForkTransform = {
        "Rotation": FQuat(0, 0, 0, 1),
        "Translation": FVector(0, 0, 0),
        "Scale3D": FVector(1, 1, 1)
    };
    float MaxForkOffset = 400;
    float MinForkOffset = -15;
    float MaxForkTiltOffset = 0;
    float MinForkTiltOffset = -45;
    float MaxForkTiltSpeed = 30;
    class UVoyageInputAction* ForkTiltUpInputAction = "VoyageInputAction'/Game/Game/Input/Vehicle/Forklift/IAV_Forklift_ForkTiltUp.IAV_Forklift_ForkTiltUp'";
    class UVoyageInputAction* ForkTiltDownInputAction = "VoyageInputAction'/Game/Game/Input/Vehicle/Forklift/IAV_Forklift_ForkTiltDown.IAV_Forklift_ForkTiltDown'";
    class UVoyageInputAction* ForkUpInputAction = "VoyageInputAction'/Game/Game/Input/Vehicle/Forklift/IAV_Forklift_ForkUp.IAV_Forklift_ForkUp'";
    class UVoyageInputAction* ForkDownInputAction = "VoyageInputAction'/Game/Game/Input/Vehicle/Forklift/IAV_Forklift_ForkDown.IAV_Forklift_ForkDown'";
    class UInputAction* ThrottleForwardAction = "InputAction'/Game/Game/Input/Vehicle/IA_VehicleForward.IA_VehicleForward'";
    class UInputAction* ThrottleBackwardAction = "InputAction'/Game/Game/Input/Vehicle/IA_VehicleBackward.IA_VehicleBackward'";
    class UInputAction* SteeringRightAction = "InputAction'/Game/Game/Input/Vehicle/IA_VehicleRight.IA_VehicleRight'";
    class UInputAction* SteeringLeftAction = "InputAction'/Game/Game/Input/Vehicle/IA_VehicleLeft.IA_VehicleLeft'";
    class UVoyageInputAction* ExitAction = "VoyageInputAction'/Game/Game/Input/Vehicle/IAV_VehicleExit.IAV_VehicleExit'";
    class UInputAction* LookRightInputAction = "InputAction'/Game/Game/Input/Character/IA_LookRight.IA_LookRight'";
    class UInputAction* LookUpInputAction = "InputAction'/Game/Game/Input/Character/IA_LookUp.IA_LookUp'";
    class UInputAction* ZoomAction = "InputAction'/Game/Game/Input/Character/IA_Zoom.IA_Zoom'";
    class UVoyageInputControlsComponent* InputControls = "VoyageInputControlsComponent'/Game/Blueprints/Vehicles/BP_Forklift_Possesable.Default__BP_Forklift_Possesable_C:InputControls'";
    class UVoyageFastSceneComponent* MeshComponent = "VoyageFastSceneComponent'/Game/Blueprints/Vehicles/BP_Forklift_Possesable.Default__BP_Forklift_Possesable_C:VehicleMesh'";
    class UVoyageLevelInstanceComponent* LevelInstanceComponent = "VoyageLevelInstanceComponent'/Game/Blueprints/Vehicles/BP_Forklift_Possesable.Default__BP_Forklift_Possesable_C:LevelInstance'";
    class UVoyageSpringArmComponent* SpringArmComponent = "VoyageSpringArmComponent'/Game/Blueprints/Vehicles/BP_Forklift_Possesable.Default__BP_Forklift_Possesable_C:SprintArm'";
    class USceneComponent* CameraOffsetCheckComponent = "SceneComponent'/Game/Blueprints/Vehicles/BP_Forklift_Possesable.Default__BP_Forklift_Possesable_C:CameraOffsetCheck'";
    class USceneComponent* CameraAttachmentComponent = "SceneComponent'/Game/Blueprints/Vehicles/BP_Forklift_Possesable.Default__BP_Forklift_Possesable_C:CameraAttachment'";
    class UCameraComponent* CameraComponent = "CameraComponent'/Game/Blueprints/Vehicles/BP_Forklift_Possesable.Default__BP_Forklift_Possesable_C:Camera'";
    class UVoyageDynamicPlayerInputComponent* DynamicPlayerInputComponent = "VoyageDynamicPlayerInputComponent'/Game/Blueprints/Vehicles/BP_Forklift_Possesable.Default__BP_Forklift_Possesable_C:DynamicPlayerInputComponent'";
    class UVoyageDestructibleObjectComponent* DestructibleObjectComponent = "VoyageDestructibleObjectComponent'/Game/Blueprints/Vehicles/BP_Forklift_Possesable.Default__BP_Forklift_Possesable_C:DestructibleObjectComponent'";
    FName ExitComponentTag = FName("ExitComponentTag");
    bool bAllowFirstPersonCamera = false;
    class UVoyageInputAction* HornInputAction = "VoyageInputAction'/Game/Game/Input/Vehicle/IAV_VehicleHorn.IAV_VehicleHorn'";
    class UAnimBlueprintGeneratedClass* PossessedVehicleAnimationBlueprint = "AnimBlueprintGeneratedClass'/Game/Characters/Mannequins/Animations/ABP_Manny_ForkLift.ABP_Manny_ForkLift_C'";
    class UVoyageFastSceneComponent* RootComponent = "VoyageFastSceneComponent'/Game/Blueprints/Vehicles/BP_Forklift_Possesable.Default__BP_Forklift_Possesable_C:VehicleMesh'";
    TArray<FName> Tags = {
        FName("NoFabrication"),
        FName("RoomVolumeExclude"),
        FName("NoWeld")
    };
    class UChildActorComponent* LightActor;
    class UAudioComponent* AudioEmitterPower;
    class UAudioComponent* AudioEmitterForkTilt;
    class UAudioComponent* AudioEmitterForkOffset;
    class UVoyageInterfaceInputControlsComponent* VoyageInterfaceInputControls;
    class UBP_DurabilityIndicatorComponent_C* BP_DurabilityIndicatorComponent;
    class UAudioComponent* AudioEmitterHorn;
    class UStaticMeshComponent* SM_Forklift_A_SM_Forklift_A_01_Fork_02;
    class USceneComponent* Scene1;
    class UVoyageDynamicCollisionComponent* VoyageDynamicCollision;
    class UBP_DynamicWavesComponent_C* BP_DynamicWavesComponent;
    class UBP_BuoyancyComponent_C* BP_BuoyancyComponent;
    class UStaticMeshComponent* SM_Forklift_A_SM_Forklift_A_01;
    class USceneComponent* Scene;
    class USceneComponent* RightHandTarget;
    class USceneComponent* LeftHandTarget;
    class UChildActorComponent* ForkLever;
    class UAudioComponent* AudioEmitterEngine;
    class UVoyageModuleSocketViewComponent* SmokeSocket;
    class UVoyageModuleSocketViewComponent* PetrolSocket;
    class UVoyageCustomModuleComponent* VoyageCustomModule;
    class UStaticMeshComponent* Fork;
    class UPhysicsConstraintComponent* ForkConstraint;
    class UStaticMeshComponent* SM_Forklift_A_SM_Forklift_A_01_Wheel3;
    class UStaticMeshComponent* SM_Forklift_A_SM_Forklift_A_01_Wheel2;
    class UStaticMeshComponent* SM_Forklift_A_SM_Forklift_A_01_Wheel1;
    class UStaticMeshComponent* SM_Forklift_A_SM_Forklift_A_01_Wheel;
    class UPhysicsConstraintComponent* PhysicsConstraint;
    class USphereComponent* Weight;
    class UPhysicsConstraintComponent* FrontConstraintLeft;
    class UPhysicsConstraintComponent* FrontConstraintRight;
    class UCapsuleComponent* FrontWheelRight;
    class UCapsuleComponent* FrontWheelLeft;
    class UPhysicsConstraintComponent* FrontSteeringLeftConstraint;
    class UBoxComponent* FrontSteeringLeft;
    class UPhysicsConstraintComponent* FrontSteeringRightConstraint;
    class UBoxComponent* FrontSteeringRight;
    class USceneComponent* Front;
    class UPhysicsConstraintComponent* RearSteeringRightConstraint;
    class UPhysicsConstraintComponent* RearConstraintRight;
    class UBoxComponent* RearSteeringRight;
    class UCapsuleComponent* RearWheelRight;
    class UBoxComponent* RearSteeringLeft;
    class UCapsuleComponent* RearWheelLeft;
    class USceneComponent* Rear;
    class UPhysicsConstraintComponent* RearSteeringLeftConstraint;
    class UPhysicsConstraintComponent* RearConstraintLeft;
    class UVoyageControllerInputControlsComponent* VoyageControllerInputControls;
    class UVoyagePersistentActorComponent* VoyagePersistentActor;
    class USceneComponent* CharacterLocation;
    class UBoxComponent* Box;
    class UInteractiveObjectComponent* InteractiveObject;
    class UStaticMeshComponent* SteeringWheel;
    double SteeringSide;
    double Steering;
    double Acceleration;
    double TotalMass;
    double ActualPower;
    bool bIsEnginePlaying;

};