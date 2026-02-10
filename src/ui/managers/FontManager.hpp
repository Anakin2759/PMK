/**
 * ************************************************************************
 *
 * @file FontManager.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-30
 * @version 0.2
 * @brief 字体管理器（基于 FreeType2）
 *
 * 使用 FreeType2 进行高质量字形渲染，配合纹理图集缓存
 * - 默认加载 ui/assets/fonts/ 下的字体文件
 * - 支持 TTF, OTF, TTC 等多种字体格式
 * - 字形位图采用抗锯齿灰度渲染
 *
 * 2026-02-10 更新说明：
 *  从 stb_truetype 迁移到 FreeType2，支持更好的渲染质量和字体格式
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <cmath>
#include "../singleton/Logger.hpp"

namespace ui::managers
{

/**
 * @brief 字形信息结构（FreeType 版本）
 */
struct GlyphInfo
{
    int width = 0;               // 位图宽度
    int height = 0;              // 位图高度
    int bearingX = 0;            // 水平偏移（左侧基准）
    int bearingY = 0;            // 垂直偏移（基线到字形顶部）
    float advanceX = 0.0F;       // 水平前进量（像素）
    std::vector<uint8_t> bitmap; // 灰度位图数据
};

/**
 * @brief 字体管理器，封装 FreeType2 功能
 */
class FontManager
{
public:
    FontManager()
    {
        FT_Error error = FT_Init_FreeType(&m_ftLibrary);
        if (error)
        {
            Logger::error("[FontManager] Failed to initialize FreeType: error {}", error);
            m_ftLibrary = nullptr;
        }
        else
        {
            Logger::info("[FontManager] FreeType initialized successfully");
        }
    }

    ~FontManager()
    {
        if (m_ftFace)
        {
            FT_Done_Face(m_ftFace);
            m_ftFace = nullptr;
        }
        if (m_ftLibrary)
        {
            FT_Done_FreeType(m_ftLibrary);
            m_ftLibrary = nullptr;
        }
    }

    // 禁止拷贝和移动
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;
    FontManager(FontManager&&) = delete;
    FontManager& operator=(FontManager&&) = delete;

    /**
     * @brief 从内存加载字体
     * @param fontData 字体数据指针
     * @param dataSize 字体数据大小
     * @param fontSize 字体大小（像素）
     * @return 加载成功返回 true
     */
    bool loadFromMemory(const uint8_t* fontData, size_t dataSize, float fontSize)
    {
        if (!m_ftLibrary)
        {
            Logger::error("[FontManager] FreeType not initialized");
            return false;
        }

        // 复制字体数据（FreeType 需要持久内存）
        m_fontData.resize(dataSize);
        std::memcpy(m_fontData.data(), fontData, dataSize);

        // 加载字体 Face
        FT_Error error =
            FT_New_Memory_Face(m_ftLibrary, m_fontData.data(), static_cast<FT_Long>(dataSize), 0, &m_ftFace);
        if (error)
        {
            Logger::error("[FontManager] Failed to load font face: error {}", error);
            return false;
        }

        // 设置像素大小
        error = FT_Set_Pixel_Sizes(m_ftFace, 0, static_cast<FT_UInt>(fontSize));
        if (error)
        {
            Logger::error("[FontManager] Failed to set pixel size: error {}", error);
            FT_Done_Face(m_ftFace);
            m_ftFace = nullptr;
            return false;
        }

        m_fontSize = fontSize;
        m_loaded = true;

        Logger::info("[FontManager] Font loaded: {} at {}px", m_ftFace->family_name, fontSize);
        return true;
    }

    /**
     * @brief 检查字体是否已加载
     */
    [[nodiscard]] bool isLoaded() const { return m_loaded && m_ftFace != nullptr; }

    /**
     * @brief 获取字体大小
     */
    [[nodiscard]] float getFontSize() const { return m_fontSize; }

    /**
     * @brief 获取字体高度（行高）- 像素
     */
    [[nodiscard]] int getFontHeight() const
    {
        if (!m_ftFace) return 0;
        return static_cast<int>(m_ftFace->size->metrics.height >> 6);
    }

