#include "DPIScalerRuleCustomization.h"

#include "DetailWidgetRow.h"
#include "DPIScalerWidget.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IPropertyTypeCustomization> FDPIScalerRuleCustomization::MakeInstance()
{
	return MakeShared<FDPIScalerRuleCustomization>();
}

void FDPIScalerRuleCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils&)
{
	RulePropertyHandle = StructPropertyHandle;
	HeaderRow
	.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(280.0f)
	[
		SNew(STextBlock)
		.Text(this, &FDPIScalerRuleCustomization::GetSummary)
		.ToolTipText(this, &FDPIScalerRuleCustomization::GetSummary)
	];
}

void FDPIScalerRuleCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils&)
{
	auto AddProperty = [&](FName PropertyName)
	{
		if (TSharedPtr<IPropertyHandle> Child = StructPropertyHandle->GetChildHandle(PropertyName))
		{
			ChildBuilder.AddProperty(Child.ToSharedRef());
		}
	};

	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, Name));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, WidthBreakpoint));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, HeightBreakpoint));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, MinAspectRatio));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, MaxAspectRatio));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, ScaleMode));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, TargetUIScale));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, MinClamp));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, MaxClamp));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, ScaleAxis));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, ScaleCurve));
}

FText FDPIScalerRuleCustomization::GetSummary() const
{
	if (!RulePropertyHandle.IsValid()) return FText::GetEmpty();
	void* ValueData = nullptr;
	if (RulePropertyHandle->GetValueData(ValueData) != FPropertyAccess::Success || ValueData == nullptr)
	{
		return FText::FromString(TEXT("Multiple Values"));
	}

	const FDPIBreakpointRule& Rule = *static_cast<const FDPIBreakpointRule*>(ValueData);
	TArray<FString> Parts;
	Parts.Add(Rule.Name.ToString());
	const TCHAR* Operator = TEXT("≤");
	if (Rule.bUseWidthBreakpoint) Parts.Add(FString::Printf(TEXT("W %s %d"), Operator, Rule.WidthBreakpoint));
	if (Rule.bUseHeightBreakpoint) Parts.Add(FString::Printf(TEXT("H %s %d"), Operator, Rule.HeightBreakpoint));
	if (!Rule.bUseWidthBreakpoint && !Rule.bUseHeightBreakpoint && !Rule.bUseMinAspectRatio && !Rule.bUseMaxAspectRatio) Parts.Add(TEXT("Any"));
	if (Rule.ScaleMode == EDPIBreakpointScaleMode::Fixed) Parts.Add(FString::Printf(TEXT("Fixed %.2f"), Rule.TargetUIScale));
	else if (Rule.ScaleMode == EDPIBreakpointScaleMode::Clamp)
	{
		if (Rule.bUseMinClamp && Rule.bUseMaxClamp) Parts.Add(FString::Printf(TEXT("Clamp %.2f–%.2f"), Rule.MinClamp, Rule.MaxClamp));
		else if (Rule.bUseMinClamp) Parts.Add(FString::Printf(TEXT("Clamp ≥ %.2f"), Rule.MinClamp));
		else if (Rule.bUseMaxClamp) Parts.Add(FString::Printf(TEXT("Clamp ≤ %.2f"), Rule.MaxClamp));
		else Parts.Add(TEXT("Clamp"));
	}
	else if (Rule.ScaleMode == EDPIBreakpointScaleMode::Curve) Parts.Add(TEXT("Curve"));
	else Parts.Add(TEXT("Project DPI"));
	return FText::FromString(FString::Join(Parts, TEXT("  •  ")));
}
