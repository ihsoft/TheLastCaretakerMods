// TYPE/CALL-ONLY MIRROR. Steam25191271, EXE
// 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B,
// UE5.8. Native registration and FFunctionParams independently decoded.
// OnEnterVehicle: one ObjectProperty NewPossessor, Engine.Controller, void.
// OnExitVehicle: no parameters, void. See GAME_DERIVED_SOURCES.md.
// No UNVERSIONED child/CDO authoring against this incomplete 38-property base.
// HC26 isolated exception: tagged properties only, no native field/default-
// subobject CDO deltas, independently verified before packaging/runtime test.
// This is not a complete native constructor/schema reconstruction.
// HC19 imports its identity and asks the SHIPPING GAME to spawn the real class.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "VoyageInputControlsComponent.h"
#include "Camera/CameraComponent.h"
#include "VoyageFastSceneComponent.h"
#include "PlayerInputInterfaceAction.h"
#include "InteractiveInterface.h"
#include "VoyageVehiclePawn.generated.h"

UCLASS(BlueprintType)
// HC32: native common base has no InteractiveInterface (current FClassParams).
// The generated station must implement that interface explicitly, not inherit it here.
class VOYAGE_API AVoyageVehiclePawn : public APawn
{
    GENERATED_BODY()
public:
    // HC28 exact base-owned action extension, renewed current registration.
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Input")
    TArray<FPlayerInputInterfaceAction> GetProvidedActionsBP();
    virtual TArray<FPlayerInputInterfaceAction> GetProvidedActionsBP_Implementation() { return {}; }
    // Exact native root only, to prevent the editor inventing DefaultSceneRoot.
    // Other native subobjects are supplied by the SHIPPING constructor; HC26
    // must not serialize null overrides of their properties.
    AVoyageVehiclePawn()
    {
        static const FName NativeRootName(TEXT("VehicleMesh"));
        SetRootComponent(CreateDefaultSubobject<UVoyageFastSceneComponent>(NativeRootName));
    }
    // Field-reference only: current mapping[0], stock CDO component type.
    UPROPERTY(BlueprintReadWrite, Category="Vehicle")
    TObjectPtr<UVoyageInputControlsComponent> InputControls;
    // HC24 field references only: current mapping[6,10,25], no native CDO.
    // Stock Forklift CameraComponent is Engine.CameraComponent.
    // bFirstPersonCamera descriptor bool setter145297590 writes actor+0x3e8.
    UPROPERTY(BlueprintReadWrite, Category="Vehicle")
    TObjectPtr<UCameraComponent> CameraComponent;
    UPROPERTY(BlueprintReadWrite, Category="Vehicle")
    bool bAllowFirstPersonCamera;
    UPROPERTY(BlueprintReadWrite, Category="Vehicle")
    bool bFirstPersonCamera;
    // Field-reference-only extension. Mapping NameProperty[9], native property
    // descriptor149b72d30, offset0x3a0. No CDO/child serialization permitted.
    UPROPERTY(BlueprintReadWrite, Category="Vehicle")
    FName ExitComponentTag;
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Vehicle")
    void OnEnterVehicle(AController* NewPossessor);
    virtual void OnEnterVehicle_Implementation(AController* NewPossessor) {}
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Vehicle")
    void OnExitVehicle();
    virtual void OnExitVehicle_Implementation() {}
};
