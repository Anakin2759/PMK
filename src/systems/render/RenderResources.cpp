/**
 * @file RenderResources.cpp
 * @brief RenderSystem -- GPU 资源创建与渲染器注册
 *
 * 包含：initializeRenderers()
 */

#include "systems/RenderSystem.hpp"
#include "RenderSystemImpl.hpp"
#include <algorithm>
#include <memory>
#include "renderers/ShapeRenderer.hpp"
#include "renderers/TextRenderer.hpp"
#include "renderers/IconRenderer.hpp"
#include "renderers/ScrollBarRenderer.hpp"
#include "renderers/SliderRenderer.hpp"
#include "renderers/ProgressBarRenderer.hpp"
#include "renderers/ImageRenderer.hpp"
#include "renderers/CanvasRenderer.hpp"
#include "renderers/TableRenderer.hpp"
#include "renderers/CheckBoxRenderer.hpp"
#include "renderers/DropDownRenderer.hpp"
#include "interface/IRenderer.hpp"
#include "core/UiRuntime.hpp"
#include "utils/Logger.hpp"

namespace ui::systems
{

void RenderSystem::initializeRenderers()
{
    m_impl->m_renderers.push_back(std::make_unique<renderers::ShapeRenderer>(*m_reg));
    m_impl->m_renderers.push_back(std::make_unique<renderers::ProgressBarRenderer>(*m_reg));
    m_impl->m_renderers.push_back(std::make_unique<renderers::SliderRenderer>(*m_reg));
    m_impl->m_renderers.push_back(std::make_unique<renderers::TextRenderer>(*m_reg));
    if (m_impl->m_iconManager)
    {
        m_impl->m_renderers.push_back(std::make_unique<renderers::IconRenderer>(*m_reg, m_impl->m_iconManager.get()));
    }
    m_impl->m_renderers.push_back(std::make_unique<renderers::ScrollBarRenderer>(*m_reg));
    m_impl->m_renderers.push_back(std::make_unique<renderers::ImageRenderer>(*m_reg));
    m_impl->m_renderers.push_back(std::make_unique<renderers::CanvasRenderer>(*m_reg));
    m_impl->m_renderers.push_back(std::make_unique<renderers::TableRenderer>(*m_reg));
    m_impl->m_renderers.push_back(std::make_unique<renderers::CheckBoxRenderer>(*m_reg));
    m_impl->m_renderers.push_back(std::make_unique<renderers::DropDownRenderer>(*m_reg));

    std::ranges::sort(m_impl->m_renderers, [](const std::unique_ptr<core::IRenderer>& leftRenderer,
                                              const std::unique_ptr<core::IRenderer>& rightRenderer)
                      { return leftRenderer->getPriority() < rightRenderer->getPriority(); });

    ui::UiRuntime::current().logger().info("[RenderSystem] 初始化了 {} 个渲染器", m_impl->m_renderers.size());
}

}  // namespace ui::systems
