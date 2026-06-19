# Desktop host — see specs/render-backend/spec.md (desktop portion).

# Add icon to executable sources
if(WIN32)
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
        resources.rc
    )
else()
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
endif()

target_link_libraries(imgui_shell_desktop PRIVATE
    imgui_shell_app
    imgui_backend_desktop
    imgui
    glfw
    Vulkan::Vulkan
)

target_compile_options(imgui_shell_desktop PRIVATE ${IMGUI_SHELL_WARNINGS})
target_compile_features(imgui_shell_desktop PRIVATE cxx_std_17)

# Bundle font assets next to the desktop binary so the relative-path lookup in
# app::init resolves at runtime. See specs/build-system delta + font-rendering.
add_custom_command(TARGET imgui_shell_desktop POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:imgui_shell_desktop>/assets
    COMMENT "Copying assets/ next to imgui_shell_desktop binary"
    VERBATIM
)


