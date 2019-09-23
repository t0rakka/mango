/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/framebuffer/framebuffer.hpp>
#include "../../window/cocoa/cocoa_window.h"

#include <CoreGraphics/CGContext.h>

@interface FramebufferView : NSView {
    mango::framebuffer::Framebuffer *framebuffer;
}

- (void)dispatchResize:(NSRect)frame;
- (id)initWithFrame:(NSRect)frame andFramebuffer:(mango::framebuffer::Framebuffer *)theFramebuffer;
- (void)trackContentView:(NSWindow *)window;

@end

namespace
{
    using namespace mango;

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

#if 1
    // TODO: Replace this with "friend class Keyboard" to send
    //       keyboard events.
    
    class WindowAdapter : public framebuffer::Framebuffer
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
#endif

} // namespace

// -----------------------------------------------------------------------
// FramebufferDelegate
// -----------------------------------------------------------------------

@interface FramebufferDelegate : NSObject {
    mango::Window *window;
}

- (id)initWithWindow:(mango::Window *)theWindow;
@end

// ...

@implementation FramebufferDelegate

- (id)initWithWindow:(mango::Window *)theWindow;
{
    if ((self = [super init]))
    {
        window = theWindow;
    }

    return self;
}

- (BOOL)windowShouldClose:(id)sender
{
    (void)sender;
    window->breakEventLoop();
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
    //NSRect frame = [[notification object] contentRectForFrameRect: [[notification object] frame]];
    //[context_handle->view dispatchResize:frame];
}

