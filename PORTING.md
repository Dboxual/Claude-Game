# Porting guide

The engine is desktop-first (Windows/Linux/macOS via SDL3 + OpenGL 3.3), but
every platform-specific system sits behind an interface so future Android,
iOS, and approved console ports do not require touching gameplay code.

## Layer map

| Layer | Interface | Current implementation | Where |
|---|---|---|---|
| Window/surface | `IWindow` | SDL3 (`SdlWindow`) | `src/platform/sdl/` |
| Input | `IInput` → `InputState` | SDL3 keyboard+mouse (`SdlInput`) | `src/platform/sdl/` |
| File system | `IFileSystem` | SDL3 IO (`SdlFileSystem`) | `src/platform/sdl/` |
| Audio | `IAudio` | SDL3 device init, no sounds yet (`SdlAudio`) | `src/platform/sdl/` |
| Renderer | `IRenderer` ← `RenderFrame` | OpenGL 3.3 (`GLRenderer`) + stubs | `src/renderer/` |
| Build/export | CMake targets + presets | `tac_core` / `tac_game` / `tac_renderer` / `tac_platform_sdl` | `CMakeLists.txt` |

The dependency rules, enforced by the static-library split (a layer that
tries to include SDL or GL headers without being granted them fails to
compile):

- `tac_game` (gameplay) depends on **glm and the interface headers only** —
  never SDL, never a graphics API. It consumes `InputState` and produces a
  `RenderFrame` (pure data, `src/renderer/render_types.h`).
- `tac_renderer` has **no SDL dependency**. The GL backend resolves its
  function pointers through `IWindow::glProcLoader()`, handed over at init.
- `tac_platform_sdl` is the **only** library that links SDL3.
- `src/main.cpp` is the composition root: it picks the platform factory and
  renderer backend and pumps the loop. It includes no SDL/GL headers.

## Renderer backends

`createRenderer(GraphicsApi)` in `src/renderer/renderer_factory.cpp` returns:

| Backend | Status | Planned targets | Notes |
|---|---|---|---|
| `GLRenderer` (OpenGL 3.3 core) | **working** | Windows, Linux, macOS | current prototype backend |
| `VulkanRenderer` | stub | Windows, Linux, Android | window will add `SDL_WINDOW_VULKAN` + surface hooks next to `glProcLoader()` |
| `MetalRenderer` | stub | macOS, iOS | will be an Objective-C++ (`.mm`) file acquiring a `CAMetalLayer` from the SDL window |
| `GlesRenderer` | stub | Android, iOS bring-up | mostly a port of `GLRenderer` with `#version 300 es` shaders; reuses `font5x7` and `RenderFrame` as-is |

Stubs compile on every platform (they reference no Vulkan/Metal headers) and
fail `init()` with a clear message. Select at runtime with
`--renderer gl|vulkan|metal|gles` — the failure path is exercised in CI-able
smoke tests today, so the seam stays honest.

## Adding a platform (Android example)

1. **Platform systems** — SDL3 already supports Android; most of
   `src/platform/sdl/` carries over. `SdlFileSystem` reads via `SDL_LoadFile`,
   which resolves APK assets on Android, which is why gameplay is not allowed
   to use `std::ifstream`.
2. **Input** — add a touch/gamepad path inside the platform layer that fills
   the same `InputState` (move axes, jump/crouch edges, look deltas). Gameplay
   does not change.
3. **Renderer** — implement `GlesRenderer` (or Vulkan). Everything it must
   draw arrives in `RenderFrame`; the font atlas and glyph metrics come from
   `renderer/common/font5x7`.
4. **Build/export** — add a CMake preset/toolchain for the target (NDK
   toolchain file, `arm64-android` vcpkg triplet) and a new job in
   `.github/workflows/build.yml`. The layer libraries build unchanged.
5. **Composition root** — Android's entry point differs (`SDL_main` inside an
   activity); the loop body in `src/main.cpp` moves into a shared function
   when that lands. Keep `main.cpp` free of platform headers so this stays a
   mechanical change.

Console ports follow the same recipe under NDA toolchains: a
`tac_platform_<console>` library, a renderer backend for the console's
graphics API, and a build preset. Nothing in `tac_game` should ever need to
know.

## Rules that keep the port cheap

- Gameplay code may include: `engine/*`, `game/*`, `platform/{input,filesystem,window,audio,graphics_api}.h`
  (interfaces only), `renderer/render_types.h`, and glm. Nothing else.
- All file reads go through `IFileSystem`. No `fopen`/`ifstream` outside the
  platform layer.
- All logging goes through `eng::logInfo/logError` so a platform can redirect
  it (logcat, console SDK logging).
- Timing uses `std::chrono::steady_clock` — portable everywhere.
- No raw window/GL handles may cross a layer boundary; extend the interfaces
  instead (as `glProcLoader()` does for GL).
