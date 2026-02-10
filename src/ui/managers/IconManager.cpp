#include "IconManager.hpp"
#include "DeviceManager.hpp"
#include <SDL3/SDL_gpu.h>
#include <stb_truetype.h>
#include <cstring>
#include <fstream> // 提供 basic_ifstream 的完整定义

namespace ui::managers
{
/**
 * @brief 加载 IconFont 字体和 codepoints 文件
 * @param name 字体库名称（用于后续引用）
 * @param fontPath TTF 字体文件路径
 * @param codepointsPath codepoints 文件路径（支持 .txt 或 .json）
 * @param fontSize 字体大小
 * @return true 加载成功
 * @return false 加载失败
 */
bool IconManager::loadIconFont(const std::string& name,
                               const std::string& fontPath,
                               const std::string& codepointsPath,
                               int fontSize)
{
    Logger::info("Loading IconFont '{}' from '{}'", name, fontPath);
    // 读取文件并初始化 stbtt_fontinfo
    std::ifstream file(fontPath, std::ios::binary | std::ios::ate); // NOLINT
    if (!file.is_open())
    {
        Logger::error("Failed to open font file: {}", fontPath);
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> buffer(size);
    if (!file.read((char*)buffer.data(), size))
    {
        Logger::error("Failed to read font file: {}", fontPath);
        return false;
    }

    stbtt_fontinfo info;
    if (stbtt_InitFont(&info, buffer.data(), stbtt_GetFontOffsetForIndex(buffer.data(), 0)) == 0)
    {
        Logger::error("Failed to init font: {}", fontPath);
        return false;
    }

    // 解析 codepoints 文件
    auto codepoints = parseCodepoints(codepointsPath);
    if (codepoints.empty())
    {
        Logger::warn("No codepoints loaded from: {}", codepointsPath);
    }

    // 存储字体和映射
    m_fonts[name] = FontData{.buffer = std::move(buffer), .info = info, .fontSize = fontSize};
    m_codepoints[name] = std::move(codepoints);

    Logger::info("IconFont '{}' loaded: {} icons", name, m_codepoints[name].size());
    return true;
}

bool IconManager::loadIconFontFromMemory(const std::string& name,
                                         const void* fontData,
                                         size_t fontLength,
                                         const void* codepointsData,
                                         size_t codepointsLength,
                                         int fontSize)
{
    if (fontData == nullptr || fontLength == 0) return false;

    // Copy font data because we store it
    std::vector<unsigned char> buffer(fontLength);
    std::memcpy(buffer.data(), fontData, fontLength);

    stbtt_fontinfo info;
    if (stbtt_InitFont(&info, buffer.data(), stbtt_GetFontOffsetForIndex(buffer.data(), 0)) == 0)
    {
        Logger::error("Failed to init font from memory: {}", name);
        return false;
    }

    // Parse codepoints from memory
    std::string codepointsStr(static_cast<const char*>(codepointsData), codepointsLength);
    std::istringstream stream(codepointsStr);

    CodepointMap codepoints;

    // Heuristic to detect JSON vs TXT: check for '{'
    char firstChar = 0;
    while (stream >> std::ws && stream.peek() != EOF)
    {
        firstChar = static_cast<char>(stream.peek());
        break; // Only peek first non-whitespace char
    }
    stream.seekg(0); // reset stream position

    if (firstChar == '{')
    {
        codepoints = parseCodepointsJSON(stream);
    }
    else
    {
        codepoints = parseCodepointsTXT(stream);
    }

    if (codepoints.empty())
    {
        Logger::warn("No codepoints loaded from memory for: {}", name);
    }

    m_fonts[name] = FontData{.buffer = std::move(buffer), .info = info, .fontSize = fontSize};
    m_codepoints[name] = std::move(codepoints);

    Logger::info("IconFont '{}' loaded from memory: {} icons", name, m_codepoints[name].size());
    return true;
}

uint32_t IconManager::getCodepoint(std::string_view fontName, std::string_view iconName) const
{
    if (auto fontIt = m_codepoints.find(fontName); fontIt != m_codepoints.end())
    {
        if (auto iconIt = fontIt->second.find(iconName); iconIt != fontIt->second.end())
        {
            return iconIt->second;
        }
        Logger::warn("Icon '{}' not found in font '{}'", iconName, fontName);
        return 0;
    }

    Logger::warn("IconFont '{}' not found", fontName);
    return 0;
}

stbtt_fontinfo* IconManager::getFont(std::string_view fontName)
{
    if (auto iterator = m_fonts.find(fontName); iterator != m_fonts.end())
    {
        return &iterator->second.info;
    }
    return nullptr;
}

bool IconManager::hasIcon(std::string_view fontName, std::string_view iconName) const
{
    if (auto it = m_codepoints.find(fontName); it != m_codepoints.end())
    {
        return it->second.contains(iconName);
    }
    return false;
}

std::vector<std::string> IconManager::getIconNames(std::string_view fontName) const
{
    std::vector<std::string> names;
    if (auto it = m_codepoints.find(fontName); it != m_codepoints.end())
    {
        names.reserve(it->second.size());
        for (const auto& [name, codepoint] : it->second)
        {
            names.push_back(name);
        }
    }
    return names;
}

void IconManager::unloadIconFont(std::string_view fontName)
{
    m_fonts.erase(fontName);
    m_codepoints.erase(fontName);
    Logger::info("IconFont '{}' unloaded", fontName);
}

void IconManager::shutdown()
{
    m_fontTextureCache.clear();
    m_imageTextureCache.clear();
    m_fonts.clear();
    m_codepoints.clear();
    Logger::info("[IconManager] Shutdown complete. Total evictions: {}", m_evictionCount);
}

const TextureInfo* IconManager::getTextureInfo(std::string_view fontName, uint32_t codepoint, float size)
{
    // 量化大小以减少缓存条目
    float quantizedSize = quantizeSize(size);

    // 构造 cache key
    std::string cacheKey =
        std::string(fontName) + "_" + std::to_string(codepoint) + "_" + std::to_string(static_cast<int>(quantizedSize));

    // 检查缓存
    auto iter = m_fontTextureCache.find(cacheKey);
    if (iter != m_fontTextureCache.end())
    {
        // 更新访问时间和计数
        iter->second.lastAccessTime = std::chrono::steady_clock::now();
        iter->second.accessCount++;
        return &iter->second.textureInfo;
    }

    // 如果缓存已满，执行 LRU 驱逐
    if (m_fontTextureCache.size() >= MAX_FONT_CACHE_SIZE)
    {
        evictLRUFromFontCache();
    }

    auto fontDataIt = m_fonts.find(fontName);
    if (fontDataIt == m_fonts.end())
    {
        return nullptr;
    }

    stbtt_fontinfo* info = &fontDataIt->second.info;
    float scale = stbtt_ScaleForPixelHeight(info, quantizedSize);

    int width{};
    int height{};
    int xoff{};
    int yoff{};
    unsigned char* bitmap =
        stbtt_GetCodepointBitmap(info, 0, scale, static_cast<int>(codepoint), &width, &height, &xoff, &yoff);

    if (bitmap == nullptr)
    {
        Logger::warn("[IconManager] Failed to generate bitmap for codepoint {}", codepoint);
        return nullptr;
    }

    SDL_GPUDevice* device = m_deviceManager->getDevice();
    if (device == nullptr)
    {
        stbtt_FreeBitmap(bitmap, nullptr);
        Logger::error("[IconManager] GPU device is null");
        return nullptr;
    }

    // Convert alpha bitmap to RGBA
    std::vector<uint32_t> rgbaPixels(static_cast<size_t>(width) * static_cast<size_t>(height));
    for (int i = 0; i < width * height; ++i)
    {
        uint8_t alpha = bitmap[i];
        rgbaPixels[i] = (alpha << 24) | 0x00FFFFFF; // White with alpha
    }

    stbtt_FreeBitmap(bitmap, nullptr);

    // 创建并上传纹理
    auto texture =
        createAndUploadIconTexture(device, rgbaPixels, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    if (texture == nullptr)
    {
        return nullptr;
    }

    // 创建缓存条目
    CachedTextureEntry entry{};
    entry.textureInfo.texture = std::move(texture);
    entry.textureInfo.uvMin = {0.0F, 0.0F};
    entry.textureInfo.uvMax = {1.0F, 1.0F};
    entry.textureInfo.width = static_cast<float>(width);
    entry.textureInfo.height = static_cast<float>(height);
    entry.lastAccessTime = std::chrono::steady_clock::now();
    entry.accessCount = 1;

    m_fontTextureCache[cacheKey] = std::move(entry);
    return &m_fontTextureCache[cacheKey].textureInfo;
}

IconManager::CodepointMap IconManager::parseCodepoints(const std::string& filePath)
{
    CodepointMap result;
    std::ifstream file(filePath);

    if (!file.is_open())
    {
        Logger::error("Failed to open codepoints file: {}", filePath);
        return result;
    }

    if (filePath.find(".json") != std::string::npos)
    {
        result = parseCodepointsJSON(file);
    }
    else
    {
        result = parseCodepointsTXT(file);
    }

    file.close();
    return result;
}

IconManager::CodepointMap IconManager::parseCodepointsTXT(std::istream& file)
{
    CodepointMap result;
    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string iconName;
        std::string hexCode;

        if (iss >> iconName >> hexCode)
        {
            try
            {
                uint32_t codepoint = std::stoul(hexCode, nullptr, 16);
                result[iconName] = codepoint;
            }
            catch (...)
            {
                Logger::warn("Invalid codepoint format: {} - {}", iconName, hexCode);
            }
        }
    }

    return result;
}

IconManager::CodepointMap IconManager::parseCodepointsJSON(std::istream& file)
{
    CodepointMap result;
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    size_t pos = 0;
    while (true)
    {
        size_t keyStart = content.find('"', pos);
        if (keyStart == std::string::npos) break;

        size_t keyEnd = content.find('"', keyStart + 1);
        if (keyEnd == std::string::npos) break;

        std::string key = content.substr(keyStart + 1, keyEnd - keyStart - 1);

        size_t valueStart = content.find('"', keyEnd + 1);
        if (valueStart == std::string::npos) break;

        size_t valueEnd = content.find('"', valueStart + 1);
        if (valueEnd == std::string::npos) break;

        std::string value = content.substr(valueStart + 1, valueEnd - valueStart - 1);

        try
        {
            uint32_t codepoint = std::stoul(value, nullptr, 16);
            result[key] = codepoint;
        }
        catch (...)
        {
            Logger::warn("Invalid codepoint in JSON: {} - {}", key, value);
        }

        pos = valueEnd + 1;
    }

    return result;
}

void IconManager::evictLRUFromFontCache()
{
    if (m_fontTextureCache.empty())
    {
        return;
    }

    SDL_GPUDevice* device = m_deviceManager->getDevice();
    if (device == nullptr)
    {
        return;
    }

    // 找到最少使用的条目
    auto lruEntry = m_fontTextureCache.begin();
    for (auto iter = m_fontTextureCache.begin(); iter != m_fontTextureCache.end(); ++iter)
    {
        if (iter->second.lastAccessTime < lruEntry->second.lastAccessTime)
        {
            lruEntry = iter;
        }
    }

    // 释放纹理资源
    /*
    if (lruEntry->second.textureInfo.texture != nullptr)
    {
        SDL_ReleaseGPUTexture(device, lruEntry->second.textureInfo.texture);
    }
    */

    Logger::debug("[IconManager] Evicted LRU entry: {} (access count: {})",
                  lruEntry->first.substr(0, 50),
                  lruEntry->second.accessCount);

    m_fontTextureCache.erase(lruEntry);
    m_evictionCount++;

    // 如果仍然过大，批量驱逐
    if (m_fontTextureCache.size() >= MAX_FONT_CACHE_SIZE)
    {
        std::vector<std::pair<std::string, std::chrono::steady_clock::time_point>> entries;
        entries.reserve(m_fontTextureCache.size());

        for (const auto& [key, entry] : m_fontTextureCache)
        {
            entries.emplace_back(key, entry.lastAccessTime);
        }

        // 按访问时间排序
        std::sort(entries.begin(),
                  entries.end(),
                  [](const auto& first, const auto& second) { return first.second < second.second; });

        // 驱逐最旧的条目
        size_t evicted = 0;
        for (size_t idx = 0; idx < std::min(EVICTION_BATCH, entries.size()); ++idx)
        {
            auto iter = m_fontTextureCache.find(entries[idx].first);
            if (iter != m_fontTextureCache.end())
            {
                m_fontTextureCache.erase(iter);
                evicted++;
            }
        }

        m_evictionCount += evicted;
        Logger::info("[IconManager] Batch evicted {} entries, cache size: {}", evicted, m_fontTextureCache.size());
    }
}
/**
 * @brief
 * @param devices 设备
 * @param rgbaPixels {comment}
 * @param width {comment}
 * @param height {comment}
 * @return wrappers::UniqueGPUTexture {comment}
 */
wrappers::UniqueGPUTexture IconManager::createAndUploadIconTexture(SDL_GPUDevice* device,
                                                                   const std::vector<uint32_t>& rgbaPixels,
                                                                   uint32_t width,
                                                                   uint32_t height)
{
    // 创建纹理
    SDL_GPUTextureCreateInfo texInfo = {};
    texInfo.type = SDL_GPU_TEXTURETYPE_2D;
    texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texInfo.width = width;
    texInfo.height = height;
    texInfo.layer_count_or_depth = 1;
    texInfo.num_levels = 1;
    texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    auto texture = wrappers::MakeGpuResource<wrappers::UniqueGPUTexture>(device, SDL_CreateGPUTexture, &texInfo);
    if (!texture)
    {
        Logger::error("[IconManager] Failed to create GPU texture");
        return nullptr;
    }

    // 创建传输缓冲区
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = static_cast<uint32_t>(rgbaPixels.size() * sizeof(uint32_t));

    auto transferBuffer = wrappers::MakeGpuResource<wrappers::UniqueGPUTransferBuffer>(
        device, SDL_CreateGPUTransferBuffer, &transferInfo);
    if (!transferBuffer)
    {
        Logger::error("[IconManager] Failed to create transfer buffer");
        return nullptr;
    }

    // 映射传输缓冲区
    void* mappedData = SDL_MapGPUTransferBuffer(device, transferBuffer.get(), false);
    if (mappedData == nullptr)
    {
        Logger::error("[IconManager] Failed to map transfer buffer");
        return nullptr;
    }

    SDL_memcpy(mappedData, rgbaPixels.data(), transferInfo.size);
    SDL_UnmapGPUTransferBuffer(device, transferBuffer.get());

    // 上传到GPU
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (cmd == nullptr)
    {
        Logger::error("[IconManager] Failed to acquire command buffer");
        return nullptr;
    }

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTextureTransferInfo srcInfo = {};
    srcInfo.transfer_buffer = transferBuffer.get();
    srcInfo.pixels_per_row = width;
    srcInfo.rows_per_layer = height;

    SDL_GPUTextureRegion dstRegion = {};
    dstRegion.texture = texture.get();
    dstRegion.w = width;
    dstRegion.h = height;
    dstRegion.d = 1;

    SDL_UploadToGPUTexture(copyPass, &srcInfo, &dstRegion, false);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);

    return texture;
}

} // namespace ui::managers
