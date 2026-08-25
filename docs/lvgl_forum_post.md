# Animating complex widget trees on MCU — a raster-buffer caching experiment

**Setup**: Cortex-M33 @240MHz, 1MB SRAM + 6MB PSRAM, LVGL v9.2. Complex widget: 50+ colored rectangles with rounded corners in a grid layout. All timings via `lv_tick_get()` (1 tick = 1ms).

[Test code](https://github.com/ElenixOS/ElenixOS/blob/dev/src/apps/test/snapshot/eos_test_snapshot.c)

---

## The problem

When animating complex widget trees, LVGL re-renders the entire subtree every frame — layout, style resolution, and full software draw of every child. Even when only one property changes (scale, position, opacity).

On our Cortex-M33 with a 50-child widget (a typical settings page), **combined translate + scale + fade animations** show severe frame drops — the worst frame in a 6-frame window reaches 159ms. The UI stutters visibly and is effectively unusable for smooth transitions.

| Single per-frame cost (direct) | 60fps budget |
|---|---|
| Combined effects | ~22ms | 16.7ms |

---

## What we tried

Before starting the animation, we take a **one-time `lv_snapshot_take_to_draw_buf()`** of the widget, then animate the resulting flat `lv_image` instead of the live tree. Memory cost: w × h × 2 bytes (RGB565).

In other words: pay 1 frame of rendering upfront, then each subsequent frame is a cheap image blit rather than a full subtree walk.

---

## Results

### Snapshot capture overhead (one-time)

| Size | Pixels | SRAM | PSRAM |
|------|--------|------|-------|
| 100×100 | 10K | 3ms | 2ms |
| 200×200 | 40K | 13ms | 14ms |
| 300×300 | 90K | 24ms | 29ms |
| 390×450 | 175K | 25ms | 34ms |

A fullscreen (390×450) snapshot costs only 4% more than a 300×300 — suggesting the overhead is dominated by draw-task dispatch, not pixel throughput.

### Animation cost — snapshot vs. direct (6 frames, avg / max / min)

| Effect | Snapshot mode | Direct mode |
|--------|--------------|-------------|
| Translate | 55 / 56 / 55 | 67 / 69 / 64 |
| Scale | 60 / 62 / 56 | 69 / 71 / 63 |
| Opacity | 55 / 56 / 55 | 84 / 89 / 63 |
| **Combined** | **60 / 62 / 57** | **132 / 159 / 63** |

Combined effects: snapshot is **55% faster** overall.

### Frame jitter (max − min)

| Effect | Snapshot | Direct |
|--------|----------|--------|
| Translate | 1ms | 5ms |
| Scale | 6ms | 8ms |
| Opacity | 1ms | 26ms |
| **Combined** | **5ms** | **96ms** |

This is the real headline. Direct-mode combined animation has **96ms jitter** — some frames are fine, some are catastrophically late. Snapshot mode stays within 5ms every frame.

### PC contrast (Apple M2)

| Effect | Snapshot | Direct |
|--------|----------|--------|
| Combined | 52ms | 50ms |

On a fast CPU the difference disappears entirely, confirming this is MCU-specific.

---

## Real-world usage

We're building ElenixOS (a watch OS targeting Cortex-M33), and the app launcher transition already uses this pattern. Two fullscreen (390×450) snapshots scale simultaneously — maintaining 20+ FPS. Without raster caching, animating even one fullscreen widget tree on this hardware drops below 10 FPS.

(Video/GIF attached — downsampled to 15 FPS; actual device runs 20+ FPS.)

---

## Observations

1. **Benefit scales with complexity.** More children → larger gap. At 50 children, combined effects are 55% faster.
2. **Even opacity alone benefits.** Blending 1 flat image is cheaper than blending 50 individual children widget-by-widget.
3. **Jitter is the real win.** 96ms → 5ms jitter is the difference between unusable and smooth.
4. **Not universally beneficial.** For a single `lv_label`, direct wins — the snapshot setup costs more than one label-render frame. This is an optimization for **complex subtrees**, not a global replacement.

---

## Caveats

- **Memory**: Fullscreen RGB565 = 351KB. Works for PSRAM platforms, impractical for pure 64KB SRAM.
- **PSRAM latency**: QSPI-accessed PSRAM adds 15–30% overhead on the snapshot step (one-time, acceptable).
- **Stale content**: The raster is frozen. Acceptable for short animations (100–500ms); longer ones need re-snapshot.
- **Complements LVGL, doesn't replace it.** This doesn't touch the render loop — it's a higher-level pattern.

---

## Questions for the community

1. Have others run into this? What workarounds have you used?
2. Is this a common enough pattern that it should be a first-class LVGL utility (e.g., a `lv_snapshot_anim` helper)?
3. Or is it better kept as a documented pattern that each project implements downstream using the existing `lv_snapshot` + `lv_image` + `lv_anim` APIs?

Curious to hear what approaches others have taken for smooth animations on MCU-class hardware.
