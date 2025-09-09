#ifndef RHI_OPENGL_CORE_H
#define RHI_OPENGL_CORE_H

fn RHI_Handle rhi_gl_window_init(OS_Handle os_window);
fn void rhi_gl_window_deinit(RHI_Handle handle);
fn void rhi_gl_window_make_current(RHI_Handle handle);
fn void rhi_gl_window_commit(RHI_Handle handle);

internal void rhi_gl_window_resize(RHI_Handle context, u32 width, u32 height);

#endif
