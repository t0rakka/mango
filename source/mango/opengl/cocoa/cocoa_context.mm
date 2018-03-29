/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/string.hpp>
#include <mango/opengl/opengl.hpp>

#import <Cocoa/Cocoa.h>

namespace mango
{

    struct WindowHandle
    {
        // window state
        id     window;
        bool   looping;
        uint32 keystate[4] = { 0, 0, 0, 0 };

        // context state
        id delegate;
        id view;
        id ctx = nil;
        uint32 modifiers;
    };
    
} // namespace mango

namespace
{
    using namespace mango;

    template <typename ContainerType>
    void parseExtensionString(ContainerType& container, const char* ext)
    {
        for (const char* s = ext; *s; ++s)
        {
            if (*s == ' ')
            {
                const std::ptrdiff_t length = s - ext;
                if (length > 0)
                {
                    container.emplace(ext, length);
                }
                ext = s + 1;
            }
        }
    }

    Keycode computeKeyCode(uint32 code)
    {
        static const Keycode table[] =
        {
            // 0
            KEYCODE_A,
            KEYCODE_S,
            KEYCODE_D,
            KEYCODE_F,
            KEYCODE_H,
            KEYCODE_G,
            KEYCODE_Z,
            KEYCODE_X,
            KEYCODE_C,
            KEYCODE_V,
            // 10
            KEYCODE_NONE, // backtick
            KEYCODE_B,
            KEYCODE_Q,
            KEYCODE_W,
            KEYCODE_E,
            KEYCODE_R,
            KEYCODE_Y,
            KEYCODE_T,
            KEYCODE_1,
            KEYCODE_2,
            // 20
            KEYCODE_3,
            KEYCODE_4,
            KEYCODE_6,
            KEYCODE_5,
            KEYCODE_NONE, // equal sign
            KEYCODE_9,
            KEYCODE_7,
            KEYCODE_NONE, // minus
            KEYCODE_8,
            KEYCODE_0,
            // 30
            KEYCODE_NONE, // right bracket
            KEYCODE_O,
            KEYCODE_U,
            KEYCODE_NONE, // left bracket
            KEYCODE_I,
            KEYCODE_P,
            KEYCODE_RETURN,
            KEYCODE_L,
            KEYCODE_J,
            KEYCODE_NONE, // apostrophe
            // 40
            KEYCODE_K,
            KEYCODE_NONE, // semicolon
            KEYCODE_NONE, // backslash
            KEYCODE_NONE, // comma
            KEYCODE_NONE, // slash
            KEYCODE_N,
            KEYCODE_M,
            KEYCODE_NONE, // period
            KEYCODE_TAB,
            KEYCODE_SPACE,
            // 50
            KEYCODE_NONE,
            KEYCODE_BACKSPACE,
            KEYCODE_NONE,
            KEYCODE_ESC,
            KEYCODE_NONE, // RSUPER,
            KEYCODE_NONE, // LSUPER,
            KEYCODE_NONE, // LSHIFT,
            KEYCODE_CAPS_LOCK,
            KEYCODE_LEFT_ALT,
            KEYCODE_NONE, // LCTRL,
            // 60
            KEYCODE_NONE, // RSHIFT,
            KEYCODE_RIGHT_ALT,
            KEYCODE_NONE, // RCTRL,
            KEYCODE_NONE, // Fn
            KEYCODE_NONE, // F17
            KEYCODE_DECIMAL,
            KEYCODE_NONE,
            KEYCODE_MULTIPLY,
            KEYCODE_NONE,
            KEYCODE_ADDITION,
            // 70
            KEYCODE_NONE,
            KEYCODE_NUMLOCK,
            KEYCODE_NONE, // volume up
            KEYCODE_NONE, // volume down
            KEYCODE_NONE, // mute
            KEYCODE_DIVIDE,
            KEYCODE_ENTER,
            KEYCODE_NONE,
            KEYCODE_SUBTRACT,
            KEYCODE_NONE, // F18
            // 80
            KEYCODE_NONE, // F19
            KEYCODE_NONE, // keypad equal
            KEYCODE_NUMPAD0,
            KEYCODE_NUMPAD1,
            KEYCODE_NUMPAD2,
            KEYCODE_NUMPAD3,
            KEYCODE_NUMPAD4,
            KEYCODE_NUMPAD5,
            KEYCODE_NUMPAD6,
            KEYCODE_NUMPAD7,
            // 90
            KEYCODE_NONE, // F20
            KEYCODE_NUMPAD8,
            KEYCODE_NUMPAD9,
            KEYCODE_NONE,
            KEYCODE_NONE,
            KEYCODE_NONE,
            KEYCODE_F5,
            KEYCODE_F6,
            KEYCODE_F7,
            KEYCODE_F3,
            // 100
            KEYCODE_F8,
            KEYCODE_F9,
            KEYCODE_NONE,
            KEYCODE_F11,
            KEYCODE_NONE,
            KEYCODE_NONE, // F13
            KEYCODE_NONE, // F16
            KEYCODE_NONE, // F14
            KEYCODE_NONE,
            KEYCODE_F10,
            // 110
            KEYCODE_NONE,
            KEYCODE_F12,
            KEYCODE_NONE,
            KEYCODE_NONE, // F15
            KEYCODE_INSERT, // TODO: check
            KEYCODE_HOME,
            KEYCODE_PAGE_UP,
            KEYCODE_DELETE,
            KEYCODE_F4,
            KEYCODE_END,
            // 120
            KEYCODE_F2,
            KEYCODE_PAGE_DOWN,
            KEYCODE_F1,
            KEYCODE_LEFT,
            KEYCODE_RIGHT,
            KEYCODE_DOWN,
            KEYCODE_UP,
            KEYCODE_NONE,
        };
        
        if (code >= 128)
            return KEYCODE_NONE;
        
        return table[code];
    }

