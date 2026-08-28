#include "DPIScalerRuleCustomization.h"

#include "DetailWidgetRow.h"
#include "DPIScalerWidget.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
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
	auto AddProperty = [&](IDetailGroup& Group, FName PropertyName)
	{
		if (TSharedPtr<IPropertyHandle> Child = StructPropertyHandle->GetChildHandle(PropertyName))
		{
			Group.AddPropertyRow(Child.ToSharedRef());
		}
	};

	IDetailGroup& RuleGroup = ChildBuilder.AddGroup(TEXT("Rule"), FText::FromString(TEXT("Rule")), true);
	AddProperty(RuleGroup, GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, Name));
	AddProperty(RuleGroup, GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, bEnabled));
	AddProperty(RuleGroup, GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, Priority));

	IDetailGroup& MatchGroup = ChildBuilder.AddGroup(TEXT("Match"), FText::FromString(TEXT("Match")), true);
	AddProperty(MatchGroup, GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, Orientation));
	AddProperty(MatchGroup, GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, BreakpointRule));
	AddProperty(MatchGroup, GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, WidthBreakpoint));
	AddProperty(MatchGroup, GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, HeightBreakpoint));

	IDetailGroup& AdvancedMatchGroup = ChildBuilder.AddGroup(TEXT("AdvancedMatch"), FText::FromString(TEXT("Advanced Match")));
	AddProperty(AdvancedMatchGroup, GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, MinAspectRatio));
	AddProperty(AdvancedMatchGroup, GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, MaxAspectRatio));

	IDetailGroup& ScaleGroup = ChildBuilder.AddGroup(TEXT("Scale"), FText::FromString(TEXT("Scale")), true);
	AddProperty(ScaleGroup, GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, ScaleMode));
	AddProperty(ScaleGroup, GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, TargetUIScale));
	AddProperty(ScaleGroup, GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, CurveAxis));
	AddProperty(ScaleGroup, GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, ScaleCurve));

	IDetailGroup& LimitsGroup = ChildBuilder.AddGroup(TEXT("Limits"), FText::FromString(TEXT("Limits")));
	AddProperty(LimitsGroup, GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, MinScale));
	AddProperty(LimitsGroup, GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, MaxScale));
	AddProperty(LimitsGroup, GET_MEMBER_NAME_CHECKED(FDPIBreakpointRule, SnapStep));
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