- (void)windowDidMove:(NSNotification *)notification
{
    (void)notification;
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

// -----------------------------------------------------------------------
// FramebufferView
// -----------------------------------------------------------------------

@implementation FramebufferView

- (id)initWithFrame:(NSRect)frame andFramebuffer:(mango::framebuffer::Framebuffer *)theFramebuffer;
{
    if ((self = [super initWithFrame:frame]))
    {
        framebuffer = theFramebuffer;
    }

    // enable file drop events
#if 1
    // deprecated in 10.14
    [self registerForDraggedTypes:[NSArray arrayWithObjects: NSFilenamesPboardType, nil]];
#else
    // requires 10.13 or later version
    // TODO: does not work, fix
    [self registerForDraggedTypes:[NSArray arrayWithObjects: NSPasteboardTypeFileURL, nil]];
#endif
    
    return self;
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
    framebuffer->onResize(frame.size.width, frame.size.height);
}

- (void)keyDown:(NSEvent *)event
{
    if (event.isARepeat != YES)
    {
        uint32 mask = computeKeyMask(event.modifierFlags);
        Keycode code = computeKeyCode(event.keyCode);
        framebuffer->onKeyPress(code, mask);

        WindowAdapter* adapter = reinterpret_cast<WindowAdapter*>(framebuffer);
        adapter->setKeyState(code, true);
    }
}

- (void)keyUp:(NSEvent *)event
{
    Keycode code = computeKeyCode(event.keyCode);
    framebuffer->onKeyRelease(code);

    WindowAdapter* adapter = reinterpret_cast<WindowAdapter*>(framebuffer);
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
    framebuffer->onMouseMove(x, y);
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

- (void)dispatchMouseClick:(NSEvent *)event andMouseButton:(mango::MouseButton)button andClickCount:(NSInteger)clickCount
{
    NSPoint pos = [self convertPoint:[event locationInWindow] fromView:nil];
    NSRect frame = [self frame];

    pos = [self convertPointToBacking:pos]; // retina conversion

    int x = int(pos.x);
    int y = int(frame.size.height - pos.y);
    int count = int(clickCount);

    framebuffer->onMouseClick(x, y, button, count);
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
    framebuffer->onDraw();
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

#if 1
    // deprecated in 10.14
    if ([[pboard types] containsObject:NSFilenamesPboardType])
#else
    // requires 10.13 or later version
    // TODO: does not work, fix
    if ([[pboard types] containsObject:NSPasteboardTypeFileURL])
#endif
    {
#if 1
        // deprecated in 10.14
        NSArray *files = [pboard propertyListForType:NSFilenamesPboardType];
#else
        // requires 10.13 or later version
        // TODO: does not work, fix
        NSArray *files = [pboard propertyListForType:NSPasteboardTypeFileURL];
#endif
        int numberOfFiles = (int) [files count];

        filesystem::FileIndex dropped;

        for (int i = 0; i < numberOfFiles; ++i)
        {
            NSString *path = [files objectAtIndex:i];

            BOOL isDirectory;
            [[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDirectory];

            std::string s = [path UTF8String];
            if (isDirectory)
            {
                dropped.emplace(s + "/", 0, mango::filesystem::FileInfo::DIRECTORY);
            }
            else
            {
                u64 size = [[[NSFileManager defaultManager] attributesOfItemAtPath:path error:nil] fileSize];
                dropped.emplace(s, size, 0);
            }
        }

        framebuffer->onDropFiles(dropped);
    }
    return YES;
}

- (void) concludeDragOperation:(id<NSDraggingInfo>)sender
{
    [self setNeedsDisplay:YES];
}

@end

namespace mango {    
namespace framebuffer {

	// -------------------------------------------------------------------
	// FramebufferContext
	// -------------------------------------------------------------------

    struct FramebufferContext
    {
        Bitmap bitmap;

        id delegate;
        id view;
        id image_view;
        u32 modifiers;

        FramebufferContext(int width, int height)
            : bitmap(width, height, Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8))
        {
        }

        ~FramebufferContext()
        {
        }

        void convertBufferToRGBA()
        {
            int count = bitmap.width * bitmap.height;
            u32* image = bitmap.address<u32>();

            for (int i = 0; i < count; ++i)
            {
                // swap RED and BLUE
                u32 color = image[i];
                color = (color & 0xff00ff00)
                     | ((color & 0x00ff0000) >> 16)
                     | ((color & 0x000000ff) << 16);
                color |= 0xff000000; // force alpha to 1.0 (compositor has alpha blending enabled)
                image[i] = color;
            }
        }

        Surface lock()
        {
            return bitmap;
        }

        void unlock()
        {
        }

        void present()
        {
            convertBufferToRGBA();

            int width = bitmap.width;
            int height = bitmap.height;
            int stride = bitmap.stride;
            u8* data = bitmap.address<u8>();

            NSBitmapImageRep *imageRep = [[NSBitmapImageRep alloc]
                        initWithBitmapDataPlanes:&data
                        pixelsWide:width
                        pixelsHigh:height
                        bitsPerSample:8 samplesPerPixel:4
                        hasAlpha:YES
                        isPlanar:NO
                        colorSpaceName:NSCalibratedRGBColorSpace
                        bytesPerRow:stride
                        bitsPerPixel:32];
            NSImage *image = [[NSImage alloc] init];
            [image addRepresentation:imageRep];
            [image_view setImage:image];

            imageRep = nil;
            image = nil;            
        }
    };

	// -------------------------------------------------------------------
	// Framebuffer
	// -------------------------------------------------------------------

    Framebuffer::Framebuffer(int width, int height)
        : Window(width, height, Window::DISABLE_RESIZE)
    {
        // app setup

        [NSApplication sharedApplication];
        
        ProcessSerialNumber psn = { 0, kCurrentProcess };
        TransformProcessType(&psn, kProcessTransformToForegroundApplication);
        [[NSApplication sharedApplication] activateIgnoringOtherApps: YES];
        
        [NSEvent setMouseCoalescingEnabled:NO];
        [NSApp finishLaunching];

        // create window

        unsigned int styleMask = NSWindowStyleMaskTitled
                               | NSWindowStyleMaskClosable
                               | NSWindowStyleMaskMiniaturizable;
                            // | NSWindowStyleMaskResizable;

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

        m_context = new FramebufferContext(width, height);

        ((CustomNSWindow *)m_handle->window).window = this;
        [ (NSWindow*) m_handle->window center];
        m_context->delegate = [[FramebufferDelegate alloc] initWithWindow:this];
        [m_handle->window setDelegate:[m_context->delegate retain]];
        [m_handle->window setAcceptsMouseMovedEvents:YES];
        [m_handle->window setTitle:@"Framebuffer"];
        [m_handle->window setReleasedWhenClosed:NO];

        m_context->view = [[FramebufferView alloc] initWithFrame:[m_handle->window frame]andFramebuffer:this];
        if (!m_context->view)
        {
            printf("NSView initWithFrame failed.\n");
            // TODO: delete window
            return;
        }

        //[m_handle->window setMinSize:NSMakeSize(width, height)];
        //[m_handle->window setMaxSize:NSMakeSize(width, height)];

        // Create menu
        [m_handle->window createMenu];

        m_context->view = [m_context->view retain];
        m_context->image_view = [[NSImageView alloc] initWithFrame:[m_handle->window frame]];

        [m_handle->window setContentView:m_context->image_view];
        [m_context->image_view addSubview:m_context->view];
        
        [m_context->view setFrameOrigin: NSMakePoint(0,-20000)];

        if ([m_handle->window respondsToSelector:@selector(setRestorable:)])
        {
            [m_handle->window setRestorable:NO];
        }

        m_context->modifiers = [NSEvent modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;

        [m_context->view dispatchResize:frame];
        setVisible(true);
    }

    Framebuffer::~Framebuffer()
    {
        if (m_handle->window)
        {
            [m_handle->window setDelegate:nil];
            [m_context->delegate release];
            [m_handle->window setContentView:nil];
            [m_context->view release];
            [m_handle->window close];
        }

        [NSApp stop:nil];

        delete m_context;
    }

    Surface Framebuffer::lock()
    {
        return m_context->lock();
    }

    void Framebuffer::unlock()
    {
        m_context->unlock();
    }

    void Framebuffer::present()
    {
        m_context->present();
    }

} // namespace framebuffer
} // namespace mango
