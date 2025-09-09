fn VkSurfaceKHR rhi_vk_surface_create(OS_Handle os_window) {
  VkSurfaceKHR res = {};
  Wl_Window *window = (Wl_Window*)os_window.h[0];
  VkWaylandSurfaceCreateInfoKHR create_surface_info = {
    .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
    .display = waystate.wl_display,
    .surface = window->wl_surface,
  };
  VkResult surface_create_result =
    vkCreateWaylandSurfaceKHR(rhi_vk_state.instance, &create_surface_info,
                              0, &res);
  Assert(surface_create_result == VK_SUCCESS);
  return res;
}

fn void rhi_vk_surface_destroy(VkSurfaceKHR surface) {}
