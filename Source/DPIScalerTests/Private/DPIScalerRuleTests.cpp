#if WITH_DEV_AUTOMATION_TESTS

#include "DPIScalerWidget.h"
#include "HAL/PlatformTime.h"
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
#if WITH_EDITOR
	Scaler->DesignerDpi = 1.0f;
	Scaler->DesignerSize = FVector2D(600.0f, 1000.0f);
	TestTrue(TEXT("Active-rule query matches the active rule name"), Scaler->IsActiveRule(TEXT("Mobile")));
	TestFalse(TEXT("Active-rule query rejects inactive rule names"), Scaler->IsActiveRule(TEXT("Tablet")));
	TestFalse(TEXT("Active-rule query rejects an empty rule name"), Scaler->IsActiveRule(NAME_None));
#endif
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDPIScalerRuleMatchConditionsTest, "DPIScaler.Rules.MatchConditions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDPIScalerRuleMatchConditionsTest::RunTest(const FString& Parameters)
{
	UDPIScalerWidget* Scaler = NewObject<UDPIScalerWidget>();

	FDPIBreakpointRule AspectRule;
	AspectRule.Name = TEXT("FourThree");
	AspectRule.bUseMinAspectRatio = true;
	AspectRule.MinAspectRatio = 1.2f;
	AspectRule.bUseMaxAspectRatio = true;
	AspectRule.MaxAspectRatio = 1.5f;
	Scaler->DPIRules = { AspectRule };

	TestNotNull(TEXT("Aspect ratio includes its lower boundary"), Scaler->FindActiveRule(FIntPoint(1200, 1000)));
	TestNotNull(TEXT("Aspect ratio includes its upper boundary"), Scaler->FindActiveRule(FIntPoint(1500, 1000)));
	TestNull(TEXT("Aspect ratio rejects a viewport below its range"), Scaler->FindActiveRule(FIntPoint(1199, 1000)));
	TestNull(TEXT("Aspect ratio rejects a viewport above its range"), Scaler->FindActiveRule(FIntPoint(1501, 1000)));

	AspectRule.bUseMinAspectRatio = false;
	Scaler->DPIRules = { AspectRule };
	TestNotNull(TEXT("Maximum aspect ratio works without a minimum"), Scaler->FindActiveRule(FIntPoint(1000, 1000)));

	AspectRule.bUseMinAspectRatio = true;
	AspectRule.MinAspectRatio = 1.2f;
	AspectRule.bUseMaxAspectRatio = false;
	Scaler->DPIRules = { AspectRule };
	TestNotNull(TEXT("Minimum aspect ratio works without a maximum"), Scaler->FindActiveRule(FIntPoint(2000, 1000)));

	Scaler->DPIRules.Reset();
	TestNull(TEXT("Empty rule list has no active rule"), Scaler->FindActiveRule(FIntPoint(1920, 1080)));
	TestNull(TEXT("Zero width viewport has no active rule"), Scaler->FindActiveRule(FIntPoint::ZeroValue));

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

	Rule.ScaleMode = EDPIBreakpointScaleMode::Curve;
	Rule.ScaleAxis = EDPIBreakpointScaleAxis::ScreenWidth;
	Rule.ScaleCurve.GetRichCurve()->Reset();
	Rule.ScaleCurve.GetRichCurve()->AddKey(0.0f, 0.5f);
	Rule.ScaleCurve.GetRichCurve()->AddKey(1000.0f, 1.0f);
	Scale = Scaler->ResolveTargetUIScale(1.25f, FIntPoint(1000, 500), &Rule);
	TestTrue(TEXT("Curve produces the final target scale"), FMath::IsNearlyEqual(Scale, 1.0f));

	Scale = Scaler->ResolveTargetUIScale(1.25f, FIntPoint(1000, 500), nullptr);
	TestTrue(TEXT("No active rule falls back to project DPI"), FMath::IsNearlyEqual(Scale, 1.25f));

	Rule.ScaleMode = EDPIBreakpointScaleMode::Curve;
	Rule.ScaleCurve.GetRichCurve()->Reset();
	Rule.ScaleCurve.GetRichCurve()->AddKey(0.0f, 0.0f);
	Rule.ScaleCurve.GetRichCurve()->AddKey(2000.0f, 2.0f);
	Rule.ScaleAxis = EDPIBreakpointScaleAxis::ShortSide;
	Scale = Scaler->ResolveTargetUIScale(1.0f, FIntPoint(1600, 900), &Rule);
	TestTrue(TEXT("Curve short-side axis is evaluated"), FMath::IsNearlyEqual(Scale, 0.9f));

	Rule.ScaleAxis = EDPIBreakpointScaleAxis::LongSide;
	Scale = Scaler->ResolveTargetUIScale(1.0f, FIntPoint(1600, 900), &Rule);
	TestTrue(TEXT("Curve long-side axis is evaluated"), FMath::IsNearlyEqual(Scale, 1.6f));

	Rule.ScaleAxis = EDPIBreakpointScaleAxis::ScreenHeight;
	Scale = Scaler->ResolveTargetUIScale(1.0f, FIntPoint(1600, 900), &Rule);
	TestTrue(TEXT("Curve screen-height axis is evaluated"), FMath::IsNearlyEqual(Scale, 0.9f));

	Rule.ScaleMode = EDPIBreakpointScaleMode::Fixed;
	Rule.TargetUIScale = -1.0f;
	Scale = Scaler->ResolveTargetUIScale(1.0f, FIntPoint(1280, 720), &Rule);
	TestTrue(TEXT("Invalid fixed scale is forced positive"), Scale > 0.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDPIScalerRuleSearchPerformanceTest, "DPIScaler.Performance.RuleSearch", EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

bool FDPIScalerRuleSearchPerformanceTest::RunTest(const FString& Parameters)
{
	constexpr int32 RuleCount = 64;
	constexpr int32 IterationCount = 100000;
	UDPIScalerWidget* Scaler = NewObject<UDPIScalerWidget>();
	Scaler->DPIRules.Reserve(RuleCount + 1);

	for (int32 Index = 0; Index < RuleCount; ++Index)
	{
		FDPIBreakpointRule Rule;
		Rule.bUseWidthBreakpoint = true;
		Rule.WidthBreakpoint = 100 + Index;
		Scaler->DPIRules.Add(Rule);
	}

	FDPIBreakpointRule FallbackRule;
	FallbackRule.Name = TEXT("Fallback");
	Scaler->DPIRules.Add(FallbackRule);

	volatile int32 MatchCount = 0;
	const double StartTime = FPlatformTime::Seconds();
	for (int32 Index = 0; Index < IterationCount; ++Index)
	{
		if (Scaler->FindActiveRule(FIntPoint(3840, 2160)) != nullptr)
		{
			++MatchCount;
		}
	}
	const double ElapsedSeconds = FPlatformTime::Seconds() - StartTime;
	const double MicrosecondsPerPass = ElapsedSeconds * 1000000.0 / IterationCount;
	AddInfo(FString::Printf(TEXT("DPI rule search: %.3f us per pass (%d rules, %d passes, worst-case fallback)"), MicrosecondsPerPass, RuleCount + 1, IterationCount));
	TestEqual(TEXT("Every benchmark pass reaches its fallback rule"), static_cast<int32>(MatchCount), IterationCount);
	TestTrue(TEXT("Worst-case rule search stays within the 1-second test budget"), ElapsedSeconds < 1.0);

	return true;
}

#endif
