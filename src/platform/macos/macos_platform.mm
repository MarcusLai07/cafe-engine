#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#import <mach/mach_time.h>

#include "../platform.h"
#include <cstring>  // for memcpy

// ============================================================================
// Forward Declarations (C++ class that needs ObjC types)
// ============================================================================

namespace cafe {
    class MacWindow;
}

// ============================================================================
// Custom NSView for rendering (interface only - implementation after MacWindow)
// ============================================================================

@interface CafeView : NSView {
    cafe::MacWindow* _engineWindow;
}
@property (nonatomic, strong) CAMetalLayer* metalLayer;
- (void)setEngineWindow:(cafe::MacWindow*)window;
@end

// ============================================================================
// Window Delegate (handles window events)
// ============================================================================

@interface CafeWindowDelegate : NSObject <NSWindowDelegate> {
    cafe::MacWindow* _engineWindow;
}
- (void)setEngineWindow:(cafe::MacWindow*)window;
@end

// ============================================================================
// Application Delegate (handles app-level events like Cmd+Q)
// ============================================================================

@interface CafeAppDelegate : NSObject <NSApplicationDelegate> {
    cafe::MacWindow* _mainWindow;
}
- (void)setMainWindow:(cafe::MacWindow*)window;
@end

// Global app delegate (declared here, used by MacWindow)
static CafeAppDelegate* g_app_delegate = nil;

// ============================================================================
// Key Code Translation (macOS keyCode -> cafe::Key)
// ============================================================================

static cafe::Key translate_key_code(unsigned short keyCode) {
    // macOS virtual key codes
    // Reference: Carbon/HIToolbox/Events.h
    switch (keyCode) {
        // Letters
        case 0x00: return cafe::Key::A;
        case 0x0B: return cafe::Key::B;
        case 0x08: return cafe::Key::C;
        case 0x02: return cafe::Key::D;
        case 0x0E: return cafe::Key::E;
        case 0x03: return cafe::Key::F;
        case 0x05: return cafe::Key::G;
        case 0x04: return cafe::Key::H;
        case 0x22: return cafe::Key::I;
        case 0x26: return cafe::Key::J;
        case 0x28: return cafe::Key::K;
        case 0x25: return cafe::Key::L;
        case 0x2E: return cafe::Key::M;
        case 0x2D: return cafe::Key::N;
        case 0x1F: return cafe::Key::O;
        case 0x23: return cafe::Key::P;
        case 0x0C: return cafe::Key::Q;
        case 0x0F: return cafe::Key::R;
        case 0x01: return cafe::Key::S;
        case 0x11: return cafe::Key::T;
        case 0x20: return cafe::Key::U;
        case 0x09: return cafe::Key::V;
        case 0x0D: return cafe::Key::W;
        case 0x07: return cafe::Key::X;
        case 0x10: return cafe::Key::Y;
        case 0x06: return cafe::Key::Z;

        // Numbers
        case 0x1D: return cafe::Key::Num0;
        case 0x12: return cafe::Key::Num1;
        case 0x13: return cafe::Key::Num2;
        case 0x14: return cafe::Key::Num3;
        case 0x15: return cafe::Key::Num4;
        case 0x17: return cafe::Key::Num5;
        case 0x16: return cafe::Key::Num6;
        case 0x1A: return cafe::Key::Num7;
        case 0x1C: return cafe::Key::Num8;
        case 0x19: return cafe::Key::Num9;

        // Function keys
        case 0x7A: return cafe::Key::F1;
        case 0x78: return cafe::Key::F2;
        case 0x63: return cafe::Key::F3;
        case 0x76: return cafe::Key::F4;
        case 0x60: return cafe::Key::F5;
        case 0x61: return cafe::Key::F6;
        case 0x62: return cafe::Key::F7;
        case 0x64: return cafe::Key::F8;
        case 0x65: return cafe::Key::F9;
        case 0x6D: return cafe::Key::F10;
        case 0x67: return cafe::Key::F11;
        case 0x6F: return cafe::Key::F12;

        // Arrow keys
        case 0x7B: return cafe::Key::Left;
        case 0x7C: return cafe::Key::Right;
        case 0x7E: return cafe::Key::Up;
        case 0x7D: return cafe::Key::Down;

        // Modifiers
        case 0x38: return cafe::Key::LeftShift;
        case 0x3C: return cafe::Key::RightShift;
        case 0x3B: return cafe::Key::LeftControl;
        case 0x3E: return cafe::Key::RightControl;
        case 0x3A: return cafe::Key::LeftAlt;
        case 0x3D: return cafe::Key::RightAlt;
        case 0x37: return cafe::Key::LeftSuper;   // Command
        case 0x36: return cafe::Key::RightSuper;

        // Special keys
        case 0x31: return cafe::Key::Space;
        case 0x24: return cafe::Key::Enter;
        case 0x30: return cafe::Key::Tab;
        case 0x33: return cafe::Key::Backspace;
        case 0x35: return cafe::Key::Escape;
        case 0x72: return cafe::Key::Insert;      // Help key on Mac
        case 0x75: return cafe::Key::Delete;      // Forward delete
        case 0x73: return cafe::Key::Home;
        case 0x77: return cafe::Key::End;
        case 0x74: return cafe::Key::PageUp;
        case 0x79: return cafe::Key::PageDown;

        // Punctuation
        case 0x2B: return cafe::Key::Comma;
        case 0x2F: return cafe::Key::Period;
        case 0x2C: return cafe::Key::Slash;
        case 0x29: return cafe::Key::Semicolon;
        case 0x27: return cafe::Key::Quote;
        case 0x21: return cafe::Key::LeftBracket;
        case 0x1E: return cafe::Key::RightBracket;
        case 0x2A: return cafe::Key::Backslash;
        case 0x32: return cafe::Key::Grave;
        case 0x1B: return cafe::Key::Minus;
        case 0x18: return cafe::Key::Equal;

        default: return cafe::Key::Unknown;
    }
}

