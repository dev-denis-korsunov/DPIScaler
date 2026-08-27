#include "DPIScalerWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "UObject/UnrealType.h"
#include "Widgets/Layout/SDPIScaler.h"

FDPIBreakpointRule::FDPIBreakpointRule()
	: Name(TEXT("Default")), bEnabled(true), Priority(0), Orientation(EDPIBreakpointOrientation::Any), SizeMetric(EDPIBreakpointSizeMetric::BothDimensions)
	, MinShortSide(0), MaxShortSide(0), MinWidth(0), MaxWidth(0), MinHeight(0), MaxHeight(0)
	, MinAspectRatio(0.0f), MaxAspectRatio(0.0f), ScaleMode(EDPIBreakpointScaleMode::UseProjectDPI)
	, TargetUIScale(1.0f), CurveAxis(EDPIBreakpointCurveAxis::ShortSide), MinScale(0.0f), MaxScale(0.0f), SnapStep(0.0f)
{
	ScaleCurve.GetRichCurve()->AddKey(0.0f, 1.0f);
	ScaleCurve.GetRichCurve()->AddKey(1080.0f, 1.0f);
}

UDPIScalerWidget::UDPIScalerWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bIsVariable = false;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

float UDPIScalerWidget::GetDPIScale() const
{
#if WITH_EDITORONLY_DATA
	if (IsDesignTime() && (bDesignerMute || !bScreenPreview)) return 1.0f;
#endif
	float ApplicationScale = 1.0f;
	FIntPoint ViewportSize = FIntPoint::ZeroValue;
#if WITH_EDITOR
	if (DesignerDpi.IsSet() && DesignerSize.IsSet()) { ApplicationScale = DesignerDpi.GetValue(); ViewportSize = DesignerSize.GetValue().IntPoint(); }
	else
#endif
	{
		ApplicationScale = UWidgetLayoutLibrary::GetViewportScale(GetWorld());
		ViewportSize = UWidgetLayoutLibrary::GetViewportSize(GetWorld()).IntPoint();
	}
	if (ViewportSize.X <= 0 || ViewportSize.Y <= 0) return 1.0f;

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

const UDPIScalerWidget* UDPIScalerWidget::FindParentDPIScaler() const
{
	const UDPIScalerWidget* ParentScaler = nullptr;
	const UPanelWidget* ParentWidget = GetParent();
	const UWidgetTree* OuterWidgetTree = Cast<UWidgetTree>(GetOuter());
	for (;;)
	{
		if (IsValid(ParentWidget))
		{
			if (ParentWidget->IsA<UDPIScalerWidget>()) { ParentScaler = Cast<UDPIScalerWidget>(ParentWidget); break; }
			ParentWidget = ParentWidget->GetParent();
			continue;
		}
		if (!IsValid(OuterWidgetTree)) break;
		const UWidget* WidgetTreeOuter = Cast<UWidget>(OuterWidgetTree->GetOuter());
		if (!IsValid(WidgetTreeOuter)) break;
		ParentWidget = WidgetTreeOuter->GetParent();
		if (!IsValid(ParentWidget)) break;
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
	if (ViewportSize.X <= 0 || ViewportSize.Y <= 0) return nullptr;
	const float AspectRatio = static_cast<float>(ViewportSize.X) / static_cast<float>(ViewportSize.Y);
	const FDPIBreakpointRule* ActiveRule = nullptr;
	for (const FDPIBreakpointRule& Rule : DPIRules)
	{
		if (!Rule.bEnabled || (ActiveRule != nullptr && Rule.Priority <= ActiveRule->Priority)) continue;
		if (Rule.Orientation == EDPIBreakpointOrientation::Portrait && ViewportSize.X >= ViewportSize.Y) continue;
		if (Rule.Orientation == EDPIBreakpointOrientation::Landscape && ViewportSize.X < ViewportSize.Y) continue;
		int32 MetricValue = 0;
		switch (Rule.SizeMetric)
		{
		case EDPIBreakpointSizeMetric::BothDimensions:
			if (Rule.MinShortSide > 0 && (ViewportSize.X < Rule.MinShortSide || ViewportSize.Y < Rule.MinShortSide)) continue;
			if (Rule.MaxShortSide > 0 && (ViewportSize.X > Rule.MaxShortSide || ViewportSize.Y > Rule.MaxShortSide)) continue;
			break;
		case EDPIBreakpointSizeMetric::LongSide:
			MetricValue = FMath::Max(ViewportSize.X, ViewportSize.Y);
			break;
		case EDPIBreakpointSizeMetric::Width:
			MetricValue = ViewportSize.X;
			break;
		case EDPIBreakpointSizeMetric::Height:
			MetricValue = ViewportSize.Y;
			break;
		default:
			MetricValue = FMath::Min(ViewportSize.X, ViewportSize.Y);
			break;
		}
		if (Rule.SizeMetric != EDPIBreakpointSizeMetric::BothDimensions)
		{
			if (Rule.MinShortSide > 0 && MetricValue < Rule.MinShortSide) continue;
			if (Rule.MaxShortSide > 0 && MetricValue > Rule.MaxShortSide) continue;
		}
		if (Rule.MinWidth > 0 && ViewportSize.X < Rule.MinWidth) continue;
		if (Rule.MaxWidth > 0 && ViewportSize.X > Rule.MaxWidth) continue;
		if (Rule.MinHeight > 0 && ViewportSize.Y < Rule.MinHeight) continue;
		if (Rule.MaxHeight > 0 && ViewportSize.Y > Rule.MaxHeight) continue;
		if (Rule.MinAspectRatio > 0.0f && AspectRatio < Rule.MinAspectRatio) continue;
		if (Rule.MaxAspectRatio > 0.0f && AspectRatio > Rule.MaxAspectRatio) continue;
		ActiveRule = &Rule;
	}
	return ActiveRule;
}

float UDPIScalerWidget::ResolveTargetUIScale(float ProjectDPIScale, const FIntPoint& ViewportSize, const FDPIBreakpointRule* Rule) const
{
	float Result = FMath::IsFinite(ProjectDPIScale) && ProjectDPIScale > 0.0f ? ProjectDPIScale : 1.0f;
	if (Rule == nullptr) return Result;

	if (Rule->ScaleMode == EDPIBreakpointScaleMode::Fixed)
	{
		Result = Rule->TargetUIScale;
	}
	else if (Rule->ScaleMode == EDPIBreakpointScaleMode::Curve)
	{
		float CurveTime = FMath::Min(ViewportSize.X, ViewportSize.Y);
		switch (Rule->CurveAxis)
		{
		case EDPIBreakpointCurveAxis::LongSide: CurveTime = FMath::Max(ViewportSize.X, ViewportSize.Y); break;
		case EDPIBreakpointCurveAxis::ScreenWidth: CurveTime = ViewportSize.X; break;
		case EDPIBreakpointCurveAxis::ScreenHeight: CurveTime = ViewportSize.Y; break;
		default: break;
		}
		Result = Rule->ScaleCurve.GetRichCurveConst()->Eval(CurveTime, 1.0f);
	}

	if (!FMath::IsFinite(Result)) Result = 1.0f;
	const float LowerLimit = Rule->MinScale > 0.0f ? Rule->MinScale : UE_SMALL_NUMBER;
	const float UpperLimit = Rule->MaxScale > 0.0f ? FMath::Max(Rule->MaxScale, LowerLimit) : BIG_NUMBER;
	Result = FMath::Clamp(Result, LowerLimit, UpperLimit);
	if (Rule->SnapStep > 0.0f) Result = FMath::GridSnap(Result, Rule->SnapStep);
	return FMath::Clamp(Result, LowerLimit, UpperLimit);
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
	if (GetChildrenCount() > 0) DPIScaler->SetContent(GetContentSlot()->Content ? GetContentSlot()->Content->TakeWidget() : SNullWidget::NullWidget);
	return DPIScaler.ToSharedRef();
}
void UDPIScalerWidget::OnSlotAdded(UPanelSlot* InSlot) { if (DPIScaler.IsValid()) DPIScaler->SetContent(InSlot->Content ? InSlot->Content->TakeWidget() : SNullWidget::NullWidget); }
void UDPIScalerWidget::OnSlotRemoved(UPanelSlot* InSlot) { if (DPIScaler.IsValid()) DPIScaler->SetContent(SNullWidget::NullWidget); }
void UDPIScalerWidget::ReleaseSlateResources(bool bReleaseChildren) { Super::ReleaseSlateResources(bReleaseChildren); DPIScaler.Reset(); }
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
	Super::PostEditChangeProperty(PropertyChangedEvent);
	InvalidateLayoutAndVolatility();
}
#endif