    uint32 computeKeyMask(NSUInteger flags)
    {
        uint32 mask = 0;
        if (flags & NSEventModifierFlagControl)    mask |= KEYMASK_CTRL;
        if (flags & NSEventModifierFlagShift)      mask |= KEYMASK_SHIFT;
        if (flags & NSEventModifierFlagCommand)    mask |= KEYMASK_SUPER;
        return mask;
    }
    
    // TODO: Replace this with "friend class Keyboard" to send
    //       keyboard events.
    
    class WindowAdapter : public opengl::Context
    {
    public:
        void setKeyState(Keycode code, bool press)
        {
            const int keyIndex = int(code) >> 5;
            const int keyMask = 1 << (int(code) & 31);
            
            if (press)
            {
                m_handle->keystate[keyIndex] |= keyMask;
            }
            else
            {
                m_handle->keystate[keyIndex] &= ~keyMask;
            }
        }
    };

} // namespace

// -----------------------------------------------------------------------
// Interface
// -----------------------------------------------------------------------

using mango::Window;
using mango::opengl::Context;

@interface CustomNSWindow : NSWindow {
    Window *window;
}
@property (assign) Window *window;
@end

@interface CustomNSWindowDelegate : NSObject {
    Context *context;
    WindowHandle *handle;
}
@end

@interface CustomView : NSOpenGLView {
    Context *context;
}
@end

// -----------------------------------------------------------------------
// CustomView
// -----------------------------------------------------------------------

@implementation CustomView

- (id)initWithFrame:(NSRect)frame andCustomWindow:(Context *)theContext
{
    if ((self = [super initWithFrame:frame]))
    {
        context = theContext;
    }

    // enable file drop events
    [self registerForDraggedTypes:[NSArray arrayWithObjects: NSFilenamesPboardType, nil]];
    
    return self;
}

- (void)prepareOpenGL
{
    [self setWantsBestResolutionOpenGLSurface:YES];
}

- (void)dealloc
{
    [super dealloc];
}

- (BOOL)isOpaque
{
    return YES;
}

