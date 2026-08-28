#if WITH_DEV_AUTOMATION_TESTS

#include "DPIScalerWidget.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDPIScalerRuleSelectionTest, "DPIScaler.Rules.Selection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDPIScalerRuleSelectionTest::RunTest(const FString& Parameters)
{
	UDPIScalerWidget* Scaler = NewObject<UDPIScalerWidget>();

	FDPIBreakpointRule DefaultRule;
	DefaultRule.Name = TEXT("Default");
	DefaultRule.Priority = 0;

	FDPIBreakpointRule TabletRule;
	TabletRule.Name = TEXT("Tablet");
	TabletRule.Priority = 50;
	TabletRule.bUseWidthBreakpoint = true;
	TabletRule.WidthBreakpoint = 1080;
	TabletRule.ScaleMode = EDPIBreakpointScaleMode::Fixed;
	TabletRule.TargetUIScale = 0.95f;

	FDPIBreakpointRule MobileRule;
	MobileRule.Name = TEXT("Mobile");
	MobileRule.Priority = 100;
	MobileRule.bUseWidthBreakpoint = true;
	MobileRule.WidthBreakpoint = 720;
	MobileRule.ScaleMode = EDPIBreakpointScaleMode::Fixed;
	MobileRule.TargetUIScale = 0.8f;

	Scaler->DPIRules = { DefaultRule, TabletRule, MobileRule };
	const FDPIBreakpointRule* ActiveRule = Scaler->FindActiveRule(FIntPoint(600, 1000));
	TestEqual(TEXT("Highest-priority matching rule wins"), ActiveRule != nullptr ? ActiveRule->Name : NAME_None, FName(TEXT("Mobile")));
	ActiveRule = Scaler->FindActiveRule(FIntPoint(900, 1280));
	TestEqual(TEXT("Width breakpoint selects tablet"), ActiveRule != nullptr ? ActiveRule->Name : NAME_None, FName(TEXT("Tablet")));
	ActiveRule = Scaler->FindActiveRule(FIntPoint(2560, 1440));
	TestEqual(TEXT("Any rule provides a default"), ActiveRule != nullptr ? ActiveRule->Name : NAME_None, FName(TEXT("Default")));

	Scaler->DPIRules[2].bEnabled = false;
	ActiveRule = Scaler->FindActiveRule(FIntPoint(600, 1000));
	TestEqual(TEXT("Disabled rules are ignored"), ActiveRule != nullptr ? ActiveRule->Name : NAME_None, FName(TEXT("Tablet")));

	FDPIBreakpointRule FirstTie = DefaultRule;
	FirstTie.Name = TEXT("First");
	FirstTie.Priority = 10;
	FDPIBreakpointRule SecondTie = FirstTie;
	SecondTie.Name = TEXT("Second");
	Scaler->DPIRules = { FirstTie, SecondTie };
	ActiveRule = Scaler->FindActiveRule(FIntPoint(1920, 1080));
	TestEqual(TEXT("Array order resolves equal priorities"), ActiveRule != nullptr ? ActiveRule->Name : NAME_None, FName(TEXT("First")));

	FDPIBreakpointRule BoundedRule;
	BoundedRule.Name = TEXT("Bounded");
	BoundedRule.bUseWidthBreakpoint = true;
	BoundedRule.WidthBreakpoint = 500;
	BoundedRule.bUseHeightBreakpoint = true;
	BoundedRule.HeightBreakpoint = 500;
	Scaler->DPIRules = { BoundedRule };
	TestNotNull(TEXT("Two Min breakpoints accept a bounded viewport"), Scaler->FindActiveRule(FIntPoint(500, 400)));
	TestNull(TEXT("Two Min breakpoints reject width outside the rectangle"), Scaler->FindActiveRule(FIntPoint(501, 400)));
	TestNull(TEXT("Two Min breakpoints reject height outside the rectangle"), Scaler->FindActiveRule(FIntPoint(400, 501)));

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

	BoundedRule.bUseWidthBreakpoint = true;
	BoundedRule.WidthBreakpoint = 500;
	BoundedRule.bUseHeightBreakpoint = false;
	BoundedRule.BreakpointRule = EDPIBreakpointRuleDirection::Max;
	Scaler->DPIRules = { BoundedRule };
	TestNotNull(TEXT("Max rule accepts values from the breakpoint"), Scaler->FindActiveRule(FIntPoint(500, 100)));
	TestNull(TEXT("Max rule rejects values below the breakpoint"), Scaler->FindActiveRule(FIntPoint(499, 100)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDPIScalerRuleScaleTest, "DPIScaler.Rules.Scale", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDPIScalerRuleScaleTest::RunTest(const FString& Parameters)
{
	UDPIScalerWidget* Scaler = NewObject<UDPIScalerWidget>();
	FDPIBreakpointRule Rule;
	Rule.ScaleMode = EDPIBreakpointScaleMode::Fixed;
	Rule.TargetUIScale = 0.78f;
	Rule.MinScale = 0.75f;
	Rule.MaxScale = 1.2f;
	Rule.SnapStep = 0.05f;
	float Scale = Scaler->ResolveTargetUIScale(1.0f, FIntPoint(1280, 720), &Rule);
	TestTrue(TEXT("Fixed target is snapped after limits"), FMath::IsNearlyEqual(Scale, 0.8f));

	Rule.ScaleMode = EDPIBreakpointScaleMode::UseProjectDPI;
	Rule.MinScale = 0.0f;
	Rule.MaxScale = 1.1f;
	Rule.SnapStep = 0.0f;
	Scale = Scaler->ResolveTargetUIScale(1.25f, FIntPoint(1280, 720), &Rule);
	TestTrue(TEXT("Project DPI is clamped"), FMath::IsNearlyEqual(Scale, 1.1f));

	Rule.ScaleMode = EDPIBreakpointScaleMode::Curve;
	Rule.CurveAxis = EDPIBreakpointCurveAxis::ScreenWidth;
	Rule.MaxScale = 0.0f;
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
