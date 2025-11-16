internal VkSurfaceKHR rhi_vulkan_surface_create(OS_Handle os_window) {
  VkSurfaceKHR res = {};
  W32_Window *window = (W32_Window*)os_window.h[0];
  VkWin32SurfaceCreateInfoKHR create_surface_info = {
    .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
    .hinstance = w32_gfxstate.instance,
    .hwnd = window->handle,
  };
  VkResult surface_create_result = vkCreateWin32SurfaceKHR(rhi_vulkan_state.instance, &create_surface_info, 0, &res);
  Assert(surface_create_result == VK_SUCCESS);
  return res;
}
