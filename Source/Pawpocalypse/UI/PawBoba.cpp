#include "UI/PawBoba.h"

#include "Audio/PawAudioSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Rendering/DrawElements.h"
#include "Save/PawSettingsSubsystem.h"
#include "UI/PawIcons.h"
#include "UI/PawWidgets.h"

namespace
{
	// The capture tools (store_shots, store_clips, screen_tour) turn her off, so no cameo lands in a shot by chance.
	TAutoConsoleVariable<int32> CVarPawBoba(TEXT("paw.Boba"), 1, TEXT("Boba's cameos in the menus (0: off; the capture tools set it)."));

	const TCHAR* const BobaSet = TEXT("/Game/Pawpocalypse/Art/UI/Boba/DA_Boba.DA_Boba");
	constexpr int32 BobaZOrder = 900; // over every menu's layer, under nothing that takes input (she takes none)
}

// --- the layer ----------------------------------------------------------------------------------------------------------

bool UPawBobaLayer::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		Photo = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Photo"));
		UOverlaySlot* PhotoSlot = Root->AddChildToOverlay(Photo);
		PhotoSlot->SetHorizontalAlignment(HAlign_Left);
		PhotoSlot->SetVerticalAlignment(VAlign_Top);
		Photo->SetRenderTransformPivot(FVector2D(0.5, 0.5));
	}
	const bool bOk = Super::Initialize();
	if (bOk)
	{
		SetVisibility(ESlateVisibility::HitTestInvisible); // never takes a click
		if (Photo)
		{
			Photo->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	return bOk;
}

void UPawBobaLayer::ShowPose(UTexture2D* Texture, double Size, double Aspect, const Pc::PcBobaPose& Pose,
	const std::vector<std::pair<double, double>>& Points)
{
	if (!Photo || !Texture || !Pose.bVisible)
	{
		HideCameo();
		return;
	}
	const FVector2D Draw(Size * Aspect, Size);
	if (Photo->GetBrush().GetResourceObject() != Texture || Photo->GetBrush().GetImageSize() != Draw)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = Draw;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Photo->SetBrush(Brush);
	}
	Centre = FVector2D(Pose.X, Pose.Y);
	FWidgetTransform T;
	T.Translation = Centre - Draw * 0.5;
	T.Angle = static_cast<float>(Pose.Rotation);
	T.Scale = FVector2D(Pose.bFlip ? -1.0 : 1.0, 1.0);
	Photo->SetRenderTransform(T);
	Photo->SetVisibility(ESlateVisibility::HitTestInvisible);

	// The trail stops at her back edge, so it never paints over her (this paint comes after the photo).
	Trail.Reset();
	const double Back = Size * Aspect * 0.45;
	for (const std::pair<double, double>& P : Points)
	{
		if (FMath::Abs(P.first - Pose.X) >= Back)
		{
			Trail.Add(FVector2f(static_cast<float>(P.first), static_cast<float>(P.second)));
		}
	}
	TrailBand = Size * 0.55 / 6.0;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void UPawBobaLayer::HideCameo()
{
	if (Photo && Photo->GetVisibility() != ESlateVisibility::Collapsed)
	{
		Photo->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Trail.Num() > 0)
	{
		Trail.Reset();
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

bool UPawBobaLayer::IsPhotoShown() const
{
	return Photo && Photo->GetVisibility() != ESlateVisibility::Collapsed;
}

const UTexture2D* UPawBobaLayer::GetPhoto() const
{
	return Photo ? Cast<UTexture2D>(Photo->GetBrush().GetResourceObject()) : nullptr;
}

int32 UPawBobaLayer::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 Layer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	if (Trail.Num() < 2)
	{
		return Layer;
	}
	// The six bands (the first try's nyan: hsv(i / 6 * 0.78, 0.75, 1), 0.9 alpha), each its own line.
	++Layer;
	for (int32 I = 0; I < 6; ++I)
	{
		const float Offset = static_cast<float>((static_cast<double>(I) - 2.5) * TrailBand);
		TArray<FVector2f> Band;
		Band.Reserve(Trail.Num());
		for (const FVector2f& P : Trail)
		{
			Band.Add(FVector2f(P.X, P.Y + Offset));
		}
		FLinearColor C = FLinearColor::MakeFromHSV8(static_cast<uint8>(255.0 * I / 6.0 * 0.78), static_cast<uint8>(255.0 * 0.75), 255);
		C.A = 0.9f;
		FSlateDrawElement::MakeLines(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), Band, ESlateDrawEffect::None, C, true,
			static_cast<float>(TrailBand));
	}
	return Layer;
}

// --- the subsystem ------------------------------------------------------------------------------------------------------

void UPawBobaSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	State.Seed = FPlatformTime::Cycles64();
	LoadPhotos();
}

void UPawBobaSubsystem::Deinitialize()
{
	if (Layer)
	{
		Layer->RemoveFromParent();
		Layer = nullptr;
	}
	Super::Deinitialize();
}

UPawBobaSubsystem* UPawBobaSubsystem::Of(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UPawBobaSubsystem>() : nullptr;
}

