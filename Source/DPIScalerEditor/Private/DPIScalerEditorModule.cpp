#include "Modules/ModuleManager.h"

#include "DPIScalerWidget.h"
#include "DPIScalerRuleCustomization.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "DesignerExtension.h"
#include "IUMGDesigner.h"
#include "IHasDesignerExtensibility.h"
#include "Rendering/DrawElements.h"
#include "PropertyEditorModule.h"
#include "Styling/CoreStyle.h"
#include "UMGEditorModule.h"
#include "WidgetReference.h"

#define LOCTEXT_NAMESPACE "DPIScalerEditor"

namespace UE::DPIScalerEditor::Private
{
	static constexpr float RulerThickness = 18.0f;
	static constexpr float RulerGap = 4.0f;
	static constexpr float InformationTextOffset = 38.0f;

	static FLinearColor GetRuleColor(int32 Index, float Saturation, float Brightness)
	{
		const float Hue = FMath::Frac(Index * 0.12f);
		return FLinearColor::MakeFromHSV8(
			static_cast<uint8>(FMath::RoundToInt(Hue * 255.0f)),
			static_cast<uint8>(FMath::RoundToInt(Saturation * 255.0f)),
			static_cast<uint8>(FMath::RoundToInt(Brightness * 255.0f)));
	}

	static int32 GetRulerMaximum(const FVector2D& ViewportSize, const TArray<FDPIBreakpointRule>& Rules, bool bWidth)
	{
		const float CurrentSize = bWidth ? ViewportSize.X : ViewportSize.Y;
		int32 Maximum = FMath::Max(1, FMath::CeilToInt(CurrentSize));
		for (const FDPIBreakpointRule& Rule : Rules)
		{
			const bool bUsesAxis = bWidth ? Rule.bUseWidthBreakpoint : Rule.bUseHeightBreakpoint;
			if (bUsesAxis) Maximum = FMath::Max(Maximum, bWidth ? Rule.WidthBreakpoint : Rule.HeightBreakpoint);

			if (bWidth && Rule.bUseMaxAspectRatio)
			{
				Maximum = FMath::Max(Maximum, FMath::CeilToInt(Rule.MaxAspectRatio * ViewportSize.Y));
			}
			else if (!bWidth && Rule.bUseMinAspectRatio && Rule.MinAspectRatio > 0.0f)
			{
				Maximum = FMath::Max(Maximum, FMath::CeilToInt(ViewportSize.X / Rule.MinAspectRatio));
			}
		}
		return Maximum;
	}

	static bool GetRuleAxisRange(const FDPIBreakpointRule& Rule, const FVector2D& ViewportSize, int32 RulerMaximum, bool bWidth, float& OutMinimum, float& OutMaximum)
	{
		const float AxisOtherSize = bWidth ? ViewportSize.Y : ViewportSize.X;
		const bool bUsesAxis = bWidth ? Rule.bUseWidthBreakpoint : Rule.bUseHeightBreakpoint;
		const bool bUsesOtherAxis = bWidth ? Rule.bUseHeightBreakpoint : Rule.bUseWidthBreakpoint;
		const int32 AxisBreakpoint = bWidth ? Rule.WidthBreakpoint : Rule.HeightBreakpoint;
		const int32 OtherAxisBreakpoint = bWidth ? Rule.HeightBreakpoint : Rule.WidthBreakpoint;
		if (bUsesOtherAxis && AxisOtherSize > OtherAxisBreakpoint) return false;

		OutMinimum = 0.0f;
		OutMaximum = RulerMaximum;
		if (bUsesAxis)
		{
			OutMaximum = FMath::Min(OutMaximum, static_cast<float>(AxisBreakpoint));
		}

		if (bWidth)
		{
			if (Rule.bUseMinAspectRatio) OutMinimum = FMath::Max(OutMinimum, Rule.MinAspectRatio * ViewportSize.Y);
			if (Rule.bUseMaxAspectRatio) OutMaximum = FMath::Min(OutMaximum, Rule.MaxAspectRatio * ViewportSize.Y);
		}
		else
		{
			if (Rule.bUseMaxAspectRatio && Rule.MaxAspectRatio > 0.0f) OutMinimum = FMath::Max(OutMinimum, ViewportSize.X / Rule.MaxAspectRatio);
			if (Rule.bUseMinAspectRatio && Rule.MinAspectRatio > 0.0f) OutMaximum = FMath::Min(OutMaximum, ViewportSize.X / Rule.MinAspectRatio);
		}

		if (OutMaximum < OutMinimum || OutMaximum < 0.0f || OutMinimum > RulerMaximum) return false;
		OutMinimum = FMath::Clamp(OutMinimum, 0.0f, static_cast<float>(RulerMaximum));
		OutMaximum = FMath::Clamp(OutMaximum, 0.0f, static_cast<float>(RulerMaximum));
		return OutMaximum >= OutMinimum;
	}