- (BOOL)canBecomeKeyView
{
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)trackContentView:(NSWindow *)window
{
    self.translatesAutoresizingMaskIntoConstraints = NO;
    
    NSArray *verticalConstraints = [NSLayoutConstraint constraintsWithVisualFormat:@"V:|[subView]|"
                                                                           options:0
                                                                           metrics:nil
                                                                           views:@{@"subView" : self}];
    
    NSArray *horizontalConstraints = [NSLayoutConstraint constraintsWithVisualFormat:@"H:|[subView]|"
                                                                             options:0
                                                                             metrics:nil
                                                                             views:@{@"subView" : self}];

    [[window contentView] addConstraints:verticalConstraints];
    [[window contentView] addConstraints:horizontalConstraints];
}

- (void)dispatchResize:(NSRect)frame
{
    frame = [self convertRectToBacking:frame]; // retina conversion
    context->onResize(frame.size.width, frame.size.height);
}

- (void)keyDown:(NSEvent *)event
{
    if (event.isARepeat != YES)
    {
        uint32 mask = computeKeyMask(event.modifierFlags);
        Keycode code = computeKeyCode(event.keyCode);
        context->onKeyPress(code, mask);

        WindowAdapter* adapter = reinterpret_cast<WindowAdapter*>(context);
        adapter->setKeyState(code, true);
    }
}

- (void)keyUp:(NSEvent *)event
{
    Keycode code = computeKeyCode(event.keyCode);
    context->onKeyRelease(code);

    WindowAdapter* adapter = reinterpret_cast<WindowAdapter*>(context);
    adapter->setKeyState(code, false);
}

- (void)flagsChanged:(NSEvent *)event
{
    /*
    NSUInteger newModifiers = [event modifierFlags] & NSDeviceIndependentModifierFlagsMask;
    bool press = (newModifiers > window->modifiers);
    window->modifiers = (uint32)newModifiers;

    if (press)
    {
        Keycode code = translateKey(event.keyCode);
        window->onKeyPress(code);
        //e.key.mod = convertModifiers(newModifiers);
    }
    else
    {
        Keycode code = translateKey(event.keyCode);
        window->onKeyRelease(code);
        //e.key.mod = convertModifiers(newModifiers);
    }
    */
}

- (void)dispatchMouseMove:(NSEvent *)event
{
    NSPoint pos = [self convertPoint:[event locationInWindow] fromView:nil];
    NSRect frame = [self frame];

    pos = [self convertPointToBacking:pos]; // retina conversion
    
    int x = int(pos.x);
    int y = int(frame.size.height - pos.y);
    context->onMouseMove(x, y);
}

- (void)mouseMoved:(NSEvent *)event
{
    [self dispatchMouseMove:event];
}

- (void)mouseDragged:(NSEvent *)event
{
    [self dispatchMouseMove:event];
}

- (void)rightMouseDragged:(NSEvent *)event
{
    [self dispatchMouseMove:event];
}

- (void)otherMouseDragged:(NSEvent *)event
{
    [self dispatchMouseMove:event];
}

- (void)dispatchMouseClick:(NSEvent *)event andMouseButton:(MouseButton)button andClickCount:(NSInteger)clickCount
{
    NSPoint pos = [self convertPoint:[event locationInWindow] fromView:nil];
    NSRect frame = [self frame];

    pos = [self convertPointToBacking:pos]; // retina conversion

    int x = int(pos.x);
    int y = int(frame.size.height - pos.y);
    int count = int(clickCount);

    context->onMouseClick(x, y, button, count);
}

- (void)mouseDown:(NSEvent *)event
{
    [self dispatchMouseClick:event andMouseButton:MOUSEBUTTON_LEFT andClickCount:[event clickCount]];
}

- (void)mouseUp:(NSEvent *)event
{
    [self dispatchMouseClick:event andMouseButton:MOUSEBUTTON_LEFT andClickCount:0];
}

- (void)rightMouseDown:(NSEvent *)event
{
    [self dispatchMouseClick:event andMouseButton:MOUSEBUTTON_RIGHT andClickCount:[event clickCount]];
}

- (void)rightMouseUp:(NSEvent *)event
{
    [self dispatchMouseClick:event andMouseButton:MOUSEBUTTON_RIGHT andClickCount:0];
}

- (void)otherMouseDown:(NSEvent *)event
{
    MouseButton button = MouseButton(event.buttonNumber);
    [self dispatchMouseClick:event andMouseButton:button andClickCount:[event clickCount]];
}