    /**
     * @brief 获取基线位置（ascender）- 像素
     */
    [[nodiscard]] int getBaseline() const
    {
        if (!m_ftFace) return 0;
        return static_cast<int>(m_ftFace->size->metrics.ascender >> 6);
    }

    /**
     * @brief 测量 UTF-8 文本的宽度
     * @param text UTF-8 编码的文本
     * @param maxWidth 最大宽度（像素），超过此宽度则停止测量
     * @param outMeasuredLength 输出实际测量的字节长度
     * @return 文本的像素宽度
     */
    int measureString(const char* text, size_t textLen, int maxWidth, size_t* outMeasuredLength)
    {
        if (!m_ftFace || text == nullptr || textLen == 0)
        {
            if (outMeasuredLength) *outMeasuredLength = 0;
            return 0;
        }

        float totalWidth = 0.0F;
        size_t bytePos = 0;
        std::string_view view(text, textLen);
        FT_UInt prevGlyphIndex = 0;
        bool useKerning = FT_HAS_KERNING(m_ftFace);

        while (bytePos < textLen)
        {
            int codepoint = 0;
            size_t charLen = decodeUTF8(view.substr(bytePos), codepoint);
            if (charLen == 0) break;

            FT_UInt glyphIndex = FT_Get_Char_Index(m_ftFace, static_cast<FT_ULong>(codepoint));

            // 应用字距调整（kerning）
            if (useKerning && prevGlyphIndex != 0 && glyphIndex != 0)
            {
                FT_Vector delta;
                FT_Get_Kerning(m_ftFace, prevGlyphIndex, glyphIndex, FT_KERNING_DEFAULT, &delta);
                totalWidth += static_cast<float>(delta.x >> 6);
            }

            // 加载字形获取 advance
            if (FT_Load_Glyph(m_ftFace, glyphIndex, FT_LOAD_DEFAULT) == 0)
            {
                float advance = static_cast<float>(m_ftFace->glyph->advance.x >> 6);

                if (maxWidth > 0 && totalWidth + advance > static_cast<float>(maxWidth))
                {
                    break;
                }

                totalWidth += advance;
            }

            prevGlyphIndex = glyphIndex;
            bytePos += charLen;
        }

        if (outMeasuredLength)
        {
            *outMeasuredLength = bytePos;
        }

        return static_cast<int>(std::ceil(totalWidth));
    }

    /**
     * @brief 测量文本宽度（简化版本）
     */
    int measureTextWidth(const std::string& text)
    {
        size_t measuredLen = 0;
        return measureString(text.c_str(), text.size(), 0, &measuredLen);
    }

    /**
     * @brief 渲染单个字形到灰度位图
     * @param codepoint Unicode 码点
     * @param fontSize 字体大小（像素），0 表示使用默认大小
     * @return 字形信息
     */
    GlyphInfo renderGlyph(int codepoint, float fontSize = 0.0F)
    {
        GlyphInfo info;

        if (!m_ftFace) return info;

        // 使用指定字体大小或默认大小
        float targetSize = (fontSize > 0.0F) ? fontSize : m_fontSize;

        // 检查缓存（包含字体大小）
        uint64_t cacheKey = makeGlyphCacheKey(codepoint, targetSize);
        auto it = m_glyphCache.find(cacheKey);
        if (it != m_glyphCache.end())
        {
            return it->second;
        }

        // 临时设置字体大小
        bool needRestore = (std::abs(targetSize - m_fontSize) > 0.1F);
        float oldSize = m_fontSize;
        if (needRestore)
        {
            setPixelSize(targetSize);
        }

        // 加载字形
        FT_UInt glyphIndex = FT_Get_Char_Index(m_ftFace, static_cast<FT_ULong>(codepoint));
        FT_Error error = FT_Load_Glyph(m_ftFace, glyphIndex, FT_LOAD_DEFAULT);
        if (error)
        {
            Logger::warn("[FontManager] Failed to load glyph for codepoint {}: error {}", codepoint, error);
            return info;
        }

        // 渲染为灰度位图
        error = FT_Render_Glyph(m_ftFace->glyph, FT_RENDER_MODE_NORMAL);
        if (error)
        {
            Logger::warn("[FontManager] Failed to render glyph for codepoint {}: error {}", codepoint, error);
            return info;
        }

        FT_GlyphSlot slot = m_ftFace->glyph;
        FT_Bitmap& bitmap = slot->bitmap;

        info.width = static_cast<int>(bitmap.width);
        info.height = static_cast<int>(bitmap.rows);
        info.bearingX = slot->bitmap_left;
        info.bearingY = slot->bitmap_top;
        info.advanceX = static_cast<float>(slot->advance.x >> 6);

        // 复制位图数据
        if (bitmap.buffer && bitmap.width > 0 && bitmap.rows > 0)
        {
            size_t dataSize = bitmap.width * bitmap.rows;
            info.bitmap.resize(dataSize);

            if (bitmap.pitch == static_cast<int>(bitmap.width))
            {
                // 连续内存，直接拷贝
                std::memcpy(info.bitmap.data(), bitmap.buffer, dataSize);
            }
            else
            {
                // 不连续，逐行拷贝
                for (unsigned int row = 0; row < bitmap.rows; ++row)
                {
                    std::memcpy(
                        info.bitmap.data() + row * bitmap.width, bitmap.buffer + row * bitmap.pitch, bitmap.width);
                }
            }
        }

        // 恢复原字体大小
        if (needRestore)
        {
            setPixelSize(oldSize);
        }

        // 缓存字形
        m_glyphCache[cacheKey] = info;

        return info;
    }