	static void DrawRuler(const UDPIScalerWidget& Scaler, const FDPIBreakpointRule* ActiveRule, const FGeometry& AllottedGeometry, const FVector2D& Origin, float UnitsToSlate, const FVector2D& ViewportSize, bool bWidth, FSlateWindowElementList& DrawElements, int32 LayerId)
	{
		const float CurrentViewportSize = bWidth ? ViewportSize.X : ViewportSize.Y;
		const int32 Maximum = GetRulerMaximum(ViewportSize, Scaler.DPIRules, bWidth);
		const float RulerLength = Maximum * UnitsToSlate;
		const FVector2D RulerPosition = bWidth ? FVector2D(Origin.X, Origin.Y - RulerThickness - RulerGap) : FVector2D(Origin.X - RulerThickness - RulerGap, Origin.Y);
		static const FSlateRoundedBoxBrush RangeFillBrush(FLinearColor::White, 8.0f);

		TArray<float> RangeBoundaries = { 0.0f, static_cast<float>(Maximum) };
		for (const FDPIBreakpointRule& Rule : Scaler.DPIRules)
		{
			float RangeMinimum = 0.0f;
			float RangeMaximum = 0.0f;
			if (!GetRuleAxisRange(Rule, ViewportSize, Maximum, bWidth, RangeMinimum, RangeMaximum)) continue;
			RangeBoundaries.Add(RangeMinimum);
			RangeBoundaries.Add(RangeMaximum);
		}
		RangeBoundaries.Sort();
		for (int32 BoundaryIndex = RangeBoundaries.Num() - 1; BoundaryIndex > 0; --BoundaryIndex)
		{
			if (FMath::IsNearlyEqual(RangeBoundaries[BoundaryIndex], RangeBoundaries[BoundaryIndex - 1]))
			{
				RangeBoundaries.RemoveAt(BoundaryIndex);
			}
		}
		for (int32 BoundaryIndex = 0; BoundaryIndex + 1 < RangeBoundaries.Num(); ++BoundaryIndex)
		{
			const float RangeMinimum = RangeBoundaries[BoundaryIndex];
			const float RangeMaximum = RangeBoundaries[BoundaryIndex + 1];
			if (RangeMaximum <= RangeMinimum) continue;
			const int32 Sample = FMath::RoundToInt((RangeMinimum + RangeMaximum) * 0.5f);
			const FIntPoint SampleViewport = bWidth
				? FIntPoint(Sample, FMath::RoundToInt(ViewportSize.Y))
				: FIntPoint(FMath::RoundToInt(ViewportSize.X), Sample);
			const FDPIBreakpointRule* SegmentRule = Scaler.FindActiveRule(SampleViewport);
			if (SegmentRule == nullptr) continue;
			const int32 RuleIndex = Scaler.DPIRules.IndexOfByPredicate([SegmentRule](const FDPIBreakpointRule& Rule) { return &Rule == SegmentRule; });
			if (RuleIndex == INDEX_NONE) continue;

			const float Start = RangeMinimum * UnitsToSlate;
			const float Length = FMath::Max((RangeMaximum - RangeMinimum) * UnitsToSlate, 1.0f);
			const FVector2D RangePosition = bWidth ? RulerPosition + FVector2D(Start, 0.0f) : RulerPosition + FVector2D(0.0f, Start);
			const FVector2D RangeSize = bWidth ? FVector2D(Length, RulerThickness) : FVector2D(RulerThickness, Length);
			const FLinearColor DisplayColor = SegmentRule == ActiveRule
				? GetRuleColor(RuleIndex, 0.60f, 0.70f)
				: GetRuleColor(RuleIndex, 0.20f, 0.10f);
			FSlateDrawElement::MakeBox(DrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(RangeSize, FSlateLayoutTransform(RangePosition)), &RangeFillBrush, ESlateDrawEffect::None, DisplayColor);
		}

		const int32 TickStep = Maximum <= 800 ? 100 : Maximum <= 2000 ? 250 : 500;
		for (int32 Value = 0; Value <= Maximum; Value += TickStep)
		{
			const float Position = Value * UnitsToSlate;
			const FVector2D LineStart = bWidth ? RulerPosition + FVector2D(Position, RulerThickness - 7.0f) : RulerPosition + FVector2D(RulerThickness - 7.0f, Position);
			const FVector2D LineEnd = bWidth ? RulerPosition + FVector2D(Position, RulerThickness) : RulerPosition + FVector2D(RulerThickness, Position);
			const TArray<FVector2D> Points = { LineStart, LineEnd };
			FSlateDrawElement::MakeLines(DrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, FLinearColor::White.CopyWithNewOpacity(0.45f), true, 1.0f);
			if (bWidth)
			{
				const float LabelOffsetY = (RulerThickness - 11.0f) * 0.5f;
				FSlateDrawElement::MakeText(DrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(FVector2D(48.0f, 11.0f), FSlateLayoutTransform(RulerPosition + FVector2D(Position + 3.0f, LabelOffsetY))), FText::AsNumber(Value), FCoreStyle::GetDefaultFontStyle("Regular", 8), ESlateDrawEffect::None, FLinearColor::White.CopyWithNewOpacity(0.8f));
			}
		}
	}