void UPawBobaSubsystem::LoadPhotos()
{
	Photos.Reset();
	Aspects.clear();
	if (const UPawIconSet* Set = LoadObject<UPawIconSet>(nullptr, BobaSet))
	{
		for (UTexture2D* Tex : Set->Textures)
		{
			const FVector2D Size = PhotoSize(Tex);
			if (Size.Y > 0.0)
			{
				Photos.Add(Tex);
				Aspects.push_back(Size.X / Size.Y);
			}
		}
	}
}

FVector2D UPawBobaSubsystem::PhotoSize(const UTexture2D* Photo)
{
	if (!Photo)
	{
		return FVector2D::ZeroVector;
	}
	FVector2D Size(Photo->GetSizeX(), Photo->GetSizeY());
#if WITH_EDITORONLY_DATA
	if (Size.X <= 0.0 || Size.Y <= 0.0)
	{
		Size = FVector2D(Photo->Source.GetSizeX(), Photo->Source.GetSizeY());
	}
#endif
	return Size;
}

UTexture2D* UPawBobaSubsystem::GetPhoto(int32 Index) const
{
	return Photos.IsValidIndex(Index) ? Photos[Index].Get() : nullptr;
}

bool UPawBobaSubsystem::EffectsOn() const
{
	if (CVarPawBoba.GetValueOnGameThread() == 0)
	{
		return false;
	}
	const UGameInstance* GI = GetGameInstance();
	const UPawSettingsSubsystem* S = GI ? GI->GetSubsystem<UPawSettingsSubsystem>() : nullptr;
	return !S || S->Get().bEffectsEnabled;
}

FVector2D UPawBobaSubsystem::ScreenSize() const
{
	if (Layer)
	{
		const FVector2D Size = Layer->GetCachedGeometry().GetLocalSize();
		if (Size.X > 0.0 && Size.Y > 0.0)
		{
			return Size;
		}
	}
	return FVector2D(1920.0, 1080.0); // not drawn yet (or a test): the design size
}

void UPawBobaSubsystem::EnsureLayer()
{
	if (!Layer)
	{
		Layer = CreateWidget<UPawBobaLayer>(GetGameInstance(), PawUi::Resolve<UPawBobaLayer>());
	}
	// In the viewport above the menus once there is one (a test's game instance has none; a level change drops it).
	const UGameInstance* GI = GetGameInstance();
	if (Layer && !Layer->IsInViewport() && GI && GI->GetGameViewportClient())
	{
		Layer->AddToViewport(BobaZOrder);
	}
}

void UPawBobaSubsystem::EnterMenu(const FString& Tag)
{
	EnsureLayer();
	Pc::BobaEnterMenu(State, TCHAR_TO_UTF8(*Tag));
}

void UPawBobaSubsystem::LeaveMenu(const FString& Tag)
{
	Pc::BobaLeaveMenu(State, TCHAR_TO_UTF8(*Tag)); // a cameo promised for later comes only if its menu is still up then (Tick)
}

void UPawBobaSubsystem::Suppress(const FString& Tag, bool bOn)
{
	Pc::BobaSuppress(State, TCHAR_TO_UTF8(*Tag), bOn);
	if (bOn)
	{
		LaterDelay = -1.0;
		Draw(); // cut now, not on the next tick
	}
}

bool UPawBobaSubsystem::Force()
{
	EnsureLayer();
	if (!Pc::BobaForceAny(State, EffectsOn(), Aspects, ScreenSize().Y))
	{
		return false;
	}
	Started();
	return true;
}

bool UPawBobaSubsystem::ForceKind(Pc::PcBobaKind Kind)
{
	EnsureLayer();
	if (!Pc::BobaForce(State, Kind, EffectsOn(), Aspects, ScreenSize().Y))
	{
		return false;
	}
	Started();
	return true;
}

void UPawBobaSubsystem::ForceLater(Pc::PcBobaKind Kind, double Delay, const FString& Tag)
{
	LaterKind = Kind;
	LaterDelay = Delay;
	LaterTag = Tag;
}

void UPawBobaSubsystem::Started()
{
	UPawAudioSubsystem::PlayFrom(Layer ? static_cast<const UObject*>(Layer) : static_cast<const UObject*>(GetGameInstance()), TEXT("boba_cameo"));
	Draw();
}

void UPawBobaSubsystem::Tick(float DeltaSeconds)
{
	if (LaterDelay >= 0.0)
	{
		LaterDelay -= DeltaSeconds;
		if (LaterDelay < 0.0 && State.Menus.count(TCHAR_TO_UTF8(*LaterTag)) > 0)
		{
			ForceKind(LaterKind);
		}
	}
	const bool bWas = State.Cameo.bLive;
	if (Pc::BobaStep(State, DeltaSeconds, EffectsOn(), Aspects, ScreenSize().Y))
	{
		Started();
	}
	if (State.Cameo.bLive || bWas)
	{
		Draw();
	}
}

void UPawBobaSubsystem::Draw()
{
	if (!Layer)
	{
		return;
	}
	const Pc::PcBobaCameo& C = State.Cameo;
	UTexture2D* Tex = GetPhoto(C.Photo);
	if (!C.bLive || !Tex)
	{
		Layer->HideCameo();
		return;
	}
	const FVector2D Screen = ScreenSize();
	Layer->ShowPose(Tex, C.Size, C.Aspect, Pc::BobaPose(C, Screen.X, Screen.Y), Pc::BobaTrail(C, Screen.X, Screen.Y));
}
