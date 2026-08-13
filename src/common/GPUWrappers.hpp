/**
 * ************************************************************************
 *
 * @file GPUWrappers.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-02-07
 * @version 0.1
 * @brief SDL GPU 资源管理 RAII 包装器
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "GpuDeviceGeneration.hpp"

#include <memory>
#include <utility>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

namespace ui::wrappers
{

/**
 * @brief RAII wrapper for SDL_PropertiesID (Uint32 handle)
 */
class UniquePropertiesID
{
   public:
    UniquePropertiesID() : m_id(0)
    {
    }
    explicit UniquePropertiesID(SDL_PropertiesID propertiesId) : m_id(propertiesId)
    {
    }
    ~UniquePropertiesID()
    {
        reset();
    }

    UniquePropertiesID(const UniquePropertiesID&) = delete;
    UniquePropertiesID& operator=(const UniquePropertiesID&) = delete;

    UniquePropertiesID(UniquePropertiesID&& other) noexcept : m_id(other.m_id)
    {
        other.m_id = 0;
    }

    UniquePropertiesID& operator=(UniquePropertiesID&& other) noexcept
    {
        if (this != &other)
        {
            reset();
            m_id = other.m_id;
            other.m_id = 0;
        }
        return *this;
    }

    void reset(SDL_PropertiesID newId = 0)
    {
        if (m_id != 0)
        {
            SDL_DestroyProperties(m_id);
        }
        m_id = newId;
    }
    /**
     * @brief  隐式转换为 SDL_PropertiesID 以便直接传递给 SDL 函数
     * @return SDL_PropertiesID
     */
    operator SDL_PropertiesID() const
    {
        return m_id;
    }  // NOLINT
    [[nodiscard]] SDL_PropertiesID get() const
    {
        return m_id;
    }

   private:
    SDL_PropertiesID m_id;
};

/**
 * @brief Deleter for SDL_GPUDevice
 */
struct GPUDeviceDeleter
{
    void operator()(SDL_GPUDevice* device) const
    {
        if (device != nullptr)
        {
            SDL_DestroyGPUDevice(device);
        }
    }
};

using UniqueGPUDevice = std::unique_ptr<SDL_GPUDevice, GPUDeviceDeleter>;

/**
 * @brief 使用共享设备代际释放 SDL GPU 资源的删除器
 * @tparam RELEASE_FUNC 对应资源类型的 SDL Release 函数
 *
 * 删除器不拥有 SDL_GPUDevice。资源可晚于 DeviceManager 存活：代际仍活动时正常释放，
 * 代际失效后仅记录防御性的晚释放跳过，不会改用后续代际设备释放旧资源。
 */
template <auto RELEASE_FUNC>
struct GPUResourceDeleter
{
    ::ui::detail::GpuDeviceGenerationHandle generation;

    /** @brief 构造不绑定设备代际的空 owner 删除器。 */
    GPUResourceDeleter() = default;

    /**
     * @brief 构造绑定指定设备代际的删除器
     * @param generationHandle 仅共享代际状态、不拥有设备的句柄
     */
    explicit GPUResourceDeleter(::ui::detail::GpuDeviceGenerationHandle generationHandle)
        : generation(std::move(generationHandle))
    {
    }

    template <typename T>
    void operator()(T* resource) const
    {
        if (resource == nullptr)
        {
            return;
        }
        if (generation)
        {
            generation.ReleaseOrSkip(resource, RELEASE_FUNC);
        }
    }
};

// Define unique_ptr types for various SDL GPU resources
using UniqueGPUBuffer = std::unique_ptr<SDL_GPUBuffer, GPUResourceDeleter<SDL_ReleaseGPUBuffer>>;
using UniqueGPUTransferBuffer =
    std::unique_ptr<SDL_GPUTransferBuffer, GPUResourceDeleter<SDL_ReleaseGPUTransferBuffer>>;
using UniqueGPUTexture = std::unique_ptr<SDL_GPUTexture, GPUResourceDeleter<SDL_ReleaseGPUTexture>>;
using UniqueGPUShader = std::unique_ptr<SDL_GPUShader, GPUResourceDeleter<SDL_ReleaseGPUShader>>;
using UniqueGPUSampler = std::unique_ptr<SDL_GPUSampler, GPUResourceDeleter<SDL_ReleaseGPUSampler>>;
using UniqueGPUGraphicsPipeline =
    std::unique_ptr<SDL_GPUGraphicsPipeline, GPUResourceDeleter<SDL_ReleaseGPUGraphicsPipeline>>;

/**
 * @brief 在指定活动代际中创建并接纳 GPU 资源
 * @param generation 资源必须绑定的设备代际句柄
 * @param creator 接收 SDL_GPUDevice 及其余参数的资源创建函数
 * @return 绑定同一代际的 move-only owner；代际无效或创建失败时为空
 * @note 不提供裸设备工厂，避免生成无法验证设备代际的资源 owner。
 */
template <typename UniqueType, typename CreatorFunc, typename... Args>
UniqueType MakeGpuResource(const ::ui::detail::GpuDeviceGenerationHandle& generation, CreatorFunc creator,
                           Args&&... args)
{
    using DeleterType = typename UniqueType::deleter_type;
    auto resource =
        generation.InvokeIfActive([&](SDL_GPUDevice* device) { return creator(device, std::forward<Args>(args)...); });
    return UniqueType(resource.value_or(nullptr), DeleterType(generation));
}

}  // namespace ui::wrappers
