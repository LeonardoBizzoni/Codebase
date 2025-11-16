internal VkSurfaceKHR rhi_vulkan_surface_create(OS_Handle os_window) {
  VkSurfaceKHR res = {0};
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window*)os_window.h[0];
  VkXlibSurfaceCreateInfoKHR create_surface_info = {
    .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
    .dpy = os_gfx_posix_state.xdisplay,
    .window = window->xwindow,
  };
  VkResult surface_create_result = vkCreateXlibSurfaceKHR(rhi_vulkan_state.instance, &create_surface_info, 0, &res);
  Assert(surface_create_result == VK_SUCCESS);
  return res;
}

// TODO(lb): implement this
fn void rhi_vulkan_surface_destroy(VkSurfaceKHR surface) {
  Unused(surface);
}
