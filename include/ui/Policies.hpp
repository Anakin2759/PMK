#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace ui::policies
{
enum class SystemManager : std::uint16_t
{
    DISABLE_ALL = 0,
    INTERACTION = 1U << 0U,
    HIT_TEST = 1U << 1U,
    TWEEN = 1U << 2U,
    LAYOUT = 1U << 3U,
    RENDER = 1U << 4U,
    STATE = 1U << 5U,
    ACTION = 1U << 6U,
    TIMER = 1U << 7U,
    THEME = 1U << 8U,
    DEFAULT = INTERACTION | HIT_TEST | TWEEN | LAYOUT | RENDER | STATE | ACTION | TIMER | THEME
};

enum class LayoutDirection : std::uint8_t { HORIZONTAL, VERTICAL };

enum class Alignment : std::uint8_t
{
    NONE = 0,
    LEFT = 1U << 0U,
    HCENTER = 1U << 1U,
    RIGHT = 1U << 2U,
    TOP = 1U << 3U,
    VCENTER = 1U << 4U,
    BOTTOM = 1U << 5U,
    CENTER = HCENTER | VCENTER,
    TOP_LEFT = TOP | LEFT
};

enum class Play : std::uint8_t { ONCE, LOOP, PINGPONG };

enum class Easing : std::uint8_t
{
    LINEAR,
    EASE_IN_SINE,
    EASE_OUT_SINE,
    EASE_IN_OUT_SINE,
    EASE_IN_QUAD,
    EASE_OUT_QUAD,
    EASE_IN_OUT_QUAD,
    CUSTOM
};

enum class Focus : std::uint8_t { NOFOCUS, TAB_FOCUS, CLICK_FOCUS, STRONG_FOCUS };

enum class Size : std::uint8_t
{
    NONE = 0,
    H_FIXED = 1 << 0,
    H_AUTO = 1 << 1,
    H_FILL = 1 << 2,
    H_PERCENTAGE = 1 << 3,
    V_FIXED = 1 << 4,
    V_AUTO = 1 << 5,
    V_FILL = 1 << 6,
    V_PERCENTAGE = 1 << 7,
    FIXED = H_FIXED | V_FIXED,
    AUTO = H_AUTO | V_AUTO,
    FILL_PARENT = H_FILL | V_FILL,
    PERCENTAGE = H_PERCENTAGE | V_PERCENTAGE,
    H_FIXED_V_AUTO = H_FIXED | V_AUTO,
    H_AUTO_V_FIXED = H_AUTO | V_FIXED,
    H_FILL_V_AUTO = H_FILL | V_AUTO
};

enum class Feature : std::uint8_t { DISABLED, ENABLED };
enum class Visibility : std::uint8_t { VISIBLE, HIDDEN, COLLAPSED };
enum class TextWrap : std::uint8_t { NONE, WORD, CHAR };
enum class TextFlag : std::uint16_t
{
    DEFAULT = 0,
    PASSWORD = 1 << 0,
    READ_ONLY = 1 << 1,
    MULTILINE = 1 << 2,
    READ_ONLY_MULTILINE = (1U << 1U) | (1U << 2U),
    TRANSFERABLE = 1 << 3,
    RICH_TEXT = 1 << 4,
    NO_WRAP = 1 << 5,
    ANSI = 1 << 6,
    UNDERLINE = 1 << 7,
    WORD_WRAP = 1U << 8U,
    CHAR_WRAP = 1U << 9U,
    NONE_WRAP = 0
};

enum class AspectRatio : std::uint8_t { IGNORE_RATIO, MAINTAIN };
enum class CheckState : std::uint8_t { UNCHECKED, CHECKED, INDETERMINATE };
enum class Orientation : std::uint8_t { HORIZONTAL, VERTICAL };
enum class Selection : std::uint8_t { SINGLE, MULTI };
enum class SortOrder : std::uint8_t { NONE, ASCENDING, DESCENDING };
enum class TableColumnSizing : std::uint8_t { EQUAL, FIXED, PROPORTIONAL, ADAPTIVE };
enum class AnimationState : std::uint8_t { STOPPED, PLAYING };
enum class LabelVisibility : std::uint8_t { HIDDEN, VISIBLE };

enum class Position : std::uint8_t
{
    DEFAULT = 0,
    V_FIXED = 1 << 0,
    V_CENTER = 1 << 1,
    V_AUTO = 1 << 2,
    V_ABSOLUTE = 1 << 3,
    H_FIXED = 1 << 4,
    H_CENTER = 1 << 5,
    H_AUTO = 1 << 6,
    H_ABSOLUTE = 1 << 7,
    AUTO = V_AUTO | H_AUTO,
    CENTER = V_CENTER | H_CENTER,
    ABSOLUTE_POS = V_ABSOLUTE | H_ABSOLUTE,
    FIXED = V_FIXED | H_FIXED
};

enum class Scroll : std::uint8_t { NO_SCROLL, VERTICAL, HORIZONTAL, BOTH };
enum class ScrollBar : std::uint8_t { DEFAULT = 0, NO_VISIBILITY = 1 << 0, DRAGGABLE = 1 << 1, AUTO_HIDE = 1 << 2 };
enum class ScrollAnchor : std::uint8_t { TOP, BOTTOM, SMART };

enum class WindowFlag : std::uint8_t
{
    DEFAULT = 0,
    NO_TITLE_BAR = 1 << 0,
    NO_RESIZE = 1 << 1,
    NO_MOVE = 1 << 2,
    NO_COLLAPSE = 1 << 3,
    NO_BACKGROUND = 1 << 4,
    NO_CLOSE = 1 << 5,
    MODAL = 1 << 6,
    HAS_TOOLBAR = 1 << 7,
    FRAMELESS = NO_TITLE_BAR | NO_RESIZE | NO_MOVE,
    DIALOG = MODAL | NO_COLLAPSE
};

enum class IconFlag : std::uint8_t { DEFAULT = 0, TEXTURE = 1 << 0, HAS_TEXT = 1 << 1 };
enum class Log : std::uint8_t { SINGLE_FILE_R = 1 << 0, SINGLE_FILE_RW = 1 << 1, TERMINAL = 1 << 1 };

// 仅对实际 flags 提供运算符；状态、方向、模式等枚举不允许伪造按位组合。
template <typename T>
concept Flag = std::same_as<T, Alignment> || std::same_as<T, Size> || std::same_as<T, TextFlag>
           || std::same_as<T, Position> || std::same_as<T, WindowFlag> || std::same_as<T, ScrollBar>
           || std::same_as<T, IconFlag> || std::same_as<T, SystemManager> || std::same_as<T, Log>;

template <Flag T>
[[nodiscard]] constexpr T operator|(T lhs, T rhs) noexcept
{
    using U = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<U>(lhs) | static_cast<U>(rhs));
}

template <Flag T>
[[nodiscard]] constexpr T operator&(T lhs, T rhs) noexcept
{
    using U = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<U>(lhs) & static_cast<U>(rhs));
}

template <Flag T>
[[nodiscard]] constexpr T operator^(T lhs, T rhs) noexcept
{
    using U = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<U>(lhs) ^ static_cast<U>(rhs));
}

template <Flag T>
[[nodiscard]] constexpr T operator~(T value) noexcept
{
    using U = std::underlying_type_t<T>;
    return static_cast<T>(~static_cast<U>(value));
}

template <Flag T>
constexpr T& operator|=(T& lhs, T rhs) noexcept { return lhs = lhs | rhs; }
template <Flag T>
constexpr T& operator&=(T& lhs, T rhs) noexcept { return lhs = lhs & rhs; }
template <Flag T>
constexpr T& operator^=(T& lhs, T rhs) noexcept { return lhs = lhs ^ rhs; }

template <Flag T>
[[nodiscard]] constexpr bool HasFlag(T value, T flag) noexcept { return (value & flag) == flag; }

} // namespace ui::policies
