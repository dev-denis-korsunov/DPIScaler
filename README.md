# DPI Scaler

`DPI Scaler` is a runtime UMG container widget that scales one child according to DPI media-query rules. It is useful when the project's global DPI curve is not enough and a specific UI subtree needs its own scaling behavior.

## Installation

Enable the **DPI Scaler** plugin in Unreal Editor, then restart the editor if prompted. The widget appears in the UMG Palette as **DPI Scaler**.

## Basic usage

1. Add a **DPI Scaler** widget to a Widget Blueprint.
2. Add one child widget to it.
3. Configure one or more entries in **Media Queries**.
4. Enable only the overrides required by a query.

The recommended hierarchy is:

```text
DPI Scaler
└── Canvas Panel
    └── UI content
```

Use the DPI Scaler as the root container for the UI subtree that must be scaled. Keep its child as a single panel, typically a `Canvas Panel`.

## Media queries

Each query can be limited by:

- Minimum and maximum viewport width or height
- Minimum and maximum aspect ratio

When a query matches, it can modify the scale with any combination of:

- **DPI Scale Override** — use an exact scale.
- **Min DPI Scale** / **Max DPI Scale** — clamp the current scale.
- **DPI Curve** — evaluate a runtime float curve by short side, long side, width, or height.
- **Snap DPI Scale Grid** — round the scale to a grid value.

Queries are evaluated in array order. `Override` replaces the current result; `Min` and `Max` combine the result with the current value.

## Runtime updates

For runtime changes, use the Blueprint node **Set Media Queries**. It replaces the media-query array and invalidates the Slate layout, so the new scale is applied immediately.

## Layout guidelines

- Prefer `DPI Scaler → Canvas Panel → content`.
- Do not place a `DPI Scaler` inside a `Scale Box`, and do not wrap a `Scale Box` with a `DPI Scaler` in the same layout branch. Both widgets calculate scale from layout geometry, which can lead to double scaling or unstable desired sizes.
- Avoid nesting DPI Scalers unless it is intentional. Nested scalers compose correctly, but each level applies its own media-query rules.
- Test at all target resolutions and aspect ratios, especially if anchors, fixed canvas offsets, or `Scale Box` are involved.

## Editor preview

The widget responds to UMG screen preview settings. Enable **Designer Mute** to temporarily disable its custom scale in the designer.

When a single **DPI Scaler** is selected in the UMG Designer, the plugin draws responsive rulers above and to the left of the widget:

- The top ruler represents viewport width.
- The left ruler represents viewport height.
- `W` and `H` show the current preview viewport size.
- Colored markers show enabled minimum and maximum width or height constraints from **Media Queries**.

The rulers are a visual editing aid. Breakpoint values are currently edited in the **Media Queries** array in the Details panel.

## Compatibility

The plugin contains a runtime module and an editor-only UMG Designer extension for Unreal Engine 5. The runtime module depends on `UMG`, `Slate`, and `SlateCore`.
