#pragma once

#include "Components/ContentWidget.h"
#include "Curves/CurveFloat.h"

#include "DPIScalerWidget.generated.h"

UENUM(BlueprintType)
enum class EDPIBreakpointOrientation : uint8
{
	Any,
	Portrait,
	Landscape
};

UENUM(BlueprintType)
enum class EDPIBreakpointScaleMode : uint8
{
	UseProjectDPI UMETA(DisplayName = "Use Project DPI"),
	Fixed,
	Curve
};

UENUM(BlueprintType)
enum class EDPIBreakpointCurveAxis : uint8
{
	ShortSide UMETA(DisplayName = "Short Side"),
	LongSide UMETA(DisplayName = "Long Side"),
	ScreenWidth UMETA(DisplayName = "Screen Width"),
	ScreenHeight UMETA(DisplayName = "Screen Height")
};

USTRUCT(BlueprintType)
struct DPISCALER_API FDPIBreakpointRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule") FName Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule") bool bEnabled;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule") int32 Priority;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match") EDPIBreakpointOrientation Orientation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match", DisplayName = "Minimum Short Side", meta = (ClampMin = "0")) int32 MinShortSide;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match", DisplayName = "Maximum Short Side", meta = (ClampMin = "0", ToolTip = "Zero means unbounded.")) int32 MaxShortSide;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Match|Advanced", DisplayName = "Minimum Width", meta = (ClampMin = "0")) int32 MinWidth;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Match|Advanced", DisplayName = "Maximum Width", meta = (ClampMin = "0")) int32 MaxWidth;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Match|Advanced", DisplayName = "Minimum Height", meta = (ClampMin = "0")) int32 MinHeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Match|Advanced", DisplayName = "Maximum Height", meta = (ClampMin = "0")) int32 MaxHeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Match|Advanced", DisplayName = "Minimum Aspect Ratio", meta = (ClampMin = "0.0")) float MinAspectRatio;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Match|Advanced", DisplayName = "Maximum Aspect Ratio", meta = (ClampMin = "0.0")) float MaxAspectRatio;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale") EDPIBreakpointScaleMode ScaleMode;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale", DisplayName = "Target UI Scale", meta = (EditCondition = "ScaleMode == EDPIBreakpointScaleMode::Fixed", EditConditionHides, ClampMin = "0.0001")) float TargetUIScale;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale", meta = (EditCondition = "ScaleMode == EDPIBreakpointScaleMode::Curve", EditConditionHides)) EDPIBreakpointCurveAxis CurveAxis;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale", DisplayName = "Scale Curve", meta = (EditCondition = "ScaleMode == EDPIBreakpointScaleMode::Curve", EditConditionHides)) FRuntimeFloatCurve ScaleCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Limits", DisplayName = "Minimum Scale", meta = (ClampMin = "0.0", ToolTip = "Zero disables the lower limit.")) float MinScale;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Limits", DisplayName = "Maximum Scale", meta = (ClampMin = "0.0", ToolTip = "Zero disables the upper limit.")) float MaxScale;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Limits", DisplayName = "Snap Step", meta = (ClampMin = "0.0", ToolTip = "Zero disables snapping.")) float SnapStep;

	FDPIBreakpointRule();
};

UCLASS(DisplayName = "DPI Scaler")
class DPISCALER_API UDPIScalerWidget final : public UContentWidget
{
	GENERATED_BODY()
public:
	UDPIScalerWidget(const FObjectInitializer& ObjectInitializer);
#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DPI Scaler") bool bDesignerMute = false;
#endif
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DPI Scaler", DisplayName = "DPI Rules") TArray<FDPIBreakpointRule> DPIRules;
	UFUNCTION(BlueprintCallable, Category = "DPI Scaler", meta = (DisplayName = "Set DPI Rules"))
	void SetDPIRules(const TArray<FDPIBreakpointRule>& InDPIRules);
	const FDPIBreakpointRule* FindActiveRule(const FIntPoint& ViewportSize) const;
	float ResolveTargetUIScale(float ProjectDPIScale, const FIntPoint& ViewportSize, const FDPIBreakpointRule* Rule) const;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void SynchronizeProperties() override;
protected:
	float GetDPIScale() const;
	float GetAbsoluteDesiredDPIScale(float ApplicationScale, const FIntPoint& ViewportSize) const;
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