	static void DrawActiveRuleStatus(const UDPIScalerWidget& Scaler, const UDPIScalerWidget* PreviewScaler, const FGeometry& AllottedGeometry, const FVector2D& Origin, const FVector2D& Size, const FVector2D& ViewportSize, FSlateWindowElementList& DrawElements, int32 LayerId)
	{
		if (Scaler.bDesignerMute)
		{
			const FVector2D StatusPosition = Origin + FVector2D(70.0f, -InformationTextOffset);
			const FVector2D StatusSize(FMath::Max(Size.X - 76.0f, 120.0f), 12.0f);
			FSlateDrawElement::MakeText(DrawElements, LayerId, AllottedGeometry.ToPaintGeometry(StatusSize, FSlateLayoutTransform(StatusPosition)), FText::FromString(TEXT("Designer Mute")), FCoreStyle::GetDefaultFontStyle("Bold", 8), ESlateDrawEffect::None, FLinearColor::White);
			return;
		}
		const FIntPoint IntViewportSize = ViewportSize.IntPoint();
		const FDPIBreakpointRule* ActiveRule = Scaler.FindActiveRule(IntViewportSize);
		const float ProjectScale = PreviewScaler != nullptr && PreviewScaler->DesignerDpi.IsSet() ? PreviewScaler->DesignerDpi.GetValue() : 1.0f;
		const float FinalScale = Scaler.ResolveTargetUIScale(ProjectScale, IntViewportSize, ActiveRule);
		const FString RuleName = ActiveRule != nullptr ? ActiveRule->Name.ToString() : TEXT("Project DPI");
		const FText Status = FText::FromString(FString::Printf(TEXT("Active: %s  •  Scale %.2f"), *RuleName, FinalScale));
		const FVector2D StatusPosition = Origin + FVector2D(70.0f, -InformationTextOffset);
		const FVector2D StatusSize(FMath::Max(Size.X - 76.0f, 120.0f), 12.0f);
		const int32 ActiveRuleIndex = Scaler.DPIRules.IndexOfByPredicate([ActiveRule](const FDPIBreakpointRule& Rule) { return &Rule == ActiveRule; });
		const FLinearColor ActiveRuleColor = ActiveRuleIndex != INDEX_NONE ? GetRuleColor(ActiveRuleIndex, 0.60f, 0.70f) : FLinearColor(0.55f, 0.55f, 0.55f);
		static const FSlateRoundedBoxBrush ActiveRuleIndicatorBrush(FLinearColor::White, 3.0f);
		const FVector2D IndicatorPosition = StatusPosition + FVector2D(-10.0f, 3.0f);
		FSlateDrawElement::MakeBox(DrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(6.0f, 6.0f), FSlateLayoutTransform(IndicatorPosition)), &ActiveRuleIndicatorBrush, ESlateDrawEffect::None, ActiveRuleColor);
		FSlateDrawElement::MakeText(DrawElements, LayerId, AllottedGeometry.ToPaintGeometry(StatusSize, FSlateLayoutTransform(StatusPosition)), Status, FCoreStyle::GetDefaultFontStyle("Bold", 8), ESlateDrawEffect::None, FLinearColor::White);
	}

	class FDPIScalerDesignerExtension final : public FDesignerExtension
	{
	public:
		virtual bool CanExtendSelection(const TArray<FWidgetReference>& Selection) const override
		{
			return Selection.Num() == 1 && Cast<UDPIScalerWidget>(Selection[0].GetTemplate()) != nullptr;
		}

		virtual void Paint(const TSet<FWidgetReference>& Selection, const FGeometry& AllottedGeometry, const FSlateRect&, FSlateWindowElementList& DrawElements, int32 LayerId) const override
		{
			if (Selection.Num() != 1 || Designer == nullptr) return;
			const FWidgetReference& Selected = *Selection.CreateConstIterator();
			const UDPIScalerWidget* Scaler = Cast<UDPIScalerWidget>(Selected.GetTemplate());
			const UDPIScalerWidget* PreviewScaler = Cast<UDPIScalerWidget>(Selected.GetPreview());
			FGeometry WidgetGeometry;
			if (Scaler == nullptr || !Designer->GetWidgetGeometry(Selected, WidgetGeometry)) return;
			WidgetGeometry = Designer->MakeGeometryWindowLocal(WidgetGeometry);
			const FVector2D Origin = AllottedGeometry.AbsoluteToLocal(WidgetGeometry.LocalToAbsolute(FVector2D::ZeroVector));
			const FVector2D End = AllottedGeometry.AbsoluteToLocal(WidgetGeometry.LocalToAbsolute(WidgetGeometry.GetLocalSize()));
			const FVector2D Size = End - Origin;
			FVector2D ViewportSize = WidgetGeometry.GetLocalSize();
			if (PreviewScaler != nullptr && PreviewScaler->DesignerSize.IsSet() && PreviewScaler->DesignerSize.GetValue().GetMin() > 0.0f)
			{
				ViewportSize = PreviewScaler->DesignerSize.GetValue();
			}
			const float UnitsToSlate = FMath::Min(
				FMath::Abs(Size.X) / FMath::Max(ViewportSize.X, 1.0f),
				FMath::Abs(Size.Y) / FMath::Max(ViewportSize.Y, 1.0f));
			if (UnitsToSlate <= KINDA_SMALL_NUMBER) return;
			const FDPIBreakpointRule* ActiveRule = Scaler->FindActiveRule(ViewportSize.IntPoint());
			DrawRuler(*Scaler, ActiveRule, AllottedGeometry, Origin, UnitsToSlate, ViewportSize, true, DrawElements, LayerId + 20);
			DrawRuler(*Scaler, ActiveRule, AllottedGeometry, Origin, UnitsToSlate, ViewportSize, false, DrawElements, LayerId + 30);
			DrawActiveRuleStatus(*Scaler, PreviewScaler, AllottedGeometry, Origin, Size, ViewportSize, DrawElements, LayerId + 40);
		}
	};

	class FDPIScalerDesignerExtensionFactory final : public IDesignerExtensionFactory
	{
	public:
		virtual TSharedRef<FDesignerExtension> CreateDesignerExtension() const override
		{
			return MakeShared<FDPIScalerDesignerExtension>();
		}
	};
}

