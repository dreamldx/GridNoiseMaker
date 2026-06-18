# iOS host — see specs/render-backend/spec.md (iOS portion).

set(IMGUI_SHELL_IOS_BUNDLE_ID     "io.github.imgui-shell.example" CACHE STRING "iOS bundle identifier")
set(IMGUI_SHELL_IOS_DISPLAY_NAME  "imgui-shell"                   CACHE STRING "iOS app display name")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
               ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
               @ONLY)

# Bundled font assets — copied into the .app bundle's Resources/fonts/ so the
# iOS host can resolve them at runtime via NSBundle.mainBundle.resourcePath.
set(IMGUI_SHELL_IOS_FONTS
    ${CMAKE_SOURCE_DIR}/assets/fonts/Inter-Regular.ttf
    ${CMAKE_SOURCE_DIR}/assets/fonts/Inter-OFL.txt
    ${CMAKE_SOURCE_DIR}/assets/fonts/JetBrainsMono-Regular.ttf
    ${CMAKE_SOURCE_DIR}/assets/fonts/JetBrainsMono-OFL.txt
    ${CMAKE_SOURCE_DIR}/assets/fonts/Lato-Regular.ttf
    ${CMAKE_SOURCE_DIR}/assets/fonts/Lato-OFL.txt
)
set_source_files_properties(${IMGUI_SHELL_IOS_FONTS} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources/fonts"
)

# Bundled theme catalog (read-only) — see specs/theme-presets.
set(IMGUI_SHELL_IOS_THEMES
    ${CMAKE_SOURCE_DIR}/assets/themes/dark.json
    ${CMAKE_SOURCE_DIR}/assets/themes/light.json
)
set_source_files_properties(${IMGUI_SHELL_IOS_THEMES} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources/themes"
)

add_executable(imgui_shell_ios MACOSX_BUNDLE
    main.mm
    AppDelegate.h
    AppDelegate.mm
    ViewController.h
    ViewController.mm
    ${IMGUI_SHELL_IOS_FONTS}
    ${IMGUI_SHELL_IOS_THEMES}
)

target_link_libraries(imgui_shell_ios PRIVATE
    imgui_shell_app
    imgui_backend_ios
    imgui
    "-framework UIKit"
    "-framework Metal"
    "-framework MetalKit"
    "-framework QuartzCore"
    "-framework Foundation"
)

target_compile_options(imgui_shell_ios PRIVATE
    -fobjc-arc
    ${IMGUI_SHELL_WARNINGS}
)
target_compile_features(imgui_shell_ios PRIVATE cxx_std_17)

set_target_properties(imgui_shell_ios PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST            "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${IMGUI_SHELL_IOS_BUNDLE_ID}"
    XCODE_ATTRIBUTE_CODE_SIGN_STYLE     "Automatic"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
)
