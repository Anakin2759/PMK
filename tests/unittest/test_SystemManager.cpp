#include <gtest/gtest.h>

#include <vector>

#include "src/core/SystemManager.hpp"
#include "src/core/UiRuntime.hpp"
#include "src/core/UiRuntimeScope.hpp"
#include "src/interface/ISystem.hpp"

namespace ui::tests
{
namespace
{

struct SystemManagerTestEvent
{
    using is_event_tag = void;
};

struct TestSystemCounters
{
    int registered = 0;
    int unregistered = 0;
    int events = 0;
};

class TestSystem : public interface::EnableRegister<TestSystem>
{
   public:
    TestSystem(Dispatcher& dispatcher, interface::SystemPhase phase,
               std::vector<interface::SystemPhase>& registrationOrder, TestSystemCounters& counters)
        : m_dispatcher(&dispatcher), m_phase(phase), m_registrationOrder(&registrationOrder), m_counters(&counters)
    {
    }

    void registerHandlersImpl()
    {
        ++m_counters->registered;
        m_registrationOrder->push_back(m_phase);
        m_dispatcher->sink<SystemManagerTestEvent>().connect<&TestSystem::onEvent>(*this);
    }

    void unregisterHandlersImpl()
    {
        ++m_counters->unregistered;
        m_dispatcher->sink<SystemManagerTestEvent>().disconnect<&TestSystem::onEvent>(*this);
    }

    void onEvent([[maybe_unused]] const SystemManagerTestEvent& event) const
    {
        ++m_counters->events;
    }
    [[nodiscard]] interface::SystemPhase getPhase() const noexcept
    {
        return m_phase;
    }

   private:
    Dispatcher* m_dispatcher = nullptr;
    interface::SystemPhase m_phase = interface::SystemPhase::LOGIC;
    std::vector<interface::SystemPhase>* m_registrationOrder = nullptr;
    TestSystemCounters* m_counters = nullptr;
};

TestSystem MakeSystem(UiRuntime& runtime, interface::SystemPhase phase,
                      std::vector<interface::SystemPhase>& registrationOrder, TestSystemCounters& counters)
{
    return TestSystem(runtime.dispatcher(), phase, registrationOrder, counters);
}

TEST(SystemManagerTest, RegistersByPhaseAndLifecycleOperationsAreIdempotent)
{
    UiRuntime runtime;
    SystemManager manager(&runtime, false);
    std::vector<interface::SystemPhase> order;
    TestSystemCounters counters;

    ASSERT_TRUE(manager.addSystemBeforeRegister(MakeSystem(runtime, interface::SystemPhase::RENDER, order, counters)));
    ASSERT_TRUE(manager.addSystemBeforeRegister(MakeSystem(runtime, interface::SystemPhase::INPUT, order, counters)));
    ASSERT_TRUE(manager.addSystemBeforeRegister(MakeSystem(runtime, interface::SystemPhase::LAYOUT, order, counters)));

    manager.registerAllHandlers();
    manager.registerAllHandlers();

    EXPECT_EQ(manager.getState(), SystemManager::State::REGISTERED);
    EXPECT_EQ(counters.registered, 3);
    EXPECT_EQ(order, (std::vector{interface::SystemPhase::INPUT, interface::SystemPhase::LAYOUT,
                                  interface::SystemPhase::RENDER}));

    manager.unregisterAllHandlers();
    manager.unregisterAllHandlers();

    EXPECT_EQ(manager.getState(), SystemManager::State::STOPPED);
    EXPECT_EQ(counters.unregistered, 3);
}

TEST(SystemManagerTest, RemovingRegisteredSystemDisconnectsBeforeDestruction)
{
    UiRuntime runtime;
    SystemManager manager(&runtime, false);
    std::vector<interface::SystemPhase> order;
    TestSystemCounters counters;

    ASSERT_TRUE(manager.addSystemBeforeRegister(MakeSystem(runtime, interface::SystemPhase::LOGIC, order, counters)));
    manager.registerAllHandlers();
    runtime.dispatcher().trigger(SystemManagerTestEvent{});
    ASSERT_EQ(counters.events, 1);

    EXPECT_TRUE(manager.removeSystem(0));
    EXPECT_EQ(counters.unregistered, 1);
    EXPECT_EQ(manager.getSystemCount(), 0U);

    runtime.dispatcher().trigger(SystemManagerTestEvent{});
    EXPECT_EQ(counters.events, 1);
    EXPECT_FALSE(manager.removeSystem(0));
}

TEST(SystemManagerTest, RejectsSystemAdditionAfterRegistrationAndAfterStop)
{
    UiRuntime runtime;
    SystemManager manager(&runtime, false);
    std::vector<interface::SystemPhase> order;
    TestSystemCounters counters;

    manager.registerAllHandlers();
    EXPECT_FALSE(manager.addSystem(MakeSystem(runtime, interface::SystemPhase::LOGIC, order, counters)));

    manager.unregisterAllHandlers();
    EXPECT_FALSE(manager.addSystemBeforeRegister(MakeSystem(runtime, interface::SystemPhase::LOGIC, order, counters)));
    EXPECT_EQ(manager.getSystemCount(), 0U);
}

TEST(SystemManagerTest, BuiltInSystemsExposeExpectedPhaseContract)
{
    UiRuntime runtime;
    UiRuntimeScope const scope(runtime);
    SystemManager manager(&runtime);

    EXPECT_EQ(manager.getSystemPhases(),
              (std::vector{interface::SystemPhase::INPUT,      // InteractionSystem
                           interface::SystemPhase::INPUT,      // TextInputSystem
                           interface::SystemPhase::LOGIC,      // HitTestSystem
                           interface::SystemPhase::ANIMATION,  // TweenSystem
                           interface::SystemPhase::LAYOUT,     // LayoutSystem
                           interface::SystemPhase::RENDER,     // RenderSystem
                           interface::SystemPhase::LOGIC,      // StateSystem
                           interface::SystemPhase::LOGIC,      // ActionSystem
                           interface::SystemPhase::FRAME,      // TimerSystem
                           interface::SystemPhase::LOGIC,      // ThemeSystem
                           interface::SystemPhase::LOGIC,      // ShortcutSystem
                           interface::SystemPhase::LOGIC,      // FocusNavigationSystem
                           interface::SystemPhase::LOGIC}));   // OverlaySystem
}

}  // namespace
}  // namespace ui::tests