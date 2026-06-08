#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "common/EntityTypes.hpp"

namespace ui::event
{
using EventId = std::uint32_t;
inline constexpr EventId INVALID_EVENT_ID = 0;

struct EventPayload
{
    ui::entity source = ui::null_entity;
    ui::entity target = ui::null_entity;
    std::string name;
    std::string text;
    std::int64_t intValue = 0;
    double floatValue = 0.0;
};

using EventCallback = std::move_only_function<void(const EventPayload&)>;
} // namespace ui::event