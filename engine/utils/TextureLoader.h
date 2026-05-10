#pragma once
#include <vector>
#include <filesystem>
#include <cstdint>

// CPU-side image decode — format-agnostic pixel data
// Supports PNG, JPG, BMP, TGA via stb_image
// For HDR (.hdr) files, pixels are float — isHDR = true

struct TextureData
{
    std::vector<uint8_t> pixels;   // raw RGBA8 bytes (or RGBAF32 if isHDR)
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t channels = 4;         // always 4 after load (forced RGBA)
    bool     isHDR = false;

    // When true, Texture::Init will use eR8G8B8A8Srgb so the GPU
    // linearises the values on sample. Set for diffuse/albedo textures.
    // Leave false for data maps (normal, specular, gloss) — they are
    // already linear and must not be double-converted.
    bool     isSrgb = false;
};

class TextureLoader
{
public:
    // Loads any stb_image-supported format. Always returns RGBA8 (or RGBAF32 for .hdr).
    // isSrgb is NOT set here — callers (Material) set it per slot.
    // Throws std::runtime_error on failure.
    static TextureData Load(const std::filesystem::path& path);

    // 1x1 fallback textures for missing material slots
    static TextureData White();         // diffuse fallback      — isSrgb left false
    static TextureData FlatNormal();    // normal map fallback   (128,128,255,255)
    static TextureData Grey();          // specular/gloss fallback
};