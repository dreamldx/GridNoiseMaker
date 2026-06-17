#pragma once

// Compile-time platform traits. The #ifdef lives here, in a header, so that
// `app/` source files can stay platform-uniform per app-shell spec
// "Identical source set across platforms".

namespace app {

#if defined(IMGUI_SHELL_PLATFORM_DESKTOP)
    constexpr bool kIsDesktop = true;
    constexpr bool kIsMobile  = false;
    #if defined(_WIN32)
        constexpr bool kIsWindows = true;
        constexpr bool kIsMacOS   = false;
        constexpr bool kIsLinux   = false;
    #elif defined(__APPLE__)
        constexpr bool kIsWindows = false;
        constexpr bool kIsMacOS   = true;
        constexpr bool kIsLinux   = false;
    #else
        constexpr bool kIsWindows = false;
        constexpr bool kIsMacOS   = false;
        constexpr bool kIsLinux   = true;
    #endif
#elif defined(IMGUI_SHELL_PLATFORM_IOS)
    constexpr bool kIsDesktop = false;
    constexpr bool kIsMobile  = true;
    constexpr bool kIsWindows = false;
    constexpr bool kIsMacOS   = false;
    constexpr bool kIsLinux   = false;
#else
    #error "Exactly one of IMGUI_SHELL_PLATFORM_DESKTOP / IMGUI_SHELL_PLATFORM_IOS must be defined."
#endif

} // namespace app
