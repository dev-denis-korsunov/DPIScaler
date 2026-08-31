#include "DPIScalerWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "UObject/UnrealType.h"
#include "Widgets/Layout/SDPIScaler.h"

FDPIBreakpointRule::FDPIBreakpointRule()
	: Name(TEXT("Default")), WidthBreakpoint(0), HeightBreakpoint(0), MinAspectRatio(0.75f), MaxAspectRatio(1.78f)
	, ScaleMode(EDPIBreakpointScaleMode::Curve), TargetUIScale(1.0f), ScaleAxis(EDPIBreakpointScaleAxis::ScreenWidth)
	, bUseWidthBreakpoint(false), bUseHeightBreakpoint(false), bUseMinAspectRatio(false), bUseMaxAspectRatio(false)
{
	FRichCurve* Curve = ScaleCurve.GetRichCurve();
	Curve->SetKeyInterpMode(Curve->AddKey(0.0f, 0.0f), RCIM_Linear);
	Curve->SetKeyInterpMode(Curve->AddKey(1920.0f, 1.0f), RCIM_Linear);
}

UDPIScalerWidget::UDPIScalerWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsVariable = false;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

float UDPIScalerWidget::GetDPIScale() const
{
#if WITH_EDITORONLY_DATA
	if (IsDesignTime() && (bDesignerMute || !bScreenPreview))
	{
	#if WITH_EDITOR
		return DesignerDpi.Get(1.0f);
	#else
		return 1.0f;
	#endif
	}
#endif
	float ApplicationScale = 1.0f;
	FIntPoint ViewportSize = FIntPoint::ZeroValue;
	if (!GetDPIContext(ApplicationScale, ViewportSize))
	{
		return 1.0f;
	}

	const UDPIScalerWidget* ParentScaler = FindParentDPIScaler();
	const float SourceScale = IsValid(ParentScaler) ? ParentScaler->GetAbsoluteDesiredDPIScale(ApplicationScale, ViewportSize) : ApplicationScale;
	const float ResultScale = GetAbsoluteDesiredDPIScale(ApplicationScale, ViewportSize) / FMath::Max(SourceScale, KINDA_SMALL_NUMBER);
	if (!FMath::IsFinite(ResultScale) || ResultScale <= 0.0f)
	{
		ensureMsgf(false, TEXT("DPI Scaler produced an invalid scale (%f). Falling back to 1.0."), ResultScale);
		return 1.0f;
	}
	return ResultScale;
}

bool UDPIScalerWidget::GetDPIContext(float& OutApplicationScale, FIntPoint& OutViewportSize) const
{
	OutApplicationScale = 1.0f;
	OutViewportSize = FIntPoint::ZeroValue;
#if WITH_EDITOR
	if (DesignerDpi.IsSet() && DesignerSize.IsSet())
	{
		OutApplicationScale = DesignerDpi.GetValue();
		OutViewportSize = DesignerSize.GetValue().IntPoint();
	}
	else
#endif
	{
		OutApplicationScale = UWidgetLayoutLibrary::GetViewportScale(GetWorld());
		OutViewportSize = UWidgetLayoutLibrary::GetViewportSize(GetWorld()).IntPoint();
	}
	return OutViewportSize.X > 0 && OutViewportSize.Y > 0;
}

bool UDPIScalerWidget::IsActiveRule(FName RuleName) const
{
	if (RuleName.IsNone())
	{
		return false;
	}

	float ApplicationScale = 1.0f;
	FIntPoint ViewportSize = FIntPoint::ZeroValue;
	if (!GetDPIContext(ApplicationScale, ViewportSize))
	{
		return false;
	}

	const FDPIBreakpointRule* ActiveRule = FindActiveRule(ViewportSize);
	return ActiveRule != nullptr && ActiveRule->Name == RuleName;
}

const UDPIScalerWidget* UDPIScalerWidget::FindParentDPIScaler() const
{
	const UDPIScalerWidget* ParentScaler = nullptr;
	const UPanelWidget* ParentWidget = GetParent();
	const UWidgetTree* OuterWidgetTree = Cast<UWidgetTree>(GetOuter());
	for (;;)
	{
		if (IsValid(ParentWidget))
		{
			if (ParentWidget->IsA<UDPIScalerWidget>())
			{
				ParentScaler = Cast<UDPIScalerWidget>(ParentWidget);
				break;
			}
			ParentWidget = ParentWidget->GetParent();
			continue;
		}
		if (!IsValid(OuterWidgetTree))
		{
			break;
		}
		const UWidget* WidgetTreeOuter = Cast<UWidget>(OuterWidgetTree->GetOuter());
		if (!IsValid(WidgetTreeOuter))
		{
			break;
		}
		ParentWidget = WidgetTreeOuter->GetParent();
		if (!IsValid(ParentWidget))
		{
			break;
		}
		OuterWidgetTree = Cast<UWidgetTree>(ParentWidget->GetOuter());
	}
	return ParentScaler;
}

float UDPIScalerWidget::GetAbsoluteDesiredDPIScale(float ApplicationScale, const FIntPoint& ViewportSize) const
{
	return ResolveTargetUIScale(ApplicationScale, ViewportSize, FindActiveRule(ViewportSize));
}

