## Context

The imgui-shell application currently implements a Preferences dialog using ImGui's multi-viewport system. On desktop platforms, this creates a separate OS window that can be moved and resized independently. The dialog has three tabs (General, Theme, About) and provides runtime theme editing with live preview, Save/Reset/Discard buttons, and diagnostic information.

Current implementation limitations:
1. Appearance doesn't match native OS dialog styling
2. Missing standard native dialog behaviors (system modal state, native buttons)
3. Platform inconsistencies between Windows, macOS, Linux
4. Relies on ImGui multi-viewport which may have performance implications

## Goals / Non-Goals

**Goals:**
1. Replace ImGui-based Preferences dialog with native OS dialog APIs
2. Maintain all existing functionality (three tabs, theme editing, persistence)
3. Provide consistent platform-native appearance and behavior
4. Support all target platforms: Windows, macOS, Linux, iOS
5. Preserve backward compatibility for theme persistence

**Non-Goals:**
1. Rewriting the entire UI system - only the Preferences dialog changes
2. Changing theme persistence format or location
3. Modifying other parts of the application that use ImGui windows
4. Supporting custom dialog themes beyond native OS styling

## Decisions

**1. Platform Abstraction Layer**
- Create a new `NativeDialog` class that provides a platform-agnostic interface
- Implement platform-specific backends: `Win32NativeDialog`, `CocoaNativeDialog`, `GtkNativeDialog`, `UIKitNativeDialog`
- Maintain existing `Preferences` class as facade that uses `NativeDialog`

**2. Dialog Content Rendering**
- Use ImGui to render dialog content internally (for theme editing widgets)
- Host ImGui within native dialog container (HWND/NSWindow/GtkWindow/UIView)
- Create platform-specific "ImGui host window" integration

**3. Multi-Viewport Compatibility**
- Keep ImGui multi-viewport enabled for other potential uses
- Preferences dialog will bypass multi-viewport and use native dialog directly
- Update `render-backend` spec to clarify multi-viewport is optional for dialogs

**4. Theme Live Preview Integration**
- Native dialog must still update `ImGui::GetStyle()` for live preview
- Implement callback system between native dialog and main ImGui context
- Ensure theme changes apply immediately to main application window

**5. Dialog Persistence**
- Replace ImGui ini-based persistence with platform-native mechanisms
- Windows: Registry or ini file with dialog position/size
- macOS: NSUserDefaults
- Linux: GSettings or config file
- iOS: UserDefaults (limited utility for mobile dialogs)

**Alternative Considered: Hybrid Approach**
- Keep ImGui window but apply native styling via platform APIs
- **Rejected**: Doesn't provide true native dialog behaviors (modal state, system integration)

## Risks / Trade-offs

**Risk 1: Platform API Complexity** → Each platform requires different implementation approach
- **Mitigation**: Abstract common patterns, implement incrementally (Windows first)

**Risk 2: ImGui Integration Challenges** → Hosting ImGui within native dialog may have rendering issues
- **Mitigation**: Test with simple ImGui window first, then complex Preferences UI

**Risk 3: iOS Platform Limitations** → iOS has different dialog paradigms (UIAlertController vs windows)
-C **Mitigation**: Research iOS best practices, possibly keep inline panel for iOS

**Risk 4: Theme Preview Synchronization** → Live preview requires communication between dialog and main window
- **Mitigation**: Implement event/callback system, test with theme changes

**Trade-off: Development Time vs User Experience**
. **Option A**: Quick implementation using existing ImGui approach (current)
. **Option B**: Native dialog with better UX but longer development
. **Chosen**: Option B - aligns with goal of improving platform consistency

## Migration Plan

1. **Phase 1**: Create `NativeDialog` abstraction with Windows implementation
2. **Phase 2**: Update `Preferences` class to use `NativeDialog` on Windows
3. **Phase 3**: Test Windows implementation, fix integration issues
4. **Phase 4**: Implement macOS, Linux, iOS backends
5. **Phase 5**: Update build system and documentation
6. **Phase 6**: Remove old ImGui multi-viewport code for Preferences dialog

**Rollback Strategy**: Keep old `Preferences` implementation as fallback, use compile-time flag to switch between implementations during testing.

## Open Questions

1. Should the dialog be truly modal (blocking main window) or modeless?
2. How to handle dialog tab switching in native controls (native tab control vs ImGui tab bar)?
3. Best approach for hosting ImGui rendering within native window on each platform?
4. Should we support both native and ImGui dialog modes via configuration flag?