// ============================================================================
// MacOS Window Implementation (C++)
// ============================================================================

namespace cafe {

class MacWindow : public Window {
private:
    NSWindow* window_ = nil;
    CafeView* view_ = nil;
    CafeWindowDelegate* delegate_ = nil;
    bool is_open_ = true;

    // Input state
    static constexpr int kKeyCount = static_cast<int>(Key::KeyCount);
    static constexpr int kMouseButtonCount = static_cast<int>(MouseButton::ButtonCount);

    bool keys_down_[kKeyCount] = {};       // Current frame: is key held
    bool keys_down_prev_[kKeyCount] = {};  // Previous frame: was key held
    bool mouse_buttons_[kMouseButtonCount] = {};
    float mouse_x_ = 0.0f;
    float mouse_y_ = 0.0f;

public:
    MacWindow(const WindowConfig& config);
    ~MacWindow() override;

    bool is_open() const override { return is_open_; }
    void close() override;

    int width() const override;
    int height() const override;
    float scale_factor() const override;
    void set_title(const std::string& title) override;

    void* native_handle() override;

    // Input - Keyboard
    bool is_key_down(Key key) const override;
    bool is_key_pressed(Key key) const override;
    bool is_key_released(Key key) const override;

    // Input - Mouse
    bool is_mouse_button_down(MouseButton button) const override;
    float mouse_x() const override { return mouse_x_; }
    float mouse_y() const override { return mouse_y_; }

    void update_input() override;

    // Internal methods for event handling
    void set_closed() { is_open_ = false; }
    NSWindow* ns_window() { return window_; }
    void handle_key_down(unsigned short keyCode);
    void handle_key_up(unsigned short keyCode);
    void handle_mouse_down(int button);
    void handle_mouse_up(int button);
    void handle_mouse_move(float x, float y);
};

} // namespace cafe

// ============================================================================
// CafeView Implementation (after MacWindow so we can call its methods)
// ============================================================================

@implementation CafeView

- (void)setEngineWindow:(cafe::MacWindow*)window {
    _engineWindow = window;
}

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        self.wantsLayer = YES;
        self.layerContentsRedrawPolicy = NSViewLayerContentsRedrawDuringViewResize;

        // Enable mouse tracking
        NSTrackingAreaOptions options = NSTrackingMouseMoved |
                                        NSTrackingActiveInKeyWindow |
                                        NSTrackingInVisibleRect;
        NSTrackingArea* trackingArea = [[NSTrackingArea alloc]
            initWithRect:NSZeroRect
                 options:options
                   owner:self
                userInfo:nil];
        [self addTrackingArea:trackingArea];
    }
    return self;
}

- (CALayer*)makeBackingLayer {
    _metalLayer = [CAMetalLayer layer];
    return _metalLayer;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)isOpaque {
    return YES;
}

// Keyboard events
- (void)keyDown:(NSEvent*)event {
    if (_engineWindow) {
        _engineWindow->handle_key_down([event keyCode]);
    }
}

- (void)keyUp:(NSEvent*)event {
    if (_engineWindow) {
        _engineWindow->handle_key_up([event keyCode]);
    }
}

// Mouse button events
- (void)mouseDown:(NSEvent*)event {
    (void)event;
    if (_engineWindow) {
        _engineWindow->handle_mouse_down(0);  // Left button
    }
}

