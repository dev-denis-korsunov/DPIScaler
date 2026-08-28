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
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, bEnabled));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, Priority));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, Orientation));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, BreakpointRule));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, WidthBreakpoint));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, HeightBreakpoint));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, MinAspectRatio));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, MaxAspectRatio));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, ScaleMode));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, TargetUIScale));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, CurveAxis));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, ScaleCurve));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, MinScale));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, MaxScale));
	AddProperty(GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, SnapStep));
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
	Parts.Add(Rule.bEnabled ? Rule.Name.ToString() : FString::Printf(TEXT("%s (Disabled)"), *Rule.Name.ToString()));
	if (Rule.Orientation == EDPIBreakpointOrientation::Portrait) Parts.Add(TEXT("Portrait"));
	if (Rule.Orientation == EDPIBreakpointOrientation::Landscape) Parts.Add(TEXT("Landscape"));
	const TCHAR* Operator = Rule.BreakpointRule == EDPIBreakpointRuleDirection::Min ? TEXT("≤") : TEXT("≥");
	if (Rule.bUseWidthBreakpoint) Parts.Add(FString::Printf(TEXT("W %s %d"), Operator, Rule.WidthBreakpoint));
	if (Rule.bUseHeightBreakpoint) Parts.Add(FString::Printf(TEXT("H %s %d"), Operator, Rule.HeightBreakpoint));
	if (!Rule.bUseWidthBreakpoint && !Rule.bUseHeightBreakpoint && Rule.Orientation == EDPIBreakpointOrientation::Any) Parts.Add(TEXT("Any"));
	if (Rule.ScaleMode == EDPIBreakpointScaleMode::Fixed) Parts.Add(FString::Printf(TEXT("Fixed %.2f"), Rule.TargetUIScale));
	else if (Rule.ScaleMode == EDPIBreakpointScaleMode::Curve) Parts.Add(TEXT("Curve"));
	else Parts.Add(TEXT("Project DPI"));
	Parts.Add(FString::Printf(TEXT("P%d"), Rule.Priority));
	return FText::FromString(FString::Join(Parts, TEXT("  •  ")));
}