- (void)otherMouseUp:(NSEvent *)event
{
    MouseButton button = MouseButton(event.buttonNumber);
    [self dispatchMouseClick:event andMouseButton:button andClickCount:0];
}

- (void) mouseEntered:(NSEvent *)event
{
}

- (void) mouseExited:(NSEvent *)event
{
}

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;
    context->onDraw();
}

- (NSDragOperation) draggingEntered:(id<NSDraggingInfo>)sender
{
    if ((NSDragOperationGeneric & [sender draggingSourceOperationMask]) == NSDragOperationGeneric)
        return NSDragOperationGeneric;
    else
        return NSDragOperationNone;
    
}

- (BOOL) prepareForDragOperation:(id<NSDraggingInfo>)sender
{
    return YES;
}

- (BOOL) performDragOperation:(id <NSDraggingInfo>)sender
{
    NSPasteboard *pboard = [sender draggingPasteboard];
    
    if ([[pboard types] containsObject:NSFilenamesPboardType])
    {
        NSArray *files = [pboard propertyListForType:NSFilenamesPboardType];
        int numberOfFiles = (int) [files count];

        FileIndex dropped;
        
        for (int i = 0; i < numberOfFiles; ++i)
        {
            NSString *path = [files objectAtIndex:i];

            BOOL isDirectory;
            [[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDirectory];
            
            std::string s = [path UTF8String];
            if (isDirectory)
            {
                dropped.emplace(s + "/", 0, mango::FileInfo::DIRECTORY);
            }
            else
            {
                uint64 size = [[[NSFileManager defaultManager] attributesOfItemAtPath:path error:nil] fileSize];
                dropped.emplace(s, size, 0);
            }
        }

        context->onDropFiles(dropped);
    }
    return YES;
}

- (void) concludeDragOperation:(id<NSDraggingInfo>)sender
{
    [self setNeedsDisplay:YES];
}

@end

// -----------------------------------------------------------------------
// CustomNSWindow
// -----------------------------------------------------------------------

@implementation CustomNSWindow

@synthesize window;

- (BOOL)canBecomeKeyWindow
{
    return YES;
}

- (BOOL)canBecomeMainWindow
{
    return YES;
}

- (void)createMenu
{
    id menubar = [[NSMenu new] autorelease];
    id appMenuItem = [[NSMenuItem new] autorelease];
    [menubar addItem:appMenuItem];
    [NSApp setMainMenu:menubar];
    id appMenu = [[NSMenu new] autorelease];
    id appName = [[NSProcessInfo processInfo] processName];
    id quitTitle = [@"Quit " stringByAppendingString:appName];
    id quitMenuItem = [[[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"] autorelease];
    [appMenu addItem:quitMenuItem];
    [appMenuItem setSubmenu:appMenu];
}

@end

// -----------------------------------------------------------------------
// CustomNSWindowDelegate
// -----------------------------------------------------------------------

@implementation CustomNSWindowDelegate

- (id)initWithCustomWindow:(Context *)theContext andHandle:(WindowHandle *)theHandle
{
    if ((self = [super init]))
    {
        context = theContext;
        handle = theHandle;
    }

    return self;
}

- (BOOL)windowShouldClose:(id)sender
{
    (void)sender;
    context->breakEventLoop();
    return NO;
}

- (void)windowDidBecomeKey:(NSNotification *)notification
{
    (void)notification;
    // NOTE: window gained focus
}

- (void)windowDidResignKey:(NSNotification *)notification
{
    (void)notification;
    // NOTE: window lost focus
}

- (void)windowDidResize:(NSNotification *)notification
{
    //[handle->ctx update];
    NSRect frame = [[notification object] contentRectForFrameRect: [[notification object] frame]];
    [handle->view dispatchResize:frame];
}

- (void)windowDidMove:(NSNotification *)notification
{
    (void)notification;
    //[handle->ctx update];
}

- (void)windowDidMiniaturize:(NSNotification *)notification
{
    (void)notification;
}

- (void)windowDidDeminiaturize:(NSNotification *)notification
{
    (void)notification;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication
{
    return YES;
}

@end

namespace mango
{

    // -----------------------------------------------------------------------
    // Window
    // -----------------------------------------------------------------------

    Window::Window(int width, int height)
    {
        // NOTE: Cocoa/OSX implementation only uses Window as interface and does NOT
        //       use the window constructor for anything else except creating the internal state (m_handle).
        MANGO_UNREFERENCED_PARAMETER(width);
        MANGO_UNREFERENCED_PARAMETER(height);

        m_handle = new WindowHandle();
    }

    Window::~Window()
    {
        delete m_handle;
    }

    void Window::setWindowSize(int width, int height)
    {
        [m_handle->window setContentSize:NSMakeSize(width, height)];
    }

    void Window::setTitle(const std::string& title)
    {
        [m_handle->window setTitle:[NSString stringWithUTF8String:title.c_str()]];
    }

    void Window::setIcon(const Surface& icon)
    {
        const int width = icon.width;
        const int height = icon.height;

        Bitmap bitmap(width, height, FORMAT_R8G8B8A8);
        bitmap.blit(0, 0, icon);

        NSBitmapImageRep* bitmapImage = [[NSBitmapImageRep alloc]
                                         initWithBitmapDataPlanes:&bitmap.image
                                         pixelsWide:width
                                         pixelsHigh:height
                                         bitsPerSample:8
                                         samplesPerPixel:4
                                         hasAlpha:YES
                                         isPlanar:NO
                                         colorSpaceName:NSCalibratedRGBColorSpace
                                         bytesPerRow:width * 4
                                         bitsPerPixel:32];

        NSImage* iconImage = [[NSImage alloc] initWithSize:NSMakeSize(width, height)];
        [iconImage addRepresentation:bitmapImage];

        [[NSApplication sharedApplication] setApplicationIconImage:iconImage];
        [bitmapImage release];
        [iconImage release];
    }

    void Window::setVisible(bool enable)
    {
        if (enable)
        {
            [m_handle->window makeKeyAndOrderFront:nil];
        }
        else
        {
            [m_handle->window orderOut:nil];
        }
    }

    int2 Window::getWindowSize() const
    {
        NSRect rect = [[m_handle->window contentView] frame];
        rect = [[m_handle->window contentView] convertRectToBacking:rect];
        return int2(rect.size.width, rect.size.height);
    }

	int2 Window::getCursorPosition() const
	{
        NSRect rect = [[m_handle->window contentView] frame];
		NSPoint point = [m_handle->window mouseLocationOutsideOfEventStream];
		return int2(point.x, rect.size.height - point.y - 1);
	}

    bool Window::isKeyPressed(Keycode code) const
    {
        const int keyIndex = int(code);
        uint32 state = m_handle->keystate[keyIndex >> 5] & (1 << (keyIndex & 31));
        return state != 0;
    }
    
    void Window::enterEventLoop()
    {
#if 1
        m_handle->looping = true;
        
        while (m_handle->looping)
        {
            NSEvent* event = [NSApp nextEventMatchingMask: NSEventMaskAny
                              untilDate: nil
                              inMode: NSDefaultRunLoopMode
                              dequeue: YES];
            
            if (event)
            {
                //if ([event type] == NSKeyUp && ([event modifierFlags] & NSCommandKeyMask))
                //    [[NSApp keyWindow] sendEvent:event];
                //else
                    [NSApp sendEvent:event];
            }
            else
            {
                onIdle();
            }
        }
#else
        [NSApp run];
#endif
    }
    
    void Window::breakEventLoop()
    {
#if 1
        m_handle->looping = false;
#else
        [NSApp stop:nil];
#endif
    }

    void Window::onIdle()
    {
    }

    void Window::onDraw()
    {
    }

    void Window::onResize(int width, int height)
    {
        MANGO_UNREFERENCED_PARAMETER(width);
        MANGO_UNREFERENCED_PARAMETER(height);
    }

    void Window::onMinimize()
    {
    }

    void Window::onMaximize()
    {
    }

    void Window::onKeyPress(Keycode code, uint32 mask)
    {
        MANGO_UNREFERENCED_PARAMETER(code);
        MANGO_UNREFERENCED_PARAMETER(mask);
    }

    void Window::onKeyRelease(Keycode code)
    {
        MANGO_UNREFERENCED_PARAMETER(code);
    }

    void Window::onMouseMove(int x, int y)
    {
        MANGO_UNREFERENCED_PARAMETER(x);
        MANGO_UNREFERENCED_PARAMETER(y);
    }

    void Window::onMouseClick(int x, int y, MouseButton button, int count)
    {
        MANGO_UNREFERENCED_PARAMETER(x);
        MANGO_UNREFERENCED_PARAMETER(y);
        MANGO_UNREFERENCED_PARAMETER(button);
        MANGO_UNREFERENCED_PARAMETER(count);
    }

    void Window::onDropFiles(const FileIndex& index)
    {
        MANGO_UNREFERENCED_PARAMETER(index);
    }

    void Window::onClose()
    {
    }

    void Window::onShow()
    {
    }

    void Window::onHide()
    {
    }

namespace opengl {

    // -----------------------------------------------------------------------
    // Context
    // -----------------------------------------------------------------------

    Context::Context(int width, int height, const ContextAttribute* contextAttribute, Context* shared)
	: Window(width, height), m_context(nullptr)
    {
        [NSApplication sharedApplication];
        //xxx.autorelease_pool = [[NSAutoreleasePool alloc] init];
        
        ProcessSerialNumber psn = { 0, kCurrentProcess };
        TransformProcessType(&psn, kProcessTransformToForegroundApplication);
        [[NSApplication sharedApplication] activateIgnoringOtherApps: YES];
        
        [NSEvent setMouseCoalescingEnabled:NO];
        [NSApp finishLaunching];

        // "createContext"

        m_handle->view = nil;
        
        unsigned int styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
            NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
        
        NSRect frame = NSMakeRect(0, 0, width, height);

        m_handle->window = [[CustomNSWindow alloc]
                      initWithContentRect:frame
                      styleMask:styleMask
                      backing:NSBackingStoreBuffered
                      defer:NO];
        if (!m_handle->window)
        {
            printf("NSWindow initWithContentRect failed.\n");
            return;
        }
        
        ((CustomNSWindow *)m_handle->window).window = this;
        [m_handle->window center];
        m_handle->delegate = [[CustomNSWindowDelegate alloc] initWithCustomWindow:this andHandle:m_handle];
        [m_handle->window setDelegate:[m_handle->delegate retain]];
        [m_handle->window setAcceptsMouseMovedEvents:YES];
        [m_handle->window setTitle:@"OpenGL"];
        [m_handle->window setReleasedWhenClosed:NO];
        
        m_handle->view = [[CustomView alloc] initWithFrame:[m_handle->window frame]andCustomWindow:this];
        if (!m_handle->view)
        {
            printf("NSView initWithFrame failed.\n");
            // TODO: delete window
            return;
        }
        
        m_handle->view = [m_handle->view retain];
#if 0
        [m_handle->window setContentView:m_view];
#else
        [[m_handle->window contentView] addSubview:m_handle->view];
        [m_handle->view trackContentView:m_handle->window];
#endif

        // Configure the smallest allowed window size
        [m_handle->window setMinSize:NSMakeSize(32, 32)];

        // Create menu
        [m_handle->window createMenu];
        
        // configure attributes
        ContextAttribute attrib;
        if (contextAttribute)
        {
            // override defaults
            attrib = *contextAttribute;
        }

        std::vector<NSOpenGLPixelFormatAttribute> attribs;

#ifdef MANGO_CORE_PROFILE
        attribs.push_back(NSOpenGLPFAOpenGLProfile);
        attribs.push_back(NSOpenGLProfileVersion3_2Core);
#endif
        attribs.push_back(NSOpenGLPFAAccelerated);
        attribs.push_back(NSOpenGLPFADoubleBuffer);
        //attribs.push_back(NSOpenGLPFATripleBuffer);
        //attribs.push_back(NSOpenGLPFAFullScreen);
        //attribs.push_back(NSOpenGLPFAWindow);

        attribs.push_back(NSOpenGLPFAColorSize);
        attribs.push_back(attrib.red + attrib.green + attrib.blue);
        attribs.push_back(NSOpenGLPFAAlphaSize);
        attribs.push_back(attrib.alpha);
        attribs.push_back(NSOpenGLPFADepthSize);
        attribs.push_back(attrib.depth);
        attribs.push_back(NSOpenGLPFAStencilSize);
        attribs.push_back(attrib.stencil);

        if (attrib.samples > 1)
        {
            attribs.push_back(NSOpenGLPFASampleBuffers);
            attribs.push_back(1);
            attribs.push_back(NSOpenGLPFASamples);
            attribs.push_back(attrib.samples);
#if 0
            attribs.push_back(NSOpenGLPFASupersample);
#else
            attribs.push_back(NSOpenGLPFAMultisample);
#endif
        }

        // terminate attribs vector
        attribs.push_back(0);

        id pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs.data()];
        if (pixelFormat == nil)
        {
            printf("NSOpenGLPixelFormat initWithAttributes failed.\n");
            // TODO: delete window
            return;
        }
        
        m_handle->ctx = [[NSOpenGLContext alloc]
                  initWithFormat:pixelFormat
                  shareContext: shared ? shared->m_handle->ctx : nil];
        [pixelFormat release];

        if (!m_handle->ctx)
        {
            printf("Failed to create NSOpenGL Context.\n");
            // TODO: delete window
            return;
        }

        [m_handle->view setOpenGLContext:m_handle->ctx];
        [m_handle->ctx makeCurrentContext];

        if ([m_handle->window respondsToSelector:@selector(setRestorable:)])
        {
            [m_handle->window setRestorable:NO];
        }

        m_handle->modifiers = [NSEvent modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;

        // parse extension string
#ifdef MANGO_CORE_PROFILE
        GLint numExtensions;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
        for (int i = 0; i < numExtensions; ++i)
        {
            const GLubyte* extension = glGetStringi(GL_EXTENSIONS, i);
            m_extensions.insert(reinterpret_cast<const char*>(extension));
        }
#else
        const GLubyte* extensions = glGetString(GL_EXTENSIONS);
        if (extensions)
        {
            parseExtensionString(m_extensions, reinterpret_cast<const char*>(extensions));
        }
#endif

        initExtensionMask();

        [m_handle->ctx update];
        [m_handle->view dispatchResize:frame];
        setVisible(true);
    }

    Context::~Context()
    {
        [NSOpenGLContext clearCurrentContext];

        if (m_handle->ctx)
        {
            [m_handle->ctx release];
            m_handle->ctx = nil;
        }

        if (m_handle->window)
        {
            [m_handle->window setDelegate:nil];
            [m_handle->delegate release];
            [m_handle->window setContentView:nil];
            [m_handle->view release];
            [m_handle->window close];
        }

        [NSApp stop:nil];

#if 0
        if (xxx.autorelease_pool)
        {
            [xxx.autorelease_pool drain];
            xxx.autorelease_pool = nil;
        }
#endif
    }

    void Context::makeCurrent()
    {
        [m_handle->ctx makeCurrentContext];
    }

    void Context::swapBuffers()
    {
        [m_handle->ctx flushBuffer];
    }

    void Context::swapInterval(int interval)
    {
        GLint sync = interval;
        [m_handle->ctx setValues:&sync forParameter:NSOpenGLCPSwapInterval];
    }

    void Context::toggleFullscreen()
    {
        CustomView* view = m_handle->view;
        CustomNSWindow* window = m_handle->window;

        [view setHidden:YES];

        if ([view isInFullScreenMode])
        {
            [view exitFullScreenModeWithOptions:nil];
            [window makeFirstResponder:view];
            [window makeKeyAndOrderFront:view];
            [view trackContentView:window];
        }
        else
        {
            [view enterFullScreenMode:[window screen] withOptions:nil];
        }

        [view dispatchResize:[view frame]];
        [view setHidden:NO];
    }

    bool Context::isFullscreen() const
	{
        return [m_handle->view isInFullScreenMode];
	}

} // namespace opengl
} // namespace mango
