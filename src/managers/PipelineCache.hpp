/**
 * ************************************************************************
 *
 * @file PipelineCache.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-30
 * @version 0.1
 * @brief 渲染管线缓存管理器
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once
#include <bit>
#include <array>
#include <memory>
#include <SDL3/SDL_gpu.h>
#include "core/UiRuntime.hpp"
#include "utils/Logger.hpp"
#include "common/CustomizationPoints.hpp"
#include "common/RenderTypes.hpp"
#include "common/GPUWrappers.hpp"
#include "common/GpuFailureInjection.hpp"
#include "DeviceManager.hpp"
#include "ResourceProvider.hpp"
#include "ui/Result.hpp"
#include "ui/ErrorCodes.hpp"

namespace ui::managers
{

/**
 * @brief 按 GPU 设备代际构建着色器、图形管线和采样器
 *
 * 所有 GPU owner 绑定同一 generation id；发现代际变化时清理旧资源。管线与采样器
 * 作为候选完整创建后才提交到正式成员，避免缓存跨代混装或留下半初始化组合。
 */
class PipelineCache
{
   public:
    explicit PipelineCache(DeviceManager& deviceManager, utils::Logger& logger,
                           std::shared_ptr<const IResourceProvider> resourceProvider = nullptr)
        : m_deviceManager(&deviceManager), m_logger(&logger), m_resourceProvider(std::move(resourceProvider))
    {
    }
    ~PipelineCache()
    {
        cleanup();
    }
    PipelineCache(const PipelineCache&) = delete;
    PipelineCache& operator=(const PipelineCache&) = delete;
    PipelineCache(PipelineCache&&) = default;
    PipelineCache& operator=(PipelineCache&&) = default;

    Result<void> loadShaders()
    {
        SDL_GPUDevice* device = m_deviceManager->getDevice();
        const auto generation = m_deviceManager->getGeneration();
        if (device == nullptr || !generation || generation.Status() != detail::GpuDeviceGenerationStatus::ACTIVE)
        {
            return Err(UiErrc::DEVICE_UNAVAILABLE);
        }
        if (m_generationId != 0 && m_generationId != generation.Id())
        {
            cleanup();
        }

        // 根据驱动类型选择着色器格式
        const std::string& driver = m_deviceManager->getDriverName();
        bool isVulkan = (driver == "vulkan");

        wrappers::UniqueGPUShader candidateVertexShader;
        wrappers::UniqueGPUShader candidateFragmentShader;
        if (isVulkan)
        {
            candidateVertexShader = loadShaderFromResource(generation, "assets/shader/vert.spv",
                                                           SDL_GPU_SHADERSTAGE_VERTEX, SDL_GPU_SHADERFORMAT_SPIRV);
            candidateFragmentShader = loadShaderFromResource(generation, "assets/shader/frag.spv",
                                                             SDL_GPU_SHADERSTAGE_FRAGMENT, SDL_GPU_SHADERFORMAT_SPIRV);
        }
        else
        {
            candidateVertexShader = loadShaderFromResource(generation, "assets/shader/vert.dxil",
                                                           SDL_GPU_SHADERSTAGE_VERTEX, SDL_GPU_SHADERFORMAT_DXIL);
            candidateFragmentShader = loadShaderFromResource(generation, "assets/shader/frag.dxil",
                                                             SDL_GPU_SHADERSTAGE_FRAGMENT, SDL_GPU_SHADERFORMAT_DXIL);
        }

        if (candidateVertexShader == nullptr || candidateFragmentShader == nullptr)
        {
            m_logger->error("着色器加载失败 (驱动: {})", driver);
            return Err(UiErrc::SHADER_COMPILE_FAILED, driver);
        }
        if (generation.Status() != detail::GpuDeviceGenerationStatus::ACTIVE)
        {
            return Err(UiErrc::DEVICE_UNAVAILABLE);
        }
        m_vertexShader = std::move(candidateVertexShader);
        m_fragmentShader = std::move(candidateFragmentShader);
        m_generationId = generation.Id();
        m_logger->info("着色器加载成功 (驱动: {})", driver);
        return Ok();
    }

