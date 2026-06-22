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

# Platform-specific file dialog implementations
if(WIN32)
    target_sources(imgui_shell_desktop PRIVATE
        file_dialog/WindowsFileDialog.cpp
        file_dialog/WindowsFileDialog.h
    )
elseif(APPLE)
    target_sources(imgui_shell_desktop PRIVATE
        file_dialog/MacOSFileDialog.mm
        file_dialog/MacOSFileDialog.h
    )
else() # Linux
    target_sources(imgui_shell_desktop PRIVATE
        file_dialog/LinuxFileDialog.cpp
        file_dialog/LinuxFileDialog.h
    )
endif()

# Add Windows resource file if building for Windows
if(WIN32)
    target_sources(imgui_shell_desktop PRIVATE resources.rc resource.h)
endif()

target_link_libraries(imgui_shell_desktop PRIVATE
    imgui_shell_app
    imgui_backend_desktop
    imgui
    glfw
    Vulkan::Vulkan
)

# Windows-specific libraries
if(WIN32)
    target_link_libraries(imgui_shell_desktop PRIVATE comdlg32 ole32 shell32)
endif()

# macOS-specific frameworks
if(APPLE)
    target_link_libraries(imgui_shell_desktop PRIVATE 
        "-framework Foundation"
        "-framework AppKit"
        "-framework Cocoa")
endif()

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

# Copy executable and assets to root bin/ folder for easy access
# Using add_custom_target instead of POST_BUILD for reliable execution
# Always copy without timestamp checking for guaranteed up-to-date binaries
# Path calculation:
# From apps/imgui-shell/platform/desktop:
# - ../../../../ goes to project root (E:/Projects/imgui_playground)
# - ../../assets goes to apps/imgui-shell/assets
# Uses cmake -E copy on all platforms (works reliably on Windows, cross-platform on Unix)
add_custom_target(copy_to_bin ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../bin
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:imgui_shell_desktop>
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../bin/
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/../../assets
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../bin/assets
    COMMENT "Copying executable and assets to root bin/ folder"
    VERBATIM
    DEPENDS imgui_shell_desktop
)