- (void)mouseUp:(NSEvent*)event {
    (void)event;
    if (_engineWindow) {
        _engineWindow->handle_mouse_up(0);
    }
}

- (void)rightMouseDown:(NSEvent*)event {
    (void)event;
    if (_engineWindow) {
        _engineWindow->handle_mouse_down(1);  // Right button
    }
}

- (void)rightMouseUp:(NSEvent*)event {
    (void)event;
    if (_engineWindow) {
        _engineWindow->handle_mouse_up(1);
    }
}

- (void)otherMouseDown:(NSEvent*)event {
    if ([event buttonNumber] == 2 && _engineWindow) {
        _engineWindow->handle_mouse_down(2);  // Middle button
    }
}

- (void)otherMouseUp:(NSEvent*)event {
    if ([event buttonNumber] == 2 && _engineWindow) {
        _engineWindow->handle_mouse_up(2);
    }
}

// Mouse movement
- (void)mouseMoved:(NSEvent*)event {
    if (_engineWindow) {
        NSPoint pos = [self convertPoint:[event locationInWindow] fromView:nil];
        _engineWindow->handle_mouse_move(static_cast<float>(pos.x),
                                         static_cast<float>(pos.y));
    }
}

- (void)mouseDragged:(NSEvent*)event {
    [self mouseMoved:event];
}

- (void)rightMouseDragged:(NSEvent*)event {
    [self mouseMoved:event];
}

- (void)otherMouseDragged:(NSEvent*)event {
    [self mouseMoved:event];
}

@end

// ============================================================================
// Window Delegate Implementation
// ============================================================================

@implementation CafeWindowDelegate

- (void)setEngineWindow:(cafe::MacWindow*)window {
    _engineWindow = window;
}

- (BOOL)windowShouldClose:(NSWindow*)sender {
    (void)sender;
    if (_engineWindow) {
        _engineWindow->set_closed();
    }
    // Return NO - don't let Cocoa close the window automatically.
    // The main loop will detect is_open() == false, clean up renderer,
    // then the Window destructor will close the NSWindow safely.
    return NO;
}

- (void)windowDidResize:(NSNotification*)notification {
    (void)notification;
    // Will handle resize events later
}

@end

// ============================================================================
// Application Delegate Implementation
// ============================================================================

@implementation CafeAppDelegate

