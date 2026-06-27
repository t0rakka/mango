/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include "cocoa_input.hpp"
#include "cocoa_window.h"

#if defined(MANGO_WINDOW_SYSTEM_COCOA)

#import <QuartzCore/QuartzCore.h>

#if defined(MANGO_ENABLE_VULKAN)
#include <vulkan/vulkan_metal.h>
#endif

namespace mango
{

    void WindowContext::updateMetalDrawableSize()
    {
        if (!layer || !content_view)
        {
            return;
        }

        NSView* view = (__bridge NSView*)content_view;
        CAMetalLayer* metalLayer = (__bridge CAMetalLayer*)layer;
        NSRect frame = [view convertRectToBacking:[view bounds]];
        metalLayer.drawableSize = CGSizeMake(frame.size.width, frame.size.height);
    }

    bool WindowContext::init(Window* owner, int width, int height, u32 flags, const char* title)
    {
        is_opengl = (flags & Window::API_OPENGL) != 0;
        is_vulkan = (flags & Window::API_VULKAN) != 0;

        cocoa::initApplication();

        unsigned int styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
            NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;

        if (flags & Window::DISABLE_RESIZE)
        {
            styleMask &= ~NSWindowStyleMaskResizable;
        }

        NSRect frame = NSMakeRect(0, 0, width, height);

        CustomNSWindow* win = [[CustomNSWindow alloc]
            initWithContentRect:frame
            styleMask:styleMask
            backing:NSBackingStoreBuffered
            defer:NO];

        if (!win)
        {
            return false;
        }

        win->mangoWindow = owner;
        ns_window = (__bridge void*)win;

        [win center];
        [win setAcceptsMouseMovedEvents:YES];
        [win setTitle:[NSString stringWithUTF8String:title]];
        [win setReleasedWhenClosed:NO];
        [win setMinSize:NSMakeSize(32, 32)];
        [win createMenu];

        if ([win respondsToSelector:@selector(setRestorable:)])
        {
            [win setRestorable:NO];
        }

        // Enable AppKit native (window-level) fullscreen. This keeps the same window,
        // content view and CAMetalLayer when toggling fullscreen, so the Vulkan surface
        // stays valid and the swapchain recreates via the normal windowDidResize path.
        if (!(flags & Window::DISABLE_RESIZE))
        {
            [win setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
        }

#if defined(MANGO_ENABLE_VULKAN)
        if (is_vulkan)
        {
            MangoMetalView* view = [[MangoMetalView alloc] initWithFrame:frame window:owner context:this];
            if (!view)
            {
                return false;
            }

            content_view = (__bridge void*)[view retain];
            layer = (__bridge void*)view.layer;

            [[win contentView] addSubview:view];
            [view trackContentView:win];
            updateMetalDrawableSize();
        }
#endif

        if (content_view)
        {
            ns_delegate = cocoa::createWindowDelegate(owner, content_view);
            [win setDelegate:(__bridge id)ns_delegate];
        }

        return true;
    }

    void WindowContext::attachContentView(Window* owner, void* view)
    {
        if (!view || content_view)
        {
            return;
        }

        NSView* nsView = (__bridge NSView*)view;
        CustomNSWindow* win = (__bridge CustomNSWindow*)ns_window;

        content_view = (__bridge void*)[nsView retain];
        [[win contentView] addSubview:nsView];
        cocoa::trackContentView(nsView, win);

        ns_delegate = cocoa::createWindowDelegate(owner, content_view);
        [win setDelegate:(__bridge id)ns_delegate];
    }

    void WindowContext::shutdown()
    {
        if (!ns_window)
        {
            return;
        }

        CustomNSWindow* win = (__bridge CustomNSWindow*)ns_window;

        [win setDelegate:nil];
        if (ns_delegate)
        {
            [(id)(__bridge id)ns_delegate release];
            ns_delegate = nullptr;
        }

        if (content_view)
        {
            [(NSView*)(__bridge id)content_view release];
            content_view = nullptr;
        }

        layer = nullptr;
        [win close];
        [win release];
        ns_window = nullptr;

        [NSApp stop:nil];
    }

    WindowContext::~WindowContext()
    {
        shutdown();
    }

    void WindowContext::toggleFullscreen()
    {
        if (!content_view || !ns_window)
        {
            return;
        }

        CustomNSWindow* win = (__bridge CustomNSWindow*)ns_window;
        cocoa::toggleViewFullscreen(win->mangoWindow, this, win, (__bridge NSView*)content_view);
    }

    bool WindowContext::isFullscreen() const
    {
        if (ns_window)
        {
            NSWindow* win = (__bridge NSWindow*)ns_window;
            return ([win styleMask] & NSWindowStyleMaskFullScreen) != 0;
        }

        return false;
    }

#if defined(MANGO_ENABLE_VULKAN)

    VkSurfaceKHR WindowContext::createVulkanSurface(VkInstance instance)
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;

        VkMetalSurfaceCreateInfoEXT surfaceCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
            .pLayer = static_cast<const CAMetalLayer*>(layer),
        };

        VkResult result = vkCreateMetalSurfaceEXT(instance, &surfaceCreateInfo, nullptr, &surface);
        if (result != VK_SUCCESS)
        {
            // getString lives in the vulkan namespace; keep this self-contained.
            MANGO_EXCEPTION("[WindowContext] vkCreateMetalSurfaceEXT failed.");
        }

        return surface;
    }

    bool WindowContext::getPresentationSupport(VkPhysicalDevice physicalDevice, u32 queueFamilyIndex)
    {
        MANGO_UNREFERENCED(physicalDevice);
        MANGO_UNREFERENCED(queueFamilyIndex);
        return true;
    }

#endif // defined(MANGO_ENABLE_VULKAN)

    math::int32x2 WindowContext::getContentSize() const
    {
        if (!content_view)
        {
            return math::int32x2(0, 0);
        }

        // Use the view's own bounds (the coordinate space convertRectToBacking
        // expects) so the reported size matches the Metal drawable / swapchain
        // extent exactly. [view frame] is in the superview's coordinate space and
        // could diverge across reparenting (e.g. fullscreen enter/exit).
        NSView* view = (__bridge NSView*)content_view;
        NSRect rect = [view convertRectToBacking:[view bounds]];
        return math::int32x2(int(rect.size.width), int(rect.size.height));
    }

} // namespace mango

#endif // defined(MANGO_WINDOW_SYSTEM_COCOA)
