#ifndef RHI_CORE_H
#define RHI_CORE_H

typedef struct {
  u64 h[1];
} RHI_Handle;

typedef u8 RHI_ShaderType;
enum {
  RHI_ShaderType_None,
  RHI_ShaderType_Vertex,
  RHI_ShaderType_Pixel,
};

typedef u8 RHI_BufferType;
enum {
  RHI_BufferType_None,
  RHI_BufferType_Staging,
  RHI_BufferType_Vertex,
  RHI_BufferType_Index,
};

typedef u8 RHI_ShaderDataType;
enum {
  RHI_ShaderDataType_Float,
  RHI_ShaderDataType_Int,
  RHI_ShaderDataType_Bool,

  RHI_ShaderDataType_Vec2F32,
  RHI_ShaderDataType_Vec2I32,
  RHI_ShaderDataType_Vec3F32,
  RHI_ShaderDataType_Vec3I32,
  RHI_ShaderDataType_Vec4F32,
  RHI_ShaderDataType_Vec4I32,

  RHI_ShaderDataType_Mat3F32,
  RHI_ShaderDataType_Mat4F32,
};

typedef struct {
  RHI_ShaderDataType type;
  String8 name;
  bool to_normalize;
} RHI_BufferElement;

fn RHI_Handle rhi_context_create(Arena *arena, OS_Handle window);
fn void rhi_context_destroy(RHI_Handle hcontext);

fn RHI_Handle rhi_buffer_alloc(Arena *arena, RHI_Handle hcontext,
                               i32 size, RHI_BufferType type);
fn void rhi_buffer_host_send(RHI_Handle hcontext, RHI_Handle hbuffer,
                             const void *data, i32 size);
fn void rhi_buffer_copy(RHI_Handle hcontext, RHI_Handle htarget_buffer,
                        RHI_Handle hsource_buffer, i32 size);

fn RHI_Handle rhi_shader_from_file(Arena *arena, RHI_Handle hcontext,
                                   String8 vertex_shader_path,
                                   String8 fragment_shader_path);

#define rhi_buffer_set_layout(HContext, HBuffer, Layout) \
  _rhi_buffer_set_layout((HContext), (HBuffer), (Layout), Arrsize(Layout))

internal void rhi_init(void);
internal void rhi_deinit(void);

internal void _rhi_buffer_set_layout(RHI_Handle hcontext, RHI_Handle hbuffer,
                                     RHI_BufferElement *layout, u32 layout_size);

#endif
