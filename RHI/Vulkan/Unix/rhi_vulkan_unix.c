fn VkSurfaceKHR rhi_vk_surface_create(OS_Handle os_window) {
  VkSurfaceKHR res = {0};
  X11_Window *window = (X11_Window*)os_window.h[0];
  VkXlibSurfaceCreateInfoKHR create_surface_info = {
    .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
    .dpy = x11_state.xdisplay,
    .window = window->xwindow,
  };
  VkResult surface_create_result =
    vkCreateXlibSurfaceKHR(rhi_vk_state.instance, &create_surface_info, 0, &res);
  Assert(surface_create_result == VK_SUCCESS);
  return res;
}

// TODO(lb): implement this
fn void rhi_vk_surface_destroy(VkSurfaceKHR surface) {
  Unused(surface);
}
