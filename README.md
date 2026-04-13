# XVEngine

This is an ongoing project with the main goal of creating a 3D engine built from scratch in C++20 using modern Vulkan (1.4). 

![C++](https://img.shields.io/badge/C%2B%2B-20-blue?logo=c%2B%2B) ![Vulkan](https://img.shields.io/badge/Vulkan-1.4-red?logo=vulkan) ![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey?logo=windows)


# Current State: Hello triangle

## Features

- **Vulkan 1.4** via `vulkan-hpp` RAII wrappers — no manual `vkDestroy*` calls
- **Dynamic rendering** — no render passes or framebuffers; uses `VK_KHR_dynamic_rendering`
- **Synchronization2** — modern `vkQueueSubmit2` and `pipelineBarrier2` throughout
- **Slang shaders** — compiled to SPIR-V directly at build time via CMake
- **GLFW 3.4** windowing with Vulkan surface integration
- **Scored GPU selection** — picks the best available device, configurable at runtime


## Architecture

```
XVEngine/
├── engine/
│   ├── core/
│   │   ├── App          — main application loop (init / loop / shutdown)
│   │   └── Window       — GLFW window and input
│   ├── renderer/
│   │   ├── Renderer     — frame orchestration (BeginFrame / EndFrame / DrawFrame)
│   │   ├── Instance     — Vulkan instance, validation layers, debug messenger
│   │   ├── Device       — physical device selection, logical device, graphics queue
│   │   ├── Swapchain    — surface format, present mode, image views
│   │   ├── CommandManager — command pool, command buffers, sync objects
│   │   ├── Pipeline     — graphics pipeline with dynamic rendering and push constants
│   │   └── Image        — generic GPU image (allocation, view, layout transitions)
│   └── utils/
│       └── Logger       — colour-coded console logging (Info / Warn / Error)
├── shaders/
│   └── *.slang          — Slang shader sources (compiled to .spv at build time)
└── CMakeLists.txt
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


## Roadmap

- [x] Instance, validation layers, debug messenger
- [x] Physical device selection (scored by type + capability)
- [x] Logical device, graphics queue
- [x] Swapchain (triple buffered, mailbox preferred)
- [x] Command pool, command buffers, sync objects
- [x] Generic `Image` class with layout transitions via `pipelineBarrier2`
- [x] Graphics pipeline with dynamic rendering
- [x] Depth buffer
- [x] Hello triangle (currntly hardcoded in shader, RGB gradient)
- [ ] Vertex buffer 
- [ ] Push constants — object transform matrix
- [ ] Swapchain resize / `eErrorOutOfDateKHR` handling
- [ ] Index buffer
- [ ] Mesh loading (tinyobjloader)
- [ ] Descriptor sets — UBOs and textures
- [ ] MSAA
- [ ] Camera and scene