class FDPIScalerEditorModule final : public IModuleInterface
{
	virtual void StartupModule() override
	{
		ExtensionFactory = MakeShared<UE::DPIScalerEditor::Private::FDPIScalerDesignerExtensionFactory>();
		FModuleManager::LoadModuleChecked<IUMGEditorModule>("UMGEditor").GetDesignerExtensibilityManager()->AddDesignerExtensionFactory(ExtensionFactory.ToSharedRef());
		FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditor.RegisterCustomPropertyTypeLayout(FDPIBreakpointRule::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FDPIScalerRuleCustomization::MakeInstance));
		PropertyEditor.NotifyCustomizationModuleChanged();
	}

	virtual void ShutdownModule() override
	{
		if (ExtensionFactory.IsValid())
		{
			if (IUMGEditorModule* UMGEditor = FModuleManager::GetModulePtr<IUMGEditorModule>("UMGEditor"))
			{
				UMGEditor->GetDesignerExtensibilityManager()->RemoveDesignerExtensionFactory(ExtensionFactory.ToSharedRef());
			}
		}
		ExtensionFactory.Reset();
		if (FPropertyEditorModule* PropertyEditor = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor"))
		{
			PropertyEditor->UnregisterCustomPropertyTypeLayout(FDPIBreakpointRule::StaticStruct()->GetFName());
			PropertyEditor->NotifyCustomizationModuleChanged();
		}
	}

	TSharedPtr<IDesignerExtensionFactory> ExtensionFactory;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDPIScalerEditorModule, DPIScalerEditor)
