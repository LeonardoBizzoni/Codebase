global OS_InputDeviceState os_input_device = {0};

fn OS_Handle os_window_open_on_monitor(String8 name, i32 width, i32 height, OS_Handle hmonitor) {
  OS_Handle res = os_window_open(name, width, height);
  os_window_set_monitor(res, hmonitor);
  return res;
}
