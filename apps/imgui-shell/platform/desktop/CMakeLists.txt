# Desktop host — see specs/render-backend/spec.md (desktop portion).

add_executable(imgui_shell_desktop
    main.cpp
    vk_check.h
    vk_instance.cpp
    vk_instance.h
    vk_device.cpp
    vk_device.h
    vk_swapchain.cpp
    vk_swapchain.h
    vk_frame.cpp
    vk_frame.h
)

target_link_libraries(imgui_shell_desktop PRIVATE
    imgui_shell_app
    imgui_backend_desktop
    imgui
    glfw
    Vulkan::Vulkan
)

target_compile_options(imgui_shell_desktop PRIVATE ${IMGUI_SHELL_WARNINGS})
target_compile_features(imgui_shell_desktop PRIVATE cxx_std_17)

# Windows subsystem configuration: Release builds hide console window, Debug builds show console
if(WIN32)
    # Use generator expression to conditionally set WIN32_EXECUTABLE based on configuration
    set_target_properties(imgui_shell_desktop PROPERTIES
        WIN32_EXECUTABLE "$<CONFIG:Release>"
    )
    # When using Windows subsystem with main() entry point, need to specify entry point
    # Only apply /ENTRY:mainCRTStartup for Release configuration
    target_link_options(imgui_shell_desktop PRIVATE
        "$<$<CONFIG:Release>:/ENTRY:mainCRTStartup>"
    )
    message(STATUS "imgui-shell: Windows Release builds hide console window (/SUBSYSTEM:WINDOWS), Debug shows console")
endif()

# Bundle font assets next to the desktop binary so the relative-path lookup in
# app::init resolves at runtime. See specs/build-system delta + font-rendering.
add_custom_command(TARGET imgui_shell_desktop POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:imgui_shell_desktop>/assets
    COMMENT "Copying assets/ next to imgui_shell_desktop binary"
    VERBATIM
)
