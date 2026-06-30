/**
 * ************************************************************************
 *
 * @file EntityTypes.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-05-30
 * @version 0.1
 * @brief ui::entity 实体句柄的单一权威定义。
 *
 * 过渡期仍映射到 entt::entity，以保持系统层和服务层既有内部调用可构建。
 * 后续按 `修改规划.md` 完成全量边界迁移后，再切换为 std::uint32_t。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <cstdint>


namespace ui
{

using entity = uint32_t;

/// @brief 空实体常量，等价于 entt::null。
inline constexpr entity null_entity = 0xFFFFFFFFU;

/**
 * @brief 运行时归属令牌。
 *
 * RuntimeToken 是公开句柄的轻量归属标识，只能由 UiRuntime 生成。
 * 外部代码可以比较和判空，但不能伪造一个有效 token。
 */
class RuntimeToken
{
public:
	constexpr RuntimeToken() noexcept = default;

	[[nodiscard]] constexpr bool valid() const noexcept { return m_value != 0; }

	[[nodiscard]] constexpr explicit operator bool() const noexcept { return valid(); }

	[[nodiscard]] friend constexpr bool operator==(RuntimeToken lhs, RuntimeToken rhs) noexcept = default;

private:
	friend class UiRuntime;

	explicit constexpr RuntimeToken(const void* value) noexcept : m_value(value) {}

	const void* m_value = nullptr;
};

/**
 * @brief runtime-aware 实体句柄。
 *
 * 这是替代裸 ui::entity 的公开操作句柄骨架。当前阶段先落地归属校验能力，
 * 旧 API 迁移到显式 runtime/window 入口时逐步改为返回 EntityHandle。
 */
class EntityHandle
{
public:
	constexpr EntityHandle() noexcept = default;
	constexpr EntityHandle(RuntimeToken runtime, entity value) noexcept : m_runtime(runtime), m_entity(value) {}

	[[nodiscard]] constexpr RuntimeToken runtime() const noexcept { return m_runtime; }

	[[nodiscard]] constexpr entity id() const noexcept { return m_entity; }

	[[nodiscard]] constexpr bool valid() const noexcept { return m_runtime.valid() && m_entity != null_entity; }

	[[nodiscard]] constexpr explicit operator bool() const noexcept { return valid(); }

	[[nodiscard]] friend constexpr bool operator==(EntityHandle lhs, EntityHandle rhs) noexcept = default;

private:
	RuntimeToken m_runtime{};
	entity m_entity = null_entity;
};

/**
 * @brief runtime-aware 窗口句柄。
 */
class WindowHandle
{
public:
	constexpr WindowHandle() noexcept = default;
	constexpr WindowHandle(RuntimeToken runtime, entity value, std::uint32_t platformWindowId = 0) noexcept
		: m_entity(runtime, value), m_platformWindowId(platformWindowId)
	{
	}

	[[nodiscard]] constexpr RuntimeToken runtime() const noexcept { return m_entity.runtime(); }

	[[nodiscard]] constexpr entity id() const noexcept { return m_entity.id(); }

	[[nodiscard]] constexpr std::uint32_t platformWindowId() const noexcept { return m_platformWindowId; }

	[[nodiscard]] constexpr bool valid() const noexcept { return m_entity.valid(); }

	[[nodiscard]] constexpr explicit operator bool() const noexcept { return valid(); }

	[[nodiscard]] friend constexpr bool operator==(WindowHandle lhs, WindowHandle rhs) noexcept = default;

private:
	EntityHandle m_entity{};
	std::uint32_t m_platformWindowId = 0;
};

[[nodiscard]] constexpr EntityHandle MakeEntityHandle(RuntimeToken runtime, entity value) noexcept
{
	return EntityHandle{runtime, value};
}

[[nodiscard]] constexpr WindowHandle
	MakeWindowHandle(RuntimeToken runtime, entity value, std::uint32_t platformWindowId = 0) noexcept
{
	return WindowHandle{runtime, value, platformWindowId};
}

[[nodiscard]] constexpr bool SameRuntime(RuntimeToken lhs, RuntimeToken rhs) noexcept
{
	return lhs.valid() && lhs == rhs;
}

[[nodiscard]] constexpr bool SameRuntime(EntityHandle lhs, EntityHandle rhs) noexcept
{
	return SameRuntime(lhs.runtime(), rhs.runtime());
}

[[nodiscard]] constexpr bool SameRuntime(WindowHandle lhs, WindowHandle rhs) noexcept
{
	return SameRuntime(lhs.runtime(), rhs.runtime());
}

} // namespace ui
