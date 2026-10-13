# TinyTTF 字号实例资源测量（第一阶段）

> **2026-10-13 策略更新：** 当前代码不再使用 24 个实例上限，支持范围内请求的字号会按需创建并保留到字体系统关闭。旧的 v0.7 上限测试与选择记录保留为历史数据。内存与文件句柄的常驻增长记为 v0.8 TODO。

## 范围和环境

本次只新增独立测量程序及原始数据，没有修改 ElenixOS 字体、Theme、SNI、生成器或现有应用。测试程序直接链接 Simulator 当前构建的 LVGL 静态库，在独立进程中调用文件型 TinyTTF API；因此测量结果不包含真实 UI、应用、脚本引擎或显示刷新所占资源。

- 平台：macOS 27.0.1、arm64；LVGL checkout `v9.6.0-49-g389dbdeef`。
- 字库：`fs/.sys/res/font/font.ttf`，SHA-256 `6c8e5b6639d46d38962ffb91bc43c13bbffa61752999b3287ac1245ca59f1c5c`，约 1.6 MB。
- 配置：`LV_USE_TINY_TTF=1`、文件支持开启、glyph/draw data 缓存各 256 项、kerning 缓存 256 项、LVGL 内建分配池 5,242,880 B、内存完整性断言关闭。
- 内存口径：`lv_mem_monitor()` 的 `total_size - free_size`，包含 LVGL 池内分配开销；文件系统的 `FILE` 分配、系统页缓存不在该数字中。另记录进程 RSS 和 `fcntl(F_GETFD)` 统计的文件描述符。RSS 以页为单位变化，不适合用来归因单个实例。
- 时间口径：`CLOCK_MONOTONIC`；创建时间是文件型实例创建调用的耗时。冷字形是该实例第一次请求 glyph 描述和 bitmap，热字形是随后反复请求。它们是 API 微基准，不等于屏幕绘制帧时间，也不是冷磁盘读取测试。

完整原始数据见 [2026-10-10-simulator.csv](results/2026-10-10-simulator.csv)。常见文本使用 24 个不同的中文、26 个英文大写字母、10 个数字，每个字号各创建一个实例；热路径对每组重复 200 次。表中创建耗时取 5 个独立进程的中位数；字形内存数据在 5 次运行中一致。

## 实例与常见文本开销

每个空实例在 LVGL 池内增加 **752 B / 5 个分配块**，并保持 **1 个文件描述符**。8 个字号空实例总计增加 6,016 B 和 8 个描述符。下表的中文、英文、数字列分别是首次使用该组字形后额外增加的 LVGL 池字节数；三列相加是该字号 60 个字形及 kerning 的缓存增长。

| px | 创建中位数 µs | 中文 24 字 B | 英文 26 字 B | 数字 10 字 B | 合计 B | 描述符 |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 12 | 98 | 14,088 | 13,128 | 4,968 | 32,184 | +1 |
| 16 | 81 | 16,752 | 14,728 | 5,520 | 37,000 | +1 |
| 18 | 77 | 18,360 | 15,600 | 5,840 | 39,800 | +1 |
| 24 | 78 | 23,960 | 18,376 | 6,712 | 49,048 | +1 |
| 26 | 77 | 26,040 | 19,864 | 7,200 | 53,104 | +1 |
| 32 | 79 | 33,176 | 23,760 | 8,624 | 65,560 | +1 |
| 48 | 76 | 60,416 | 38,208 | 13,472 | 112,096 | +1 |
| 64 | 82 | 97,272 | 58,312 | 20,448 | 176,032 | +1 |

8 个实例加载上述常见文本后，LVGL 池比基线多 **570,840 B**，描述符多 **8 个**。首个创建调用可能受宿主文件页缓存影响，表中的微秒值只能说明热宿主环境下的量级。

## 缓存命中、容量与回收

对 5 次运行、8 个字号的每个类别计算每字形耗时中位数：

| 字形组 | 首次请求 µs/字形 | 缓存命中 µs/字形 | 首次/命中 |
| --- | ---: | ---: | ---: |
| 中文 | 150.938 | 0.387 | 390 倍 |
| 英文 | 77.038 | 0.418 | 184 倍 |
| 数字 | 89.400 | 0.408 | 219 倍 |

容量压力测试先从该字库的 U+4E00–U+9FFF 中选出 256 个实际存在的汉字，再对每个字号分别请求 bitmap，并重复遍历 10 次。当前字库并不覆盖起始连续 256 个码点，因此使用“实际存在”集合。默认 256 项缓存的结果如下：

| px | 256 字后该实例增量 B | 8 个实例累计增量 B |
| ---: | ---: | ---: |
| 12 | 152,792 | 158,808 |
| 16 | 180,160 | 338,968 |
| 18 | 197,512 | 536,480 |
| 24 | 258,448 | 794,928 |
| 26 | 280,872 | 1,075,800 |
| 32 | 359,472 | 1,435,272 |
| 48 | 651,640 | 2,086,912 |
| 64 | 1,053,272 | **3,140,184** |

