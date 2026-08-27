# DPI Scaler

`DPI Scaler` is a runtime UMG container widget that scales one child according to simple DPI breakpoint rules. It is useful when the project's global DPI curve is not enough and a specific UI subtree needs its own scaling behavior.

## Installation

Enable the **DPI Scaler** plugin in Unreal Editor, then restart the editor if prompted. The widget appears in the UMG Palette as **DPI Scaler**.

## Basic usage

1. Add a **DPI Scaler** widget to a Widget Blueprint.
2. Add one child widget to it.
3. Configure one or more entries in **DPI Rules**.
4. Give more specific rules a higher **Priority**.

The recommended hierarchy is:

```text
DPI Scaler
└── Canvas Panel
    └── UI content
```

Use the DPI Scaler as the root container for the UI subtree that must be scaled. Keep its child as a single panel, typically a `Canvas Panel`.

## DPI breakpoint rules

Each enabled rule contains:

- **Name**, **Enabled**, and **Priority**.
- **Match**: orientation and a minimum or maximum short side.
- **Advanced Match**: minimum or maximum width, height, and aspect ratio.
- **Scale Mode**: `Use Project DPI`, `Fixed`, or `Curve`.
- Optional **Limits**: minimum scale, maximum scale, and snap step.

Zero means unbounded or disabled for optional bounds and limits. `Fixed` and `Curve` produce a final **Target UI Scale**, not a multiplier.

Rules resolve as follows:

1. The matching enabled rule with the highest priority wins.
2. If priorities are equal, the first rule in the array wins.
3. If no rule matches, the project DPI scale is used.
4. Limits and snap are applied to the selected result, and the final scale is kept positive.

A practical starting set is:

| Rule | Match | Scale |
| --- | --- | --- |
| Mobile | Short Side ≤ 720 | Fixed 0.80 |
| Tablet | Short Side ≤ 1080 | Fixed 0.95 |
| Default | Any | Use Project DPI |

Assign priorities such as `100`, `50`, and `0` respectively.

## Runtime updates

For runtime changes, use the Blueprint node **Set DPI Rules**. It replaces the rule array and invalidates the Slate layout, so the new scale is applied immediately.

## Layout guidelines

- Prefer `DPI Scaler → Canvas Panel → content`.
- Do not place a `DPI Scaler` inside a `Scale Box`, and do not wrap a `Scale Box` with a `DPI Scaler` in the same layout branch. Both widgets calculate scale from layout geometry, which can lead to double scaling or unstable desired sizes.
- Avoid nesting DPI Scalers unless it is intentional. Each nested scaler resolves its own final target scale relative to the parent scaler.
- Test at all target resolutions and aspect ratios, especially if anchors, fixed canvas offsets, or `Scale Box` are involved.

## Editor preview

The widget responds to UMG screen preview settings. Enable **Designer Mute** to temporarily disable its custom scale in the designer.

When a single **DPI Scaler** is selected in the UMG Designer, the plugin draws responsive rulers above and to the left of the widget:

- The top ruler represents viewport width.
- The left ruler represents viewport height.
- `W` and `H` show the current preview viewport size.
- Each enabled rule gets a stable palette color. Colored ranges show where that rule can match for the current opposite axis, including orientation and aspect-ratio conditions.
- Each ruler is a slice through the complete two-dimensional match region: a width range is shown only when the current height satisfies the rule, and a height range is shown only when the current width satisfies it.
- Higher-priority rules paint over lower-priority rules only inside their own ranges. The currently active rule is rendered with dominant opacity; inactive ranges remain visible but muted.
- Colored markers show exact width, height, and short-side breakpoints from **DPI Rules**.
- A compact status inside the top ruler shows the active rule and final target scale.

The rulers are a visual editing aid. Breakpoint values are currently edited in the **DPI Rules** array in the Details panel. Collapsed array entries show summaries such as `Mobile • Short Side ≤ 720 • Fixed 0.80 • P100`.

## Compatibility

The plugin contains a runtime module and an editor-only UMG Designer extension for Unreal Engine 5. The runtime module depends on `UMG`, `Slate`, and `SlateCore`.

The old Media Queries API is intentionally not retained. Projects upgrading from an earlier plugin revision must recreate old rules with `FDPIBreakpointRule` and replace `Set Media Queries` Blueprint nodes with `Set DPI Rules`.