- (void)setMainWindow:(cafe::MacWindow*)window {
    _mainWindow = window;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender {
    (void)sender;
    if (_mainWindow) {
        _mainWindow->set_closed();
    }
    // Cancel termination - let main loop handle cleanup
    return NSTerminateCancel;
}

@end

// ============================================================================
// MacWindow Implementation
// ============================================================================

namespace cafe {

MacWindow::MacWindow(const WindowConfig& config) {
    @autoreleasepool {
        NSRect frame = NSMakeRect(0, 0, config.width, config.height);

        NSWindowStyleMask style = NSWindowStyleMaskTitled |
                                  NSWindowStyleMaskClosable |
                                  NSWindowStyleMaskMiniaturizable;
        if (config.resizable) {
            style |= NSWindowStyleMaskResizable;
        }

        window_ = [[NSWindow alloc]
            initWithContentRect:frame
            styleMask:style
            backing:NSBackingStoreBuffered
            defer:NO];

        [window_ setTitle:[NSString stringWithUTF8String:config.title.c_str()]];
        [window_ center];

        // Create our custom view
        view_ = [[CafeView alloc] initWithFrame:frame];
        [view_ setEngineWindow:this];
        [window_ setContentView:view_];

        // Set up delegate for window events
        delegate_ = [[CafeWindowDelegate alloc] init];
        [delegate_ setEngineWindow:this];
        [window_ setDelegate:delegate_];

        // Show window
        [window_ makeKeyAndOrderFront:nil];

        // Register with app delegate for Cmd+Q handling
        if (g_app_delegate) {
            [g_app_delegate setMainWindow:this];
        }
    }
}

MacWindow::~MacWindow() {
    @autoreleasepool {
        // Clear app delegate reference to avoid dangling pointer
        if (g_app_delegate) {
            [g_app_delegate setMainWindow:nil];
        }
        // Clear view's reference to avoid dangling pointer
        if (view_) {
            [view_ setEngineWindow:nil];
        }
        if (window_) {
            [window_ setDelegate:nil];
            [window_ close];
            window_ = nil;
        }
        view_ = nil;
        delegate_ = nil;
    }
}

void MacWindow::close() {
    is_open_ = false;
    if (window_) {
        [window_ close];
    }
}

int MacWindow::width() const {
    if (!view_) return 0;
    return static_cast<int>([view_ bounds].size.width);
}

int MacWindow::height() const {
    if (!view_) return 0;
    return static_cast<int>([view_ bounds].size.height);
}

float MacWindow::scale_factor() const {
    if (!window_) return 1.0f;
    return static_cast<float>([window_ backingScaleFactor]);
}

void MacWindow::set_title(const std::string& title) {
    if (window_) {
        [window_ setTitle:[NSString stringWithUTF8String:title.c_str()]];
    }
}

void* MacWindow::native_handle() {
    return (__bridge void*)view_;
}

// Input implementations
bool MacWindow::is_key_down(Key key) const {
    int index = static_cast<int>(key);
    if (index < 0 || index >= kKeyCount) return false;
    return keys_down_[index];
}

bool MacWindow::is_key_pressed(Key key) const {
    int index = static_cast<int>(key);
    if (index < 0 || index >= kKeyCount) return false;
    return keys_down_[index] && !keys_down_prev_[index];
}

bool MacWindow::is_key_released(Key key) const {
    int index = static_cast<int>(key);
    if (index < 0 || index >= kKeyCount) return false;
    return !keys_down_[index] && keys_down_prev_[index];
}

bool MacWindow::is_mouse_button_down(MouseButton button) const {
    int index = static_cast<int>(button);
    if (index < 0 || index >= kMouseButtonCount) return false;
    return mouse_buttons_[index];
}

void MacWindow::update_input() {
    // Copy current state to previous state for pressed/released detection
    std::memcpy(keys_down_prev_, keys_down_, sizeof(keys_down_));
}

void MacWindow::handle_key_down(unsigned short keyCode) {
    Key key = translate_key_code(keyCode);
    int index = static_cast<int>(key);
    if (index > 0 && index < kKeyCount) {
        keys_down_[index] = true;
    }
}

void MacWindow::handle_key_up(unsigned short keyCode) {
    Key key = translate_key_code(keyCode);
    int index = static_cast<int>(key);
    if (index > 0 && index < kKeyCount) {
        keys_down_[index] = false;
    }
}

void MacWindow::handle_mouse_down(int button) {
    if (button >= 0 && button < kMouseButtonCount) {
        mouse_buttons_[button] = true;
    }
}

void MacWindow::handle_mouse_up(int button) {
    if (button >= 0 && button < kMouseButtonCount) {
        mouse_buttons_[button] = false;
    }
}

void MacWindow::handle_mouse_move(float x, float y) {
    mouse_x_ = x;
    mouse_y_ = y;
}

// ============================================================================
// MacOS Platform Implementation
// ============================================================================

class MacPlatform : public Platform {
private:
    mach_timebase_info_data_t timebase_;
    uint64_t start_time_;

public:
    MacPlatform() {
        @autoreleasepool {
            // Initialize NSApplication
            [NSApplication sharedApplication];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

            // Set up app delegate to handle Cmd+Q properly
            g_app_delegate = [[CafeAppDelegate alloc] init];
            [NSApp setDelegate:g_app_delegate];

            // Create a simple menu bar
            NSMenu* menuBar = [[NSMenu alloc] init];
            NSMenuItem* appMenuItem = [[NSMenuItem alloc] init];
            [menuBar addItem:appMenuItem];
            [NSApp setMainMenu:menuBar];

            NSMenu* appMenu = [[NSMenu alloc] init];
            NSMenuItem* quitItem = [[NSMenuItem alloc]
                initWithTitle:@"Quit"
                action:@selector(terminate:)
                keyEquivalent:@"q"];
            [appMenu addItem:quitItem];
            [appMenuItem setSubmenu:appMenu];

            [NSApp finishLaunching];
            [NSApp activateIgnoringOtherApps:YES];
        }

        // Initialize timing
        mach_timebase_info(&timebase_);
        start_time_ = mach_absolute_time();
    }

    ~MacPlatform() override = default;

    std::unique_ptr<Window> create_window(const WindowConfig& config) override {
        return std::make_unique<MacWindow>(config);
    }

    void poll_events() override {
        @autoreleasepool {
            NSEvent* event;
            while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                              untilDate:nil
                                                 inMode:NSDefaultRunLoopMode
                                                dequeue:YES])) {
                [NSApp sendEvent:event];
            }
        }
    }

    double get_time() const override {
        uint64_t elapsed = mach_absolute_time() - start_time_;
        return static_cast<double>(elapsed * timebase_.numer) /
               static_cast<double>(timebase_.denom) / 1e9;
    }

    const char* name() const override {
        return "macOS";
    }
};

// ============================================================================
// Factory Function
// ============================================================================

std::unique_ptr<Platform> create_platform() {
    return std::make_unique<MacPlatform>();
}

} // namespace cafe
