#pragma once

namespace ui::systems::render
{

/**
 * @brief GPU 设备从候选创建到资源就绪的最小生命周期状态。
 */
class DeviceClaimState final
{
public:
    /** @brief 首个窗口成功声明后锁定设备代际。 */
    void MarkDeviceLocked() noexcept { m_deviceLocked = true; }

    /** @brief 仅允许在设备锁定后标记设备资源就绪。 */
    [[nodiscard]] bool MarkResourcesReady() noexcept
    {
        if (!m_deviceLocked)
        {
            return false;
        }
        m_resourcesReady = true;
        return true;
    }

    [[nodiscard]] bool IsDeviceLocked() const noexcept { return m_deviceLocked; }
    [[nodiscard]] bool AreResourcesReady() const noexcept { return m_resourcesReady; }
    [[nodiscard]] bool MayTryAnotherBackend() const noexcept { return !m_deviceLocked; }
    [[nodiscard]] bool MayCreateDeviceResources() const noexcept { return m_deviceLocked && !m_resourcesReady; }
    [[nodiscard]] bool MayCreateWhiteTexture() const noexcept { return m_deviceLocked && m_resourcesReady; }

    void Reset() noexcept
    {
        m_resourcesReady = false;
        m_deviceLocked = false;
    }

private:
    bool m_deviceLocked = false;
    bool m_resourcesReady = false;
};

} // namespace ui::systems::render
