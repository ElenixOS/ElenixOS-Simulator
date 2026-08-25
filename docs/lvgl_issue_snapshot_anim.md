---
name: Feature planning
description: Discuss and design new features
title: "[Discussion] High per-frame cost when animating complex widget trees on MCU"
labels: ["discussion", "optimization", "animation", "snapshot"]
---

### Problem to solve

When animating complex widget trees on MCU, LVGL re-renders the entire widget subtree every animation frame — including layout recalculation, style resolution, and full software draw of every child. Even when only a single property changes (scale, position, opacity).

On Cortex-M33 @240MHz with a 50+ child widget (typical settings page), combined animations (translate + scale + fade) see **severe frame drops and 96ms jitter**, making UI animations unusable.

### Success criteria

Maintainers agree that the per-frame cost of animating complex widget trees is a problem worth addressing, and that **pre-rendering into a raster buffer** is a valid direction to explore — whether as an upstream feature or as a documented optimization pattern.

### Solution outline

One idea we tried: render the widget **once** into a raster buffer before the animation starts, then animate the resulting flat image instead of the live widget tree. This trades a modest amount of memory for a significant reduction in per-frame CPU work — each animation frame becomes a cheap image blit rather than a full subtree re-render.

**Benchmark setup**: Cortex-M33 @240MHz, 1MB SRAM + 6MB PSRAM, LVGL v9.2. Complex widget: 50+ colored rectangles with rounded corners in grid layout. All timings via `lv_tick_get()` (1 tick = 1ms). [Test code](https://github.com/ElenixOS/ElenixOS/blob/dev/src/apps/test/snapshot/eos_test_snapshot.c).

**Snapshot capture overhead** (one-time cost):

| Size | Pixels | SRAM | PSRAM |
|------|--------|------|-------|
| 100×100 | 10K | 3ms | 2ms |
| 200×200 | 40K | 13ms | 14ms |
| 300×300 | 90K | 24ms | 29ms |
| 390×450 | 175K | 25ms | 34ms |

A 175K-pixel fullscreen snapshot costs only 4% more than a 90K-pixel one. This suggests the overhead is dominated by LVGL's rendering pipeline rather than raw pixel throughput.

**Animation cost** (6 frames, format: `avg / max / min ticks`):

| Effect | Snapshot mode | Direct mode | Improvement |
|--------|--------------|-------------|-------------|
| Translate | 55 / 56 / 55 | 67 / 69 / 64 | 18% |
| Scale | 60 / 62 / 56 | 69 / 71 / 63 | 13% |
| Opacity | 55 / 56 / 55 | 84 / 89 / 63 | **35%** |
| **Combined** | **60 / 62 / 57** | **132 / 159 / 63** | **55%** |

**Frame jitter** (max-min, lower = smoother):

| Effect | Snapshot | Direct | Factor |
|--------|----------|--------|--------|
| Translate | 1ms | 5ms | 5× |
| Scale | 6ms | 8ms | 1.3× |
| Opacity | 1ms | 26ms | **26×** |
| **Combined** | **5ms** | **96ms** | **19×** |

**PC contrast** (Apple M2, same test): Combined snapshot 52ms vs direct 50ms — difference disappears on a fast CPU, indicating this is specifically an MCU-class optimization.

**Key takeaways**:
- Benefit scales with widget complexity: the more children, the larger the gap
- Even opacity alone benefits: blending 1 flat image is cheaper than blending 50 individual children
- Frame jitter is the real win: 96ms → 5ms is the difference between unusable and perfectly smooth

**Real-world usage**: ElenixOS (a watch OS under development targeting Cortex-M33) uses this approach for its app launcher transition. Two fullscreen (390×450) snapshots are animated simultaneously — both fullscreen (390×450) snapshots scaling simultaneously — maintaining 20+ FPS. Without the snapshot approach, animating even one fullscreen widget tree on this hardware drops below 10 FPS.

(Attached: video/GIF of the launcher transition. Note: the GIF is downsampled to 15 FPS — actual on‑device frame rate is 20+ FPS.)

### Rabbit holes

1. **Memory cost**: Fullscreen RGB565 = 351KB. Viable for PSRAM-equipped MCUs, impractical for pure SRAM devices (64-256KB). Any implementation should be compile-time optional.
2. **PSRAM bandwidth**: QSPI-accessed PSRAM adds 15-30% latency vs SRAM for the snapshot step. One-time cost, should be acceptable.
3. **Stale snapshot**: The raster buffer is frozen. If widget content changes mid-animation, the snapshot shows outdated content. Acceptable for short animations (100-500ms); longer animations may need a re-snapshot mechanism.
4. **Not universally beneficial**: For simple widgets (single label), direct animation wins — the snapshot overhead exceeds one frame of label rendering. The approach is an optimization for complex subtrees, not a global replacement.
5. **Complements, not replaces**: This doesn't fundamentally change how LVGL animates — it's an optional optimization for specific use cases.

### Testing

The [test harness](https://github.com/ElenixOS/ElenixOS/blob/dev/src/apps/test/snapshot/eos_test_snapshot.c) measures both snapshot capture time and per-frame animation cost under controlled conditions. It could be adapted into an LVGL benchmark scene for automated A/B comparison.

### Teaching

- Document the trade-off (memory for CPU) and when the approach is beneficial vs when it isn't
- Provide an example scene showing the visual difference on a complex widget with and without the optimization
- A config flag (`LV_USE_SNAPSHOT_ANIM` or similar) with platform-specific guidance for when to enable

### Considerations

- **Deeper integration**: `LV_OBJ_FLAG_RENDER_CACHE` — the refresh loop automatically caches marked objects. More elegant but invasive (touches `lv_refr.c`). A utility-layer approach is a safer first step.
- **DIY downstream**: Users can already implement this pattern themselves using `lv_snapshot_take` + `lv_image` + `lv_anim`, as demonstrated in the linked test code and the app launcher described above. The question is whether this is a common enough pattern to warrant a first-class utility in LVGL itself.
- **GPU offload (VG-Lite/PXP)**: Different hardware path with different bottlenecks — this approach is for software-rendered platforms.
- **Do nothing**: Accept that developers must simplify widget trees for animation. Works, but limits UI expressiveness.