3,140,184 B 是独立进程中上述 8 个字号的占用，约为 Simulator 5 MiB LVGL 池的 **59.9%**。真实系统还要与 UI、显示和其他 LVGL 资源共享该池。256 项缓存对这组 256 字的后续遍历为约 **0.5–0.6 µs/字形**。

作为对照，仅通过 LVGL 的 `lv_tiny_ttf_create_file_ex(..., cache_size=64)` 把每个实例的 glyph/draw data 缓存限制为 64 项：相同 8 个字号遍历 256 字后约占 **987,320 B**，但第二至第十次遍历持续发生淘汰与重建，本次运行耗时约 **128–239 µs/字形**（宿主运行间有波动）。常见的 60 字工作集能容纳于 64 项缓存，测得总内存及命中耗时与 256 项配置基本相同。直接把全系统缓存降到 64 项会使较大的中文工作集失去缓存收益，不宜仅为驻留方案这样做。

默认缓存与 64 项缓存下，8 个实例按逆序销毁后，LVGL 池占用、块数、描述符数均回到各自的 `lv_init()` 基线，`lv_mem_test()` 成功。常见文本两种缓存配置各重复 5 个进程，每次又进行 20 次“创建 26px → 使用 60 字 → 销毁”的同进程循环；共 200 次循环均回到基线。RSS 在销毁后未立即回落，不能由此判定字体泄漏；进程分配器及系统页驻留需另测。

## 方案选择

**推荐 B：按字号共享实例，活跃对象持有引用，最后一个引用消失后在 LVGL 安全点销毁。** 常见文本下 A 看起来便宜，但 8 个已测字号填满缓存后占掉独立进程 LVGL 池约 60%；有界驻留还会在达到字号数量上限后拒绝新字号，即使旧字号已不再使用。64 项缓存能降低占用，却会使大中文工作集持续重新生成字形。B 也必须限制同时活跃的不同字号，并在分配失败或达到限制时保持对象原样并返回明确错误；具体数量和内存门槛应由 E84 实测后确定。默认 26px 字体应固定存活至字体系统关闭，作为 Theme 和继承字体的根引用。

这是基于资源测量对长期方案的建议。后续 v0.7 发布范围曾选择更小的过渡实现 A：按字号共享、容量 24 的实例表。该决定已由 2026-10-13 的当前指示覆盖：不设 24 个实例硬上限，先支持所有可请求字号；内存与文件句柄回收留待 v0.8。

最小 B 只需要“字号 → TinyTTF 实例/引用数”的表，不开发字形缓存、LRU、通用字体描述对象或 JS 字体对象。C/JS 设置操作必须先成功取得新实例及对象记录，再改 LVGL 样式，最后释放旧引用。对象删除、再次设置、样式覆盖和字体系统关闭都要执行同一套配对清理。现有 `eos_wdata_set()` 返回 `void`，分配失败时静默返回，覆盖同类型数据时不调用旧析构函数；直接拿它记字体引用会泄漏或提前释放，需要先修正该合同或使用专门且可检查的对象附着记录。运行时字体重载路径应禁用，不能释放 Theme 或对象仍在使用的字体。

## E84 必须补测的项目

E84 使用 `LV_STDLIB_CUSTOM → eos_malloc()`，其 16 MiB PSRAM 分配器在就绪时优先分配到 PSRAM，失败后可能回退到 newlib heap；这与 Simulator 的 5 MiB LVGL 内建池不同。E84 的 TinyTTF 同样使用文件模式，LittleFS 文件配置的 cache size 为 8,192 B，文件句柄内存与打开/关闭成本须在板上测得。Simulator 的字节数、微秒数和 `FILE` 描述符数都不能直接套用到 E84。

正式确定活跃字号上限前，应在 E84 上测：冷启动可用 PSRAM 与 newlib heap、每个字号的实际分配和峰值、同时打开 8 个字库文件的 LittleFS 缓存及句柄、字形首次/命中耗时、256 汉字压力下的实时 UI 帧率、销毁后的堆与文件回收，以及多次切换应用后的碎片与大块可用内存。E84 的分配器每块有 EOS 和 PSRAM 元数据，Simulator 记录的 22,544 个压力测试分配块意味着 MCU 侧额外开销可能可观。板上未测前，不应承诺具体的安全上限。

## v0.7 实施状态与剩余风险

