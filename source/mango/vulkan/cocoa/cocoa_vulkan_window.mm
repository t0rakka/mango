/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/window/window.hpp>
#include "../window_surface.hpp"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_metal.h>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

#include "../../window/cocoa/cocoa_window.h"
#include "../../window/cocoa/cocoa_window.hpp"
#include "../../window/cocoa/cocoa_input.hpp"
#include "cocoa_metal_view.h"

namespace mango::vulkan
{

    void ensureCocoaVulkanContent(Window* window, WindowBackend* backend, int width, int height)
    {
        auto* context = static_cast<WindowContext*>(backend);
        if (context->content_view)
        {
            return;
        }

        CustomNSWindow* win = (__bridge CustomNSWindow*)context->ns_window;
        if (!win)
        {
            MANGO_EXCEPTION("[Vulkan] Cocoa window is not initialized.");
        }

        NSRect frame = NSMakeRect(0, 0, width, height);
        MangoMetalView* view = [[MangoMetalView alloc] initWithFrame:frame window:window context:context];
        if (!view)
        {
            MANGO_EXCEPTION("[Vulkan] Failed to create Metal view.");
        }

        context->content_view = (__bridge void*)[view retain];
        context->layer = (__bridge void*)view.layer;

        [[win contentView] addSubview:view];
        [view trackContentView:win];
        context->updateMetalDrawableSize();

        context->ns_delegate = cocoa::createWindowDelegate(window, context->content_view);
        [win setDelegate:(__bridge id)context->ns_delegate];
    }

    VkSurfaceKHR createVulkanSurfaceCocoa(WindowBackend* backend, VkInstance instance)
    {
        auto* context = static_cast<WindowContext*>(backend);

        VkSurfaceKHR surface = VK_NULL_HANDLE;

        VkMetalSurfaceCreateInfoEXT surfaceCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
            .pLayer = static_cast<const CAMetalLayer*>(context->layer),
        };

        VkResult result = vkCreateMetalSurfaceEXT(instance, &surfaceCreateInfo, nullptr, &surface);
        if (result != VK_SUCCESS)
        {
            MANGO_EXCEPTION("[WindowContext] vkCreateMetalSurfaceEXT failed.");
        }

        return surface;
    }

    bool getVulkanPresentationSupportCocoa(WindowBackend* backend, VkPhysicalDevice physicalDevice, u32 queueFamilyIndex)
    {
        MANGO_UNREFERENCED(backend);
        MANGO_UNREFERENCED(physicalDevice);
        MANGO_UNREFERENCED(queueFamilyIndex);
        return true;
    }

} // namespace mango::vulkan

// -----------------------------------------------------------------------
// MangoMetalView
// -----------------------------------------------------------------------

@implementation MangoMetalView

- (id)initWithFrame:(NSRect)frame window:(mango::Window*)window context:(mango::WindowContext*)context
{
    if ((self = [super initWithFrame:frame]))
    {
        mangoWindow = window;
        mangoContext = context;

        self.wantsLayer = YES;
        self.layer = [CAMetalLayer layer];
        self.layer.contentsScale = [[NSScreen mainScreen] backingScaleFactor];

        [self registerForDraggedTypes:[NSArray arrayWithObjects:NSPasteboardTypeFileURL, nil]];
    }

    return self;
}

- (BOOL)isOpaque
{
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)viewDidChangeBackingProperties
{
    [super viewDidChangeBackingProperties];
    self.layer.contentsScale = self.window.backingScaleFactor;
    mangoContext->updateMetalDrawableSize();
}

- (void)dispatchResize:(NSRect)frame
{
    mango::cocoa::dispatchResize(mangoWindow, mangoContext, self, frame);
}

- (void)trackContentView:(NSWindow*)window
{
    mango::cocoa::trackContentView(self, window);
}

- (void)keyDown:(NSEvent*)event
{
    mango::cocoa::viewKeyDown(mangoWindow, mangoContext, event);
}

- (void)keyUp:(NSEvent*)event
{
    mango::cocoa::viewKeyUp(mangoWindow, mangoContext, event);
}

- (void)mouseMoved:(NSEvent*)event { mango::cocoa::viewMouseMove(mangoWindow, self, event); }
- (void)mouseDragged:(NSEvent*)event { mango::cocoa::viewMouseMove(mangoWindow, self, event); }
- (void)rightMouseDragged:(NSEvent*)event { mango::cocoa::viewMouseMove(mangoWindow, self, event); }
- (void)otherMouseDragged:(NSEvent*)event { mango::cocoa::viewMouseMove(mangoWindow, self, event); }

- (void)mouseDown:(NSEvent*)event
{
    mango::cocoa::viewMouseClick(mangoWindow, self, event, mango::MOUSEBUTTON_LEFT, int([event clickCount]));
}

- (void)mouseUp:(NSEvent*)event
{
    mango::cocoa::viewMouseClick(mangoWindow, self, event, mango::MOUSEBUTTON_LEFT, 0);
}

- (void)rightMouseDown:(NSEvent*)event
{
    mango::cocoa::viewMouseClick(mangoWindow, self, event, mango::MOUSEBUTTON_RIGHT, int([event clickCount]));
}

- (void)rightMouseUp:(NSEvent*)event
{
    mango::cocoa::viewMouseClick(mangoWindow, self, event, mango::MOUSEBUTTON_RIGHT, 0);
}

- (void)otherMouseDown:(NSEvent*)event
{
    mango::cocoa::viewMouseClick(mangoWindow, self, event, mango::MouseButton(event.buttonNumber), int([event clickCount]));
}

- (void)otherMouseUp:(NSEvent*)event
{
    mango::cocoa::viewMouseClick(mangoWindow, self, event, mango::MouseButton(event.buttonNumber), 0);
}

- (void)scrollWheel:(NSEvent*)event
{
    mango::cocoa::viewScrollWheel(mangoWindow, self, event);
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender
{
    if ((NSDragOperationGeneric & [sender draggingSourceOperationMask]) == NSDragOperationGeneric)
        return NSDragOperationGeneric;
    return NSDragOperationNone;
}

- (BOOL)prepareForDragOperation:(id<NSDraggingInfo>)sender
{
    (void)sender;
    return YES;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender
{
    mango::cocoa::viewDropFiles(mangoWindow, (__bridge void*)sender);
    return YES;
}

@end
