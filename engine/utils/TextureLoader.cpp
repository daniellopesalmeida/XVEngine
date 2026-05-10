#define STB_IMAGE_IMPLEMENTATION
#include "TextureLoader.h"
#include <stb_image.h>
#include <utils/Logger.h>
#include <stdexcept>

TextureData TextureLoader::Load(const std::filesystem::path& path)
{
    TextureData result;

    bool isHDR = stbi_is_hdr(path.string().c_str());

    if (isHDR)
    {
        int w, h, ch;
        float* raw = stbi_loadf(path.string().c_str(), &w, &h, &ch, STBI_rgb_alpha);
        if (!raw)
        {
            throw std::runtime_error("TextureLoader: failed to load HDR: " + path.string());
        }

        result.width = static_cast<uint32_t>(w);
        result.height = static_cast<uint32_t>(h);
        result.channels = 4;
        result.isHDR = true;

        size_t byteCount = static_cast<size_t>(w) * h * 4 * sizeof(float);
        result.pixels.resize(byteCount);
        memcpy(result.pixels.data(), raw, byteCount);
        stbi_image_free(raw);
    }
    else
    {
        int w, h, ch;
        uint8_t* raw = stbi_load(path.string().c_str(), &w, &h, &ch, STBI_rgb_alpha);
        if (!raw)
        {
            throw std::runtime_error("TextureLoader: failed to load: " + path.string()
                + " — " + stbi_failure_reason());
        }

        result.width = static_cast<uint32_t>(w);
        result.height = static_cast<uint32_t>(h);
        result.channels = 4;
        result.isHDR = false;

        size_t byteCount = static_cast<size_t>(w) * h * 4;
        result.pixels.resize(byteCount);
        memcpy(result.pixels.data(), raw, byteCount);
        stbi_image_free(raw);
    }

    Logger::Info("Texture loaded: '", path.filename().string(), "' ", result.width, "x", result.height);
    return result;
}

TextureData TextureLoader::White()
{
    TextureData d;
    d.width = d.height = 1;
    d.channels = 4;
    d.pixels = { 255, 255, 255, 255 };
    return d;
}

TextureData TextureLoader::FlatNormal()
{
    TextureData d;
    d.width = d.height = 1;
    d.channels = 4;
    //tangent-space "pointing straight up" = (0,0,1) encoded as (128,128,255)
    d.pixels = { 128, 128, 255, 255 };
    return d;
}

TextureData TextureLoader::Grey()
{
    TextureData d;
    d.width = d.height = 1;
    d.channels = 4;
    d.pixels = { 128, 128, 128, 255 };
    return d;
}