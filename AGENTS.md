# AGENTS.md — ElenixOS Simulator

## Project Overview

This is the **simulator** for ElenixOS — it builds a native desktop app (SDL2) or a WASM web app that runs the ElenixOS operating system. The core OS lives in the `ElenixOS/` submodule as a static library; this repository provides the platform layer and entry point.

- **Entry point**: `main/src/main.c`
- **Platform port implementations**: `main/src/port/` (display, audio, sensors, battery, vibrator, time, power)
- **Build System**: CMake (C99, with C++17 for FreeRTOS wrapper)
- **Platforms**: Native (SDL2) and WASM (Emscripten)

## Submodule

The ElenixOS core lives at `ElenixOS/`. Its AGENTS.md at `ElenixOS/AGENTS.md` defines constraints for the OS code. When modifying files under `ElenixOS/`, follow that document. When modifying `main/src/`, follow this one.

## Architecture Gate

Same as `ElenixOS/AGENTS.md` — before touching anything in `ElenixOS/src/` (script engine, SNI, framework, port interfaces, kernel), you **MUST** consult [ElenixOS-Docs](https://github.com/ElenixOS/ElenixOS-Docs), with fallback to `{WorkSpace}/../ElenixOS-Docs/`. If neither is found, **STOP**.

## Build Commands

### Prerequisites

- **Native**: CMake ≥ 3.10, SDL2, Python 3
- **WASM**: Emscripten SDK
- **Python deps**: `pip install -r requirements.txt`

### Native (macOS / Linux)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(sysctl -n hw.ncpu 2>/dev/null || nproc)

# Run
./bin/main
```

### WASM

```bash
cmake -B build-wasm -DCMAKE_BUILD_TYPE=Debug -DEOS_PLATFORM=WASM
cmake --build build-wasm -j$(sysctl -n hw.ncpu 2>/dev/null || nproc)
```

### Other variants

```bash
# AddressSanitizer
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DASAN=ON
cmake --build build

# FreeRTOS (native only)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DUSE_FREERTOS=ON
cmake --build build

# Kconfig
cmake --build build --target menuconfig
```

## Logs

Native builds write logs to `tmp/latest.log` with timestamped archives at `tmp/YYYY-MM-DD_HH-MM-SS.log`. After making changes that affect runtime behavior, check this file for errors.

## Code Quality — MUST

### For code under `ElenixOS/`

Run `check.py` from the ElenixOS submodule root:

```bash
cd ElenixOS
pip install -r scripts/requirements.txt pyyaml
python3 scripts/check.py --fix
python3 scripts/check.py
```

### For code under `main/src/`

`check.py` does **not** scan `main/src/`. You **MUST** manually apply the same conventions:

- **Formatting**: follow `.clang-format` in `ElenixOS/` — run `clang-format-20 -i --style=file:ElenixOS/.clang-format <file>` for files under `main/src/`
- **Naming**: match the surrounding code — `_` prefix for static functions, `eos_port_` for port implementations, `hal_` for HAL helpers
- **File structure**: same Doxygen `@file` header and 65-char section dividers as ElenixOS code
- **Memory**: use `eos_malloc()`/`eos_free()` — these are provided by the OS core
- **Warnings**: resolve ALL compiler warnings

## Testing — MUST Inform the User

This is a graphical simulator. Changes that affect behavior **MUST** be verified by running the simulator. Tell the user:

1. Build: `cmake --build build`
2. Launch: `./bin/main`
3. Interact with the simulator to verify the change visually
4. Check `tmp/latest.log` for runtime errors

Do **NOT** claim something works without the user actually running it.

## Security — MUST

- **MUST** check all `eos_malloc()` return values for NULL
- **MUST NOT** expose raw native pointers to the JS engine (SNI boundary)
- **MUST** enable ASAN (`-DASAN=ON`) when debugging memory issues

## Performance Pitfalls

### `LV_USE_ASSERT_MEM_INTEGRITY` — perf cost awareness

In `lv_conf.h`, `LV_USE_ASSERT_MEM_INTEGRITY 1` calls `lv_mem_test()` after **every** LVGL timer callback (`lv_timer.c:335`), object-tree operation (`lv_obj_tree.c:86,115`), and draw operation (`lv_draw_rect.c:295`, `lv_draw_label.c:374`).

`lv_mem_test()` walks the entire TLSF memory pool checking every block header. With the built-in TLSF allocator (`LV_STDLIB_BUILTIN`), each suspended app (recents) adds more allocated blocks → integrity check time grows linearly → FPS drops with recents count.

This option is **useful for catching heap corruption** during development, but **MUST be disabled when doing any frame-rate, scrolling, or animation performance testing.** The other asserts (`LV_USE_ASSERT_NULL`, `LV_USE_ASSERT_MALLOC`) are O(1) and safe to leave on.

**Discovery**: An Instruments Time Profiler trace showed `lv_tlsf_check` / `integrity_walker` / `block_is_free` dominating the frame budget, with ~500-900 µs extra per frame per suspended app from memory integrity checks running inside the render and timer dispatch paths.

## Commit Messages — MUST

After completing changes, **MUST** output a suggested commit message in Conventional Commits format. **MUST NOT** run `git commit` or `git push` — the user does this manually.