    /**
     * @brief 获取超采样缩放因子（FreeType 不需要超采样，始终返回 1.0）
     * @note 保留此接口以兼容旧的渲染管线
     */
    [[nodiscard]] float getOversampleScale() const { return 1.0F; }

    /**
     * @brief 渲染整个文本到 RGBA 位图（兼容旧接口）
     * @param text UTF-8 文本
     * @param red R 通道 (0-255)
     * @param green G 通道 (0-255)
     * @param blue B 通道 (0-255)
     * @param alpha A 通道 (0-255)
     * @param outWidth 输出位图宽度
     * @param outHeight 输出位图高度
     * @param fontSize 字体大小（像素），0 表示使用默认大小
     * @return RGBA 位图数据
     */
    std::vector<uint8_t> renderTextBitmap(const std::string& text,
                                          uint8_t red,
                                          uint8_t green,
                                          uint8_t blue,
                                          uint8_t alpha,
                                          int& outWidth,
                                          int& outHeight,
                                          float fontSize = 0.0F)
    {
        std::vector<uint8_t> result;

        if (!m_loaded || text.empty())
        {
            outWidth = outHeight = 0;
            return result;
        }

        // 使用指定字体大小或默认大小
        float targetSize = (fontSize > 0.0F) ? fontSize : m_fontSize;
        bool needRestore = (std::abs(targetSize - m_fontSize) > 0.1F);
        float oldSize = m_fontSize;
        if (needRestore)
        {
            setPixelSize(targetSize);
        }

        // 布局所有字形
        struct GlyphLayout
        {
            GlyphInfo glyph;
            float xPos = 0.0F;
        };
        std::vector<GlyphLayout> layouts;
        float cursorX = 0.0F;

        size_t bytePos = 0;
        std::string_view view(text);
        while (bytePos < text.size())
        {
            int codepoint = 0;
            size_t charLen = decodeUTF8(view.substr(bytePos), codepoint);
            if (charLen == 0) break;

            GlyphInfo glyph = renderGlyph(codepoint, targetSize);
            layouts.push_back({glyph, cursorX});
            cursorX += glyph.advanceX;
            bytePos += charLen;
        }

        if (layouts.empty())
        {
            if (needRestore) setPixelSize(oldSize);
            outWidth = outHeight = 0;
            return result;
        }

        int baseline = getBaseline();
        int fontHeight = getFontHeight();
        outWidth = static_cast<int>(std::ceil(cursorX));
        outHeight = fontHeight;

        if (outWidth <= 0 || outHeight <= 0)
        {
            outWidth = outHeight = 0;
            return result;
        }

        // 创建 RGBA 位图
        result.resize(static_cast<size_t>(outWidth) * static_cast<size_t>(outHeight) * 4);
        for (int idx = 0; idx < outWidth * outHeight; ++idx)
        {
            result[(idx * 4) + 0] = red;
            result[(idx * 4) + 1] = green;
            result[(idx * 4) + 2] = blue;
            result[(idx * 4) + 3] = 0;
        }

        // 混合字形
        for (const auto& layout : layouts)
        {
            const auto& glyph = layout.glyph;
            int xPos = static_cast<int>(std::floor(layout.xPos)) + glyph.bearingX;
            int yPos = baseline - glyph.bearingY;

            for (int yOff = 0; yOff < glyph.height; ++yOff)
            {
                for (int xOff = 0; xOff < glyph.width; ++xOff)
                {
                    int bx = xPos + xOff;
                    int by = yPos + yOff;
                    if (bx < 0 || bx >= outWidth || by < 0 || by >= outHeight) continue;

                    int pixelIndex = (by * outWidth + bx) * 4;
                    uint8_t srcAlpha = glyph.bitmap[(yOff * glyph.width) + xOff];
                    auto newAlpha = static_cast<uint8_t>(srcAlpha * alpha / 255);
                    result[pixelIndex + 3] = std::max(result[pixelIndex + 3], newAlpha);
                }
            }
        }

        // 恢复原字体大小
        if (needRestore)
        {
            setPixelSize(oldSize);
        }

        return result;
    }

