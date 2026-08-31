# DPI Scaler

![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.7-0E8A16?logo=unrealengine&logoColor=white)
![Version](https://img.shields.io/badge/version-1.0.0-0E7AC4)
![License](https://img.shields.io/github/license/dev-denis-korsunov/DPIScaler)
![UMG](https://img.shields.io/badge/UMG-Widget-7952B3)
![Blueprint](https://img.shields.io/badge/Blueprint-Ready-137CBD)

<img width="1416" height="858" alt="image 11" src="https://github.com/user-attachments/assets/f41f5e67-c413-4633-9cd8-58a7a572372d" />



DPI Scaler is an **Unreal Engine 5 UMG plugin** for resolving responsive-layout conflicts in a specific UI subtree. It gives that branch an independent, predictable scale context when the project DPI curve and ordinary layout rules lead to overlapping, clipping, or unstable composition at unusual viewport sizes.

Rather than maintaining exact per-device resolution profiles, define a small ordered set of broad viewport rules and let the scaler select the first one that matches. This keeps exceptional mobile, tablet, aspect-ratio, and edge-resolution cases contained to the affected UI without changing the project-wide DPI curve.

Keep DPI Scaler local to the UI branch that needs it. Avoid placing it at the root of a large tree: UMG Designer always uses the project `DPI Scale` from Project Settings, so a global scaler cannot be previewed with a correct local scale context and may affect unrelated widgets at runtime.

## What it adds

- **Local DPI scaling** that isolates exceptional responsive cases without changing the project's global DPI settings.
- **Scale Reference** can use the project DPI or inherit the absolute scale from the nearest parent DPI Scaler.
- **Ordered viewport rules** with optional width, height, and aspect-ratio constraints.
- **Project DPI**, **Fixed**, and **Curve** scale modes.
- The Blueprint function `IsActiveRule` lets you determine the active resolution range by its name without changing the DPI Scale.
- **Designer rulers** that visualize which width and height ranges match each rule.

## Installation

Enable the DPI Scaler plugin in Unreal Editor. The DPI Scaler widget appears in the UMG Palette after the plugin loads.

## Basic setup

1. Add a DPI Scaler to a Widget Blueprint.
2. Give it one child panel — usually a `Canvas Panel`.
3. Add rules in **DPI Rules**. Array entries use the rule `Name` as their title.
4. Put the most specific rule first; the first matching rule wins.

Recommended hierarchy:

```text
DPI Scaler
└── Canvas Panel
    └── UI content
```

Keep the scaler close to the content it controls. This makes its rules and scale context explicit when the complete widget tree is assembled at runtime.

Do not wrap a `Scale Box` with a DPI Scaler, or place a DPI Scaler inside a `Scale Box`, in the same layout branch. Use one scaling mechanism for that branch. Nested DPI Scalers are supported intentionally: an inner scaler resolves its local scale relative to the outer scaler rather than multiplying both final scales.

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

### Scale reference

`Scale Reference` controls the fallback scale used by rules in `Use Project DPI` mode:

- **Project DPI** uses the viewport's project DPI scale.
- **Incoming DPI** inherits the absolute scale from the nearest parent DPI Scaler; without such a parent it falls back to Project DPI.

`Fixed` and `Curve` values are always final absolute UI scales. A `Scale Box` or arbitrary render transform is not treated as an incoming DPI context.

## Blueprint logic

Use **Is Active Rule** with a rule name to branch Blueprint logic for the rule currently selected by the viewport.

## Designer preview

Select one DPI Scaler in the UMG Designer to see rulers above and to the left of the widget:

- The top ruler shows the currently valid width ranges.
- The left ruler shows the currently valid height ranges.
- Colors are deterministic from rule order. The active rule is brighter; inactive ranges remain muted.
- Aspect-ratio and opposite-axis conditions clip the visible range correctly.
- The status text shows the active rule and final scale. **Designer Mute** temporarily uses the preview project DPI scale in the Designer.

Rules are edited in the Details panel; the rulers are a visual aid, not a direct editor for breakpoints.

## Testing

Automation tests cover rule precedence, width/height and aspect-ratio boundaries, scale outputs, and a worst-case rule-search benchmark. Run:

```text
DPIScaler.Rules
DPIScaler.Performance.RuleSearch
```

from Unreal's Automation window.
