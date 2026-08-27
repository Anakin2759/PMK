/**
 * ************************************************************************
 *
 * @file ResourceProvider.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-03-14
 * @version 0.1
 * @brief UI 资源抽象层默认实现（cmrc 后端）
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#include "ResourceProvider.hpp"
#include "core/UiRuntime.hpp"
#include "ui/ErrorCodes.hpp"
#include "utils/Logger.hpp"
#include <string_view>
#include <string>
#include "ui/Result.hpp"
#include <memory>
#include <span>

#ifdef UI_RESOURCE_BACKEND_CMRC
#include <cmrc/cmrc.hpp>

// NOLINTNEXTLINE(modernize-type-traits)
CMRC_DECLARE(ui_fonts);
#elifdef UI_RESOURCE_BACKEND_STD_EMBED
#include "StdEmbedResourceTable.hpp"
#endif

namespace ui::managers
{
namespace
{

#ifdef UI_RESOURCE_BACKEND_CMRC
class CmrcResourceProvider final : public IResourceProvider
{
   public:
    explicit CmrcResourceProvider(utils::Logger& logger) : IResourceProvider(logger) {}
    [[nodiscard]] bool exists(std::string_view path) const override
    {
        const auto fileSystem = cmrc::ui_fonts::get_filesystem();
        return fileSystem.exists(std::string(path));
    }

    [[nodiscard]] ui::Result<BinaryResource> loadBinary(std::string_view path) const override
    {
        const auto fileSystem = cmrc::ui_fonts::get_filesystem();
        const std::string normalizedPath(path);
        if (!fileSystem.exists(normalizedPath))
        {
            m_logger->error("[ResourceProvider/cmrc] resource not found: {}", normalizedPath);
            return ui::Err(ui::UiErrc::ASSET_NOT_FOUND, normalizedPath);
        }

        auto file = std::make_shared<cmrc::file>(fileSystem.open(normalizedPath));

        BinaryResource resource{};
        resource.owner = std::shared_ptr<const void>(file, file.get());
        resource.bytes = std::as_bytes(std::span(file->begin(), file->size()));
        return resource;
    }
};
#endif

#ifdef UI_RESOURCE_BACKEND_STD_EMBED
class StdEmbedResourceProvider final : public IResourceProvider
{
   public:
    explicit StdEmbedResourceProvider(utils::Logger& logger) : IResourceProvider(logger) {}
    [[nodiscard]] bool exists(std::string_view path) const override
    {
        return FindStdEmbedResource(path) != nullptr;
    }

    [[nodiscard]] ui::Result<BinaryResource> loadBinary(std::string_view path) const override
    {
        const StdEmbedResourceEntry* entry = FindStdEmbedResource(path);
        if (entry == nullptr)
        {
            m_logger->error("[ResourceProvider/std_embed] resource not found: {}", path);
            return ui::Err(ui::UiErrc::ASSET_NOT_FOUND, std::string(path));
        }

        BinaryResource resource{};
        resource.bytes = entry->bytes;
        return resource;
    }
};
#endif

#if !defined(UI_RESOURCE_BACKEND_STD_EMBED) && !defined(UI_RESOURCE_BACKEND_CMRC)
class UnavailableResourceProvider final : public IResourceProvider
{
   public:
    explicit UnavailableResourceProvider(utils::Logger& logger) : IResourceProvider(logger) {}
    [[nodiscard]] bool exists(std::string_view path) const override
    {
        static_cast<void>(path);
        return false;
    }

    [[nodiscard]] ui::Result<BinaryResource> loadBinary(std::string_view path) const override
    {
        m_logger->error(
            "[ResourceProvider] no UI resource backend selected at compile time for: {}", path);
        static_cast<void>(path);
        return ui::Err(ui::UiErrc::BACKEND_UNAVAILABLE, std::string(path));
    }
};
#endif

}  // namespace

std::shared_ptr<const IResourceProvider> GetDefaultUiResourceProvider(utils::Logger& logger)
{
#ifdef UI_RESOURCE_BACKEND_STD_EMBED
    static const std::shared_ptr<const IResourceProvider> defaultUiResourceProvider =
        std::make_shared<StdEmbedResourceProvider>(logger);
    return defaultUiResourceProvider;
#elifdef UI_RESOURCE_BACKEND_CMRC
    static const std::shared_ptr<const IResourceProvider> defaultUiResourceProvider =
        std::make_shared<CmrcResourceProvider>(logger);
    return defaultUiResourceProvider;
#else
    static const std::shared_ptr<const IResourceProvider> defaultUiResourceProvider =
        std::make_shared<UnavailableResourceProvider>(logger);
    return defaultUiResourceProvider;
#endif
}

}  // namespace ui::managers