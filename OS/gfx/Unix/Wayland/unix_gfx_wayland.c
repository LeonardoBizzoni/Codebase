global Wl_State waystate = {0};

// =============================================================================
// Common Linux definitions
fn void unx_gfx_init(void) {
  waystate.arena = ArenaBuild();
  waystate.events.mutex = os_mutex_alloc();

  // NOTE(lb): object-id:1 is the wayland display itself
  waystate.curr_id = 1;
  waystate.display = wl_display_connect();
  waystate.registry = wl_display_get_registry();

  waystate.msg_dispatcher = os_thread_start(wl_compositor_msg_dispatcher, 0);
  while (!(waystate.wl_compositor && waystate.wl_shm && waystate.xdg_wm_base));
}

fn void unx_gfx_deinit(void) {
  os_thread_cancel(waystate.msg_dispatcher);
}

// =============================================================================
// wayland specific definitions
inline fn u32 wl_allocate_id(void) {
  waystate.curr_id += 1;
  return waystate.curr_id;
}

fn i32 wl_display_connect(void) {
  String8 xdg_runtime_dir = str8_from_cstr(getenv("XDG_RUNTIME_DIR"));
  String8 wayland_display = str8_from_cstr(getenv("WAYLAND_DISPLAY"));
  if (!wayland_display.size) { wayland_display = Strlit("wayland-0"); }

  // UNIX domain sockets
  struct sockaddr_un addr = {};
  addr.sun_family = AF_UNIX;
  Assert(xdg_runtime_dir.size > 0 && xdg_runtime_dir.size <= Arrsize(addr.sun_path));

  Scratch scratch = ScratchBegin(0, 0);
  StringStream ss = {};
  strstream_append_str(scratch.arena, &ss, xdg_runtime_dir);
  strstream_append_str(scratch.arena, &ss, Strlit("/"));
  strstream_append_str(scratch.arena, &ss, wayland_display);

  String8 fullpath = str8_from_stream(scratch.arena, ss);
  Info("%.*s", Strexpand(fullpath));
  memcopy(addr.sun_path, fullpath.str, fullpath.size);

  i32 fd = socket(AF_UNIX, SOCK_STREAM, 0);
  Assert(fd != -1);
  Assert(connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != -1);
  ScratchEnd(scratch);
  return fd;
}

fn u32 wl_display_get_registry(void) {
  struct { Wl_MessageHeader msg; u32 new_id; } req = {
    .msg = {
      .id = WL_DISPLAY_OBJECT_ID,
      .opcode = WL_DISPLAY_GET_REGISTRY_OPCODE,
      .size = sizeof(req),
    },
    .new_id = wl_allocate_id(),
  };
#if DEBUG
  u8 *raw = (u8*)&req;
  dbg_print("wl_display_get_registry:");
  for (int i = 0; i < sizeof(req); i += 4) {
    dbg_print("\t%02x %02x %02x %02x", raw[i], raw[i+1], raw[i+2], raw[i+3]);
  }
#endif
  Assert(sizeof(req) == 12);
  Assert(req.msg.size == send(waystate.display, &req, req.msg.size, MSG_DONTWAIT));
  Info("wl_display#%u.get_registry: wl_registry=%u", WL_DISPLAY_OBJECT_ID, waystate.curr_id);
  return req.new_id;
}