    Result<void> createPipeline(SDL_Window* sdlWindow)
    {
        SDL_GPUDevice* device = m_deviceManager->getDevice();
        const auto generation = m_deviceManager->getGeneration();
        if (device == nullptr || !generation || generation.Id() != m_generationId ||
            generation.Status() != detail::GpuDeviceGenerationStatus::ACTIVE || m_vertexShader == nullptr ||
            m_fragmentShader == nullptr)
        {
            return Err(UiErrc::DEVICE_UNAVAILABLE);
        }

        if (m_pipeline != nullptr)
        {
            return Ok();
        }
        if (m_creationFailed)
        {
            return Err(UiErrc::PIPELINE_UNAVAILABLE, "previous creation failed");
        }

        SDL_GPUVertexBufferDescription vertexBufferDesc = {};
        vertexBufferDesc.slot = 0;
        vertexBufferDesc.pitch = sizeof(ui::render::Vertex);
        vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        vertexBufferDesc.instance_step_rate = 0;

        const auto vertexAttributes = buildVertexAttributes();
        const SDL_GPUVertexInputState vertexInputState = buildVertexInputState(vertexBufferDesc, vertexAttributes);

        // 颜色附件描述
        SDL_GPUColorTargetDescription colorTargetDesc = {};
        colorTargetDesc.format = SDL_GetGPUSwapchainTextureFormat(device, sdlWindow);
        if (colorTargetDesc.format == SDL_GPU_TEXTUREFORMAT_INVALID)
        {
            m_logger->warn("Swapchain format invalid, falling back to B8G8R8A8_UNORM");
            colorTargetDesc.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
        }
        colorTargetDesc.blend_state = buildBlendState();

        // 光栅化状态
        const SDL_GPURasterizerState rasterizerState = buildRasterizerState();

        SDL_GPUMultisampleState multisampleState = {};
        multisampleState.sample_count = SDL_GPU_SAMPLECOUNT_1;

        // 深度/模板状态
        const SDL_GPUDepthStencilState depthStencilState = buildDepthStencilState();

        // 创建图形管线
        SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.vertex_shader = m_vertexShader.get();
        pipelineInfo.fragment_shader = m_fragmentShader.get();
        pipelineInfo.vertex_input_state = vertexInputState;
        pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipelineInfo.rasterizer_state = rasterizerState;
        pipelineInfo.multisample_state = multisampleState;
        pipelineInfo.depth_stencil_state = depthStencilState;
        pipelineInfo.target_info.num_color_targets = 1;
        pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;

        if (detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::RESOURCE_CREATE))
        {
            m_creationFailed = true;
            return Err(UiErrc::PIPELINE_UNAVAILABLE, "injected pipeline creation failure");
        }
        auto candidatePipeline = wrappers::MakeGpuResource<wrappers::UniqueGPUGraphicsPipeline>(
            generation, SDL_CreateGPUGraphicsPipeline, &pipelineInfo);

        if (candidatePipeline == nullptr)
        {
            m_logger->error("图形管线创建失败: {}", SDL_GetError());
            m_creationFailed = true;  // 标记失败，阻止后续重试
            return Err(UiErrc::PIPELINE_UNAVAILABLE,
                       SDL_GetError());  // 管线失败则不创建采样器，避免下次 guard 失效导致重复重试
        }

        // 创建采样器
        SDL_GPUSamplerCreateInfo samplerInfo = {};
        samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
        samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
        samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

