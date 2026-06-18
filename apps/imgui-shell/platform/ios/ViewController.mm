#import "ViewController.h"

#include "App.h"
#include "RenderContext.h"

#include <imgui.h>
#include <imgui_impl_metal.h>

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

@interface ViewController () {
    id<MTLDevice>       _device;
    id<MTLCommandQueue> _queue;
    MTKView*            _mtkView;
    app::RenderContext  _ctx;
    BOOL                _appInitialized;
}
@end

// Captured during viewDidLoad so the font-atlas rebuild callback can find
// the Metal device. See specs/font-rendering "Platform supplies the rebuild
// callback during init".
static id<MTLDevice> g_metalDevice = nil;

static void rebuildFontAtlasIos() {
    // Metal's reference-counted texture lifecycle keeps the previous texture
    // alive until in-flight command buffers complete; no explicit GPU wait
    // is needed before destroy.
    ImGui_ImplMetal_DestroyFontsTexture();
    ImGui::GetIO().Fonts->Clear();
    app::configureFontAtlas();
    ImGui_ImplMetal_CreateFontsTexture(g_metalDevice);
}

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    _device = MTLCreateSystemDefaultDevice();
    NSAssert(_device, @"Metal is not supported on this device.");
    _queue = [_device newCommandQueue];

    _mtkView = [[MTKView alloc] initWithFrame:self.view.bounds device:_device];
    _mtkView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    _mtkView.clearColor       = MTLClearColorMake(0.10, 0.10, 0.12, 1.0);
    _mtkView.delegate         = self;
    _mtkView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [self.view addSubview:_mtkView];

    // ImGui Metal backend init — must be after MTLDevice exists.
    ImGui_ImplMetal_Init(_device);

    _ctx.device       = (__bridge void*)_device;
    _ctx.commandQueue = (__bridge void*)_queue;
    _ctx.metalLayer   = (__bridge void*)_mtkView.layer;

    // Tell the shared core where bundled assets (default font, etc.) live
    // before init reaches the font-loading code.
    app::setBundleResourcePath([[NSBundle mainBundle].resourcePath UTF8String]);

    app::init(_ctx);
    _appInitialized = YES;

    // Wire the font-atlas rebuild callback so Preferences typography edits
    // take effect live (specs/font-rendering "Font atlas can be rebuilt at runtime").
    g_metalDevice = _device;
    app::registerRebuildFontAtlasCallback(&rebuildFontAtlasIos);
}

- (void)dealloc {
    if (_appInitialized) {
        // Match the reverse-order teardown contract from app-shell spec.
        app::shutdown();
        ImGui_ImplMetal_Shutdown();
        _appInitialized = NO;
    }
}

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    (void)view; (void)size;
    // No swapchain to recreate on iOS — MTKView handles it. ImGui picks up the
    // new framebuffer size automatically through DisplaySize on next frame.
}

- (void)drawInMTKView:(MTKView*)view {
    if (!_appInitialized) return;

    id<MTLCommandBuffer> cmd = [_queue commandBuffer];
    MTLRenderPassDescriptor* rpd = view.currentRenderPassDescriptor;
    if (!rpd || !view.currentDrawable) {
        [cmd commit];
        return;
    }

    // Update ImGui display size (MTKView gives us a CGSize per frame).
    ImGuiIO& io   = ImGui::GetIO();
    io.DisplaySize = ImVec2(view.bounds.size.width, view.bounds.size.height);
    io.DisplayFramebufferScale = ImVec2(view.contentScaleFactor, view.contentScaleFactor);
    io.DeltaTime = 1.0f / 60.0f; // simple, no high-res timer in v1

    ImGui_ImplMetal_NewFrame(rpd);
    app::frame(_ctx);

    id<MTLRenderCommandEncoder> enc = [cmd renderCommandEncoderWithDescriptor:rpd];
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), cmd, enc);
    [enc endEncoding];

    [cmd presentDrawable:view.currentDrawable];
    [cmd commit];
}

@end
