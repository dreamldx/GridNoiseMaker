include(FetchContent)

# -----------------------------------------------------------------------------
# Dear ImGui (docking branch).
#
# Per design.md D1/D2: the docking branch, pinned by SHA, fetched at configure
# time. To update the pin: `git ls-remote https://github.com/ocornut/imgui docking`
# for the newest SHA, then replace IMGUI_GIT_TAG below.
# -----------------------------------------------------------------------------
set(IMGUI_GIT_TAG "v1.91.9-docking" CACHE STRING
    "Dear ImGui git ref to fetch (SHA preferred; tag acceptable as starting point)")

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        ${IMGUI_GIT_TAG}
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

# -----------------------------------------------------------------------------
# nlohmann/json (header-only, MIT) — see openspec change `add-theme-persistence`.
# Used by app/ThemeStorage for JSON read/write of persisted ImGuiStyle values.
# -----------------------------------------------------------------------------
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install    OFF CACHE BOOL "" FORCE)
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# ImGui does not ship a CMakeLists.txt — define our own static-library target.
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
)

# -----------------------------------------------------------------------------
# FreeType (optional, default ON) — see openspec change `add-font-antialiasing`.
# When enabled, FreeType is fetched and `imgui_freetype.cpp` is compiled into
# the `imgui` static library so its consumers transparently get sharper text.
# -----------------------------------------------------------------------------
if(IMGUI_SHELL_USE_FREETYPE)
    # Disable FreeType's optional codepaths we don't use — keeps the binary lean.
    set(FT_DISABLE_ZLIB     ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BZIP2    ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_PNG      ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BROTLI   ON CACHE BOOL "" FORCE)
    set(SKIP_INSTALL_ALL    ON CACHE BOOL "" FORCE)

    FetchContent_Declare(freetype
        GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
        GIT_TAG        VER-2-13-3
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(freetype)

    list(APPEND IMGUI_SOURCES ${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp)
endif()

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
target_compile_features(imgui PUBLIC cxx_std_17)
set_target_properties(imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(IMGUI_SHELL_USE_FREETYPE)
    target_link_libraries(imgui PUBLIC freetype)
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR}/misc/freetype)
    target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_FREETYPE)
    message(STATUS "imgui-shell: font rasterizer = FreeType (imgui_freetype.cpp linked)")
else()
    message(STATUS "imgui-shell: font rasterizer = stb_truetype (FreeType disabled by IMGUI_SHELL_USE_FREETYPE=OFF)")
endif()

# -----------------------------------------------------------------------------
# Desktop-only deps: GLFW + Vulkan.
# -----------------------------------------------------------------------------
if(IMGUI_SHELL_DESKTOP)
    # GLFW via FetchContent — gives us a reproducible build on every desktop OS.
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(glfw)

    # Vulkan loader + headers from the LunarG SDK (or system package on Linux).
    find_package(Vulkan 1.2 QUIET)
    if(NOT Vulkan_FOUND)
        message(FATAL_ERROR
            "LunarG Vulkan SDK >= 1.2 not found.\n"
            "Install from https://vulkan.lunarg.com/ and ensure either\n"
            "  - VULKAN_SDK is exported (usually by sourcing setup-env.sh from the SDK), or\n"
            "  - the loader+headers are installed system-wide (Linux: 'apt install vulkan-sdk').\n")
    endif()
    message(STATUS "imgui-shell: Vulkan ${Vulkan_VERSION} found at ${Vulkan_LIBRARY}")

    # ImGui backends used by the desktop host: glfw + vulkan.
    add_library(imgui_backend_desktop STATIC
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    )
    target_include_directories(imgui_backend_desktop PUBLIC
        ${imgui_SOURCE_DIR}/backends
    )
    target_link_libraries(imgui_backend_desktop PUBLIC
        imgui
        glfw
        Vulkan::Vulkan
    )
    set_target_properties(imgui_backend_desktop PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# -----------------------------------------------------------------------------
# iOS-only deps: ImGui Metal backend + system frameworks.
# -----------------------------------------------------------------------------
if(IMGUI_SHELL_MOBILE)
    add_library(imgui_backend_ios STATIC
        ${imgui_SOURCE_DIR}/backends/imgui_impl_metal.mm
    )
    target_include_directories(imgui_backend_ios PUBLIC
        ${imgui_SOURCE_DIR}/backends
    )
    target_link_libraries(imgui_backend_ios PUBLIC imgui)
    target_compile_options(imgui_backend_ios PRIVATE -fobjc-arc)
    set_target_properties(imgui_backend_ios PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
