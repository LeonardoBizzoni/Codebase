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
} RHI_BufferLayoutElement;

typedef struct {
  RHI_BufferLayoutElement *elements;
  i32 count;
} RHI_BufferLayout;

typedef struct {
  Vec3F32 position;
  Vec3F32 direction_front;
  Vec3F32 direction_right;
  Vec3F32 direction_up;
  Vec3F32 direction_up_world;

  f32 pitch;
  f32 yaw;

  f32 mouse_sensitivity;
  f32 mouse_zoom_factor;

  // NOTE(lb): for keyboard control
  f32 speed_vertical;
  f32 speed_movement;
  f32 speed_rotation;
} RHI_Camera;

fn void rhi_camera_update(RHI_Camera *camera);

global const i32 rhi_shadertype_map_size[] = {
  [RHI_ShaderDataType_Float]   = sizeof(f32),
  [RHI_ShaderDataType_Int]     = sizeof(i32),
  [RHI_ShaderDataType_Bool]    = sizeof(bool),
  [RHI_ShaderDataType_Vec2F32] = 2 * sizeof(f32),
  [RHI_ShaderDataType_Vec2I32] = 2 * sizeof(i32),
  [RHI_ShaderDataType_Vec3F32] = 3 * sizeof(f32),
  [RHI_ShaderDataType_Vec3I32] = 3 * sizeof(i32),
  [RHI_ShaderDataType_Vec4F32] = 4 * sizeof(f32),
  [RHI_ShaderDataType_Vec4I32] = 4 * sizeof(i32),
  [RHI_ShaderDataType_Mat3F32] = 3 * 3 * sizeof(f32),
  [RHI_ShaderDataType_Mat4F32] = 4 * 4 * sizeof(f32),
};

global const i32 rhi_shadertype_map_element_count[] = {
  [RHI_ShaderDataType_Float]   = 1,
  [RHI_ShaderDataType_Int]     = 1,
  [RHI_ShaderDataType_Bool]    = 1,
  [RHI_ShaderDataType_Vec2F32] = 2,
  [RHI_ShaderDataType_Vec2I32] = 2,
  [RHI_ShaderDataType_Vec3F32] = 3,
  [RHI_ShaderDataType_Vec3I32] = 3,
  [RHI_ShaderDataType_Vec4F32] = 4,
  [RHI_ShaderDataType_Vec4I32] = 4,
  [RHI_ShaderDataType_Mat3F32] = 3 * 3,
  [RHI_ShaderDataType_Mat4F32] = 4 * 4,
};

fn RHI_Handle rhi_context_create(Arena *arena, OS_Handle window);
fn void rhi_context_destroy(RHI_Handle hcontext);

fn RHI_Handle rhi_buffer_alloc(Arena *arena, RHI_Handle hcontext,
                               i32 size, RHI_BufferType type);
fn void rhi_buffer_host_send(RHI_Handle hcontext, RHI_Handle hbuffer,
                             const void *data, i32 size);
fn void rhi_buffer_copy(RHI_Handle hcontext, RHI_Handle htarget_buffer,
                        RHI_Handle hsource_buffer, i32 size);
fn void rhi_buffer_set_layout(RHI_Handle hcontext, RHI_Handle hbuffer, RHI_BufferLayout layout);

fn RHI_Handle rhi_shader_from_file(Arena *arena, RHI_Handle hcontext,
                                   String8 vertex_shader_path,
                                   String8 fragment_shader_path);

fn RHI_Handle rhi_pipeline_create(Arena *arena, RHI_Handle hcontext,
                                  RHI_Handle hshader, RHI_BufferLayout layout);
fn void rhi_pipeline_destroy(RHI_Handle hcontext, RHI_Handle hpipeline);

internal void rhi_init(void);
internal void rhi_deinit(void);

#endif
