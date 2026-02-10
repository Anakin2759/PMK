#  ui 模块 CMake 配置文件
cmake_minimum_required(VERSION 3.20)
OPTION(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
OPTION(BUILD_STATIC_LIBS "Build using static libraries" ON)




set(UI_SOURCES 
    core/Application.cpp
    core/EventLoop.cpp
    core/SystemManager.cpp
    systems/RenderSystem.cpp
    systems/TimerSystem.cpp
    managers/FontManager.cpp
    managers/IconManager.cpp
    api/Size.cpp
    api/Visibility.cpp
    api/Layout.cpp
    api/Text.cpp
    api/Animation.cpp
    api/Hierarchy.cpp
    api/Icon.cpp
    api/Utils.cpp
    api/Factory.cpp
)
set(UI_HEADERS
    # Components
    common/Components.hpp
    common/Policies.hpp
    common/Tags.hpp
    common/Events.hpp
    common/Types.hpp
    common/RenderTypes.hpp
    
    # Systems
    systems/InteractionSystem.hpp
    systems/TweenSystem.hpp
    systems/LayoutSystem.hpp
    systems/RenderSystem.hpp
    systems/StateSystem.hpp
    systems/HitTestSystem.hpp
    systems/ActionSystem.hpp
    systems/TimerSystem.hpp
    
    # Managers
    managers/DeviceManager.hpp
    managers/FontManager.hpp
    managers/PipelineCache.hpp
    managers/TextTextureCache.hpp
    managers/IconManager.hpp
    managers/BatchManager.hpp
    managers/CommandBuffer.hpp
    
    # Core
    core/Application.hpp
    core/SystemManager.hpp
    core/EventLoop.hpp
    core/TaskChain.hpp
    core/TextUtils.hpp
    interface/IRenderer.hpp
    core/RenderContext.hpp
    renderers/ShapeRenderer.hpp
    renderers/TextRenderer.hpp
    renderers/IconRenderer.hpp
    renderers/ScrollBarRenderer.hpp


    singleton/Registry.hpp
    singleton/SingletonBase.hpp
    singleton/Logger.hpp
    singleton/Dispatcher.hpp
    
    traits/ComponentsTraits.hpp
    traits/PoliciesTraits.hpp

    # Interface
    interface/Isystem.hpp
    
    # API
    api/Utils.hpp
    api/Layout.hpp
    
    ui/ui.hpp
)
# UI 模块库定义
add_library(ui STATIC ${UI_SOURCES} ${UI_HEADERS})

# 头文件目录
target_include_directories(ui
    PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party/stb
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

cmrc_add_resource_library(ui_fonts
    ALIAS fonts              # 在 C++ 中调用的别名
    NAMESPACE ui_fonts          # C++ 命名空间
    assets/fonts/NotoSansSC-VariableFont_wght.ttf
    assets/shader/vert.spv
    assets/shader/frag.spv
    assets/shader/vert.dxil
    assets/shader/frag.dxil

)

# cmrc_add_resource_library(ui_swiftshader
#     ALIAS swiftshader             # 在 C++ 中调用的别名
#     NAMESPACE ui_swiftshader          # C++ 命名空间
#     assets/Windows/vk_swiftshader.dll
#     assets/Windows/vk_swiftshader_icd.json
# )

cmrc_add_resource_library(ui_icons
    ALIAS icons              # 在 C++ 中调用的别名
    NAMESPACE ui_icons          # C++ 命名空间
    assets/icons/MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf
    assets/icons/MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].codepoints
    assets/icons/MaterialSymbolsRounded[FILL,GRAD,opsz,wght].ttf
    assets/icons/MaterialSymbolsRounded[FILL,GRAD,opsz,wght].codepoints
    assets/icons/MaterialSymbolsSharp[FILL,GRAD,opsz,wght].ttf
    assets/icons/MaterialSymbolsSharp[FILL,GRAD,opsz,wght].codepoints
    
)



# 依赖项
target_link_libraries(ui PRIVATE
    SDL3::SDL3
    EnTT::EnTT
    asio::asio
    yogacore
    eigen
    ui_fonts
    ui_icons
    spdlog::spdlog_header_only
)
target_compile_features(ui PUBLIC cxx_std_23)

target_compile_options(ui PUBLIC
     
    # GCC
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g >
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG >

    # Clang (non-MSVC frontend)
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g >
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG >

    # MSVC
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8  /EHsc>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/W4 /Od /Zi>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/O2 /DNDEBUG>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:MinSizeRel>>:/O2 /DNDEBUG>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RelWithDebInfo>>:/O2 /DNDEBUG /W4 /Zi>

    # Clang-cl (MSVC frontend)
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Debug>>:/EHsc /Zi /W4>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Release>>:/EHsc /O2 /DNDEBUG >

)