        if (detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::RESOURCE_CREATE))
        {
            m_creationFailed = true;
            return Err(UiErrc::PIPELINE_UNAVAILABLE, "injected sampler creation failure");
        }
        auto candidateSampler =
            wrappers::MakeGpuResource<wrappers::UniqueGPUSampler>(generation, SDL_CreateGPUSampler, &samplerInfo);
        if (candidateSampler == nullptr)
        {
            m_logger->error("采样器创建失败: {}", SDL_GetError());
            m_creationFailed = true;
            return Err(UiErrc::PIPELINE_UNAVAILABLE, SDL_GetError());
        }
        if (generation.Status() != detail::GpuDeviceGenerationStatus::ACTIVE)
        {
            return Err(UiErrc::DEVICE_UNAVAILABLE);
        }
        m_pipeline = std::move(candidatePipeline);
        m_sampler = std::move(candidateSampler);
        return Ok();
    }

    void cleanup()
    {
        m_sampler.reset();
        m_pipeline.reset();
        m_vertexShader.reset();
        m_fragmentShader.reset();
        m_generationId = 0;
        m_creationFailed = false;
    }

    [[nodiscard]] SDL_GPUGraphicsPipeline* getPipeline() const
    {
        return m_pipeline.get();
    }
    [[nodiscard]] SDL_GPUSampler* getSampler() const
    {
        return m_sampler.get();
    }
    [[nodiscard]] bool hasCreationFailed() const
    {
        return m_creationFailed;
    }

   private:
    [[nodiscard]] static std::array<SDL_GPUVertexAttribute, 7> buildVertexAttributes()
    {
        std::array<SDL_GPUVertexAttribute, 7> vertexAttributes{};

        vertexAttributes[0] = {.location = 0,
                               .buffer_slot = 0,
                               .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                               .offset = static_cast<uint32_t>(offsetof(ui::render::Vertex, position))};
        vertexAttributes[1] = {.location = 1,
                               .buffer_slot = 0,
                               .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                               .offset = static_cast<uint32_t>(offsetof(ui::render::Vertex, texCoord))};
        vertexAttributes[2] = {.location = 2,
                               .buffer_slot = 0,
                               .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                               .offset = static_cast<uint32_t>(offsetof(ui::render::Vertex, color))};
        vertexAttributes[3] = {.location = 3,
                               .buffer_slot = 0,
                               .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                               .offset = static_cast<uint32_t>(offsetof(ui::render::Vertex, rect_size))};
        vertexAttributes[4] = {.location = 4,
                               .buffer_slot = 0,
                               .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                               .offset = static_cast<uint32_t>(offsetof(ui::render::Vertex, radius))};
        vertexAttributes[5] = {.location = 5,
                               .buffer_slot = 0,
                               .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                               .offset = static_cast<uint32_t>(offsetof(ui::render::Vertex, shadow_params))};
        vertexAttributes[6] = {.location = 6,
                               .buffer_slot = 0,
                               .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                               .offset = static_cast<uint32_t>(offsetof(ui::render::Vertex, mode_params))};
        return vertexAttributes;
    }

    [[nodiscard]] static SDL_GPUVertexInputState buildVertexInputState(
        SDL_GPUVertexBufferDescription& vertexBufferDesc, const std::array<SDL_GPUVertexAttribute, 7>& vertexAttributes)
    {
        SDL_GPUVertexInputState vertexInputState = {};
        vertexInputState.vertex_buffer_descriptions = &vertexBufferDesc;
        vertexInputState.num_vertex_buffers = 1;
        vertexInputState.vertex_attributes = vertexAttributes.data();
        vertexInputState.num_vertex_attributes = static_cast<uint32_t>(vertexAttributes.size());
        return vertexInputState;
    }

    [[nodiscard]] static SDL_GPUColorTargetBlendState buildBlendState()
    {
        SDL_GPUColorTargetBlendState blendState = {};
        blendState.enable_blend = true;
        blendState.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        blendState.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        blendState.color_blend_op = SDL_GPU_BLENDOP_ADD;
        blendState.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        blendState.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        blendState.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        blendState.color_write_mask =
            SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A;
        blendState.enable_color_write_mask = true;
        return blendState;
    }

    [[nodiscard]] static SDL_GPURasterizerState buildRasterizerState()
    {
        SDL_GPURasterizerState rasterizerState = {};
        rasterizerState.fill_mode = SDL_GPU_FILLMODE_FILL;
        rasterizerState.cull_mode = SDL_GPU_CULLMODE_NONE;
        rasterizerState.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        rasterizerState.enable_depth_clip = true;
        return rasterizerState;
    }

    [[nodiscard]] static SDL_GPUDepthStencilState buildDepthStencilState()
    {
        SDL_GPUDepthStencilState depthStencilState = {};
        depthStencilState.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
        depthStencilState.back_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
        depthStencilState.back_stencil_state.fail_op = SDL_GPU_STENCILOP_KEEP;
        depthStencilState.back_stencil_state.pass_op = SDL_GPU_STENCILOP_KEEP;
        depthStencilState.back_stencil_state.depth_fail_op = SDL_GPU_STENCILOP_KEEP;
        depthStencilState.front_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
        depthStencilState.front_stencil_state.fail_op = SDL_GPU_STENCILOP_KEEP;
        depthStencilState.front_stencil_state.pass_op = SDL_GPU_STENCILOP_KEEP;
        depthStencilState.front_stencil_state.depth_fail_op = SDL_GPU_STENCILOP_KEEP;
        depthStencilState.enable_depth_test = false;
        depthStencilState.enable_stencil_test = false;
        return depthStencilState;
    }

    wrappers::UniqueGPUShader loadShaderFromResource(const detail::GpuDeviceGenerationHandle& generation,
                                                     const char* resourcePath, SDL_GPUShaderStage stage,
                                                     SDL_GPUShaderFormat format)
    {
        if (m_resourceProvider == nullptr)
        {
            m_logger->error("着色器资源提供器未初始化: {}", resourcePath);
            return nullptr;
        }

        auto resourceResult = ui::cpo::load_binary_resource(*m_resourceProvider, resourcePath);
        if (!resourceResult.has_value())
        {
            m_logger->error("着色器资源加载失败: {} ({})", resourcePath,
                                                    resourceResult.error().ToString());
            return nullptr;
        }

        const BinaryResource& resource = resourceResult.value();
        SDL_GPUShaderCreateInfo shaderInfo = {};
        shaderInfo.code = std::bit_cast<const uint8_t*>(resource.data());
        shaderInfo.code_size = resource.size();
        shaderInfo.entrypoint = (stage == SDL_GPU_SHADERSTAGE_VERTEX) ? "main_vs" : "main_ps";
        shaderInfo.format = format;
        shaderInfo.stage = stage;
        shaderInfo.num_samplers = (stage == SDL_GPU_SHADERSTAGE_FRAGMENT) ? 1U : 0U;
        shaderInfo.num_uniform_buffers = 1U;

        if (detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::RESOURCE_CREATE))
        {
            return nullptr;
        }
        return wrappers::MakeGpuResource<wrappers::UniqueGPUShader>(generation, SDL_CreateGPUShader, &shaderInfo);
    }

    DeviceManager* m_deviceManager;
    utils::Logger* m_logger;
    std::shared_ptr<const IResourceProvider> m_resourceProvider;
    wrappers::UniqueGPUGraphicsPipeline m_pipeline;
    wrappers::UniqueGPUShader m_vertexShader;
    wrappers::UniqueGPUShader m_fragmentShader;
    wrappers::UniqueGPUSampler m_sampler;
    std::uint64_t m_generationId = 0;
    bool m_creationFailed = false;
};

}  // namespace ui::managers
