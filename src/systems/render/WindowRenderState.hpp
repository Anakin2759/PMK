/**
 * @file WindowRenderState.hpp
 * @brief Runtime-local 的每窗口渲染状态。
 */

#pragma once

#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_set>

#include "core/RenderContext.hpp"
#include "interface/IBackendRenderer.hpp"
#include "utils/Logger.hpp"

namespace ui::systems::render_detail
{
inline constexpr float TEXT_SCALE_QUANTIZATION = 100.0F;

[[nodiscard]] inline int MakeTextScaleKey(float renderScale) noexcept
{
    constexpr float MIN_TEXT_OVERSAMPLE = 2.0F;
    const float normalized = std::isfinite(renderScale) && renderScale > 0.0F ? renderScale : 1.0F;
    return static_cast<int>(std::lround(std::max(MIN_TEXT_OVERSAMPLE, normalized) * TEXT_SCALE_QUANTIZATION));
}

struct ScalingSnapshot
{
    int pixelWidth = 0;
    int pixelHeight = 0;
    int logicalWidth = 0;
    int logicalHeight = 0;
    int rootWidth = 0;
    int rootHeight = 0;
    int batchCount = 0;
    float dpiScale = 1.0F;
    float clearAlpha = 1.0F;
    int nativeClientWidth = 0;
    int nativeClientHeight = 0;
    int nativeWindowWidth = 0;
    int nativeWindowHeight = 0;
    int borderTop = 0;
    int borderBottom = 0;

    [[nodiscard]] bool operator==(const ScalingSnapshot& other) const = default;
};

class WindowCapabilityDiagnostics final : public core::IRenderCapabilityDiagnostics
{
   public:
    void SetLogger(utils::Logger& logger) noexcept
    {
        m_logger = &logger;
    }

    void report(interface::BackendCapability capability, interface::BackendCapabilityStatus status,
                std::string_view feature, std::string_view fallbackAction) override
    {
        const auto key = (static_cast<std::uint16_t>(capability) << 8U) | static_cast<std::uint16_t>(status);
        if (m_logger == nullptr || !m_reported.insert(key).second)
        {
            return;
        }
        m_logger->warn("[RenderCapability] {} is {}: {}", feature,
                       status == interface::BackendCapabilityStatus::DEGRADED ? "degraded" : "unsupported",
                       fallbackAction);
    }

   private:
    utils::Logger* m_logger = nullptr;
    std::unordered_set<std::uint16_t> m_reported;
};

struct WindowRenderState
{
    std::uint32_t windowID = 0;
    int textScaleKey = MakeTextScaleKey(1.0F);
    std::optional<ScalingSnapshot> scalingSnapshot;
    std::unique_ptr<interface::IBackendRenderer> fallbackBackend;
    WindowCapabilityDiagnostics capabilityDiagnostics;
};
}  // namespace ui::systems::render_detail