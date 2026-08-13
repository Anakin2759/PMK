/**
 * ************************************************************************
 *
 * @file GpuDeviceGeneration.hpp
 * @brief 内部 SDL GPU device 代际状态与共享句柄
 *
 * ************************************************************************
 */
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>

#include <SDL3/SDL_gpu.h>

namespace ui::detail
{

/**
 * @brief GPU 设备代际的生命周期状态
 *
 * 失效过程先进入 INVALIDATING 并清空非拥有设备指针，再进入 INVALID。
 */
enum class GpuDeviceGenerationStatus : std::uint8_t
{
    ACTIVE,
    INVALIDATING,
    INVALID
};

class GpuDeviceGenerationHandle;

/**
 * @brief 由 DeviceManager 持有的设备代际控制器
 *
 * 共享状态仅记录非拥有的 SDL_GPUDevice 指针，不负责销毁设备。生产路径中只有
 * DeviceManager 创建该控制器并调用 Invalidate()；控制器失效后，共享句柄仍可晚于
 * DeviceManager 存活，以便资源 deleter 安全识别已销毁的设备代际。
 */
class GpuDeviceGeneration final
{
public:
    /**
     * @brief 为指定设备建立新的活动代际
     * @param device 由 DeviceManager 拥有的非拥有设备指针
     */
    explicit GpuDeviceGeneration(SDL_GPUDevice* device)
        : m_state(std::make_shared<State>(NextId(), device))
    {
    }
    ~GpuDeviceGeneration() = default;

    GpuDeviceGeneration(const GpuDeviceGeneration&) = delete;
    GpuDeviceGeneration& operator=(const GpuDeviceGeneration&) = delete;
    GpuDeviceGeneration(GpuDeviceGeneration&&) noexcept = default;
    GpuDeviceGeneration& operator=(GpuDeviceGeneration&&) noexcept = default;

    /** @brief 获取共享本代状态但不拥有设备的句柄。 */
    [[nodiscard]] GpuDeviceGenerationHandle GetHandle() const noexcept;

    /**
     * @brief 使本代永久失效并清空非拥有设备指针
     * @note DeviceManager 必须在销毁 SDL_GPUDevice 前调用；重复调用不改变状态。
     */
    void Invalidate() noexcept
    {
        if (m_state == nullptr)
        {
            return;
        }

        const std::scoped_lock lock(m_state->mutex);
        if (m_state->status != GpuDeviceGenerationStatus::ACTIVE)
        {
            return;
        }
        m_state->status = GpuDeviceGenerationStatus::INVALIDATING;
        m_state->device = nullptr;
        m_state->status = GpuDeviceGenerationStatus::INVALID;
    }

private:
    struct State final
    {
        State(std::uint64_t generationId, SDL_GPUDevice* gpuDevice) : id(generationId), device(gpuDevice) {}

        mutable std::mutex mutex;
        const std::uint64_t id;
        SDL_GPUDevice* device;
        GpuDeviceGenerationStatus status = GpuDeviceGenerationStatus::ACTIVE;
        std::uint64_t lateReleaseSkipped = 0;
    };

    [[nodiscard]] static std::uint64_t NextId() noexcept
    {
        static std::atomic_uint64_t nextId{1};
        return nextId.fetch_add(1, std::memory_order_relaxed);
    }

    std::shared_ptr<State> m_state;

    friend class GpuDeviceGenerationHandle;
};

/**
 * @brief 共享设备代际状态的非设备所有句柄
 *
 * 句柄仅延长状态对象寿命，不延长 SDL_GPUDevice 寿命。设备失效后，句柄仍可查询状态与
 * LateReleaseSkipped()，用于观测防御性晚释放；它不能恢复设备或使代际失效。
 */
class GpuDeviceGenerationHandle final
{
public:
    GpuDeviceGenerationHandle() = default;

    [[nodiscard]] explicit operator bool() const noexcept { return m_state != nullptr; }

    [[nodiscard]] std::uint64_t Id() const noexcept
    {
        return m_state == nullptr ? 0 : m_state->id;
    }

    [[nodiscard]] GpuDeviceGenerationStatus Status() const noexcept
    {
        if (m_state == nullptr)
        {
            return GpuDeviceGenerationStatus::INVALID;
        }
        const std::scoped_lock lock(m_state->mutex);
        return m_state->status;
    }

    [[nodiscard]] std::uint64_t LateReleaseSkipped() const noexcept
    {
        if (m_state == nullptr)
        {
            return 0;
        }
        const std::scoped_lock lock(m_state->mutex);
        return m_state->lateReleaseSkipped;
    }

    template <typename Function>
    /**
     * @brief 仅在本代仍活动时以非拥有设备指针调用函数
     * @return 活动时返回调用结果，否则返回 std::nullopt
     */
    [[nodiscard]] auto InvokeIfActive(Function&& function) const
        -> std::optional<std::invoke_result_t<Function, SDL_GPUDevice*>>
    {
        using Result = std::invoke_result_t<Function, SDL_GPUDevice*>;
        static_assert(!std::is_void_v<Result>);

        if (m_state == nullptr)
        {
            return std::nullopt;
        }
        const std::scoped_lock lock(m_state->mutex);
        if (m_state->status != GpuDeviceGenerationStatus::ACTIVE || m_state->device == nullptr)
        {
            return std::nullopt;
        }
        return std::invoke(std::forward<Function>(function), m_state->device);
    }

    template <typename Resource, typename ReleaseFunction>
    /**
     * @brief 在本代活动时释放资源，否则记录一次晚释放跳过
     * @note 跳过是防止解引用已销毁设备的异常兜底，不是正常关闭时保留资源的策略；
     * 正常关闭应先释放全部 owner，并保持 LateReleaseSkipped() 为 0。
     */
    void ReleaseOrSkip(Resource* resource, ReleaseFunction releaseFunction) const noexcept
    {
        if (resource == nullptr || m_state == nullptr)
        {
            return;
        }

        const std::scoped_lock lock(m_state->mutex);
        if (m_state->status == GpuDeviceGenerationStatus::ACTIVE && m_state->device != nullptr)
        {
            std::invoke(releaseFunction, m_state->device, resource);
            return;
        }
        ++m_state->lateReleaseSkipped;
    }

private:
    explicit GpuDeviceGenerationHandle(std::shared_ptr<GpuDeviceGeneration::State> state) noexcept
        : m_state(std::move(state))
    {
    }

    std::shared_ptr<GpuDeviceGeneration::State> m_state;

    friend class GpuDeviceGeneration;
};

inline GpuDeviceGenerationHandle GpuDeviceGeneration::GetHandle() const noexcept
{
    return GpuDeviceGenerationHandle(m_state);
}

} // namespace ui::detail
