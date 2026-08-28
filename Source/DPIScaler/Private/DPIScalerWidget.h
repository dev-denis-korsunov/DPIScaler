#pragma once

#include "Components/ContentWidget.h"
#include "Curves/CurveFloat.h"

#include "DPIScalerWidget.generated.h"

UENUM(BlueprintType)
enum class EDPIBreakpointScaleMode : uint8
{
	UseProjectDPI UMETA(DisplayName = "Use Project DPI"),
	Fixed,
	Clamp,
	Curve
};

UENUM(BlueprintType)
enum class EDPIBreakpointScaleAxis : uint8
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match", meta = (InlineEditConditionToggle)) bool bUseWidthBreakpoint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match", meta = (EditCondition = "bUseWidthBreakpoint", ClampMin = "0")) int32 WidthBreakpoint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match", meta = (InlineEditConditionToggle)) bool bUseHeightBreakpoint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match", meta = (EditCondition = "bUseHeightBreakpoint", ClampMin = "0")) int32 HeightBreakpoint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match", meta = (InlineEditConditionToggle)) bool bUseMinAspectRatio;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match", DisplayName = "Minimum Aspect Ratio", meta = (EditCondition = "bUseMinAspectRatio", ClampMin = "0.0001")) float MinAspectRatio;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match", meta = (InlineEditConditionToggle)) bool bUseMaxAspectRatio;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match", DisplayName = "Maximum Aspect Ratio", meta = (EditCondition = "bUseMaxAspectRatio", ClampMin = "0.0001")) float MaxAspectRatio;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale") EDPIBreakpointScaleMode ScaleMode;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale", DisplayName = "Target UI Scale", meta = (EditCondition = "ScaleMode == EDPIBreakpointScaleMode::Fixed", EditConditionHides, ClampMin = "0.0001")) float TargetUIScale;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale", meta = (InlineEditConditionToggle)) bool bUseMinClamp;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale", DisplayName = "Min Scale", meta = (EditCondition = "ScaleMode == EDPIBreakpointScaleMode::Clamp && bUseMinClamp", EditConditionHides, ClampMin = "0.0001")) float MinClamp;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale", meta = (InlineEditConditionToggle)) bool bUseMaxClamp;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale", DisplayName = "Max Scale", meta = (EditCondition = "ScaleMode == EDPIBreakpointScaleMode::Clamp && bUseMaxClamp", EditConditionHides, ClampMin = "0.0001")) float MaxClamp;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale", DisplayName = "Scale Axis", meta = (EditCondition = "ScaleMode == EDPIBreakpointScaleMode::Curve", EditConditionHides)) EDPIBreakpointScaleAxis ScaleAxis;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale", DisplayName = "Scale Curve", meta = (EditCondition = "ScaleMode == EDPIBreakpointScaleMode::Curve", EditConditionHides)) FRuntimeFloatCurve ScaleCurve;

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