    /**
     * @brief 清空字形缓存
     */
    void clearCache()
    {
        m_glyphCache.clear();
        Logger::info("[FontManager] Glyph cache cleared");
    }

    /**
     * @brief 解码 UTF-8 字符
     * @param text UTF-8 字符串视图
     * @param outCodepoint 输出码点
     * @return 字符占用的字节数，失败返回 0
     */
    static size_t decodeUTF8(std::string_view text, int& outCodepoint)
    {
        if (text.empty()) return 0;

        const auto byte0 = static_cast<uint8_t>(text[0]);

        // 单字节 ASCII
        if (byte0 < 0x80)
        {
            outCodepoint = byte0;
            return 1;
        }

        // 2 字节
        if ((byte0 & 0xE0U) == 0xC0)
        {
            if (text.size() < 2) return 0;
            outCodepoint = static_cast<int>(((byte0 & 0x1FU) << 6U) | (static_cast<uint8_t>(text[1]) & 0x3FU));
            return 2;
        }

        // 3 字节
        if ((byte0 & 0xF0U) == 0xE0)
        {
            if (text.size() < 3) return 0;
            outCodepoint = static_cast<int>(((byte0 & 0x0FU) << 12U) | ((static_cast<uint8_t>(text[1]) & 0x3FU) << 6U) |
                                            (static_cast<uint8_t>(text[2]) & 0x3FU));
            return 3;
        }

        // 4 字节
        if ((byte0 & 0xF8U) == 0xF0)
        {
            if (text.size() < 4) return 0;
            outCodepoint = static_cast<int>(
                ((byte0 & 0x07U) << 18U) | ((static_cast<uint8_t>(text[1]) & 0x3FU) << 12U) |
                ((static_cast<uint8_t>(text[2]) & 0x3FU) << 6U) | (static_cast<uint8_t>(text[3]) & 0x3FU));
            return 4;
        }

        return 0;
    }

private:
    /**
     * @brief 设置字体像素大小
     */
    void setPixelSize(float size)
    {
        if (!m_ftFace || size <= 0.0F) return;
        FT_Error error = FT_Set_Pixel_Sizes(m_ftFace, 0, static_cast<FT_UInt>(size));
        if (error == 0)
        {
            m_fontSize = size;
        }
    }

    /**
     * @brief 生成字形缓存键（包含字体大小）
     */
    static uint64_t makeGlyphCacheKey(int codepoint, float fontSize)
    {
        // 将字体大小转换为整数（精度 0.1px）
        uint32_t sizeKey = static_cast<uint32_t>(fontSize * 10.0F);
        return (static_cast<uint64_t>(sizeKey) << 32) | static_cast<uint32_t>(codepoint);
    }

    bool m_loaded = false;
    float m_fontSize = 16.0F;

    std::vector<uint8_t> m_fontData;

    FT_Library m_ftLibrary = nullptr;
    FT_Face m_ftFace = nullptr;

    // 字形缓存（key = (fontSize << 32) | codepoint）
    std::unordered_map<uint64_t, GlyphInfo> m_glyphCache;
};

} // namespace ui::managers
