#pragma once

#include "Components/ContentWidget.h"
#include "Curves/CurveFloat.h"

#include "DPIScalerWidget.generated.h"

UENUM(BlueprintType)
enum class EDPIBreakpointScaleMode : uint8
{
	UseProjectDPI UMETA(DisplayName = "Use Project DPI"),
	Fixed,
	Curve
};

UENUM(BlueprintType)
enum class EDPIBreakpointScaleAxis : uint8
{
	ShortSide,
	LongSide,
	ScreenWidth,
	ScreenHeight
};

USTRUCT(BlueprintType)
struct DPISCALER_API FDPIBreakpointRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="bUseWidthBreakpoint", ClampMin="0"))
	int32 WidthBreakpoint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="bUseHeightBreakpoint", ClampMin="0"))
	int32 HeightBreakpoint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="bUseMinAspectRatio", ClampMin="0.0001"))
	float MinAspectRatio;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="bUseMaxAspectRatio", ClampMin="0.0001"))
	float MaxAspectRatio;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EDPIBreakpointScaleMode ScaleMode;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="ScaleMode == EDPIBreakpointScaleMode::Fixed", EditConditionHides, ClampMin="0.0001"))
	float TargetUIScale;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="ScaleMode == EDPIBreakpointScaleMode::Curve", EditConditionHides))
	EDPIBreakpointScaleAxis ScaleAxis;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="ScaleMode == EDPIBreakpointScaleMode::Curve", EditConditionHides))
	FRuntimeFloatCurve ScaleCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(InlineEditConditionToggle))
	uint8 bUseWidthBreakpoint : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(InlineEditConditionToggle))
	uint8 bUseHeightBreakpoint : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(InlineEditConditionToggle))
	uint8 bUseMinAspectRatio : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(InlineEditConditionToggle))
	uint8 bUseMaxAspectRatio : 1;

	FDPIBreakpointRule();
};

UCLASS(DisplayName = "DPI Scaler")
class DPISCALER_API UDPIScalerWidget final : public UContentWidget
{
	GENERATED_BODY()
public:
	UDPIScalerWidget(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DPI Scaler")
	bool bDesignerMute = false;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DPI Scaler", DisplayName = "DPI Rules")
	TArray<FDPIBreakpointRule> DPIRules;

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