fn u32 wl_registry_bind(u32 name, String8 interface, u32 version) {
  Scratch scratch = ScratchBegin(0, 0);
  usize padded_interface_len = align_forward(interface.size, 4);
  usize total_size = align_forward(sizeof(Wl_MessageHeader) + 4 * sizeof(u32) +
                                  padded_interface_len, 4);

  u8 *req = New(scratch.arena, u8, total_size);

  Wl_MessageHeader *header = (Wl_MessageHeader *)req;
  header->id = waystate.registry;
  header->opcode = WL_REGISTRY_BIND_OPCODE;
  header->size = total_size;

  u8 *body = req + sizeof(Wl_MessageHeader);
  *body = name; body += sizeof(u32);
  *body = padded_interface_len; body += sizeof(u32);
  memcopy(body, interface.str, interface.size);
  body += padded_interface_len;
  *body = version; body += sizeof(u32);
  u32 new_id = *body = wl_allocate_id();

#if DEBUG
  u8 *raw = req;
  dbg_print("wl_registry_bind:");
  for (int i = 0; i < total_size; i += 4) {
    dbg_print("\t%02x %02x %02x %02x", raw[i], raw[i+1], raw[i+2], raw[i+3]);
  }
#endif

  Assert(total_size == send(waystate.display, req, total_size, MSG_DONTWAIT));
  ScratchEnd(scratch);
  return new_id;
}

fn u32 wl_compositor_create_surface(void) {
  struct { Wl_MessageHeader msg; u32 new_id; } req = {
    .msg = {
      .id = waystate.wl_compositor,
      .opcode = WL_COMPOSITOR_CREATE_SURFACE_OPCODE,
      .size = sizeof(req),
    },
    .new_id = wl_allocate_id(),
  };
  Assert(sizeof(req) == send(waystate.display, &req, sizeof(req), MSG_DONTWAIT));
  return req.new_id;
}

fn u32 wl_xdg_wm_base_get_xdg_surface(u32 wl_surface) {
  struct { Wl_MessageHeader msg; u32 new_id; u32 surface_id; } req = {
    .msg = {
      .id = waystate.xdg_wm_base,
      .opcode = WL_XDG_WM_BASE_GET_XDG_SURFACE_OPCODE,
      .size = sizeof(req),
    },
    .new_id = wl_allocate_id(),
    .surface_id = wl_surface,
  };
  Assert(sizeof(req) == send(waystate.display, &req, sizeof(req), MSG_DONTWAIT));
  return req.new_id;
}

fn u32 wl_xdg_surface_get_toplevel(u32 xdg_surface) {
  struct { Wl_MessageHeader msg; u32 new_id; } req = {
    .msg = {
      .id = xdg_surface,
      .opcode = WL_XDG_SURFACE_GET_TOPLEVEL_OPCODE,
      .size = sizeof(req),
    },
    .new_id = wl_allocate_id(),
  };
  Assert(sizeof(req) == send(waystate.display, &req, sizeof(req), MSG_DONTWAIT));
  return req.new_id;
}

fn void wl_surface_commit(u32 wl_surface) {
  Wl_MessageHeader req = {
    .id = wl_surface,
    .opcode = WL_SURFACE_COMMIT_OPCODE,
    .size = sizeof(req),
  };
  Assert(sizeof(req) == send(waystate.display, &req, sizeof(req), MSG_DONTWAIT));
}

fn void wl_surface_attach(u32 wl_surface, u32 wl_buffer) {
  struct { Wl_MessageHeader header; u32 wl_buffer; u32 x; u32 y;} req = {
    .header = {
      .id = wl_surface,
      .opcode = WL_SURFACE_ATTACH_OPCODE,
      .size = sizeof(req),
    },
    .wl_buffer = wl_buffer,
    .x = 0,
    .y = 0,
  };
  Assert(sizeof(req) == send(waystate.display, &req, sizeof(req), MSG_DONTWAIT));
}

fn void wl_buffer_destroy(u32 wl_buffer) {
  Wl_MessageHeader req = {
    .id = wl_buffer,
    .opcode = WL_BUFFER_DESTROY_OPCODE,
    .size = sizeof(req),
  };
  Assert(sizeof(req) == send(waystate.display, &req, sizeof(req), MSG_DONTWAIT));
}

fn void wl_shm_pool_destroy(u32 wl_shm_pool) {
  Wl_MessageHeader req = {
    .id = wl_shm_pool,
    .opcode = WL_SHM_POOL_DESTROY_OPCODE,
    .size = sizeof(req),
  };
  Assert(sizeof(req) == send(waystate.display, &req, sizeof(req), MSG_DONTWAIT));
}

