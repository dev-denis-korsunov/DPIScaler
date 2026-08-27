#include "Modules/ModuleManager.h"

#include "DPIScalerWidget.h"
#include "DesignerExtension.h"
#include "IUMGDesigner.h"
#include "IHasDesignerExtensibility.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "UMGEditorModule.h"
#include "WidgetReference.h"

#define LOCTEXT_NAMESPACE "DPIScalerEditor"

namespace UE::DPIScalerEditor::Private
{
	static constexpr float RulerThickness = 24.0f;
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

	static int32 GetRulerMaximum(float CurrentSize, const TArray<FDPIMediaQuery>& Queries, bool bWidth)
	{
		int32 Maximum = FMath::Max(1, FMath::CeilToInt(CurrentSize));
		for (const FDPIMediaQuery& Query : Queries)
		{
			const bool bHasMin = bWidth ? Query.bOverrideScreenMinWidth : Query.bOverrideScreenMinHeight;
			const bool bHasMax = bWidth ? Query.bOverrideScreenMaxWidth : Query.bOverrideScreenMaxHeight;
			if (bHasMin) Maximum = FMath::Max(Maximum, bWidth ? Query.ScreenMinWidth : Query.ScreenMinHeight);
			if (bHasMax) Maximum = FMath::Max(Maximum, bWidth ? Query.ScreenMaxWidth : Query.ScreenMaxHeight);
		}
		return Maximum;
	}

	static void DrawRuler(const UDPIScalerWidget& Scaler, const FVector2D& Origin, float UnitsToSlate, float CurrentViewportSize, bool bWidth, FSlateWindowElementList& DrawElements, int32 LayerId)
	{
		const int32 Maximum = GetRulerMaximum(CurrentViewportSize, Scaler.MediaQueries, bWidth);
		const float RulerLength = Maximum * UnitsToSlate;
		const FVector2D RulerPosition = bWidth ? FVector2D(Origin.X, Origin.Y - RulerThickness - RulerGap) : FVector2D(Origin.X - RulerThickness - RulerGap, Origin.Y);
		const FVector2D RulerSize = bWidth ? FVector2D(RulerLength, RulerThickness) : FVector2D(RulerThickness, RulerLength);
		const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
		FSlateDrawElement::MakeBox(DrawElements, LayerId, FPaintGeometry(RulerPosition, RulerSize, 1.0f), WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.045f, 0.055f, 0.075f, 0.94f));

		const int32 TickStep = Maximum <= 800 ? 100 : Maximum <= 2000 ? 250 : 500;
		for (int32 Value = 0; Value <= Maximum; Value += TickStep)
		{
			const float Position = Value * UnitsToSlate;
			const FVector2D LineStart = bWidth ? RulerPosition + FVector2D(Position, RulerThickness - 7.0f) : RulerPosition + FVector2D(RulerThickness - 7.0f, Position);
			const FVector2D LineEnd = bWidth ? RulerPosition + FVector2D(Position, RulerThickness) : RulerPosition + FVector2D(RulerThickness, Position);
			const TArray<FVector2D> Points = { LineStart, LineEnd };
			FSlateDrawElement::MakeLines(DrawElements, LayerId + 1, FPaintGeometry(), Points, ESlateDrawEffect::None, FLinearColor::White.CopyWithNewOpacity(0.35f), true, 1.0f);
			if (bWidth)
			{
				FSlateDrawElement::MakeText(DrawElements, LayerId + 2, FPaintGeometry(RulerPosition + FVector2D(Position + 3.0f, 2.0f), FVector2D::ZeroVector, 1.0f), FText::AsNumber(Value), FCoreStyle::GetDefaultFontStyle("Regular", 8), ESlateDrawEffect::None, FLinearColor::White.CopyWithNewOpacity(0.7f));
			}
		}

		for (int32 QueryIndex = 0; QueryIndex < Scaler.MediaQueries.Num(); ++QueryIndex)
		{
			const FDPIMediaQuery& Query = Scaler.MediaQueries[QueryIndex];
			const FLinearColor Color = GetRuleColor(QueryIndex);
			const bool bHasMin = bWidth ? Query.bOverrideScreenMinWidth : Query.bOverrideScreenMinHeight;
			const bool bHasMax = bWidth ? Query.bOverrideScreenMaxWidth : Query.bOverrideScreenMaxHeight;
			const int32 MinValue = bWidth ? Query.ScreenMinWidth : Query.ScreenMinHeight;
			const int32 MaxValue = bWidth ? Query.ScreenMaxWidth : Query.ScreenMaxHeight;
			auto DrawMarker = [&](int32 Value, bool bMinimum)
			{
				const float Position = FMath::Clamp(Value * UnitsToSlate, 0.0f, RulerLength);
				const FVector2D Start = bWidth ? RulerPosition + FVector2D(Position, 0.0f) : RulerPosition + FVector2D(0.0f, Position);
				const FVector2D End = bWidth ? RulerPosition + FVector2D(Position, RulerThickness) : RulerPosition + FVector2D(RulerThickness, Position);
				const TArray<FVector2D> Points = { Start, End };
				FSlateDrawElement::MakeLines(DrawElements, LayerId + 3, FPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, bMinimum ? 2.5f : 1.5f);
				const FString Label = FString::Printf(TEXT("%s%d"), bMinimum ? TEXT("≥") : TEXT("≤"), Value);
				const FVector2D MarkerLabelPosition = bWidth
					? RulerPosition + FVector2D(Position + 3.0f, 11.0f)
					: RulerPosition + FVector2D(2.0f, Position + 2.0f);
				FSlateDrawElement::MakeText(DrawElements, LayerId + 4, FPaintGeometry(MarkerLabelPosition, FVector2D::ZeroVector, 1.0f), FText::FromString(Label), FCoreStyle::GetDefaultFontStyle("Bold", 8), ESlateDrawEffect::None, Color);
			};
			if (bHasMin) DrawMarker(MinValue, true);
			if (bHasMax) DrawMarker(MaxValue, false);
		}

		const FString SizeLabel = FString::Printf(TEXT("%s %d"), bWidth ? TEXT("W") : TEXT("H"), FMath::RoundToInt(CurrentViewportSize));
		const FVector2D LabelPosition = bWidth ? RulerPosition + FVector2D(3.0f, RulerThickness - 12.0f) : RulerPosition + FVector2D(2.0f, 2.0f);
		FSlateDrawElement::MakeText(DrawElements, LayerId + 5, FPaintGeometry(LabelPosition, FVector2D::ZeroVector, 1.0f), FText::FromString(SizeLabel), FCoreStyle::GetDefaultFontStyle("Bold", 8), ESlateDrawEffect::None, FLinearColor::White);
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
			DrawRuler(*Scaler, Origin, UnitsToSlate, ViewportSize.X, true, DrawElements, LayerId + 20);
			DrawRuler(*Scaler, Origin, UnitsToSlate, ViewportSize.Y, false, DrawElements, LayerId + 30);
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
	}

	TSharedPtr<IDesignerExtensionFactory> ExtensionFactory;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDPIScalerEditorModule, DPIScalerEditor)
