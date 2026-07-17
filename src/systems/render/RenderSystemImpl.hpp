/**
 * @file RenderSystemImpl.hpp
 * @brief RenderSystem PIMPL 实现结构体（仅供 render/ 下 .cpp 包含，不暴露给外部）
 *
 * 将所有重量级私有成员（9 个 manager unique_ptr、渲染队列、GPU 资源等）
 * 隔离在此内部头文件中，使 RenderSystem.hpp 保持轻量。
 *
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <SDL3/SDL_gpu.h>
#include <entt/entt.hpp>

#include "common/GPUWrappers.hpp"
#include "core/RenderContext.hpp"
#include "interface/IBackendRenderer.hpp"
#include "interface/IRenderer.hpp"
#include "managers/BatchManager.hpp"
#include "managers/CommandBuffer.hpp"
#include "managers/DeviceManager.hpp"
#include "managers/FontManager.hpp"
#include "managers/IconManager.hpp"
#include "managers/ImageManager.hpp"
#include "managers/PipelineCache.hpp"
#include "managers/TextTextureCache.hpp"
#include "systems/RenderSystem.hpp"
#include "systems/render/DeviceClaimState.hpp"
#include "systems/render/GpuInitializationTransaction.hpp"

namespace ui::systems
{

/**
 * @brief RenderSystem PIMPL 实现结构体
 *
 * 持有所有私有状态：9 个 manager unique_ptr、渲染队列、GPU 资源、运行时标志。
 * 仅通过 RenderSystem::m_impl 访问，外部 TU 不可见此头文件。
 */
struct RenderSystemImpl
{
    /**
     * @brief 渲染项结构体
     * @note 包含排序键、实体、渲染器指针和渲染上下文，用于渲染队列
     */
    struct RenderItem
    {
        uint64_t sortKey = 0;
        entt::entity entity = entt::null;
        core::IRenderer* renderer = nullptr;
        core::RenderContext context;

        bool operator<(const RenderItem& other) const { return sortKey < other.sortKey; }
    };

    explicit RenderSystemImpl(bool forceFallback);

    std::unique_ptr<managers::DeviceManager> m_deviceManager; // 负责 GPU 设备和上下文管理
    std::unique_ptr<managers::FontManager> m_fontManager;     // 负责字体管理
    std::unique_ptr<managers::IconManager> m_iconManager;     // 负责图标管理
    std::unique_ptr<managers::ImageManager> m_imageManager;   // 负责图像管理
    std::unique_ptr<managers::PipelineCache> m_pipelineCache; // 负责管线缓存管理
    std::unique_ptr<managers::TextTextureCache> m_textTextureCache; // 负责文本纹理缓存管理
    std::unique_ptr<managers::BatchManager> m_batchManager;
    std::unique_ptr<managers::CommandBuffer> m_commandBuffer;
    std::unique_ptr<interface::IBackendRenderer> m_backendRenderer;

    std::vector<std::unique_ptr<core::IRenderer>> m_renderers;
    std::vector<RenderItem> m_renderQueue;
    uint32_t m_submissionIndex = 0;

    RenderSystem::RenderStats m_stats;

    wrappers::UniqueGPUTexture m_whiteTexture;
    std::byte m_fallbackWhiteTextureCookie{};
    SDL_GPUTexture* m_fallbackWhiteTextureTag = nullptr; ///< 指向 m_fallbackWhiteTextureCookie，由构造函数初始化

    bool m_useFallback = false;
    bool m_forceFallback = false;
    bool m_backendSelectionLogged = false;
    bool m_firstUpdate = true;
    bool m_iconsLoaded = false;
    render::DeviceClaimState m_deviceClaimState;
    render::GpuInitializationTransaction m_gpuInitialization;

    float m_screenWidth = 0.0F;
    float m_screenHeight = 0.0F;
};

} // namespace ui::systems