- TinyTTF 实现统一放在 `ElenixOS/src/ui/font/eos_font.c`；不再有 `eos_font_ttf.c` 或静态 `RemixIcon.c` 的常规渲染依赖。`EOS_FONT_TYPE` 已移除，常规文本与图标都使用 TinyTTF。
- Text 字体从 `/.sys/res/font/font.ttf` 加载；Icon 字体从 `/.sys/res/font/remixicon.ttf` 加载。Simulator CMake 将 RemixIcon TTF 从第三方子模块复制到运行时系统资源目录；E84 通过 LittleFS 资源目录提供同名文件。
- Text/字号实例的 `fallback` 指向相同像素大小的 Icon 实例；Icon 的 fallback 指向静态 LVGL rescue 字体以保留 `LV_SYMBOL_*`。实例按来源和像素大小共享，没有 24 个实例上限；Text 字号首次创建时，Text 与同字号 Icon 会一起创建或一并回滚。实例目前保留到字体系统关闭。
- `setFontSize(px)` / `clearFontSize()` 保持原 JS 调用方式并注册在 `lv.obj`；清除后回到 LVGL 原生样式继承。旧 C 字号枚举和包装函数保留，枚举数值作为精确像素传入。字体热重载关闭。
- `eos_icon_glyph_image_create()` 使用对应字号的 Icon TTF，复制 TinyTTF 返回的缓存位图到 Image 自有 A8 buffer，再释放 TinyTTF glyph cache 引用；对象删除时释放自有 buffer。
- `EOS_FONT_TYPE` 及静态普通字号后端已移除。`RI_*` 头文件的 2,987 个图标码点都存在于部署 TTF cmap 中；`RI__NULL` 和 `RI_NONMARKINGRETURN` 是两个控制码，不是图标字形。映射校验脚本为 `tests/font/verify_remixicon_cmap.py`。
- Simulator 双字体 smoke 的旧结果（2026-10-10）：当时的 24 实例策略下，常见字号与 Image A8 生命周期测试通过，输出为 `PASS text+icon=14,18,24,32,36,48 fallback=matching-size images=dynamic max_instances=24 repeat=1000 default_pair_bytes=1504 additional_instance_bytes=36552 font_fds=24 lv_bytes=49520`。当前更新后的 smoke 会遍历完整字号范围并验证无 24 上限。
- 2026-10-13 更新后的 smoke：遍历 8–64px 全部 57 个有效整数字号，文本/图标字形 fallback、A8 图标图像、重复字号请求和字体池增长检查通过。输出：`PASS text+icon=14,18,24,32,48 fallback=matching-size images=dynamic size_range=8-64 repeat=1000 default_pair_bytes=1552 additional_instance_bytes=106952 font_fds=114 lv_bytes=119968`。该测试仅证明在 Simulator 当前构建和字号范围内不再被 24 限制，不代表设备内存或文件句柄安全。

剩余风险是实例常驻会持续增加 RAM 与文件句柄，需在 v0.8 实现安全回收并补测 E84 峰值 RAM、LittleFS 句柄和实际字形绘制；资源需部署在两个约定路径；运行时字体热重载不支持。E84 两份源 TTF 约 2.3 MiB，低于当前约 55.5 MiB 的名义 LittleFS 资源区容量；这不包含 LittleFS 元数据及已有文件占用。

本轮验证：Simulator Native 完整构建通过；`scripts/check.py` 全部检查通过（3 条既有 TODO 提示，静态分析器未安装）；SNI 配置校验 0 error / 0 warning；JS smoke 文件语法和 RemixIcon cmap 校验通过。E84 的 `eos_font.c` ARM 对象编译通过，`eos_image.c` ARM 对象单独编译通过。完整 CM55 构建仍失败：当前工作区的 `arc_track` 源码缺少 `<math.h>` 声明，同时 E84 `mtb_shared/lvgl/master` 与当前生成的 ArcLabel SNI 版本不匹配。Simulator 图形界面、JS 运行期调用及 E84 硬件字形显示均未验证。

## 复现

在 Simulator 根目录执行：

```sh
cmake --build build --target lvgl -j8
/usr/bin/clang -std=c99 -Wall -Wextra -Wno-error -DLV_CONF_INCLUDE_SIMPLE -DLV_KCONFIG_IGNORE \
  -I. -Ilvgl/include -Ilvgl/include/lvgl -Ilvgl -IElenixOS/src/kernel/debug \
  -c tests/font/tiny_ttf_profile.c -o build/tiny_ttf_profile.o
/usr/bin/clang++ build/tiny_ttf_profile.o build/lvgl/liblvgl.a \
  build/lvgl/liblvgl_thorvg.a -lm -o build/tiny_ttf_profile
./build/tiny_ttf_profile
./build/tiny_ttf_profile common64
./build/tiny_ttf_profile saturate
./build/tiny_ttf_profile saturate64
```

编译时唯一警告来自现有 `lv_conf.h` 的 `LV_COLOR_DEPTH` 弃用提示。程序退出码为 0 表示 glyph 获取、池完整性和释放基线检查通过。计时会随 CPU、系统页缓存和 LVGL 配置变化，应重跑多个进程并以中位数比较。