const FDPIBreakpointRule* UDPIScalerWidget::FindActiveRule(const FIntPoint& ViewportSize) const
{
	if (ViewportSize.X <= 0 || ViewportSize.Y <= 0)
	{
		return nullptr;
	}
	const float AspectRatio = static_cast<float>(ViewportSize.X) / static_cast<float>(ViewportSize.Y);
	for (const FDPIBreakpointRule& Rule : DPIRules)
	{
		if (Rule.bUseWidthBreakpoint && ViewportSize.X > Rule.WidthBreakpoint)
		{
			continue;
		}
		if (Rule.bUseHeightBreakpoint && ViewportSize.Y > Rule.HeightBreakpoint)
		{
			continue;
		}
		if (Rule.bUseMinAspectRatio && AspectRatio < Rule.MinAspectRatio)
		{
			continue;
		}
		if (Rule.bUseMaxAspectRatio && AspectRatio > Rule.MaxAspectRatio)
		{
			continue;
		}
		return &Rule;
	}
	return nullptr;
}

float UDPIScalerWidget::ResolveTargetUIScale(float ProjectDPIScale, const FIntPoint& ViewportSize, const FDPIBreakpointRule* Rule) const
{
	float Result = FMath::IsFinite(ProjectDPIScale) && ProjectDPIScale > 0.0f ? ProjectDPIScale : 1.0f;
	if (Rule == nullptr)
	{
		return Result;
	}

	if (Rule->ScaleMode == EDPIBreakpointScaleMode::Fixed)
	{
		Result = Rule->TargetUIScale;
	}
	else if (Rule->ScaleMode == EDPIBreakpointScaleMode::Curve)
	{
		float AxisValue = FMath::Min(ViewportSize.X, ViewportSize.Y);
		switch (Rule->ScaleAxis)
		{
		case EDPIBreakpointScaleAxis::LongSide:
			AxisValue = FMath::Max(ViewportSize.X, ViewportSize.Y);
			break;
		case EDPIBreakpointScaleAxis::ScreenWidth:
			AxisValue = ViewportSize.X;
			break;
		case EDPIBreakpointScaleAxis::ScreenHeight:
			AxisValue = ViewportSize.Y;
			break;
		default:
			break;
		}
		Result = Rule->ScaleCurve.GetRichCurveConst()->Eval(AxisValue, 1.0f);
	}

	if (!FMath::IsFinite(Result))
	{
		Result = 1.0f;
	}
	return FMath::Max(Result, UE_SMALL_NUMBER);
}

void UDPIScalerWidget::SetDPIRules(const TArray<FDPIBreakpointRule>& InDPIRules)
{
	DPIRules = InDPIRules;
	InvalidateLayoutAndVolatility();
}

void UDPIScalerWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	InvalidateLayoutAndVolatility();
}

TSharedRef<SWidget> UDPIScalerWidget::RebuildWidget()
{
	SAssignNew(DPIScaler, SDPIScaler).DPIScale_UObject(this, &UDPIScalerWidget::GetDPIScale);
	if (GetChildrenCount() > 0)
	{
		DPIScaler->SetContent(GetContentSlot()->Content ? GetContentSlot()->Content->TakeWidget() : SNullWidget::NullWidget);
	}
	return DPIScaler.ToSharedRef();
}

void UDPIScalerWidget::OnSlotAdded(UPanelSlot* InSlot)
{
	if (DPIScaler.IsValid())
	{
		DPIScaler->SetContent(InSlot->Content ? InSlot->Content->TakeWidget() : SNullWidget::NullWidget);
	}
}

void UDPIScalerWidget::OnSlotRemoved(UPanelSlot* InSlot)
{
	if (DPIScaler.IsValid())
	{
		DPIScaler->SetContent(SNullWidget::NullWidget);
	}
}

void UDPIScalerWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	DPIScaler.Reset();
}
#if WITH_EDITOR
void UDPIScalerWidget::OnDesignerChanged(const FDesignerChangedEventArgs& EventArgs)
{
	Super::OnDesignerChanged(EventArgs);
	bScreenPreview = EventArgs.bScreenPreview;
	DesignerSize = EventArgs.bScreenPreview ? EventArgs.Size : FVector2D::ZeroVector;
	DesignerDpi = EventArgs.DpiScale;
	InvalidateLayoutAndVolatility();
}

void UDPIScalerWidget::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.MemberProperty != nullptr
		&& PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UDPIScalerWidget, DPIRules)
		&& (PropertyChangedEvent.ChangeType & EPropertyChangeType::ArrayAdd) != 0)
	{
		const int32 RuleIndex = PropertyChangedEvent.GetArrayIndex(GET_MEMBER_NAME_CHECKED(UDPIScalerWidget, DPIRules).ToString());
		if (DPIRules.IsValidIndex(RuleIndex))
		{
			DPIRules[RuleIndex].Name = FName(*FString::Printf(TEXT("Rule %d"), RuleIndex + 1));
		}
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
	InvalidateLayoutAndVolatility();
}
#endif
