# XVEngine

This is an ongoing project with the main goal of creating a 3D engine built from scratch in C++20 using modern Vulkan (1.4). 

![C++](https://img.shields.io/badge/C%2B%2B-20-blue?logo=c%2B%2B) ![Vulkan](https://img.shields.io/badge/Vulkan-1.4-red?logo=vulkan) ![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey?logo=windows)


# Current State: Colored rotating quad

## Features

- **Vulkan 1.4** via `vulkan-hpp` RAII wrappers — no manual `vkDestroy*` calls
- **Dynamic rendering** — no render passes or framebuffers; uses `VK_KHR_dynamic_rendering`
- **Synchronization2** — modern `vkQueueSubmit2` and `pipelineBarrier2` throughout
- **Slang shaders** — compiled to SPIR-V directly at build time via CMake
- **GLFW 3.4** windowing with Vulkan surface integration
- **Scored GPU selection** — picks the best available device, configurable at runtime
- **Scene system** — `Scene` owns objects (mesh + transform), `SceneManager` drives lifecycle
- **FPS camera** — WASD movement, right-click drag to look
- **Staging buffer uploads** — CPU geometry copied to device-local GPU memory
- **Push constants** — per-object MVP matrix, 64 bytes


## Architecture

```
/XVEngine
  /app           — App layer
  /engine
    /core        — Engine, Window, SceneManager
    /renderer    — Renderer, Instance, Device, Swapchain, CommandManager,
                   Image, Pipeline, Buffer, Mesh, Vertex, ObjectData
    /scene       — Scene, SceneObject, SceneManager, Camera, Transform,
                   MeshInstance, ObjectHandle, RenderList
    /utils       — Logger
  /shaders       — *.slang → *.spv at build time
```


## Build

### Prerequisites

- CMake 3.20+
- Vulkan SDK (with `slangc` on `PATH` via `VULKAN_SDK`)
- MSVC (C++20)
- Git (for FetchContent dependencies)

### Steps

```bash
# Clone
git clone https://github.com/daniellopesalmeida/XVEngine.git
cd XVEngine

# Configure and build (Visual Studio CMake integration or CLI)
cmake -B build -S . -G Ninja
cmake --build build
```

Shaders are compiled automatically on build and copied next to the executable under `shaders/`.


## Dependencies

| Library | Version | How |
|---------|---------|-----|
| [vulkan-hpp](https://github.com/KhronosGroup/Vulkan-Hpp) | Vulkan SDK | System (`find_package`) |
| [GLFW](https://github.com/glfw/glfw) | 3.4 | CMake FetchContent |
| [GLM](https://github.com/g-truc/glm) | 1.0.1 | CMake FetchContent |
| [tinyobjloader](https://github.com/tinyobjobj/tinyobjloader) | — | Planned |


## Render Loop

Each frame follows this sequence:

```
waitForFences → acquireNextImage → resetFences
  → beginCommandBuffer
    → barrier: Undefined → ColorAttachmentOptimal
    → beginRendering (dynamic, no renderpass)
      → bindPipeline → setViewport / setScissor
      → draw calls
    → endRendering
    → barrier: ColorAttachmentOptimal → PresentSrcKHR
  → endCommandBuffer
→ submit2 (sync2) → presentKHR
```

Two frames in flight. Semaphores are per-swapchain-image for render-finished signals, per-frame for present-complete.

## Controls

| Input | Action |
|-------|--------|
| W / S | Move forward / back |
| A / D | Move left / right |
| Q / E | Move up / down |
| Right-click + drag | Look around |

## Roadmap

- [x] Instance, validation layers, debug messenger
- [x] Physical device selection (scored by type + capability)
- [x] Logical device, graphics queue
- [x] Swapchain (triple buffered, mailbox preferred)
- [x] Command pool, command buffers, sync objects
- [x] Generic `Image` class with layout transitions
- [x] Graphics pipeline with dynamic rendering
- [x] Depth buffer
- [x] Vertex + index buffers (staging buffer upload)
- [x] Push constants — per-object MVP matrix
- [x] Scene system — object handles, mesh ownership, render list
- [x] FPS camera — WASD + mouse look, Vulkan Y-flip
- [x] Engine loop — fixed + variable update, frame cap
- [x] Colored rotating quad end-to-end
- [ ] Swapchain resize handling
- [ ] Mesh loading (tinyobjloader)
- [ ] Descriptor sets — UBOs and textures
- [ ] MSAA

