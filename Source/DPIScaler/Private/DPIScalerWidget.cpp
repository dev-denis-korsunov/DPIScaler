#include "DPIScalerWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Widgets/Layout/SDPIScaler.h"

FDPIMediaQuery::FDPIMediaQuery()
	: BlendType(EDPIMediaQueryBlendType::Override), ScreenMinWidth(0), ScreenMaxWidth(0), ScreenMinHeight(0), ScreenMaxHeight(0)
	, ScreenMinAspectRatio(0.75f), ScreenMaxAspectRatio(1.33f), DPIScaleOverride(0.0f), MinDPIScale(0.0f), MaxDPIScale(1.0f)
	, CurveRule(EDPICurveRuleType::ByShortSide), SnapDPIScaleGrid(0.1f)
	, bOverrideScreenMinWidth(false), bOverrideScreenMaxWidth(false), bOverrideScreenMinHeight(false), bOverrideScreenMaxHeight(false)
	, bOverrideScreenMinAspectRatio(false), bOverrideScreenMaxAspectRatio(false), bOverrideDPIScaleOverride(false)
	, bOverrideMinDPIScale(false), bOverrideMaxDPIScale(false), bOverrideCurve(false), bSnapDPIToGrid(false)
{
	DPICurve.GetRichCurve()->AddKey(0.0f, 0.0f);
	DPICurve.GetRichCurve()->AddKey(1080.0f, 1.0f);
}

UDPIScalerWidget::UDPIScalerWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bIsVariable = false;
	Visibility = ESlateVisibility::SelfHitTestInvisible;
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
		if (Cache.ViewportSize == ViewportSize) return Cache.DPIScale;
	}
	if (ViewportSize.X <= 0 || ViewportSize.Y <= 0) return 1.0f;

	float ParentDesiredScale = 1.0f;
	bool bUseParentScale = false;
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
	if (IsValid(ParentScaler))
	{
		if (ParentScaler->Cache.ViewportSize != ViewportSize) return 1.0f;
		ParentDesiredScale = ParentScaler->Cache.DesiredScale;
		bUseParentScale = true;
	}

	float DesiredScale = bUseParentScale ? ParentDesiredScale : ApplicationScale;
	for (const FDPIMediaQuery& Query : MediaQueries)
	{
		if (!(Query.bOverrideDPIScaleOverride || Query.bOverrideMinDPIScale || Query.bOverrideMaxDPIScale || Query.bOverrideCurve || Query.bSnapDPIToGrid)) continue;
		const FIntRect AllowedRegion(Query.bOverrideScreenMinWidth ? Query.ScreenMinWidth : 0, Query.bOverrideScreenMinHeight ? Query.ScreenMinHeight : 0, (Query.bOverrideScreenMaxWidth ? Query.ScreenMaxWidth : ViewportSize.X) + 1, (Query.bOverrideScreenMaxHeight ? Query.ScreenMaxHeight : ViewportSize.Y) + 1);
		if (!AllowedRegion.Contains(ViewportSize)) continue;
		const float AspectRatio = static_cast<float>(ViewportSize.X) / static_cast<float>(ViewportSize.Y);
		if (!FMath::IsWithin(AspectRatio, Query.bOverrideScreenMinAspectRatio ? Query.ScreenMinAspectRatio : KINDA_SMALL_NUMBER, Query.bOverrideScreenMaxAspectRatio ? Query.ScreenMaxAspectRatio : BIG_NUMBER)) continue;
		float QueryScale = DesiredScale;
		if (Query.bOverrideCurve)
		{
			float CurveTime = ViewportSize.X;
			switch (Query.CurveRule) { case EDPICurveRuleType::ByScreenHeight: CurveTime = ViewportSize.Y; break; case EDPICurveRuleType::ByShortSide: CurveTime = FMath::Min(ViewportSize.X, ViewportSize.Y); break; case EDPICurveRuleType::ByLongSide: CurveTime = FMath::Max(ViewportSize.X, ViewportSize.Y); break; default: break; }
			QueryScale = Query.DPICurve.GetRichCurveConst()->Eval(CurveTime);
		}
		if (Query.bOverrideDPIScaleOverride) QueryScale = Query.DPIScaleOverride;
		if (Query.bOverrideMinDPIScale) QueryScale = FMath::Max(Query.MinDPIScale, QueryScale);
		if (Query.bOverrideMaxDPIScale) QueryScale = FMath::Min(Query.MaxDPIScale, QueryScale);
		if (Query.bSnapDPIToGrid && !FMath::IsNearlyZero(Query.SnapDPIScaleGrid)) QueryScale = FMath::GridSnap(QueryScale, Query.SnapDPIScaleGrid);
		switch (Query.BlendType) { case EDPIMediaQueryBlendType::Override: DesiredScale = QueryScale; break; case EDPIMediaQueryBlendType::Min: DesiredScale = FMath::Min(DesiredScale, QueryScale); break; case EDPIMediaQueryBlendType::Max: DesiredScale = FMath::Max(DesiredScale, QueryScale); break; }
	}
	Cache.ViewportSize = ViewportSize;
	Cache.DesiredScale = DesiredScale;
	Cache.SourceScale = bUseParentScale ? ParentDesiredScale : ApplicationScale;
	Cache.DPIScale = DesiredScale / FMath::Max(Cache.SourceScale, KINDA_SMALL_NUMBER);
	return Cache.DPIScale;
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
}
#endif
