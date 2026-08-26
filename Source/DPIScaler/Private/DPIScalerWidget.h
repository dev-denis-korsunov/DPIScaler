#pragma once

#include "Components/ContentWidget.h"
#include "Curves/CurveFloat.h"

#include "DPIScalerWidget.generated.h"

UENUM(BlueprintType)
enum class EDPIMediaQueryBlendType : uint8 { Override, Min, Max };

UENUM(BlueprintType)
enum class EDPICurveRuleType : uint8 { ByShortSide, ByLongSide, ByScreenWidth, ByScreenHeight };

USTRUCT(BlueprintType)
struct DPISCALER_API FDPIMediaQuery
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, meta = (MultiLine = true)) FString CommentText;
#endif
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EDPIMediaQueryBlendType BlendType;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bOverrideScreenMinWidth")) int32 ScreenMinWidth;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bOverrideScreenMaxWidth")) int32 ScreenMaxWidth;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bOverrideScreenMinHeight")) int32 ScreenMinHeight;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bOverrideScreenMaxHeight")) int32 ScreenMaxHeight;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bOverrideScreenMinAspectRatio", ClampMin = "0.000001")) float ScreenMinAspectRatio;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bOverrideScreenMaxAspectRatio")) float ScreenMaxAspectRatio;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "DPI Scale Override", meta = (EditCondition = "bOverrideDPIScaleOverride", ClampMin = "0.0001")) float DPIScaleOverride;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Min DPI Scale", meta = (EditCondition = "bOverrideMinDPIScale", ClampMin = "0.0001")) float MinDPIScale;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Max DPI Scale", meta = (EditCondition = "bOverrideMaxDPIScale", ClampMin = "0.0001")) float MaxDPIScale;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bOverrideCurve")) EDPICurveRuleType CurveRule;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "DPI Curve", meta = (EditCondition = "bOverrideCurve", EditConditionHides)) FRuntimeFloatCurve DPICurve;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Snap DPI Scale Grid", meta = (EditCondition = "bSnapDPIToGrid", ClampMin = "0.0001")) float SnapDPIScaleGrid;
	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle)) uint8 bOverrideScreenMinWidth : 1;
	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle)) uint8 bOverrideScreenMaxWidth : 1;
	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle)) uint8 bOverrideScreenMinHeight : 1;
	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle)) uint8 bOverrideScreenMaxHeight : 1;
	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle)) uint8 bOverrideScreenMinAspectRatio : 1;
	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle)) uint8 bOverrideScreenMaxAspectRatio : 1;
	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle)) uint8 bOverrideDPIScaleOverride : 1;
	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle)) uint8 bOverrideMinDPIScale : 1;
	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle)) uint8 bOverrideMaxDPIScale : 1;
	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle)) uint8 bOverrideCurve : 1;
	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle)) uint8 bSnapDPIToGrid : 1;

	FDPIMediaQuery();
};

UCLASS(DisplayName = "DPI Scaler")
class DPISCALER_API UDPIScalerWidget final : public UContentWidget
{
	GENERATED_BODY()
public:
	UDPIScalerWidget(const FObjectInitializer& ObjectInitializer);
#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bDesignerMute = false;
#endif
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FDPIMediaQuery> MediaQueries;
	UFUNCTION(BlueprintCallable, Category = "DPI Scaler")
	void SetMediaQueries(const TArray<FDPIMediaQuery>& InMediaQueries);
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void SynchronizeProperties() override;
protected:
	float GetDPIScale() const;
	float GetAbsoluteDesiredDPIScale(float ApplicationScale, const FIntPoint& ViewportSize) const;
	float GetDesiredDPIScale(float SourceScale, const FIntPoint& ViewportSize) const;
	const UDPIScalerWidget* FindParentDPIScaler() const;
	virtual void OnSlotAdded(UPanelSlot* InSlot) override;
	virtual void OnSlotRemoved(UPanelSlot* InSlot) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	TSharedPtr<class SDPIScaler> DPIScaler;
#if WITH_EDITOR
public:
	virtual void OnDesignerChanged(const FDesignerChangedEventArgs& EventArgs) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	TOptional<FVector2D> DesignerSize;
	TOptional<float> DesignerDpi;
	bool bScreenPreview = false;
#endif
};
