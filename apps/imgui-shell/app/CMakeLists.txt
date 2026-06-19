# Shared application core — see specs/app-shell/spec.md.
# Compiles identically across all four platform targets.

add_library(imgui_shell_app STATIC
    App.cpp
    App.h
    Platform.h
    Preferences.cpp
    Preferences.h
    RenderContext.h
    Theme.h
    Theme.cpp
    ThemeStorage.h
    ThemeStorage.cpp
    
    # Node graph widget files
    VulkanTexture.cpp
    VulkanTexture.h
    GridTextureCache.cpp
    GridTextureCache.h
    ViewTransform.cpp
    ViewTransform.h
    GridRenderer.cpp
    GridRenderer.h
    SimpleGridRenderer.cpp
    SimpleGridRenderer.h
    NodeGraphWidget.cpp
    NodeGraphWidget.h
)

target_include_directories(imgui_shell_app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(imgui_shell_app PUBLIC imgui nlohmann_json::nlohmann_json)

if(IMGUI_SHELL_DESKTOP)
    target_include_directories(imgui_shell_app PRIVATE
        ${Vulkan_INCLUDE_DIR}
    )
endif()

target_compile_features(imgui_shell_app PUBLIC cxx_std_17)
target_compile_options(imgui_shell_app PRIVATE ${IMGUI_SHELL_WARNINGS})
set_target_properties(imgui_shell_app PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Platform identification — see specs/ui-sample/spec.md (About dialog).
if(IMGUI_SHELL_DESKTOP)
    target_compile_definitions(imgui_shell_app PUBLIC IMGUI_SHELL_PLATFORM_DESKTOP=1)
elseif(IMGUI_SHELL_MOBILE)
    target_compile_definitions(imgui_shell_app PUBLIC IMGUI_SHELL_PLATFORM_IOS=1)
endif()

target_compile_definitions(imgui_shell_app PRIVATE
    IMGUI_SHELL_PLATFORM_NAME="${IMGUI_SHELL_PLATFORM}"
    IMGUI_SHELL_THEME_NAME="imgui-shell dark"
)

if(IMGUI_SHELL_USE_FREETYPE)
    target_compile_definitions(imgui_shell_app PRIVATE
        IMGUI_SHELL_FONT_BACKEND="FreeType"
    )
else()
    target_compile_definitions(imgui_shell_app PRIVATE
        IMGUI_SHELL_FONT_BACKEND="stb_truetype"
    )
endif()

# Asset-path policy:
#   Desktop: assets are copied next to the executable; relative "assets" works.
#   iOS:     assets live inside the .app bundle's Resources/; the iOS host
#            supplies the absolute path at runtime via app::setBundleResourcePath().
if(IMGUI_SHELL_DESKTOP)
    target_compile_definitions(imgui_shell_app PRIVATE
        IMGUI_SHELL_ASSETS_DIR="assets"
    )
endif()
