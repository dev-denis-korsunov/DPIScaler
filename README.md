# DPI Scaler

<img width="951" height="576" alt="DPI Scaler designer rulers" src="https://github.com/user-attachments/assets/4d6f49dd-0e8f-46f8-b09b-fa86c040938c" />

`DPI Scaler` is an **Unreal Engine 5 UMG plugin**. It adds a container widget that scales one child from viewport-size rules, so a specific UI subtree can respond independently of the project-wide DPI curve.

## What it adds

- **Local DPI scaling** for a UMG subtree, without changing the project's global DPI settings.
- **Ordered viewport rules** with optional width, height, and aspect-ratio constraints.
- **Project DPI**, **Fixed**, and **Curve** scale modes.
- The Blueprint node **Set DPI Rules** for runtime rule replacement.
- **Designer rulers** that visualize which width and height ranges match each rule.

## Installation

Enable the **DPI Scaler** plugin in Unreal Editor. The **DPI Scaler** widget appears in the UMG Palette after the plugin loads.

## Basic setup

1. Add a **DPI Scaler** to a Widget Blueprint.
2. Give it one child panel — usually a `Canvas Panel`.
3. Add rules in **DPI Rules**. Array entries use the rule `Name` as their title.
4. Put the most specific rule first; the first matching rule wins.

Recommended hierarchy:

```text
DPI Scaler
└── Canvas Panel
    └── UI content
```

Do not wrap a `Scale Box` with a `DPI Scaler`, or place a `DPI Scaler` inside a `Scale Box`, in the same layout branch. Use one scaling mechanism for that branch. Nested DPI Scalers are supported intentionally: an inner scaler resolves its local scale relative to the outer scaler rather than multiplying both final scales.

## DPI rules

A rule has a `Name`, optional match conditions, and a scale mode.

### Matching a viewport

All enabled conditions are combined with `AND`:

- **Width Breakpoint**: viewport width is `≤` its value.
- **Height Breakpoint**: viewport height is `≤` its value.
- **Minimum Aspect Ratio**: `width / height` is at least the configured value.
- **Maximum Aspect Ratio**: `width / height` is at most the configured value.

Each condition is optional. With only a width breakpoint, every height is accepted; with no conditions, the rule matches every valid viewport. Rules are evaluated from the beginning of the array, and the first match is used. If nothing matches, the project DPI scale is used.

For portrait device classes, `Width Breakpoint` is the short side. A practical broad phone rule is `Width ≤ 1440`; use lower breakpoints when the design has distinct compact and large-phone layouts.

### Scale modes

- **Use Project DPI**: returns the project DPI scale unchanged.
- **Fixed**: returns `Target UI Scale` as the final scale.
- **Curve**: evaluates `Scale Curve` on `Short Side`, `Long Side`, `Screen Width`, or `Screen Height`. The curve value is the final scale, not a multiplier.

New rules default to a width curve from `(0, 0)` to `(1920, 1)`. Tune this curve for continuous responsive scaling, or use `Fixed` for a discrete target scale.

### Common configurations

| Rule order | Match | Scale mode | Use case |
| --- | --- | --- | --- |
| 1. Compact phone | Width ≤ 720 | Fixed 0.80 | Small portrait layouts |
| 2. Phone | Width ≤ 1440 | Curve / Fixed | General phones, including high-density devices |
| 3. Default | No conditions | Use Project DPI | Tablets, desktop, and fallback |

The compact rule must be first because it also satisfies the wider phone rule.

## Runtime updates

Use the Blueprint node **Set DPI Rules** to replace the whole array at runtime. The widget invalidates its layout immediately, so the new result is applied on the next Slate pass.

## Designer preview

Select one **DPI Scaler** in the UMG Designer to see rulers above and to the left of the widget:

- The top ruler shows the currently valid width ranges.
- The left ruler shows the currently valid height ranges.
- Colors are deterministic from rule order. The active rule is brighter; inactive ranges remain muted.
- Aspect-ratio and opposite-axis conditions clip the visible range correctly.
- The status text shows the active rule and final scale. **Designer Mute** temporarily returns scale `1.0` in the Designer.

Rules are edited in the Details panel; the rulers are a visual aid, not a direct editor for breakpoints.

## Testing

Automation tests cover rule precedence, width/height and aspect-ratio boundaries, scale outputs, and a worst-case rule-search benchmark. Run:

```text
DPIScaler.Rules
DPIScaler.Performance.RuleSearch
```

from Unreal's Automation window.
