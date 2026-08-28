#if WITH_DEV_AUTOMATION_TESTS

#include "DPIScalerWidget.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDPIScalerRuleSelectionTest, "DPIScaler.Rules.Selection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDPIScalerRuleSelectionTest::RunTest(const FString& Parameters)
{
	UDPIScalerWidget* Scaler = NewObject<UDPIScalerWidget>();

	FDPIBreakpointRule DefaultRule;
	DefaultRule.Name = TEXT("Default");

	FDPIBreakpointRule TabletRule;
	TabletRule.Name = TEXT("Tablet");
	TabletRule.bUseWidthBreakpoint = true;
	TabletRule.WidthBreakpoint = 1080;
	TabletRule.ScaleMode = EDPIBreakpointScaleMode::Fixed;
	TabletRule.TargetUIScale = 0.95f;

	FDPIBreakpointRule MobileRule;
	MobileRule.Name = TEXT("Mobile");
	MobileRule.bUseWidthBreakpoint = true;
	MobileRule.WidthBreakpoint = 720;
	MobileRule.ScaleMode = EDPIBreakpointScaleMode::Fixed;
	MobileRule.TargetUIScale = 0.8f;

	Scaler->DPIRules = { MobileRule, TabletRule, DefaultRule };
	const FDPIBreakpointRule* ActiveRule = Scaler->FindActiveRule(FIntPoint(600, 1000));
	TestEqual(TEXT("First matching rule wins"), ActiveRule != nullptr ? ActiveRule->Name : NAME_None, FName(TEXT("Mobile")));
	ActiveRule = Scaler->FindActiveRule(FIntPoint(900, 1280));
	TestEqual(TEXT("Width breakpoint selects tablet"), ActiveRule != nullptr ? ActiveRule->Name : NAME_None, FName(TEXT("Tablet")));
	ActiveRule = Scaler->FindActiveRule(FIntPoint(2560, 1440));
	TestEqual(TEXT("Any rule provides a default"), ActiveRule != nullptr ? ActiveRule->Name : NAME_None, FName(TEXT("Default")));

	Scaler->DPIRules = { TabletRule, MobileRule, DefaultRule };
	ActiveRule = Scaler->FindActiveRule(FIntPoint(600, 1000));
	TestEqual(TEXT("Moving a rule earlier changes precedence"), ActiveRule != nullptr ? ActiveRule->Name : NAME_None, FName(TEXT("Tablet")));

	FDPIBreakpointRule BoundedRule;
	BoundedRule.Name = TEXT("Bounded");
	BoundedRule.bUseWidthBreakpoint = true;
	BoundedRule.WidthBreakpoint = 500;
	BoundedRule.bUseHeightBreakpoint = true;
	BoundedRule.HeightBreakpoint = 500;
	Scaler->DPIRules = { BoundedRule };
	TestNotNull(TEXT("Two Max breakpoints accept a bounded viewport"), Scaler->FindActiveRule(FIntPoint(500, 400)));
	TestNull(TEXT("Two Max breakpoints reject width outside the rectangle"), Scaler->FindActiveRule(FIntPoint(501, 400)));
	TestNull(TEXT("Two Max breakpoints reject height outside the rectangle"), Scaler->FindActiveRule(FIntPoint(400, 501)));

	BoundedRule.bUseHeightBreakpoint = false;
	Scaler->DPIRules = { BoundedRule };
	TestNotNull(TEXT("Missing height breakpoint accepts every height"), Scaler->FindActiveRule(FIntPoint(500, 4000)));
	TestNull(TEXT("Width-only rule still rejects width outside its range"), Scaler->FindActiveRule(FIntPoint(501, 4000)));

	BoundedRule.bUseWidthBreakpoint = false;
	BoundedRule.bUseHeightBreakpoint = true;
	BoundedRule.HeightBreakpoint = 500;
	Scaler->DPIRules = { BoundedRule };
	TestNotNull(TEXT("Missing width breakpoint accepts every width"), Scaler->FindActiveRule(FIntPoint(4000, 500)));
	TestNull(TEXT("Height-only rule still rejects height outside its range"), Scaler->FindActiveRule(FIntPoint(4000, 501)));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDPIScalerRuleScaleTest, "DPIScaler.Rules.Scale", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDPIScalerRuleScaleTest::RunTest(const FString& Parameters)
{
	UDPIScalerWidget* Scaler = NewObject<UDPIScalerWidget>();
	FDPIBreakpointRule Rule;
	Rule.ScaleMode = EDPIBreakpointScaleMode::Fixed;
	Rule.TargetUIScale = 0.78f;
	float Scale = Scaler->ResolveTargetUIScale(1.0f, FIntPoint(1280, 720), &Rule);
	TestTrue(TEXT("Fixed target is used directly"), FMath::IsNearlyEqual(Scale, 0.78f));

	Rule.ScaleMode = EDPIBreakpointScaleMode::UseProjectDPI;
	Scale = Scaler->ResolveTargetUIScale(1.25f, FIntPoint(1280, 720), &Rule);
	TestTrue(TEXT("Project DPI is used directly"), FMath::IsNearlyEqual(Scale, 1.25f));

	Rule.ScaleMode = EDPIBreakpointScaleMode::Clamp;
	Rule.bUseMinClamp = true;
	Rule.MinClamp = 0.75f;
	Rule.bUseMaxClamp = true;
	Rule.MaxClamp = 1.1f;
	Scale = Scaler->ResolveTargetUIScale(1.25f, FIntPoint(1280, 720), &Rule);
	TestTrue(TEXT("Clamp limits project DPI"), FMath::IsNearlyEqual(Scale, 1.1f));

	Rule.ScaleMode = EDPIBreakpointScaleMode::Curve;
	Rule.ScaleAxis = EDPIBreakpointScaleAxis::ScreenWidth;
	Rule.ScaleCurve.GetRichCurve()->Reset();
	Rule.ScaleCurve.GetRichCurve()->AddKey(0.0f, 0.5f);
	Rule.ScaleCurve.GetRichCurve()->AddKey(1000.0f, 1.0f);
	Scale = Scaler->ResolveTargetUIScale(1.25f, FIntPoint(1000, 500), &Rule);
	TestTrue(TEXT("Curve produces the final target scale"), FMath::IsNearlyEqual(Scale, 1.0f));

	Scale = Scaler->ResolveTargetUIScale(1.25f, FIntPoint(1000, 500), nullptr);
	TestTrue(TEXT("No active rule falls back to project DPI"), FMath::IsNearlyEqual(Scale, 1.25f));
	return true;
}

#endif
