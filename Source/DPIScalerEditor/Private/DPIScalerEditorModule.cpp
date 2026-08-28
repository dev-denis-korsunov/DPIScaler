#include "Modules/ModuleManager.h"

#include "DPIScalerWidget.h"
#include "DPIScalerRuleCustomization.h"
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
	static constexpr float RulerThickness = 36.0f;
	static constexpr float RulerGap = 4.0f;

	static FLinearColor GetRuleColor(int32 Index)
	{
		static const FLinearColor Colors[] =
		{
			FLinearColor(0.62f, 0.50f, 0.96f),
			FLinearColor(0.31f, 0.65f, 0.96f),
			FLinearColor(0.28f, 0.86f, 0.78f),
			FLinearColor(0.80f, 0.88f, 0.34f),
		};
		return Colors[Index % UE_ARRAY_COUNT(Colors)];
	}

	static FLinearColor GetMutedRuleColor(const FLinearColor& Color)
	{
		constexpr float MutedGray = 0.22f;
		constexpr float MutedAmount = 0.62f;
		return FLinearColor(
			FMath::Lerp(Color.R, MutedGray, MutedAmount),
			FMath::Lerp(Color.G, MutedGray, MutedAmount),
			FMath::Lerp(Color.B, MutedGray, MutedAmount),
			1.0f);
	}

	static int32 GetRulerMaximum(float CurrentSize, const TArray<FDPIBreakpointRule>& Rules, bool bWidth)
	{
		int32 Maximum = FMath::Max(1, FMath::CeilToInt(CurrentSize));
		for (const FDPIBreakpointRule& Rule : Rules)
		{
			const bool bUsesAxis = bWidth ? Rule.bUseWidthBreakpoint : Rule.bUseHeightBreakpoint;
			if (bUsesAxis) Maximum = FMath::Max(Maximum, bWidth ? Rule.WidthBreakpoint : Rule.HeightBreakpoint);
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

	static void DrawRuler(const UDPIScalerWidget& Scaler, const FDPIBreakpointRule* ActiveRule, const FVector2D& Origin, float UnitsToSlate, const FVector2D& ViewportSize, bool bWidth, FSlateWindowElementList& DrawElements, int32 LayerId)
	{
		const float CurrentViewportSize = bWidth ? ViewportSize.X : ViewportSize.Y;
		const int32 Maximum = GetRulerMaximum(CurrentViewportSize, Scaler.DPIRules, bWidth);
		const float RulerLength = Maximum * UnitsToSlate;
		const FVector2D RulerPosition = bWidth ? FVector2D(Origin.X, Origin.Y - RulerThickness - RulerGap) : FVector2D(Origin.X - RulerThickness - RulerGap, Origin.Y);
		const FVector2D RulerSize = bWidth ? FVector2D(RulerLength, RulerThickness) : FVector2D(RulerThickness, RulerLength);
		const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
		FSlateDrawElement::MakeBox(DrawElements, LayerId, FPaintGeometry(RulerPosition, RulerSize, 1.0f), WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.045f, 0.055f, 0.075f, 0.94f));

		TArray<int32> RuleDrawOrder;
		for (int32 RuleIndex = 0; RuleIndex < Scaler.DPIRules.Num(); ++RuleIndex)
		{
			RuleDrawOrder.Add(RuleIndex);
		}
		RuleDrawOrder.Sort([&](int32 LeftIndex, int32 RightIndex)
		{
			const FDPIBreakpointRule& LeftRule = Scaler.DPIRules[LeftIndex];
			const FDPIBreakpointRule& RightRule = Scaler.DPIRules[RightIndex];
			const int32 LeftBreakpoint = bWidth ? (LeftRule.bUseWidthBreakpoint ? LeftRule.WidthBreakpoint : MIN_int32) : (LeftRule.bUseHeightBreakpoint ? LeftRule.HeightBreakpoint : MIN_int32);
			const int32 RightBreakpoint = bWidth ? (RightRule.bUseWidthBreakpoint ? RightRule.WidthBreakpoint : MIN_int32) : (RightRule.bUseHeightBreakpoint ? RightRule.HeightBreakpoint : MIN_int32);
			return LeftBreakpoint == RightBreakpoint ? LeftIndex < RightIndex : LeftBreakpoint < RightBreakpoint;
		});

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
			const FLinearColor RuleColor = GetRuleColor(RuleIndex);
			const FLinearColor DisplayColor = SegmentRule == ActiveRule ? RuleColor : GetMutedRuleColor(RuleColor);
			FSlateDrawElement::MakeBox(DrawElements, LayerId + 1, FPaintGeometry(RangePosition, RangeSize, 1.0f), WhiteBrush, ESlateDrawEffect::None, DisplayColor);
		}

		const int32 TickStep = Maximum <= 800 ? 100 : Maximum <= 2000 ? 250 : 500;
		for (int32 Value = 0; Value <= Maximum; Value += TickStep)
		{
			const float Position = Value * UnitsToSlate;
			const FVector2D LineStart = bWidth ? RulerPosition + FVector2D(Position, RulerThickness - 7.0f) : RulerPosition + FVector2D(RulerThickness - 7.0f, Position);
			const FVector2D LineEnd = bWidth ? RulerPosition + FVector2D(Position, RulerThickness) : RulerPosition + FVector2D(RulerThickness, Position);
			const TArray<FVector2D> Points = { LineStart, LineEnd };
			FSlateDrawElement::MakeLines(DrawElements, LayerId + 2, FPaintGeometry(), Points, ESlateDrawEffect::None, FLinearColor::White.CopyWithNewOpacity(0.45f), true, 1.0f);
			if (bWidth)
			{
				FSlateDrawElement::MakeText(DrawElements, LayerId + 3, FPaintGeometry(RulerPosition + FVector2D(Position + 3.0f, 14.0f), FVector2D(48.0f, 11.0f), 1.0f), FText::AsNumber(Value), FCoreStyle::GetDefaultFontStyle("Regular", 8), ESlateDrawEffect::None, FLinearColor::White.CopyWithNewOpacity(0.8f));
			}
		}

		for (int32 RuleIndex : RuleDrawOrder)
		{
			const FDPIBreakpointRule& Rule = Scaler.DPIRules[RuleIndex];
			const FLinearColor BaseColor = GetRuleColor(RuleIndex);
			const FLinearColor Color = &Rule == ActiveRule ? BaseColor : GetMutedRuleColor(BaseColor);
			auto DrawMarker = [&](int32 Value, const TCHAR* Axis)
			{
				if (Value <= 0) return;
				const float Position = FMath::Clamp(Value * UnitsToSlate, 0.0f, RulerLength);
				const FVector2D Start = bWidth ? RulerPosition + FVector2D(Position, 0.0f) : RulerPosition + FVector2D(0.0f, Position);
				const FVector2D End = bWidth ? RulerPosition + FVector2D(Position, RulerThickness) : RulerPosition + FVector2D(RulerThickness, Position);
				const TArray<FVector2D> Points = { Start, End };
				FSlateDrawElement::MakeLines(DrawElements, LayerId + 4, FPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, 1.5f);
				const FString Label = FString::Printf(TEXT("%s≤%d"), Axis, Value);
				const FVector2D MarkerLabelPosition = bWidth
					? RulerPosition + FVector2D(Position + 3.0f, 24.0f)
					: RulerPosition + FVector2D(2.0f, Position + 2.0f);
				FSlateDrawElement::MakeText(DrawElements, LayerId + 5, FPaintGeometry(MarkerLabelPosition, FVector2D(72.0f, 11.0f), 1.0f), FText::FromString(Label), FCoreStyle::GetDefaultFontStyle("Bold", 8), ESlateDrawEffect::None, Color);
			};
			const bool bUsesAxis = bWidth ? Rule.bUseWidthBreakpoint : Rule.bUseHeightBreakpoint;
			if (bUsesAxis)
			{
				const int32 Breakpoint = bWidth ? Rule.WidthBreakpoint : Rule.HeightBreakpoint;
				DrawMarker(Breakpoint, bWidth ? TEXT("W") : TEXT("H"));
			}
		}

		const FString SizeLabel = FString::Printf(TEXT("%s %d"), bWidth ? TEXT("W") : TEXT("H"), FMath::RoundToInt(CurrentViewportSize));
		const FVector2D LabelPosition = RulerPosition + FVector2D(3.0f, 2.0f);
		FSlateDrawElement::MakeText(DrawElements, LayerId + 6, FPaintGeometry(LabelPosition, FVector2D(64.0f, 11.0f), 1.0f), FText::FromString(SizeLabel), FCoreStyle::GetDefaultFontStyle("Bold", 8), ESlateDrawEffect::None, FLinearColor::White);
	}

	static void DrawActiveRuleStatus(const UDPIScalerWidget& Scaler, const UDPIScalerWidget* PreviewScaler, const FVector2D& Origin, const FVector2D& Size, const FVector2D& ViewportSize, FSlateWindowElementList& DrawElements, int32 LayerId)
	{
		if (Scaler.bDesignerMute)
		{
			const FVector2D StatusPosition = Origin + FVector2D(70.0f, -RulerThickness - RulerGap + 2.0f);
			const FVector2D StatusSize(FMath::Max(Size.X - 76.0f, 120.0f), 12.0f);
			FSlateDrawElement::MakeText(DrawElements, LayerId, FPaintGeometry(StatusPosition, StatusSize, 1.0f), FText::FromString(TEXT("Designer Mute")), FCoreStyle::GetDefaultFontStyle("Bold", 8), ESlateDrawEffect::None, FLinearColor::White);
			return;
		}
		const FIntPoint IntViewportSize = ViewportSize.IntPoint();
		const FDPIBreakpointRule* ActiveRule = Scaler.FindActiveRule(IntViewportSize);
		const float ProjectScale = PreviewScaler != nullptr && PreviewScaler->DesignerDpi.IsSet() ? PreviewScaler->DesignerDpi.GetValue() : 1.0f;
		const float FinalScale = Scaler.ResolveTargetUIScale(ProjectScale, IntViewportSize, ActiveRule);
		const FString RuleName = ActiveRule != nullptr ? ActiveRule->Name.ToString() : TEXT("Project DPI");
		const FText Status = FText::FromString(FString::Printf(TEXT("Active: %s  •  Scale %.2f"), *RuleName, FinalScale));
		const FVector2D StatusPosition = Origin + FVector2D(70.0f, -RulerThickness - RulerGap + 2.0f);
		const FVector2D StatusSize(FMath::Max(Size.X - 76.0f, 120.0f), 12.0f);
		FSlateDrawElement::MakeText(DrawElements, LayerId, FPaintGeometry(StatusPosition, StatusSize, 1.0f), Status, FCoreStyle::GetDefaultFontStyle("Bold", 8), ESlateDrawEffect::None, FLinearColor::White);
	}

	class FDPIScalerDesignerExtension final : public FDesignerExtension
	{
	public:
		virtual bool CanExtendSelection(const TArray<FWidgetReference>& Selection) const override
		{
			return Selection.Num() == 1 && Cast<UDPIScalerWidget>(Selection[0].GetTemplate()) != nullptr;
		}

		virtual void Paint(const TSet<FWidgetReference>& Selection, const FGeometry&, const FSlateRect&, FSlateWindowElementList& DrawElements, int32 LayerId) const override
		{
			if (Selection.Num() != 1 || Designer == nullptr) return;
			const FWidgetReference& Selected = *Selection.CreateConstIterator();
			const UDPIScalerWidget* Scaler = Cast<UDPIScalerWidget>(Selected.GetTemplate());
			const UDPIScalerWidget* PreviewScaler = Cast<UDPIScalerWidget>(Selected.GetPreview());
			FGeometry WidgetGeometry;
			if (Scaler == nullptr || !Designer->GetWidgetGeometry(Selected, WidgetGeometry)) return;
			WidgetGeometry = Designer->MakeGeometryWindowLocal(WidgetGeometry);
			const FVector2D Origin = WidgetGeometry.LocalToAbsolute(FVector2D::ZeroVector);
			const FVector2D End = WidgetGeometry.LocalToAbsolute(WidgetGeometry.GetLocalSize());
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
			DrawRuler(*Scaler, ActiveRule, Origin, UnitsToSlate, ViewportSize, true, DrawElements, LayerId + 20);
			DrawRuler(*Scaler, ActiveRule, Origin, UnitsToSlate, ViewportSize, false, DrawElements, LayerId + 30);
			DrawActiveRuleStatus(*Scaler, PreviewScaler, Origin, Size, ViewportSize, DrawElements, LayerId + 40);
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
