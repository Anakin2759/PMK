#include <ui/api/Factory.hpp>

#ifdef _WIN32
#include <Windows.h>

#ifdef CreateWindow
#error "ui/api/Factory.hpp must shield the Windows CreateWindow macro"
#endif
#ifdef CreateDialog
#error "ui/api/Factory.hpp must shield the Windows CreateDialog macro"
#endif

static_assert(&CreateWindowExW != nullptr);
static_assert(&CreateDialogParamW != nullptr);
#endif

void CompileFactoryCallsAfterWindowsHeaders(ui::UiRuntime& runtime)
{
    static_cast<void>(ui::factory::CreateWindow(runtime, "window", "alias"));
    static_cast<void>(ui::factory::CreateDialog("dialog", "alias"));
}