fn u32 wl_shm_create_pool(SharedMem shm, usize size) {
  struct { Wl_MessageHeader header; u32 new_id; u32 pool_size; } req = {
    .header = {
      .id = waystate.wl_shm,
      .opcode = WL_SHM_CREATE_POOL_OPCODE,
      .size = sizeof(req),
    },
    .new_id = wl_allocate_id(),
    .pool_size = size,
  };

  u8 buff[CMSG_SPACE(sizeof(shm.file_handle.h[0]))] = {};
  struct iovec io = {
    .iov_base = &req,
    .iov_len = req.header.size,
  };
  struct msghdr socket_msg = {
    .msg_iov = &io,
    .msg_iovlen = 1,
    .msg_control = buff,
    .msg_controllen = sizeof(buff),
  };

  struct cmsghdr *cmsg = CMSG_FIRSTHDR(&socket_msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN(sizeof(shm.file_handle.h[0]));

  *((i32*)CMSG_DATA(cmsg)) = shm.file_handle.h[0];
  socket_msg.msg_controllen = CMSG_SPACE(sizeof(shm.file_handle.h[0]));
  Assert(sendmsg(waystate.display, &socket_msg, 0) != -1);
  return req.new_id;
}

fn u32 wl_shm_pool_create_buffer(u32 wl_shm_pool, u32 width, u32 height, u32 stride) {
  struct { Wl_MessageHeader header;
           u32 new_id;
           u32 offset;
           u32 width;
           u32 height;
           u32 stride;
           u32 format;
  } req = {
    .header = {
      .id = wl_shm_pool,
      .opcode = WL_SHM_POOL_CREATE_BUFFER_OPCODE,
      .size = sizeof(req),
    },
    .new_id = wl_allocate_id(),
    .offset = 0,
    .width = width,
    .height = height,
    .stride = stride,
    .format = WL_FORMAT_XRGB8888,
  };

  Assert(sizeof(req) == send(waystate.display, &req, sizeof(req), MSG_DONTWAIT));
  return req.new_id;
}

fn void wl_shm_pool_resize(u32 wl_shm_pool, u32 memsize) {
  struct { Wl_MessageHeader header; u32 size;} req = {
    .header = {
      .id = wl_shm_pool,
      .opcode = WL_SHM_POOL_RESIZE_OPCODE,
      .size = sizeof(req),
    },
    .size = memsize,
  };
  Assert(sizeof(req) == send(waystate.display, &req, sizeof(req), MSG_DONTWAIT));
}

fn Wl_WindowEvent* wl_alloc_windowevent(void) {
  Wl_WindowEvent *event = 0;
  OS_MutexScope(waystate.events.mutex) {
    event = waystate.events.first;
    if (event) {
      QueuePop(waystate.events.first);
      memzero(event, sizeof(Wl_WindowEvent));
    } else {
      event = New(waystate.arena, Wl_WindowEvent);
    }
  }
  Assert(event);
  return event;
}

fn void wl_compositor_register_global(u32 name, String8 interface, u32 version) {
  Info("name: %d, interface: %.*s, version: %d", name, Strexpand(interface), version);

  if (str8_eq(interface, Strlit("wl_shm"))) {
    waystate.wl_shm = wl_registry_bind(name, interface, version);
  } else if (str8_eq(interface, Strlit("wl_compositor"))) {
    waystate.wl_compositor = wl_registry_bind(name, interface, version);
  } else if (str8_eq(interface, Strlit("xdg_wm_base"))) {
    waystate.xdg_wm_base = wl_registry_bind(name, interface, version);
  }
}

fn void wl_compositor_msg_parse(Wl_MessageHeader *header, u8 *body) {
  if (header->id == WL_DISPLAY_OBJECT_ID && header->opcode == WL_DISPLAY_EVENT_ERROR) {
    u32 object_id = *((u32*)body); body += sizeof(u32);
    u32 error_code = *(u32*)body; body += sizeof(u32);
    String8 message = {
      .size = *(u32*)body,
      .str = body + sizeof(u32),
    };
    Panic("id: %d, code: %d, message: %.*s", object_id, error_code, Strexpand(message));
  } else if (header->id == waystate.registry) {
    switch (header->opcode) {
    case WL_REGISTRY_EVENT_GLOBAL: {
      u32 name = *((u32*)body); body += sizeof(u32);
      String8 interface = {};
      interface.size = *(u32*)body - 1; body += sizeof(u32);
      interface.str = body; body += interface.size;
      body = (u8*)align_forward((usize)body, sizeof(u32));
      u32 version = *((u32*)body); body += sizeof(u32);
      wl_compositor_register_global(name, interface, version);
    } break;
    case WL_REGISTRY_EVENT_GLOBAL_REMOVE: {
      /* Info("WL_REGISTRY_EVENT_GLOBAL_REMOVE"); */
    } break;
    }
  } else if (header->id == waystate.xdg_wm_base) {
    switch (header->opcode) {
    case WL_XDG_WM_BASE_EVENT_PING: {
      struct { Wl_MessageHeader header; u32 serial; } req = {
        .header = {
          .id = waystate.xdg_wm_base,
          .opcode = WL_XDG_WM_BASE_PONG_OPCODE,
          .size = sizeof(req),
        },
        .serial = *(u32*)body,
      };
      Assert(req.header.size == send(waystate.display, &req, req.header.size, MSG_DONTWAIT));
    } break;
    }
  } else { // NOTE(lb): window specific events
    for (Wl_Window *curr = waystate.first_window; curr; curr = curr->next) {
      if (header->id == curr->xdg_surface) {
        switch (header->opcode) {
        case WL_XDG_SURFACE_EVENT_CONFIGURE: {
          curr->xdg_surface_acked = true;
          struct { Wl_MessageHeader header; u32 serial; } req = {
            .header = {
              .id = curr->xdg_surface,
              .opcode = WL_XDG_SURFACE_ACK_CONFIGURE_OPCODE,
              .size = sizeof(req),
            },
            .serial = *(u32*)body,
          };
          Assert(req.header.size == send(waystate.display, &req, req.header.size, MSG_DONTWAIT));
        } break;
        }
      } else if (header->id == curr->xdg_toplevel) {
        switch (header->opcode) {
        case WL_XDG_TOPLEVEL_EVENT_CONFIGURE: {
          i32 width = *(i32*)body;
          i32 height = *(i32*)(body + sizeof(i32));
          isize memsize = width * height * 4;
          if (memsize > 0) {
            Wl_WindowEvent *event = wl_alloc_windowevent();
            event->value.type = OS_EventType_Expose;
            event->value.expose.width = width;
            event->value.expose.height = height;
            OS_MutexScope(curr->events.mutex) {
              QueuePush(curr->events.first, curr->events.last, event);
              os_cond_signal(curr->events.condvar);
            }
          }
        } break;
        case WL_XDG_TOPLEVEL_EVENT_CLOSE: {
          Wl_WindowEvent *event = wl_alloc_windowevent();
          event->value.type = OS_EventType_Kill;
          OS_MutexScope(curr->events.mutex) {
            QueuePush(curr->events.first, curr->events.last, event);
            os_cond_signal(curr->events.condvar);
          }
        } break;
        }
      }
    }
  }
}

fn void wl_compositor_msg_advance_chunk(void) {
  memzero(waystate.msg_ringbuffer.bytes[waystate.msg_ringbuffer.head], WL_RINGBUFFER_BYTE_COUNT);
  waystate.msg_ringbuffer.head = (waystate.msg_ringbuffer.head + 1) % WL_RINGBUFFER_SIZE;
}

fn void wl_compositor_msg_dispatcher(void *_) {
  struct pollfd pfd = {
    .fd = waystate.display,
    .events = POLLIN,
  };

  for (;poll(&pfd, 1, -1);) {
    while (*(u64*)waystate.msg_ringbuffer.bytes[waystate.msg_ringbuffer.tail] == 0 &&
           poll(&pfd, 1, 0) > 0) {
      recv(waystate.display, waystate.msg_ringbuffer.bytes[waystate.msg_ringbuffer.tail],
           WL_RINGBUFFER_BYTE_COUNT, 0);
      waystate.msg_ringbuffer.tail = (waystate.msg_ringbuffer.tail + 1) % WL_RINGBUFFER_SIZE;
    }

    for (isize pos = 0, offset = waystate.msg_ringbuffer.head * WL_RINGBUFFER_BYTE_COUNT; 1;) {
      if (pos >= WL_RINGBUFFER_BYTE_COUNT) {
        pos -= WL_RINGBUFFER_BYTE_COUNT;
        wl_compositor_msg_advance_chunk();
      }

      Wl_MessageHeader *header = 0;
      u8 *bytes = &waystate.msg_ringbuffer.bytes[0][0] + offset;

      if (offset + sizeof(Wl_MessageHeader) >= WL_RINGBUFFER_CAPACITY) {
        // rest of header+message is wrapped around
        Scratch scratch = ScratchBegin(0, 0);

        isize approx_msg_size = 256;
        isize n_bytes_at_end = WL_RINGBUFFER_CAPACITY - offset;
        u8 *message = New(scratch.arena, u8, approx_msg_size);
        memcopy(message, bytes, n_bytes_at_end);
        memcopy(message + n_bytes_at_end, waystate.msg_ringbuffer.bytes[0], approx_msg_size - n_bytes_at_end);
        wl_compositor_msg_parse((Wl_MessageHeader*)message, message + sizeof(Wl_MessageHeader));

        isize n_bytes_wrapped = ((Wl_MessageHeader*)message)->size - n_bytes_at_end;
        pos = n_bytes_wrapped;
        offset = n_bytes_wrapped;

        wl_compositor_msg_advance_chunk();
        ScratchEnd(scratch);
      } else {
        header = (Wl_MessageHeader*)bytes;

        if (!header->size) {
          // no messages in this chunk
          wl_compositor_msg_advance_chunk();
          break;
        } else if (offset + header->size < WL_RINGBUFFER_CAPACITY) {
          // entire message is present
          wl_compositor_msg_parse(header, bytes + sizeof(Wl_MessageHeader));
          pos += header->size;
          offset += header->size;
        } else {
          // part of message body is wrapped around
          Scratch scratch = ScratchBegin(0, 0);
          u8 *message = New(scratch.arena, u8, header->size);

          isize n_bytes_wrapped = offset + header->size - WL_RINGBUFFER_CAPACITY;
          isize n_bytes_at_end = header->size - n_bytes_wrapped;
          memcopy(message, bytes, n_bytes_at_end);
          memcopy(message + n_bytes_at_end, waystate.msg_ringbuffer.bytes[0], n_bytes_wrapped);
          wl_compositor_msg_parse((Wl_MessageHeader*)message, message + sizeof(Wl_MessageHeader));

          pos = n_bytes_wrapped;
          offset = n_bytes_wrapped;

          wl_compositor_msg_advance_chunk();
          ScratchEnd(scratch);
        }
      }
    }

    Assert(waystate.msg_ringbuffer.head == waystate.msg_ringbuffer.tail);
  }
  Panic("wl_compositor_msg_dispatcher terminated");
}

// =============================================================================
// OS agnostic definitions
fn OS_Window os_window_open(String8 name, u32 x, u32 y, u32 width, u32 height) {
  Wl_Window *window = waystate.freelist_window;
  if (window) {
    memzero(window, sizeof(Wl_Window));
    StackPop(waystate.freelist_window);
  } else {
    window = New(waystate.arena, Wl_Window);
  }
  DLLPushBack(waystate.first_window, waystate.last_window, window);
  window->events.mutex = os_mutex_alloc();
  window->events.condvar = os_cond_alloc();

  {
    Scratch scratch = ScratchBegin(0, 0);
    StringStream ss = {};
    strstream_append_str(scratch.arena, &ss, Strlit("/"));
    strstream_append_str(scratch.arena, &ss, str8_from_i64(scratch.arena, Random(10000, 99999)));
    window->shm = os_sharedmem_open(str8_from_stream(scratch.arena, ss), width * height * 4,
                                    OS_acfRead | OS_acfWrite | OS_acfExecute);
    os_sharedmem_unlink_name(&window->shm);
    ScratchEnd(scratch);
  }

  window->wl_surface = wl_compositor_create_surface();
  window->xdg_surface = wl_xdg_wm_base_get_xdg_surface(window->wl_surface);
  window->xdg_toplevel = wl_xdg_surface_get_toplevel(window->xdg_surface);
  wl_surface_commit(window->wl_surface);

  while (!window->xdg_surface_acked);
  Assert(window->wl_surface && window->xdg_surface && window->xdg_toplevel);

  window->wl_shm_pool = wl_shm_create_pool(window->shm, width * height * 4);
  window->wl_buffer = wl_shm_pool_create_buffer(window->wl_shm_pool, width, height, width * 4);

  OS_Window res = {0};
  res.handle.h[0] = (u64)window;
  res.width = width;
  res.height = height;
  return res;
}

fn void os_window_show(OS_Window window_) {
  Wl_Window *window = (Wl_Window*)window_.handle.h[0];
  wl_surface_attach(window->wl_surface, window->wl_buffer);
  wl_surface_commit(window->wl_surface);
}

fn void os_window_hide(OS_Window handle) {}

fn void os_window_close(OS_Window window_) {}

fn void os_window_swapBuffers(OS_Window handle) {}

fn void os_window_render(OS_Window window_, void *mem) {
  Wl_Window *window = (Wl_Window*)window_.handle.h[0];
  usize memsize = window_.width * window_.height * 4;
  if (memsize > window->shm.prop.size) {
    os_sharedmem_resize(&window->shm, memsize);
    wl_shm_pool_resize(window->wl_shm_pool, memsize);
  }

  if (window->wl_buffer) { wl_buffer_destroy(window->wl_buffer); }
  window->wl_buffer = wl_shm_pool_create_buffer(window->wl_shm_pool,
                                                window_.width,
                                                window_.height,
                                                window_.width * 4);
  memcopy(window->shm.content, mem, memsize);
  wl_surface_attach(window->wl_surface, window->wl_buffer);
  wl_surface_commit(window->wl_surface);
}

fn OS_Event os_window_get_event(OS_Window window_) {
  Wl_Window *window = (Wl_Window*)window_.handle.h[0];
  OS_Event res = {0};

  Wl_WindowEvent *event = 0;
  OS_MutexScope(window->events.mutex) {
    event = window->events.first;
    if (window->events.first) {
      memcopy(&res, window->events.first, sizeof(OS_Event));
      QueuePop(window->events.first);
    }
  }
  if (event) {
    OS_MutexScope(waystate.events.mutex) {
      QueuePush(waystate.events.first, waystate.events.first, event);
    }
  }

  return res;
}

fn OS_Event os_window_wait_event(OS_Window window_) {
  Wl_Window *window = (Wl_Window*)window_.handle.h[0];
  OS_Event res = {0};

  Wl_WindowEvent *event = 0;
  OS_MutexScope(window->events.mutex) {
    for (; !event; event = window->events.first) {
      os_cond_wait(window->events.condvar, window->events.mutex, 0);
    }
    memcopy(&res, window->events.first, sizeof(OS_Event));
    QueuePop(window->events.first);
  }
  OS_MutexScope(waystate.events.mutex) {
    QueuePush(waystate.events.first, waystate.events.first, event);
  }

  return res;
}

fn String8 os_keyname_from_event(Arena *arena, OS_Event event) {
  String8 res = {0};
  return res;
}

#if USING_OPENGL
fn void opengl_init(OS_Window handle) {}

fn void opengl_deinit(OS_Window handle) {}

fn void opengl_make_current(OS_Window handle) {}
#endif
